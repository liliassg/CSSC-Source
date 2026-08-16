/*
 * cssc_host_extras.c — v6-native runtime extensions.
 *
 * Adds the host-game-facing symbols that cssc_host_game.c doesn't cover:
 *   * Video primitives layered on cssc_video_pixel:
 *       - cssc_video_fillrect(p, x, y, w, h, argb)
 *       - cssc_video_draw_rect(p, x, y, w, h, argb)      (outlined)
 *       - cssc_video_darken(p, amount01)                  (multiplicative)
 *       - cssc_video_draw_text(p, x, y, cstr, argb)      (8x8 bitmap font)
 *   * Sprite sheet with frame indexing:
 *       - cssc_sprite_sheet_load(path, fw, fh, transparent_argb) -> sheet*
 *       - cssc_sprite_sheet_draw_frame(sheet, video, x, y, frame_idx)
 *       - cssc_sprite_sheet_free(sheet)
 *   * Tilemap camera + tileset overloads:
 *       - cssc_tm_tileset_load(path, tile_w, tile_h) -> tileset*  (sheet ptr)
 *       - cssc_tm_create_from_tileset(tileset, cols, rows) -> tmap*
 *       - cssc_camera_new() -> cam*
 *       - cssc_camera_free(cam)
 *       - cssc_camera_set_bounds(cam, x, y, w, h)
 *       - cssc_camera_follow(cam, tx, ty, view_w, view_h)
 *       - cssc_camera_x(cam) -> i64
 *       - cssc_camera_y(cam) -> i64
 *       - cssc_tm_draw_cam(tmap, video, cam)
 *   * Sound extension:
 *       - cssc_sound_loop(handle, path) — starts a looping playback
 *       - cssc_sound_pause(handle, path)
 *       - cssc_sound_resume(handle, path)
 *   * Devdebug + console sync:
 *       - cssc_devdebug_push(msg) — appends to internal log + stderr
 *       - cssc_devdebug_stdout() -> cssc_str* (all entries joined)
 *       - cssc_console_sync(cons, tag, ptr_source)
 *       - cssc_console_desync(cons, tag)
 *
 * All video / sprite / tilemap symbols reach the raw framebuffer via
 * cssc_video_pixel / cssc_video_get_pixel (already provided by
 * cssc_host_game.c). We do NOT poke Win32 GDI directly — that keeps the
 * extras portable to any host build target where cssc_host_game.c has
 * its own concrete backend (Win32 today; X11/etc. later without rewrites).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define CSSC_EX_EXPORT __declspec(dllexport)
    #include <windows.h>
    #include <mmsystem.h>
#elif defined(__GNUC__) || defined(__clang__)
    #define CSSC_EX_EXPORT __attribute__((visibility("default")))
#else
    #define CSSC_EX_EXPORT
#endif

/* Provided by cssc_host_game.c */
extern void    cssc_video_pixel(void* p, int64_t x, int64_t y, int64_t argb);
extern int64_t cssc_video_get_pixel(void* p, int64_t x, int64_t y);
extern void*   cssc_video_hwnd(void* v);
extern uint32_t* cssc_video_backbuf(void* v);
/* Provided by the transembly runtime.ll. Signature: (const char*, size_t). */
extern void* cssc_string_lit(const char* data, size_t len);

/* system::run(cmd) — run a shell command, return its exit code. Used by the
 * compiled CSSC installer to drive copy / pip / PATH steps. */
CSSC_EX_EXPORT int64_t cssc_system_run(const char* cmd) {
    if (!cmd) return -1;
    return (int64_t)system(cmd);
}

/* system::capture(cmd) — run a command HIDDEN (no console flash) and return
 * its combined stdout+stderr as a heap string. Synchronous. Used by the IDE
 * to capture `cssc analyze` output for in-editor diagnostics. */
CSSC_EX_EXPORT void* cssc_system_capture(const char* cmd) {
#if defined(_WIN32)
    if (!cmd) return cssc_string_lit("", 0);
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
    HANDLE rd = NULL, wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return cssc_string_lit("", 0);
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr; si.hStdError = wr; si.hStdInput = NULL;
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    char cl[4200]; snprintf(cl, sizeof(cl), "%s", cmd);
    BOOL ok = CreateProcessA(NULL, cl, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, NULL, &si, &pi);
    CloseHandle(wr);
    if (!ok) { CloseHandle(rd); return cssc_string_lit("", 0); }
    CloseHandle(pi.hThread);
    char* buf = NULL; size_t len = 0, cap = 0;
    char tmp[4096]; DWORD got = 0;
    while (ReadFile(rd, tmp, sizeof(tmp), &got, NULL) && got > 0) {
        if (len + got + 1 > cap) {
            size_t nc = (len + got + 1) * 2;
            char* nb = (char*)realloc(buf, nc);
            if (!nb) break;
            buf = nb; cap = nc;
        }
        memcpy(buf + len, tmp, got); len += got;
    }
    CloseHandle(rd);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    void* r = cssc_string_lit(buf ? buf : "", len);
    if (buf) free(buf);
    return r;
#else
    (void)cmd;
    return cssc_string_lit("", 0);
#endif
}

/* --- ASYNC capture: spawn a hidden command, drain its output over several
 * frames without ever blocking the UI thread. The IDE uses this for the
 * debounced diagnostics / ownership passes so a slow analyzer on a large file
 * no longer stalls rendering. Flow: h = captureAsync(cmd); each frame poll(h)
 * (0 = still running, 1 = finished); when 1, take(h) returns the output and
 * frees the handle. ---------------------------------------------------------- */
#if defined(_WIN32)
typedef struct {
    HANDLE rd;        /* read end of the child's stdout/stderr pipe */
    HANDLE proc;      /* child process handle */
    char*  buf;
    size_t len, cap;
    int    done;
} cssc_async_cap;

/* Drain whatever bytes are currently available on the pipe (never blocks). */
static void cssc_async_drain(cssc_async_cap* a) {
    for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(a->rd, NULL, 0, NULL, &avail, NULL)) return;
        if (avail == 0) return;
        if (a->len + avail + 1 > a->cap) {
            size_t nc = (a->len + avail + 1) * 2;
            char* nb = (char*)realloc(a->buf, nc);
            if (!nb) return;
            a->buf = nb; a->cap = nc;
        }
        DWORD got = 0;
        if (!ReadFile(a->rd, a->buf + a->len, avail, &got, NULL) || got == 0) return;
        a->len += got;
    }
}
#endif

CSSC_EX_EXPORT void* cssc_system_capture_async(const char* cmd) {
#if defined(_WIN32)
    if (!cmd) return NULL;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
    HANDLE rd = NULL, wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return NULL;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr; si.hStdError = wr; si.hStdInput = NULL;
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    char cl[4200]; snprintf(cl, sizeof(cl), "%s", cmd);
    BOOL ok = CreateProcessA(NULL, cl, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, NULL, &si, &pi);
    CloseHandle(wr);
    if (!ok) { CloseHandle(rd); return NULL; }
    CloseHandle(pi.hThread);
    cssc_async_cap* a = (cssc_async_cap*)calloc(1, sizeof(cssc_async_cap));
    if (!a) { CloseHandle(rd); CloseHandle(pi.hProcess); return NULL; }
    a->rd = rd; a->proc = pi.hProcess;
    return a;
#else
    (void)cmd; return NULL;
#endif
}

/* 1 = finished (output ready for take), 0 = still running. NULL handle -> 1. */
CSSC_EX_EXPORT int64_t cssc_system_capture_poll(void* handle) {
#if defined(_WIN32)
    cssc_async_cap* a = (cssc_async_cap*)handle;
    if (!a) return 1;
    if (a->done) return 1;
    cssc_async_drain(a);
    if (WaitForSingleObject(a->proc, 0) == WAIT_OBJECT_0) {
        cssc_async_drain(a);          /* final flush after exit */
        a->done = 1;
    }
    return a->done;
#else
    (void)handle; return 1;
#endif
}

/* Return the captured output as a heap string and free the handle. Call once,
 * after poll() has returned 1. NULL handle -> "". */
CSSC_EX_EXPORT void* cssc_system_capture_take(void* handle) {
#if defined(_WIN32)
    cssc_async_cap* a = (cssc_async_cap*)handle;
    if (!a) return cssc_string_lit("", 0);
    if (!a->done) cssc_async_drain(a);
    void* r = cssc_string_lit(a->buf ? a->buf : "", a->len);
    if (a->rd) CloseHandle(a->rd);
    if (a->proc) CloseHandle(a->proc);
    if (a->buf) free(a->buf);
    free(a);
    return r;
#else
    (void)handle; return cssc_string_lit("", 0);
#endif
}

/* --- Serial (COM) support for the IDE's Embedded tab. Handles are the raw
 * HANDLE (opaque i64 to CSSC; 0 = closed/failed). Reads are non-blocking:
 * comRead returns whatever is buffered right now, "" if nothing. ---------- */
