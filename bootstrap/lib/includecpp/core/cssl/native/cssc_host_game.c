

#ifdef CSSC_HOST

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601
    #endif
    #include <windows.h>
    #include <mmsystem.h>
    #include <io.h>

#else
    #include <unistd.h>
    #include <time.h>
    #include <sys/time.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define CSSC_WEAK   __attribute__((weak))
    #define CSSC_UNUSED __attribute__((unused))
#else
    #define CSSC_WEAK
    #define CSSC_UNUSED
#endif

#ifndef CSSC_INLINE
    #define CSSC_INLINE static inline
#endif

extern void* cssc_string_lit(const void* src, int64_t len);
extern void* cssc_map_ii_new(int64_t cap_hint);
extern void  cssc_map_ii_set(void* m, int64_t key, int64_t val);

CSSC_WEAK void blit_bitmap(uint32_t* dst, int64_t dst_w, int64_t dst_h,
                           int64_t dst_x, int64_t dst_y,
                           const uint32_t* src, int64_t src_w, int64_t src_h);
CSSC_WEAK void blit_bitmap_scaled(uint32_t* dst, int64_t dst_w, int64_t dst_h,
                                  int64_t dst_x, int64_t dst_y,
                                  const uint32_t* src, int64_t src_w, int64_t src_h,
                                  int64_t scale);
CSSC_WEAK void blit_bitmap_tinted(uint32_t* dst, int64_t dst_w, int64_t dst_h,
                                  int64_t dst_x, int64_t dst_y,
                                  const uint32_t* src, int64_t src_w, int64_t src_h,
                                  uint32_t tint_argb);
CSSC_WEAK void blit_bitmap_alpha(uint32_t* dst, int64_t dst_w, int64_t dst_h,
                                 int64_t dst_x, int64_t dst_y,
                                 const uint32_t* src, int64_t src_w, int64_t src_h,
                                 int64_t alpha_255);
CSSC_WEAK void clear_framebuffer(uint32_t* dst, int64_t w, int64_t h, uint32_t argb);
CSSC_WEAK void fill_rect_native(uint32_t* dst, int64_t w, int64_t h,
                                int64_t x, int64_t y, int64_t rw, int64_t rh,
                                uint32_t argb);

CSSC_INLINE uint32_t bgra_of(int64_t argb) {
    uint32_t v = (uint32_t)(argb & 0xFFFFFFFF);
    uint32_t a = (v >> 24) & 0xFFu;
    uint32_t r = (v >> 16) & 0xFFu;
    uint32_t g = (v >>  8) & 0xFFu;
    uint32_t b = (v      ) & 0xFFu;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

extern uint64_t cssc_rng_state;

typedef struct {
    int32_t   w;
    int32_t   h;
    int32_t   fps;
    volatile int32_t is_open;
    uint32_t* backbuf;
    int64_t   next_deadline;

    int       charq[256]; int chead, ctail;
    int       keyq[256];  int khead, ktail;
    int       wheel_accum;
#ifdef _WIN32
    HWND      hwnd;
    HDC       hdc;
    BITMAPINFO bmi;
    ATOM      wc_atom;
#endif
} cssc_video_t;

#ifdef _WIN32

static LRESULT CALLBACK cssc_video_wndproc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp) {
    cssc_video_t* v = (cssc_video_t*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CLOSE:
        if (v) v->is_open = 0;
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (v) v->is_open = 0;
        return 0;
    case WM_ERASEBKGND:

        return 1;
    case WM_CHAR:

        if (v && wp) {
            int nt = (v->ctail + 1) & 255;
            v->charq[v->ctail] = (int)wp;
            v->ctail = nt;
            if (v->ctail == v->chead) v->chead = (v->chead + 1) & 255;
        }
        return 0;
    case WM_KEYDOWN:

        if (v) {
            int vk = (int)wp;
            int ctrl_down = (GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0;
            if (vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
                vk == VK_DELETE || vk == VK_HOME || vk == VK_END ||
                vk == VK_PRIOR || vk == VK_NEXT ||

                (ctrl_down && (vk == VK_OEM_PLUS || vk == VK_OEM_MINUS ||
                               vk == VK_ADD || vk == VK_SUBTRACT))) {
                int nt = (v->ktail + 1) & 255;
                v->keyq[v->ktail] = vk;
                v->ktail = nt;
                if (v->ktail == v->khead) v->khead = (v->khead + 1) & 255;
            }
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (v) v->wheel_accum += GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

static const wchar_t CSSC_VIDEO_CLASS[] = L"CsscVideoWindow";

#endif

#ifdef _WIN32

static void cssc_mkdirs_for(wchar_t* fullpath) {
    for (wchar_t* p = fullpath + 1; *p; ++p) {
        if (*p == L'\\' || *p == L'/') {
            wchar_t c = *p; *p = 0;
            CreateDirectoryW(fullpath, NULL);
            *p = c;
        }
    }
}

static void cssc_bootstrap_assets(void) {
    static int done = 0;
    if (done) return;
    done = 1;

    wchar_t exe_path[1024];
    DWORD n = GetModuleFileNameW(NULL, exe_path, 1024);
    if (n == 0 || n >= 1024) return;

    wchar_t exe_dir[1024];
    wcsncpy(exe_dir, exe_path, 1023); exe_dir[1023] = 0;
    for (DWORD i = n; i > 0; --i) {
        if (exe_dir[i - 1] == L'\\' || exe_dir[i - 1] == L'/') {
            exe_dir[i - 1] = 0; break;
        }
    }

    int extracted = 0;
    FILE* f = _wfopen(exe_path, L"rb");
    if (f) {
        unsigned char footer[16];
        if (_fseeki64(f, -16, SEEK_END) == 0 &&
            fread(footer, 1, 16, f) == 16 &&
            memcmp(footer + 8, "CSSCPK01", 8) == 0) {
            unsigned long long arch_size;
            memcpy(&arch_size, footer, 8);
            if (arch_size > 0 &&
                _fseeki64(f, -(long long)(16 + arch_size), SEEK_END) == 0) {
                wchar_t root[1024];
                DWORD tn = GetTempPathW(800, root);
                if (tn > 0 && tn < 800) {

                    const wchar_t* eb = exe_path;
                    for (const wchar_t* q = exe_path; *q; ++q)
                        if (*q == L'\\' || *q == L'/') eb = q + 1;
                    size_t rl = wcslen(root);
                    if (rl < 1000) { root[rl++] = L'c'; root[rl++] = L's';
                                     root[rl++] = L's'; root[rl++] = L'c';
                                     root[rl++] = L'_'; }
                    for (; *eb && *eb != L'.' && rl < 1000; ++eb) {
                        wchar_t c = *eb;
                        int an = (c>=L'0'&&c<=L'9')||(c>=L'A'&&c<=L'Z')||(c>=L'a'&&c<=L'z');
                        root[rl++] = an ? c : L'_';
                    }
                    root[rl] = 0;
                    CreateDirectoryW(root, NULL);

                    unsigned long long rem = arch_size;
                    int ok = 1;
                    while (rem >= 12 && ok) {
                        unsigned int plen;
                        if (fread(&plen, 1, 4, f) != 4) { ok = 0; break; }
                        rem -= 4;
                        if (plen == 0 || plen > 1024 ||
                            (unsigned long long)plen > rem) { ok = 0; break; }
                        char pbuf[1025];
                        if (fread(pbuf, 1, plen, f) != plen) { ok = 0; break; }
                        pbuf[plen] = 0; rem -= plen;
                        unsigned long long dlen;
                        if (fread(&dlen, 1, 8, f) != 8) { ok = 0; break; }
                        rem -= 8;
                        if (dlen > rem) { ok = 0; break; }
                        FILE* out = NULL;
                        wchar_t wrel[1024];
                        if (MultiByteToWideChar(CP_UTF8, 0, pbuf, -1, wrel, 1024) > 0) {
                            for (wchar_t* p = wrel; *p; ++p)
                                if (*p == L'/') *p = L'\\';
                            wchar_t dest[2048];
                            wcscpy(dest, root); wcscat(dest, L"\\");
                            wcscat(dest, wrel);
                            cssc_mkdirs_for(dest);
                            out = _wfopen(dest, L"wb");
                        }
                        unsigned long long left = dlen;
                        static char cbuf[65536];
                        while (left > 0) {
                            size_t chunk = left > sizeof(cbuf)
                                           ? sizeof(cbuf) : (size_t)left;
                            size_t rd = fread(cbuf, 1, chunk, f);
                            if (rd == 0) { ok = 0; break; }
                            if (out) fwrite(cbuf, 1, rd, out);
                            left -= rd;
                        }
                        if (out) fclose(out);
                        rem -= (dlen - left);
                        if (left > 0) ok = 0;
                    }
                    if (ok) { SetCurrentDirectoryW(root); extracted = 1; }
                }
            }
        }
        fclose(f);
    }
    if (!extracted) SetCurrentDirectoryW(exe_dir);
}
#endif

void* cssc_video_new(int64_t w, int64_t h, int64_t fps) {
#ifdef _WIN32

    cssc_bootstrap_assets();
#endif
    cssc_video_t* v = (cssc_video_t*)calloc(1, sizeof(cssc_video_t));
    if (!v) return NULL;
    v->w   = (int32_t)(w > 0 ? w : 1);
    v->h   = (int32_t)(h > 0 ? h : 1);
    v->fps = (int32_t)(fps > 0 ? fps : 60);
    v->is_open = 1;
    v->backbuf = (uint32_t*)calloc((size_t)v->w * (size_t)v->h, sizeof(uint32_t));
    if (!v->backbuf) { free(v); return NULL; }
#ifdef _WIN32
    v->bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    v->bmi.bmiHeader.biWidth       = v->w;
    v->bmi.bmiHeader.biHeight      = -v->h;
    v->bmi.bmiHeader.biPlanes      = 1;
    v->bmi.bmiHeader.biBitCount    = 32;
    v->bmi.bmiHeader.biCompression = BI_RGB;
#endif
    return v;
}

int64_t cssc_video_resize(void* p, int64_t w, int64_t h) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v || w <= 0 || h <= 0) return 0;
    if ((int32_t)w == v->w && (int32_t)h == v->h) return 1;
    uint32_t* nb = (uint32_t*)calloc((size_t)w * (size_t)h, sizeof(uint32_t));
    if (!nb) return 0;
    free(v->backbuf);
    v->backbuf = nb;
    v->w = (int32_t)w;
    v->h = (int32_t)h;
#ifdef _WIN32
    v->bmi.bmiHeader.biWidth  = v->w;
    v->bmi.bmiHeader.biHeight = -v->h;
#endif
    return 1;
}

void cssc_video_free(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return;
#ifdef _WIN32
    if (v->hwnd) {
        DestroyWindow(v->hwnd);
        v->hwnd = NULL;
    }
    if (v->wc_atom) {
        UnregisterClassW(CSSC_VIDEO_CLASS, GetModuleHandleW(NULL));
        v->wc_atom = 0;
    }
#endif
    free(v->backbuf);
    free(v);
}

static void cssc_watermark_play(void* p);

void cssc_video_begin(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return;
#ifdef _WIN32
    if (v->hwnd) return;

    HINSTANCE hinst = GetModuleHandleW(NULL);
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = cssc_video_wndproc;
    wc.hInstance     = hinst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CSSC_VIDEO_CLASS;

    v->wc_atom = RegisterClassExW(&wc);

    RECT r = {0, 0, v->w, v->h};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    v->hwnd = CreateWindowExW(0, CSSC_VIDEO_CLASS, L"CSSC Video",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              r.right - r.left, r.bottom - r.top,
                              NULL, NULL, hinst, NULL);
    if (!v->hwnd) { v->is_open = 0; return; }
    SetWindowLongPtrW(v->hwnd, GWLP_USERDATA, (LONG_PTR)v);
    v->hdc = GetDC(v->hwnd);
    ShowWindow(v->hwnd, SW_SHOW);
    UpdateWindow(v->hwnd);

    cssc_watermark_play(v);
#else
    (void)v;
#endif
}

void cssc_video_close(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return;
    v->is_open = 0;
#ifdef _WIN32
    if (v->hwnd) {
        if (v->hdc) { ReleaseDC(v->hwnd, v->hdc); v->hdc = NULL; }
        DestroyWindow(v->hwnd);
        v->hwnd = NULL;
    }
#endif
}

int64_t cssc_video_is_open(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    return v ? (int64_t)v->is_open : 0;
}

int64_t cssc_video_poll_char(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v || v->chead == v->ctail) return 0;
    int c = v->charq[v->chead];
    v->chead = (v->chead + 1) & 255;
    return (int64_t)c;
}
int64_t cssc_video_poll_key(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v || v->khead == v->ktail) return 0;
    int c = v->keyq[v->khead];
    v->khead = (v->khead + 1) & 255;
    return (int64_t)c;
}
int64_t cssc_video_wheel(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return 0;
    int w = v->wheel_accum;
    v->wheel_accum = 0;
    return (int64_t)w;
}

