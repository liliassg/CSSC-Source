/* cssc_st7735.c — ST7735 / ST7789 / ILI9341-style SPI TFT driver for CSSC 6.
 *
 * Bit-banged SPI on ARBITRARY GPIO pins (works with any wiring, not just the
 * hardware-SPI mux), direct rendering (no full framebuffer — an ESP8266 has
 * no room for 128x160x2 = 40 KB). Colours are RGB565 (matching `tft::WHITE`
 * = 0xFFFF, `tft::BLACK` = 0x0000, `tft::RED` = 0xF800 from cir_lower).
 *
 * ABI: the CSSC call-lowering passes every scalar arg as a 64-bit value
 * (INT64) and strings as `cssc_string_data(...)` = a raw NUL-terminated
 * `const char*`. Hence the `long long` params (they land in Xtensa
 * register-pairs; a plain `int` would misread the pair ABI) and the
 * `const char*` text arg.
 *
 * On the host build (`!__XTENSA__`) the GPIO/SPI ops are compiled out and the
 * calls are silent no-ops, so `#tft["st7735", …]` still compiles and links
 * everywhere; the pixels just don't go anywhere without the panel.
 */
#include <stdint.h>
#include <stddef.h>

typedef long long cssc_i64;

typedef struct {
    int32_t w, h;
    int32_t cs, dc, res, clk, mosi;
    int32_t xoff, yoff;   /* panel column/row offset (green-tab vs red-tab) */
} CsscSt7735;

/* Single display instance — no heap alloc needed (one panel per board). */
static CsscSt7735 _st;

/* 5x7 ASCII font (same table as cssc_tft.c → identical glyphs host/native). */
static const uint8_t _ST_FONT[96][5] = {
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

#ifdef __XTENSA__
/* ================= ESP8266 register-level bit-bang SPI ================= */
#define _REG(a)  (*(volatile uint32_t*)(uintptr_t)(a))
#define GPIO_OUT_W1TS     0x60000304u   /* set   GPIO0..15 high */
#define GPIO_OUT_W1TC     0x60000308u   /* set   GPIO0..15 low  */
#define GPIO_ENABLE_W1TS  0x60000310u   /* enable GPIO0..15 output */
/* GPIO16 lives in the RTC domain — separate registers. */
#define GP16_OUT   0x60000768u
#define GP16_ENA   0x60000774u
#define GP16_FUNC  0x60000090u
#define GP16_CTRL  0x60000790u

static void _pin_out(int32_t g) {
    if (g < 0) return;
    if (g == 16) {
        /* GPIO16 lives in the RTC block — same sequence the Arduino core's
         * pinMode(16, OUTPUT) uses: GPF16 = GP16FFS(GPFFS_GPIO(16)) = 1
         * (select the GPIO function for pin 16), GPC16 = 0, then enable the
         * output driver. Getting GPF16 wrong (e.g. clearing it to 0) leaves
         * the pad in an alternate mux so the level never reaches the pin. */
        _REG(GP16_FUNC) = 1u;                                /* GPIO function */
        _REG(GP16_CTRL) = 0u;
        _REG(GP16_ENA)  = (_REG(GP16_ENA) & ~0x1u) | 0x1u;   /* output enable */
    } else {
        _REG(GPIO_ENABLE_W1TS) = (1u << g);
    }
}
static inline void _pin_hi(int32_t g) {
    if (g < 0) return;
    if (g == 16) _REG(GP16_OUT) = (_REG(GP16_OUT) & ~0x1u) | 0x1u;
    else _REG(GPIO_OUT_W1TS) = (1u << g);
}
static inline void _pin_lo(int32_t g) {
    if (g < 0) return;
    if (g == 16) _REG(GP16_OUT) = (_REG(GP16_OUT) & ~0x1u);
    else _REG(GPIO_OUT_W1TC) = (1u << g);
}
static void _busy(volatile uint32_t n) { while (n--) { __asm__ __volatile__("nop"); } }

/* One byte, MSB first, SPI mode 0 (clock idles low, sampled on rising edge). */
static void _spi(uint8_t b) {
    int32_t clk = _st.clk, mosi = _st.mosi;
    for (int i = 0; i < 8; i++) {
        if (b & 0x80) _pin_hi(mosi); else _pin_lo(mosi);
        _pin_hi(clk);
        _pin_lo(clk);
        b = (uint8_t)(b << 1);
    }
}
#else
/* ================= host build — silent no-op stubs ================= */
static void _pin_out(int32_t g) { (void)g; }
static void _pin_hi(int32_t g)  { (void)g; }
static void _pin_lo(int32_t g)  { (void)g; }
static void _busy(volatile uint32_t n) { (void)n; }
static void _spi(uint8_t b) { (void)b; }
#endif

static void _cmd(uint8_t c) { _pin_lo(_st.dc); _spi(c); }
static void _dat(uint8_t v) { _pin_hi(_st.dc); _spi(v); }

/* Set the RAM address window (CASET / RASET) then start a pixel write (RAMWR).
 * Applies the panel offset so 0,0 lands top-left on off-centred variants. */
static void _win(int32_t x0, int32_t y0, int32_t x1, int32_t y1) {
    x0 += _st.xoff; x1 += _st.xoff;
    y0 += _st.yoff; y1 += _st.yoff;
    _cmd(0x2A);
    _dat((uint8_t)(x0 >> 8)); _dat((uint8_t)x0);
    _dat((uint8_t)(x1 >> 8)); _dat((uint8_t)x1);
    _cmd(0x2B);
    _dat((uint8_t)(y0 >> 8)); _dat((uint8_t)y0);
    _dat((uint8_t)(y1 >> 8)); _dat((uint8_t)y1);
    _cmd(0x2C);
}

/* Stream `n` pixels of one colour (dc already high after RAMWR). */
static void _blast(uint16_t color, int32_t n) {
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)color;
    _pin_hi(_st.dc);
    while (n-- > 0) { _spi(hi); _spi(lo); }
}

