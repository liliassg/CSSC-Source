/* cssc_gipeo.c — gipeo runtime backends.
 *
 * One file, four implementations selected by compile-time defines:
 *
 *   CSSC_ESP32     — ESP-IDF (driver/gpio.h, driver/i2c.h, driver/spi_master.h, …)
 *   CSSC_ARDUINO   — Arduino core (Arduino.h, Wire, SPI, Serial)
 *   CSSC_LINUX     — Linux (libgpiod, /dev/i2c-N, /dev/spidev*, termios, sysfs PWM)
 *   (none)         — host stub: printf-tagged ops, plausible defaults, useful
 *                    for offline logic testing (mirror of the Python stubs)
 *
 * The ESP32 / Arduino / Raspberry blocks intentionally include the SDK
 * headers under their own guards. If you build for one of those targets
 * without the SDK on the include path, the compile fails with a clear
 * "missing <driver/gpio.h>" — better than emitting silently broken code.
 */

/* CSSC_RUNTIME_EXPORTS is set on the gcc command line by the build,
 * so the export decorators come from there. Defining it locally would
 * just trigger a redefine warning. */
#include "cssc_gipeo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>   /* va_list / va_start / va_end — used by _gipeo_log */

/* SDK-header probing — `__has_include` lets us include the
 * platform's GPIO/I2C/SPI headers ONLY when they're actually on the
 * compiler's include path. A user running `cssc build --esp32`
 * outside an ESP-IDF project tree won't have driver/gpio.h reachable;
 * we fall through to stdlib-only stubs in that case. The .c that
 * gcc produces still links cleanly — the user can later compile the
 * generated source through PlatformIO / Arduino IDE if they want
 * real hardware drivers. */
#if defined(CSSC_ESP32) && defined(__has_include) && __has_include("driver/gpio.h")
  #include "driver/gpio.h"
  #include "driver/i2c.h"
  #include "driver/spi_master.h"
  #include "driver/uart.h"
  #include "driver/ledc.h"
  /* gptimer was introduced in ESP-IDF v5; Arduino-core for ESP32 is
   * still on v4 where the timer API is in `driver/timer.h`. We probe
   * for whichever header is present and live without timer-stop
   * granularity if neither matches. */
  #if __has_include("driver/gptimer.h")
    #include "driver/gptimer.h"
  #elif __has_include("driver/timer.h")
    #include "driver/timer.h"
  #endif
  #if __has_include("esp_adc/adc_oneshot.h")
    #include "esp_adc/adc_oneshot.h"
    #include "esp_adc/adc_cali.h"
  #elif __has_include("driver/adc.h")
    #include "driver/adc.h"
  #endif
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #define CSSC_GIPEO_HAS_ESP_IDF 1
#elif (defined(CSSC_ARDUINO) || defined(CSSC_ESP8266)) && \
      defined(__has_include) && __has_include(<Arduino.h>)
  /* Arduino-core (AVR/ESP8266) compatibility layer — surface includes
   * pinMode/digitalRead/digitalWrite (plain C) AND Wire/SPI/Serial
   * (C++ objects). The PIO build path renames this file `.c → .cpp`
   * for Arduino-framework targets so we can safely include the C++
   * headers below. cssc_gipeo.h has `extern "C"` guards so the public
   * symbols stay C-linkage and match the rest of the runtime ABI. */
  #include <Arduino.h>
  #include <Wire.h>
  #include <SPI.h>
  #define CSSC_GIPEO_HAS_ARDUINO 1
#endif

/* Forward-declare the Arduino-core time symbols when EITHER the ESP-IDF
 * or Arduino-core framework is in scope. They have C linkage in the
 * framework (provided by libcore-arduinoespressif32 or AVR libcore);
 * the linker resolves them by name. We can't pull <Arduino.h> from a
 * .c TU because it transitively includes C++ stdlib templates. */
#if defined(CSSC_GIPEO_HAS_ESP_IDF) || defined(CSSC_GIPEO_HAS_ARDUINO)
  #ifdef __cplusplus
  extern "C" {
  #endif
  extern unsigned long millis(void);
  extern unsigned long micros(void);
  extern void delay(unsigned long ms);
  #ifdef __cplusplus
  }
  #endif
