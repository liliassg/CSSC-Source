

#include "cssc_tft.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#if defined(CSSC_ESP32) && defined(__has_include) && __has_include("esp_lcd_panel_io.h")
  #include "esp_lcd_panel_io.h"
  #include "esp_lcd_panel_ops.h"
  #include "esp_lcd_panel_vendor.h"
  #define CSSC_TFT_HAS_ESP_LCD 1
#elif (defined(CSSC_ESP8266) || defined(CSSC_ARDUINO)) && \
      defined(__has_include) && __has_include(<Arduino.h>) && __has_include(<Wire.h>)

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

#if CSSC_TFT_HAS_SSD1306

#define _SSD_DEFAULT_ADDR 0x3C

typedef struct {
    uint8_t  i2c_addr;
    uint8_t  begun;
    int8_t   user_sda;
    int8_t   user_scl;
    uint8_t  user_addr;
    uint8_t  fb[1024];
} CsscTftSSD1306;

static uint8_t g_ssd_last_err = 0;

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

    uint8_t mux  = (h == 32) ? 0x1F : 0x3F;
    uint8_t cmpn = (h == 32) ? 0x02 : 0x12;
    _ssd_cmd1(addr, 0xAE);
    _ssd_cmd2(addr, 0xD5, 0x80);
    _ssd_cmd2(addr, 0xA8, mux);
    _ssd_cmd2(addr, 0xD3, 0x00);
    _ssd_cmd1(addr, 0x40);
    _ssd_cmd2(addr, 0x8D, 0x14);
    _ssd_cmd2(addr, 0x20, 0x00);
    _ssd_cmd1(addr, 0xA1);
    _ssd_cmd1(addr, 0xC8);
    _ssd_cmd2(addr, 0xDA, cmpn);
    _ssd_cmd2(addr, 0x81, 0xCF);
    _ssd_cmd2(addr, 0xD9, 0xF1);
    _ssd_cmd2(addr, 0xDB, 0x40);
    _ssd_cmd1(addr, 0xA4);
    _ssd_cmd1(addr, 0xA6);
    _ssd_cmd1(addr, 0xAF);
    (void)w;
}

static void _ssd_flush(CsscTftSSD1306* s, uint16_t w, uint16_t h) {
    uint8_t pages = (uint8_t)((h + 7) / 8);
    uint8_t cols  = (uint8_t)w;

    _ssd_cmd1(s->i2c_addr, 0x21);
    _ssd_cmd2(s->i2c_addr, 0x00, (uint8_t)(cols - 1));
    _ssd_cmd1(s->i2c_addr, 0x22);
    _ssd_cmd2(s->i2c_addr, 0x00, (uint8_t)(pages - 1));

    uint16_t total = (uint16_t)(cols * pages);
    uint16_t i = 0;
    while (i < total) {
        uint8_t chunk = 16;
        if ((uint16_t)(total - i) < chunk) chunk = (uint8_t)(total - i);
        Wire.beginTransmission(s->i2c_addr);
        Wire.write((uint8_t)0x40);
        Wire.write(s->fb + i, chunk);
        Wire.endTransmission();
        i += chunk;
    }
}

#endif

static inline CsscTft* _tft_from(CsscVal v) {
    return (CsscTft*)v.data.ptr;
}

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

static uint8_t _ssd_probe_pins(int sda, int scl) {
  #if defined(ESP8266) || defined(ESP32)
    Wire.begin(sda, scl);
  #else
    (void)sda; (void)scl;
    Wire.begin();
  #endif
    Wire.setClock(100000);

    Wire.beginTransmission((uint8_t)0x00);
    Wire.endTransmission();
    delay(2);

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
#endif

CSSC_TFT_API void cssc_tft_begin(CsscVal v) {
    CsscTft* t = _tft_from(v);
    if (!t) return;
#if CSSC_TFT_HAS_SSD1306
    CsscTftSSD1306* s = (CsscTftSSD1306*)t->backend;
    if (!s) return;

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

    static const struct { int sda; int scl; const char* name; } kPinSets[] = {
      #if defined(ESP8266)
        {  4,  5, "GPIO4/5 (D2/D1) SDA/SCL" },
        {  5,  4, "GPIO5/4 (D1/D2) SDA/SCL" },
        { 14, 12, "GPIO14/12 (D5/D6)"       },
        { 12, 14, "GPIO12/14 (D6/D5)"       },
        {  0,  2, "GPIO0/2 (D3/D4)"         },
        {  2,  0, "GPIO2/0 (D4/D3)"         },
        { 13, 15, "GPIO13/15 (D7/D8)"       },
      #elif defined(ESP32)
        { 21, 22, "GPIO21/22"         },
        {  4, 15, "GPIO4/15"          },
        {  5,  4, "GPIO5/4"           },
      #else
        { -1, -1, "default"           },
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

  #if defined(ESP8266) || defined(ESP32)
    Wire.begin(kPinSets[found_idx].sda, kPinSets[found_idx].scl);
  #else
    Wire.begin();
  #endif

    Wire.setClock(100000);
    s->i2c_addr = found_addr;
    _ssd_log_kv("found at 0x", (int)found_addr);
    _ssd_log(kPinSets[found_idx].name);

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