CSSC_EX_EXPORT void* cssc_serial_scan(void) {
#if defined(_WIN32)
    char* names = (char*)malloc(65536);
    if (!names) return cssc_string_lit("", 0);
    DWORD n = QueryDosDeviceA(NULL, names, 65536);
    char* out = NULL; size_t olen = 0, ocap = 0;
    if (n > 0) {
        const char* p = names;
        while (*p) {
            size_t l = strlen(p);
            if (l >= 4 && p[0] == 'C' && p[1] == 'O' && p[2] == 'M' &&
                p[3] >= '0' && p[3] <= '9') {
                if (olen + l + 2 > ocap) {
                    size_t nc = (olen + l + 2) * 2 + 64;
                    char* nb = (char*)realloc(out, nc);
                    if (!nb) break;
                    out = nb; ocap = nc;
                }
                memcpy(out + olen, p, l); olen += l; out[olen++] = '\n';
            }
            p += l + 1;
        }
    }
    free(names);
    void* r = cssc_string_lit(out ? out : "", olen);
    if (out) free(out);
    return r;
#else
    return cssc_string_lit("", 0);
#endif
}
CSSC_EX_EXPORT void* cssc_serial_open(const char* port, int64_t baud) {
#if defined(_WIN32)
    if (!port) return NULL;
    char path[64]; snprintf(path, sizeof(path), "\\\\.\\%s", port);
    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    DCB dcb; ZeroMemory(&dcb, sizeof(dcb)); dcb.DCBlength = sizeof(dcb);
    if (GetCommState(h, &dcb)) {
        dcb.BaudRate = (DWORD)(baud > 0 ? baud : 115200);
        dcb.ByteSize = 8; dcb.Parity = NOPARITY; dcb.StopBits = ONESTOPBIT;
        SetCommState(h, &dcb);
    }
    COMMTIMEOUTS to; ZeroMemory(&to, sizeof(to));
    to.ReadIntervalTimeout = MAXDWORD;   /* return immediately with whatever's there */
    SetCommTimeouts(h, &to);
    return h;
#else
    (void)port; (void)baud; return NULL;
#endif
}
CSSC_EX_EXPORT void* cssc_serial_read(void* h) {
#if defined(_WIN32)
    if (!h) return cssc_string_lit("", 0);
    char buf[4096]; DWORD got = 0;
    if (ReadFile((HANDLE)h, buf, sizeof(buf), &got, NULL) && got > 0)
        return cssc_string_lit(buf, (size_t)got);
    return cssc_string_lit("", 0);
#else
    (void)h; return cssc_string_lit("", 0);
#endif
}
CSSC_EX_EXPORT void cssc_serial_write(void* h, const char* text) {
#if defined(_WIN32)
    if (!h || !text) return;
    DWORD wr = 0; WriteFile((HANDLE)h, text, (DWORD)strlen(text), &wr, NULL);
#else
    (void)h; (void)text;
#endif
}
CSSC_EX_EXPORT void cssc_serial_close(void* h) {
#if defined(_WIN32)
    if (h) CloseHandle((HANDLE)h);
#else
    (void)h;
#endif
}

/* system::color(attr) — set the console text colour (Win32 attribute word;
 * e.g. 13 = bright magenta, 7 = default grey, 15 = bright white). Works on
 * every Windows console without VT — used by the CSSC console installer for
 * its purple/magenta look. Non-Windows: no-op. */
CSSC_EX_EXPORT void cssc_console_color(int64_t attr) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h && h != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(h, (WORD)(attr & 0xFFFF));
#else
    (void)attr;
#endif
}

/* Mirror of the header of cssc_video_t in cssc_host_game.c. The full
 * struct is opaque to us but the first four fields — w, h, fps, is_open —
 * are declared int32_t (NOT int64_t). Reading them with the wrong width
 * produces catastrophically-large values that make bound loops run for
 * billions of iterations (this bit us: a fillrect / darken pixel-scan
 * appeared to hang the app because vw ended up as ~2.75e12 instead of
 * 960 — width bits merged with height bits from the adjacent field).
 * MUST stay in sync with the typedef in cssc_host_game.c line ~151. */
struct cssc_video_hdr {
    int32_t w;
    int32_t h;
    int32_t fps;
    int32_t is_open;
};

static inline int64_t vid_w(void* p) {
    if (!p) return 0;
    return (int64_t)((struct cssc_video_hdr*)p)->w;
}
static inline int64_t vid_h(void* p) {
    if (!p) return 0;
    return (int64_t)((struct cssc_video_hdr*)p)->h;
}

/* Per-pixel alpha compositor. src is ARGB (alpha in top byte);
 * dst is what the framebuffer currently holds (BGRA byte layout via
 * cssc_video_get_pixel's return). Returns the blended value in the
 * same ARGB layout so cssc_video_pixel round-trips it correctly.
 *
 * alpha=0xFF → src wins (fast opaque path)
 * alpha=0x00 → dst wins (skip)
 * else       → linear src/dst blend, per-channel, alpha-preserving.
 *
 * Mirrors the interpreter's fill_rect_alpha semantics in
 * cssl_video_native.py so identical ARGB literals render identically. */
static inline int64_t ex_blend_argb(int64_t src_argb, int64_t dst_stored) {
    /* src is user-supplied ARGB (R at bit 16, B at bit 0).
     * dst_stored is what cssc_video_get_pixel returned — the raw
     * framebuffer uint32. bgra_of is now identity, so the stored value
     * is true ARGB: R at bit 16, B at bit 0, same layout as src.
     *
     * We blend the R/G/B channels, then return an ARGB value;
     * cssc_video_pixel's (identity) bgra_of writes it to storage. */
    uint32_t s = (uint32_t)src_argb;
    uint32_t d = (uint32_t)dst_stored;
    uint32_t sa = (s >> 24) & 0xFF;
    if (sa >= 0xFF) return src_argb;
    if (sa == 0) {
        uint32_t dr_ = (d >> 16) & 0xFF;
        uint32_t dg_ = (d >>  8) & 0xFF;
        uint32_t db_ = (d      ) & 0xFF;
        return (int64_t)((0xFFu << 24) | (dr_ << 16) | (dg_ << 8) | db_);
    }
    uint32_t sr = (s >> 16) & 0xFF;
    uint32_t sg = (s >>  8) & 0xFF;
    uint32_t sb = (s      ) & 0xFF;
    uint32_t dr = (d >> 16) & 0xFF;
    uint32_t dg = (d >>  8) & 0xFF;
    uint32_t db = (d      ) & 0xFF;
    uint32_t inv = 255 - sa;
    uint32_t r = (sr * sa + dr * inv) / 255;
    uint32_t g = (sg * sa + dg * inv) / 255;
    uint32_t b = (sb * sa + db * inv) / 255;
    return (int64_t)((0xFFu << 24) | (r << 16) | (g << 8) | b);
}

/* ---------------------------------------------------------------------------
 * video::fillrect(v, x, y, w, h, argb) — alpha-aware. When the argb's
 * alpha byte is < 0xFF and > 0, each pixel gets blended over the current
 * framebuffer content. Matches the interpreter's fill_rect_alpha path.
 * ------------------------------------------------------------------------- */
CSSC_EX_EXPORT void cssc_video_fillrect(void* v, int64_t x, int64_t y,
                                          int64_t w, int64_t h, int64_t argb) {
    if (!v) return;
    int64_t vw = vid_w(v);
    int64_t vh = vid_h(v);
    if (vw <= 0 || vh <= 0) return;
    uint32_t alpha = ((uint32_t)argb >> 24) & 0xFF;
    if (alpha == 0) return;
    int64_t x0 = x < 0 ? 0 : x;
    int64_t y0 = y < 0 ? 0 : y;
    int64_t x1 = x + w; if (x1 > vw) x1 = vw;
    int64_t y1 = y + h; if (y1 > vh) y1 = vh;
    if (alpha >= 0xFF) {
        for (int64_t py = y0; py < y1; ++py) {
            for (int64_t px = x0; px < x1; ++px) {
                cssc_video_pixel(v, px, py, argb);
            }
        }
    } else {
        for (int64_t py = y0; py < y1; ++py) {
            for (int64_t px = x0; px < x1; ++px) {
                int64_t dst = cssc_video_get_pixel(v, px, py);
                int64_t out = ex_blend_argb(argb, dst);
                cssc_video_pixel(v, px, py, out);
            }
        }
    }
}

/* ---------------------------------------------------------------------------
 * video::drawRect(v, x, y, w, h, argb) — outlined rect, 1px stroke
 * ------------------------------------------------------------------------- */
CSSC_EX_EXPORT void cssc_video_draw_rect(void* v, int64_t x, int64_t y,
                                           int64_t w, int64_t h, int64_t argb) {
    if (!v || w <= 0 || h <= 0) return;
    int64_t vw = vid_w(v);
    int64_t vh = vid_h(v);
    int64_t x0 = x < 0 ? 0 : x;
    int64_t y0 = y < 0 ? 0 : y;
    int64_t x1 = x + w - 1; if (x1 >= vw) x1 = vw - 1;
    int64_t y1 = y + h - 1; if (y1 >= vh) y1 = vh - 1;
    if (x1 < x0 || y1 < y0) return;
    for (int64_t px = x0; px <= x1; ++px) {
        if (y >= 0 && y < vh) cssc_video_pixel(v, px, y, argb);
        if (y + h - 1 >= 0 && y + h - 1 < vh)
            cssc_video_pixel(v, px, y + h - 1, argb);
    }
    for (int64_t py = y0; py <= y1; ++py) {
        if (x >= 0 && x < vw) cssc_video_pixel(v, x, py, argb);
        if (x + w - 1 >= 0 && x + w - 1 < vw)
            cssc_video_pixel(v, x + w - 1, py, argb);
    }
}

/* ---------------------------------------------------------------------------
 * video::darken(v, amount) — multiplicative dim of the whole viewport.
 *   amount is 0..1 (0 = no change, 1 = pitch black). Values outside get
 *   clamped. Alpha channel preserved.
 * ------------------------------------------------------------------------- */
CSSC_EX_EXPORT void cssc_video_darken(void* v, double amount) {
    if (!v) return;
    if (amount <= 0.0) return;
    if (amount > 1.0) amount = 1.0;
    double keep = 1.0 - amount;
    int64_t vw = vid_w(v);
    int64_t vh = vid_h(v);
    /* The framebuffer stores true ARGB (bgra_of is identity): R at bit
     * 16, G at bit 8, B at bit 0. darken scales every channel by the same
     * factor. We operate DIRECTLY on the backbuffer in one tight loop —
     * the old version issued two exported, non-inlined calls (get_pixel +
     * pixel) per pixel, ~1.23M calls/frame at 960x640, which capped fps and
     * added variable per-frame cost. Same arithmetic (double truncation),
     * just no call overhead and sequential cache-friendly access. */
    uint32_t* buf = cssc_video_backbuf(v);
    if (!buf) return;
    size_t count = (size_t)vw * (size_t)vh;
    for (size_t i = 0; i < count; ++i) {
        uint32_t p = buf[i];
        uint32_t a  = p & 0xFF000000u;
        uint32_t bh = (uint32_t)(((double)((p >> 16) & 0xFF)) * keep);
        uint32_t bm = (uint32_t)(((double)((p >>  8) & 0xFF)) * keep);
        uint32_t bl = (uint32_t)(((double)((p      ) & 0xFF)) * keep);
        buf[i] = a | (bh << 16) | (bm << 8) | bl;
    }
}