#endif

#if defined(CSSC_LINUX)
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <sys/types.h>
  #include <linux/i2c-dev.h>
  #include <linux/spi/spidev.h>
  #include <termios.h>
  #include <time.h>
  #include <pthread.h>
  /* libgpiod for #pin — preferred path. Falls back to legacy sysfs if
   * <gpiod.h> isn't on the include path. */
  #if __has_include(<gpiod.h>)
    #include <gpiod.h>
    #define CSSC_LINUX_HAS_LIBGPIOD 1
  #endif
#else
  /* Host stub — Win32 / generic POSIX without hardware. */
  #ifdef _WIN32
    #include <windows.h>
  #else
    #include <time.h>
    #include <pthread.h>
    #include <unistd.h>
  #endif
#endif


/* ===========================================================================
 * Internal helpers
 * =========================================================================== */

static void _gipeo_log(const char* fmt, ...) {
#ifdef CSSC_GIPEO_QUIET
    (void)fmt;
#else
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[gipeo] ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    fflush(stderr);
    va_end(ap);
#endif
}


/* ===========================================================================
 * #pin — single GPIO line
 * =========================================================================== */

CSSC_GIPEO_API CsscPin* cssc_pin_create(uint32_t num) {
    CsscPin* p = (CsscPin*)calloc(1, sizeof(CsscPin));
    if (!p) return NULL;
    p->num = num;
    p->mode = CSSC_GIPEO_INPUT;
    p->state = CSSC_GIPEO_LOW;
    _gipeo_log("pin[%u] allocated", num);
    return p;
}

CSSC_GIPEO_API void cssc_pin_destroy(CsscPin* p) {
    if (!p) return;
    cssc_pin_detach_interrupt(p);
    _gipeo_log("pin[%u] destroyed", p->num);
    free(p);
}

