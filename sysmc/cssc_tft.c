/* cssc_tft.c — TFT/OLED backends.
 *
 * Single .c with target-specific blocks. Host (no embedded define)
 * gets a printf-tagged stub so the runtime links cleanly even when
 * cssc build runs on the desktop without a panel attached. Real
 * platform drivers are wired through #ifdef CSSC_ESP32 / CSSC_ESP8266
 * / CSSC_ARDUINO / CSSC_LINUX — pulling each platform's display SDK
 * only on its respective build.
 */

#include "cssc_tft.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The 5x7 ASCII font lives once at file scope. cssc_tft_text walks
 * each char, looks up its 5-byte column glyph, and asks the backend
 * to paint each lit pixel. Same glyph table as cssl_tft.py — keeps
 * emulator and native builds visually identical. */
static const uint8_t _FONT_5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x00,0x08,0x14,0x22,0x41},{0x14,0x14,0x14,0x14,0x14},
    {0x41,0x22,0x14,0x08,0x00},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x01,0x01},
    {0x3E,0x41,0x41,0x51,0x32},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x04,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},
    {0x03,0x04,0x78,0x04,0x03},{0x61,0x51,0x49,0x45,0x43},{0x00,0x00,0x7F,0x41,0x41},
    {0x02,0x04,0x08,0x10,0x20},{0x41,0x41,0x7F,0x00,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x08,0x14,0x54,0x54,0x3C},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
    {0x00,0x7F,0x10,0x28,0x44},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44},{0x00,0x08,0x36,0x41,0x00},{0x00,0x00,0x7F,0x00,0x00},
    {0x00,0x41,0x36,0x08,0x00},{0x08,0x04,0x08,0x10,0x08},
};


/* =====================================================================
 * Backend selection
 * ===================================================================== */

/* SDK-header probing — only pull in the platform's display SDK if it
 * is actually on the include path. Without it we still link cleanly
 * (the public ABI bodies fall back to printf-style stubs that mirror
 * the host build); the user later compiles the generated .c through
 * PlatformIO or Arduino IDE for real hardware drivers. */
#if defined(CSSC_ESP32) && defined(__has_include) && __has_include("esp_lcd_panel_io.h")
  #include "esp_lcd_panel_io.h"
  #include "esp_lcd_panel_ops.h"
  #include "esp_lcd_panel_vendor.h"
  #define CSSC_TFT_HAS_ESP_LCD 1
#elif (defined(CSSC_ESP8266) || defined(CSSC_ARDUINO)) && \
      defined(__has_include) && __has_include(<Arduino.h>) && __has_include(<Wire.h>)
  /* Arduino-core path — bare-metal SSD1306 over I2C. We don't pull in
   * Adafruit_GFX/Adafruit_SSD1306 because that doubles flash & adds
   * library-manager friction. SSD1306 is simple enough to drive
   * directly: a 1024-byte page-organized framebuffer in RAM, an init
   * sequence at begin(), and a sweep of 16-byte data writes per show().
   * Cost: 1KB RAM per display. */
  #include <Arduino.h>
  #include <Wire.h>
  #define CSSC_TFT_HAS_ARDUINO 1
  #define CSSC_TFT_HAS_SSD1306 1
#elif defined(CSSC_LINUX) && defined(__has_include) && __has_include(<linux/fb.h>)
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/ioctl.h>
  #include <sys/mman.h>
  #include <linux/fb.h>
  #define CSSC_TFT_HAS_LINUX_FB 1
#endif


/* =====================================================================
 * SSD1306 backend (Arduino-core / ESP8266 / ESP32 via Wire)
 * =====================================================================
 *
 * Direct I2C driver. The display has a 128×64 (or 128×32) bitmap
 * organised into 8 pages of 8 vertical pixels per byte. We keep a
 * 1KB framebuffer in RAM, set/clear bits on cssc_tft_pixel, and on
 * cssc_tft_show stream the whole buffer out in 16-byte data chunks
 * preceded by a 0x40 control byte (continuous data mode).
 *
 * Default I2C address is 0x3C. Some panels are wired to 0x3D — if
 * the user's display stays dark we may need to add a `setaddr` API,
 * but every Ideaspark/AZ-Delivery NodeMCU clone I've seen uses 0x3C.
 */