/* ---------------------------------------------------------------------------
 * 8x8 bitmap font — public-domain "IBM VGA" ASCII subset (32..127).
 * Each glyph is 8 bytes (one per row, top->bottom); bit 0 of each byte is
 * the LEFTMOST pixel (LSB-first, as the blitter tests `row & (1u << gx)`).
 * Only printable ASCII 32..126 is populated; other codepoints render as
 * a hollow square placeholder. This is the CANONICAL CSSC font ROM — the
 * interpreter mirrors these exact bytes in cssl_font8x8.py.
 * ------------------------------------------------------------------------- */
static const uint8_t CSSC_FONT8x8[96][8] = {
    {0,0,0,0,0,0,0,0},                       /* ' ' */
    {0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00}, /* '!' */
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, /* '"' */
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, /* '#' */
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, /* '$' */
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, /* '%' */
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, /* '&' */
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, /* '\'' */
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, /* '(' */
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, /* ')' */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* '*' */
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, /* '+' */
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, /* ',' */
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, /* '-' */
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, /* '.' */
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, /* '/' */
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, /* '0' */
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, /* '1' */
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, /* '2' */
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, /* '3' */
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, /* '4' */
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, /* '5' */
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, /* '6' */
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, /* '7' */
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, /* '8' */
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, /* '9' */
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, /* ':' */
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, /* ';' */
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, /* '<' */
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, /* '=' */
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, /* '>' */
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, /* '?' */
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, /* '@' */
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, /* 'A' */
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, /* 'B' */
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, /* 'C' */
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, /* 'D' */
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, /* 'E' */
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, /* 'F' */
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, /* 'G' */
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, /* 'H' */
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, /* 'I' */
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, /* 'J' */
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, /* 'K' */
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, /* 'L' */
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, /* 'M' */
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, /* 'N' */
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, /* 'O' */
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, /* 'P' */
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, /* 'Q' */
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, /* 'R' */
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, /* 'S' */
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, /* 'T' */
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, /* 'U' */
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, /* 'V' */
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, /* 'W' */
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, /* 'X' */
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, /* 'Y' */
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, /* 'Z' */
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, /* '[' */
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, /* '\\' */
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, /* ']' */
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, /* '^' */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, /* '_' */
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, /* '`' */
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, /* 'a' */
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, /* 'b' */
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, /* 'c' */
    {0x38,0x30,0x30,0x3e,0x33,0x33,0x6E,0x00}, /* 'd' */
    {0x00,0x00,0x1E,0x33,0x3f,0x03,0x1E,0x00}, /* 'e' */
    {0x1C,0x36,0x06,0x0f,0x06,0x06,0x0F,0x00}, /* 'f' */
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, /* 'g' */
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, /* 'h' */
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, /* 'i' */
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, /* 'j' */
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, /* 'k' */
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, /* 'l' */
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, /* 'm' */
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00}, /* 'n' */
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, /* 'o' */
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, /* 'p' */
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, /* 'q' */
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, /* 'r' */
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, /* 's' */
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, /* 't' */
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, /* 'u' */
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, /* 'v' */
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00}, /* 'w' */
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, /* 'x' */
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, /* 'y' */
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, /* 'z' */
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, /* '{' */
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, /* '|' */
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, /* '}' */
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, /* '~' */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* DEL */
};

static void ex_draw_glyph(void* v, int64_t x, int64_t y,
                             int ch, int64_t argb, int64_t scale) {
    if (scale < 1) scale = 1;
    if (ch < 32 || ch > 127) {
        for (int gy = 0; gy < 8; ++gy) {
            for (int gx = 0; gx < 8; ++gx) {
                if (gy == 0 || gy == 7 || gx == 0 || gx == 7) {
                    for (int64_t sy = 0; sy < scale; ++sy) {
                        for (int64_t sx = 0; sx < scale; ++sx) {
                            cssc_video_pixel(v,
                                x + (int64_t)gx * scale + sx,
                                y + (int64_t)gy * scale + sy, argb);
                        }
                    }
                }
            }
        }
        return;
    }
    const uint8_t* g = CSSC_FONT8x8[ch - 32];
    for (int gy = 0; gy < 8; ++gy) {
        uint8_t row = g[gy];
        for (int gx = 0; gx < 8; ++gx) {
            if (row & (0x01u << gx)) {
                for (int64_t sy = 0; sy < scale; ++sy) {
                    for (int64_t sx = 0; sx < scale; ++sx) {
                        cssc_video_pixel(v,
                            x + (int64_t)gx * scale + sx,
                            y + (int64_t)gy * scale + sy, argb);
                    }
                }
            }
        }
    }
}

CSSC_EX_EXPORT void cssc_video_draw_text(void* v, int64_t x, int64_t y,
                                           const char* text, int64_t argb,
                                           int64_t scale) {
    if (!v || !text) return;
    if (scale < 1) scale = 1;
    int64_t cx = x;
    int64_t glyph_w = 8 * scale;
    int64_t line_h = 10 * scale;
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        if (*p == '\n') { cx = x; y += line_h; continue; }
        ex_draw_glyph(v, cx, y, (int)*p, argb, scale);
        cx += glyph_w;
    }
}

/* ---------------------------------------------------------------------------
 * Sprite sheet — sprite* backed by a single big BMP + fixed frame size.
 * frame_idx addresses a col-major cell in the sheet (frames wrap by width).
 * ------------------------------------------------------------------------- */
typedef struct cssc_sheet {
    int64_t sheet_w;      /* full sheet in pixels */
    int64_t sheet_h;
    int64_t frame_w;
    int64_t frame_h;
    int64_t cols;         /* sheet_w / frame_w */
    uint32_t transparent; /* argb; matching pixels skipped on draw */
    uint32_t* pixels;
} cssc_sheet;

/* Provided by cssc_host_game.c — sprite BMP loader keyed off the same
 * runtime, so we get whatever platform-native loader it uses (GDI+ on
 * Windows). We stay layered by pulling sprite pixels via the accessor. */
extern void* cssc_sprite_load(const char* path);
extern int64_t cssc_sprite_width(void* p);
extern int64_t cssc_sprite_height(void* p);
extern int64_t cssc_sprite_get_pixel(void* p, int64_t x, int64_t y);
extern void  cssc_sprite_free(void* p);

CSSC_EX_EXPORT void* cssc_sprite_sheet_load(const char* path,
                                              int64_t frame_w, int64_t frame_h,
                                              int64_t transparent_argb) {
    if (!path || frame_w <= 0 || frame_h <= 0) return NULL;
    void* sprite = cssc_sprite_load(path);
    if (!sprite) return NULL;
    int64_t sw = cssc_sprite_width(sprite);
    int64_t sh = cssc_sprite_height(sprite);
    cssc_sheet* s = (cssc_sheet*)malloc(sizeof(cssc_sheet));
    if (!s) { cssc_sprite_free(sprite); return NULL; }
    s->sheet_w = sw;
    s->sheet_h = sh;
    s->frame_w = frame_w;
    s->frame_h = frame_h;
    s->cols = frame_w > 0 ? sw / frame_w : 0;
    s->transparent = (uint32_t)transparent_argb;
    s->pixels = (uint32_t*)malloc((size_t)(sw * sh) * sizeof(uint32_t));
    if (!s->pixels) {
        free(s);
        cssc_sprite_free(sprite);
        return NULL;
    }
    for (int64_t yy = 0; yy < sh; ++yy) {
        for (int64_t xx = 0; xx < sw; ++xx) {
            s->pixels[yy * sw + xx] = (uint32_t)cssc_sprite_get_pixel(sprite, xx, yy);
        }
    }
    cssc_sprite_free(sprite);
    return s;
}

CSSC_EX_EXPORT void cssc_sprite_sheet_free(void* p) {
    if (!p) return;
    cssc_sheet* s = (cssc_sheet*)p;
    free(s->pixels);
    free(s);
}