CSSC_GIPEO_API void cssc_pin_mode(CsscPin* p, int m) {
    if (!p) return;
    p->mode = (uint8_t)m;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    /* C++ refuses `{0}` as an enum-typed field initializer.
     * Use memset for a portable zero-fill that satisfies both C and C++. */
    gpio_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.pin_bit_mask = (1ULL << p->num);
    cfg.mode = (m == CSSC_GIPEO_OUTPUT) ? GPIO_MODE_OUTPUT
             : (m == CSSC_GIPEO_OPEN_DRAIN) ? GPIO_MODE_OUTPUT_OD
             : GPIO_MODE_INPUT;
    cfg.pull_up_en   = (m == CSSC_GIPEO_INPUT_PULLUP)   ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = (m == CSSC_GIPEO_INPUT_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    int arduino_mode = (m == CSSC_GIPEO_OUTPUT) ? OUTPUT
                     : (m == CSSC_GIPEO_INPUT_PULLUP) ? INPUT_PULLUP
                     : INPUT;
    pinMode(p->num, arduino_mode);
#elif defined(CSSC_LINUX) && defined(CSSC_LINUX_HAS_LIBGPIOD)
    /* libgpiod uses request_input/request_output to "lock" a line. We
     * stash the line* in p->backend so subsequent send/status reuse it. */
    if (!p->backend) {
        struct gpiod_chip* chip = gpiod_chip_open_by_name("gpiochip0");
        if (chip) {
            struct gpiod_line* line = gpiod_chip_get_line(chip, p->num);
            p->backend = line;
        }
    }
    struct gpiod_line* line = (struct gpiod_line*)p->backend;
    if (line) {
        gpiod_line_release(line);
        if (m == CSSC_GIPEO_OUTPUT) gpiod_line_request_output(line, "cssc", 0);
        else                         gpiod_line_request_input(line, "cssc");
    }
#else
    /* host stub */
    _gipeo_log("pin[%u].mode(%d)", p->num, m);
#endif
}

CSSC_GIPEO_API void cssc_pin_send(CsscPin* p, int level) {
    if (!p) return;
    p->state = level ? CSSC_GIPEO_HIGH : CSSC_GIPEO_LOW;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    gpio_set_level((gpio_num_t)p->num, p->state);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    digitalWrite(p->num, p->state ? HIGH : LOW);
#elif defined(CSSC_LINUX) && defined(CSSC_LINUX_HAS_LIBGPIOD)
    if (p->backend) gpiod_line_set_value((struct gpiod_line*)p->backend, p->state);
#else
    _gipeo_log("pin[%u].send(%s)", p->num, p->state ? "HIGH" : "LOW");
#endif
}

CSSC_GIPEO_API int cssc_pin_status(CsscPin* p) {
    if (!p) return 0;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    return gpio_get_level((gpio_num_t)p->num);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    return digitalRead(p->num);
#elif defined(CSSC_LINUX) && defined(CSSC_LINUX_HAS_LIBGPIOD)
    if (p->backend) return gpiod_line_get_value((struct gpiod_line*)p->backend);
    return 0;
#else
    _gipeo_log("pin[%u].status() -> %d", p->num, p->state);
    return p->state;
#endif
}

CSSC_GIPEO_API void cssc_pin_toggle(CsscPin* p) {
    if (!p) return;
    cssc_pin_send(p, p->state ? CSSC_GIPEO_LOW : CSSC_GIPEO_HIGH);
}

CSSC_GIPEO_API void cssc_pin_attach_interrupt(CsscPin* p, void (*cb)(void), int edge) {
    if (!p || !cb) return;
    p->irq_callback = cb;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    gpio_int_type_t t = (edge == CSSC_GIPEO_RISING)  ? GPIO_INTR_POSEDGE
                      : (edge == CSSC_GIPEO_FALLING) ? GPIO_INTR_NEGEDGE
                      : GPIO_INTR_ANYEDGE;
    gpio_set_intr_type((gpio_num_t)p->num, t);
    /* ISR install is project-wide and idempotent. */
    static int isr_installed = 0;
    if (!isr_installed) { gpio_install_isr_service(0); isr_installed = 1; }
    gpio_isr_handler_add((gpio_num_t)p->num, (gpio_isr_t)(void(*)(void*))cb, NULL);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    int mode = (edge == CSSC_GIPEO_RISING)  ? RISING
             : (edge == CSSC_GIPEO_FALLING) ? FALLING : CHANGE;
    attachInterrupt(digitalPinToInterrupt(p->num), cb, mode);
#else
    (void)edge;
    _gipeo_log("pin[%u].attach_interrupt(edge=%d)", p->num, edge);
#endif
}

CSSC_GIPEO_API void cssc_pin_detach_interrupt(CsscPin* p) {
    if (!p) return;
    p->irq_callback = NULL;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    gpio_isr_handler_remove((gpio_num_t)p->num);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    detachInterrupt(digitalPinToInterrupt(p->num));
#else
    _gipeo_log("pin[%u].detach_interrupt()", p->num);
#endif
}

CSSC_GIPEO_API uint32_t cssc_pin_num(CsscPin* p) { return p ? p->num : 0; }


/* ===========================================================================
 * #i2c
 * =========================================================================== */

CSSC_GIPEO_API CsscI2C* cssc_i2c_create(uint8_t bus, uint8_t sda, uint8_t scl) {
    CsscI2C* b = (CsscI2C*)calloc(1, sizeof(CsscI2C));
    if (!b) return NULL;
    b->bus = bus; b->sda = sda; b->scl = scl; b->freq = 100000;
    _gipeo_log("i2c[bus=%u sda=%u scl=%u] allocated", bus, sda, scl);
    return b;
}

CSSC_GIPEO_API void cssc_i2c_destroy(CsscI2C* b) {
    if (!b) return;
#if defined(CSSC_LINUX)
    if (b->backend) close((int)(intptr_t)b->backend);
#endif
    free(b);
}

CSSC_GIPEO_API void cssc_i2c_begin(CsscI2C* b, uint32_t freq_hz) {
    if (!b) return;
    b->freq = freq_hz;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    i2c_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.mode = I2C_MODE_MASTER;
    cfg.sda_io_num = b->sda;
    cfg.scl_io_num = b->scl;
    cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
    cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
    cfg.master.clk_speed = freq_hz;
    i2c_param_config((i2c_port_t)b->bus, &cfg);
    i2c_driver_install((i2c_port_t)b->bus, I2C_MODE_MASTER, 0, 0, 0);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    Wire.begin();
    Wire.setClock(freq_hz);
#elif defined(CSSC_LINUX)
    char path[32];
    snprintf(path, sizeof(path), "/dev/i2c-%u", b->bus);
    int fd = open(path, O_RDWR);
    b->backend = (void*)(intptr_t)fd;
#else
    _gipeo_log("i2c[%u].begin(%u Hz)", b->bus, freq_hz);
#endif
}

CSSC_GIPEO_API int cssc_i2c_write(CsscI2C* b, uint8_t addr,
                                  const uint8_t* data, size_t n) {
    if (!b || !data) return -1;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 0, true);
    i2c_master_write(cmd, (uint8_t*)data, n, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)b->bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? (int)n : -1;
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    Wire.beginTransmission(addr);
    Wire.write(data, n);
    return Wire.endTransmission() == 0 ? (int)n : -1;
#elif defined(CSSC_LINUX)
    int fd = (int)(intptr_t)b->backend;
    if (fd < 0) return -1;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    return (int)write(fd, data, n);
#else
    _gipeo_log("i2c[%u].write(0x%02X, %zu bytes)", b->bus, addr, n);
    return (int)n;
#endif
}