/* ---- public API (called from the CSSC dot-dispatch as cssc_st7735_*) ---- */

void* cssc_st7735_new(cssc_i64 w, cssc_i64 h, cssc_i64 cs, cssc_i64 dc,
                      cssc_i64 res, cssc_i64 clk, cssc_i64 mosi) {
    _st.w = (int32_t)w;   _st.h = (int32_t)h;
    _st.cs = (int32_t)cs; _st.dc = (int32_t)dc; _st.res = (int32_t)res;
    _st.clk = (int32_t)clk; _st.mosi = (int32_t)mosi;
    _st.xoff = 0; _st.yoff = 0;   /* red-tab default; adjust per panel */
    return &_st;
}

void cssc_st7735_begin(void* v) {
    (void)v;
    _pin_out(_st.cs); _pin_out(_st.dc);
    _pin_out(_st.clk); _pin_out(_st.mosi);
    _pin_lo(_st.clk);                 /* SPI mode-0 idle */
    _pin_lo(_st.cs);                  /* single device → hold CS asserted */
    if (_st.res >= 0) {
        _pin_out(_st.res);
        _pin_hi(_st.res); _busy(200000);
        _pin_lo(_st.res); _busy(200000);
        _pin_hi(_st.res); _busy(2000000);
    }
    _cmd(0x01); _busy(3000000);       /* SWRESET  (~150 ms) */
    _cmd(0x11); _busy(3000000);       /* SLPOUT   (~150 ms) */
    _cmd(0x3A); _dat(0x05);           /* COLMOD   = 16-bit/pixel (RGB565) */
    _cmd(0x36); _dat(0xC8);           /* MADCTL   = MX|MY|RGB (top-left origin) */
    _cmd(0x29); _busy(500000);        /* DISPON */
}

void cssc_st7735_fill(void* v, cssc_i64 color) {
    (void)v;
    _win(0, 0, _st.w - 1, _st.h - 1);
    _blast((uint16_t)color, _st.w * _st.h);
}

void cssc_st7735_fillrect(void* v, cssc_i64 x, cssc_i64 y,
                          cssc_i64 w, cssc_i64 h, cssc_i64 color) {
    (void)v;
    int32_t X = (int32_t)x, Y = (int32_t)y, W = (int32_t)w, H = (int32_t)h;
    if (X < 0) { W += X; X = 0; }
    if (Y < 0) { H += Y; Y = 0; }
    if (X + W > _st.w) W = _st.w - X;
    if (Y + H > _st.h) H = _st.h - Y;
    if (W <= 0 || H <= 0) return;
    _win(X, Y, X + W - 1, Y + H - 1);
    _blast((uint16_t)color, W * H);
}

void cssc_st7735_pixel(void* v, cssc_i64 x, cssc_i64 y, cssc_i64 color) {
    (void)v;
    int32_t X = (int32_t)x, Y = (int32_t)y;
    if (X < 0 || Y < 0 || X >= _st.w || Y >= _st.h) return;
    _win(X, Y, X, Y);
    _blast((uint16_t)color, 1);
}

void cssc_st7735_line(void* v, cssc_i64 x0, cssc_i64 y0,
                      cssc_i64 x1, cssc_i64 y1, cssc_i64 color) {
    (void)v;
    int32_t X0 = (int32_t)x0, Y0 = (int32_t)y0;
    int32_t X1 = (int32_t)x1, Y1 = (int32_t)y1;
    int32_t dx = X1 - X0, dy = Y1 - Y0;
    int32_t adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    int32_t sx = X0 < X1 ? 1 : -1, sy = Y0 < Y1 ? 1 : -1;
    int32_t err = (adx > ady ? adx : -ady) / 2, e2;
    for (;;) {
        cssc_st7735_pixel(v, X0, Y0, color);
        if (X0 == X1 && Y0 == Y1) break;
        e2 = err;
        if (e2 > -adx) { err -= ady; X0 += sx; }
        if (e2 <  ady) { err += adx; Y0 += sy; }
    }
}

void cssc_st7735_text(void* v, cssc_i64 x, cssc_i64 y, const char* s,
                      cssc_i64 color, cssc_i64 scale) {
    if (!s) return;
    int32_t sc = (int32_t)scale; if (sc <= 0) sc = 1;
    int32_t cx = (int32_t)x, cy = (int32_t)y;
    while (*s) {
        unsigned char ch = (unsigned char)*s++;
        if (ch < 0x20 || ch > 0x7E) ch = '.';
        const uint8_t* g = _ST_FONT[ch - 0x20];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = g[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    if (sc == 1)
                        cssc_st7735_pixel(v, cx + col, cy + row, color);
                    else
                        cssc_st7735_fillrect(v, cx + col * sc, cy + row * sc,
                                             sc, sc, color);
                }
            }
        }
        cx += 6 * sc;
    }
}

/* Direct-render backend → nothing to flush. Present for API symmetry. */
void cssc_st7735_show(void* v)  { (void)v; }
void cssc_st7735_close(void* v) { (void)v; }