CSSC_EX_EXPORT void cssc_sprite_sheet_draw_frame(void* sheet_p, void* v,
                                                    int64_t dst_x, int64_t dst_y,
                                                    int64_t frame_idx) {
    if (!sheet_p || !v) return;
    cssc_sheet* s = (cssc_sheet*)sheet_p;
    if (s->cols <= 0) return;
    int64_t col = frame_idx % s->cols;
    int64_t row = frame_idx / s->cols;
    int64_t src_x0 = col * s->frame_w;
    int64_t src_y0 = row * s->frame_h;
    if (src_y0 + s->frame_h > s->sheet_h) return;
    for (int64_t fy = 0; fy < s->frame_h; ++fy) {
        for (int64_t fx = 0; fx < s->frame_w; ++fx) {
            uint32_t px = s->pixels[(src_y0 + fy) * s->sheet_w
                                     + (src_x0 + fx)];
            if (px == s->transparent) continue;
            cssc_video_pixel(v, dst_x + fx, dst_y + fy, (int64_t)px);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Tilemap camera + tileset overloads
 * ------------------------------------------------------------------------- */
typedef struct cssc_camera {
    int64_t x, y;           /* current view origin (top-left, world px) */
    int64_t bx, by;         /* bound rect origin */
    int64_t bw, bh;         /* bound rect size (world px) */
    int     has_bounds;
} cssc_camera;

CSSC_EX_EXPORT void* cssc_camera_new(void) {
    cssc_camera* c = (cssc_camera*)calloc(1, sizeof(cssc_camera));
    return c;
}

CSSC_EX_EXPORT void cssc_camera_free(void* p) {
    free(p);
}

CSSC_EX_EXPORT void cssc_camera_set_bounds(void* p, int64_t x, int64_t y,
                                              int64_t w, int64_t h) {
    if (!p) return;
    cssc_camera* c = (cssc_camera*)p;
    c->bx = x; c->by = y; c->bw = w; c->bh = h; c->has_bounds = 1;
}

CSSC_EX_EXPORT void cssc_camera_follow(void* p, int64_t tx, int64_t ty,
                                         int64_t view_w, int64_t view_h) {
    if (!p) return;
    cssc_camera* c = (cssc_camera*)p;
    int64_t nx = tx - view_w / 2;
    int64_t ny = ty - view_h / 2;
    if (c->has_bounds) {
        int64_t max_x = c->bx + c->bw - view_w;
        int64_t max_y = c->by + c->bh - view_h;
        if (max_x < c->bx) max_x = c->bx;
        if (max_y < c->by) max_y = c->by;
        if (nx < c->bx) nx = c->bx;
        if (nx > max_x) nx = max_x;
        if (ny < c->by) ny = c->by;
        if (ny > max_y) ny = max_y;
    }
    c->x = nx;
    c->y = ny;
}

CSSC_EX_EXPORT int64_t cssc_camera_x(void* p) {
    return p ? ((cssc_camera*)p)->x : 0;
}

CSSC_EX_EXPORT int64_t cssc_camera_y(void* p) {
    return p ? ((cssc_camera*)p)->y : 0;
}

/* Tileset — same shape as a sprite sheet, so we reuse cssc_sheet + a
 * distinct constructor. The tilemap holds an int table of tile-ids and
 * blits from the tileset per (col, row). */
CSSC_EX_EXPORT void* cssc_tm_tileset_load(const char* path,
                                            int64_t tile_w, int64_t tile_h) {
    return cssc_sprite_sheet_load(path, tile_w, tile_h, 0);
}

typedef struct cssc_tmap {
    int64_t cols, rows;
    int64_t tile_w, tile_h;
    cssc_sheet* tileset;
    int32_t* tiles;      /* cols*rows tile indices; 0 = empty/first tile */
} cssc_tmap;

CSSC_EX_EXPORT void* cssc_tm_create_from_tileset(void* tileset,
                                                    int64_t cols, int64_t rows) {
    if (!tileset || cols <= 0 || rows <= 0) return NULL;
    cssc_tmap* t = (cssc_tmap*)malloc(sizeof(cssc_tmap));
    if (!t) return NULL;
    cssc_sheet* s = (cssc_sheet*)tileset;
    t->cols = cols;
    t->rows = rows;
    t->tile_w = s->frame_w;
    t->tile_h = s->frame_h;
    t->tileset = s;
    t->tiles = (int32_t*)calloc((size_t)(cols * rows), sizeof(int32_t));
    if (!t->tiles) { free(t); return NULL; }
    return t;
}

/* Note: cssc_host_game.c already declares cssc_tm_set / cssc_tm_get /
 * cssc_tm_cols / cssc_tm_rows / cssc_tm_free / cssc_tm_new for the
 * older-style tilemap (that one uses per-tile sprite bindings). To keep
 * both callable, the tilemap constructor from a tileset returns a struct
 * with the SAME initial 32-byte layout so cssc_tm_cols / cssc_tm_rows
 * still work — see the struct order above. */

CSSC_EX_EXPORT void cssc_tm_set_at(void* p, int64_t col, int64_t row,
                                     int64_t tile_id) {
    if (!p) return;
    cssc_tmap* t = (cssc_tmap*)p;
    if (col < 0 || row < 0 || col >= t->cols || row >= t->rows) return;
    t->tiles[row * t->cols + col] = (int32_t)tile_id;
}

CSSC_EX_EXPORT void cssc_tm_draw_cam(void* map_p, void* v, void* cam_p) {
    if (!map_p || !v) return;
    cssc_tmap* t = (cssc_tmap*)map_p;
    if (!t->tileset || !t->tiles) return;
    int64_t cam_x = cam_p ? ((cssc_camera*)cam_p)->x : 0;
    int64_t cam_y = cam_p ? ((cssc_camera*)cam_p)->y : 0;
    int64_t vw = vid_w(v);
    int64_t vh = vid_h(v);

    int64_t col_first = cam_x / t->tile_w;
    int64_t row_first = cam_y / t->tile_h;
    int64_t col_last  = (cam_x + vw) / t->tile_w + 1;
    int64_t row_last  = (cam_y + vh) / t->tile_h + 1;
    if (col_first < 0) col_first = 0;
    if (row_first < 0) row_first = 0;
    if (col_last > t->cols) col_last = t->cols;
    if (row_last > t->rows) row_last = t->rows;

    for (int64_t ry = row_first; ry < row_last; ++ry) {
        for (int64_t rx = col_first; rx < col_last; ++rx) {
            int32_t tile_id = t->tiles[ry * t->cols + rx];
            int64_t dst_x = rx * t->tile_w - cam_x;
            int64_t dst_y = ry * t->tile_h - cam_y;
            cssc_sprite_sheet_draw_frame(t->tileset, v,
                                           dst_x, dst_y, (int64_t)tile_id);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Sound extensions — loop / pause / resume.
 * On Windows we lean on the MCI API (mciSendStringW) for arbitrary WAV/MP3
 * playback with pause/resume; on other platforms sound is a no-op today,
 * matching the base cssc_sound runtime's non-Windows fallback.
 * ------------------------------------------------------------------------- */
#if defined(_MSC_VER) || defined(__MINGW32__)
static void ex_mci(const char* command) {
    if (!command) return;
    int len = MultiByteToWideChar(CP_UTF8, 0, command, -1, NULL, 0);
    if (len <= 0) return;
    wchar_t* wcmd = (wchar_t*)malloc((size_t)len * sizeof(wchar_t));
    if (!wcmd) return;
    MultiByteToWideChar(CP_UTF8, 0, command, -1, wcmd, len);
    mciSendStringW(wcmd, NULL, 0, NULL);
    free(wcmd);
}
#else
static void ex_mci(const char* command) { (void)command; }
#endif

static const char* ex_short_alias(const char* path) {
    if (!path) return "sfx";
    const char* p = path;
    for (const char* q = path; *q; ++q) if (*q == '/' || *q == '\\') p = q + 1;
    static char buf[64];
    size_t n = strlen(p);
    if (n >= sizeof(buf)) n = sizeof(buf) - 1;
    size_t j = 0;
    for (size_t i = 0; i < n && j < sizeof(buf) - 1; ++i) {
        char c = p[i];
        if (isalnum((unsigned char)c) || c == '_') buf[j++] = c;
    }
    if (j == 0) buf[j++] = 's';
    buf[j] = '\0';
    return buf;
}

CSSC_EX_EXPORT void cssc_sound_loop(void* handle, const char* path) {
    (void)handle;
    if (!path) return;
    const char* alias = ex_short_alias(path);
    char open_cmd[512];
    char play_cmd[128];
    snprintf(open_cmd, sizeof(open_cmd),
             "open \"%s\" type mpegvideo alias %s", path, alias);
    snprintf(play_cmd, sizeof(play_cmd), "play %s repeat", alias);
    ex_mci(open_cmd);
    ex_mci(play_cmd);
}

CSSC_EX_EXPORT void cssc_sound_pause(void* handle, const char* path) {
    (void)handle;
    if (!path) return;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "pause %s", ex_short_alias(path));
    ex_mci(cmd);
}

CSSC_EX_EXPORT void cssc_sound_resume(void* handle, const char* path) {
    (void)handle;
    if (!path) return;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "resume %s", ex_short_alias(path));
    ex_mci(cmd);
}

/* ---------------------------------------------------------------------------
 * Global-default sound handle — matches the interpreter's `snd::play(path)`
 * (no-handle) usage pattern. Lazily created via cssc_sound_new so we don't
 * depend on the user having created a handle first.
 * ------------------------------------------------------------------------- */
extern void* cssc_sound_new(void);
extern void* cssc_sound_load(void* handle, const char* path);
extern void  cssc_sound_play(void* handle, const char* path);
extern void  cssc_sound_stop(void* handle, const char* path);
extern void  cssc_sound_stop_all(void* handle);
extern void  cssc_sound_set_volume(void* handle, int64_t vol);
extern int64_t cssc_sound_is_playing(void* handle, const char* path);

static void* g_default_sound = NULL;

static void* ex_default_sound(void) {
    if (!g_default_sound) {
        g_default_sound = cssc_sound_new();
    }
    return g_default_sound;
}

CSSC_EX_EXPORT void* cssc_sound_default_load(const char* path) {
    return cssc_sound_load(ex_default_sound(), path);
}
CSSC_EX_EXPORT void cssc_sound_default_play(const char* path) {
    if (!path) return;
    /* PlaySoundA (base cssc_sound_play) only handles WAV. For MP3 / OGG
     * / other formats, route through MCI (mciSendString) which auto-
     * detects the format from the file extension. Same alias scheme as
     * cssc_sound_loop so subsequent stop/pause resolves the right
     * voice. Warms the lazy default handle so cssc_sound_default_*
     * stays consistent for volume changes / isPlaying queries. */
    (void)ex_default_sound();
    const char* alias = ex_short_alias(path);
    /* `close alias` cleans up any prior instance under the same alias
     * (idempotent — silently no-ops if the alias is fresh). This lets
     * the user call snd::play multiple times on the same track and
     * always restart from the beginning without a "alias already
     * assigned" MCI error blocking the reopen. */
    char close_cmd[128];
    snprintf(close_cmd, sizeof(close_cmd), "close %s", alias);
    ex_mci(close_cmd);
    char open_cmd[512];
    char play_cmd[128];
    snprintf(open_cmd, sizeof(open_cmd),
             "open \"%s\" alias %s", path, alias);
    snprintf(play_cmd, sizeof(play_cmd), "play %s from 0", alias);
    ex_mci(open_cmd);
    ex_mci(play_cmd);
}
CSSC_EX_EXPORT void cssc_sound_default_stop(const char* path) {
    cssc_sound_stop(ex_default_sound(), path);
}
CSSC_EX_EXPORT void cssc_sound_default_stop_all(void) {
    /* `stop all` closes every MCI voice we opened (matches "all sound
     * off" semantics) AND falls through to the base PlaySoundA purge
     * for anything the base runtime is playing. Without the MCI close,
     * an intro track opened by cssc_sound_default_play kept playing
     * because PlaySoundA's SND_PURGE only stops the WAV voice — MCI's
     * MPEG dispatcher is invisible to it. */
    ex_mci("close all");
    cssc_sound_stop_all(ex_default_sound());
}
CSSC_EX_EXPORT void cssc_sound_default_set_volume(int64_t vol) {
    cssc_sound_set_volume(ex_default_sound(), vol);
}
CSSC_EX_EXPORT int64_t cssc_sound_default_is_playing(const char* path) {
    return cssc_sound_is_playing(ex_default_sound(), path);
}
CSSC_EX_EXPORT void cssc_sound_default_loop(const char* path) {
    cssc_sound_loop(ex_default_sound(), path);
}
CSSC_EX_EXPORT void cssc_sound_default_pause(const char* path) {
    cssc_sound_pause(ex_default_sound(), path);
}
CSSC_EX_EXPORT void cssc_sound_default_resume(const char* path) {
    cssc_sound_resume(ex_default_sound(), path);
}

/* ---------------------------------------------------------------------------
 * Global-default keyboard handle.
 *
 * Focus gating: when the last-bound video window is NOT the foreground
 * window, every input query returns 0 (isPressed / isHeld / isReleased)
 * and updates are skipped. Without this the game responded to keys
 * typed in OTHER applications (notepad, another shell, browser) — a
 * known GetAsyncKeyState footgun. The recommendation matches every
 * mainstream engine (SDL / GLFW / raylib).
 * ------------------------------------------------------------------------- */
extern void* cssc_keyboard_new(void);
extern void  cssc_keyboard_update(void* kb);
extern int64_t cssc_keyboard_is_pressed(void* kb, const char* key);
extern int64_t cssc_keyboard_is_down(void* kb, const char* key);
extern int64_t cssc_keyboard_is_released(void* kb, const char* key);
extern void* cssc_keyboard_get_key(void* kb);

static void* g_default_keyboard = NULL;
/* g_last_bound_video is defined later (mouse section) — forward
 * declaration so the focus-gate can peek at it. */
static void* g_last_bound_video;

static void* ex_default_keyboard(void) {
    if (!g_default_keyboard) {
        g_default_keyboard = cssc_keyboard_new();
    }
    return g_default_keyboard;
}

static int ex_trace_on(void) {
    static int cached = -1;
    if (cached < 0) {
        const char* v = getenv("CSSC_TRACE_INPUT");
        cached = (v && v[0] && v[0] != '0') ? 1 : 0;
    }
    return cached;
}

/* ---------------------------------------------------------------------------
 * SEH crash reporter: on any unhandled Win32 exception, dump the code +
 * fault address + instruction pointer to stderr BEFORE the process dies.
 * Installed once at program start (via a global-ctor-like mechanism —
 * actually installed lazily on the first ex_video_focused call, since
 * that's guaranteed to run early in any interactive session). Zero cost
 * unless a crash actually fires.
 * ------------------------------------------------------------------------- */
#if defined(_MSC_VER) || defined(__MINGW32__)
static LONG WINAPI ex_seh_filter(EXCEPTION_POINTERS* ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    void* rip = ep->ExceptionRecord->ExceptionAddress;
    const char* name = "?";
    switch (code) {
        case 0xC0000005: name = "ACCESS_VIOLATION"; break;
        case 0xC000001D: name = "ILLEGAL_INSTRUCTION"; break;
        case 0xC0000094: name = "INTEGER_DIVIDE_BY_ZERO"; break;
        case 0xC00000FD: name = "STACK_OVERFLOW"; break;
        case 0xC0000090: name = "FLOAT_INVALID_OPERATION"; break;
    }
    fprintf(stderr, "\n[SEH-CRASH] code=0x%08lX (%s) at RIP=%p\n",
            (unsigned long)code, name, rip);
    if (code == 0xC0000005 &&
            ep->ExceptionRecord->NumberParameters >= 2) {
        ULONG_PTR type = ep->ExceptionRecord->ExceptionInformation[0];
        ULONG_PTR addr = ep->ExceptionRecord->ExceptionInformation[1];
        fprintf(stderr,
            "[SEH-CRASH] %s address 0x%p (kind=%s)\n",
            (type == 0) ? "reading" : (type == 1 ? "writing" : "exec-at"),
            (void*)addr,
            (type == 0) ? "read" : (type == 1 ? "write" : "execute"));
    }
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
static void ex_install_seh(void) {
    static LONG installed = 0;
    if (InterlockedCompareExchange(&installed, 1, 0) == 0) {
        SetUnhandledExceptionFilter(ex_seh_filter);
    }
}
#else
static void ex_install_seh(void) {}
#endif

CSSC_EX_EXPORT void cssc_install_crash_handler(void) {
    ex_install_seh();
}

static int ex_video_focused(void) {
    ex_install_seh();
#if defined(_MSC_VER) || defined(__MINGW32__)
    HWND fg = GetForegroundWindow();
    if (!fg) {
        if (ex_trace_on()) fprintf(stderr,
            "[trace-focus] GetForegroundWindow=NULL -> unfocused\n");
        return 0;
    }
    DWORD fg_pid = 0;
    GetWindowThreadProcessId(fg, &fg_pid);
    DWORD our_pid = GetCurrentProcessId();
    int focused = (fg_pid == our_pid) ? 1 : 0;
    if (ex_trace_on()) {
        static int last = -1;
        if (focused != last) {
            fprintf(stderr,
                "[trace-focus] fg_pid=%lu our_pid=%lu focused=%d\n",
                (unsigned long)fg_pid, (unsigned long)our_pid, focused);
            fflush(stderr);
            last = focused;
        }
    }
    return focused;
#else
    return 1;
#endif
}

CSSC_EX_EXPORT void cssc_keyboard_default_update(void) {
    static long calls = 0;
    calls++;
    if (!ex_video_focused()) {
        if (ex_trace_on() && (calls % 60 == 1))
            fprintf(stderr, "[trace-kb] update SKIPPED (unfocused) "
                            "call#%ld\n", calls);
        return;
    }
    cssc_keyboard_update(ex_default_keyboard());
    if (ex_trace_on() && (calls % 60 == 1)) {
        fprintf(stderr, "[trace-kb] update OK call#%ld\n", calls);
        fflush(stderr);
    }
}
CSSC_EX_EXPORT int64_t cssc_keyboard_default_is_pressed(const char* key) {
    if (!ex_video_focused()) {
        static long uf_ct = 0;
        uf_ct++;
        if (ex_trace_on() && (uf_ct % 60 == 1))
            fprintf(stderr, "[trace-kb] isPressed(%s)=0 (unfocused) x%ld\n",
                    key ? key : "?", uf_ct);
        return 0;
    }
    int64_t r = cssc_keyboard_is_pressed(ex_default_keyboard(), key);
    if (ex_trace_on()) {
        static long call_ct = 0;
        call_ct++;
        /* Sniff GetAsyncKeyState directly for the same key to compare
         * against what cssc_keyboard_is_pressed returns from k->curr.
         * Divergence means the update poll isn't landing the byte in
         * the array we're reading from — either update writes to a
         * different handle, or the VK lookup diverges between poll
         * and read. */
#if defined(_MSC_VER) || defined(__MINGW32__)
        int vk_live = 0;
        if (key) {
            /* Rough map same as cssc_keyboard_lookup_vk for a few
             * hot keys — no dependency on the runtime's own lookup. */
            if (key[0] == ' ' || key[0] == 's' || key[0] == 'S') {
                if (!key[1] || (key[1] == 'p' || key[1] == 'P'))
                    vk_live = 0x20;  /* VK_SPACE */
            }
            if (!vk_live) {
                if ((key[0] == 'e' || key[0] == 'E') && key[1])
                    vk_live = 0x1B; /* VK_ESCAPE */
                else if (key[0] == '1') vk_live = '1';
                else if (key[0] == '2') vk_live = '2';
                else if (key[0] == '3') vk_live = '3';
                else if (key[0] == 'A' || key[0] == 'a') vk_live = 'A';
            }
        }
        int live = 0;
        if (vk_live) {
            SHORT s = GetAsyncKeyState(vk_live);
            live = (s & 0x8000) ? 1 : 0;
        }
        if (r != 0 || live != 0 || (call_ct % 60 == 1)) {
            fprintf(stderr,
                "[trace-kb] isPressed(%s) return=%lld  live_GAKS=%d  "
                "vk_probed=0x%02x  focused=1\n",
                key ? key : "?", (long long)r, live, vk_live);
            fflush(stderr);
        }
#else
        (void)call_ct;
#endif
    }
    return r;
}
CSSC_EX_EXPORT int64_t cssc_keyboard_default_is_down(const char* key) {
    if (!ex_video_focused()) return 0;
    return cssc_keyboard_is_down(ex_default_keyboard(), key);
}
CSSC_EX_EXPORT int64_t cssc_keyboard_default_is_released(const char* key) {
    if (!ex_video_focused()) return 0;
    return cssc_keyboard_is_released(ex_default_keyboard(), key);
}
CSSC_EX_EXPORT void* cssc_keyboard_default_get_key(void) {
    if (!ex_video_focused()) return cssc_string_lit("", 0);
    return cssc_keyboard_get_key(ex_default_keyboard());
}

/* ---------------------------------------------------------------------------
 * devdebug — light log ring buffer + stderr echo.
 * ------------------------------------------------------------------------- */
#define CSSC_DEVDEBUG_CAP  1024

static char*   g_devdebug_log[CSSC_DEVDEBUG_CAP];
static int     g_devdebug_count = 0;
static int     g_devdebug_head  = 0;

CSSC_EX_EXPORT void cssc_devdebug_push(const char* msg) {
    if (!msg) return;
    if (g_devdebug_log[g_devdebug_head]) {
        free(g_devdebug_log[g_devdebug_head]);
    }
    size_t n = strlen(msg);
    char* copy = (char*)malloc(n + 1);
    if (!copy) return;
    memcpy(copy, msg, n + 1);
    g_devdebug_log[g_devdebug_head] = copy;
    g_devdebug_head = (g_devdebug_head + 1) % CSSC_DEVDEBUG_CAP;
    if (g_devdebug_count < CSSC_DEVDEBUG_CAP) g_devdebug_count++;
    fprintf(stderr, "[debug] %s\n", msg);
    fflush(stderr);
}

CSSC_EX_EXPORT void* cssc_devdebug_stdout(void) {
    /* Join the ring buffer into one string. Emit as a heap cssc_str*. */
    size_t total = 0;
    int start = (g_devdebug_head - g_devdebug_count + CSSC_DEVDEBUG_CAP)
                 % CSSC_DEVDEBUG_CAP;
    for (int i = 0; i < g_devdebug_count; ++i) {
        int idx = (start + i) % CSSC_DEVDEBUG_CAP;
        if (g_devdebug_log[idx]) total += strlen(g_devdebug_log[idx]) + 1;
    }
    char* buf = (char*)malloc(total + 1);
    if (!buf) return cssc_string_lit("", 0);
    size_t off = 0;
    for (int i = 0; i < g_devdebug_count; ++i) {
        int idx = (start + i) % CSSC_DEVDEBUG_CAP;
        if (!g_devdebug_log[idx]) continue;
        size_t n = strlen(g_devdebug_log[idx]);
        memcpy(buf + off, g_devdebug_log[idx], n);
        off += n;
        buf[off++] = '\n';
    }
    buf[off] = '\0';
    void* r = cssc_string_lit(buf, off);
    free(buf);
    return r;
}

/* ---------------------------------------------------------------------------
 * console sync/desync — record the tag → source-ptr association. The
 * console module reads from it every refresh cycle (implementation lives
 * in cssc_host_game.c; here we only expose the binding endpoints).
 *
 * The design: a fixed-size table of (tag, ptr) pairs. cons.sync('dev',
 * *dev::stdout) registers a live pointer that resolves via
 * cssc_devdebug_stdout each read.
 * ------------------------------------------------------------------------- */
#define CSSC_CONS_SYNC_CAP  8

typedef struct cssc_cons_binding {
    void* cons;
    char  tag[32];
    void* source;
    int   in_use;
} cssc_cons_binding;

static cssc_cons_binding g_cons_bindings[CSSC_CONS_SYNC_CAP];

/* Convenience alias — the interpreter uses cons.out(str) as the write
 * primitive. The compiler passes a cssc_str* (32-byte heap struct) as the
 * text arg, but the base `cssc_console_write` in cssc_host_game.c expects
 * a raw NUL-terminated C string. Extract the raw payload via
 * `cssc_string_data` before delegating. */
extern void cssc_console_write(void* cons, const char* text);
extern void* cssc_string_data(void* cssc_str);

CSSC_EX_EXPORT void cssc_console_out(void* cons, void* text) {
    /* _lower_gpio_method already extracts the raw char* via
     * cssc_string_data BEFORE the call, so `text` here IS a pointer
     * into the string's inline data buffer — NOT a cssc_str*. Re-
     * invoking cssc_string_data would GEP another +8 bytes and hand
     * garbage past-the-NUL to WriteConsoleA (the recent gpio-string
     * extraction fix landed after cssc_console_out was written, so
     * this was double-extracting until now). */
    if (!cons || !text) return;
    cssc_console_write(cons, (const char*)text);
}

/* video::open(w, title) — sets the window title if provided (Win32 only)
 * and defers to cssc_video_begin for the actual show. Non-Windows targets
 * silently drop the title. */
extern void cssc_video_begin(void* v);
extern void* cssc_video_hwnd(void* v);

CSSC_EX_EXPORT void cssc_video_open_titled(void* v, const char* title) {
    ex_install_seh();
#if defined(_MSC_VER) || defined(__MINGW32__)
    /* Mark the process DPI-aware BEFORE creating the window so the
     * requested pixel size (e.g. 960x640) maps 1:1 to physical pixels
     * on high-DPI displays. Without this the DWM compositor shrinks
     * the framebuffer visually — the app appears "tiny scaled".
     *
     * SetProcessDpiAwarenessContext (Win10 1703+) is preferred; the
     * SetProcessDPIAware fallback covers older builds. The static
     * flag ensures we only invoke once per process. */
    static int dpi_marked = 0;
    if (!dpi_marked) {
        dpi_marked = 1;
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (user32) {
            typedef BOOL (WINAPI *SetProcessDpiAwarenessContext_t)(HANDLE);
            SetProcessDpiAwarenessContext_t setDpiCtx =
                (SetProcessDpiAwarenessContext_t)GetProcAddress(
                    user32, "SetProcessDpiAwarenessContext");
            if (setDpiCtx) {
                /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = ((HANDLE)-4) */
                setDpiCtx((HANDLE)(intptr_t)-4);
            } else {
                SetProcessDPIAware();
            }
        }
    }
    cssc_video_begin(v);
    if (v) {
        HWND hwnd = (HWND)cssc_video_hwnd(v);
        if (hwnd) {
            if (title && *title) {
                int wlen = MultiByteToWideChar(CP_UTF8, 0, title, -1,
                                                  NULL, 0);
                if (wlen > 0) {
                    wchar_t* wtitle = (wchar_t*)malloc(
                        (size_t)wlen * sizeof(wchar_t));
                    if (wtitle) {
                        MultiByteToWideChar(CP_UTF8, 0, title, -1,
                                             wtitle, wlen);
                        SetWindowTextW(hwnd, wtitle);
                        free(wtitle);
                    }
                }
            }
            /* Force foreground so the user's game window actually gets
             * focus even if AllocConsole (for a #console) grabbed it
             * first. Also grabs input focus so kb::isPressed works
             * from the very first frame. */
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            BringWindowToTop(hwnd);
        }
    }
#else
    (void)title;
    cssc_video_begin(v);
#endif
}

CSSC_EX_EXPORT void cssc_console_sync(void* cons, const char* tag,
                                        void* source) {
    if (!cons || !tag) return;
    for (int i = 0; i < CSSC_CONS_SYNC_CAP; ++i) {
        if (g_cons_bindings[i].in_use
                && g_cons_bindings[i].cons == cons
                && strncmp(g_cons_bindings[i].tag, tag,
                            sizeof(g_cons_bindings[i].tag)) == 0) {
            g_cons_bindings[i].source = source;
            return;
        }
    }
    for (int i = 0; i < CSSC_CONS_SYNC_CAP; ++i) {
        if (!g_cons_bindings[i].in_use) {
            g_cons_bindings[i].cons = cons;
            g_cons_bindings[i].source = source;
            size_t tl = strlen(tag);
            if (tl >= sizeof(g_cons_bindings[i].tag))
                tl = sizeof(g_cons_bindings[i].tag) - 1;
            memcpy(g_cons_bindings[i].tag, tag, tl);
            g_cons_bindings[i].tag[tl] = '\0';
            g_cons_bindings[i].in_use = 1;
            return;
        }
    }
}

/* ---------------------------------------------------------------------------
 * cssc::input(prompt) — read one line from stdin. The prompt is printed
 * to stdout without a trailing newline; the return value is a heap
 * cssc_str* holding the line (without the trailing '\n' or '\r').
 * ------------------------------------------------------------------------- */
CSSC_EX_EXPORT void* cssc_input(const char* prompt) {
    if (prompt && *prompt) {
        fputs(prompt, stdout);
        fflush(stdout);
    }
    char buf[4096];
    if (!fgets(buf, sizeof(buf), stdin)) {
        return cssc_string_lit("", 0);
    }
    size_t n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
        buf[--n] = '\0';
    }
    return cssc_string_lit(buf, n);
}

/* ---------------------------------------------------------------------------
 * Global-default mouse — lazily creates a mouse handle on first use so
 * `mouse::update()` / `mouse::isClicked("left")` calls work without an
 * explicit handle. The handle is bound to the last video window opened
 * via cssc_video_new; if none exists we still return a handle whose
 * coordinate ops fall back to the OS-wide cursor position.
 * ------------------------------------------------------------------------- */
extern void* cssc_mouse_new(void* video);
extern void  cssc_mouse_free(void* mouse);
extern void  cssc_mouse_update(void* mouse);
extern int64_t cssc_mouse_get_x(void* mouse);
extern int64_t cssc_mouse_get_y(void* mouse);
extern void* cssc_mouse_get_pos(void* mouse);
extern int64_t cssc_mouse_is_clicked(void* mouse, const char* button);
extern int64_t cssc_mouse_is_held(void* mouse, const char* button);
extern int64_t cssc_mouse_is_released(void* mouse, const char* button);
extern int64_t cssc_mouse_is_over(void* mouse, int64_t x, int64_t y,
                                    int64_t w, int64_t h);

static void* g_default_mouse = NULL;
/* g_last_bound_video's forward-decl lives in the keyboard section (up
 * where ex_video_focused needs it); this is the actual definition. */

static void* ex_default_mouse(void) {
    if (!g_default_mouse) {
        g_default_mouse = cssc_mouse_new(g_last_bound_video);
    }
    return g_default_mouse;
}

/* Public: bind the default mouse to a specific video window. Called by
 * the compiler-emitted redirect for `mouse::client(video)` / `mouse::
 * attach(video)` so subsequent no-handle mouse queries (mouse::isClicked,
 * mouse::getX, …) work against the SAME mouse handle whose coordinates
 * the user set up via `mouse::client`. Without this, the default mouse
 * had no video ref and the bound `_mc` handle was the only one whose
 * state was kept fresh — user calls to bare `mouse::isClicked(...)`
 * returned stale zeros forever. */
CSSC_EX_EXPORT void* cssc_mouse_bind_default(void* video) {
    g_last_bound_video = video;
    if (g_default_mouse) {
        cssc_mouse_free(g_default_mouse);
        g_default_mouse = NULL;
    }
    void* m = ex_default_mouse();
    if (ex_trace_on()) {
        fprintf(stderr,
            "[trace-bind] mouse::client bound video=%p mouse=%p\n",
            video, m);
        fflush(stderr);
    }
    return m;
}

CSSC_EX_EXPORT void cssc_mouse_default_update(void) {
    /* Always poll position (getX/getY/getPos/isOver work whether or not
     * the app is focused — hover tracking is decoupled from click
     * detection). Button-state polling honours focus so the game doesn't
     * respond to clicks that landed on OTHER applications. */
    cssc_mouse_update(ex_default_mouse());
}
CSSC_EX_EXPORT int64_t cssc_mouse_default_get_x(void) {
    return cssc_mouse_get_x(ex_default_mouse());
}
CSSC_EX_EXPORT int64_t cssc_mouse_default_get_y(void) {
    return cssc_mouse_get_y(ex_default_mouse());
}
CSSC_EX_EXPORT void* cssc_mouse_default_get_pos(void) {
    return cssc_mouse_get_pos(ex_default_mouse());
}
CSSC_EX_EXPORT int64_t cssc_mouse_default_is_clicked(const char* button) {
    int foc = ex_video_focused();
    if (!foc) {
        static long uf_ct = 0;
        uf_ct++;
        if (ex_trace_on() && (uf_ct % 60 == 1))
            fprintf(stderr, "[trace-mouse] isClicked(%s)=0 (unfocused) x%ld\n",
                    button ? button : "?", uf_ct);
        return 0;
    }
    int64_t r = cssc_mouse_is_clicked(ex_default_mouse(), button);
    if (ex_trace_on()) {
        static long call_ct = 0;
        call_ct++;
#if defined(_MSC_VER) || defined(__MINGW32__)
        int vk = 0x01; /* VK_LBUTTON default */
        if (button && (button[0] == 'r' || button[0] == 'R')) vk = 0x02;
        else if (button && (button[0] == 'm' || button[0] == 'M')) vk = 0x04;
        SHORT s = GetAsyncKeyState(vk);
        int live = (s & 0x8000) ? 1 : 0;
        if (r != 0 || live != 0 || (call_ct % 60 == 1)) {
            fprintf(stderr,
                "[trace-mouse] isClicked(%s) return=%lld  live_GAKS=%d  "
                "handle=%p  focused=1\n",
                button ? button : "?", (long long)r, live,
                ex_default_mouse());
            fflush(stderr);
        }
#else
        (void)call_ct;
#endif
    }
    return r;
}
CSSC_EX_EXPORT int64_t cssc_mouse_default_is_held(const char* button) {
    if (!ex_video_focused()) return 0;
    return cssc_mouse_is_held(ex_default_mouse(), button);
}
CSSC_EX_EXPORT int64_t cssc_mouse_default_is_released(const char* button) {
    if (!ex_video_focused()) return 0;
    return cssc_mouse_is_released(ex_default_mouse(), button);
}
CSSC_EX_EXPORT int64_t cssc_mouse_default_is_over(int64_t x, int64_t y,
                                                    int64_t w, int64_t h) {
    return cssc_mouse_is_over(ex_default_mouse(), x, y, w, h);
}

CSSC_EX_EXPORT void cssc_console_desync(void* cons, const char* tag) {
    if (!cons || !tag) return;
    for (int i = 0; i < CSSC_CONS_SYNC_CAP; ++i) {
        if (g_cons_bindings[i].in_use
                && g_cons_bindings[i].cons == cons
                && strncmp(g_cons_bindings[i].tag, tag,
                            sizeof(g_cons_bindings[i].tag)) == 0) {
            g_cons_bindings[i].in_use = 0;
            g_cons_bindings[i].source = NULL;
            g_cons_bindings[i].cons = NULL;
            g_cons_bindings[i].tag[0] = '\0';
            return;
        }
    }
}

/* ------------------------------------------------------------------------- *
 * Mutier `%` shared pagetables — cross-runtime key/value memory.
 *
 * Byte layout is identical to the interpreter (cssl_cssc.py `_MutierTable`):
 * a 16-byte header [magic u32 'MUT1'=0x4D555431, count u32, cap u32, pad u32]
 * followed by `cap` entries of [tag i64, value i64]. Tag 0 = free/unset,
 * 1 = int, 2 = float (f64 bits), 3 = bool. Tables are OS named shared memory
 * ("cssc_mutier_public" / "cssc_mutier_<id>"), pure volatile RAM per the
 * locked design, so any concurrent CSSC runtime — compiled or interpreted —
 * that opens the same name sees the same entries. Each process keeps its
 * mapping open for its whole lifetime (slot cache) so the RAM stays live while
 * any runtime holds it; a per-table named mutex serialises the alloc counter.
 * ------------------------------------------------------------------------- */
#define CSSC_MUTIER_MAGIC 0x4D555432u          /* 'MUT2' — + heap region */
#define CSSC_MUTIER_CAP   65536
#define CSSC_MUTIER_HDR   32
#define CSSC_MUTIER_HEAP  (8 * 1024 * 1024)    /* variable-length value region */
/* entry tags: 0 free, 1 int, 2 float(f64 bits), 3 bool, 4 string(heap offset) */
#define CSSC_MUTIER_TAG_INT   1
#define CSSC_MUTIER_TAG_STR   4

typedef struct { uint32_t magic; uint32_t count; uint32_t cap; uint32_t pad;
                 int64_t heap_top; int64_t heap_cap; } cssc_mutier_hdr;
typedef struct { int64_t tag; int64_t value; } cssc_mutier_ent;

typedef struct {
    int            used;
    int            is_public;
    uint64_t       pid;
    unsigned char* base;
#if defined(_WIN32)
    HANDLE         map;
    HANDLE         mutex;
#else
    int            fd;
    void*          sem;
#endif
} cssc_mutier_slot;

static cssc_mutier_slot g_mutier_slots[256];

static size_t cssc_mutier_bytes(void) {
    return (size_t)CSSC_MUTIER_HDR + (size_t)CSSC_MUTIER_CAP * sizeof(cssc_mutier_ent)
           + (size_t)CSSC_MUTIER_HEAP;
}
#define CSSC_MUTIER_HEAP_BASE \
    ((size_t)CSSC_MUTIER_HDR + (size_t)CSSC_MUTIER_CAP * sizeof(cssc_mutier_ent))

/* Append length-prefixed bytes to a table's heap; return the start offset (or
 * -1 if full). Caller holds the table lock. Mirrors the interpreter's
 * `_heap_put` so a string written by either engine is read back by the other. */
static int64_t cssc_mutier_heap_put(unsigned char* base, const char* data, int64_t len) {
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)base;
    if (len < 0) len = 0;
    int64_t need = 4 + len;
    if (hd->heap_top + need > hd->heap_cap) return -1;
    unsigned char* p = base + CSSC_MUTIER_HEAP_BASE + hd->heap_top;
    uint32_t l32 = (uint32_t)len;
    memcpy(p, &l32, 4);
    if (len > 0 && data) memcpy(p + 4, data, (size_t)len);
    int64_t off = hd->heap_top;
    hd->heap_top += need;
    return off;
}

#if defined(_WIN32)
static cssc_mutier_slot* cssc_mutier_get(int is_public, uint64_t pid) {
    int i, free_slot = -1;
    for (i = 0; i < 256; i++) {
        if (g_mutier_slots[i].used) {
            if (g_mutier_slots[i].is_public == is_public
                    && (is_public || g_mutier_slots[i].pid == pid))
                return &g_mutier_slots[i];
        } else if (free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot < 0) return NULL;

    char name[64], mname[80];
    if (is_public) {
        snprintf(name, sizeof(name), "cssc_mutier_public");
        snprintf(mname, sizeof(mname), "cssc_mutex_public");
    } else {
        snprintf(name, sizeof(name), "cssc_mutier_%llu", (unsigned long long)pid);
        snprintf(mname, sizeof(mname), "cssc_mutex_%llu", (unsigned long long)pid);
    }
    size_t size = cssc_mutier_bytes();
    HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                  (DWORD)(size >> 32), (DWORD)(size & 0xFFFFFFFFu), name);
    if (!h) return NULL;
    int created_new = (GetLastError() != ERROR_ALREADY_EXISTS);
    unsigned char* base = (unsigned char*)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (!base) { CloseHandle(h); return NULL; }
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)base;
    if (created_new || hd->magic != CSSC_MUTIER_MAGIC) {
        hd->magic = CSSC_MUTIER_MAGIC; hd->count = 0; hd->cap = CSSC_MUTIER_CAP; hd->pad = 0;
        hd->heap_top = 0; hd->heap_cap = CSSC_MUTIER_HEAP;
    }
    cssc_mutier_slot* s = &g_mutier_slots[free_slot];
    s->is_public = is_public; s->pid = pid; s->base = base;
    s->map = h; s->mutex = CreateMutexA(NULL, FALSE, mname);
    s->used = 1;
    return s;
}

static void cssc_mutier_lock(cssc_mutier_slot* s)   { if (s && s->mutex) WaitForSingleObject(s->mutex, 5000); }
static void cssc_mutier_unlock(cssc_mutier_slot* s) { if (s && s->mutex) ReleaseMutex(s->mutex); }
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
static cssc_mutier_slot* cssc_mutier_get(int is_public, uint64_t pid) {
    int i, free_slot = -1;
    for (i = 0; i < 256; i++) {
        if (g_mutier_slots[i].used) {
            if (g_mutier_slots[i].is_public == is_public
                    && (is_public || g_mutier_slots[i].pid == pid))
                return &g_mutier_slots[i];
        } else if (free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot < 0) return NULL;

    char name[64], sname[80];
    if (is_public) {
        snprintf(name, sizeof(name), "/cssc_mutier_public");
        snprintf(sname, sizeof(sname), "/cssc_mutex_public");
    } else {
        snprintf(name, sizeof(name), "/cssc_mutier_%llu", (unsigned long long)pid);
        snprintf(sname, sizeof(sname), "/cssc_mutex_%llu", (unsigned long long)pid);
    }
    size_t size = cssc_mutier_bytes();
    int created_new = 0;
    int fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd >= 0) { created_new = 1; if (ftruncate(fd, (off_t)size) != 0) { close(fd); return NULL; } }
    else { fd = shm_open(name, O_RDWR, 0600); if (fd < 0) return NULL; }
    unsigned char* base = (unsigned char*)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) { close(fd); return NULL; }
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)base;
    if (created_new || hd->magic != CSSC_MUTIER_MAGIC) {
        hd->magic = CSSC_MUTIER_MAGIC; hd->count = 0; hd->cap = CSSC_MUTIER_CAP; hd->pad = 0;
        hd->heap_top = 0; hd->heap_cap = CSSC_MUTIER_HEAP;
    }
    cssc_mutier_slot* s = &g_mutier_slots[free_slot];
    s->is_public = is_public; s->pid = pid; s->base = base; s->fd = fd;
    s->sem = (void*)sem_open(sname, O_CREAT, 0600, 1);
    s->used = 1;
    return s;
}