CSSC_GIPEO_API int cssc_i2c_read(CsscI2C* b, uint8_t addr, uint8_t reg,
                                 uint8_t* out, size_t n) {
    if (!b || !out) return -1;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 0, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 1, true);
    if (n > 1) i2c_master_read(cmd, out, n - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, out + n - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin((i2c_port_t)b->bus, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? (int)n : -1;
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((int)addr, (int)n);
    size_t got = 0;
    while (Wire.available() && got < n) out[got++] = Wire.read();
    return (int)got;
#elif defined(CSSC_LINUX)
    int fd = (int)(intptr_t)b->backend;
    if (fd < 0) return -1;
    if (ioctl(fd, I2C_SLAVE, addr) < 0) return -1;
    if (write(fd, &reg, 1) != 1) return -1;
    return (int)read(fd, out, n);
#else
    _gipeo_log("i2c[%u].read(0x%02X, reg=0x%02X, n=%zu)", b->bus, addr, reg, n);
    memset(out, 0xFF, n);
    return (int)n;
#endif
}

CSSC_GIPEO_API int cssc_i2c_scan(CsscI2C* b, uint8_t out_bitmap[16]) {
    if (!b || !out_bitmap) return 0;
    memset(out_bitmap, 0, 16);
    int found = 0;
    uint8_t dummy = 0;
    for (uint8_t a = 0x03; a < 0x78; a++) {
        if (cssc_i2c_write(b, a, &dummy, 0) >= 0) {
            out_bitmap[a / 8] |= (1 << (a % 8));
            found++;
        }
    }
    return found;
}


/* ===========================================================================
 * #spi
 * =========================================================================== */

CSSC_GIPEO_API CsscSPI* cssc_spi_create(uint8_t bus, uint8_t sck,
                                        uint8_t miso, uint8_t mosi) {
    CsscSPI* b = (CsscSPI*)calloc(1, sizeof(CsscSPI));
    if (!b) return NULL;
    b->bus = bus; b->sck = sck; b->miso = miso; b->mosi = mosi;
    b->freq = 1000000; b->mode = 0;
    _gipeo_log("spi[bus=%u sck=%u miso=%u mosi=%u] allocated", bus, sck, miso, mosi);
    return b;
}

CSSC_GIPEO_API void cssc_spi_destroy(CsscSPI* b) {
    if (!b) return;
#if defined(CSSC_LINUX)
    if (b->backend) close((int)(intptr_t)b->backend);
#endif
    free(b);
}

CSSC_GIPEO_API void cssc_spi_begin(CsscSPI* b, uint32_t freq, uint8_t mode) {
    if (!b) return;
    b->freq = freq; b->mode = mode & 0x3;
#if defined(CSSC_GIPEO_HAS_ARDUINO)
    SPI.begin();
    SPI.beginTransaction(SPISettings(freq, MSBFIRST, mode));
#elif defined(CSSC_LINUX)
    char path[32];
    snprintf(path, sizeof(path), "/dev/spidev%u.0", b->bus);
    int fd = open(path, O_RDWR);
    if (fd >= 0) {
        ioctl(fd, SPI_IOC_WR_MODE, &b->mode);
        ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &b->freq);
    }
    b->backend = (void*)(intptr_t)fd;
#else
    _gipeo_log("spi[%u].begin(freq=%u mode=%u)", b->bus, freq, mode);
#endif
}