void cssc_video_clear(void* p, int64_t argb) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v || !v->backbuf) return;
    uint32_t px = bgra_of(argb);
    if (clear_framebuffer) {
        clear_framebuffer(v->backbuf, v->w, v->h, px);
        return;
    }
    size_t n = (size_t)v->w * (size_t)v->h;
    for (size_t i = 0; i < n; ++i) v->backbuf[i] = px;
}

void cssc_video_pixel(void* p, int64_t x, int64_t y, int64_t argb) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v || !v->backbuf) return;
    if (x < 0 || y < 0 || x >= v->w || y >= v->h) return;
    v->backbuf[(size_t)y * (size_t)v->w + (size_t)x] = bgra_of(argb);
}

int64_t cssc_video_get_pixel(void* p, int64_t x, int64_t y) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v || !v->backbuf) return 0;
    if (x < 0 || y < 0 || x >= v->w || y >= v->h) return 0;
    return (int64_t)v->backbuf[(size_t)y * (size_t)v->w + (size_t)x];
}

uint32_t* cssc_video_backbuf(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    return v ? v->backbuf : NULL;
}

void cssc_video_present(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return;
#ifdef _WIN32
    if (!v->hwnd) return;

    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { v->is_open = 0; break; }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (v->is_open && v->hdc && v->backbuf) {
        SetDIBitsToDevice(v->hdc,
                          0, 0, v->w, v->h,
                          0, 0, 0, v->h,
                          v->backbuf, &v->bmi, DIB_RGB_COLORS);
    }

    if (v->fps > 0) {
        static int s_timer_res = 0;
        if (!s_timer_res) { timeBeginPeriod(1); s_timer_res = 1; }
        LARGE_INTEGER freq, now;
        QueryPerformanceFrequency(&freq);
        int64_t period = freq.QuadPart / v->fps;
        v->next_deadline += period;
        QueryPerformanceCounter(&now);
        if (v->next_deadline <= now.QuadPart) {

            v->next_deadline = now.QuadPart + period;
        } else {
            for (;;) {
                QueryPerformanceCounter(&now);
                int64_t remain = v->next_deadline - now.QuadPart;
                if (remain <= 0) break;
                int64_t remain_ms = (remain * 1000) / freq.QuadPart;
                if (remain_ms > 1) {
                    Sleep((DWORD)(remain_ms - 1));
                }

            }
        }
    }
#else
    (void)v;
#endif
}

void* cssc_video_hwnd(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return NULL;
#ifdef _WIN32
    return (void*)v->hwnd;
#else
    return NULL;
#endif
}

typedef struct {
    int32_t   w;
    int32_t   h;
    int32_t   scale;
    uint32_t* pixels;
} cssc_matrix_t;

typedef struct {
    int32_t   w;
    int32_t   h;
    uint32_t* pixels;
} cssc_framebuffer_t;

typedef struct {
    int32_t w;
    int32_t h;
#ifdef _WIN32
    HANDLE  stdout_h;
#else
    int     stdout_fd;
#endif
} cssc_console_t;

void* cssc_matrix_new(int64_t w, int64_t h, int64_t scale) {
    cssc_matrix_t* m = (cssc_matrix_t*)calloc(1, sizeof(cssc_matrix_t));
    if (!m) return NULL;
    m->w = (int32_t)(w > 0 ? w : 1);
    m->h = (int32_t)(h > 0 ? h : 1);
    m->scale = (int32_t)(scale > 0 ? scale : 1);
    m->pixels = (uint32_t*)calloc((size_t)m->w * (size_t)m->h, sizeof(uint32_t));
    if (!m->pixels) { free(m); return NULL; }
    return m;
}

void cssc_matrix_free(void* p) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    if (!m) return;
    free(m->pixels);
    free(m);
}

void cssc_matrix_fill(void* p, int64_t argb) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    if (!m || !m->pixels) return;
    uint32_t px = bgra_of(argb);
    if (clear_framebuffer) {
        clear_framebuffer(m->pixels, m->w, m->h, px);
        return;
    }
    size_t n = (size_t)m->w * (size_t)m->h;
    for (size_t i = 0; i < n; ++i) m->pixels[i] = px;
}

void cssc_matrix_pixel(void* p, int64_t x, int64_t y, int64_t argb) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    if (!m || !m->pixels) return;
    if (x < 0 || y < 0 || x >= m->w || y >= m->h) return;
    m->pixels[(size_t)y * (size_t)m->w + (size_t)x] = bgra_of(argb);
}

int64_t cssc_matrix_get_pixel(void* p, int64_t x, int64_t y) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    if (!m || !m->pixels) return 0;
    if (x < 0 || y < 0 || x >= m->w || y >= m->h) return 0;
    return (int64_t)m->pixels[(size_t)y * (size_t)m->w + (size_t)x];
}