static void cssc_mutier_lock(cssc_mutier_slot* s)   { if (s && s->sem && s->sem != SEM_FAILED) sem_wait((sem_t*)s->sem); }
static void cssc_mutier_unlock(cssc_mutier_slot* s) { if (s && s->sem && s->sem != SEM_FAILED) sem_post((sem_t*)s->sem); }
#endif

CSSC_EX_EXPORT int64_t cssc_mutier_alloc(int64_t is_public, int64_t pid, int64_t value, int64_t tag) {
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return -1;
    cssc_mutier_lock(s);
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)s->base;
    int64_t addr = -1;
    if (hd->count < (uint32_t)CSSC_MUTIER_CAP) {
        cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + hd->count;
        e->tag = (tag == 0 ? 1 : tag); e->value = value;
        addr = (int64_t)hd->count;
        hd->count += 1;
    }
    cssc_mutier_unlock(s);
    return addr;
}

CSSC_EX_EXPORT int64_t cssc_mutier_read(int64_t is_public, int64_t pid, int64_t addr) {
    if (addr < 0 || addr >= CSSC_MUTIER_CAP) return 0;
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return 0;
    cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + addr;
    return e->tag ? e->value : 0;
}

CSSC_EX_EXPORT void cssc_mutier_write(int64_t is_public, int64_t pid, int64_t addr,
                                      int64_t value, int64_t tag) {
    if (addr < 0 || addr >= CSSC_MUTIER_CAP) return;
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return;
    cssc_mutier_lock(s);
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)s->base;
    cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + addr;
    e->tag = (tag == 0 ? 1 : tag); e->value = value;
    if ((uint32_t)addr >= hd->count) hd->count = (uint32_t)addr + 1;
    cssc_mutier_unlock(s);
}