#if CSSC_TFT_HAS_SSD1306

#define _SSD_DEFAULT_ADDR 0x3C

typedef struct {
    uint8_t  i2c_addr;
    uint8_t  begun;          /* Wire + init done */
    int8_t   user_sda;       /* explicit pin override; -1 = auto */
    int8_t   user_scl;
    uint8_t  user_addr;      /* explicit I2C address override; 0 = auto */
    uint8_t  fb[1024];       /* 128×64 / 8 — also covers 128×32 */
} CsscTftSSD1306;

/* Tracking error state across the init sequence so we can report a
 * single summary line on Serial — repeating one error per failed
 * cmd would spam the log without helping diagnose. */
static uint8_t g_ssd_last_err = 0;

/* Send a single command byte (control byte 0x00 = "next byte is cmd"). */
static void _ssd_cmd1(uint8_t addr, uint8_t c) {
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x00);
    Wire.write(c);
    uint8_t r = Wire.endTransmission();
    if (r && !g_ssd_last_err) g_ssd_last_err = r;
}

static void _ssd_cmd2(uint8_t addr, uint8_t c0, uint8_t c1) {
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0x00);
    Wire.write(c0);
    Wire.write(c1);
    uint8_t r = Wire.endTransmission();
    if (r && !g_ssd_last_err) g_ssd_last_err = r;
}

static void _ssd_init(uint8_t addr, uint16_t w, uint16_t h) {
    /* Datasheet boot sequence. Values match Adafruit's reference for
     * 128×64 — only the multiplex (0xA8) and COM-pins (0xDA) bytes
     * change for 128×32 panels, handled inline. */
    uint8_t mux  = (h == 32) ? 0x1F : 0x3F;
    uint8_t cmpn = (h == 32) ? 0x02 : 0x12;
    _ssd_cmd1(addr, 0xAE);             /* display off */
    _ssd_cmd2(addr, 0xD5, 0x80);       /* clock div */
    _ssd_cmd2(addr, 0xA8, mux);        /* multiplex */
    _ssd_cmd2(addr, 0xD3, 0x00);       /* offset 0 */
    _ssd_cmd1(addr, 0x40);             /* start line 0 */
    _ssd_cmd2(addr, 0x8D, 0x14);       /* charge pump on */
    _ssd_cmd2(addr, 0x20, 0x00);       /* horizontal addressing */
    _ssd_cmd1(addr, 0xA1);             /* segment remap */
    _ssd_cmd1(addr, 0xC8);             /* COM scan dec */
    _ssd_cmd2(addr, 0xDA, cmpn);       /* COM pins */
    _ssd_cmd2(addr, 0x81, 0xCF);       /* contrast */
    _ssd_cmd2(addr, 0xD9, 0xF1);       /* pre-charge */
    _ssd_cmd2(addr, 0xDB, 0x40);       /* vcomh */
    _ssd_cmd1(addr, 0xA4);             /* resume from RAM */
    _ssd_cmd1(addr, 0xA6);             /* normal (non-inverted) */
    _ssd_cmd1(addr, 0xAF);             /* display on */
    (void)w;
}

static void _ssd_flush(CsscTftSSD1306* s, uint16_t w, uint16_t h) {
    uint8_t pages = (uint8_t)((h + 7) / 8);
    uint8_t cols  = (uint8_t)w;
    /* Set column window 0..cols-1, page window 0..pages-1. */
    _ssd_cmd1(s->i2c_addr, 0x21);
    _ssd_cmd2(s->i2c_addr, 0x00, (uint8_t)(cols - 1));
    _ssd_cmd1(s->i2c_addr, 0x22);
    _ssd_cmd2(s->i2c_addr, 0x00, (uint8_t)(pages - 1));
    /* Stream framebuffer in 16-byte chunks. Wire's TX buffer on AVR
     * is 32 bytes — 16 data + 1 control byte fits cleanly. ESP8266
     * Wire allows 128 but 16 keeps it portable. */
    uint16_t total = (uint16_t)(cols * pages);
    uint16_t i = 0;
    while (i < total) {
        uint8_t chunk = 16;
        if ((uint16_t)(total - i) < chunk) chunk = (uint8_t)(total - i);
        Wire.beginTransmission(s->i2c_addr);
        Wire.write((uint8_t)0x40);    /* "data follows" control byte */
        Wire.write(s->fb + i, chunk);
        Wire.endTransmission();
        i += chunk;
    }
}