int64_t cssc_matrix_width(void* p) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    return m ? (int64_t)m->w : 0;
}

int64_t cssc_matrix_height(void* p) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    return m ? (int64_t)m->h : 0;
}

void cssc_matrix_show(void* p) {
    cssc_matrix_t* m = (cssc_matrix_t*)p;
    if (!m || !m->pixels) return;

    for (int32_t y = 0; y < m->h; ++y) {
        for (int32_t x = 0; x < m->w; ++x) {
            uint32_t v = m->pixels[(size_t)y * (size_t)m->w + (size_t)x];
            unsigned r = (v >> 16) & 0xFF;
            unsigned g = (v >>  8) & 0xFF;
            unsigned b = (v      ) & 0xFF;
            fprintf(stdout, "\x1b[48;2;%u;%u;%um  ", r, g, b);
        }
        fputs("\x1b[0m\n", stdout);
    }
    fflush(stdout);
}

void* cssc_framebuffer_new(int64_t w, int64_t h) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)calloc(1, sizeof(cssc_framebuffer_t));
    if (!fb) return NULL;
    fb->w = (int32_t)(w > 0 ? w : 1);
    fb->h = (int32_t)(h > 0 ? h : 1);
    fb->pixels = (uint32_t*)calloc((size_t)fb->w * (size_t)fb->h, sizeof(uint32_t));
    if (!fb->pixels) { free(fb); return NULL; }
    return fb;
}

void cssc_framebuffer_free(void* p) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    if (!fb) return;
    free(fb->pixels);
    free(fb);
}

void cssc_framebuffer_fill(void* p, int64_t argb) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    if (!fb || !fb->pixels) return;
    uint32_t px = bgra_of(argb);
    if (clear_framebuffer) {
        clear_framebuffer(fb->pixels, fb->w, fb->h, px);
        return;
    }
    size_t n = (size_t)fb->w * (size_t)fb->h;
    for (size_t i = 0; i < n; ++i) fb->pixels[i] = px;
}

void cssc_framebuffer_pixel(void* p, int64_t x, int64_t y, int64_t argb) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    if (!fb || !fb->pixels) return;
    if (x < 0 || y < 0 || x >= fb->w || y >= fb->h) return;
    fb->pixels[(size_t)y * (size_t)fb->w + (size_t)x] = bgra_of(argb);
}

int64_t cssc_framebuffer_get_pixel(void* p, int64_t x, int64_t y) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    if (!fb || !fb->pixels) return 0;
    if (x < 0 || y < 0 || x >= fb->w || y >= fb->h) return 0;
    return (int64_t)fb->pixels[(size_t)y * (size_t)fb->w + (size_t)x];
}

int64_t cssc_framebuffer_width(void* p) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    return fb ? (int64_t)fb->w : 0;
}

int64_t cssc_framebuffer_height(void* p) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    return fb ? (int64_t)fb->h : 0;
}

void cssc_framebuffer_present(void* p, void* video) {
    cssc_framebuffer_t* fb = (cssc_framebuffer_t*)p;
    cssc_video_t* v = (cssc_video_t*)video;
    if (!fb || !fb->pixels || !v || !v->backbuf) return;

    if (fb->w == v->w && fb->h == v->h) {
        memcpy(v->backbuf, fb->pixels,
               (size_t)fb->w * (size_t)fb->h * sizeof(uint32_t));
    } else {
        int32_t cw = fb->w < v->w ? fb->w : v->w;
        int32_t ch = fb->h < v->h ? fb->h : v->h;
        for (int32_t y = 0; y < ch; ++y) {
            memcpy(&v->backbuf[(size_t)y * (size_t)v->w],
                   &fb->pixels[(size_t)y * (size_t)fb->w],
                   (size_t)cw * sizeof(uint32_t));
        }
    }
    cssc_video_present(v);
}

void* cssc_console_new(int64_t w, int64_t h) {
    cssc_console_t* c = (cssc_console_t*)calloc(1, sizeof(cssc_console_t));
    if (!c) return NULL;
    c->w = (int32_t)(w > 0 ? w : 80);
    c->h = (int32_t)(h > 0 ? h : 25);
#ifdef _WIN32
    c->stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (c->stdout_h == INVALID_HANDLE_VALUE || c->stdout_h == NULL) {

        if (AllocConsole()) {
            c->stdout_h = GetStdHandle(STD_OUTPUT_HANDLE);
        }
    }
#else
    c->stdout_fd = 1;
#endif
    return c;
}

void cssc_console_free(void* p) {
    cssc_console_t* c = (cssc_console_t*)p;
    if (!c) return;
    free(c);
}

void cssc_console_write(void* p, const char* s) {
    cssc_console_t* c = (cssc_console_t*)p;
    if (!c || !s) return;
    size_t n = strlen(s);
#ifdef _WIN32
    if (c->stdout_h && c->stdout_h != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteConsoleA(c->stdout_h, s, (DWORD)n, &written, NULL);
    } else {
        fwrite(s, 1, n, stdout);
        fflush(stdout);
    }
#else
    (void)write(c->stdout_fd, s, n);
#endif
}

void cssc_console_clear(void* p) {
    const char* clr = "\x1b[2J\x1b[H";
    cssc_console_write(p, clr);
}

void cssc_console_close(void* p) {
    cssc_console_t* c = (cssc_console_t*)p;
    if (!c) return;
#ifdef _WIN32

    c->stdout_h = NULL;
#endif
}

typedef struct {
    const char* name;
    int         vk;
} cssc_vk_entry_t;

static const cssc_vk_entry_t cssc_vk_table[] = {
#ifdef _WIN32
    {"LEFT",  VK_LEFT}, {"RIGHT", VK_RIGHT}, {"UP", VK_UP}, {"DOWN", VK_DOWN},
    {"SPACE", VK_SPACE}, {"ENTER", VK_RETURN}, {"RETURN", VK_RETURN},
    {"ESC",   VK_ESCAPE}, {"ESCAPE", VK_ESCAPE},
    {"TAB",   VK_TAB},  {"BACK",  VK_BACK}, {"SHIFT", VK_SHIFT},
    {"CTRL",  VK_CONTROL}, {"ALT", VK_MENU},
    {"F1", VK_F1}, {"F2", VK_F2}, {"F3", VK_F3}, {"F4", VK_F4},
    {"F5", VK_F5}, {"F6", VK_F6}, {"F7", VK_F7}, {"F8", VK_F8},
    {"F9", VK_F9}, {"F10", VK_F10}, {"F11", VK_F11}, {"F12", VK_F12},
    {"A", 'A'}, {"B", 'B'}, {"C", 'C'}, {"D", 'D'}, {"E", 'E'},
    {"F", 'F'}, {"G", 'G'}, {"H", 'H'}, {"I", 'I'}, {"J", 'J'},
    {"K", 'K'}, {"L", 'L'}, {"M", 'M'}, {"N", 'N'}, {"O", 'O'},
    {"P", 'P'}, {"Q", 'Q'}, {"R", 'R'}, {"S", 'S'}, {"T", 'T'},
    {"U", 'U'}, {"V", 'V'}, {"W", 'W'}, {"X", 'X'}, {"Y", 'Y'}, {"Z", 'Z'},
    {"0", '0'}, {"1", '1'}, {"2", '2'}, {"3", '3'}, {"4", '4'},
    {"5", '5'}, {"6", '6'}, {"7", '7'}, {"8", '8'}, {"9", '9'},
#else

    {"LEFT", 0}, {"RIGHT", 0}, {"UP", 0}, {"DOWN", 0},
#endif
    {NULL, 0}
};

typedef struct {
    uint8_t curr[256];
    uint8_t prev[256];
} cssc_keyboard_t;

CSSC_INLINE int cssc_keyboard_lookup_vk(const char* name) {
    if (!name) return -1;
    for (size_t i = 0; cssc_vk_table[i].name; ++i) {

        const char* a = cssc_vk_table[i].name;
        const char* b = name;
        while (*a && *b) {
            char ca = *a, cb = *b;
            if (ca >= 'a' && ca <= 'z') ca = (char)(ca - 'a' + 'A');
            if (cb >= 'a' && cb <= 'z') cb = (char)(cb - 'a' + 'A');
            if (ca != cb) break;
            ++a; ++b;
        }
        if (*a == '\0' && *b == '\0') return cssc_vk_table[i].vk;
    }
    return -1;
}

void* cssc_keyboard_new(void) {
    cssc_keyboard_t* k = (cssc_keyboard_t*)calloc(1, sizeof(cssc_keyboard_t));
    return k;
}

void cssc_keyboard_free(void* p) {
    free(p);
}