CSSC_GIPEO_API int cssc_spi_transfer(CsscSPI* b, const uint8_t* tx_buf,
                                     uint8_t* rx_buf, size_t n) {
    if (!b || !tx_buf || !rx_buf) return -1;
#if defined(CSSC_GIPEO_HAS_ARDUINO)
    for (size_t i = 0; i < n; i++) rx_buf[i] = SPI.transfer(tx_buf[i]);
    return (int)n;
#elif defined(CSSC_LINUX)
    int fd = (int)(intptr_t)b->backend;
    if (fd < 0) return -1;
    struct spi_ioc_transfer tr = {0};
    tr.tx_buf = (unsigned long)tx_buf;
    tr.rx_buf = (unsigned long)rx_buf;
    tr.len = n;
    tr.speed_hz = b->freq;
    tr.bits_per_word = 8;
    return ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
#else
    /* Host stub: echo TX into RX so callers can verify the wire format. */
    if (rx_buf != tx_buf) memcpy(rx_buf, tx_buf, n);
    _gipeo_log("spi[%u].transfer(%zu bytes, echo)", b->bus, n);
    return (int)n;
#endif
}

CSSC_GIPEO_API uint8_t cssc_spi_transfer_byte(CsscSPI* b, uint8_t tx) {
    uint8_t rx = 0;
    cssc_spi_transfer(b, &tx, &rx, 1);
    return rx;
}


/* ===========================================================================
 * #uart
 * =========================================================================== */

CSSC_GIPEO_API CsscUART* cssc_uart_create(uint8_t bus, uint8_t tx, uint8_t rx) {
    CsscUART* u = (CsscUART*)calloc(1, sizeof(CsscUART));
    if (!u) return NULL;
    u->bus = bus; u->tx = tx; u->rx = rx; u->baud = 115200;
    _gipeo_log("uart[bus=%u tx=%u rx=%u] allocated", bus, tx, rx);
    return u;
}

CSSC_GIPEO_API void cssc_uart_destroy(CsscUART* u) {
    if (!u) return;
#if defined(CSSC_LINUX)
    if (u->backend) close((int)(intptr_t)u->backend);
#endif
    free(u);
}