CSSC_EX_EXPORT void cssc_mutier_free(int64_t is_public, int64_t pid, int64_t addr) {
    if (addr < 0 || addr >= CSSC_MUTIER_CAP) return;
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return;
    cssc_mutier_lock(s);
    cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + addr;
    e->tag = 0;
    cssc_mutier_unlock(s);
}

/* String pagetable values — spill the bytes to the table heap and tag the entry
 * TAG_STR so any runtime (compiled or interpreted) reads them back as a string. */
CSSC_EX_EXPORT int64_t cssc_mutier_alloc_str(int64_t is_public, int64_t pid,
                                             const char* data, int64_t len) {
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return -1;
    cssc_mutier_lock(s);
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)s->base;
    int64_t addr = -1;
    if (hd->count < (uint32_t)CSSC_MUTIER_CAP) {
        int64_t off = cssc_mutier_heap_put(s->base, data, len);
        if (off >= 0) {
            cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + hd->count;
            e->tag = CSSC_MUTIER_TAG_STR; e->value = off;
            addr = (int64_t)hd->count;
            hd->count += 1;
        }
    }
    cssc_mutier_unlock(s);
    return addr;
}

CSSC_EX_EXPORT void cssc_mutier_write_str(int64_t is_public, int64_t pid, int64_t addr,
                                          const char* data, int64_t len) {
    if (addr < 0 || addr >= CSSC_MUTIER_CAP) return;
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return;
    cssc_mutier_lock(s);
    cssc_mutier_hdr* hd = (cssc_mutier_hdr*)s->base;
    int64_t off = cssc_mutier_heap_put(s->base, data, len);
    if (off >= 0) {
        cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + addr;
        e->tag = CSSC_MUTIER_TAG_STR; e->value = off;
        if ((uint32_t)addr >= hd->count) hd->count = (uint32_t)addr + 1;
    }
    cssc_mutier_unlock(s);
}