void cssc_keyboard_update(void* p) {
    cssc_keyboard_t* k = (cssc_keyboard_t*)p;
    if (!k) return;
    memcpy(k->prev, k->curr, sizeof(k->curr));
#ifdef _WIN32
    for (size_t i = 0; cssc_vk_table[i].name; ++i) {
        int vk = cssc_vk_table[i].vk;
        if (vk <= 0 || vk >= 256) continue;
        SHORT s = GetAsyncKeyState(vk);
        k->curr[vk] = (s & 0x8000) ? 1 : 0;
    }
#else
    memset(k->curr, 0, sizeof(k->curr));
#endif
}

int64_t cssc_keyboard_is_pressed(void* p, const char* key) {
    cssc_keyboard_t* k = (cssc_keyboard_t*)p;
    if (!k) return 0;
    int vk = cssc_keyboard_lookup_vk(key);
    if (vk < 0 || vk >= 256) return 0;
    return k->curr[vk] ? 1 : 0;
}

int64_t cssc_keyboard_is_down(void* p, const char* key) {
    cssc_keyboard_t* k = (cssc_keyboard_t*)p;
    if (!k) return 0;
    int vk = cssc_keyboard_lookup_vk(key);
    if (vk < 0 || vk >= 256) return 0;
    return (k->curr[vk] && !k->prev[vk]) ? 1 : 0;
}

int64_t cssc_keyboard_is_released(void* p, const char* key) {
    cssc_keyboard_t* k = (cssc_keyboard_t*)p;
    if (!k) return 0;
    int vk = cssc_keyboard_lookup_vk(key);
    if (vk < 0 || vk >= 256) return 0;
    return (!k->curr[vk] && k->prev[vk]) ? 1 : 0;
}

void* cssc_keyboard_get_key(void* p) {
    cssc_keyboard_t* k = (cssc_keyboard_t*)p;
    if (!k) return cssc_string_lit("", 0);
    for (size_t i = 0; cssc_vk_table[i].name; ++i) {
        int vk = cssc_vk_table[i].vk;
        if (vk <= 0 || vk >= 256) continue;
        if (k->curr[vk]) {
            const char* name = cssc_vk_table[i].name;
            return cssc_string_lit(name, (int64_t)strlen(name));
        }
    }
    return cssc_string_lit("", 0);
}

typedef struct {
    void*   video_ref;
    int64_t x;
    int64_t y;
    uint8_t curr[3];
    uint8_t prev[3];
} cssc_mouse_t;

CSSC_INLINE int cssc_mouse_button_index(const char* name) {
    if (!name) return -1;
    if (name[0] == 'L' || name[0] == 'l') return 0;
    if (name[0] == 'R' || name[0] == 'r') return 1;
    if (name[0] == 'M' || name[0] == 'm') return 2;
    return -1;
}

void* cssc_mouse_new(void* video_or_null) {
    cssc_mouse_t* m = (cssc_mouse_t*)calloc(1, sizeof(cssc_mouse_t));
    if (!m) return NULL;
    m->video_ref = video_or_null;
    return m;
}

void cssc_mouse_free(void* p) {
    free(p);
}

void cssc_mouse_update(void* p) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    if (!m) return;
    memcpy(m->prev, m->curr, sizeof(m->curr));
#ifdef _WIN32
    POINT pt = {0, 0};
    if (GetCursorPos(&pt)) {
        if (m->video_ref) {
            HWND h = (HWND)cssc_video_hwnd(m->video_ref);
            if (h) ScreenToClient(h, &pt);
        }
        m->x = pt.x;
        m->y = pt.y;
    }
    m->curr[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
    m->curr[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;
    m->curr[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ? 1 : 0;
#else
    memset(m->curr, 0, sizeof(m->curr));
#endif
}

int64_t cssc_mouse_get_x(void* p) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    return m ? m->x : 0;
}

int64_t cssc_mouse_get_y(void* p) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    return m ? m->y : 0;
}

void* cssc_mouse_get_pos(void* p) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;

    void* map = cssc_map_ii_new(2);
    if (!map) return NULL;
    if (m) {
        cssc_map_ii_set(map, 0, m->x);
        cssc_map_ii_set(map, 1, m->y);
    }
    return map;
}

int64_t cssc_mouse_is_clicked(void* p, const char* button) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    if (!m) return 0;
    int b = cssc_mouse_button_index(button);
    if (b < 0) return 0;
    return (m->curr[b] && !m->prev[b]) ? 1 : 0;
}

int64_t cssc_mouse_is_held(void* p, const char* button) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    if (!m) return 0;
    int b = cssc_mouse_button_index(button);
    if (b < 0) return 0;
    return m->curr[b] ? 1 : 0;
}

int64_t cssc_mouse_is_released(void* p, const char* button) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    if (!m) return 0;
    int b = cssc_mouse_button_index(button);
    if (b < 0) return 0;
    return (!m->curr[b] && m->prev[b]) ? 1 : 0;
}

int64_t cssc_mouse_is_over(void* p, int64_t x, int64_t y, int64_t w, int64_t h) {
    cssc_mouse_t* m = (cssc_mouse_t*)p;
    if (!m) return 0;
    return (m->x >= x && m->x < x + w && m->y >= y && m->y < y + h) ? 1 : 0;
}

#define CSSC_SOUND_MAX_ALIASES 32

typedef struct {
    char*  path;
    char   alias[16];
} cssc_sound_slot_t;

typedef struct {
    cssc_sound_slot_t slots[CSSC_SOUND_MAX_ALIASES];
    int32_t           next_id;
    int32_t           volume;
} cssc_sound_t;

CSSC_INLINE cssc_sound_slot_t* cssc_sound_find_by_alias(cssc_sound_t* s,
                                                        const char* alias) {
    if (!s || !alias) return NULL;
    for (int i = 0; i < CSSC_SOUND_MAX_ALIASES; ++i) {
        if (s->slots[i].path && strcmp(s->slots[i].alias, alias) == 0)
            return &s->slots[i];
    }
    return NULL;
}

void* cssc_sound_new(void) {
    cssc_sound_t* s = (cssc_sound_t*)calloc(1, sizeof(cssc_sound_t));
    if (!s) return NULL;
    s->volume = 100;
    return s;
}

void cssc_sound_free(void* p) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    if (!s) return;
#ifdef _WIN32

    PlaySoundA(NULL, NULL, SND_PURGE);
#endif
    for (int i = 0; i < CSSC_SOUND_MAX_ALIASES; ++i) {
        free(s->slots[i].path);
    }
    free(s);
}

void* cssc_sound_load(void* p, const char* path) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    if (!s || !path) return cssc_string_lit("", 0);

    cssc_sound_slot_t* slot = NULL;
    for (int i = 0; i < CSSC_SOUND_MAX_ALIASES; ++i) {
        if (!s->slots[i].path) { slot = &s->slots[i]; break; }
    }
    if (!slot) return cssc_string_lit("", 0);
    size_t n = strlen(path);
    slot->path = (char*)malloc(n + 1);
    if (!slot->path) return cssc_string_lit("", 0);
    memcpy(slot->path, path, n + 1);
    int id = s->next_id++;
    snprintf(slot->alias, sizeof(slot->alias), "cssc%d", id);
    return cssc_string_lit(slot->alias, (int64_t)strlen(slot->alias));
}

void cssc_sound_play(void* p, const char* handle_or_path) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    if (!s || !handle_or_path) return;
#ifdef _WIN32
    cssc_sound_slot_t* slot = cssc_sound_find_by_alias(s, handle_or_path);
    if (slot) {
        PlaySoundA(slot->path, NULL,
                   SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
    } else {
        PlaySoundA(handle_or_path, NULL,
                   SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
    }
#else
    (void)handle_or_path;
#endif
}

void cssc_sound_stop(void* p, const char* handle) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    if (!s) return;
    (void)handle;
#ifdef _WIN32

    PlaySoundA(NULL, NULL, SND_PURGE);
#endif
}

void cssc_sound_stop_all(void* p) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    if (!s) return;
#ifdef _WIN32
    PlaySoundA(NULL, NULL, SND_PURGE);
#endif
}

void cssc_sound_set_volume(void* p, int64_t vol) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    if (!s) return;
    if (vol < 0)   vol = 0;
    if (vol > 100) vol = 100;
    s->volume = (int32_t)vol;
#ifdef _WIN32

    DWORD v = (DWORD)((vol * 0xFFFF) / 100);
    DWORD packed = (v & 0xFFFF) | ((v & 0xFFFF) << 16);
    waveOutSetVolume((HWAVEOUT)0, packed);
#endif
}

int64_t cssc_sound_is_playing(void* p, const char* handle) {
    cssc_sound_t* s = (cssc_sound_t*)p;
    (void)handle;

    return s ? 0 : 0;
}