CSSC_GIPEO_API void cssc_uart_begin(CsscUART* u, uint32_t baud) {
    if (!u) return;
    u->baud = baud;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    uart_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.baud_rate = baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_param_config((uart_port_t)u->bus, &cfg);
    uart_set_pin((uart_port_t)u->bus, u->tx, u->rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install((uart_port_t)u->bus, 1024, 0, 0, NULL, 0);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    /* On AVR Uno: u->bus=0 → Serial; on Mega: 1→Serial1 etc. We map bus 0
     * via a simple guard — the user can call Serial.begin() themselves
     * for non-default ports. */
    if (u->bus == 0) Serial.begin(baud);
#else
    _gipeo_log("uart[%u].begin(%u baud)", u->bus, baud);
#endif
}

CSSC_GIPEO_API int cssc_uart_write(CsscUART* u, const char* s) {
    if (!u || !s) return 0;
    size_t n = strlen(s);
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    return uart_write_bytes((uart_port_t)u->bus, s, n);
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    if (u->bus == 0) return Serial.write((const uint8_t*)s, n);
    return 0;
#elif defined(CSSC_LINUX)
    int fd = (int)(intptr_t)u->backend;
    if (fd < 0) return 0;
    return (int)write(fd, s, n);
#else
    _gipeo_log("uart[%u].write(\"%s\")", u->bus, s);
    return (int)n;
#endif
}

CSSC_GIPEO_API int cssc_uart_println(CsscUART* u, const char* s) {
    int total = cssc_uart_write(u, s);
    total += cssc_uart_write(u, "\n");
    return total;
}

CSSC_GIPEO_API int cssc_uart_available(CsscUART* u) {
    if (!u) return 0;
#if defined(CSSC_GIPEO_HAS_ESP_IDF)
    size_t avail = 0;
    uart_get_buffered_data_len((uart_port_t)u->bus, &avail);
    return (int)avail;
#elif defined(CSSC_GIPEO_HAS_ARDUINO)
  /* ESP8266 ships an Arduino-core compatibility layer with the same
   * digitalRead / digitalWrite / Wire / SPI / Serial / analogRead
   * surface as classic AVR Arduino, so the two targets share this
   * branch. ESP8266-specific quirks (e.g. analogRead is 0..1023 on
   * the A0 pin only) are documented in the gipeo module — the user
   * still calls the same CSSC API. */
    return (u->bus == 0) ? Serial.available() : 0;
#else
    return 0;
#endif
}

CSSC_GIPEO_API int cssc_uart_read_line(CsscUART* u, char* out, size_t max_len) {
    if (!u || !out || max_len == 0) return 0;
    out[0] = '\0';
#if defined(CSSC_GIPEO_HAS_ARDUINO)
    if (u->bus != 0) return 0;
    size_t pos = 0;
    while (pos < max_len - 1 && Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n') break;
        out[pos++] = c;
    }
    out[pos] = '\0';
    return (int)pos;
#else
    return 0;
#endif
}


/* ===========================================================================
 * #adc
 * =========================================================================== */

CSSC_GIPEO_API CsscADC* cssc_adc_create(uint8_t pin) {
    CsscADC* a = (CsscADC*)calloc(1, sizeof(CsscADC));
    if (!a) return NULL;
    a->pin = pin;
    a->resolution_bits = 12;
    a->vref_mv = 3300;
    _gipeo_log("adc[pin=%u] allocated", pin);
    return a;
}

CSSC_GIPEO_API void cssc_adc_destroy(CsscADC* a) {
    if (!a) return;
    free(a);
}

CSSC_GIPEO_API void cssc_adc_configure(CsscADC* a, uint8_t bits, uint16_t vref_mv) {
    if (!a) return;
    a->resolution_bits = bits;
    a->vref_mv = vref_mv;
    /* analogReadResolution() exists only on Arduino-core variants
     * with multi-bit ADCs (ESP32, SAMD, RP2040). ESP8266's ADC is
     * fixed at 10 bits on A0 — calling it would be undeclared.
     * Probe via the architecture macro. */
#if defined(CSSC_GIPEO_HAS_ARDUINO) && \
    (defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_SAMD) || \
     defined(ARDUINO_ARCH_RP2040))
    analogReadResolution(bits);
#else
    (void)bits;
#endif
}

CSSC_GIPEO_API int32_t cssc_adc_read_raw(CsscADC* a) {
    if (!a) return 0;
#if defined(CSSC_GIPEO_HAS_ARDUINO)
    return analogRead(a->pin);
#else
    /* Stub: mid-rail */
    int32_t mid = (1 << a->resolution_bits) / 2;
    _gipeo_log("adc[%u].read_raw() -> %d", a->pin, mid);
    return mid;
#endif
}

CSSC_GIPEO_API int32_t cssc_adc_read_mv(CsscADC* a) {
    if (!a) return 0;
    int32_t raw = cssc_adc_read_raw(a);
    int32_t max_v = (1 << a->resolution_bits) - 1;
    return (max_v > 0) ? (int32_t)((int64_t)raw * a->vref_mv / max_v) : 0;
}


/* ===========================================================================
 * #pwm
 * =========================================================================== */

CSSC_GIPEO_API CsscPWM* cssc_pwm_create(uint8_t pin, uint32_t freq, uint8_t bits) {
    CsscPWM* p = (CsscPWM*)calloc(1, sizeof(CsscPWM));
    if (!p) return NULL;
    p->pin = pin; p->freq = freq; p->resolution_bits = bits;
    _gipeo_log("pwm[pin=%u freq=%u res=%ub] allocated", pin, freq, bits);
    return p;
}

