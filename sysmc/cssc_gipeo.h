/* cssc_gipeo.h — GPIO + embedded peripherals for native CSSC builds.
 *
 * Single header for the gipeo module's C ABI. Same surface across
 * targets — host (printf stubs), ESP32-S3 (ESP-IDF), Arduino (AVR/ARM
 * core), Raspberry Pi (Linux libgpiod / dev-files). The implementation
 * branches on these compile-time defines:
 *
 *   CSSC_ESP32       — set by `cssc build --esp32`
 *   CSSC_ARDUINO     — set by `cssc build --arduino`
 *   CSSC_LINUX       — set by `cssc build --raspberry`
 *   (none)           — host build (Win32 / generic POSIX), printf stubs
 *
 * All `cssc_*_create` calls allocate one struct on the heap and return
 * an opaque handle. CSSC's #delete[…] calls cssc_*_destroy. Every
 * runtime method takes the handle as its first arg, matching the
 * dot-call lowering the compiler emits.
 */

#ifndef CSSC_GIPEO_H
#define CSSC_GIPEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
  #ifdef CSSC_RUNTIME_EXPORTS
    #define CSSC_GIPEO_API __declspec(dllexport)
  #else
    #define CSSC_GIPEO_API __declspec(dllimport)
  #endif
#else
  #define CSSC_GIPEO_API
#endif

/* gipeo:: constants — kept in sync with cssl_gipeo.py so the same
 * #include('gipeo') script behaves identically under interpreter and
 * native build. */
#define CSSC_GIPEO_INPUT          0
#define CSSC_GIPEO_OUTPUT         1
#define CSSC_GIPEO_INPUT_PULLUP   2
#define CSSC_GIPEO_INPUT_PULLDOWN 3
#define CSSC_GIPEO_OPEN_DRAIN     4

#define CSSC_GIPEO_LOW            0
#define CSSC_GIPEO_HIGH           1

#define CSSC_GIPEO_RISING         1
#define CSSC_GIPEO_FALLING        2
#define CSSC_GIPEO_CHANGE         3


/* ------------------------------------------------------------------------
 * #pin[N] — single GPIO line
 * ------------------------------------------------------------------------ */

typedef struct CsscPin {
    uint32_t num;
    uint8_t  mode;
    uint8_t  state;
    /* Backend-specific handle stash (e.g. gpiod_line* on Linux). The
     * struct stays opaque to the host; backends cast as needed. */
    void*    backend;
    void   (*irq_callback)(void);
} CsscPin;

CSSC_GIPEO_API CsscPin* cssc_pin_create(uint32_t num);
CSSC_GIPEO_API void     cssc_pin_destroy(CsscPin* p);
CSSC_GIPEO_API void     cssc_pin_mode(CsscPin* p, int m);
CSSC_GIPEO_API void     cssc_pin_send(CsscPin* p, int level);
CSSC_GIPEO_API int      cssc_pin_status(CsscPin* p);
CSSC_GIPEO_API void     cssc_pin_toggle(CsscPin* p);
CSSC_GIPEO_API void     cssc_pin_attach_interrupt(CsscPin* p, void (*cb)(void), int edge);
CSSC_GIPEO_API void     cssc_pin_detach_interrupt(CsscPin* p);
CSSC_GIPEO_API uint32_t cssc_pin_num(CsscPin* p);


/* ------------------------------------------------------------------------
 * #i2c[bus, sda, scl] — I2C master
 * ------------------------------------------------------------------------ */

typedef struct CsscI2C {
    uint8_t  bus;
    uint8_t  sda;
    uint8_t  scl;
    uint32_t freq;
    void*    backend;       /* fd on Linux, esp_i2c_dev_t* on ESP32, etc. */
} CsscI2C;

CSSC_GIPEO_API CsscI2C* cssc_i2c_create(uint8_t bus, uint8_t sda, uint8_t scl);
CSSC_GIPEO_API void     cssc_i2c_destroy(CsscI2C* b);
CSSC_GIPEO_API void     cssc_i2c_begin(CsscI2C* b, uint32_t freq_hz);
/* write returns bytes ACKed; read returns bytes received (or -1 on err). */
CSSC_GIPEO_API int      cssc_i2c_write(CsscI2C* b, uint8_t addr,
                                       const uint8_t* data, size_t n);
CSSC_GIPEO_API int      cssc_i2c_read(CsscI2C* b, uint8_t addr, uint8_t reg,
                                      uint8_t* out, size_t n);
/* scan returns bitmap of ACK'd 7-bit addresses 0x00..0x7F packed into out
 * (16 bytes = 128 bits). Returns count of devices found. */
CSSC_GIPEO_API int      cssc_i2c_scan(CsscI2C* b, uint8_t out_bitmap[16]);


/* ------------------------------------------------------------------------
 * #spi[bus, sck, miso, mosi] — SPI master
 * ------------------------------------------------------------------------ */

typedef struct CsscSPI {
    uint8_t  bus;
    uint8_t  sck;
    uint8_t  miso;
    uint8_t  mosi;
    uint32_t freq;
    uint8_t  mode;
    void*    backend;
} CsscSPI;