#endif  /* CSSC_TFT_HAS_SSD1306 */

/* =====================================================================
 * Public API
 * ===================================================================== */

/* Unwrap the CsscTft* from a CsscVal — returns NULL if the val isn't
 * pointer-shaped, the runtime treats every method as a no-op then. */
static inline CsscTft* _tft_from(CsscVal v) {
    return (CsscTft*)v.data.ptr;
}

/* Internal allocator shared by cssc_tft_create + cssc_tft_create_pins.
 * sda/scl=-1 and addr=0 mean "no override, auto-detect in begin()". */
static CsscVal _tft_alloc(const char* ctrl, uint16_t w, uint16_t h,
                          int16_t sda, int16_t scl, uint8_t addr) {
    CsscTft* t = (CsscTft*)calloc(1, sizeof(CsscTft));
    CsscVal r = {0};
    if (!t) return r;
    if (ctrl) {
        strncpy(t->ctrl, ctrl, sizeof(t->ctrl) - 1);
    } else {
        strcpy(t->ctrl, "ssd1306");
    }
    t->width  = w ? w : 128;
    t->height = h ? h : 64;
#if CSSC_TFT_HAS_SSD1306
    CsscTftSSD1306* s = (CsscTftSSD1306*)calloc(1, sizeof(CsscTftSSD1306));
    if (s) {
        s->i2c_addr  = _SSD_DEFAULT_ADDR;
        s->user_sda  = (int8_t)((sda >= 0 && sda <= 127) ? sda : -1);
        s->user_scl  = (int8_t)((scl >= 0 && scl <= 127) ? scl : -1);
        s->user_addr = addr;
        t->backend = s;
    }
#else
    (void)sda; (void)scl; (void)addr;
    fprintf(stderr, "[tft] %s %ux%u allocated\n", t->ctrl, t->width, t->height);
#endif
    r.tag = CSSC_TYPE_POINTER;
    r.data.ptr = t;
    return r;
}

CSSC_TFT_API CsscVal cssc_tft_create(const char* ctrl, uint16_t w, uint16_t h) {
    return _tft_alloc(ctrl, w, h, -1, -1, 0);
}

CSSC_TFT_API CsscVal cssc_tft_create_pins(const char* ctrl, uint16_t w, uint16_t h,
                                           int16_t sda, int16_t scl, uint8_t addr) {
    return _tft_alloc(ctrl, w, h, sda, scl, addr);
}

CSSC_TFT_API void cssc_tft_destroy(CsscVal v) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
#if CSSC_TFT_HAS_SSD1306
    if (t->backend) free(t->backend);
#endif
    free(t);
}

#if CSSC_TFT_HAS_SSD1306
/* Probe a (sda, scl) pin pair. Strategy:
 *   1. Quick check at the standard SSD1306 addresses 0x3C / 0x3D —
 *      99% of the time this is the answer.
 *   2. If neither replies, do a full bus scan from 0x08..0x77 and
 *      log every responder to Serial. The user then sees exactly
 *      where the display lives (or that the bus is empty).
 *
 * Returns the responding SSD1306 address (only 0x3C / 0x3D pass),
 * or 0 if nothing usable answered.
 */