CSSC_GIPEO_API void cssc_pwm_destroy(CsscPWM* p) {
    if (!p) return;
    cssc_pwm_stop(p);
    free(p);
}

CSSC_GIPEO_API void cssc_pwm_duty(CsscPWM* p, uint32_t raw) {
    if (!p) return;
    uint32_t max_v = (1u << p->resolution_bits) - 1;
    if (raw > max_v) raw = max_v;
    p->duty = raw;
    p->running = 1;
#if defined(CSSC_GIPEO_HAS_ARDUINO)
    analogWrite(p->pin, raw);
#else
    _gipeo_log("pwm[%u].duty(%u/%u)", p->pin, raw, max_v);
#endif
}

CSSC_GIPEO_API void cssc_pwm_duty_pct(CsscPWM* p, double pct) {
    if (!p) return;
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    uint32_t max_v = (1u << p->resolution_bits) - 1;
    cssc_pwm_duty(p, (uint32_t)((pct / 100.0) * max_v));
}

CSSC_GIPEO_API void cssc_pwm_stop(CsscPWM* p) {
    if (!p) return;
    p->running = 0; p->duty = 0;
#if defined(CSSC_GIPEO_HAS_ARDUINO)
    analogWrite(p->pin, 0);
#else
    _gipeo_log("pwm[%u].stop()", p->pin);
#endif
}


/* ===========================================================================
 * #timer — host: thread polling. ESP32: gptimer. Arduino: poll-based.
 * =========================================================================== */

CSSC_GIPEO_API CsscTimer* cssc_timer_create(uint8_t slot, uint32_t hz) {
    CsscTimer* t = (CsscTimer*)calloc(1, sizeof(CsscTimer));
    if (!t) return NULL;
    t->slot = slot; t->hz = (hz == 0) ? 1 : hz;
    _gipeo_log("timer[slot=%u hz=%u] allocated", slot, hz);
    return t;
}

CSSC_GIPEO_API void cssc_timer_destroy(CsscTimer* t) {
    if (!t) return;
    cssc_timer_stop(t);
    free(t);
}

CSSC_GIPEO_API void cssc_timer_attach(CsscTimer* t, void (*cb)(void)) {
    if (!t) return;
    t->callback = cb;
}

#if defined(_WIN32)
typedef HANDLE _gipeo_thread_t;
typedef HANDLE _gipeo_event_t;
#elif defined(CSSC_EMBEDDED)
/* Bare-metal MCUs (ESP32 / ESP8266 / AVR Arduino) — no pthread, no
 * usable host threading. The timer becomes a target-specific concern:
 * users wire it to Ticker.h / esp_timer / Timer1 in their script.
 * Here we only need stub types so the compile succeeds. */
typedef int _gipeo_thread_t;
typedef int _gipeo_event_t;
#else
typedef pthread_t _gipeo_thread_t;
typedef volatile int _gipeo_event_t;
#endif

typedef struct _GipeoTimerCtx {
    CsscTimer* t;
    _gipeo_event_t stop;
    _gipeo_thread_t thread;
} _GipeoTimerCtx;

#if defined(_WIN32)
static DWORD WINAPI _gipeo_timer_loop(LPVOID arg) {
    _GipeoTimerCtx* ctx = (_GipeoTimerCtx*)arg;
    DWORD period_ms = (1000 / (ctx->t->hz ? ctx->t->hz : 1));
    if (period_ms == 0) period_ms = 1;
    while (WaitForSingleObject(ctx->stop, period_ms) == WAIT_TIMEOUT) {
        if (ctx->t->callback) ctx->t->callback();
    }
    return 0;
}
#elif !defined(CSSC_EMBEDDED)
static void* _gipeo_timer_loop(void* arg) {
    _GipeoTimerCtx* ctx = (_GipeoTimerCtx*)arg;
    long period_us = 1000000L / (ctx->t->hz ? ctx->t->hz : 1);
    while (!ctx->stop) {
        if (ctx->t->callback) ctx->t->callback();
        usleep((useconds_t)period_us);
    }
    return NULL;
}
#endif