CSSC_GIPEO_API CsscSPI* cssc_spi_create(uint8_t bus, uint8_t sck,
                                        uint8_t miso, uint8_t mosi);
CSSC_GIPEO_API void     cssc_spi_destroy(CsscSPI* b);
CSSC_GIPEO_API void     cssc_spi_begin(CsscSPI* b, uint32_t freq, uint8_t mode);
/* Full-duplex: tx_buf and rx_buf may alias for in-place. */
CSSC_GIPEO_API int      cssc_spi_transfer(CsscSPI* b,
                                          const uint8_t* tx_buf,
                                          uint8_t* rx_buf,
                                          size_t n);
CSSC_GIPEO_API uint8_t  cssc_spi_transfer_byte(CsscSPI* b, uint8_t tx);


/* ------------------------------------------------------------------------
 * #uart[bus, tx, rx] — UART / Serial
 * ------------------------------------------------------------------------ */

typedef struct CsscUART {
    uint8_t  bus;
    uint8_t  tx;
    uint8_t  rx;
    uint32_t baud;
    void*    backend;
} CsscUART;

CSSC_GIPEO_API CsscUART* cssc_uart_create(uint8_t bus, uint8_t tx, uint8_t rx);
CSSC_GIPEO_API void      cssc_uart_destroy(CsscUART* u);
CSSC_GIPEO_API void      cssc_uart_begin(CsscUART* u, uint32_t baud);
CSSC_GIPEO_API int       cssc_uart_write(CsscUART* u, const char* s);
CSSC_GIPEO_API int       cssc_uart_println(CsscUART* u, const char* s);
CSSC_GIPEO_API int       cssc_uart_available(CsscUART* u);
/* read_line writes up to (max_len-1) chars + NUL into out. Returns chars read. */
CSSC_GIPEO_API int       cssc_uart_read_line(CsscUART* u, char* out, size_t max_len);


/* ------------------------------------------------------------------------
 * #adc[pin] — analog input
 * ------------------------------------------------------------------------ */

typedef struct CsscADC {
    uint8_t  pin;
    uint8_t  resolution_bits;   /* 12 default (ESP32); 10 (Arduino AVR) */
    uint16_t vref_mv;            /* 3300 default */
    void*    backend;
} CsscADC;

CSSC_GIPEO_API CsscADC* cssc_adc_create(uint8_t pin);
CSSC_GIPEO_API void     cssc_adc_destroy(CsscADC* a);
CSSC_GIPEO_API void     cssc_adc_configure(CsscADC* a, uint8_t res_bits, uint16_t vref_mv);
CSSC_GIPEO_API int32_t  cssc_adc_read_raw(CsscADC* a);
CSSC_GIPEO_API int32_t  cssc_adc_read_mv(CsscADC* a);


/* ------------------------------------------------------------------------
 * #pwm[pin, freq, bits] — pulse-width modulation
 * ------------------------------------------------------------------------ */

typedef struct CsscPWM {
    uint8_t  pin;
    uint32_t freq;
    uint8_t  resolution_bits;
    uint32_t duty;
    uint8_t  running;
    void*    backend;
} CsscPWM;

CSSC_GIPEO_API CsscPWM* cssc_pwm_create(uint8_t pin, uint32_t freq, uint8_t res_bits);
CSSC_GIPEO_API void     cssc_pwm_destroy(CsscPWM* p);
CSSC_GIPEO_API void     cssc_pwm_duty(CsscPWM* p, uint32_t raw);
CSSC_GIPEO_API void     cssc_pwm_duty_pct(CsscPWM* p, double pct);
CSSC_GIPEO_API void     cssc_pwm_stop(CsscPWM* p);


/* ------------------------------------------------------------------------
 * #timer[slot, hz] — periodic timer (callback-driven)
 * ------------------------------------------------------------------------ */

typedef struct CsscTimer {
    uint8_t  slot;
    uint32_t hz;
    void   (*callback)(void);
    uint8_t  running;
    void*    backend;       /* host: thread handle; ESP32: gptimer_handle_t */
} CsscTimer;

CSSC_GIPEO_API CsscTimer* cssc_timer_create(uint8_t slot, uint32_t hz);
CSSC_GIPEO_API void       cssc_timer_destroy(CsscTimer* t);
CSSC_GIPEO_API void       cssc_timer_attach(CsscTimer* t, void (*cb)(void));
CSSC_GIPEO_API void       cssc_timer_start(CsscTimer* t);
CSSC_GIPEO_API void       cssc_timer_stop(CsscTimer* t);


/* ------------------------------------------------------------------------
 * gipeo:: namespace helpers — millis/micros/delay
 * ------------------------------------------------------------------------ */

CSSC_GIPEO_API uint32_t cssc_gipeo_millis(void);
CSSC_GIPEO_API uint64_t cssc_gipeo_micros(void);
CSSC_GIPEO_API void     cssc_gipeo_delay_ms(uint32_t ms);


#ifdef __cplusplus
}
#endif

#endif /* CSSC_GIPEO_H */