double  cssc_sin  (double x)               { return sin(x); }
double  cssc_cos  (double x)               { return cos(x); }
double  cssc_tan  (double x)               { return tan(x); }
double  cssc_atan2(double y, double x)     { return atan2(y, x); }
double  cssc_floor(double x)               { return floor(x); }
double  cssc_ceil (double x)               { return ceil(x); }
double  cssc_round(double x)               { return round(x); }
double  cssc_pow  (double b, double e)     { return pow(b, e); }
double  cssc_exp  (double x)               { return exp(x); }
double  cssc_log  (double x)               { return log(x); }
double  cssc_fabs (double x)               { return fabs(x); }
double  cssc_fmin (double a, double b)     { return (a < b) ? a : b; }
double  cssc_fmax (double a, double b)     { return (a > b) ? a : b; }

void cssc_seed(int64_t s) {

    cssc_rng_state = (uint64_t)s;
    if (cssc_rng_state == 0) {
        cssc_rng_state = (uint64_t)-7046029254386353131LL;
    }
}

typedef struct {
    int32_t   w;
    int32_t   h;
    uint32_t* pixels;
} cssc_sprite_t;

void* cssc_sprite_new(int64_t w, int64_t h) {
    cssc_sprite_t* sp = (cssc_sprite_t*)calloc(1, sizeof(cssc_sprite_t));
    if (!sp) return NULL;
    sp->w = (int32_t)(w > 0 ? w : 1);
    sp->h = (int32_t)(h > 0 ? h : 1);
    sp->pixels = (uint32_t*)calloc((size_t)sp->w * (size_t)sp->h, sizeof(uint32_t));
    if (!sp->pixels) { free(sp); return NULL; }
    return sp;
}

void cssc_sprite_free(void* p) {
    cssc_sprite_t* sp = (cssc_sprite_t*)p;
    if (!sp) return;
    free(sp->pixels);
    free(sp);
}

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} cssc_bmp_file_hdr_t;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} cssc_bmp_info_hdr_t;
#pragma pack(pop)

typedef struct { const uint8_t* d; size_t len; size_t pos; int bitbuf; int bitcnt; } cssc_png_bits;
static int cssc_png_getbit(cssc_png_bits* b) {
    if (b->bitcnt == 0) {
        if (b->pos >= b->len) return 0;
        b->bitbuf = b->d[b->pos++]; b->bitcnt = 8;
    }
    int r = b->bitbuf & 1; b->bitbuf >>= 1; b->bitcnt--; return r;
}
static int cssc_png_getbits(cssc_png_bits* b, int n) {
    int v = 0; for (int i = 0; i < n; i++) v |= cssc_png_getbit(b) << i; return v;
}
typedef struct { int counts[16]; int symbols[288]; } cssc_png_huff;
static void cssc_png_huff_build(cssc_png_huff* h, const uint8_t* lengths, int n) {
    for (int i = 0; i < 16; i++) h->counts[i] = 0;
    for (int i = 0; i < n; i++) h->counts[lengths[i]]++;
    h->counts[0] = 0;
    int offs[16]; offs[0] = 0;
    for (int i = 1; i < 16; i++) offs[i] = offs[i - 1] + h->counts[i - 1];
    for (int i = 0; i < n; i++) if (lengths[i]) h->symbols[offs[lengths[i]]++] = i;
}
static int cssc_png_decode_sym(cssc_png_bits* b, const cssc_png_huff* h) {
    int code = 0, first = 0, index = 0;
    for (int len = 1; len < 16; len++) {
        code |= cssc_png_getbit(b);
        int count = h->counts[len];
        if (code - first < count) return h->symbols[index + (code - first)];
        index += count; first += count; first <<= 1; code <<= 1;
    }
    return -1;
}
typedef struct { uint8_t* p; size_t len; size_t cap; } cssc_png_buf;
static int cssc_png_buf_push(cssc_png_buf* o, uint8_t v) {
    if (o->len >= o->cap) {
        size_t nc = o->cap ? o->cap * 2 : 1024;
        uint8_t* np = (uint8_t*)realloc(o->p, nc);
        if (!np) return 0;
        o->p = np; o->cap = nc;
    }
    o->p[o->len++] = v; return 1;
}
static const int CSSC_PNG_LBASE[29] = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258};
static const int CSSC_PNG_LEXT[29]  = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,
    4,4,4,4,5,5,5,5,0};
static const int CSSC_PNG_DBASE[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
static const int CSSC_PNG_DEXT[30]  = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,
    9,9,10,10,11,11,12,12,13,13};