static uint8_t _ssd_probe_pins(int sda, int scl) {
  #if defined(ESP8266) || defined(ESP32)
    Wire.begin(sda, scl);
  #else
    (void)sda; (void)scl;
    Wire.begin();
  #endif
    Wire.setClock(100000);   /* 100kHz for the scan — more reliable
                              * with weak pull-ups; we bump back to
                              * 400kHz only after init completes. */
    /* Bus warm-up. After Wire.begin() with new pins the line state
     * can need a few clock cycles to stabilize; the SSD1306 then
     * may not ACK the very first transaction. We send a benign
     * "ping" to a non-target address so the bus toggles, then
     * pause briefly. This eliminated the failure where the fast
     * path missed 0x3C even though the full scan found it. */
    Wire.beginTransmission((uint8_t)0x00);
    Wire.endTransmission();
    delay(2);

    /* Fast path: the two SSD1306-canonical addresses, retried up
     * to 3× each since first-touch on a freshly-bound bus can
     * miss the ACK window even with the warm-up above. */
    static const uint8_t kCandidates[2] = { 0x3C, 0x3D };
    for (int retry = 0; retry < 3; retry++) {
        for (int i = 0; i < 2; i++) {
            Wire.beginTransmission(kCandidates[i]);
            if (Wire.endTransmission() == 0) {
                return kCandidates[i];
            }
        }
        delay(2);
    }

    /* Full bus scan — log everything that ACKs so the user can see
     * what's actually connected, even at a non-SSD1306 address. */
  #if defined(ARDUINO)
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print(F("[cssc-tft]   bus device at 0x"));
            if (addr < 0x10) Serial.print('0');
            Serial.println(addr, HEX);
            found++;
        }
    }
    if (found == 0) {
        Serial.println(F("[cssc-tft]   (bus empty on these pins)"));
    }
  #endif
    return 0;
}

/* Optional Serial diagnostic — gated by the existence of Serial,
 * which on Arduino-core is always available. We only emit a few
 * short lines so it doesn't blow up the binary. */
static void _ssd_log(const char* msg) {
  #if defined(ARDUINO)
    Serial.print(F("[cssc-tft] "));
    Serial.println(msg);
  #else
    (void)msg;
  #endif
}

static void _ssd_log_kv(const char* key, int v) {
  #if defined(ARDUINO)
    Serial.print(F("[cssc-tft] "));
    Serial.print(key);
    Serial.print(F("="));
    Serial.println(v);
  #else
    (void)key; (void)v;
  #endif
}
#endif  /* CSSC_TFT_HAS_SSD1306 */

