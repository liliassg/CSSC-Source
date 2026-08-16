

#ifndef CSSC_TFT_H
#define CSSC_TFT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "cssc_runtime.h"

#ifdef _WIN32
  #ifdef CSSC_RUNTIME_EXPORTS
    #define CSSC_TFT_API __declspec(dllexport)
  #else
    #define CSSC_TFT_API __declspec(dllimport)
  #endif
#else
  #define CSSC_TFT_API
#endif

#define CSSC_TFT_BLACK   0x0000
#define CSSC_TFT_WHITE   0xFFFF
#define CSSC_TFT_RED     0xF800
#define CSSC_TFT_GREEN   0x07E0
#define CSSC_TFT_BLUE    0x001F
#define CSSC_TFT_CYAN    0x07FF
#define CSSC_TFT_MAGENTA 0xF81F
#define CSSC_TFT_YELLOW  0xFFE0

typedef struct CsscTft {
    char     ctrl[16];
    uint16_t width;
    uint16_t height;
    void*    backend;
} CsscTft;

CSSC_TFT_API CsscVal  cssc_tft_create(const char* ctrl, uint16_t w, uint16_t h);

CSSC_TFT_API CsscVal  cssc_tft_create_pins(const char* ctrl, uint16_t w, uint16_t h,
                                            int16_t sda, int16_t scl, uint8_t addr);
CSSC_TFT_API void     cssc_tft_destroy(CsscVal v);
CSSC_TFT_API void     cssc_tft_begin(CsscVal v);
CSSC_TFT_API void     cssc_tft_fill(CsscVal v, uint32_t color);
CSSC_TFT_API void     cssc_tft_clear(CsscVal v);
CSSC_TFT_API void     cssc_tft_pixel(CsscVal v, int16_t x, int16_t y, uint32_t color);
CSSC_TFT_API void     cssc_tft_line(CsscVal v, int16_t x0, int16_t y0,
                                     int16_t x1, int16_t y1, uint32_t color);
CSSC_TFT_API void     cssc_tft_rect(CsscVal v, int16_t x, int16_t y,
                                     int16_t w, int16_t h, uint32_t color);
CSSC_TFT_API void     cssc_tft_fillrect(CsscVal v, int16_t x, int16_t y,
                                         int16_t w, int16_t h, uint32_t color);
CSSC_TFT_API void     cssc_tft_text(CsscVal v, int16_t x, int16_t y,
                                     const char* s, uint32_t color, uint8_t scale);
CSSC_TFT_API void     cssc_tft_show(CsscVal v);
CSSC_TFT_API uint16_t cssc_tft_width(CsscVal v);
CSSC_TFT_API uint16_t cssc_tft_height(CsscVal v);

#ifdef __cplusplus
}
#endif
#endif