static const int CSSC_PNG_CLORD[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
static int cssc_png_inflate_block(cssc_png_bits* b, cssc_png_buf* o,
                                  const cssc_png_huff* lh, const cssc_png_huff* dh) {
    for (;;) {
        int sym = cssc_png_decode_sym(b, lh);
        if (sym < 0) return 0;
        if (sym == 256) return 1;
        if (sym < 256) { if (!cssc_png_buf_push(o, (uint8_t)sym)) return 0; }
        else {
            sym -= 257; if (sym >= 29) return 0;
            int length = CSSC_PNG_LBASE[sym] + cssc_png_getbits(b, CSSC_PNG_LEXT[sym]);
            int dsym = cssc_png_decode_sym(b, dh);
            if (dsym < 0 || dsym >= 30) return 0;
            int dist = CSSC_PNG_DBASE[dsym] + cssc_png_getbits(b, CSSC_PNG_DEXT[dsym]);
            if ((size_t)dist > o->len) return 0;
            size_t start = o->len - (size_t)dist;
            for (int i = 0; i < length; i++)
                if (!cssc_png_buf_push(o, o->p[start + (size_t)i])) return 0;
        }
    }
}
static void cssc_png_build_fixed(cssc_png_huff* lh, cssc_png_huff* dh) {
    uint8_t ll[288];
    for (int i = 0; i < 144; i++) ll[i] = 8;
    for (int i = 144; i < 256; i++) ll[i] = 9;
    for (int i = 256; i < 280; i++) ll[i] = 7;
    for (int i = 280; i < 288; i++) ll[i] = 8;
    cssc_png_huff_build(lh, ll, 288);
    uint8_t dl[30]; for (int i = 0; i < 30; i++) dl[i] = 5;
    cssc_png_huff_build(dh, dl, 30);
}
static int cssc_png_build_dynamic(cssc_png_bits* b, cssc_png_huff* lh, cssc_png_huff* dh) {
    int hlit = cssc_png_getbits(b, 5) + 257;
    int hdist = cssc_png_getbits(b, 5) + 1;
    int hclen = cssc_png_getbits(b, 4) + 4;
    uint8_t cll[19]; for (int i = 0; i < 19; i++) cll[i] = 0;
    for (int i = 0; i < hclen; i++) cll[CSSC_PNG_CLORD[i]] = (uint8_t)cssc_png_getbits(b, 3);
    cssc_png_huff clh; cssc_png_huff_build(&clh, cll, 19);
    uint8_t lengths[320]; int total = hlit + hdist; int i = 0;
    if (total > 320) return 0;
    while (i < total) {
        int sym = cssc_png_decode_sym(b, &clh);
        if (sym < 0) return 0;
        if (sym < 16) { lengths[i++] = (uint8_t)sym; }
        else if (sym == 16) {
            if (i == 0) return 0;
            int rep = cssc_png_getbits(b, 2) + 3; uint8_t prev = lengths[i - 1];
            while (rep-- > 0 && i < total) lengths[i++] = prev;
        } else if (sym == 17) {
            int rep = cssc_png_getbits(b, 3) + 3;
            while (rep-- > 0 && i < total) lengths[i++] = 0;
        } else {
            int rep = cssc_png_getbits(b, 7) + 11;
            while (rep-- > 0 && i < total) lengths[i++] = 0;
        }
    }
    cssc_png_huff_build(lh, lengths, hlit);
    cssc_png_huff_build(dh, lengths + hlit, hdist);
    return 1;
}
static int cssc_png_inflate(const uint8_t* data, size_t len, cssc_png_buf* o) {
    cssc_png_bits b; b.d = data; b.len = len; b.pos = 0; b.bitbuf = 0; b.bitcnt = 0;
    for (;;) {
        int bfinal = cssc_png_getbit(&b);
        int btype = cssc_png_getbits(&b, 2);
        if (btype == 0) {
            b.bitcnt = 0;
            if (b.pos + 4 > b.len) return 0;
            int length = b.d[b.pos] | (b.d[b.pos + 1] << 8); b.pos += 4;
            if (b.pos + (size_t)length > b.len) return 0;
            for (int i = 0; i < length; i++)
                if (!cssc_png_buf_push(o, b.d[b.pos++])) return 0;
        } else if (btype == 1) {
            cssc_png_huff lh, dh; cssc_png_build_fixed(&lh, &dh);
            if (!cssc_png_inflate_block(&b, o, &lh, &dh)) return 0;
        } else if (btype == 2) {
            cssc_png_huff lh, dh;
            if (!cssc_png_build_dynamic(&b, &lh, &dh)) return 0;
            if (!cssc_png_inflate_block(&b, o, &lh, &dh)) return 0;
        } else return 0;
        if (bfinal) break;
    }
    return 1;
}
static int cssc_png_paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = p > a ? p - a : a - p;
    int pb = p > b ? p - b : b - p;
    int pc = p > c ? p - c : c - p;
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}
static cssc_sprite_t* cssc_png_parse(FILE* fp) {
    fseek(fp, 0, SEEK_END); long fsize = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (fsize < 8) return NULL;
    uint8_t* file = (uint8_t*)malloc((size_t)fsize);
    if (!file) return NULL;
    if (fread(file, 1, (size_t)fsize, fp) != (size_t)fsize) { free(file); return NULL; }
    static const uint8_t sig[8] = {0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A};
    if (memcmp(file, sig, 8) != 0) { free(file); return NULL; }
    size_t off = 8; uint32_t W = 0, H = 0; int bitd = 0, ct = 0;
    uint8_t* idat = NULL; size_t idat_len = 0, idat_cap = 0;
    while (off + 12 <= (size_t)fsize) {
        uint32_t clen = ((uint32_t)file[off] << 24) | ((uint32_t)file[off+1] << 16) |
                        ((uint32_t)file[off+2] << 8) | (uint32_t)file[off+3];
        const uint8_t* typ = file + off + 4;
        const uint8_t* body = file + off + 8;
        if (off + 12 + clen > (size_t)fsize) break;
        if (memcmp(typ, "IHDR", 4) == 0) {
            W = ((uint32_t)body[0] << 24) | ((uint32_t)body[1] << 16) |
                ((uint32_t)body[2] << 8) | (uint32_t)body[3];
            H = ((uint32_t)body[4] << 24) | ((uint32_t)body[5] << 16) |
                ((uint32_t)body[6] << 8) | (uint32_t)body[7];
            bitd = body[8]; ct = body[9];
        } else if (memcmp(typ, "IDAT", 4) == 0) {
            if (idat_len + clen > idat_cap) {
                size_t nc = idat_cap ? idat_cap : (clen ? clen : 1);
                while (nc < idat_len + clen) nc *= 2;
                uint8_t* ni = (uint8_t*)realloc(idat, nc);
                if (!ni) { free(idat); free(file); return NULL; }
                idat = ni; idat_cap = nc;
            }
            memcpy(idat + idat_len, body, clen); idat_len += clen;
        } else if (memcmp(typ, "IEND", 4) == 0) break;
        off += 12 + clen;
    }
    free(file);
    if (W == 0 || H == 0 || W > 32768 || H > 32768) { free(idat); return NULL; }
    if (bitd != 8 || (ct != 0 && ct != 2 && ct != 6)) { free(idat); return NULL; }
    if (idat_len < 2) { free(idat); return NULL; }
    cssc_png_buf raw; raw.p = NULL; raw.len = 0; raw.cap = 0;
    if (!cssc_png_inflate(idat + 2, idat_len - 2, &raw)) { free(idat); free(raw.p); return NULL; }
    free(idat);
    int ch = (ct == 6) ? 4 : (ct == 2) ? 3 : 1;
    size_t stride = (size_t)W * (size_t)ch;
    if (raw.len < (stride + 1) * (size_t)H) { free(raw.p); return NULL; }
    cssc_sprite_t* sp = (cssc_sprite_t*)cssc_sprite_new(W, H);
    if (!sp) { free(raw.p); return NULL; }
    uint8_t* prev = (uint8_t*)calloc(stride ? stride : 1, 1);
    uint8_t* line = (uint8_t*)malloc(stride ? stride : 1);
    if (!prev || !line) { free(prev); free(line); free(raw.p); cssc_sprite_free(sp); return NULL; }
    size_t p = 0;
    for (uint32_t y = 0; y < H; y++) {
        int ft = raw.p[p++];
        memcpy(line, raw.p + p, stride); p += stride;
        for (size_t x = 0; x < stride; x++) {
            int a = (x >= (size_t)ch) ? line[x - ch] : 0;
            int bb = prev[x];
            int c = (x >= (size_t)ch) ? prev[x - ch] : 0;
            int v = line[x];
            if (ft == 1) v = (v + a) & 255;
            else if (ft == 2) v = (v + bb) & 255;
            else if (ft == 3) v = (v + ((a + bb) >> 1)) & 255;
            else if (ft == 4) v = (v + cssc_png_paeth(a, bb, c)) & 255;
            line[x] = (uint8_t)v;
        }
        uint32_t* dst = &sp->pixels[(size_t)y * (size_t)W];
        for (uint32_t x = 0; x < W; x++) {
            int r, g, bl, al;
            if (ct == 6) { r = line[x*4+0]; g = line[x*4+1]; bl = line[x*4+2]; al = line[x*4+3]; }
            else if (ct == 2) { r = line[x*3+0]; g = line[x*3+1]; bl = line[x*3+2]; al = 255; }
            else { r = g = bl = line[x]; al = 255; }
            dst[x] = ((uint32_t)al << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bl;
        }
        uint8_t* t = prev; prev = line; line = t;
    }
    free(prev); free(line); free(raw.p);
    return sp;
}

void* cssc_sprite_load(const char* path) {
    if (!path) return NULL;
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    uint8_t magic[8];
    size_t mgot = fread(magic, 1, 8, fp);
    if (mgot >= 8 && magic[0] == 0x89 && magic[1] == 'P' &&
        magic[2] == 'N' && magic[3] == 'G') {
        cssc_sprite_t* sp = cssc_png_parse(fp);
        fclose(fp);
        return sp;
    }
    fseek(fp, 0, SEEK_SET);
    cssc_bmp_file_hdr_t fh;
    cssc_bmp_info_hdr_t ih;
    if (fread(&fh, sizeof(fh), 1, fp) != 1 ||
        fread(&ih, sizeof(ih), 1, fp) != 1) {
        fclose(fp); return NULL;
    }
    if (fh.bfType != 0x4D42 ) { fclose(fp); return NULL; }
    if (ih.biCompression != 0)          { fclose(fp); return NULL; }
    if (ih.biBitCount != 24 && ih.biBitCount != 32) {
        fclose(fp); return NULL;
    }
    int32_t w = ih.biWidth;
    int32_t h_signed = ih.biHeight;
    int32_t h = h_signed < 0 ? -h_signed : h_signed;
    int      bottom_up = h_signed > 0;
    if (w <= 0 || h <= 0 || w > 32768 || h > 32768) {
        fclose(fp); return NULL;
    }
    cssc_sprite_t* sp = (cssc_sprite_t*)cssc_sprite_new(w, h);
    if (!sp) { fclose(fp); return NULL; }
    if (fseek(fp, (long)fh.bfOffBits, SEEK_SET) != 0) {
        cssc_sprite_free(sp); fclose(fp); return NULL;
    }
    int bpp = ih.biBitCount / 8;

    int32_t row_bytes = ((w * bpp) + 3) & ~3;
    uint8_t* row = (uint8_t*)malloc((size_t)row_bytes);
    if (!row) { cssc_sprite_free(sp); fclose(fp); return NULL; }
    for (int32_t sy = 0; sy < h; ++sy) {
        if (fread(row, (size_t)row_bytes, 1, fp) != 1) {
            free(row); cssc_sprite_free(sp); fclose(fp); return NULL;
        }
        int32_t dy = bottom_up ? (h - 1 - sy) : sy;
        uint32_t* dst_row = &sp->pixels[(size_t)dy * (size_t)w];
        for (int32_t x = 0; x < w; ++x) {
            uint32_t b = row[x * bpp + 0];
            uint32_t g = row[x * bpp + 1];
            uint32_t r = row[x * bpp + 2];
            uint32_t a = (bpp == 4) ? row[x * bpp + 3] : 0xFF;

            dst_row[x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    free(row);
    fclose(fp);
    return sp;
}

void cssc_sprite_set_pixel(void* p, int64_t x, int64_t y, int64_t argb) {
    cssc_sprite_t* sp = (cssc_sprite_t*)p;
    if (!sp || !sp->pixels) return;
    if (x < 0 || y < 0 || x >= sp->w || y >= sp->h) return;
    sp->pixels[(size_t)y * (size_t)sp->w + (size_t)x] = bgra_of(argb);
}

int64_t cssc_sprite_get_pixel(void* p, int64_t x, int64_t y) {
    cssc_sprite_t* sp = (cssc_sprite_t*)p;
    if (!sp || !sp->pixels) return 0;
    if (x < 0 || y < 0 || x >= sp->w || y >= sp->h) return 0;
    return (int64_t)sp->pixels[(size_t)y * (size_t)sp->w + (size_t)x];
}

int64_t cssc_sprite_width(void* p) {
    cssc_sprite_t* sp = (cssc_sprite_t*)p;
    return sp ? (int64_t)sp->w : 0;
}

int64_t cssc_sprite_height(void* p) {
    cssc_sprite_t* sp = (cssc_sprite_t*)p;
    return sp ? (int64_t)sp->h : 0;
}

CSSC_INLINE uint32_t* cssc_pixels_for(void* fb, int32_t* out_w, int32_t* out_h) {
    cssc_framebuffer_t* f = (cssc_framebuffer_t*)fb;
    if (!f) return NULL;
    if (out_w) *out_w = f->w;
    if (out_h) *out_h = f->h;
    return f->pixels;
}

void cssc_sprite_draw(void* sprite, void* fb, int64_t x, int64_t y) {
    cssc_sprite_t* sp = (cssc_sprite_t*)sprite;
    int32_t dw = 0, dh = 0;
    uint32_t* dst = cssc_pixels_for(fb, &dw, &dh);
    if (!sp || !sp->pixels || !dst) return;
    if (blit_bitmap) {
        blit_bitmap(dst, dw, dh, x, y, sp->pixels, sp->w, sp->h);
        return;
    }
    for (int32_t sy = 0; sy < sp->h; ++sy) {
        int64_t dy = y + sy;
        if (dy < 0 || dy >= dh) continue;
        for (int32_t sx = 0; sx < sp->w; ++sx) {
            int64_t dx = x + sx;
            if (dx < 0 || dx >= dw) continue;
            uint32_t s = sp->pixels[(size_t)sy * (size_t)sp->w + (size_t)sx];
            if ((s >> 24) == 0) continue;
            dst[(size_t)dy * (size_t)dw + (size_t)dx] = s;
        }
    }
}

void cssc_sprite_draw_scaled(void* sprite, void* fb,
                             int64_t x, int64_t y, int64_t scale) {
    cssc_sprite_t* sp = (cssc_sprite_t*)sprite;
    int32_t dw = 0, dh = 0;
    uint32_t* dst = cssc_pixels_for(fb, &dw, &dh);
    if (!sp || !sp->pixels || !dst) return;
    if (scale <= 0) scale = 1;
    if (blit_bitmap_scaled) {
        blit_bitmap_scaled(dst, dw, dh, x, y, sp->pixels, sp->w, sp->h, scale);
        return;
    }
    for (int32_t sy = 0; sy < sp->h; ++sy) {
        for (int32_t sx = 0; sx < sp->w; ++sx) {
            uint32_t s = sp->pixels[(size_t)sy * (size_t)sp->w + (size_t)sx];
            if ((s >> 24) == 0) continue;
            for (int64_t iy = 0; iy < scale; ++iy) {
                int64_t dy = y + (int64_t)sy * scale + iy;
                if (dy < 0 || dy >= dh) continue;
                for (int64_t ix = 0; ix < scale; ++ix) {
                    int64_t dx = x + (int64_t)sx * scale + ix;
                    if (dx < 0 || dx >= dw) continue;
                    dst[(size_t)dy * (size_t)dw + (size_t)dx] = s;
                }
            }
        }
    }
}

void cssc_sprite_draw_tinted(void* sprite, void* fb,
                             int64_t x, int64_t y, int64_t tint) {
    cssc_sprite_t* sp = (cssc_sprite_t*)sprite;
    int32_t dw = 0, dh = 0;
    uint32_t* dst = cssc_pixels_for(fb, &dw, &dh);
    if (!sp || !sp->pixels || !dst) return;
    uint32_t tt = bgra_of(tint);
    if (blit_bitmap_tinted) {
        blit_bitmap_tinted(dst, dw, dh, x, y, sp->pixels, sp->w, sp->h, tt);
        return;
    }
    uint32_t tr = (tt >> 16) & 0xFF;
    uint32_t tg = (tt >>  8) & 0xFF;
    uint32_t tb = (tt      ) & 0xFF;
    for (int32_t sy = 0; sy < sp->h; ++sy) {
        int64_t dy = y + sy;
        if (dy < 0 || dy >= dh) continue;
        for (int32_t sx = 0; sx < sp->w; ++sx) {
            int64_t dx = x + sx;
            if (dx < 0 || dx >= dw) continue;
            uint32_t s = sp->pixels[(size_t)sy * (size_t)sp->w + (size_t)sx];
            if ((s >> 24) == 0) continue;
            uint32_t sr = (s >> 16) & 0xFF;
            uint32_t sg = (s >>  8) & 0xFF;
            uint32_t sb = (s      ) & 0xFF;
            uint32_t r = (sr * tr) / 255;
            uint32_t g = (sg * tg) / 255;
            uint32_t b = (sb * tb) / 255;
            dst[(size_t)dy * (size_t)dw + (size_t)dx] =
                (s & 0xFF000000u) | (r << 16) | (g << 8) | b;
        }
    }
}

void cssc_sprite_draw_alpha(void* sprite, void* fb,
                            int64_t x, int64_t y, int64_t alpha_255) {
    cssc_sprite_t* sp = (cssc_sprite_t*)sprite;
    int32_t dw = 0, dh = 0;
    uint32_t* dst = cssc_pixels_for(fb, &dw, &dh);
    if (!sp || !sp->pixels || !dst) return;
    if (alpha_255 < 0) alpha_255 = 0;
    if (alpha_255 > 255) alpha_255 = 255;
    if (blit_bitmap_alpha) {
        blit_bitmap_alpha(dst, dw, dh, x, y, sp->pixels, sp->w, sp->h,
                          alpha_255);
        return;
    }
    uint32_t a = (uint32_t)alpha_255;
    uint32_t ia = 255 - a;
    for (int32_t sy = 0; sy < sp->h; ++sy) {
        int64_t dy = y + sy;
        if (dy < 0 || dy >= dh) continue;
        for (int32_t sx = 0; sx < sp->w; ++sx) {
            int64_t dx = x + sx;
            if (dx < 0 || dx >= dw) continue;
            uint32_t s = sp->pixels[(size_t)sy * (size_t)sp->w + (size_t)sx];
            if ((s >> 24) == 0) continue;
            uint32_t* pd = &dst[(size_t)dy * (size_t)dw + (size_t)dx];
            uint32_t d = *pd;
            uint32_t sr = (s >> 16) & 0xFF, sg = (s >> 8) & 0xFF, sb = s & 0xFF;
            uint32_t dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
            uint32_t r = (sr * a + dr * ia) / 255;
            uint32_t g = (sg * a + dg * ia) / 255;
            uint32_t b = (sb * a + db * ia) / 255;
            *pd = 0xFF000000u | (r << 16) | (g << 8) | b;
        }
    }
}

typedef struct {
    int32_t  cols;
    int32_t  rows;
    int32_t  tw;
    int32_t  th;
    int32_t* indices;
    void*    tiles[256];
} cssc_tilemap_t;

void* cssc_tm_new(int64_t cols, int64_t rows, int64_t tile_w, int64_t tile_h) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)calloc(1, sizeof(cssc_tilemap_t));
    if (!tm) return NULL;
    tm->cols = (int32_t)(cols > 0 ? cols : 1);
    tm->rows = (int32_t)(rows > 0 ? rows : 1);
    tm->tw   = (int32_t)(tile_w > 0 ? tile_w : 1);
    tm->th   = (int32_t)(tile_h > 0 ? tile_h : 1);
    tm->indices = (int32_t*)calloc((size_t)tm->cols * (size_t)tm->rows,
                                   sizeof(int32_t));
    if (!tm->indices) { free(tm); return NULL; }
    return tm;
}

void cssc_tm_free(void* p) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    if (!tm) return;
    free(tm->indices);
    free(tm);
}

void cssc_tm_set_tile(void* p, int64_t tile_id, void* sprite) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    if (!tm) return;
    if (tile_id < 0 || tile_id >= 256) return;
    tm->tiles[tile_id] = sprite;
}