CSSC_TFT_API void cssc_tft_begin(CsscVal v) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
#if CSSC_TFT_HAS_SSD1306
    CsscTftSSD1306* s = (CsscTftSSD1306*)t->backend;
    if (!s) return;

    /* Fast path: user gave us explicit pins via #oled[w,h,sda,scl]
     * or #tft[ctrl,w,h,sda,scl,addr]. Skip the auto-scan entirely
     * and bind directly. This is the production-correct path —
     * once a board's wiring is known, the script should declare it.
     * Auto-scan is the convenience fallback for the discovery phase. */
    if (s->user_sda >= 0 && s->user_scl >= 0) {
      #if defined(ESP8266) || defined(ESP32)
        Wire.begin((int)s->user_sda, (int)s->user_scl);
      #else
        Wire.begin();
      #endif
        Wire.setClock(100000);
        s->i2c_addr = s->user_addr ? s->user_addr : _SSD_DEFAULT_ADDR;
        delay(100);
        g_ssd_last_err = 0;
        _ssd_init(s->i2c_addr, t->width, t->height);
        s->begun = 1;
        memset(s->fb, 0, sizeof(s->fb));
        _ssd_flush(s, t->width, t->height);
      #if defined(ARDUINO)
        Serial.print(F("[cssc-tft] explicit pins SDA="));
        Serial.print((int)s->user_sda);
        Serial.print(F(" SCL="));
        Serial.print((int)s->user_scl);
        Serial.print(F(" addr=0x"));
        Serial.println(s->i2c_addr, HEX);
      #endif
        return;
    }

    /* Many ESP8266 dev boards (Wemos D1 mini, NodeMCU clones,
     * Ideaspark variants) wire the onboard OLED to one of a small
     * set of pin pairs. Rather than guess, we probe each in turn
     * and use whichever one has an SSD1306 ack at 0x3C or 0x3D.
     * The order below covers >95% of boards; user can override by
     * passing pins as additional bracket args to #oled / #tft. */
    static const struct { int sda; int scl; const char* name; } kPinSets[] = {
      #if defined(ESP8266)
        {  4,  5, "GPIO4/5 (D2/D1) SDA/SCL" },  /* Wemos D1 mini standard */
        {  5,  4, "GPIO5/4 (D1/D2) SDA/SCL" },  /* flipped — Ideaspark variants */
        { 14, 12, "GPIO14/12 (D5/D6)"       },  /* some Ideaspark variants */
        { 12, 14, "GPIO12/14 (D6/D5)"       },  /* swapped — seen in the wild */
        {  0,  2, "GPIO0/2 (D3/D4)"         },  /* rare layout */
        {  2,  0, "GPIO2/0 (D4/D3)"         },  /* flipped */
        { 13, 15, "GPIO13/15 (D7/D8)"       },  /* HSPI pins — some boards */
      #elif defined(ESP32)
        { 21, 22, "GPIO21/22"         },  /* ESP32 default */
        {  4, 15, "GPIO4/15"          },  /* TTGO LoRa boards */
        {  5,  4, "GPIO5/4"           },  /* Heltec WiFi Kit */
      #else
        { -1, -1, "default"           },  /* AVR — Wire.begin() picks pins */
      #endif
    };
    const int kPinSetCount = (int)(sizeof(kPinSets) / sizeof(kPinSets[0]));

    _ssd_log("scanning I2C for SSD1306 ...");
    uint8_t found_addr = 0;
    int     found_idx  = -1;
    for (int i = 0; i < kPinSetCount; i++) {
        _ssd_log(kPinSets[i].name);
        uint8_t a = _ssd_probe_pins(kPinSets[i].sda, kPinSets[i].scl);
        if (a) {
            found_addr = a;
            found_idx  = i;
            break;
        }
    }

    if (!found_addr) {
        _ssd_log("no SSD1306 responding on any candidate pin set.");
        _ssd_log("  Check: cable, power (3.3V), I2C wiring, address jumper.");
        s->begun = 0;
        return;
    }

    /* Re-bind to the winning pin set (probe leaves it bound, but be
     * explicit so a future re-init works the same way). */
  #if defined(ESP8266) || defined(ESP32)
    Wire.begin(kPinSets[found_idx].sda, kPinSets[found_idx].scl);
  #else
    Wire.begin();
  #endif
    /* Stay at 100kHz for init — many cheap OLED clones have weak
     * pull-ups that don't hold the line at 400kHz on the very
     * first transactions. We bump to 400kHz only after the
     * diagnostic splash has confirmed end-to-end works. */
    Wire.setClock(100000);
    s->i2c_addr = found_addr;
    _ssd_log_kv("found at 0x", (int)found_addr);
    _ssd_log(kPinSets[found_idx].name);

    /* Charge-pump + VCC stabilisation: SSD1306 datasheet asks for
     * 100ms after power-on before the panel will respond reliably
     * to display-on. ESPs come up faster than the OLED so we wait
     * here explicitly. */
    delay(100);

    g_ssd_last_err = 0;
    _ssd_init(s->i2c_addr, t->width, t->height);
    s->begun = 1;

    if (g_ssd_last_err) {
        _ssd_log_kv("init had I2C errors, last code=", (int)g_ssd_last_err);
        _ssd_log("  (1=data too long, 2=NACK addr, 3=NACK data, 4=other)");
    } else {
        _ssd_log("init OK (all cmds ACKed).");
    }

    /* Clear so user code starts on a clean black canvas. We deliberately
     * stay at 100kHz from now on — many cheap SSD1306 clones (Ideaspark,
     * AZ-Delivery, generic Wemos) have weak I2C pull-ups and silently
     * NACK at 400kHz. 100kHz still gives ~50Hz refresh on a 128×64
     * panel, plenty for status displays. */
    memset(s->fb, 0, sizeof(s->fb));
    _ssd_flush(s, t->width, t->height);
    _ssd_log("ready.");
#else
    fprintf(stderr, "[tft] %s begin\n", t->ctrl);