CSSC_EX_EXPORT void* cssc_mutier_read_str(int64_t is_public, int64_t pid, int64_t addr) {
    if (addr < 0 || addr >= CSSC_MUTIER_CAP) return cssc_string_lit("", 0);
    cssc_mutier_slot* s = cssc_mutier_get((int)is_public, (uint64_t)pid);
    if (!s) return cssc_string_lit("", 0);
    cssc_mutier_ent* e = (cssc_mutier_ent*)(s->base + CSSC_MUTIER_HDR) + addr;
    if (e->tag != CSSC_MUTIER_TAG_STR) return cssc_string_lit("", 0);
    unsigned char* p = s->base + CSSC_MUTIER_HEAP_BASE + (size_t)e->value;
    uint32_t l32; memcpy(&l32, p, 4);
    return cssc_string_lit((const char*)(p + 4), (size_t)l32);
}

/* cssc::run(file) — execute a .cssc file through the CSSC "run" interpreter as
 * a child process; return 1 on success (child exit 0), 0 otherwise. Compiled
 * programs have no embedded interpreter, so this shells out to the toolchain
 * launcher. It probes `cssc` first (the shipped launcher) and falls back to
 * `csscd` (the dev entry point) so the same binary works in either install.
 * The path arrives as (data, len) — CSSC strings are not null-terminated — so
 * it is copied into a null-terminated buffer before use. */