void cssc_tm_set(void* p, int64_t col, int64_t row, int64_t tile_id) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    if (!tm || !tm->indices) return;
    if (col < 0 || row < 0 || col >= tm->cols || row >= tm->rows) return;
    tm->indices[(size_t)row * (size_t)tm->cols + (size_t)col] = (int32_t)tile_id;
}

int64_t cssc_tm_get(void* p, int64_t col, int64_t row) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    if (!tm || !tm->indices) return 0;
    if (col < 0 || row < 0 || col >= tm->cols || row >= tm->rows) return 0;
    return (int64_t)tm->indices[(size_t)row * (size_t)tm->cols + (size_t)col];
}

int64_t cssc_tm_cols(void* p) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    return tm ? (int64_t)tm->cols : 0;
}

int64_t cssc_tm_rows(void* p) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    return tm ? (int64_t)tm->rows : 0;
}

void cssc_tm_draw(void* p, void* fb, int64_t cam_x, int64_t cam_y) {
    cssc_tilemap_t* tm = (cssc_tilemap_t*)p;
    int32_t dw = 0, dh = 0;
    uint32_t* dst = cssc_pixels_for(fb, &dw, &dh);
    if (!tm || !tm->indices || !dst) return;

    int64_t first_col = cam_x / tm->tw;
    int64_t first_row = cam_y / tm->th;
    int64_t last_col  = (cam_x + dw) / tm->tw + 1;
    int64_t last_row  = (cam_y + dh) / tm->th + 1;
    if (first_col < 0) first_col = 0;
    if (first_row < 0) first_row = 0;
    if (last_col > tm->cols) last_col = tm->cols;
    if (last_row > tm->rows) last_row = tm->rows;
    for (int64_t row = first_row; row < last_row; ++row) {
        for (int64_t col = first_col; col < last_col; ++col) {
            int32_t id = tm->indices[(size_t)row * (size_t)tm->cols + (size_t)col];
            if (id <= 0 || id >= 256) continue;
            void* sprite = tm->tiles[id];
            if (!sprite) continue;
            int64_t dx = col * tm->tw - cam_x;
            int64_t dy = row * tm->th - cam_y;
            cssc_sprite_draw(sprite, fb, dx, dy);
        }
    }
}