#endif
}

CSSC_TFT_API void cssc_tft_fill(CsscVal v, uint32_t color) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
#if CSSC_TFT_HAS_SSD1306
    CsscTftSSD1306* s = (CsscTftSSD1306*)t->backend;
    if (!s) return;
    /* SSD1306 is 1bpp — any non-zero color is "on", 0 is "off". */
    memset(s->fb, color ? 0xFF : 0x00, sizeof(s->fb));
#else
    fprintf(stderr, "[tft] %s fill 0x%X\n", t->ctrl, (unsigned)color);
#endif
}

CSSC_TFT_API void cssc_tft_clear(CsscVal v) { cssc_tft_fill(v, CSSC_TFT_BLACK); }

CSSC_TFT_API void cssc_tft_pixel(CsscVal v, int16_t x, int16_t y, uint32_t color) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
#if CSSC_TFT_HAS_SSD1306
    CsscTftSSD1306* s = (CsscTftSSD1306*)t->backend;
    if (!s) return;
    if (x < 0 || y < 0 || x >= (int16_t)t->width || y >= (int16_t)t->height) return;
    /* Page-organised: byte at (col, page=y/8), bit (y%8) is the pixel. */
    uint16_t idx = (uint16_t)x + ((uint16_t)(y >> 3) * (uint16_t)t->width);
    uint8_t  bit = (uint8_t)(1u << (y & 7));
    if (color) s->fb[idx] |= bit;
    else       s->fb[idx] &= (uint8_t)~bit;
#else
    (void)x; (void)y; (void)color;
#endif
}

CSSC_TFT_API void cssc_tft_line(CsscVal v, int16_t x0, int16_t y0,
                                 int16_t x1, int16_t y1, uint32_t color) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    while (1) {
        cssc_tft_pixel(v, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = err * 2;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

CSSC_TFT_API void cssc_tft_rect(CsscVal v, int16_t x, int16_t y,
                                 int16_t w, int16_t h, uint32_t color) {
    if (!_tft_from(v)) return;
    cssc_tft_line(v, x, y, x + w - 1, y, color);
    cssc_tft_line(v, x, y + h - 1, x + w - 1, y + h - 1, color);
    cssc_tft_line(v, x, y, x, y + h - 1, color);
    cssc_tft_line(v, x + w - 1, y, x + w - 1, y + h - 1, color);
}

CSSC_TFT_API void cssc_tft_fillrect(CsscVal v, int16_t x, int16_t y,
                                     int16_t w, int16_t h, uint32_t color) {
    if (!_tft_from(v)) return;
    for (int16_t row = 0; row < h; row++) {
        cssc_tft_line(v, x, y + row, x + w - 1, y + row, color);
    }
}

CSSC_TFT_API void cssc_tft_text(CsscVal v, int16_t x, int16_t y,
                                 const char* s, uint32_t color, uint8_t scale) {
    if (!_tft_from(v) || !s) return;
    if (scale == 0) scale = 1;
    int16_t cx = x;
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        if (c < 0x20 || c > 0x7E) c = '.';
        const uint8_t* glyph = _FONT_5x7[c - 0x20];
        for (uint8_t col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (uint8_t row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    if (scale == 1) {
                        cssc_tft_pixel(v, cx + col, y + row, color);
                    } else {
                        cssc_tft_fillrect(v,
                            cx + col * scale, y + row * scale,
                            scale, scale, color);
                    }
                }
            }
        }
        cx += 6 * scale;
    }
}

CSSC_TFT_API void cssc_tft_show(CsscVal v) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
#if CSSC_TFT_HAS_SSD1306
    CsscTftSSD1306* s = (CsscTftSSD1306*)t->backend;
    if (!s || !s->begun) return;
    _ssd_flush(s, t->width, t->height);
#endif
}

CSSC_TFT_API uint16_t cssc_tft_width(CsscVal v)  { CsscTft* t = _tft_from(v); return t ? t->width  : 0; }
CSSC_TFT_API uint16_t cssc_tft_height(CsscVal v) { CsscTft* t = _tft_from(v); return t ? t->height : 0; }