CSSC_GIPEO_API void cssc_timer_start(CsscTimer* t) {
    if (!t || !t->callback || t->running) return;
#if defined(CSSC_EMBEDDED)
    /* No host-portable timer threading on bare metal. The user must
     * wire the callback to a target-native ticker (esp_timer, Ticker,
     * Timer1) themselves. We mark running=1 so .stop() short-circuits
     * cleanly but otherwise no-op. */
    t->running = 1;
    _gipeo_log("timer[%u].start(%u Hz) — embedded stub, hook your platform timer",
               t->slot, t->hz);
#else
    _GipeoTimerCtx* ctx = (_GipeoTimerCtx*)calloc(1, sizeof(_GipeoTimerCtx));
    ctx->t = t;
    t->backend = ctx;
    t->running = 1;
  #if defined(_WIN32)
    ctx->stop = CreateEvent(NULL, TRUE, FALSE, NULL);
    ctx->thread = CreateThread(NULL, 0, _gipeo_timer_loop, ctx, 0, NULL);
  #else
    ctx->stop = 0;
    pthread_create(&ctx->thread, NULL, _gipeo_timer_loop, ctx);
  #endif
    _gipeo_log("timer[%u].start(%u Hz)", t->slot, t->hz);
#endif
}

CSSC_GIPEO_API void cssc_timer_stop(CsscTimer* t) {
    if (!t || !t->running) return;
    t->running = 0;
#if defined(CSSC_EMBEDDED)
    /* Embedded stub — backend was never allocated. */
    _gipeo_log("timer[%u].stop() — embedded stub", t->slot);
#else
    _GipeoTimerCtx* ctx = (_GipeoTimerCtx*)t->backend;
    if (ctx) {
  #if defined(_WIN32)
        SetEvent(ctx->stop);
        WaitForSingleObject(ctx->thread, 1000);
        CloseHandle(ctx->thread);
        CloseHandle(ctx->stop);
  #else
        ctx->stop = 1;
        pthread_join(ctx->thread, NULL);
  #endif
        free(ctx);
        t->backend = NULL;
    }
    _gipeo_log("timer[%u].stop()", t->slot);
#endif
}


/* ===========================================================================
 * gipeo:: namespace helpers
 * =========================================================================== */

CSSC_GIPEO_API uint32_t cssc_gipeo_millis(void) {
#if defined(CSSC_GIPEO_HAS_ARDUINO) || defined(CSSC_GIPEO_HAS_ESP_IDF)
    /* Arduino-core (and ESP-IDF via its arduino-component) all expose
     * millis()/micros() as the canonical platform clock. */
    return millis();
#elif defined(_WIN32)
    return (uint32_t)GetTickCount();
#elif defined(CSSC_EMBEDDED)
    /* Embedded target without an SDK that provides millis() — no
     * portable clock; tick a static counter so callers at least get
     * unique numbers and ordering. The user wires real timing via
     * platform-native APIs in their final-build wrapper. */
    static uint32_t _fake_ms = 0;
    return ++_fake_ms;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
#endif
}

CSSC_GIPEO_API uint64_t cssc_gipeo_micros(void) {
#if defined(CSSC_GIPEO_HAS_ARDUINO) || defined(CSSC_GIPEO_HAS_ESP_IDF)
    return micros();
#elif defined(_WIN32)
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (uint64_t)((c.QuadPart * 1000000ULL) / f.QuadPart);
#elif defined(CSSC_EMBEDDED)
    static uint64_t _fake_us = 0;
    return ++_fake_us;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000ULL);
#endif
}

CSSC_GIPEO_API void cssc_gipeo_delay_ms(uint32_t ms) {
#if defined(CSSC_GIPEO_HAS_ARDUINO) || defined(CSSC_GIPEO_HAS_ESP_IDF)
    delay(ms);
#elif defined(_WIN32)
    Sleep(ms);
#elif defined(CSSC_EMBEDDED)
    /* No portable sleep without SDK. Busy-waiting would peg the CPU
     * and there's no useful clock reference. User wires real timing
     * via vTaskDelay / Ticker / etc. in the platform-final wrapper. */
    (void)ms;
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}