typedef struct {
    int32_t        glyph_w;
    int32_t        glyph_h;
    int32_t        owned;
    const uint8_t* glyphs;
} cssc_font_t;

static const uint8_t cssc_font_8x8[128][8] = {

    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},

    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00},
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},

    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00},
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06},
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00},
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},

    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00},
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00},
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00},
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00},
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},

    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00},
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00},
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00},
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00},
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},

    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00},
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00},
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00},
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00},
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00},
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},

    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00},
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00},
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00},
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00},
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},

    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00},
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00},
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00},
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},

    {0x63,0x63,0x36,0x1C,0x36,0x63,0x63,0x00},
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00},
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00},
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},

    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00},
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00},
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00},
    {0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00},
    {0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00},
    {0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00},
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F},

    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00},
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E},
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00},
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00},
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00},
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00},

    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F},
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78},
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00},
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00},
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00},
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00},
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00},
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},

    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00},
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F},
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00},
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00},
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00},
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00},
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

static cssc_font_t cssc_font_builtin = {
     8,
     8,
     0,
     &cssc_font_8x8[0][0],
};

#ifndef CSSC_WATERMARK_EVERY_WINDOW
#define CSSC_WATERMARK_EVERY_WINDOW 1
#endif

static void cssc_watermark_play(void* p) {
    cssc_video_t* v = (cssc_video_t*)p;
    if (!v) return;
#ifdef _WIN32
#if !CSSC_WATERMARK_EVERY_WINDOW
    static int s_wm_done = 0;
    if (s_wm_done) return;
    s_wm_done = 1;
#endif
    const int64_t WM_MS = 2000, IBASE = 150, HILITE = 160;
    int64_t W = v->w, H = v->h;
    if (W <= 0 || H <= 0) return;

    static const char TEXT[] = "CSSC";
    const int NCH = 4;

    int64_t s1 = (7 * W) / (10 * 32);
    int64_t s2 = (4 * H) / (10 * 8);
    int64_t S  = s1 < s2 ? s1 : s2;
    if (S < 1) S = 1;
    int64_t TW = (int64_t)(NCH * 8) * S;
    int64_t text_x = (W - TW) / 2;
    int64_t text_y = (H - 8 * S) / 2;
    int64_t R = (3 * TW) / 10; if (R < 1) R = 1;
    int64_t sweep_start = text_x - R;
    int64_t sweep_span  = TW + 2 * R;

    LARGE_INTEGER freq, t0, tnow;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    cssc_video_clear(v, 0xFF000000);
    for (;;) {
        if (!v->is_open) return;
        QueryPerformanceCounter(&tnow);
        int64_t e = ((tnow.QuadPart - t0.QuadPart) * 1000) / freq.QuadPart;
        if (e < 0) e = 0;
        if (e > WM_MS) e = WM_MS;
        int64_t hx = sweep_start + (e * sweep_span) / WM_MS;

        for (int ci = 0; ci < NCH; ++ci) {
            const uint8_t* g = cssc_font_8x8[(unsigned char)TEXT[ci]];
            int64_t gx0 = text_x + (int64_t)ci * 8 * S;
            for (int gy = 0; gy < 8; ++gy) {
                uint8_t row = g[gy];
                for (int gx = 0; gx < 8; ++gx) {
                    if (!(row & (0x01u << gx))) continue;
                    for (int64_t sx = 0; sx < S; ++sx) {
                        int64_t px = gx0 + (int64_t)gx * S + sx;
                        int64_t d = px - hx; if (d < 0) d = -d;
                        int64_t tri = (d < R) ? (HILITE * (R - d)) / R : 0;
                        int64_t I = IBASE + tri; if (I > 255) I = 255;
                        int64_t col = 0xFF000000LL | (I << 16) | (I << 8) | I;
                        int64_t cb = text_y + (int64_t)gy * S;
                        for (int64_t sy = 0; sy < S; ++sy)
                            cssc_video_pixel(v, px, cb + sy, col);
                    }
                }
            }
        }
        cssc_video_present(v);
        if (e >= WM_MS) break;
    }
    cssc_video_clear(v, 0xFF000000);
    cssc_video_present(v);
#else
    (void)p;
#endif
}

void* cssc_font_new(void) {
    return &cssc_font_builtin;
}

void cssc_font_free(void* p) {
    cssc_font_t* f = (cssc_font_t*)p;
    if (!f || !f->owned) return;
    free((void*)f->glyphs);
    free(f);
}

void* cssc_font_load(const char* path) {
    if (!path) return NULL;
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    char magic[4];
    if (fread(magic, 1, 4, fp) != 4) { fclose(fp); return NULL; }
    if (memcmp(magic, "CSF1", 4) != 0) { fclose(fp); return NULL; }
    uint32_t gw = 0, gh = 0;
    if (fread(&gw, sizeof(gw), 1, fp) != 1 ||
        fread(&gh, sizeof(gh), 1, fp) != 1) {
        fclose(fp); return NULL;
    }
    if (gw == 0 || gh == 0 || gw > 64 || gh > 64) {
        fclose(fp); return NULL;
    }
    size_t bytes_per_glyph = (size_t)((gw + 7) / 8) * gh;
    size_t total = bytes_per_glyph * 128;
    uint8_t* buf = (uint8_t*)malloc(total);
    if (!buf) { fclose(fp); return NULL; }
    if (fread(buf, 1, total, fp) != total) {
        free(buf); fclose(fp); return NULL;
    }
    fclose(fp);
    cssc_font_t* f = (cssc_font_t*)calloc(1, sizeof(cssc_font_t));
    if (!f) { free(buf); return NULL; }
    f->glyph_w = (int32_t)gw;
    f->glyph_h = (int32_t)gh;
    f->owned   = 1;
    f->glyphs  = buf;
    return f;
}

int64_t cssc_font_height(void* p) {
    cssc_font_t* f = (cssc_font_t*)p;
    return f ? (int64_t)f->glyph_h : 0;
}

int64_t cssc_font_measure(void* p, const char* text) {
    cssc_font_t* f = (cssc_font_t*)p;
    if (!f || !text) return 0;
    return (int64_t)strlen(text) * (int64_t)f->glyph_w;
}

void cssc_font_draw(void* p, void* fb, const char* text,
                    int64_t x, int64_t y, int64_t argb) {
    cssc_font_t* f = (cssc_font_t*)p;
    int32_t dw = 0, dh = 0;
    uint32_t* dst = cssc_pixels_for(fb, &dw, &dh);
    if (!f || !f->glyphs || !text || !dst) return;
    uint32_t px = bgra_of(argb);
    int32_t gw = f->glyph_w;
    int32_t gh = f->glyph_h;
    int32_t bytes_per_row = (gw + 7) / 8;
    int64_t pen = x;
    for (const char* s = text; *s; ++s) {
        unsigned char cu = (unsigned char)*s;
        if (cu >= 128) { pen += gw; continue; }
        const uint8_t* g = &f->glyphs[(size_t)cu * (size_t)(bytes_per_row * gh)];
        for (int32_t row = 0; row < gh; ++row) {
            int64_t dy = y + row;
            if (dy < 0 || dy >= dh) continue;
            for (int32_t col = 0; col < gw; ++col) {
                int64_t dx = pen + col;
                if (dx < 0 || dx >= dw) continue;
                uint8_t byte = g[row * bytes_per_row + (col >> 3)];
                if (byte & (1u << (col & 7))) {
                    dst[(size_t)dy * (size_t)dw + (size_t)dx] = px;
                }
            }
        }
        pen += gw;
    }
}

CSSC_UNUSED static void cssc_host_game_touch_weak(void) {
    (void)blit_bitmap;
    (void)blit_bitmap_scaled;
    (void)blit_bitmap_tinted;
    (void)blit_bitmap_alpha;
    (void)clear_framebuffer;
    (void)fill_rect_native;
}

#endif