CSSC_EX_EXPORT int64_t cssc_run_cssc(const char* path_data, int64_t path_len) {
    if (path_len < 0) path_len = 0;
    if (path_len > 900) path_len = 900;
    char path[1024];
    if (path_len > 0 && path_data) memcpy(path, path_data, (size_t)path_len);
    path[path_len] = '\0';
    /* Missing file -> failure, matching the interpreter's os.path.isfile guard
     * (the `cssc run` CLI itself prints a message but still exits 0). */
    FILE* probe = fopen(path, "rb");
    if (!probe) return 0;
    fclose(probe);
    const char* launcher = "cssc";
#if defined(_WIN32)
    if (system("cssc --version >nul 2>&1") != 0) launcher = "csscd";
#else
    if (system("cssc --version >/dev/null 2>&1") != 0) launcher = "csscd";
#endif
    char cmd[1200];
    snprintf(cmd, sizeof(cmd), "%s run \"%s\"", launcher, path);
    int rc = system(cmd);
    return (rc == 0) ? 1 : 0;
}

/* cssc::ipcall / cssc::ipto — computed control flow. `addr` is a function
 * ADDRESS (what `func[1]` / `cssc::ip()` yield: in a native build that is the
 * literal function pointer). Cast it back and call it. RAW by design: the
 * target is invoked as a no-arg int-returning function (CSSC's uniform call
 * ABI for address dispatch); passing an address that is not a function is
 * undefined, exactly like a raw jump. `pcto`'s "don't return" is emitted by the
 * compiler as this call followed by the caller's own `ret`. */
CSSC_EX_EXPORT int64_t cssc_ipcall(int64_t addr) {
    if (!addr) return 0;
    int64_t (*fn)(void) = (int64_t (*)(void))(intptr_t)addr;
    return fn();
}

/* String case/trim transforms for the v6 native backend (the emitted IR runtime
 * has size/get_byte/starts_with but not these). Each returns a fresh cssc_str. */
extern int64_t cssc_string_size(void* cssc_str);

/* colors module: print (data,len) wrapped in a 24-bit truecolor SGR escape.
 * `newline` appends '\n'. Enables VT on legacy Windows consoles once so the
 * escape renders instead of showing as raw text. */
CSSC_EX_EXPORT void cssc_color_out(const char* data, int64_t len,
                                   int64_t hexcol, int64_t newline) {
    static int vt = 0;
    if (!vt) {
        vt = 1;
#if defined(_WIN32)
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode)) SetConsoleMode(h, mode | 0x0004);
#endif
    }
    int r = (int)((hexcol >> 16) & 0xFF);
    int g = (int)((hexcol >> 8) & 0xFF);
    int b = (int)(hexcol & 0xFF);
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    if (len > 0 && data) fwrite(data, 1, (size_t)len, stdout);
    fputs("\033[0m", stdout);
    if (newline) putchar('\n');
    fflush(stdout);
}

CSSC_EX_EXPORT void* cssc_string_upper(void* s) {
    const char* d = (const char*)cssc_string_data(s);
    int64_t n = cssc_string_size(s);
    if (n <= 0 || !d) return cssc_string_lit("", 0);
    char* buf = (char*)malloc((size_t)n);
    if (!buf) return cssc_string_lit(d, (size_t)n);
    for (int64_t i = 0; i < n; i++) {
        char c = d[i]; buf[i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    }
    void* r = cssc_string_lit(buf, (size_t)n);
    free(buf);
    return r;
}

CSSC_EX_EXPORT void* cssc_string_lower(void* s) {
    const char* d = (const char*)cssc_string_data(s);
    int64_t n = cssc_string_size(s);
    if (n <= 0 || !d) return cssc_string_lit("", 0);
    char* buf = (char*)malloc((size_t)n);
    if (!buf) return cssc_string_lit(d, (size_t)n);
    for (int64_t i = 0; i < n; i++) {
        char c = d[i]; buf[i] = (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
    }
    void* r = cssc_string_lit(buf, (size_t)n);
    free(buf);
    return r;
}

CSSC_EX_EXPORT void* cssc_string_trim(void* s) {
    const char* d = (const char*)cssc_string_data(s);
    int64_t n = cssc_string_size(s);
    if (n <= 0 || !d) return cssc_string_lit("", 0);
    int64_t a = 0, b = n;
    while (a < b && (d[a] == ' ' || d[a] == '\t' || d[a] == '\n' || d[a] == '\r')) a++;
    while (b > a && (d[b-1] == ' ' || d[b-1] == '\t' || d[b-1] == '\n' || d[b-1] == '\r')) b--;
    return cssc_string_lit(d + a, (size_t)(b - a));
}

/* Replace every occurrence of `oldp` with `newp` in `s`. Returns a fresh str. */
CSSC_EX_EXPORT void* cssc_string_replace(void* s, void* oldp, void* newp) {
    const char* d = (const char*)cssc_string_data(s);   int64_t n  = cssc_string_size(s);
    const char* o = (const char*)cssc_string_data(oldp); int64_t on = cssc_string_size(oldp);
    const char* w = (const char*)cssc_string_data(newp); int64_t wn = cssc_string_size(newp);
    if (on <= 0 || n <= 0 || !d) return cssc_string_lit(d ? d : "", (size_t)(n > 0 ? n : 0));
    int64_t count = 0;
    for (int64_t i = 0; i + on <= n; ) {
        if (memcmp(d + i, o, (size_t)on) == 0) { count++; i += on; } else i++;
    }
    int64_t outlen = n + count * (wn - on);
    if (outlen < 0) outlen = 0;
    char* buf = (char*)malloc((size_t)(outlen > 0 ? outlen : 1));
    if (!buf) return cssc_string_lit(d, (size_t)n);
    int64_t oi = 0;
    for (int64_t i = 0; i < n; ) {
        if (i + on <= n && memcmp(d + i, o, (size_t)on) == 0) {
            if (wn > 0) memcpy(buf + oi, w, (size_t)wn);
            oi += wn; i += on;
        } else buf[oi++] = d[i++];
    }
    void* r = cssc_string_lit(buf, (size_t)oi);
    free(buf);
    return r;
}
