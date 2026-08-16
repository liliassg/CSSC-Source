
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define CSSC_GUI_EXPORT __declspec(dllexport)
    #include <windows.h>
#elif defined(__GNUC__) || defined(__clang__)
    #define CSSC_GUI_EXPORT __attribute__((visibility("default")))
    #include <dirent.h>
    #include <sys/stat.h>
#else
    #define CSSC_GUI_EXPORT
#endif

extern void*   cssc_video_new(int64_t w, int64_t h, int64_t fps);
extern void    cssc_video_begin(void* v);
extern void    cssc_video_present(void* v);
extern void    cssc_video_clear(void* v, int64_t argb);
extern int64_t cssc_video_is_open(void* v);
extern void    cssc_video_close(void* v);
extern void    cssc_video_free(void* v);
extern int64_t cssc_video_resize(void* v, int64_t w, int64_t h);
extern void*   cssc_video_hwnd(void* v);
extern void    cssc_video_fillrect(void* v, int64_t x, int64_t y,
                                   int64_t w, int64_t h, int64_t argb);
extern void    cssc_video_draw_rect(void* v, int64_t x, int64_t y,
                                    int64_t w, int64_t h, int64_t argb);
extern void    cssc_video_draw_text(void* v, int64_t x, int64_t y,
                                    const char* text, int64_t argb, int64_t scale);
extern int64_t cssc_video_poll_char(void* v);
extern int64_t cssc_video_poll_key(void* v);
extern int64_t cssc_video_wheel(void* v);

extern void* cssc_string_lit(const char* data, size_t len);

extern void*   cssc_sprite_load(const char* path);
extern int64_t cssc_sprite_width(void* p);
extern int64_t cssc_sprite_height(void* p);
extern int64_t cssc_sprite_get_pixel(void* p, int64_t x, int64_t y);
extern void    cssc_sprite_free(void* p);

enum { GW_TEXT = 1, GW_BUTTON = 2, GW_TOOLBAR = 3, GW_TEXTBOX = 4,
       GW_EDITOR = 5, GW_LIST = 6, GW_TREE = 7, GW_TERMINAL = 8, GW_MENU = 9,
       GW_PROMPT = 10, GW_TABS = 11, GW_BROWSER = 12, GW_DEBUGGER = 13 };

typedef struct cssc_gui_screen {
    void*    vid;
    int64_t  w, h;
    int64_t  mx, my, down, prev_down, rdown, prev_rdown;
    int64_t  ctrl, shift, alt;
    int64_t  input_captured;
    int64_t  cssc_modal;
    int64_t  hk_prev_b, hk_prev_j;
    int64_t  hk_prev_caret;
    int64_t  hk_prev_f1, hk_prev_f2, hk_prev_f5, hk_prev_f6, hk_prev_f9, hk_prev_f11;
    void**   widgets;
    int      n_widgets, cap_widgets;
    int64_t  last_cb;
    int64_t  fs_on;
    int64_t  fs_style;
    int64_t  fs_rx, fs_ry, fs_rw, fs_rh;
    int64_t  fs_vw, fs_vh;
} cssc_gui_screen;

typedef struct { int64_t scale; int64_t color; } cssc_gui_font;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    char*            text;
    int64_t          x, y, scale, color;
    int              visible;
} cssc_gui_text;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            label;
    int64_t          scale, base, hover, text_color, cb_id;
    int              hovered, pressed, visible;
} cssc_gui_button;

typedef struct {
    int               kind;
    cssc_gui_screen*  screen;
    int64_t           x, y, w, h, orient, spacing;
    cssc_gui_button** items;
    int               n_items, cap_items;
    char*             right_text;
    int64_t           right_color;
} cssc_gui_toolbar;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            buf;
    int              len, cap, cursor;
    int64_t          scale, bg, fg, border;
    int              focused, visible;
} cssc_gui_textbox;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char**           lines;
    int              nlines, cap_lines;
    int              cur_line, cur_col, top_line, left_col;
    int              sel_line, sel_col, sel_active, dragging;
    int64_t          scale, bg, fg, gutter, cursor_c, rev;
    int              focused, visible, gutter_on, language;
    char**           undo; int n_undo, cap_undo;
    char**           redo; int n_redo, cap_redo;
    int              last_op, save_req;
    char*            search; int search_len;
    int              dbl_line, dbl_col, dbl_timer;
    int              follow;

    int*             own_aline;  int* own_afreed; int* own_aleaked; int own_na;
    int*             own_dline;  int* own_dtarget; int own_nd;
    int              own_cap_a, own_cap_d;

    char**           cmp_items; int cmp_n, cmp_cap;
    int*             cmp_filt;  int cmp_nf, cmp_capf;
    int              cmp_open, cmp_sel, cmp_top;
    int              cmp_start, cmp_line;
    int              cmp_req, cmp_pending;

    char*            hov_text;
    int              hov_line, hov_col, hov_ws, hov_we;
    int              hov_mx, hov_my, hov_dwell;
    int              hov_open, hov_req;

    int*             dg_line; int* dg_col; int* dg_sev; int dg_n, dg_cap;

    int*             sd_line; int* sd_sev; char** sd_msg; int sd_n, sd_cap, sd_on;

    char*            sig_text;
    int              sig_open, sig_req;
    int              sig_line, sig_col;
    int              sig_open_line, sig_open_col, sig_arg;

    int              ip_line;
} cssc_gui_editor;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char**           items;
    int*             depth;
    int              nitems, cap;
    int              selected, top, right_hit;
    int64_t          scale, bg, fg, sel_bg, sel_fg;
    int              focused, visible;
} cssc_gui_list;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char             root[520];
    char**           paths;
    char**           names;
    int*             depths;
    int*             isdir;
    int              nrows, cap;
    int              selected, top, right_hit;
    int64_t          scale, bg, fg, sel_bg, sel_fg;
    int              focused, visible;
    char**           fold_paths;
    int*             fold_open;
    int              fold_n, fold_cap;
    char             icondir[520];
    void*            ico_folder; void* ico_cssc; void* ico_md;
    void*            ico_ini;    void* ico_file;
    void*            ico_exe;    void* ico_csscu; void* ico_project;
    void*            ico_arrow_open; void* ico_arrow_closed;

    struct { char ext[24]; void* spr; } ico_ext[48];
    int              ico_ext_n;
    int              ico_loaded;

    int              drag_on, drag_idx, drag_active, drag_dx, drag_dy;
    int              drop_ready;
    char             drop_src[520], drop_dst[520];
} cssc_gui_tree;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char**           lines;
    int64_t*         line_col;
    int              nlines, cap, top, stick;
    int              sa_l, sa_c, se_l, se_c, sel_active, dragging;
    char             input[1024];
    int              input_len, input_cur;
    char             cwd[1024];
    char**           hist;
    int              nhist, cap_hist, hist_pos;
    int64_t          scale, bg, fg;
    int              focused, visible;
    void*            proc;
    void*            pipe_read;
    void*            stdin_write;
    int              running;
    char             partial[4096];
    int              partial_len;
    int              alt_c_prev;

    int              esc_st;
    char             esc_buf[48];
    int              esc_len;
    int64_t          line_col_use;
    int              line_col_set;
    unsigned int     utf_cp;
    int              utf_need;
} cssc_gui_terminal;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            titles[16];
    int              n_titles;
    char*            item_label[128];
    int              item_menu[128];
    int64_t          item_action[128];
    int              n_items;
    int              open;
    int64_t          last_action;
    int64_t          scale, bg, fg;
    int              visible;
    char*            right_text;
    int64_t          right_color;
} cssc_gui_menu;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    char             label[256];
    char             input[1024];
    int              input_len, input_cur;
    int              active;
    int              result;
    int64_t          scale, bg, fg;
    int              visible;
} cssc_gui_prompt;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            paths[64];
    int              n_tabs, active;
    int64_t          switch_hit;
    int64_t          close_hit;
    int64_t          scale, bg, fg;
    int              visible;
} cssc_gui_tabs;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;
    char             dir[520];
    char*            names[512];
    int              isdir[512];
    int              n, sel, top;
    int              mode;
    int              active;
    int              result;
    char             chosen[600];
    int64_t          scale, bg, fg;
    int              visible;
    char             icondir[520];
    void*            ico_folder; void* ico_cssc; void* ico_md; void* ico_ini;
    void*            ico_file;   void* ico_exe;  void* ico_csscu; void* ico_project;
    int              ico_loaded;
} cssc_gui_browser;

static char* gui_strdup(const char* s) {
    if (!s) s = "";
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (p) memcpy(p, s, n + 1);
    return p;
}

static void gui_poll_mouse(cssc_gui_screen* s) {
    s->prev_down = s->down;
    s->prev_rdown = s->rdown;
#ifdef _WIN32
    HWND hwnd = (HWND)cssc_video_hwnd(s->vid);
    if (!hwnd) return;

    if (GetForegroundWindow() != hwnd) {
        s->down = 0; s->rdown = 0;
        s->ctrl = 0; s->shift = 0; s->alt = 0;
        s->mx = -100000; s->my = -100000;
        return;
    }
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);
    s->mx = (int64_t)pt.x;
    s->my = (int64_t)pt.y;
    s->down = (GetAsyncKeyState(0x01) & 0x8000) ? 1 : 0;
    s->rdown = (GetAsyncKeyState(0x02) & 0x8000) ? 1 : 0;
    s->ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 1 : 0;
    s->shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : 0;
    s->alt = (GetAsyncKeyState(VK_MENU) & 0x8000) ? 1 : 0;
#endif
}

static void gui_panel(void* vid, int64_t x, int64_t y, int64_t w, int64_t h,
                      int hovered, int pressed) {
    if (w <= 0 || h <= 0) return;
    int64_t sheen = h * 3 / 10; if (sheen < 1) sheen = 1;
    cssc_video_fillrect(vid, x, y, w, h, (int64_t)0xB0101018);
    cssc_video_fillrect(vid, x, y, w, sheen, (int64_t)0x24FFFFFF);
    cssc_video_fillrect(vid, x, y, w, 1, (int64_t)0x60FFFFFF);
    cssc_video_fillrect(vid, x, y, 1, h, (int64_t)0x30FFFFFF);
    cssc_video_fillrect(vid, x, y + h - 1, w, 1, (int64_t)0x50000000);
    cssc_video_fillrect(vid, x + w - 1, y, 1, h, (int64_t)0x30000000);
    if (pressed)      cssc_video_fillrect(vid, x, y, w, h, (int64_t)0x34000000);
    else if (hovered) cssc_video_fillrect(vid, x, y, w, h, (int64_t)0x18FFFFFF);
}

CSSC_GUI_EXPORT void* cssc_gui_screen_new(int64_t w, int64_t h, int64_t fps) {
    cssc_gui_screen* s = (cssc_gui_screen*)calloc(1, sizeof(cssc_gui_screen));
    if (!s) return NULL;
    s->w = w > 0 ? w : 1;
    s->h = h > 0 ? h : 1;
    s->vid = cssc_video_new(s->w, s->h, fps > 0 ? fps : 60);
    cssc_video_begin(s->vid);
    return s;
}

CSSC_GUI_EXPORT void cssc_gui_screen_clear(void* p, int64_t argb) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; if (s) cssc_video_clear(s->vid, argb);
}
CSSC_GUI_EXPORT void cssc_gui_screen_present(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; if (s) cssc_video_present(s->vid);
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_isopen(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? cssc_video_is_open(s->vid) : 0;
}
CSSC_GUI_EXPORT void cssc_gui_screen_close(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; if (s) cssc_video_close(s->vid);
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_width(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->w : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_height(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->h : 0;
}

CSSC_GUI_EXPORT void cssc_gui_screen_setfullscreen(void* p, int64_t on) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return;
#ifdef _WIN32
    HWND hwnd = (HWND)cssc_video_hwnd(s->vid);
    if (!hwnd) return;
    if (on && !s->fs_on) {
        s->fs_style = (int64_t)GetWindowLongPtrW(hwnd, GWL_STYLE);
        RECT wr; GetWindowRect(hwnd, &wr);
        s->fs_rx = wr.left; s->fs_ry = wr.top;
        s->fs_rw = wr.right - wr.left; s->fs_rh = wr.bottom - wr.top;
        s->fs_vw = s->w; s->fs_vh = s->h;
        HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi; mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoW(mon, &mi)) return;
        int64_t mw = mi.rcMonitor.right - mi.rcMonitor.left;
        int64_t mh = mi.rcMonitor.bottom - mi.rcMonitor.top;
        SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR)(WS_POPUP | WS_VISIBLE));
        if (!cssc_video_resize(s->vid, mw, mh)) return;
        s->w = mw; s->h = mh;
        SetWindowPos(hwnd, HWND_TOP, (int)mi.rcMonitor.left, (int)mi.rcMonitor.top,
                     (int)mw, (int)mh, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        s->fs_on = 1;
    } else if (!on && s->fs_on) {
        SetWindowLongPtrW(hwnd, GWL_STYLE, (LONG_PTR)s->fs_style);
        cssc_video_resize(s->vid, s->fs_vw, s->fs_vh);
        s->w = s->fs_vw; s->h = s->fs_vh;
        SetWindowPos(hwnd, HWND_TOP, (int)s->fs_rx, (int)s->fs_ry,
                     (int)s->fs_rw, (int)s->fs_rh, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        s->fs_on = 0;
    }
#endif
}
CSSC_GUI_EXPORT void cssc_gui_screen_togglefullscreen(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (s) cssc_gui_screen_setfullscreen(p, s->fs_on ? 0 : 1);
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_isfullscreen(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->fs_on : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_mousex(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->mx : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_mousey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->my : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_mousedown(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->down : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_mouseright(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->rdown : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_mouseclicked(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    return (s && s->down && !s->prev_down) ? 1 : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_ctrl(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->ctrl : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_shift(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->shift : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_alt(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; return s ? s->alt : 0;
}

CSSC_GUI_EXPORT void cssc_gui_screen_setmodal(void* p, int64_t on) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; if (s) s->cssc_modal = on ? 1 : 0;
}

static int screen_focused(cssc_gui_screen* s) {
#ifdef _WIN32
    if (!s || !s->vid) return 1;
    HWND hwnd = (HWND)cssc_video_hwnd(s->vid);
    if (!hwnd) return 1;
    return (GetForegroundWindow() == hwnd) ? 1 : 0;
#else
    return 1;
#endif
}

CSSC_GUI_EXPORT int64_t cssc_gui_screen_hotkey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return 0;
#ifdef _WIN32
    int ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 1 : 0;
    int alt  = (GetAsyncKeyState(VK_MENU) & 0x8000) ? 1 : 0;
    int b = (ctrl && (GetAsyncKeyState('B') & 0x8000)) ? 1 : 0;
    int j = (ctrl && (GetAsyncKeyState('J') & 0x8000)) ? 1 : 0;

    int caret = (alt && ((GetAsyncKeyState(0xDC) & 0x8000) ||
                         (GetAsyncKeyState(0xC0) & 0x8000))) ? 1 : 0;
    if (!screen_focused(s)) {
        s->hk_prev_b = b; s->hk_prev_j = j; s->hk_prev_caret = caret;
        return 0;
    }
    int64_t r = 0;
    if (b && !s->hk_prev_b) r = 1;
    else if (j && !s->hk_prev_j) r = 2;
    else if (caret && !s->hk_prev_caret) r = 3;
    s->hk_prev_b = b;
    s->hk_prev_j = j;
    s->hk_prev_caret = caret;
    return r;
#else
    return 0;
#endif
}

CSSC_GUI_EXPORT int64_t cssc_gui_screen_funckey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return 0;
#ifdef _WIN32
    int f1 = (GetAsyncKeyState(VK_F1) & 0x8000) ? 1 : 0;
    int f2 = (GetAsyncKeyState(VK_F2) & 0x8000) ? 1 : 0;
    int f5 = (GetAsyncKeyState(VK_F5) & 0x8000) ? 1 : 0;
    int f6 = (GetAsyncKeyState(VK_F6) & 0x8000) ? 1 : 0;
    int f9 = (GetAsyncKeyState(VK_F9) & 0x8000) ? 1 : 0;
    int f11 = (GetAsyncKeyState(VK_F11) & 0x8000) ? 1 : 0;
    if (!screen_focused(s)) {
        s->hk_prev_f1 = f1; s->hk_prev_f2 = f2; s->hk_prev_f5 = f5;
        s->hk_prev_f6 = f6; s->hk_prev_f9 = f9; s->hk_prev_f11 = f11;
        return 0;
    }
    int64_t r = 0;
    if (f1 && !s->hk_prev_f1) r = 1;
    else if (f2 && !s->hk_prev_f2) r = 2;
    else if (f5 && !s->hk_prev_f5) r = 5;
    else if (f6 && !s->hk_prev_f6) r = 6;
    else if (f9 && !s->hk_prev_f9) r = 9;
    else if (f11 && !s->hk_prev_f11) r = 11;
    s->hk_prev_f1 = f1;
    s->hk_prev_f2 = f2;
    s->hk_prev_f5 = f5;
    s->hk_prev_f6 = f6;
    s->hk_prev_f9 = f9;
    s->hk_prev_f11 = f11;
    return r;
#else
    return 0;
#endif
}

CSSC_GUI_EXPORT int64_t cssc_gui_screen_fkeydown(void* p, int64_t n) {
#ifdef _WIN32
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (n < 1 || n > 12) return 0;
    if (s && !screen_focused(s)) return 0;
    return (GetAsyncKeyState((int)(VK_F1 + (n - 1))) & 0x8000) ? 1 : 0;
#else
    (void)p; (void)n; return 0;
#endif
}

static void gui_clipboard_set(const char* text);
CSSC_GUI_EXPORT void cssc_gui_screen_clipboardset(void* p, const char* text) {
    (void)p;
    if (text) gui_clipboard_set(text);
}

CSSC_GUI_EXPORT int64_t cssc_gui_screen_pollchar(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    return s ? cssc_video_poll_char(s->vid) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_pollkey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    return s ? cssc_video_poll_key(s->vid) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_screen_wheel(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    return s ? cssc_video_wheel(s->vid) : 0;
}

CSSC_GUI_EXPORT void cssc_gui_screen_fillrect(void* p, int64_t x, int64_t y,
                                              int64_t w, int64_t h, int64_t argb) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (s) cssc_video_fillrect(s->vid, x, y, w, h, argb);
}
CSSC_GUI_EXPORT void cssc_gui_screen_drawrect(void* p, int64_t x, int64_t y,
                                              int64_t w, int64_t h, int64_t argb) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (s) cssc_video_draw_rect(s->vid, x, y, w, h, argb);
}
CSSC_GUI_EXPORT void cssc_gui_screen_drawtext(void* p, int64_t x, int64_t y,
                                              const char* text, int64_t argb,
                                              int64_t scale) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (s) cssc_video_draw_text(s->vid, x, y, text ? text : "", argb, scale);
}

static void gui_blit_sprite(void* v, void* icon, int64_t x, int64_t y, int64_t scale);
#define GUI_ICO_CACHE 16
static struct { char path[600]; void* spr; } g_ico_cache[GUI_ICO_CACHE];
static int g_ico_cache_n = 0;
static void* gui_icon_cached(const char* path) {
    if (!path || !path[0]) return NULL;
    for (int i = 0; i < g_ico_cache_n; i++)
        if (!strcmp(g_ico_cache[i].path, path)) return g_ico_cache[i].spr;
    void* spr = cssc_sprite_load(path);
    if (g_ico_cache_n < GUI_ICO_CACHE) {
        snprintf(g_ico_cache[g_ico_cache_n].path, 600, "%s", path);
        g_ico_cache[g_ico_cache_n].spr = spr;
        g_ico_cache_n++;
    }
    return spr;
}
CSSC_GUI_EXPORT void cssc_gui_screen_drawicon(void* p, const char* path,
                                              int64_t x, int64_t y, int64_t scale) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s || !s->vid) return;
    void* spr = gui_icon_cached(path);
    if (spr) gui_blit_sprite(s->vid, spr, x, y, scale > 0 ? scale : 1);
}
CSSC_GUI_EXPORT void cssc_gui_screen_seticon(void* p, const char* path) {
#ifdef _WIN32
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s || !s->vid) return;
    HWND hwnd = (HWND)cssc_video_hwnd(s->vid);
    if (!hwnd) return;
    void* spr = gui_icon_cached(path);
    if (!spr) return;
    int w = (int)cssc_sprite_width(spr), h = (int)cssc_sprite_height(spr);
    if (w <= 0 || h <= 0) return;

    BITMAPV5HEADER bi; ZeroMemory(&bi, sizeof(bi));
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = w; bi.bV5Height = -h; bi.bV5Planes = 1; bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000; bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF; bi.bV5AlphaMask = 0xFF000000;
    HDC hdc = GetDC(NULL);
    void* bits = NULL;
    HBITMAP color = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (!color || !bits) { if (color) DeleteObject(color); return; }
    uint32_t* px = (uint32_t*)bits;
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++)
            px[yy * w + xx] = (uint32_t)cssc_sprite_get_pixel(spr, xx, yy);

    int mstride = ((w + 15) / 16) * 2;
    unsigned char* mbits = (unsigned char*)calloc((size_t)mstride * h, 1);
    HBITMAP mask = CreateBitmap(w, h, 1, 1, mbits);
    if (mbits) free(mbits);
    ICONINFO ii; ZeroMemory(&ii, sizeof(ii));
    ii.fIcon = TRUE; ii.hbmColor = color; ii.hbmMask = mask;
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(color); DeleteObject(mask);
    if (hIcon) {
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SetClassLongPtrW(hwnd, GCLP_HICON, (LONG_PTR)hIcon);
    }
#else
    (void)p; (void)path;
#endif
}

CSSC_GUI_EXPORT void cssc_gui_screen_add(void* p, void* widget) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s || !widget) return;
    if (s->n_widgets >= s->cap_widgets) {
        int nc = s->cap_widgets ? s->cap_widgets * 2 : 8;
        void** nw = (void**)realloc(s->widgets, (size_t)nc * sizeof(void*));
        if (!nw) return;
        s->widgets = nw; s->cap_widgets = nc;
    }
    s->widgets[s->n_widgets++] = widget;
}

CSSC_GUI_EXPORT int64_t cssc_gui_button_update(void* bp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_button_draw(void* bp);
CSSC_GUI_EXPORT void    cssc_gui_text_draw(void* tp);
CSSC_GUI_EXPORT int64_t cssc_gui_toolbar_update(void* tp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_toolbar_draw(void* tp);
CSSC_GUI_EXPORT int64_t cssc_gui_textbox_update(void* tp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_textbox_draw(void* tp);
CSSC_GUI_EXPORT int64_t cssc_gui_editor_update(void* ep, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_editor_draw(void* ep);
CSSC_GUI_EXPORT int64_t cssc_gui_list_update(void* lp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_list_draw(void* lp);
CSSC_GUI_EXPORT int64_t cssc_gui_tree_update(void* tp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_tree_draw(void* tp);
CSSC_GUI_EXPORT int64_t cssc_gui_terminal_update(void* tp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_terminal_draw(void* tp);
CSSC_GUI_EXPORT int64_t cssc_gui_menu_update(void* mp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_menu_draw(void* mp);
CSSC_GUI_EXPORT int64_t cssc_gui_prompt_update(void* pp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_prompt_draw(void* pp);
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_update(void* tp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_tabs_draw(void* tp);
CSSC_GUI_EXPORT int64_t cssc_gui_browser_update(void* bp, void* sp);
CSSC_GUI_EXPORT void    cssc_gui_browser_draw(void* bp);

CSSC_GUI_EXPORT int64_t cssc_gui_screen_update(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return 0;
    gui_poll_mouse(s);
    s->last_cb = 0;
    s->input_captured = 0;
    if (s->cssc_modal) s->input_captured = 1;

    for (int i = 0; i < s->n_widgets; ++i) {
        if (*(int*)s->widgets[i] == GW_PROMPT) {
            cssc_gui_prompt* pr = (cssc_gui_prompt*)s->widgets[i];
            if (pr->active) s->input_captured = 1;
            cssc_gui_prompt_update(pr, s);
        } else if (*(int*)s->widgets[i] == GW_BROWSER) {
            cssc_gui_browser* br = (cssc_gui_browser*)s->widgets[i];
            if (br->active) s->input_captured = 1;
            cssc_gui_browser_update(br, s);
        }
    }
    for (int i = 0; i < s->n_widgets; ++i) {
        if (*(int*)s->widgets[i] == GW_MENU) {
            cssc_gui_menu* mm = (cssc_gui_menu*)s->widgets[i];
            int was_open = mm->open;
            cssc_gui_menu_update(mm, s);
            if (was_open || mm->open || mm->last_action) s->input_captured = 1;
        }
    }

    for (int i = 0; i < s->n_widgets; ++i) {
        int kind = *(int*)s->widgets[i];
        if (kind == GW_MENU || kind == GW_PROMPT || kind == GW_BROWSER) {
            continue;
        } else if (kind == GW_BUTTON) {
            if (cssc_gui_button_update(s->widgets[i], s)) {
                cssc_gui_button* b = (cssc_gui_button*)s->widgets[i];
                if (b->cb_id) s->last_cb = b->cb_id;
            }
        } else if (kind == GW_TOOLBAR) {
            cssc_gui_toolbar_update(s->widgets[i], s);
        } else if (kind == GW_TEXTBOX) {
            cssc_gui_textbox_update(s->widgets[i], s);
        } else if (kind == GW_EDITOR) {
            cssc_gui_editor_update(s->widgets[i], s);
        } else if (kind == GW_LIST) {
            int64_t act = cssc_gui_list_update(s->widgets[i], s);
            if (act) s->last_cb = act;
        } else if (kind == GW_TREE) {
            int64_t act = cssc_gui_tree_update(s->widgets[i], s);
            if (act) s->last_cb = act;
        } else if (kind == GW_TERMINAL) {
            cssc_gui_terminal_update(s->widgets[i], s);
        } else if (kind == GW_TABS) {
            cssc_gui_tabs_update(s->widgets[i], s);
        }
    }
    return s->last_cb;
}

CSSC_GUI_EXPORT void cssc_gui_screen_render(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return;
    for (int i = 0; i < s->n_widgets; ++i) {
        int kind = *(int*)s->widgets[i];
        if (kind == GW_TEXT)         cssc_gui_text_draw(s->widgets[i]);
        else if (kind == GW_BUTTON)  cssc_gui_button_draw(s->widgets[i]);
        else if (kind == GW_TOOLBAR) cssc_gui_toolbar_draw(s->widgets[i]);
        else if (kind == GW_TEXTBOX) cssc_gui_textbox_draw(s->widgets[i]);
        else if (kind == GW_EDITOR)  cssc_gui_editor_draw(s->widgets[i]);
        else if (kind == GW_LIST)    cssc_gui_list_draw(s->widgets[i]);
        else if (kind == GW_TREE)    cssc_gui_tree_draw(s->widgets[i]);
        else if (kind == GW_TERMINAL) cssc_gui_terminal_draw(s->widgets[i]);
        else if (kind == GW_MENU)    cssc_gui_menu_draw(s->widgets[i]);
        else if (kind == GW_PROMPT)  cssc_gui_prompt_draw(s->widgets[i]);
        else if (kind == GW_TABS)    cssc_gui_tabs_draw(s->widgets[i]);
        else if (kind == GW_BROWSER) cssc_gui_browser_draw(s->widgets[i]);
    }
}

CSSC_GUI_EXPORT void* cssc_gui_font_new(int64_t scale) {
    cssc_gui_font* f = (cssc_gui_font*)calloc(1, sizeof(cssc_gui_font));
    if (!f) return NULL;
    f->scale = scale > 0 ? scale : 1;
    f->color = (int64_t)0xFFFFFFFF;
    return f;
}
CSSC_GUI_EXPORT void cssc_gui_font_setscale(void* p, int64_t s) {
    cssc_gui_font* f = (cssc_gui_font*)p; if (f) f->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT int64_t cssc_gui_font_scale(void* p) {
    cssc_gui_font* f = (cssc_gui_font*)p; return f ? f->scale : 1;
}
CSSC_GUI_EXPORT void cssc_gui_font_setcolor(void* p, int64_t c) {
    cssc_gui_font* f = (cssc_gui_font*)p; if (f) f->color = c & 0xFFFFFFFF;
}
CSSC_GUI_EXPORT int64_t cssc_gui_font_color(void* p) {
    cssc_gui_font* f = (cssc_gui_font*)p; return f ? f->color : (int64_t)0xFFFFFFFF;
}
CSSC_GUI_EXPORT int64_t cssc_gui_font_measure(void* p, const char* text) {
    cssc_gui_font* f = (cssc_gui_font*)p;
    int64_t sc = f ? f->scale : 1;
    return (int64_t)(text ? strlen(text) : 0) * 8 * sc;
}
CSSC_GUI_EXPORT int64_t cssc_gui_font_height(void* p) {
    cssc_gui_font* f = (cssc_gui_font*)p; return (f ? f->scale : 1) * 8;
}

CSSC_GUI_EXPORT void* cssc_gui_text_new(void* screen) {
    cssc_gui_text* t = (cssc_gui_text*)calloc(1, sizeof(cssc_gui_text));
    if (!t) return NULL;
    t->kind = GW_TEXT;
    t->screen = (cssc_gui_screen*)screen;
    t->text = gui_strdup("");
    t->scale = 1;
    t->color = (int64_t)0xFFFFFFFF;
    t->visible = 1;
    return t;
}
CSSC_GUI_EXPORT void cssc_gui_text_settext(void* p, const char* s) {
    cssc_gui_text* t = (cssc_gui_text*)p;
    if (!t) return;
    free(t->text); t->text = gui_strdup(s);
}
CSSC_GUI_EXPORT void* cssc_gui_text_text(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p;
    const char* s = (t && t->text) ? t->text : "";
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT void cssc_gui_text_setpos(void* p, int64_t x, int64_t y) {
    cssc_gui_text* t = (cssc_gui_text*)p; if (t) { t->x = x; t->y = y; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_text_x(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p; return t ? t->x : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_text_y(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p; return t ? t->y : 0;
}
CSSC_GUI_EXPORT void cssc_gui_text_setcolor(void* p, int64_t c) {
    cssc_gui_text* t = (cssc_gui_text*)p; if (t) t->color = c & 0xFFFFFFFF;
}
CSSC_GUI_EXPORT void cssc_gui_text_setscale(void* p, int64_t s) {
    cssc_gui_text* t = (cssc_gui_text*)p; if (t) t->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_text_setfont(void* p, void* fp) {
    cssc_gui_text* t = (cssc_gui_text*)p; cssc_gui_font* f = (cssc_gui_font*)fp;
    if (t && f) { t->scale = f->scale; t->color = f->color; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_text_measure(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p;
    if (!t) return 0;
    return (int64_t)strlen(t->text) * 8 * t->scale;
}
CSSC_GUI_EXPORT void cssc_gui_text_draw(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p;
    if (!t || !t->visible || !t->screen) return;
    cssc_video_draw_text(t->screen->vid, t->x, t->y, t->text, t->color, t->scale);
}
CSSC_GUI_EXPORT void cssc_gui_text_hide(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p; if (t) t->visible = 0;
}
CSSC_GUI_EXPORT void cssc_gui_text_show(void* p) {
    cssc_gui_text* t = (cssc_gui_text*)p; if (t) t->visible = 1;
}

CSSC_GUI_EXPORT void* cssc_gui_button_new(void* screen) {
    cssc_gui_button* b = (cssc_gui_button*)calloc(1, sizeof(cssc_gui_button));
    if (!b) return NULL;
    b->kind = GW_BUTTON;
    b->screen = (cssc_gui_screen*)screen;
    b->w = 120; b->h = 36;
    b->label = gui_strdup("");
    b->scale = 2;
    b->base = (int64_t)0xB0101018;
    b->hover = (int64_t)0x18FFFFFF;
    b->text_color = (int64_t)0xFFFFFFFF;
    b->visible = 1;
    return b;
}
CSSC_GUI_EXPORT void cssc_gui_button_setlabel(void* p, const char* s) {
    cssc_gui_button* b = (cssc_gui_button*)p;
    if (!b) return;
    free(b->label); b->label = gui_strdup(s);
}
CSSC_GUI_EXPORT void* cssc_gui_button_label(void* p) {
    cssc_gui_button* b = (cssc_gui_button*)p;
    const char* s = (b && b->label) ? b->label : "";
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT void cssc_gui_button_setrect(void* p, int64_t x, int64_t y,
                                             int64_t w, int64_t h) {
    cssc_gui_button* b = (cssc_gui_button*)p;
    if (b) { b->x = x; b->y = y; b->w = w; b->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_button_position(void* p, int64_t x, int64_t y) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) { b->x = x; b->y = y; }
}
CSSC_GUI_EXPORT void cssc_gui_button_size(void* p, int64_t w, int64_t h) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) { b->w = w; b->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_button_setcolor(void* p, int64_t c) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) b->base = c & 0xFFFFFFFF;
}
CSSC_GUI_EXPORT void cssc_gui_button_sethovercolor(void* p, int64_t c) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) b->hover = c & 0xFFFFFFFF;
}
CSSC_GUI_EXPORT void cssc_gui_button_settextcolor(void* p, int64_t c) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) b->text_color = c & 0xFFFFFFFF;
}
CSSC_GUI_EXPORT void cssc_gui_button_onclick(void* p, int64_t cb_id) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) b->cb_id = cb_id;
}
CSSC_GUI_EXPORT int64_t cssc_gui_button_update(void* bp, void* sp) {
    cssc_gui_button* b = (cssc_gui_button*)bp;
    if (!b) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : b->screen;
    if (!s) return 0;
    int hover = (b->x <= s->mx && s->mx < b->x + b->w &&
                 b->y <= s->my && s->my < b->y + b->h) ? 1 : 0;
    b->hovered = hover;
    b->pressed = (hover && s->down) ? 1 : 0;
    return (hover && s->down && !s->prev_down) ? 1 : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_button_ishovered(void* p) {
    cssc_gui_button* b = (cssc_gui_button*)p; return b ? b->hovered : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_button_ispressed(void* p) {
    cssc_gui_button* b = (cssc_gui_button*)p; return b ? b->pressed : 0;
}
CSSC_GUI_EXPORT void cssc_gui_button_draw(void* p) {
    cssc_gui_button* b = (cssc_gui_button*)p;
    if (!b || !b->visible || !b->screen) return;
    void* vid = b->screen->vid;
    gui_panel(vid, b->x, b->y, b->w, b->h, b->hovered, b->pressed);
    if (b->label && b->label[0]) {
        int64_t tw = (int64_t)strlen(b->label) * 8 * b->scale;
        int64_t th = 8 * b->scale;
        int64_t tx = b->x + (b->w - tw) / 2;
        int64_t ty = b->y + (b->h - th) / 2;
        cssc_video_draw_text(vid, tx, ty, b->label, b->text_color, b->scale);
    }
}
CSSC_GUI_EXPORT void cssc_gui_button_hide(void* p) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) b->visible = 0;
}
CSSC_GUI_EXPORT void cssc_gui_button_show(void* p) {
    cssc_gui_button* b = (cssc_gui_button*)p; if (b) b->visible = 1;
}

static void gui_toolbar_relayout(cssc_gui_toolbar* t) {
    int64_t cx = t->x + t->spacing;
    int64_t cy = t->y + t->spacing;
    for (int i = 0; i < t->n_items; ++i) {
        cssc_gui_button* b = t->items[i];
        b->x = cx; b->y = cy;
        if (t->orient == 1) cy += b->h + t->spacing;
        else                cx += b->w + t->spacing;
    }
}
CSSC_GUI_EXPORT void* cssc_gui_toolbar_new(void* screen) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)calloc(1, sizeof(cssc_gui_toolbar));
    if (!t) return NULL;
    t->kind = GW_TOOLBAR;
    t->screen = (cssc_gui_screen*)screen;
    t->h = 44; t->spacing = 8;
    return t;
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_add(void* p, void* button) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (!t || !button) return;
    if (t->n_items >= t->cap_items) {
        int nc = t->cap_items ? t->cap_items * 2 : 8;
        cssc_gui_button** ni = (cssc_gui_button**)realloc(
            t->items, (size_t)nc * sizeof(cssc_gui_button*));
        if (!ni) return;
        t->items = ni; t->cap_items = nc;
    }
    t->items[t->n_items++] = (cssc_gui_button*)button;
    gui_toolbar_relayout(t);
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_setpos(void* p, int64_t x, int64_t y) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (t) { t->x = x; t->y = y; gui_toolbar_relayout(t); }
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_setsize(void* p, int64_t w, int64_t h) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (t) { t->w = w; t->h = h; gui_toolbar_relayout(t); }
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_setorientation(void* p, int64_t o) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (t) { t->orient = o; gui_toolbar_relayout(t); }
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_setspacing(void* p, int64_t s) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (t) { t->spacing = s; gui_toolbar_relayout(t); }
}

CSSC_GUI_EXPORT void cssc_gui_toolbar_setrighttext(void* p, const char* text) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (!t) return;
    if (t->right_text) { free(t->right_text); t->right_text = NULL; }
    if (text && text[0]) t->right_text = gui_strdup(text);
    if (t->right_color == 0) t->right_color = (int64_t)0xFF8FA6B8;
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_setrightcolor(void* p, int64_t argb) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (t) t->right_color = argb;
}
CSSC_GUI_EXPORT int64_t cssc_gui_toolbar_update(void* p, void* sp) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (!t) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : t->screen;
    int64_t hit = 0;
    for (int i = 0; i < t->n_items; ++i)
        if (cssc_gui_button_update(t->items[i], s)) hit = 1;
    return hit;
}
CSSC_GUI_EXPORT void cssc_gui_toolbar_draw(void* p) {
    cssc_gui_toolbar* t = (cssc_gui_toolbar*)p;
    if (!t || !t->screen) return;
    if (t->w > 0 && t->h > 0)
        gui_panel(t->screen->vid, t->x, t->y, t->w, t->h, 0, 0);
    for (int i = 0; i < t->n_items; ++i)
        cssc_gui_button_draw(t->items[i]);
    if (t->right_text && t->right_text[0]) {
        int64_t scale = 1;
        int64_t glyph = 8 * scale;
        int64_t tw = (int64_t)strlen(t->right_text) * glyph;
        int64_t tx = t->x + t->w - tw - 12;
        int64_t ty = t->y + (t->h - glyph) / 2;
        cssc_video_draw_text(t->screen->vid, tx, ty, t->right_text,
                             t->right_color ? t->right_color : (int64_t)0xFF8FA6B8, scale);
    }
}

static void tb_ensure(cssc_gui_textbox* t, int extra) {
    if (t->len + extra + 1 <= t->cap) return;
    int nc = t->cap ? t->cap : 64;
    while (nc < t->len + extra + 1) nc *= 2;
    char* nb = (char*)realloc(t->buf, (size_t)nc);
    if (!nb) return;
    t->buf = nb; t->cap = nc;
}
static void tb_insert(cssc_gui_textbox* t, char c) {
    tb_ensure(t, 1);
    if (t->len + 1 >= t->cap) return;
    memmove(t->buf + t->cursor + 1, t->buf + t->cursor,
            (size_t)(t->len - t->cursor + 1));
    t->buf[t->cursor] = c;
    t->len++; t->cursor++;
}
static void tb_backspace(cssc_gui_textbox* t) {
    if (t->cursor <= 0) return;
    memmove(t->buf + t->cursor - 1, t->buf + t->cursor,
            (size_t)(t->len - t->cursor + 1));
    t->len--; t->cursor--;
}
static void tb_delete(cssc_gui_textbox* t) {
    if (t->cursor >= t->len) return;
    memmove(t->buf + t->cursor, t->buf + t->cursor + 1,
            (size_t)(t->len - t->cursor));
    t->len--;
}

CSSC_GUI_EXPORT void* cssc_gui_textbox_new(void* screen) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)calloc(1, sizeof(cssc_gui_textbox));
    if (!t) return NULL;
    t->kind = GW_TEXTBOX;
    t->screen = (cssc_gui_screen*)screen;
    t->x = 20; t->y = 20; t->w = 400; t->h = 40;
    t->cap = 64; t->buf = (char*)calloc(1, (size_t)t->cap);
    t->len = 0; t->cursor = 0;
    t->scale = 2;
    t->bg = (int64_t)0xC0141A24;
    t->fg = (int64_t)0xFFEAEAEA;
    t->border = (int64_t)0x80FFFFFF;
    t->focused = 1; t->visible = 1;
    return t;
}
CSSC_GUI_EXPORT void cssc_gui_textbox_setrect(void* p, int64_t x, int64_t y,
                                              int64_t w, int64_t h) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p;
    if (t) { t->x = x; t->y = y; t->w = w; t->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_textbox_settext(void* p, const char* s) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p;
    if (!t) return;
    if (!s) s = "";
    int n = (int)strlen(s);
    if (n + 1 > t->cap) { tb_ensure(t, n - t->len); }
    if (t->cap < n + 1) return;
    memcpy(t->buf, s, (size_t)n + 1);
    t->len = n; t->cursor = n;
}
CSSC_GUI_EXPORT void* cssc_gui_textbox_text(void* p) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p;
    const char* s = (t && t->buf) ? t->buf : "";
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT int64_t cssc_gui_textbox_length(void* p) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p; return t ? (int64_t)t->len : 0;
}
CSSC_GUI_EXPORT void cssc_gui_textbox_setfocus(void* p, int64_t f) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p; if (t) t->focused = f ? 1 : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_textbox_focused(void* p) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p; return t ? t->focused : 0;
}
CSSC_GUI_EXPORT void cssc_gui_textbox_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p;
    if (t) { t->bg = bg & 0xFFFFFFFF; t->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT void cssc_gui_textbox_setscale(void* p, int64_t s) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)p; if (t) t->scale = s > 0 ? s : 1;
}

CSSC_GUI_EXPORT int64_t cssc_gui_textbox_update(void* tp, void* sp) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)tp;
    if (!t) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : t->screen;
    if (!s || !t->focused) return 0;
    int64_t c;
    while ((c = cssc_video_poll_char(s->vid)) != 0) {
        if (c == 8) tb_backspace(t);
        else if (c == 9) { tb_insert(t, ' '); tb_insert(t, ' '); }
        else if (c >= 32 && c < 127) tb_insert(t, (char)c);

    }
    int64_t k;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        if (k == 0x25) { if (t->cursor > 0) t->cursor--; }
        else if (k == 0x27) { if (t->cursor < t->len) t->cursor++; }
        else if (k == 0x24) t->cursor = 0;
        else if (k == 0x23) t->cursor = t->len;
        else if (k == 0x2E) tb_delete(t);
    }
    return 0;
}

CSSC_GUI_EXPORT void cssc_gui_textbox_draw(void* tp) {
    cssc_gui_textbox* t = (cssc_gui_textbox*)tp;
    if (!t || !t->visible || !t->screen) return;
    void* v = t->screen->vid;
    gui_panel(v, t->x, t->y, t->w, t->h, 0, 0);
    cssc_video_draw_rect(v, t->x, t->y, t->w, t->h,
                         t->focused ? (int64_t)0xC03FD0A0 : t->border);
    int64_t glyph = 8 * t->scale;
    int64_t tx = t->x + 8;
    int64_t ty = t->y + (t->h - glyph) / 2;
    cssc_video_draw_text(v, tx, ty, t->buf, t->fg, t->scale);
    if (t->focused) {
        int64_t cx = tx + (int64_t)t->cursor * glyph;
        cssc_video_fillrect(v, cx, ty, 2, glyph, (int64_t)0xFF3FD0A0);
    }
}

static int ed_itoa(int v, char* out) {
    char tmp[16]; int n = 0;
    if (v == 0) { out[0] = '0'; out[1] = 0; return 1; }
    unsigned uv = (unsigned)v;
    while (uv) { tmp[n++] = (char)('0' + (uv % 10)); uv /= 10; }
    int i = 0;
    while (n) out[i++] = tmp[--n];
    out[i] = 0;
    return i;
}
static int ed_isword(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}

static void ed_select_word(cssc_gui_editor* e, int line, int col) {
    if (line < 0 || line >= e->nlines) return;
    const char* L = e->lines[line];
    int len = (int)strlen(L);
    if (col > len) col = len;
    int a = col, b = col;
    if (col < len && ed_isword(L[col])) {
        while (a > 0 && ed_isword(L[a - 1])) a--;
        while (b < len && ed_isword(L[b])) b++;
    } else if (col > 0 && ed_isword(L[col - 1])) {
        b = col;
        while (a > 0 && ed_isword(L[a - 1])) a--;
    } else {
        return;
    }
    e->sel_line = line; e->sel_col = a;
    e->cur_line = line; e->cur_col = b;
    e->sel_active = (b > a) ? 1 : 0;
}
static void ed_grow_lines(cssc_gui_editor* e, int need) {
    if (need <= e->cap_lines) return;
    int nc = e->cap_lines ? e->cap_lines : 8;
    while (nc < need) nc *= 2;
    char** nl = (char**)realloc(e->lines, (size_t)nc * sizeof(char*));
    if (!nl) return;
    e->lines = nl; e->cap_lines = nc;
}
static void ed_insert_line(cssc_gui_editor* e, int idx, const char* s) {
    ed_grow_lines(e, e->nlines + 1);
    if (e->cap_lines < e->nlines + 1) return;
    for (int i = e->nlines; i > idx; --i) e->lines[i] = e->lines[i - 1];
    e->lines[idx] = gui_strdup(s ? s : "");
    e->nlines++;
}
static void ed_remove_line(cssc_gui_editor* e, int idx) {
    if (idx < 0 || idx >= e->nlines) return;
    free(e->lines[idx]);
    for (int i = idx; i < e->nlines - 1; ++i) e->lines[i] = e->lines[i + 1];
    e->nlines--;
}

static char* ed_scanp_bump_dup(const char* src) {
    const char* p = src;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "#scanp", 6) != 0) return NULL;
    const char* q = p + 6;
    while (*q == ' ' || *q == '\t') q++;
    if (*q != '(') return NULL;
    const char* r = q + 1;
    int depth = 0, commas = 0;
    const char* numstart = NULL;
    while (*r) {
        char c = *r;
        if (c == '(' || c == '[' || c == '{' || c == '<') depth++;
        else if (c == ')') { if (depth == 0) break; depth--; }
        else if (c == ']' || c == '}' || c == '>') { if (depth > 0) depth--; }
        else if (c == ',' && depth == 0) {
            if (++commas == 2) {
                const char* s = r + 1;
                while (*s == ' ' || *s == '\t') s++;
                numstart = s;
                break;
            }
        }
        r++;
    }
    if (!numstart || *numstart < '0' || *numstart > '9') return NULL;
    const char* ne = numstart;
    long val = 0;
    while (*ne >= '0' && *ne <= '9') { val = val * 10 + (*ne - '0'); ne++; }
    val++;
    size_t pre = (size_t)(numstart - src);
    size_t tail = strlen(ne);
    char* out = (char*)malloc(pre + 24 + tail + 1);
    if (!out) return NULL;
    memcpy(out, src, pre);
    int w = snprintf(out + pre, 24, "%ld", val);
    if (w < 0 || w >= 24) { free(out); return NULL; }
    memcpy(out + pre + (size_t)w, ne, tail + 1);
    return out;
}
static void ed_clamp(cssc_gui_editor* e) {
    if (e->nlines == 0) ed_insert_line(e, 0, "");
    if (e->cur_line < 0) e->cur_line = 0;
    if (e->cur_line >= e->nlines) e->cur_line = e->nlines - 1;
    int ll = (int)strlen(e->lines[e->cur_line]);
    if (e->cur_col < 0) e->cur_col = 0;
    if (e->cur_col > ll) e->cur_col = ll;
}
static void ed_insert_char(cssc_gui_editor* e, char c) {
    char* line = e->lines[e->cur_line];
    int ll = (int)strlen(line);
    char* nl = (char*)malloc((size_t)ll + 2);
    if (!nl) return;
    memcpy(nl, line, (size_t)e->cur_col);
    nl[e->cur_col] = c;
    memcpy(nl + e->cur_col + 1, line + e->cur_col, (size_t)(ll - e->cur_col) + 1);
    free(line); e->lines[e->cur_line] = nl;
    e->cur_col++;
    e->rev++;
}
static void ed_newline(cssc_gui_editor* e) {
    char* line = e->lines[e->cur_line];
    char* tail = gui_strdup(line + e->cur_col);
    line[e->cur_col] = 0;
    char* head = gui_strdup(line);
    free(e->lines[e->cur_line]); e->lines[e->cur_line] = head;
    ed_insert_line(e, e->cur_line + 1, tail);
    free(tail);
    e->cur_line++; e->cur_col = 0;
    e->rev++;
}
static void ed_backspace(cssc_gui_editor* e) {
    if (e->cur_col > 0) {
        char* line = e->lines[e->cur_line];
        int ll = (int)strlen(line);
        memmove(line + e->cur_col - 1, line + e->cur_col, (size_t)(ll - e->cur_col) + 1);
        e->cur_col--;
        e->rev++;
    } else if (e->cur_line > 0) {
        int prev = e->cur_line - 1;
        int pl = (int)strlen(e->lines[prev]);
        int cl = (int)strlen(e->lines[e->cur_line]);
        char* joined = (char*)malloc((size_t)pl + cl + 1);
        if (joined) {
            memcpy(joined, e->lines[prev], (size_t)pl);
            memcpy(joined + pl, e->lines[e->cur_line], (size_t)cl + 1);
            free(e->lines[prev]); e->lines[prev] = joined;
        }
        ed_remove_line(e, e->cur_line);
        e->cur_line = prev; e->cur_col = pl;
        e->rev++;
    }
}
static void ed_delete(cssc_gui_editor* e) {
    char* line = e->lines[e->cur_line];
    int ll = (int)strlen(line);
    if (e->cur_col < ll) {
        memmove(line + e->cur_col, line + e->cur_col + 1, (size_t)(ll - e->cur_col));
        e->rev++;
    } else if (e->cur_line < e->nlines - 1) {
        int cl = ll;
        int nll = (int)strlen(e->lines[e->cur_line + 1]);
        char* joined = (char*)malloc((size_t)cl + nll + 1);
        if (joined) {
            memcpy(joined, line, (size_t)cl);
            memcpy(joined + cl, e->lines[e->cur_line + 1], (size_t)nll + 1);
            free(e->lines[e->cur_line]); e->lines[e->cur_line] = joined;
        }
        ed_remove_line(e, e->cur_line + 1);
        e->rev++;
    }
}

static void ed_metrics(cssc_gui_editor* e, int64_t* glyph, int64_t* line_h,
                       int64_t* gutter_w, int64_t* text_x) {
    int64_t g = 8 * e->scale;
    *glyph = g;
    *line_h = g + 4 * e->scale;
    int digits = 1, nn = e->nlines; while (nn >= 10) { nn /= 10; digits++; }
    *gutter_w = e->gutter_on ? (int64_t)(digits + 1) * g + 8 : 0;
    *text_x = e->x + *gutter_w + 6;
}

static int ed_hittest(cssc_gui_editor* e, int64_t mx, int64_t my,
                      int* out_line, int* out_col) {
    if (mx < e->x || mx >= e->x + e->w || my < e->y || my >= e->y + e->h)
        return 0;
    int64_t glyph, line_h, gutter_w, text_x;
    ed_metrics(e, &glyph, &line_h, &gutter_w, &text_x);
    int r = (int)((my - (e->y + 5)) / line_h);
    if (r < 0) r = 0;
    int line = e->top_line + r;
    if (line < 0) line = 0;
    if (line >= e->nlines) line = e->nlines - 1;
    int col = (int)((mx - text_x + glyph / 2) / glyph) + e->left_col;
    if (col < 0) col = 0;
    int ll = (int)strlen(e->lines[line]);
    if (col > ll) col = ll;
    *out_line = line; *out_col = col;
    return 1;
}

static void ed_sel_norm(cssc_gui_editor* e, int* sl, int* sc, int* el, int* ec) {
    if (e->sel_line < e->cur_line ||
        (e->sel_line == e->cur_line && e->sel_col <= e->cur_col)) {
        *sl = e->sel_line; *sc = e->sel_col; *el = e->cur_line; *ec = e->cur_col;
    } else {
        *sl = e->cur_line; *sc = e->cur_col; *el = e->sel_line; *ec = e->sel_col;
    }
}
static void ed_delete_selection(cssc_gui_editor* e) {
    if (!e->sel_active) return;
    int sl, sc, el, ec; ed_sel_norm(e, &sl, &sc, &el, &ec);
    if (sl == el) {
        char* line = e->lines[sl];
        int ll = (int)strlen(line);
        if (ec > ll) ec = ll; if (sc > ll) sc = ll;
        memmove(line + sc, line + ec, (size_t)(ll - ec) + 1);
    } else {
        char* first = e->lines[sl];
        char* last = e->lines[el];
        int fl = sc;
        int tail = (int)strlen(last) - ec; if (tail < 0) tail = 0;
        char* joined = (char*)malloc((size_t)fl + tail + 1);
        if (joined) {
            memcpy(joined, first, (size_t)fl);
            memcpy(joined + fl, last + ec, (size_t)tail);
            joined[fl + tail] = 0;
            free(e->lines[sl]); e->lines[sl] = joined;
        }
        for (int i = el; i > sl; --i) ed_remove_line(e, i);
    }
    e->cur_line = sl; e->cur_col = sc;
    e->sel_active = 0;
    e->rev++;
    ed_clamp(e);
}

static void gui_clipboard_set(const char* text) {
#ifdef _WIN32
    if (!text) return;
    size_t n = strlen(text);
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, n + 1);
    if (hg) {
        char* dst = (char*)GlobalLock(hg);
        if (dst) {
            memcpy(dst, text, n); dst[n] = 0;
            GlobalUnlock(hg);
            SetClipboardData(CF_TEXT, hg);
        } else GlobalFree(hg);
    }
    CloseClipboard();
#else
    (void)text;
#endif
}
static char* gui_clipboard_get(void) {
#ifdef _WIN32
    if (!OpenClipboard(NULL)) return NULL;
    HANDLE h = GetClipboardData(CF_TEXT);
    char* out = NULL;
    if (h) {
        char* src = (char*)GlobalLock(h);
        if (src) {
            size_t n = strlen(src);
            out = (char*)malloc(n + 1);
            if (out) memcpy(out, src, n + 1);
            GlobalUnlock(h);
        }
    }
    CloseClipboard();
    return out;
#else
    return NULL;
#endif
}

#define ED_UNDO_MAX 200
static char* ed_doc_string(cssc_gui_editor* e) {
    size_t total = 1;
    for (int i = 0; i < e->nlines; i++) total += strlen(e->lines[i]) + 1;
    char* out = (char*)malloc(total);
    if (!out) return NULL;
    size_t off = 0;
    for (int i = 0; i < e->nlines; i++) {
        size_t ln = strlen(e->lines[i]);
        memcpy(out + off, e->lines[i], ln); off += ln;
        if (i < e->nlines - 1) out[off++] = '\n';
    }
    out[off] = 0;
    return out;
}
static void ed_load_doc(cssc_gui_editor* e, const char* s) {
    for (int i = 0; i < e->nlines; i++) free(e->lines[i]);
    e->nlines = 0;
    if (!s) s = "";
    const char* start = s;
    for (const char* q = s; ; ++q) {
        if (*q == '\n' || *q == 0) {
            int len = (int)(q - start);
            if (len > 0 && start[len - 1] == '\r') len--;
            char* ln = (char*)malloc((size_t)len + 1);
            if (ln) {
                memcpy(ln, start, (size_t)len); ln[len] = 0;
                ed_grow_lines(e, e->nlines + 1);
                if (e->cap_lines >= e->nlines + 1) e->lines[e->nlines++] = ln;
                else free(ln);
            }
            start = q + 1;
            if (*q == 0) break;
        }
    }
    if (e->nlines == 0) ed_insert_line(e, 0, "");
    e->sel_active = 0;
    ed_clamp(e);
    e->rev++;
}
static void ed_stack_push(char*** st, int* n, int* cap, char* s) {
    if (*n >= *cap) {
        int nc = *cap ? *cap * 2 : 16;
        char** ns = (char**)realloc(*st, (size_t)nc * sizeof(char*));
        if (!ns) { free(s); return; }
        *st = ns; *cap = nc;
    }
    (*st)[(*n)++] = s;
}
static void ed_clear_redo(cssc_gui_editor* e) {
    for (int i = 0; i < e->n_redo; i++) free(e->redo[i]);
    e->n_redo = 0;
}
static void ed_push_undo(cssc_gui_editor* e) {
    char* s = ed_doc_string(e);
    if (!s) return;
    if (e->n_undo > 0 && !strcmp(e->undo[e->n_undo - 1], s)) { free(s); return; }
    ed_stack_push(&e->undo, &e->n_undo, &e->cap_undo, s);
    if (e->n_undo > ED_UNDO_MAX) {
        free(e->undo[0]);
        memmove(e->undo, e->undo + 1, (size_t)(e->n_undo - 1) * sizeof(char*));
        e->n_undo--;
    }
    ed_clear_redo(e);
}

static void ed_pre_edit(cssc_gui_editor* e, int op) {
    if (op == 1 && e->last_op == 1) return;
    ed_push_undo(e);
    e->last_op = op;
}
static void ed_undo(cssc_gui_editor* e) {
    if (e->n_undo == 0) return;
    char* cur = ed_doc_string(e);
    if (cur) ed_stack_push(&e->redo, &e->n_redo, &e->cap_redo, cur);
    char* prev = e->undo[--e->n_undo];
    ed_load_doc(e, prev);
    free(prev);
    e->last_op = 2;
}
static void ed_redo(cssc_gui_editor* e) {
    if (e->n_redo == 0) return;
    char* cur = ed_doc_string(e);
    if (cur) ed_stack_push(&e->undo, &e->n_undo, &e->cap_undo, cur);
    char* nxt = e->redo[--e->n_redo];
    ed_load_doc(e, nxt);
    free(nxt);
    e->last_op = 2;
}
static void ed_select_all(cssc_gui_editor* e) {
    e->sel_line = 0; e->sel_col = 0;
    e->cur_line = e->nlines - 1;
    e->cur_col = (int)strlen(e->lines[e->nlines - 1]);
    e->sel_active = (e->nlines > 1 || strlen(e->lines[0]) > 0) ? 1 : 0;
}
static void ed_insert_str(cssc_gui_editor* e, const char* s) {
    for (const char* q = s; *q; ++q) {
        if (*q == '\n') ed_newline(e);
        else if (*q == '\r') continue;
        else if (*q == '\t') { ed_insert_char(e, ' '); ed_insert_char(e, ' ');
                               ed_insert_char(e, ' '); ed_insert_char(e, ' '); }
        else if ((unsigned char)*q >= 32) ed_insert_char(e, *q);
    }
}
static void ed_indent_block(cssc_gui_editor* e) {
    int sl, el;
    if (e->sel_active) { int sc, ec; ed_sel_norm(e, &sl, &sc, &el, &ec); }
    else { sl = el = e->cur_line; }
    for (int i = sl; i <= el; i++) {
        char* line = e->lines[i];
        int ll = (int)strlen(line);
        char* nl = (char*)malloc((size_t)ll + 5);
        if (nl) {
            memcpy(nl, "    ", 4);
            memcpy(nl + 4, line, (size_t)ll + 1);
            free(e->lines[i]); e->lines[i] = nl;
        }
    }
    e->cur_col += 4;
    if (e->sel_active) e->sel_col += 4;
    e->rev++;
    ed_clamp(e);
}
static void ed_dedent_block(cssc_gui_editor* e) {
    int sl, el;
    if (e->sel_active) { int sc, ec; ed_sel_norm(e, &sl, &sc, &el, &ec); }
    else { sl = el = e->cur_line; }
    for (int i = sl; i <= el; i++) {
        char* line = e->lines[i];
        int rm = 0; while (rm < 4 && line[rm] == ' ') rm++;
        if (rm > 0) memmove(line, line + rm, strlen(line) - rm + 1);
    }
    e->cur_col = e->cur_col >= 4 ? e->cur_col - 4 : 0;
    if (e->sel_active) e->sel_col = e->sel_col >= 4 ? e->sel_col - 4 : 0;
    e->rev++;
    ed_clamp(e);
}
static char* ed_selected_str(cssc_gui_editor* e) {
    if (!e->sel_active) return NULL;
    int sl, sc, el, ec; ed_sel_norm(e, &sl, &sc, &el, &ec);
    if (sl == el) {
        int ll = (int)strlen(e->lines[sl]);
        if (ec > ll) ec = ll; if (sc > ll) sc = ll;
        char* o = (char*)malloc((size_t)(ec - sc) + 1);
        if (!o) return NULL;
        memcpy(o, e->lines[sl] + sc, (size_t)(ec - sc)); o[ec - sc] = 0;
        return o;
    }
    size_t total = strlen(e->lines[sl]) - (size_t)sc + 1;
    for (int i = sl + 1; i < el; ++i) total += strlen(e->lines[i]) + 1;
    total += (size_t)ec;
    char* o = (char*)malloc(total + 1);
    if (!o) return NULL;
    size_t off = 0;
    int fl = (int)strlen(e->lines[sl]);
    memcpy(o + off, e->lines[sl] + sc, (size_t)(fl - sc)); off += (size_t)(fl - sc);
    o[off++] = '\n';
    for (int i = sl + 1; i < el; ++i) {
        int l = (int)strlen(e->lines[i]);
        memcpy(o + off, e->lines[i], (size_t)l); off += (size_t)l;
        o[off++] = '\n';
    }
    int lc = (int)strlen(e->lines[el]); if (ec > lc) ec = lc;
    memcpy(o + off, e->lines[el], (size_t)ec); off += (size_t)ec;
    o[off] = 0;
    return o;
}

static void ed_cmp_free_items(cssc_gui_editor* e) {
    for (int i = 0; i < e->cmp_n; i++) free(e->cmp_items[i]);
    e->cmp_n = 0;
}
static void ed_cmp_close(cssc_gui_editor* e) {
    e->cmp_open = 0; e->cmp_pending = 0;
    e->cmp_sel = 0; e->cmp_top = 0; e->cmp_nf = 0;
}

static int ed_ci_starts_with(const char* item, const char* pfx, int plen) {
    for (int i = 0; i < plen; i++) {
        char a = item[i]; if (!a) return 0;
        char b = pfx[i];
        if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
        if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
        if (a != b) return 0;
    }
    return 1;
}

static void ed_cmp_refilter(cssc_gui_editor* e) {
    e->cmp_nf = 0;
    if (e->cmp_n == 0 || e->cmp_line < 0 || e->cmp_line >= e->nlines) return;
    const char* line = e->lines[e->cmp_line];
    int ll = (int)strlen(line);
    int a = e->cmp_start; if (a > ll) a = ll; if (a < 0) a = 0;
    int b = (e->cmp_line == e->cur_line) ? e->cur_col : ll;
    if (b > ll) b = ll; if (b < a) b = a;
    const char* pfx = line + a;
    int plen = b - a;
    if (e->cmp_n > e->cmp_capf) {
        int* nf = (int*)realloc(e->cmp_filt, (size_t)e->cmp_n * sizeof(int));
        if (!nf) return;
        e->cmp_filt = nf; e->cmp_capf = e->cmp_n;
    }
    if (!e->cmp_filt) return;
    for (int i = 0; i < e->cmp_n; i++)
        if (ed_ci_starts_with(e->cmp_items[i], pfx, plen))
            e->cmp_filt[e->cmp_nf++] = i;
    if (e->cmp_sel >= e->cmp_nf) e->cmp_sel = e->cmp_nf - 1;
    if (e->cmp_sel < 0) e->cmp_sel = 0;
}

static char ed_cmp_snippet_bracket(const char* label) {
    static const char* brk[] = {"#stack","#heap","#auto","#delete","#req",
                                 "#free","#load","#unload","#depend", 0};
    static const char* par[] = {"#scanp","#include","#define","#cdefine",
                                 "#DEFINE", 0};
    for (int i = 0; brk[i]; i++) if (!strcmp(label, brk[i])) return '[';
    for (int i = 0; par[i]; i++) if (!strcmp(label, par[i])) return '(';
    return 0;
}

static void ed_cmp_accept(cssc_gui_editor* e) {
    if (!e->cmp_open || e->cmp_nf <= 0) return;
    if (e->cmp_line != e->cur_line) { ed_cmp_close(e); return; }
    const char* label = e->cmp_items[e->cmp_filt[e->cmp_sel]];
    ed_pre_edit(e, 2);
    char* line = e->lines[e->cur_line];
    int ll = (int)strlen(line);
    int pstart = e->cmp_start; if (pstart > ll) pstart = ll; if (pstart < 0) pstart = 0;
    int pend = pstart;
    if (pend < ll && line[pend] == '#') pend++;
    while (pend < ll && ed_isword(line[pend])) pend++;
    memmove(line + pstart, line + pend, (size_t)(ll - pend) + 1);
    e->cur_col = pstart;

    int wrapQuote = 0;
    if (pstart > 0 && line[pstart - 1] == '(') {
        int ts = pstart - 1;
        int ws = ts; while (ws > 0 && ed_isword(line[ws - 1])) ws--;
        if (ws > 0 && line[ws - 1] == '#') {
            int dl = ts - ws;
            if (dl == 7 && memcmp(line + ws, "include", 7) == 0) wrapQuote = 1;
            if (dl == 6 && memcmp(line + ws, "depend", 6) == 0) wrapQuote = 1;
        }
    }
    if (wrapQuote) {
        ed_insert_char(e, '"');
        for (const char* s = label; *s; s++) ed_insert_char(e, *s);
        ed_insert_char(e, '"');
        ed_cmp_close(e);
        e->rev++;
        e->follow = 1;
        return;
    }
    for (const char* s = label; *s; s++) ed_insert_char(e, *s);
    char br = ed_cmp_snippet_bracket(label);
    if (br) {
        char closer = br == '[' ? ']' : ')';
        ed_insert_char(e, br); ed_insert_char(e, closer);
        e->cur_col--;
        ed_cmp_free_items(e);
        e->cmp_open = 0; e->cmp_sel = 0; e->cmp_top = 0; e->cmp_nf = 0;
        e->cmp_start = e->cur_col; e->cmp_line = e->cur_line;
        e->cmp_req = 1; e->cmp_pending = 1;
    } else {
        ed_cmp_close(e);
    }
    e->rev++;
    e->follow = 1;
}

static void ed_cmp_on_type(cssc_gui_editor* e, char cc) {
    if (e->language != 1) { ed_cmp_close(e); return; }
    char* ln = e->lines[e->cur_line];
    int cur = e->cur_col;
    char b2 = cur >= 2 ? ln[cur - 2] : 0;
    int startc = cur;
    while (startc > 0 && ed_isword(ln[startc - 1])) startc--;
    int trig = 0, reqStart = startc;
    if (cc == '.')                    { trig = 1; reqStart = cur; }
    else if (cc == '#')               { trig = 1; reqStart = cur - 1; }
    else if (cc == ':' && b2 == ':')  { trig = 1; reqStart = cur; }
    else if (cc == '>' && b2 == '-')  { trig = 1; reqStart = cur; }
    else if (cc == '[' || cc == '(') {
        int bpos = cur - 1;
        if (bpos >= 0 && (ln[bpos] == '[' || ln[bpos] == '(')) {
            int ws = bpos; while (ws > 0 && ed_isword(ln[ws - 1])) ws--;
            char hsh = ws > 0 ? ln[ws - 1] : 0;
            if (hsh == '#' && ws < bpos) { trig = 1; reqStart = cur; }
        }
    }
    else if (ed_isword(cc)) {
        if (!e->cmp_open && !e->cmp_pending) { trig = 1; reqStart = startc; }
    }
    else { ed_cmp_close(e); return; }
    if (trig) {
        e->cmp_start = reqStart; e->cmp_line = e->cur_line;
        e->cmp_req = 1; e->cmp_pending = 1;
        if (e->cmp_open) ed_cmp_refilter(e);
    } else if (e->cmp_open) {
        ed_cmp_refilter(e);
    }
}

static void ed_sig_close(cssc_gui_editor* e) {
    if (e->sig_text) { free(e->sig_text); e->sig_text = NULL; }
    e->sig_open = 0;
    e->sig_open_line = -1; e->sig_open_col = -1; e->sig_arg = 0;
}

static void ed_sig_scan(cssc_gui_editor* e) {
    if (e->language != 1 || e->cmp_open) { ed_sig_close(e); return; }
    const char* L = e->lines[e->cur_line];
    int col = e->cur_col;
    int ll = (int)strlen(L); if (col > ll) col = ll;
    int depth = 0, open = -1;
    for (int j = col - 1; j >= 0; j--) {
        char c = L[j];
        if (c == ')' || c == ']' || c == '}') depth++;
        else if (c == '(' || c == '[' || c == '{') {
            if (c == '(' && depth == 0) { open = j; break; }
            if (depth > 0) depth--;
        }
    }
    int inCall = 0, activeArg = 0;
    if (open >= 0) {
        int k = open; while (k > 0 && (L[k - 1] == ' ' || L[k - 1] == '\t')) k--;
        if (k > 0 && ed_isword(L[k - 1])) {
            inCall = 1;
            int d2 = 0;
            for (int j = open + 1; j < col && L[j]; j++) {
                char c = L[j];
                if (c == '(' || c == '[' || c == '{') d2++;
                else if (c == ')' || c == ']' || c == '}') { if (d2 > 0) d2--; }
                else if (c == ',' && d2 == 0) activeArg++;
            }
        }
    }
    if (!inCall) { ed_sig_close(e); return; }
    if (open != e->sig_open_col || e->cur_line != e->sig_open_line) {
        e->sig_open_col = open; e->sig_open_line = e->cur_line;
        e->sig_arg = activeArg;
        e->sig_line = e->cur_line; e->sig_col = e->cur_col;
        e->sig_req = 1;
    } else {
        e->sig_arg = activeArg;
    }
}

CSSC_GUI_EXPORT void* cssc_gui_editor_new(void* screen) {
    cssc_gui_editor* e = (cssc_gui_editor*)calloc(1, sizeof(cssc_gui_editor));
    if (!e) return NULL;
    e->kind = GW_EDITOR;
    e->screen = (cssc_gui_screen*)screen;
    e->x = 20; e->y = 20; e->w = 600; e->h = 360;
    e->scale = 2;
    e->bg = (int64_t)0xFF0E1016;
    e->fg = (int64_t)0xFFD8DEE9;
    e->gutter = (int64_t)0xFF5A6473;
    e->cursor_c = (int64_t)0xFF3FD0A0;
    e->focused = 1; e->visible = 1; e->gutter_on = 1;
    ed_insert_line(e, 0, "");
    return e;
}
CSSC_GUI_EXPORT void cssc_gui_editor_setrect(void* p, int64_t x, int64_t y,
                                             int64_t w, int64_t h) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (e) { e->x = x; e->y = y; e->w = w; e->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_editor_settext(void* p, const char* s) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    for (int i = 0; i < e->nlines; ++i) free(e->lines[i]);
    e->nlines = 0;
    if (!s) s = "";
    const char* start = s;
    for (const char* q = s; ; ++q) {
        if (*q == '\n' || *q == 0) {
            int len = (int)(q - start);
            if (len > 0 && start[len - 1] == '\r') len--;
            char* ln = (char*)malloc((size_t)len + 1);
            if (ln) {
                memcpy(ln, start, (size_t)len); ln[len] = 0;
                ed_grow_lines(e, e->nlines + 1);
                if (e->cap_lines >= e->nlines + 1) e->lines[e->nlines++] = ln;
                else free(ln);
            }
            start = q + 1;
            if (*q == 0) break;
        }
    }
    if (e->nlines == 0) ed_insert_line(e, 0, "");
    e->cur_line = 0; e->cur_col = 0; e->top_line = 0;
    if (e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
    ed_cmp_free_items(e); ed_cmp_close(e);
    if (e->hov_text) { free(e->hov_text); e->hov_text = NULL; }
    e->hov_open = 0; e->hov_dwell = 0; e->hov_line = -1;
    ed_sig_close(e);
    e->dg_n = 0;
}
CSSC_GUI_EXPORT void* cssc_gui_editor_text(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || e->nlines == 0) return cssc_string_lit("", 0);
    size_t total = 0;
    for (int i = 0; i < e->nlines; ++i) total += strlen(e->lines[i]) + 1;
    char* out = (char*)malloc(total + 1);
    if (!out) return cssc_string_lit("", 0);
    size_t off = 0;
    for (int i = 0; i < e->nlines; ++i) {
        size_t ln = strlen(e->lines[i]);
        memcpy(out + off, e->lines[i], ln); off += ln;
        if (i < e->nlines - 1) out[off++] = '\n';
    }
    out[off] = 0;
    void* r = cssc_string_lit(out, off);
    free(out);
    return r;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_linecount(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? (int64_t)e->nlines : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_cursorline(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? (int64_t)(e->cur_line + 1) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_cursorcol(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? (int64_t)(e->cur_col + 1) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_revision(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? e->rev : 0;
}

CSSC_GUI_EXPORT void cssc_gui_editor_setipline(void* p, int64_t line) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (!e) return;
    int prev = e->ip_line;
    e->ip_line = (int)line;
    if (e->ip_line > 0 && e->ip_line != prev) {
        int target = e->ip_line - 1;
        if (target >= 0 && target < e->nlines) {
            e->cur_line = target;
            if (e->cur_col < 0) e->cur_col = 0;
            e->follow = 1;
        }
    }
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_caretpixelx(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int64_t glyph, line_h, gutter_w, text_x;
    ed_metrics(e, &glyph, &line_h, &gutter_w, &text_x);
    return text_x + (int64_t)(e->cur_col - e->left_col) * glyph;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_caretpixely(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int64_t glyph, line_h, gutter_w, text_x;
    ed_metrics(e, &glyph, &line_h, &gutter_w, &text_x);
    return e->y + 5 + (int64_t)(e->cur_line - e->top_line) * line_h;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_lineheight(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    return 8 * e->scale + 4 * e->scale;
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_completereq(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int r = e->cmp_req; e->cmp_req = 0; return r;
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_completeactive(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    return (e && e->cmp_open) ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_completecancel(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (e) { ed_cmp_free_items(e); ed_cmp_close(e); }
}

CSSC_GUI_EXPORT void* cssc_gui_editor_completionquery(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || e->nlines == 0) return cssc_string_lit("", 0);
    int cl = e->cmp_line; if (cl < 0) cl = 0; if (cl >= e->nlines) cl = e->nlines - 1;
    int cll = (int)strlen(e->lines[cl]);
    int cs = e->cmp_start; if (cs < 0) cs = 0; if (cs > cll) cs = cll;
    if (cs < cll && e->lines[cl][cs] == '#') cs++;
    size_t total = 1;
    for (int i = 0; i < e->nlines; i++) total += strlen(e->lines[i]) + 1;
    char* out = (char*)malloc(total + 1);
    if (!out) return cssc_string_lit("", 0);
    size_t off = 0;
    for (int i = 0; i < e->nlines; i++) {
        const char* ln = e->lines[i]; size_t len = strlen(ln);
        if (i == cl) {
            memcpy(out + off, ln, (size_t)cs); off += (size_t)cs;
            out[off++] = 4;
            memcpy(out + off, ln + cs, len - (size_t)cs); off += (len - (size_t)cs);
        } else {
            memcpy(out + off, ln, len); off += len;
        }
        if (i < e->nlines - 1) out[off++] = '\n';
    }
    out[off] = 0;
    void* r = cssc_string_lit(out, off);
    free(out);
    return r;
}

CSSC_GUI_EXPORT void cssc_gui_editor_setcompletions(void* p, const char* text) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    ed_cmp_free_items(e);
    e->cmp_pending = 0;
    if (text && *text) {
        const char* start = text;
        for (const char* q = text; ; ++q) {
            if (*q == '\n' || *q == 0) {
                int len = (int)(q - start);
                if (len > 0 && start[len - 1] == '\r') len--;
                if (len > 0) {
                    char* item = (char*)malloc((size_t)len + 1);
                    if (item) {
                        memcpy(item, start, (size_t)len); item[len] = 0;
                        if (e->cmp_n >= e->cmp_cap) {
                            int nc = e->cmp_cap ? e->cmp_cap * 2 : 32;
                            char** ni = (char**)realloc(e->cmp_items,
                                                        (size_t)nc * sizeof(char*));
                            if (ni) { e->cmp_items = ni; e->cmp_cap = nc; }
                            else { free(item); item = NULL; }
                        }
                        if (item) e->cmp_items[e->cmp_n++] = item;
                    }
                }
                start = q + 1;
                if (*q == 0) break;
            }
        }
    }
    if (e->cmp_n > 0) {
        e->cmp_sel = 0; e->cmp_top = 0; e->cmp_open = 1;
        ed_cmp_refilter(e);
    } else {
        ed_cmp_close(e);
    }
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_hoverreq(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int r = e->hov_req; e->hov_req = 0; return r;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_hoveractive(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    return (e && e->hov_open) ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_hovercancel(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    if (e->hov_text) { free(e->hov_text); e->hov_text = NULL; }
    e->hov_open = 0; e->hov_dwell = 0;
}

CSSC_GUI_EXPORT void* cssc_gui_editor_hoverquery(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || e->nlines == 0) return cssc_string_lit("", 0);
    int cl = e->hov_line; if (cl < 0) cl = 0; if (cl >= e->nlines) cl = e->nlines - 1;
    int cll = (int)strlen(e->lines[cl]);
    int cs = e->hov_col; if (cs < 0) cs = 0; if (cs > cll) cs = cll;
    size_t total = 1;
    for (int i = 0; i < e->nlines; i++) total += strlen(e->lines[i]) + 1;
    char* out = (char*)malloc(total + 1);
    if (!out) return cssc_string_lit("", 0);
    size_t off = 0;
    for (int i = 0; i < e->nlines; i++) {
        const char* ln = e->lines[i]; size_t len = strlen(ln);
        if (i == cl) {
            memcpy(out + off, ln, (size_t)cs); off += (size_t)cs;
            out[off++] = 4;
            memcpy(out + off, ln + cs, len - (size_t)cs); off += (len - (size_t)cs);
        } else {
            memcpy(out + off, ln, len); off += len;
        }
        if (i < e->nlines - 1) out[off++] = '\n';
    }
    out[off] = 0;
    void* r = cssc_string_lit(out, off);
    free(out);
    return r;
}

CSSC_GUI_EXPORT void cssc_gui_editor_sethover(void* p, const char* text) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    if (e->hov_text) { free(e->hov_text); e->hov_text = NULL; }
    int has = 0;
    if (text) for (const char* q = text; *q; q++) if (*q > ' ') { has = 1; break; }
    if (has) {
        size_t n = strlen(text);
        e->hov_text = (char*)malloc(n + 1);
        if (e->hov_text) { memcpy(e->hov_text, text, n + 1); e->hov_open = 1; }
        else e->hov_open = 0;
    } else {
        e->hov_open = 0;
    }
}

CSSC_GUI_EXPORT void cssc_gui_editor_setdiagnostics(void* p, const char* text) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    e->dg_n = 0;
    if (!text) return;
    const char* q = text;
    while (*q) {
        while (*q == '\r' || *q == '\n') q++;
        if (!*q) break;
        int ln = 0, col = 0, sev = 0, field = 0, seen = 0;
        while (*q && *q != '\n' && *q != '\r') {
            char ch = *q;
            if (ch >= '0' && ch <= '9') {
                int d = ch - '0';
                if (field == 0) ln = ln * 10 + d;
                else if (field == 1) col = col * 10 + d;
                else if (field == 2) sev = sev * 10 + d;
                seen = 1;
            } else if (ch == ':') {
                field++;
                if (field > 2) { while (*q && *q != '\n') q++; break; }
            }
            if (*q) q++;
        }
        if (seen && ln > 0) {
            if (e->dg_n >= e->dg_cap) {
                int nc = e->dg_cap ? e->dg_cap * 2 : 32;
                int* nl = (int*)realloc(e->dg_line, (size_t)nc * sizeof(int));
                int* nco = (int*)realloc(e->dg_col, (size_t)nc * sizeof(int));
                int* ns = (int*)realloc(e->dg_sev, (size_t)nc * sizeof(int));
                if (nl) e->dg_line = nl;
                if (nco) e->dg_col = nco;
                if (ns) e->dg_sev = ns;
                if (nl && nco && ns) e->dg_cap = nc; else break;
            }
            e->dg_line[e->dg_n] = ln;
            e->dg_col[e->dg_n] = col;
            e->dg_sev[e->dg_n] = sev ? sev : 1;
            e->dg_n++;
        }
    }
}

static void ed_sticky_clear(cssc_gui_editor* e) {
    if (!e) return;
    for (int i = 0; i < e->sd_n; i++) { if (e->sd_msg[i]) free(e->sd_msg[i]); }
    e->sd_n = 0;
    e->sd_on = 0;
}

CSSC_GUI_EXPORT void cssc_gui_editor_setstickydiag(void* p, const char* text) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    ed_sticky_clear(e);
    if (!text) return;
    const char* q = text;
    while (*q) {
        while (*q == '\r' || *q == '\n') q++;
        if (!*q) break;
        int ln = 0, sev = 0, field = 0, seen = 0;
        const char* msg = NULL; int mlen = 0;
        while (*q && *q != '\n' && *q != '\r') {
            char ch = *q;
            if (ch == ':' && field < 4) {
                field++;
                if (field == 4) {
                    msg = q + 1;
                    const char* m = msg;
                    while (*m && *m != '\n' && *m != '\r') m++;
                    mlen = (int)(m - msg);
                    q = m;
                    break;
                }
            } else if (ch >= '0' && ch <= '9') {
                int d = ch - '0';
                if (field == 0) ln = ln * 10 + d;
                else if (field == 2) sev = sev * 10 + d;
                seen = 1;
            }
            if (*q) q++;
        }
        if (seen && ln > 0) {
            if (e->sd_n >= e->sd_cap) {
                int nc = e->sd_cap ? e->sd_cap * 2 : 32;
                int* nl = (int*)realloc(e->sd_line, (size_t)nc * sizeof(int));
                int* ns = (int*)realloc(e->sd_sev, (size_t)nc * sizeof(int));
                char** nm = (char**)realloc(e->sd_msg, (size_t)nc * sizeof(char*));
                if (nl) e->sd_line = nl;
                if (ns) e->sd_sev = ns;
                if (nm) e->sd_msg = nm;
                if (nl && ns && nm) e->sd_cap = nc; else break;
            }
            e->sd_line[e->sd_n] = ln;
            e->sd_sev[e->sd_n] = sev ? sev : 1;
            char* cp = (char*)malloc((size_t)mlen + 1);
            if (cp) { if (mlen > 0) memcpy(cp, msg, (size_t)mlen); cp[mlen] = 0; }
            e->sd_msg[e->sd_n] = cp;
            e->sd_n++;
        }
    }
    if (e->sd_n > 0) e->sd_on = 1;
}
CSSC_GUI_EXPORT void cssc_gui_editor_clearstickydiag(void* p) {
    ed_sticky_clear((cssc_gui_editor*)p);
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_stickycount(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? (int64_t)e->sd_n : 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_sigreq(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int r = e->sig_req; e->sig_req = 0; return r;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_sigactive(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    return (e && e->sig_open) ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_sigcancel(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (e) ed_sig_close(e);
}

CSSC_GUI_EXPORT void* cssc_gui_editor_sigquery(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || e->nlines == 0) return cssc_string_lit("", 0);
    int cl = e->sig_line; if (cl < 0) cl = 0; if (cl >= e->nlines) cl = e->nlines - 1;
    int cll = (int)strlen(e->lines[cl]);
    int cs = e->sig_col; if (cs < 0) cs = 0; if (cs > cll) cs = cll;
    size_t total = 1;
    for (int i = 0; i < e->nlines; i++) total += strlen(e->lines[i]) + 1;
    char* out = (char*)malloc(total + 1);
    if (!out) return cssc_string_lit("", 0);
    size_t off = 0;
    for (int i = 0; i < e->nlines; i++) {
        const char* ln = e->lines[i]; size_t len = strlen(ln);
        if (i == cl) {
            memcpy(out + off, ln, (size_t)cs); off += (size_t)cs;
            out[off++] = 4;
            memcpy(out + off, ln + cs, len - (size_t)cs); off += (len - (size_t)cs);
        } else {
            memcpy(out + off, ln, len); off += len;
        }
        if (i < e->nlines - 1) out[off++] = '\n';
    }
    out[off] = 0;
    void* r = cssc_string_lit(out, off);
    free(out);
    return r;
}

CSSC_GUI_EXPORT void cssc_gui_editor_setsignature(void* p, const char* text) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    if (e->sig_text) { free(e->sig_text); e->sig_text = NULL; }
    int len = 0;
    if (text) { while (text[len] && text[len] != '\n' && text[len] != '\r') len++; }
    if (len > 0) {
        e->sig_text = (char*)malloc((size_t)len + 1);
        if (e->sig_text) { memcpy(e->sig_text, text, (size_t)len); e->sig_text[len] = 0;
                           e->sig_open = 1; }
        else e->sig_open = 0;
    } else {
        e->sig_open = 0;
    }
}
CSSC_GUI_EXPORT void cssc_gui_editor_gotoline(void* p, int64_t n) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    e->cur_line = (int)n - 1; e->cur_col = 0; ed_clamp(e);
    ed_cmp_close(e);
}
CSSC_GUI_EXPORT void cssc_gui_editor_setcursor(void* p, int64_t line, int64_t col) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    e->cur_line = (int)line - 1; e->cur_col = (int)col - 1;
    e->sel_active = 0;
    ed_clamp(e);
    ed_cmp_close(e);
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_hasselection(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    return (e && e->sel_active) ? 1 : 0;
}

CSSC_GUI_EXPORT void cssc_gui_editor_insert(void* p, const char* s) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !s || !*s) return;
    ed_clamp(e);
    ed_push_undo(e);
    for (const char* q = s; *q; q++) {
        if (*q == '\r') continue;
        if (*q == '\n') ed_newline(e);
        else if ((unsigned char)*q >= 32) ed_insert_char(e, *q);
    }
    e->sel_active = 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_undo(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) ed_undo(e);
}
CSSC_GUI_EXPORT void cssc_gui_editor_redo(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) ed_redo(e);
}
CSSC_GUI_EXPORT void cssc_gui_editor_selectall(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) ed_select_all(e);
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_search(void* p, const char* term) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    if (e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
    if (!term || !term[0]) return 0;
    e->search = gui_strdup(term);
    e->search_len = (int)strlen(term);
    int count = 0;
    for (int i = 0; i < e->nlines; ++i) {
        const char* hay = e->lines[i];
        const char* hit = hay;
        while ((hit = strstr(hit, e->search)) != NULL) { count++; hit += e->search_len; }
    }

    for (int i = 0; i < e->nlines; ++i) {
        const char* hit = strstr(e->lines[i], e->search);
        if (hit) { e->cur_line = i; e->cur_col = (int)(hit - e->lines[i]);
                   e->sel_active = 0; ed_clamp(e); break; }
    }
    return count;
}
CSSC_GUI_EXPORT void cssc_gui_editor_clearsearch(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (e && e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_searchcount(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !e->search) return 0;
    int count = 0;
    for (int i = 0; i < e->nlines; ++i) {
        const char* hit = e->lines[i];
        while ((hit = strstr(hit, e->search)) != NULL) { count++; hit += e->search_len; }
    }
    return count;
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_replaceall(void* p, const char* find,
                                                   const char* repl) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !find || !find[0]) return 0;
    if (!repl) repl = "";
    int flen = (int)strlen(find), rlen = (int)strlen(repl);

    int total = 0;
    for (int i = 0; i < e->nlines; ++i) {
        const char* hit = e->lines[i];
        while ((hit = strstr(hit, find)) != NULL) { total++; hit += flen; }
    }
    if (total == 0) return 0;
    ed_pre_edit(e, 3);
    for (int i = 0; i < e->nlines; ++i) {
        const char* src = e->lines[i];
        int hits = 0;
        for (const char* h = src; (h = strstr(h, find)) != NULL; h += flen) hits++;
        if (hits == 0) continue;
        int slen = (int)strlen(src);
        int nlen = slen + hits * (rlen - flen);
        char* out = (char*)malloc((size_t)nlen + 1);
        if (!out) continue;
        int o = 0; const char* s = src;
        while (*s) {
            if (!strncmp(s, find, (size_t)flen)) {
                memcpy(out + o, repl, (size_t)rlen); o += rlen; s += flen;
            } else out[o++] = *s++;
        }
        out[o] = 0;
        free(e->lines[i]); e->lines[i] = out;
    }
    if (e->search) { free(e->search); e->search = gui_strdup(repl);
                     e->search_len = rlen; }
    ed_clamp(e);
    e->rev++;
    return total;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_saverequested(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int r = e->save_req; e->save_req = 0;
    return r;
}
CSSC_GUI_EXPORT void* cssc_gui_editor_selectedtext(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    char* sel = e ? ed_selected_str(e) : NULL;
    if (!sel) return cssc_string_lit("", 0);
    void* r = cssc_string_lit(sel, strlen(sel));
    free(sel);
    return r;
}
CSSC_GUI_EXPORT void cssc_gui_editor_setfocus(void* p, int64_t f) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) e->focused = f ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_setscale(void* p, int64_t s) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) e->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_editor_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (e) { e->bg = bg & 0xFFFFFFFF; e->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT void cssc_gui_editor_setgutter(void* p, int64_t on) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) e->gutter_on = on ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_setvisible(void* p, int64_t vis) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) e->visible = vis ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_setlanguage(void* p, int64_t lang) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) e->language = (int)lang;
}

CSSC_GUI_EXPORT void cssc_gui_editor_setlangforpath(void* p, const char* path) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    int lang = 0;
    if (path) {
        const char* dot = strrchr(path, '.');
        if (dot && !strcmp(dot, ".cssc")) lang = 1;
    }
    e->language = lang;
}

static int own_read_int(const char* s, int len, int* pos) {
    int p = *pos;
    while (p < len && (s[p] == ' ' || s[p] == '\t')) p++;
    int neg = 0;
    if (p < len && s[p] == '-') { neg = 1; p++; }
    int v = 0, any = 0;
    while (p < len && s[p] >= '0' && s[p] <= '9') { v = v * 10 + (s[p] - '0'); p++; any = 1; }
    *pos = p;
    if (!any) return -999999;
    return neg ? -v : v;
}

CSSC_GUI_EXPORT void cssc_gui_editor_setownership(void* p, const char* text) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    e->own_na = 0; e->own_nd = 0;
    if (!text) return;
    int len = (int)strlen(text);
    int need = 1;
    for (int i = 0; i < len; i++) if (text[i] == '\n') need++;
    if (need > e->own_cap_a) {
        free(e->own_aline); free(e->own_afreed); free(e->own_aleaked);
        e->own_aline = (int*)malloc((size_t)need * sizeof(int));
        e->own_afreed = (int*)malloc((size_t)need * sizeof(int));
        e->own_aleaked = (int*)malloc((size_t)need * sizeof(int));
        e->own_cap_a = (e->own_aline && e->own_afreed && e->own_aleaked) ? need : 0;
    }
    if (need > e->own_cap_d) {
        free(e->own_dline); free(e->own_dtarget);
        e->own_dline = (int*)malloc((size_t)need * sizeof(int));
        e->own_dtarget = (int*)malloc((size_t)need * sizeof(int));
        e->own_cap_d = (e->own_dline && e->own_dtarget) ? need : 0;
    }
    int i = 0;
    while (i < len) {
        char tag = text[i];
        int ls = i;
        while (i < len && text[i] != '\n') i++;
        int le = i;
        if (i < len) i++;
        if (tag == 'A') {
            int pos = ls + 1;
            int aline = own_read_int(text, le, &pos);
            (void)own_read_int(text, le, &pos);
            (void)own_read_int(text, le, &pos);
            int afreed = own_read_int(text, le, &pos);
            int aleak = own_read_int(text, le, &pos);
            if (aline > 0 && e->own_na < e->own_cap_a) {
                e->own_aline[e->own_na] = aline;
                e->own_afreed[e->own_na] = (afreed == 1) ? 1 : 0;
                e->own_aleaked[e->own_na] = (aleak == 1) ? 1 : 0;
                e->own_na++;
            }
        } else if (tag == 'D') {
            int pos = ls + 1;
            int dline = own_read_int(text, le, &pos);
            (void)own_read_int(text, le, &pos);
            int dtgt = own_read_int(text, le, &pos);
            if (dline > 0 && e->own_nd < e->own_cap_d) {
                e->own_dline[e->own_nd] = dline;
                e->own_dtarget[e->own_nd] = dtgt;
                e->own_nd++;
            }
        }
    }
}

CSSC_GUI_EXPORT int64_t cssc_gui_editor_update(void* ep, void* sp) {
    cssc_gui_editor* e = (cssc_gui_editor*)ep;
    if (!e) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : e->screen;
    if (!s || !e->visible) return 0;
    ed_clamp(e);

    int ml, mc;
    if (e->dbl_timer > 0) e->dbl_timer--;
    if (s->down && !s->prev_down && !s->input_captured) {
        if (ed_hittest(e, s->mx, s->my, &ml, &mc)) {
            if (e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
            ed_cmp_close(e);
            e->follow = 1;
            int is_dbl = (e->dbl_timer > 0 && ml == e->dbl_line &&
                          (mc - e->dbl_col <= 1 && e->dbl_col - mc <= 1));
            e->cur_line = ml; e->cur_col = mc;
            if (is_dbl) {
                ed_select_word(e, ml, mc);
                e->dragging = 0;
            } else {
                e->sel_line = ml; e->sel_col = mc;
                e->sel_active = 0;
                e->dragging = 1;
            }
            e->dbl_line = ml; e->dbl_col = mc; e->dbl_timer = 20;
        }
    } else if (s->down && e->dragging) {
        if (ed_hittest(e, s->mx, s->my, &ml, &mc)) {
            e->cur_line = ml; e->cur_col = mc;
            e->follow = 1;
            if (ml != e->sel_line || mc != e->sel_col) e->sel_active = 1;
        }
    } else if (!s->down) {
        e->dragging = 0;
    }

    {
        int overed = (e->language == 1 && !s->input_captured && !e->cmp_open &&
                      !s->down && !e->dragging &&
                      s->mx >= e->x && s->mx < e->x + e->w &&
                      s->my >= e->y && s->my < e->y + e->h);
        int onword = 0, hl = 0, hc = 0, ws = 0, we = 0;
        if (overed && ed_hittest(e, s->mx, s->my, &hl, &hc)) {
            const char* L = e->lines[hl];
            int ll = (int)strlen(L);
            if (hc < ll && ed_isword(L[hc])) {
                onword = 1; ws = hc; we = hc;
                while (ws > 0 && ed_isword(L[ws - 1])) ws--;
                while (we < ll && ed_isword(L[we])) we++;
            }
        }
        if (!overed || !onword) {
            e->hov_dwell = 0;
            if (e->hov_open) { free(e->hov_text); e->hov_text = NULL; e->hov_open = 0; }
            e->hov_line = -1;
        } else if (hl == e->hov_line && ws == e->hov_ws && we == e->hov_we) {
            if (!e->hov_open) {
                if (e->hov_dwell < 100000) e->hov_dwell++;
                if (e->hov_dwell == 22) { e->hov_col = hc; e->hov_req = 1; }
            }
        } else {
            if (e->hov_open) { free(e->hov_text); e->hov_text = NULL; e->hov_open = 0; }
            e->hov_line = hl; e->hov_ws = ws; e->hov_we = we; e->hov_col = hc;
            e->hov_dwell = 0;
        }
    }

    if (!e->focused || s->input_captured) { ed_sig_close(e); return 0; }

    int shift = (int)s->shift;

    int64_t c;
    while ((c = cssc_video_poll_char(s->vid)) != 0) {
        e->follow = 1;
        char* line = e->lines[e->cur_line];

        if (e->cmp_open && e->cmp_nf > 0 && (c == 13 || c == 10 || c == 9)) {
            ed_cmp_accept(e); continue;
        }
        if (e->cmp_open && c == 27) { ed_cmp_close(e); continue; }
        if (c == 8) {
            ed_pre_edit(e, 2);
            if (e->sel_active) { ed_delete_selection(e); }
            else {
                char prev = e->cur_col > 0 ? line[e->cur_col - 1] : 0;
                char next = line[e->cur_col];
                if ((prev == '(' && next == ')') || (prev == '[' && next == ']') ||
                    (prev == '{' && next == '}') || (prev == '"' && next == '"')) {
                    ed_delete(e); ed_backspace(e);
                } else ed_backspace(e);
            }
            if (e->cmp_open) {
                if (e->cur_line != e->cmp_line || e->cur_col < e->cmp_start)
                    ed_cmp_close(e);
                else ed_cmp_refilter(e);
            }
        } else if (c == 13 || c == 10) {
            ed_pre_edit(e, 2);
            if (e->sel_active) ed_delete_selection(e);
            line = e->lines[e->cur_line];
            int indent = 0; while (line[indent] == ' ' && indent < e->cur_col) indent++;
            char before = e->cur_col > 0 ? line[e->cur_col - 1] : 0;
            char after = line[e->cur_col];
            ed_newline(e);
            for (int i = 0; i < indent; i++) ed_insert_char(e, ' ');
            if (before == '{') {
                for (int i = 0; i < 4; i++) ed_insert_char(e, ' ');
                if (after == '}') {
                    int sline = e->cur_line, scol = e->cur_col;
                    ed_newline(e);
                    for (int i = 0; i < indent; i++) ed_insert_char(e, ' ');
                    e->cur_line = sline; e->cur_col = scol;
                }
            }
        } else if (c == 9) {
            ed_pre_edit(e, 2);
            if (shift) ed_dedent_block(e);
            else if (e->sel_active) ed_indent_block(e);
            else { ed_insert_char(e, ' '); ed_insert_char(e, ' ');
                   ed_insert_char(e, ' '); ed_insert_char(e, ' '); }
        } else if (c == 1) {
            ed_select_all(e);
        } else if (c == 3) {
            char* sel = ed_selected_str(e);
            if (sel) { gui_clipboard_set(sel); free(sel); }
        } else if (c == 24) {
            char* sel = ed_selected_str(e);
            if (sel) { gui_clipboard_set(sel); free(sel);
                       ed_pre_edit(e, 2); ed_delete_selection(e); }
        } else if (c == 22) {
            char* clip = gui_clipboard_get();
            if (clip) { ed_pre_edit(e, 2);
                        if (e->sel_active) ed_delete_selection(e);
                        ed_insert_str(e, clip); free(clip); }
        } else if (c == 26) {
            ed_undo(e); ed_cmp_close(e);
        } else if (c == 25) {
            ed_redo(e); ed_cmp_close(e);
        } else if (c == 19) {
            e->save_req = 1;
        } else if (c >= 32 && c < 127) {
            if (e->sel_active) { ed_pre_edit(e, 2); ed_delete_selection(e); }
            else ed_pre_edit(e, 1);
            line = e->lines[e->cur_line];
            char cc = (char)c;
            char at = line[e->cur_col];
            if (cc == '(' || cc == '[' || cc == '{') {
                char closer = cc == '(' ? ')' : cc == '[' ? ']' : '}';
                ed_insert_char(e, cc); ed_insert_char(e, closer); e->cur_col--;
            } else if (cc == '"') {
                if (at == '"') e->cur_col++;
                else { ed_insert_char(e, '"'); ed_insert_char(e, '"'); e->cur_col--; }
            } else if ((cc == ')' || cc == ']' || cc == '}') && at == cc) {
                e->cur_col++;
            } else {
                ed_insert_char(e, cc);
            }
            ed_cmp_on_type(e, cc);
        }
    }
    int64_t k;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        e->follow = 1;

        if (e->cmp_open && e->cmp_nf > 0 && !(int)s->ctrl) {
            if (k == 0x26) { e->cmp_sel = (e->cmp_sel > 0) ? e->cmp_sel - 1
                                                           : e->cmp_nf - 1; continue; }
            if (k == 0x28) { e->cmp_sel = (e->cmp_sel < e->cmp_nf - 1) ? e->cmp_sel + 1
                                                                       : 0; continue; }
            if (k == 0x21) { e->cmp_sel -= 8; if (e->cmp_sel < 0) e->cmp_sel = 0; continue; }
            if (k == 0x22) { e->cmp_sel += 8; if (e->cmp_sel >= e->cmp_nf)
                                 e->cmp_sel = e->cmp_nf - 1; continue; }
            if (k == 0x25 || k == 0x27 || k == 0x24 || k == 0x23) ed_cmp_close(e);
        }

        if ((int)s->ctrl && (k == 0xBB || k == 0x6B || k == 0xBD || k == 0x6D)) {
            if ((int)s->alt) {
                e->scale = 2;
            } else {
                int inc = (k == 0xBB || k == 0x6B) ? 1 : -1;
                e->scale += inc;
                if (e->scale < 1) e->scale = 1;
                if (e->scale > 6) e->scale = 6;
            }
            e->follow = 1;
            continue;
        }
        if (k == 0x28 && (int)s->ctrl) {
            ed_pre_edit(e, 2);
            char* bumped = ed_scanp_bump_dup(e->lines[e->cur_line]);
            ed_insert_line(e, e->cur_line + 1, bumped ? bumped : e->lines[e->cur_line]);
            if (bumped) free(bumped);
            e->cur_line++;
            ed_clamp(e);
            e->rev++;
            continue;
        }
        int is_nav = (k == 0x25 || k == 0x27 || k == 0x26 || k == 0x28 ||
                      k == 0x24 || k == 0x23 || k == 0x21 || k == 0x22);
        if (is_nav) {
            if (shift && !e->sel_active) { e->sel_line = e->cur_line; e->sel_col = e->cur_col; }
            int ll = (int)strlen(e->lines[e->cur_line]);
            int ctrl = (int)s->ctrl;
            if (k == 0x25) {
                if (ctrl) {
                    const char* L = e->lines[e->cur_line];
                    int col = e->cur_col;
                    if (col == 0) {
                        if (e->cur_line > 0) { e->cur_line--; e->cur_col = (int)strlen(e->lines[e->cur_line]); }
                    } else {
                        while (col > 0 && (L[col - 1] == ' ' || L[col - 1] == '\t')) col--;
                        if (col > 0 && ed_isword(L[col - 1]))
                            while (col > 0 && ed_isword(L[col - 1])) col--;
                        else
                            while (col > 0 && !ed_isword(L[col - 1]) && L[col - 1] != ' ' && L[col - 1] != '\t') col--;
                        e->cur_col = col;
                    }
                } else {
                    if (e->cur_col > 0) e->cur_col--;
                    else if (e->cur_line > 0) { e->cur_line--; e->cur_col = (int)strlen(e->lines[e->cur_line]); }
                }
            } else if (k == 0x27) {
                if (ctrl) {
                    const char* L = e->lines[e->cur_line];
                    int col = e->cur_col;
                    if (col >= ll) {
                        if (e->cur_line < e->nlines - 1) { e->cur_line++; e->cur_col = 0; }
                    } else {
                        while (col < ll && (L[col] == ' ' || L[col] == '\t')) col++;
                        if (col < ll && ed_isword(L[col]))
                            while (col < ll && ed_isword(L[col])) col++;
                        else
                            while (col < ll && !ed_isword(L[col]) && L[col] != ' ' && L[col] != '\t') col++;
                        e->cur_col = col;
                    }
                } else {
                    if (e->cur_col < ll) e->cur_col++;
                    else if (e->cur_line < e->nlines - 1) { e->cur_line++; e->cur_col = 0; }
                }
            } else if (k == 0x26) { if (e->cur_line > 0) e->cur_line--; }
            else if (k == 0x28) { if (e->cur_line < e->nlines - 1) e->cur_line++; }
            else if (k == 0x24) e->cur_col = 0;
            else if (k == 0x23) e->cur_col = (int)strlen(e->lines[e->cur_line]);
            else if (k == 0x21) e->cur_line -= 10;
            else if (k == 0x22) e->cur_line += 10;
            if (shift) e->sel_active = (e->sel_line != e->cur_line || e->sel_col != e->cur_col) ? 1 : 0;
            else e->sel_active = 0;
            e->last_op = 2;
            ed_clamp(e);
        } else if (k == 0x2E) {
            ed_pre_edit(e, 2);
            if (e->sel_active) ed_delete_selection(e); else ed_delete(e);
            ed_clamp(e);
        }
    }
    int64_t wh = cssc_video_wheel(s->vid);
    if (wh) {
        e->top_line -= (int)wh * 3;
        if (e->top_line < 0) e->top_line = 0;
        if (e->top_line >= e->nlines) e->top_line = e->nlines - 1;
    }
    ed_sig_scan(e);
    return 0;
}

enum { TK_IDENT, TK_KW, TK_TYPE, TK_NS, TK_FUNC, TK_DIR,
       TK_STR, TK_NUM, TK_COMMENT, TK_PUNCT, TK_COPY, TK_REF, TK_WS,
       TK_FREE, TK_VIS, TK_GENERIC, TK_LABEL, TK_MUTIER, TK_COUNT };

static int64_t g_tk_palette[TK_COUNT] = {
    (int64_t)0xFFE6E2F0,
    (int64_t)0xFFE060C0,
    (int64_t)0xFF4EC9B0,
    (int64_t)0xFF4EC9B0,
    (int64_t)0xFF4DA6FF,
    (int64_t)0xFFC77DFF,
    (int64_t)0xFFF0A0C0,
    (int64_t)0xFFB5CEA8,
    (int64_t)0xFF6E7A6A,
    (int64_t)0xFFB0AAC0,
    (int64_t)0xFFFFD700,
    (int64_t)0xFFFFD700,
    (int64_t)0xFFE6E2F0,
    (int64_t)0xFFFF9E64,
    (int64_t)0xFFE5C07B,
    (int64_t)0xFF9CDCFE,
    (int64_t)0xFFD7BA7D,
    (int64_t)0xFF90EE90,
};
static int64_t tk_color(int t) {
    if (t >= 0 && t < TK_COUNT) return g_tk_palette[t];
    return g_tk_palette[TK_IDENT];
}

CSSC_GUI_EXPORT void cssc_gui_screen_set_syntax_color(void* p, int64_t kind, int64_t argb) {
    (void)p;
    if (kind >= 0 && kind < TK_COUNT) g_tk_palette[(int)kind] = argb;
}

CSSC_GUI_EXPORT int64_t cssc_gui_screen_syntax_count(void* p) { (void)p; return (int64_t)TK_COUNT; }

CSSC_GUI_EXPORT int64_t cssc_gui_screen_syntax_color(void* p, int64_t kind) {
    (void)p; return (kind >= 0 && kind < TK_COUNT) ? g_tk_palette[(int)kind] : 0;
}

static void cssc_palette_path(char* buf, int n) {
    const char* ad = getenv("APPDATA");
    if (ad && ad[0]) {
        char dir[1024];
        snprintf(dir, sizeof(dir), "%s\\CSSC", ad);
#if defined(_WIN32)
        CreateDirectoryA(dir, NULL);
#endif
        snprintf(buf, (size_t)n, "%s\\syntax_palette.bin", dir);
    } else {
        snprintf(buf, (size_t)n, "syntax_palette.bin");
    }
}
CSSC_GUI_EXPORT void cssc_gui_screen_save_palette(void* p) {
    (void)p;
    char path[1200]; cssc_palette_path(path, (int)sizeof(path));
    FILE* f = fopen(path, "wb");
    if (f) { fwrite(g_tk_palette, sizeof(int64_t), (size_t)TK_COUNT, f); fclose(f); }
}
CSSC_GUI_EXPORT void cssc_gui_screen_load_palette(void* p) {
    (void)p;
    char path[1200]; cssc_palette_path(path, (int)sizeof(path));
    FILE* f = fopen(path, "rb");
    if (f) {
        int64_t tmp[TK_COUNT];
        size_t got = fread(tmp, sizeof(int64_t), (size_t)TK_COUNT, f);
        fclose(f);
        for (size_t i = 0; i < got && i < (size_t)TK_COUNT; i++) g_tk_palette[i] = tmp[i];
    }
}
static int tk_isident(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_';
}
static int tk_isidstart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static int tk_word_in(const char* s, int n, const char* const* set) {
    for (int i = 0; set[i]; i++)
        if ((int)strlen(set[i]) == n && !strncmp(s, set[i], (size_t)n)) return 1;
    return 0;
}
static int tk_is_kw(const char* s, int n) {
    static const char* const kw[] = {"if", "else", "while", "for", "return",
        "mirror", "break", "continue", "sector", "object", "true", "false",
        "null", "select", "jump", "jump_back", 0};
    return tk_word_in(s, n, kw);
}
static int tk_is_type(const char* s, int n) {
    static const char* const ty[] = {"int", "float", "bool", "string", "void",
        "auto", "vector", "array", "bind", "map", "char", "byte", "double",
        "long", "short", "uint", 0};
    return tk_word_in(s, n, ty);
}

static void ed_draw_line_hl(cssc_gui_editor* e, void* v, const char* line,
                            int64_t ly, int64_t text_x, int64_t glyph,
                            int max_cols) {
    int n = (int)strlen(line);
    int i = 0;
    char buf[512];
    int first_nonws = 0;
    while (first_nonws < n && (line[first_nonws] == ' ' || line[first_nonws] == '\t')) first_nonws++;
    while (i < n) {
        char c = line[i];
        int start = i;
        int t;
        if (c == ' ' || c == '\t') {
            while (i < n && (line[i] == ' ' || line[i] == '\t')) i++;
            t = TK_WS;
        } else if (c == '#') {
            i++; while (i < n && tk_isident(line[i])) i++;
            t = TK_DIR;
        } else if (c == '/' && i + 1 < n && line[i + 1] == '/') {
            i = n; t = TK_COMMENT;
        } else if (c == '"') {
            i++;
            while (i < n && line[i] != '"') { if (line[i] == '\\' && i + 1 < n) i++; i++; }
            if (i < n) i++;
            t = TK_STR;
        } else if (c >= '0' && c <= '9') {
            while (i < n && (tk_isident(line[i]) || line[i] == '.')) i++;
            t = TK_NUM;
        } else if (tk_isidstart(c)) {
            while (i < n && tk_isident(line[i])) i++;
            int len = i - start;
            const char* id = line + start;
            int j = i; while (j < n && line[j] == ' ') j++;
            int is_vis = (len == 7 && !strncmp(id, "private", 7)) ||
                         (len == 6 && !strncmp(id, "public", 6));
            int single_colon = (j < n && line[j] == ':' && (j + 1 >= n || line[j + 1] != ':'));
            if (len == 4 && !strncmp(id, "free", 4)) t = TK_FREE;
            else if (is_vis && single_colon) t = TK_VIS;
            else if (tk_is_kw(id, len)) t = TK_KW;
            else if (tk_is_type(id, len)) t = TK_TYPE;
            else if (j + 1 < n && line[j] == ':' && line[j + 1] == ':') t = TK_NS;
            else if (j < n && line[j] == '(') t = TK_FUNC;
            else if (single_colon && start == first_nonws) t = TK_LABEL;
            else t = TK_IDENT;
        } else if (c == '&') {
            i++; t = TK_COPY;
        } else if (c == '*') {
            i++; t = TK_REF;
        } else if (c == '%') {
            i++; t = TK_MUTIER;
        } else {
            i++; t = TK_PUNCT;
        }
        if (t == TK_WS) continue;
        int cs = start, ce = i;
        if (cs < e->left_col) cs = e->left_col;
        if (ce > e->left_col + max_cols) ce = e->left_col + max_cols;
        int cnt = ce - cs;
        if (cnt > 0 && cnt < 512) {
            memcpy(buf, line + cs, (size_t)cnt); buf[cnt] = 0;
            int64_t dx = text_x + (int64_t)(cs - e->left_col) * glyph;
            cssc_video_draw_text(v, dx, ly, buf, tk_color(t), e->scale);
        }
    }
}

static int ed_alloc_var(const char* line, char* out, int cap) {
    int p = 0; while (line[p] == ' ' || line[p] == '\t') p++;
    if (line[p] != '#') return 0;
    p++;
    const char* d = line + p;
    int isalloc = 0;
    if (!strncmp(d, "stack", 5)) { p += 5; isalloc = 1; }
    else if (!strncmp(d, "heap", 4)) { p += 4; isalloc = 1; }
    else if (!strncmp(d, "auto", 4)) { p += 4; isalloc = 1; }
    else if (!strncmp(d, "delete", 6)) { p += 6; isalloc = 0; }
    else return 0;
    if (isalloc) {
        while (line[p] && line[p] != '[') {
            if (line[p] != ' ' && line[p] != '\t') return 0;
            p++;
        }
        if (line[p] != '[') return 0;
        int depth = 0;
        while (line[p]) {
            if (line[p] == '[') depth++;
            else if (line[p] == ']') { depth--; if (depth == 0) { p++; break; } }
            p++;
        }
    } else {
        while (line[p] == ' ' || line[p] == '\t') p++;
        if (line[p] != '[') return 0;
        p++;
    }
    while (line[p] == ' ' || line[p] == '\t') p++;
    int n = 0;
    while (line[p] && tk_isident(line[p]) && n < cap - 1) out[n++] = line[p++];
    out[n] = 0;
    if (n == 0) return 0;
    return isalloc ? 1 : 2;
}

static void ed_draw_doc_line(void* v, int64_t x, int64_t y, const char* s,
                             int64_t glyph, int64_t scale) {
    int64_t color = (int64_t)0xFFAEB6C6;
    char run[256]; int rn = 0;
    int i = 0;
    while (s[i]) {
        if (s[i] == '\\' && s[i + 1] == 'c' && s[i + 2] >= '0' && s[i + 2] <= '3') {
            if (rn) { run[rn] = 0; cssc_video_draw_text(v, x, y, run, color, scale);
                      x += (int64_t)rn * glyph; rn = 0; }
            char q = s[i + 2];
            color = q == '0' ? (int64_t)0xFFE0704A : q == '1' ? (int64_t)0xFF5FE0A0 :
                    q == '2' ? (int64_t)0xFF7FB0FF : (int64_t)0xFFC74DE0;
            i += 3;
        } else if (s[i] == '\\' && s[i + 1] == 'r') {
            int j = i + 2, R = 0, G = 0, B = 0, ok = 0;
            while (s[j] >= '0' && s[j] <= '9') R = R * 10 + (s[j++] - '0');
            if (s[j] == 'g') { j++;
                while (s[j] >= '0' && s[j] <= '9') G = G * 10 + (s[j++] - '0');
                if (s[j] == 'b') { j++;
                    while (s[j] >= '0' && s[j] <= '9') B = B * 10 + (s[j++] - '0');
                    ok = 1; } }
            if (ok) {
                if (rn) { run[rn] = 0; cssc_video_draw_text(v, x, y, run, color, scale);
                          x += (int64_t)rn * glyph; rn = 0; }
                color = (R == 0 && G == 0 && B == 0) ? (int64_t)0xFFAEB6C6 :
                        (int64_t)0xFF000000 | ((int64_t)(R & 255) << 16) |
                        ((int64_t)(G & 255) << 8) | (int64_t)(B & 255);
                i = j;
            } else { if (rn < 255) run[rn++] = s[i]; i++; }
        } else {
            if (rn < 255) run[rn++] = s[i]; i++;
        }
    }
    if (rn) { run[rn] = 0; cssc_video_draw_text(v, x, y, run, color, scale); }
}

static int64_t ed_hover_color(const char* t) {
    if (!t) return -1;
    const char* p = t;
    while (*p && *p != 'r') { if (*p == '\n') { p++; continue; } p++; }
    if (!(p[0] == 'r' && p[1] == 'g' && p[2] == 'b' && p[3] == '(')) return -1;
    p += 4;
    int vals[3] = {0, 0, 0}, vi = 0, cur = 0, have = 0;
    while (*p && *p != '\n' && vi < 3) {
        if (*p >= '0' && *p <= '9') { cur = cur * 10 + (*p - '0'); have = 1; }
        else { if (have) { vals[vi++] = cur; cur = 0; have = 0; } if (*p == ')') break; }
        p++;
    }
    if (have && vi < 3) vals[vi++] = cur;
    if (vi < 3) return -1;
    return ((int64_t)(vals[0] & 255) << 16) | ((int64_t)(vals[1] & 255) << 8) | (vals[2] & 255);
}
CSSC_GUI_EXPORT void cssc_gui_editor_draw(void* ep) {
    cssc_gui_editor* e = (cssc_gui_editor*)ep;
    if (!e || !e->visible || !e->screen) return;
    if (e->nlines == 0) ed_insert_line(e, 0, "");
    void* v = e->screen->vid;
    cssc_video_fillrect(v, e->x, e->y, e->w, e->h, e->bg);
    cssc_video_draw_rect(v, e->x, e->y, e->w, e->h,
                         e->focused ? (int64_t)0xC03FD0A0 : (int64_t)0x50FFFFFF);
    int64_t glyph = 8 * e->scale;
    int64_t line_h = glyph + 4 * e->scale;
    int digits = 1, nn = e->nlines; while (nn >= 10) { nn /= 10; digits++; }
    int64_t gutter_w = e->gutter_on ? (int64_t)(digits + 1) * glyph + 8 : 0;
    int visible_rows = (int)((e->h - 8) / line_h);
    if (visible_rows < 1) visible_rows = 1;
    if (e->follow) {
        if (e->cur_line < e->top_line) e->top_line = e->cur_line;
        if (e->cur_line >= e->top_line + visible_rows) e->top_line = e->cur_line - visible_rows + 1;
        e->follow = 0;
    }
    if (e->top_line > e->nlines - 1) e->top_line = e->nlines - 1;
    if (e->top_line < 0) e->top_line = 0;
    if (e->gutter_on)
        cssc_video_fillrect(v, e->x + 1, e->y + 1, gutter_w, e->h - 2, (int64_t)0x30000000);
    int64_t text_x = e->x + gutter_w + 6;
    int max_cols = (int)((e->x + e->w - 4 - text_x) / glyph);
    if (max_cols < 1) max_cols = 1;
    if (max_cols > 511) max_cols = 511;

    if (e->cur_col < e->left_col) e->left_col = e->cur_col;
    if (e->cur_col >= e->left_col + max_cols) e->left_col = e->cur_col - max_cols + 1;
    if (e->left_col < 0) e->left_col = 0;
    int sl = 0, sc = 0, el = 0, ec = 0;
    if (e->sel_active) ed_sel_norm(e, &sl, &sc, &el, &ec);
    char active[128]; active[0] = 0;
    int active_kind = 0;

    int useMap = (e->own_na + e->own_nd) > 0;
    int cursorGid = -1;
    if (useMap) {
        int cl1 = e->cur_line + 1;
        for (int a = 0; a < e->own_na; a++)
            if (e->own_aline[a] == cl1) { cursorGid = cl1; break; }
        if (cursorGid < 0)
            for (int d = 0; d < e->own_nd; d++)
                if (e->own_dline[d] == cl1) { cursorGid = e->own_dtarget[d]; break; }
    } else if (e->language == 1 && e->cur_line >= 0 && e->cur_line < e->nlines) {
        active_kind = ed_alloc_var(e->lines[e->cur_line], active, (int)sizeof(active));
    }
    char sub[512];
    for (int r = 0; r < visible_rows; ++r) {
        int li = e->top_line + r;
        if (li >= e->nlines) break;
        int64_t ly = e->y + 5 + (int64_t)r * line_h;
        const char* line = e->lines[li];
        int slen = (int)strlen(line);
        int marked = 0, leaked = 0;
        if (useMap) {
            if (cursorGid >= 0) {
                int ln1 = li + 1;
                for (int a = 0; a < e->own_na; a++)
                    if (e->own_aline[a] == ln1 && ln1 == cursorGid) {
                        marked = 1; leaked = (e->own_afreed[a] == 0); break;
                    }
                if (marked == 0)
                    for (int d = 0; d < e->own_nd; d++)
                        if (e->own_dline[d] == ln1 && e->own_dtarget[d] == cursorGid) {
                            marked = 1; break;
                        }
            }
        } else if (active_kind && active[0]) {
            char lv[128];
            if (ed_alloc_var(line, lv, (int)sizeof(lv)) && !strcmp(lv, active))
                marked = 1;
        }
        int64_t washCol = leaked ? (int64_t)0x28E0704A : (int64_t)0x20C74DE0;
        int64_t gutCol  = leaked ? (int64_t)0xFFE0704A : (int64_t)0xFFC74DE0;
        if (marked)
            cssc_video_fillrect(v, e->x + gutter_w + 1, ly - 1,
                                e->w - gutter_w - 2, line_h, washCol);
        if (e->ip_line == li + 1) {
            cssc_video_fillrect(v, e->x + 1, ly - 1, e->w - 2, line_h, (int64_t)0x40FF3B54);
            cssc_video_fillrect(v, e->x + 1, ly - 1, 3 * e->scale, line_h, (int64_t)0xFFFF3B54);
        }
        if (e->gutter_on) {
            char num[16]; int nlen = ed_itoa(li + 1, num);
            int64_t nx = e->x + gutter_w - 6 - (int64_t)nlen * glyph;
            cssc_video_draw_text(v, nx, ly, num,
                                 marked ? gutCol : e->gutter, e->scale);
        }
        int ind = 0; while (line[ind] == ' ') ind++;
        for (int gcol = 0; gcol < ind; gcol += 4) {
            int vc = gcol - e->left_col;
            if (vc >= 0 && vc < max_cols)
                cssc_video_fillrect(v, text_x + (int64_t)vc * glyph, ly - 1,
                                    1, line_h, (int64_t)0x2CFFFFFF);
        }
        if (e->sel_active && li >= sl && li <= el) {
            int cstart = (li == sl) ? sc : 0;
            int cend = (li == el) ? ec : slen;
            int vstart = cstart - e->left_col; if (vstart < 0) vstart = 0;
            int vend = cend - e->left_col; if (vend > max_cols) vend = max_cols;
            if (vend > vstart)
                cssc_video_fillrect(v, text_x + (int64_t)vstart * glyph, ly - 1,
                                    (int64_t)(vend - vstart) * glyph, line_h,
                                    (int64_t)0x804D6AA8);
        }
        if (e->search && e->search_len > 0) {
            const char* hit = line;
            while ((hit = strstr(hit, e->search)) != NULL) {
                int mc = (int)(hit - line);
                int vstart = mc - e->left_col; if (vstart < 0) vstart = 0;
                int vend = (mc + e->search_len) - e->left_col;
                if (vend > max_cols) vend = max_cols;
                if (vend > vstart)
                    cssc_video_fillrect(v, text_x + (int64_t)vstart * glyph, ly - 1,
                                        (int64_t)(vend - vstart) * glyph, line_h,
                                        (int64_t)0x90C74DE0);
                hit += e->search_len;
            }
        }
        if (e->language == 1) {
            ed_draw_line_hl(e, v, line, ly, text_x, glyph, max_cols);
        } else {
            int from = e->left_col; if (from > slen) from = slen;
            int cnt = slen - from; if (cnt > max_cols) cnt = max_cols;
            if (cnt > 0) { memcpy(sub, line + from, (size_t)cnt); sub[cnt] = 0; }
            else sub[0] = 0;
            cssc_video_draw_text(v, text_x, ly, sub, e->fg, e->scale);
        }

        for (int d = 0; d < e->dg_n; d++) {
            if (e->dg_line[d] != li + 1) continue;
            int dc = e->dg_col[d]; if (dc < 0) dc = 0;
            int a2 = dc, b2 = dc;
            if (dc < slen && ed_isword(line[dc])) {
                while (a2 > 0 && ed_isword(line[a2 - 1])) a2--;
                while (b2 < slen && ed_isword(line[b2])) b2++;
            } else {
                if (a2 > slen) a2 = slen;
                b2 = slen; if (b2 <= a2) b2 = a2 + 1;
            }
            int vs = a2 - e->left_col; if (vs < 0) vs = 0;
            int ve = b2 - e->left_col; if (ve > max_cols) ve = max_cols;
            if (ve <= vs) continue;
            int64_t sx = text_x + (int64_t)vs * glyph;
            int64_t sw = (int64_t)(ve - vs) * glyph;
            int64_t sy = ly + glyph + 1;
            int64_t scol = e->dg_sev[d] == 2 ? (int64_t)0xFFF0C14B :
                           e->dg_sev[d] == 3 ? (int64_t)0xFF7FB0FF :
                                               (int64_t)0xFFE0704A;
            for (int64_t x = sx; x < sx + sw; x += 2) {
                int64_t yy = sy + ((((x - sx) / 2) % 2) == 0 ? 0 : 2);
                cssc_video_fillrect(v, x, yy, 2, 1, scol);
            }
        }

        if (e->sd_on) {
            for (int sd = 0; sd < e->sd_n; sd++) {
                if (e->sd_line[sd] != li + 1) continue;
                int64_t bar = e->sd_sev[sd] == 2 ? (int64_t)0xFFF0C14B :
                              e->sd_sev[sd] == 3 ? (int64_t)0xFF7FB0FF :
                                                   (int64_t)0xFFE0704A;
                int64_t tnt = e->sd_sev[sd] == 2 ? (int64_t)0x22F0C14B :
                              e->sd_sev[sd] == 3 ? (int64_t)0x227FB0FF :
                                                   (int64_t)0x22E0704A;
                cssc_video_fillrect(v, e->x + gutter_w + 1, ly - 1,
                                    e->w - gutter_w - 2, line_h, tnt);
                cssc_video_fillrect(v, e->x + 1, ly - 1, 3, line_h, bar);
                break;
            }
        }
    }
    if (e->focused && e->cur_line >= e->top_line &&
        e->cur_line < e->top_line + visible_rows) {
        int64_t cy = e->y + 5 + (int64_t)(e->cur_line - e->top_line) * line_h;
        int64_t cx = text_x + (int64_t)(e->cur_col - e->left_col) * glyph;
        cssc_video_fillrect(v, cx, cy, 2, glyph, e->cursor_c);
    }

    if (e->focused && e->cmp_open && e->cmp_nf > 0 &&
        e->cmp_line >= e->top_line && e->cmp_line < e->top_line + visible_rows) {
        int maxrows = 8;
        int rows = e->cmp_nf < maxrows ? e->cmp_nf : maxrows;
        if (e->cmp_sel < e->cmp_top) e->cmp_top = e->cmp_sel;
        if (e->cmp_sel >= e->cmp_top + maxrows) e->cmp_top = e->cmp_sel - maxrows + 1;
        if (e->cmp_top < 0) e->cmp_top = 0;
        int maxlen = 4;
        for (int i = 0; i < e->cmp_nf; i++) {
            int L = (int)strlen(e->cmp_items[e->cmp_filt[i]]);
            if (L > maxlen) maxlen = L;
        }
        int64_t rowh = glyph + 6;
        int64_t pw = (int64_t)maxlen * glyph + 18;
        int64_t ph = (int64_t)rows * rowh + 8;
        int64_t caretRow = e->cmp_line - e->top_line;
        int64_t ax = text_x + (int64_t)(e->cmp_start - e->left_col) * glyph;
        int64_t ay = e->y + 5 + caretRow * line_h + line_h + 2;
        if (ax + pw > e->x + e->w - 4) ax = e->x + e->w - 4 - pw;
        if (ax < e->x + 2) ax = e->x + 2;
        if (ay + ph > e->y + e->h - 2)
            ay = e->y + 5 + caretRow * line_h - ph - 2;
        if (ay < e->y + 2) ay = e->y + 2;
        cssc_video_fillrect(v, ax, ay, pw, ph, (int64_t)0xF01A1424);
        cssc_video_draw_rect(v, ax, ay, pw, ph, (int64_t)0xC03FD0A0);
        for (int r = 0; r < rows; r++) {
            int fi = e->cmp_top + r;
            if (fi >= e->cmp_nf) break;
            int64_t ry = ay + 4 + (int64_t)r * rowh;
            if (fi == e->cmp_sel)
                cssc_video_fillrect(v, ax + 2, ry - 1, pw - 4, rowh, (int64_t)0x603FD0A0);
            const char* lab = e->cmp_items[e->cmp_filt[fi]];
            cssc_video_draw_text(v, ax + 8, ry + 1, lab,
                                 fi == e->cmp_sel ? (int64_t)0xFFFFFFFF
                                                  : (int64_t)0xFFD8DEE9, e->scale);
        }
        if (e->cmp_nf > maxrows) {
            int64_t track = ph - 8;
            int64_t thumb = track * maxrows / e->cmp_nf; if (thumb < 8) thumb = 8;
            int64_t ty = ay + 4 + (track - thumb) * e->cmp_top / (e->cmp_nf - maxrows);
            cssc_video_fillrect(v, ax + pw - 5, ty, 3, thumb, (int64_t)0x803FD0A0);
        }
    }

    if (e->hov_open && e->hov_text &&
        e->hov_line >= e->top_line && e->hov_line < e->top_line + visible_rows) {
        int nln = 1, maxc = 1, cw = 0;
        for (const char* q = e->hov_text; *q; q++) {
            if (*q == '\n') { if (cw > maxc) maxc = cw; cw = 0; nln++; }
            else cw++;
        }
        if (cw > maxc) maxc = cw;
        if (maxc > 96) maxc = 96;
        int64_t rowh = glyph + 4;
        int64_t swc = ed_hover_color(e->hov_text);
        int64_t sww = (swc >= 0) ? (glyph + 8) : 0;
        int64_t bw = (int64_t)maxc * glyph + 16 + sww;
        int64_t bh = (int64_t)nln * rowh + 8;
        int64_t hrow = e->hov_line - e->top_line;
        int64_t hx = text_x + (int64_t)(e->hov_ws - e->left_col) * glyph;
        int64_t hy = e->y + 5 + hrow * line_h + line_h + 2;
        if (hx + bw > e->x + e->w - 4) hx = e->x + e->w - 4 - bw;
        if (hx < e->x + 2) hx = e->x + 2;
        if (hy + bh > e->y + e->h - 2) hy = e->y + 5 + hrow * line_h - bh - 2;
        if (hy < e->y + 2) hy = e->y + 2;
        cssc_video_fillrect(v, hx, hy, bw, bh, (int64_t)0xF0141A28);
        cssc_video_draw_rect(v, hx, hy, bw, bh, (int64_t)0xC0C74DE0);
        if (swc >= 0) {
            int64_t ss = glyph + 2;
            int64_t sxp = hx + bw - ss - 5, syp = hy + 5;
            cssc_video_fillrect(v, sxp, syp, ss, ss, (int64_t)0xFF000000 | swc);
            cssc_video_draw_rect(v, sxp, syp, ss, ss, (int64_t)0xFFFFFFFF);
        }
        char lbuf[512];
        const char* q = e->hov_text;
        int row = 0;
        while (*q) {
            int li = 0;
            while (*q && *q != '\n' && li < 511) lbuf[li++] = *q++;
            lbuf[li] = 0;
            if (*q == '\n') q++;
            int64_t ry = hy + 4 + (int64_t)row * rowh;
            if (row == 0)
                cssc_video_draw_text(v, hx + 8, ry + 1, lbuf, (int64_t)0xFF7FE0C0, e->scale);
            else
                ed_draw_doc_line(v, hx + 8, ry + 1, lbuf, glyph, e->scale);
            row++;
        }
    }

    if (e->focused && e->sig_open && e->sig_text &&
        e->sig_line >= e->top_line && e->sig_line < e->top_line + visible_rows) {
        int shown = (int)strlen(e->sig_text); if (shown > 96) shown = 96;
        int64_t bw = (int64_t)shown * glyph + 16;
        int64_t bh = glyph + 12;
        int64_t srow = e->sig_line - e->top_line;
        int64_t sxp = text_x + (int64_t)(e->sig_open_col - e->left_col) * glyph;
        int64_t syp = e->y + 5 + srow * line_h - bh - 2;
        if (sxp + bw > e->x + e->w - 4) sxp = e->x + e->w - 4 - bw;
        if (sxp < e->x + 2) sxp = e->x + 2;
        if (syp < e->y + 2) syp = e->y + 5 + srow * line_h + line_h + 2;
        cssc_video_fillrect(v, sxp, syp, bw, bh, (int64_t)0xF0101A26);
        cssc_video_draw_rect(v, sxp, syp, bw, bh, (int64_t)0xC07FB0FF);
        cssc_video_draw_text(v, sxp + 8, syp + 6, e->sig_text, (int64_t)0xFFC8D2E0, e->scale);
        const char* st = e->sig_text;
        int paren = -1;
        for (int i = 0; st[i]; i++) if (st[i] == '(') { paren = i; break; }
        if (paren >= 0) {
            int seg = 0, segStart = paren + 1, d3 = 0, aStart = -1, aEnd = -1;
            for (int i = paren + 1; st[i]; i++) {
                char c = st[i];
                if (c == '(' || c == '[') d3++;
                else if (c == ']') { if (d3 > 0) d3--; }
                else if (c == ')') {
                    if (d3 == 0) { if (seg == e->sig_arg) { aStart = segStart; aEnd = i; } break; }
                    d3--;
                } else if (c == ',' && d3 == 0) {
                    if (seg == e->sig_arg) { aStart = segStart; aEnd = i; }
                    seg++; segStart = i + 1;
                }
            }
            if (aStart >= 0 && aEnd > aStart) {
                while (aStart < aEnd && st[aStart] == ' ') aStart++;
                char segbuf[128];
                int sl = aEnd - aStart; if (sl > 127) sl = 127; if (sl < 0) sl = 0;
                memcpy(segbuf, st + aStart, (size_t)sl); segbuf[sl] = 0;
                cssc_video_draw_text(v, sxp + 8 + (int64_t)aStart * glyph, syp + 6,
                                     segbuf, (int64_t)0xFF7FE0C0, e->scale);
            }
        }
    }

    if (e->sd_on && e->screen) {
        int64_t mx = e->screen->mx, my = e->screen->my;
        if (mx >= e->x && mx < e->x + e->w && my >= e->y + 5 && my < e->y + e->h) {
            int hrow = (int)((my - (e->y + 5)) / line_h);
            int hli = e->top_line + hrow;
            const char* hmsg = NULL; int hsev = 1;
            for (int sd = 0; sd < e->sd_n; sd++)
                if (e->sd_line[sd] == hli + 1) { hmsg = e->sd_msg[sd]; hsev = e->sd_sev[sd]; break; }
            if (hmsg && hmsg[0]) {
                int mlen = (int)strlen(hmsg);
                int maxfit = (int)((e->w - 24) / glyph);
                if (maxfit < 8) maxfit = 8; if (maxfit > 150) maxfit = 150;
                int shown = mlen < maxfit ? mlen : maxfit;
                int64_t bw = (int64_t)shown * glyph + 16;
                int64_t bh = glyph + 12;
                int64_t hx = mx + 12, hy = my + 16;
                if (hx + bw > e->x + e->w - 4) hx = e->x + e->w - 4 - bw;
                if (hx < e->x + 2) hx = e->x + 2;
                if (hy + bh > e->y + e->h - 2) hy = my - bh - 4;
                if (hy < e->y + 2) hy = e->y + 2;
                int64_t rim = hsev == 2 ? (int64_t)0xC0F0C14B :
                              hsev == 3 ? (int64_t)0xC07FB0FF : (int64_t)0xC0E0704A;
                cssc_video_fillrect(v, hx, hy, bw, bh, (int64_t)0xF0141A28);
                cssc_video_draw_rect(v, hx, hy, bw, bh, rim);
                char hb[160];
                int n = shown; if (n > 159) n = 159;
                memcpy(hb, hmsg, (size_t)n);
                if (shown < mlen && n >= 3) { hb[n-1] = '.'; hb[n-2] = '.'; hb[n-3] = '.'; }
                hb[n] = 0;
                cssc_video_draw_text(v, hx + 8, hy + 6, hb, (int64_t)0xFFE6E2F0, e->scale);
            }
        }
    }
}

static int64_t list_icon_color(const char* s) {
    const char* dot = strrchr(s, '.');
    if (!dot) return (int64_t)0xFF6E7681;
    if (!strcmp(dot, ".cssc")) return (int64_t)0xFF3FD0A0;
    if (!strcmp(dot, ".md"))   return (int64_t)0xFF7FB0FF;
    if (!strcmp(dot, ".py"))   return (int64_t)0xFFF0C14B;
    if (!strcmp(dot, ".json") || !strcmp(dot, ".toml") || !strcmp(dot, ".hsim") ||
        !strcmp(dot, ".cproject"))
        return (int64_t)0xFFE0894B;
    if (!strcmp(dot, ".png") || !strcmp(dot, ".mp3") || !strcmp(dot, ".wav") ||
        !strcmp(dot, ".ogg") || !strcmp(dot, ".jpg") || !strcmp(dot, ".gif"))
        return (int64_t)0xFFC08BFF;
    if (!strcmp(dot, ".c") || !strcmp(dot, ".h") || !strcmp(dot, ".cpp") ||
        !strcmp(dot, ".hpp"))
        return (int64_t)0xFF88C0D0;
    return (int64_t)0xFFB8C0CC;
}
static void list_grow(cssc_gui_list* l, int need) {
    if (need <= l->cap) return;
    int nc = l->cap ? l->cap : 16;
    while (nc < need) nc *= 2;
    char** ni = (char**)realloc(l->items, (size_t)nc * sizeof(char*));
    int*   nd = (int*)realloc(l->depth, (size_t)nc * sizeof(int));
    if (ni) l->items = ni;
    if (nd) l->depth = nd;
    if (ni && nd) l->cap = nc;
}
CSSC_GUI_EXPORT void* cssc_gui_list_new(void* screen) {
    cssc_gui_list* l = (cssc_gui_list*)calloc(1, sizeof(cssc_gui_list));
    if (!l) return NULL;
    l->kind = GW_LIST;
    l->screen = (cssc_gui_screen*)screen;
    l->x = 20; l->y = 20; l->w = 240; l->h = 360;
    l->selected = 0; l->top = 0;
    l->scale = 2;
    l->bg = (int64_t)0xFF0C1119;
    l->fg = (int64_t)0xFFB8C0CC;
    l->sel_bg = (int64_t)0x603FD0A0;
    l->sel_fg = (int64_t)0xFFFFFFFF;
    l->focused = 0; l->visible = 1;
    return l;
}
CSSC_GUI_EXPORT void cssc_gui_list_setrect(void* p, int64_t x, int64_t y,
                                           int64_t w, int64_t h) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (l) { l->x = x; l->y = y; l->w = w; l->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_list_add(void* p, const char* s) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (!l) return;
    list_grow(l, l->nitems + 1);
    if (l->cap < l->nitems + 1) return;
    l->items[l->nitems] = gui_strdup(s ? s : "");
    l->depth[l->nitems] = 0;
    l->nitems++;
}
CSSC_GUI_EXPORT void cssc_gui_list_addat(void* p, const char* s, int64_t depth) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (!l) return;
    list_grow(l, l->nitems + 1);
    if (l->cap < l->nitems + 1) return;
    l->items[l->nitems] = gui_strdup(s ? s : "");
    l->depth[l->nitems] = (int)(depth > 0 ? depth : 0);
    l->nitems++;
}
CSSC_GUI_EXPORT void cssc_gui_list_clear(void* p) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (!l) return;
    for (int i = 0; i < l->nitems; ++i) free(l->items[i]);
    l->nitems = 0; l->selected = 0; l->top = 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_list_count(void* p) {
    cssc_gui_list* l = (cssc_gui_list*)p; return l ? (int64_t)l->nitems : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_list_selected(void* p) {
    cssc_gui_list* l = (cssc_gui_list*)p;
    return (l && l->nitems) ? (int64_t)(l->selected + 1) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_list_rightclicked(void* p) {
    cssc_gui_list* l = (cssc_gui_list*)p; return l ? (int64_t)l->right_hit : 0;
}
CSSC_GUI_EXPORT void* cssc_gui_list_selectedtext(void* p) {
    cssc_gui_list* l = (cssc_gui_list*)p;
    if (!l || l->nitems == 0 || l->selected < 0 || l->selected >= l->nitems)
        return cssc_string_lit("", 0);
    const char* s = l->items[l->selected];
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT void cssc_gui_list_setselected(void* p, int64_t i) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (!l) return;
    int idx = (int)i - 1;
    if (idx < 0) idx = 0;
    if (idx >= l->nitems) idx = l->nitems - 1;
    l->selected = idx;
}
CSSC_GUI_EXPORT void cssc_gui_list_setfocus(void* p, int64_t f) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (l) l->focused = f ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_list_setscale(void* p, int64_t s) {
    cssc_gui_list* l = (cssc_gui_list*)p; if (l) l->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_list_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_list* l = (cssc_gui_list*)p;
    if (l) { l->bg = bg & 0xFFFFFFFF; l->fg = fg & 0xFFFFFFFF; }
}

CSSC_GUI_EXPORT int64_t cssc_gui_list_update(void* lp, void* sp) {
    cssc_gui_list* l = (cssc_gui_list*)lp;
    if (!l) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : l->screen;
    if (!s) return 0;
    int64_t glyph = 8 * l->scale;
    int64_t row_h = glyph + 4;
    int visible_rows = (int)((l->h - 4) / row_h);
    if (visible_rows < 1) visible_rows = 1;
    int64_t activated = 0;
    l->right_hit = 0;
    int inside = (!s->input_captured && s->mx >= l->x && s->mx < l->x + l->w &&
                  s->my >= l->y && s->my < l->y + l->h);
    if (s->down && !s->prev_down && inside) {
        int row = (int)((s->my - l->y - 2) / row_h);
        int idx = l->top + row;
        if (idx >= 0 && idx < l->nitems) { l->selected = idx; activated = idx + 1; }
    }
    if (s->rdown && !s->prev_rdown && inside) {
        int row = (int)((s->my - l->y - 2) / row_h);
        int idx = l->top + row;
        if (idx >= 0 && idx < l->nitems) { l->selected = idx; l->right_hit = idx + 1; }
    }
    if (l->focused) {
        int64_t k;
        while ((k = cssc_video_poll_key(s->vid)) != 0) {
            if (k == 0x26 && l->selected > 0) l->selected--;
            else if (k == 0x28 && l->selected < l->nitems - 1) l->selected++;
        }
        int64_t c;
        while ((c = cssc_video_poll_char(s->vid)) != 0) {
            if (c == 13) activated = l->selected + 1;
        }
        int64_t wh = cssc_video_wheel(s->vid);
        if (wh) { l->top -= (int)wh * 3; if (l->top < 0) l->top = 0; }
    }
    if (l->selected < l->top) l->top = l->selected;
    if (l->selected >= l->top + visible_rows) l->top = l->selected - visible_rows + 1;
    if (l->top < 0) l->top = 0;
    return activated;
}

CSSC_GUI_EXPORT void cssc_gui_list_draw(void* lp) {
    cssc_gui_list* l = (cssc_gui_list*)lp;
    if (!l || !l->visible || !l->screen) return;
    void* v = l->screen->vid;
    cssc_video_fillrect(v, l->x, l->y, l->w, l->h, l->bg);
    cssc_video_draw_rect(v, l->x, l->y, l->w, l->h,
                         l->focused ? (int64_t)0xC03FD0A0 : (int64_t)0x50FFFFFF);
    int64_t glyph = 8 * l->scale;
    int64_t row_h = glyph + 4;
    int visible_rows = (int)((l->h - 4) / row_h);
    if (visible_rows < 1) visible_rows = 1;
    for (int r = 0; r < visible_rows; ++r) {
        int idx = l->top + r;
        if (idx >= l->nitems) break;
        int64_t ry = l->y + 2 + (int64_t)r * row_h;
        if (idx == l->selected)
            cssc_video_fillrect(v, l->x + 1, ry, l->w - 2, row_h, l->sel_bg);
        int64_t tx = l->x + 8 + (int64_t)l->depth[idx] * glyph;
        int64_t isz = glyph / 2; if (isz < 4) isz = 4;
        cssc_video_fillrect(v, tx, ry + (row_h - isz) / 2, isz, isz,
                            list_icon_color(l->items[idx]));
        const char* disp = l->items[idx];
        const char* bs = strrchr(disp, '/');
        if (bs) disp = bs + 1;
        int64_t col = (idx == l->selected) ? l->sel_fg : l->fg;
        cssc_video_draw_text(v, tx + glyph + 2, ry + 2, disp, col, l->scale);
    }
}

static int tree_namecmp(const void* a, const void* b) {
    const unsigned char* x = *(const unsigned char* const*)a;
    const unsigned char* y = *(const unsigned char* const*)b;
    while (*x && *y) {
        unsigned char cx = *x, cy = *y;
        if (cx >= 'A' && cx <= 'Z') cx += 32;
        if (cy >= 'A' && cy <= 'Z') cy += 32;
        if (cx != cy) return (int)cx - (int)cy;
        x++; y++;
    }
    return (int)*x - (int)*y;
}
static int tree_skip_name(const char* n) {
    static const char* sk[] = {"build", "dist", "node_modules", "obj", "bin",
                               "__pycache__", 0};
    if (n[0] == '.') return 1;
    for (int i = 0; sk[i]; i++) if (!strcmp(n, sk[i])) return 1;
    return 0;
}
static int tree_is_open(cssc_gui_tree* t, const char* path) {
    for (int i = 0; i < t->fold_n; i++)
        if (!strcmp(t->fold_paths[i], path)) return t->fold_open[i];
    return 0;
}
static void tree_set_open(cssc_gui_tree* t, const char* path, int open) {
    for (int i = 0; i < t->fold_n; i++)
        if (!strcmp(t->fold_paths[i], path)) { t->fold_open[i] = open; return; }
    if (t->fold_n >= t->fold_cap) {
        int nc = t->fold_cap ? t->fold_cap * 2 : 32;
        char** np = (char**)realloc(t->fold_paths, (size_t)nc * sizeof(char*));
        int*   no = (int*)realloc(t->fold_open, (size_t)nc * sizeof(int));
        if (np) t->fold_paths = np;
        if (no) t->fold_open = no;
        if (np && no) t->fold_cap = nc; else return;
    }
    t->fold_paths[t->fold_n] = gui_strdup(path);
    t->fold_open[t->fold_n] = open;
    t->fold_n++;
}
static void tree_rows_grow(cssc_gui_tree* t, int need) {
    if (need <= t->cap) return;
    int nc = t->cap ? t->cap : 32;
    while (nc < need) nc *= 2;
    char** np = (char**)realloc(t->paths, (size_t)nc * sizeof(char*));
    char** nn = (char**)realloc(t->names, (size_t)nc * sizeof(char*));
    int*   nd = (int*)realloc(t->depths, (size_t)nc * sizeof(int));
    int*   ni = (int*)realloc(t->isdir, (size_t)nc * sizeof(int));
    if (np) t->paths = np;
    if (nn) t->names = nn;
    if (nd) t->depths = nd;
    if (ni) t->isdir = ni;
    if (np && nn && nd && ni) t->cap = nc;
}
static void tree_emit(cssc_gui_tree* t, const char* full, const char* name,
                      int depth, int isdir) {
    tree_rows_grow(t, t->nrows + 1);
    if (t->cap < t->nrows + 1) return;
    t->paths[t->nrows] = gui_strdup(full);
    t->names[t->nrows] = gui_strdup(name);
    t->depths[t->nrows] = depth;
    t->isdir[t->nrows] = isdir;
    t->nrows++;
}
static void tree_push(char*** arr, int* n, int* cap, const char* s) {
    if (*n >= *cap) {
        int nc = *cap ? *cap * 2 : 16;
        char** na = (char**)realloc(*arr, (size_t)nc * sizeof(char*));
        if (!na) return;
        *arr = na; *cap = nc;
    }
    (*arr)[(*n)++] = gui_strdup(s);
}
static void tree_walk(cssc_gui_tree* t, const char* dir, int depth) {
    if (t->nrows >= 4000 || depth > 40) return;
    char** dn = NULL; int nd = 0, cd = 0;
    char** fn = NULL; int nf = 0, cf = 0;
#ifdef _WIN32
    char pat[560]; snprintf(pat, sizeof(pat), "%s\\*", dir);
    WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            const char* n = fd.cFileName;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            if (tree_skip_name(n)) continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                tree_push(&dn, &nd, &cd, n);
            else
                tree_push(&fn, &nf, &cf, n);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(dir);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            const char* n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            if (tree_skip_name(n)) continue;
            char full[560]; snprintf(full, sizeof(full), "%s/%s", dir, n);
            struct stat st;
            int isd = (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) ? 1 : 0;
            if (isd) tree_push(&dn, &nd, &cd, n);
            else     tree_push(&fn, &nf, &cf, n);
        }
        closedir(d);
    }
#endif
    if (nd > 1) qsort(dn, nd, sizeof(char*), tree_namecmp);
    if (nf > 1) qsort(fn, nf, sizeof(char*), tree_namecmp);
    for (int i = 0; i < nd; i++) {
        char full[560]; snprintf(full, sizeof(full), "%s/%s", dir, dn[i]);
        tree_emit(t, full, dn[i], depth, 1);
        if (tree_is_open(t, full)) tree_walk(t, full, depth + 1);
        free(dn[i]);
    }
    for (int i = 0; i < nf; i++) {
        char full[560]; snprintf(full, sizeof(full), "%s/%s", dir, fn[i]);
        tree_emit(t, full, fn[i], depth, 0);
        free(fn[i]);
    }
    free(dn); free(fn);
}
static void tree_clear_rows(cssc_gui_tree* t) {
    for (int i = 0; i < t->nrows; i++) { free(t->paths[i]); free(t->names[i]); }
    t->nrows = 0;
}
static void tree_rebuild(cssc_gui_tree* t) {
    tree_clear_rows(t);
    if (t->root[0]) tree_walk(t, t->root, 0);
    if (t->selected >= t->nrows) t->selected = t->nrows ? t->nrows - 1 : 0;
    if (t->selected < 0) t->selected = 0;
}
static void tree_chevron(void* v, int64_t x, int64_t y, int64_t sz, int open,
                         int64_t col) {
    if (sz < 4) sz = 4;
    if (open)   for (int i = 0; i < sz / 2; i++)
                    cssc_video_fillrect(v, x + i, y + i, sz - 2 * i, 1, col);
    else        for (int i = 0; i < sz / 2; i++)
                    cssc_video_fillrect(v, x + i, y + i, 1, sz - 2 * i, col);
}

CSSC_GUI_EXPORT void* cssc_gui_tree_new(void* screen) {
    cssc_gui_tree* t = (cssc_gui_tree*)calloc(1, sizeof(cssc_gui_tree));
    if (!t) return NULL;
    t->kind = GW_TREE;
    t->screen = (cssc_gui_screen*)screen;
    t->x = 20; t->y = 20; t->w = 280; t->h = 400;
    t->scale = 2;
    t->bg = (int64_t)0xFF141019;
    t->fg = (int64_t)0xFFB8C0CC;
    t->sel_bg = (int64_t)0x60C74DE0;
    t->sel_fg = (int64_t)0xFFFFFFFF;
    t->visible = 1;
    return t;
}
CSSC_GUI_EXPORT void cssc_gui_tree_setrect(void* p, int64_t x, int64_t y,
                                           int64_t w, int64_t h) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (t) { t->x = x; t->y = y; t->w = w; t->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_tree_setroot(void* p, const char* dir) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (!t) return;
    const char* d = dir ? dir : "";
    size_t n = strlen(d);
    if (n >= sizeof(t->root)) n = sizeof(t->root) - 1;
    memcpy(t->root, d, n); t->root[n] = 0;
    t->selected = 0; t->top = 0;
    tree_rebuild(t);
}
CSSC_GUI_EXPORT void cssc_gui_tree_refresh(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (t) tree_rebuild(t);
}
CSSC_GUI_EXPORT void cssc_gui_tree_seticondir(void* p, const char* dir) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (!t) return;
    const char* d = dir ? dir : "";
    size_t n = strlen(d);
    if (n >= sizeof(t->icondir)) n = sizeof(t->icondir) - 1;
    memcpy(t->icondir, d, n); t->icondir[n] = 0;
    t->ico_loaded = 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tree_count(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; return t ? (int64_t)t->nrows : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tree_selected(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    return (t && t->nrows) ? (int64_t)(t->selected + 1) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tree_rightclicked(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; return t ? (int64_t)t->right_hit : 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_tree_dropready(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; return t ? (int64_t)t->drop_ready : 0;
}
CSSC_GUI_EXPORT void* cssc_gui_tree_dropsrc(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    return cssc_string_lit(t ? t->drop_src : "", t ? strlen(t->drop_src) : 0);
}
CSSC_GUI_EXPORT void* cssc_gui_tree_dropdst(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    return cssc_string_lit(t ? t->drop_dst : "", t ? strlen(t->drop_dst) : 0);
}
CSSC_GUI_EXPORT int64_t cssc_gui_tree_selectedisdir(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    if (!t || t->nrows == 0 || t->selected < 0 || t->selected >= t->nrows) return 0;
    return (int64_t)t->isdir[t->selected];
}
CSSC_GUI_EXPORT void* cssc_gui_tree_selectedpath(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    if (!t || t->nrows == 0 || t->selected < 0 || t->selected >= t->nrows)
        return cssc_string_lit("", 0);
    const char* s = t->paths[t->selected];
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT void* cssc_gui_tree_selectedname(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    if (!t || t->nrows == 0 || t->selected < 0 || t->selected >= t->nrows)
        return cssc_string_lit("", 0);
    const char* s = t->names[t->selected];
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT void cssc_gui_tree_setfocus(void* p, int64_t f) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (t) t->focused = f ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_tree_setscale(void* p, int64_t s) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (t) t->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_tree_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    if (t) { t->bg = bg & 0xFFFFFFFF; t->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT void cssc_gui_tree_setvisible(void* p, int64_t vis) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; if (t) t->visible = vis ? 1 : 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_tree_update(void* tp, void* sp) {
    cssc_gui_tree* t = (cssc_gui_tree*)tp;
    if (!t) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : t->screen;
    if (!s || !t->visible) return 0;
    int64_t glyph = 8 * t->scale;
    int64_t row_h = glyph + 4;
    int visible_rows = (int)((t->h - 4) / row_h);
    if (visible_rows < 1) visible_rows = 1;
    int64_t activated = 0;
    t->right_hit = 0;
    int inside = (!s->input_captured && s->mx >= t->x && s->mx < t->x + t->w &&
                  s->my >= t->y && s->my < t->y + t->h);
    if (t->drop_ready) t->drop_ready = 0;
    if (s->down && !s->prev_down && inside) {
        int row = (int)((s->my - t->y - 2) / row_h);
        int idx = t->top + row;
        if (idx >= 0 && idx < t->nrows) {
            t->selected = idx;
            t->drag_on = 1; t->drag_idx = idx; t->drag_active = 0;
            t->drag_dx = (int)s->mx; t->drag_dy = (int)s->my;
        }
    }
    if (s->down && t->drag_on) {
        int ax = (int)s->mx - t->drag_dx; if (ax < 0) ax = -ax;
        int ay = (int)s->my - t->drag_dy; if (ay < 0) ay = -ay;
        if (ax > 5 || ay > 5) t->drag_active = 1;
    }
    if (!s->down && s->prev_down && t->drag_on) {
        if (t->drag_active) {
            if (inside) {
                int row = (int)((s->my - t->y - 2) / row_h);
                int tidx = t->top + row;
                if (tidx >= 0 && tidx < t->nrows && tidx != t->drag_idx &&
                    t->isdir[tidx] && t->drag_idx < t->nrows) {
                    const char* src = t->paths[t->drag_idx];
                    const char* dst = t->paths[tidx];
                    size_t sl = strlen(src);

                    if (!(strncmp(dst, src, sl) == 0 && (dst[sl] == '/' || dst[sl] == '\\' || dst[sl] == 0))) {
                        snprintf(t->drop_src, sizeof(t->drop_src), "%s", src);
                        snprintf(t->drop_dst, sizeof(t->drop_dst), "%s", dst);
                        t->drop_ready = 1;
                    }
                }
            }
        } else {
            int idx = t->drag_idx;
            if (idx >= 0 && idx < t->nrows) {
                if (t->isdir[idx]) {
                    tree_set_open(t, t->paths[idx], !tree_is_open(t, t->paths[idx]));
                    tree_rebuild(t);
                } else {
                    activated = idx + 1;
                }
            }
        }
        t->drag_on = 0; t->drag_active = 0;
    }
    if (s->rdown && !s->prev_rdown && inside) {
        int row = (int)((s->my - t->y - 2) / row_h);
        int idx = t->top + row;
        if (idx >= 0 && idx < t->nrows) { t->selected = idx; t->right_hit = idx + 1; }
    }
    if (t->focused) {
        int64_t k;
        while ((k = cssc_video_poll_key(s->vid)) != 0) {
            if (k == 0x26 && t->selected > 0) t->selected--;
            else if (k == 0x28 && t->selected < t->nrows - 1) t->selected++;
            else if (k == 0x25) {
                if (t->selected < t->nrows && t->isdir[t->selected] &&
                    tree_is_open(t, t->paths[t->selected])) {
                    tree_set_open(t, t->paths[t->selected], 0); tree_rebuild(t);
                }
            } else if (k == 0x27) {
                if (t->selected < t->nrows && t->isdir[t->selected] &&
                    !tree_is_open(t, t->paths[t->selected])) {
                    tree_set_open(t, t->paths[t->selected], 1); tree_rebuild(t);
                }
            }
        }
        int64_t c;
        while ((c = cssc_video_poll_char(s->vid)) != 0) {
            if (c == 13 && t->selected >= 0 && t->selected < t->nrows) {
                if (t->isdir[t->selected]) {
                    tree_set_open(t, t->paths[t->selected],
                                  !tree_is_open(t, t->paths[t->selected]));
                    tree_rebuild(t);
                } else {
                    activated = t->selected + 1;
                }
            }
        }
        int64_t wh = cssc_video_wheel(s->vid);
        if (wh) { t->top -= (int)wh * 3; if (t->top < 0) t->top = 0; }
    }
    if (t->nrows == 0) { t->selected = 0; t->top = 0; }
    else {
        if (t->selected < 0) t->selected = 0;
        if (t->selected >= t->nrows) t->selected = t->nrows - 1;
        if (t->selected < t->top) t->top = t->selected;
        if (t->selected >= t->top + visible_rows) t->top = t->selected - visible_rows + 1;
        if (t->top < 0) t->top = 0;
    }
    return activated;
}

static void* tree_load_icon(const char* dir, const char* name) {
    char path[600]; snprintf(path, sizeof(path), "%s/%s", dir, name);
    return cssc_sprite_load(path);
}
static void tree_ensure_icons(cssc_gui_tree* t) {
    if (t->ico_loaded || !t->icondir[0]) return;
    if (t->ico_folder) cssc_sprite_free(t->ico_folder);
    if (t->ico_cssc)   cssc_sprite_free(t->ico_cssc);
    if (t->ico_md)     cssc_sprite_free(t->ico_md);
    if (t->ico_ini)    cssc_sprite_free(t->ico_ini);
    if (t->ico_file)   cssc_sprite_free(t->ico_file);
    if (t->ico_exe)     cssc_sprite_free(t->ico_exe);
    if (t->ico_csscu)   cssc_sprite_free(t->ico_csscu);
    if (t->ico_project) cssc_sprite_free(t->ico_project);
    if (t->ico_arrow_open)   cssc_sprite_free(t->ico_arrow_open);
    if (t->ico_arrow_closed) cssc_sprite_free(t->ico_arrow_closed);
    for (int i = 0; i < t->ico_ext_n; i++)
        if (t->ico_ext[i].spr) cssc_sprite_free(t->ico_ext[i].spr);
    t->ico_ext_n = 0;
    t->ico_folder = tree_load_icon(t->icondir, "folder.png");
    t->ico_cssc   = tree_load_icon(t->icondir, "cssc_file.png");
    t->ico_md     = tree_load_icon(t->icondir, "markdown.png");
    t->ico_ini    = tree_load_icon(t->icondir, "ini.png");
    t->ico_file   = tree_load_icon(t->icondir, "file.png");
    t->ico_exe     = tree_load_icon(t->icondir, "exe.png");
    t->ico_csscu   = tree_load_icon(t->icondir, "csscu.png");
    t->ico_project = tree_load_icon(t->icondir, "cssc.project.png");
    t->ico_arrow_open   = tree_load_icon(t->icondir, "arrow_opened.png");
    t->ico_arrow_closed = tree_load_icon(t->icondir, "arrow_closed.png");
    t->ico_loaded = 1;
}

static void* tree_ext_icon(cssc_gui_tree* t, const char* dotext) {
    if (!dotext || !dotext[0] || !t->icondir[0]) return NULL;
    char ext[24]; int j = 0;
    for (const char* p = dotext + 1; *p && j < 23; p++) {
        char c = *p; if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        ext[j++] = c;
    }
    ext[j] = 0;
    if (!ext[0]) return NULL;
    for (int i = 0; i < t->ico_ext_n; i++)
        if (!strcmp(t->ico_ext[i].ext, ext)) return t->ico_ext[i].spr;
    void* spr = NULL;
    if (t->ico_ext_n < 48) {
        char fn[40]; snprintf(fn, sizeof(fn), "%s.png", ext);
        spr = tree_load_icon(t->icondir, fn);
        snprintf(t->ico_ext[t->ico_ext_n].ext, 24, "%s", ext);
        t->ico_ext[t->ico_ext_n].spr = spr;
        t->ico_ext_n++;
    }
    return spr;
}

static void* tree_pick_icon(cssc_gui_tree* t, int idx) {
    if (t->isdir[idx]) return t->ico_folder;
    const char* nm = t->names[idx];

    if (t->ico_project && (!strcmp(nm, "cssc.proj") || !strcmp(nm, "cssc.cproject")))
        return t->ico_project;
    const char* dot = strrchr(nm, '.');
    if (dot) {
        if (!strcmp(dot, ".cssc")) return t->ico_cssc;
        if (!strcmp(dot, ".md"))   return t->ico_md;
        if (t->ico_exe && !strcmp(dot, ".exe"))     return t->ico_exe;
        if (t->ico_csscu && !strcmp(dot, ".csscu")) return t->ico_csscu;
        if (t->ico_project && (!strcmp(dot, ".proj") || !strcmp(dot, ".cproject")))
            return t->ico_project;
        if (!strcmp(dot, ".ini") || !strcmp(dot, ".toml") || !strcmp(dot, ".json"))
            return t->ico_ini;

        void* de = tree_ext_icon(t, dot);
        if (de) return de;
    }
    return t->ico_file;
}

static void gui_blit_sprite(void* v, void* icon, int64_t x, int64_t y, int64_t scale) {
    if (!icon) return;
    int64_t iw = cssc_sprite_width(icon), ih = cssc_sprite_height(icon);
    for (int64_t sy = 0; sy < ih; ++sy)
        for (int64_t sx = 0; sx < iw; ++sx) {
            int64_t argb = cssc_sprite_get_pixel(icon, sx, sy);
            if (((uint64_t)argb >> 24) == 0) continue;
            cssc_video_fillrect(v, x + sx * scale, y + sy * scale, scale, scale, argb);
        }
}

static void gui_blit_sprite_fit(void* v, void* icon, int64_t x, int64_t y, int64_t box) {
    if (!icon || box <= 0) return;
    int64_t iw = cssc_sprite_width(icon), ih = cssc_sprite_height(icon);
    if (iw <= 0 || ih <= 0) return;
    for (int64_t dy = 0; dy < box; ++dy)
        for (int64_t dx = 0; dx < box; ++dx) {
            int64_t sx = dx * iw / box, sy = dy * ih / box;
            int64_t argb = cssc_sprite_get_pixel(icon, sx, sy);
            if (((uint64_t)argb >> 24) == 0) continue;
            cssc_video_fillrect(v, x + dx, y + dy, 1, 1, argb);
        }
}

CSSC_GUI_EXPORT void cssc_gui_tree_draw(void* tp) {
    cssc_gui_tree* t = (cssc_gui_tree*)tp;
    if (!t || !t->visible || !t->screen) return;
    tree_ensure_icons(t);
    void* v = t->screen->vid;
    cssc_video_fillrect(v, t->x, t->y, t->w, t->h, t->bg);
    cssc_video_draw_rect(v, t->x, t->y, t->w, t->h,
                         t->focused ? (int64_t)0xC0C74DE0 : (int64_t)0x50FFFFFF);
    int64_t glyph = 8 * t->scale;
    int64_t row_h = glyph + 4;
    int visible_rows = (int)((t->h - 4) / row_h);
    if (visible_rows < 1) visible_rows = 1;
    for (int r = 0; r < visible_rows; ++r) {
        int idx = t->top + r;
        if (idx >= t->nrows) break;
        int64_t ry = t->y + 2 + (int64_t)r * row_h;
        if (idx == t->selected)
            cssc_video_fillrect(v, t->x + 1, ry, t->w - 2, row_h, t->sel_bg);
        int64_t bx = t->x + 6 + (int64_t)t->depths[idx] * glyph;
        int64_t col = (idx == t->selected) ? t->sel_fg : t->fg;
        int64_t iy = ry + (row_h - glyph) / 2;
        if (t->isdir[idx]) {

            int open = tree_is_open(t, t->paths[idx]);
            void* arrow = open ? t->ico_arrow_open : t->ico_arrow_closed;
            if (t->ico_loaded && arrow) gui_blit_sprite_fit(v, arrow, bx, iy, 8 * t->scale);
            else tree_chevron(v, bx, iy, glyph, open, (int64_t)0xFF8A84A0);
        }
        int64_t ix = bx + glyph;
        void* icon = t->ico_loaded ? tree_pick_icon(t, idx) : NULL;
        if (icon) {

            gui_blit_sprite_fit(v, icon, ix, iy, 8 * t->scale);
        } else {
            int64_t isz = glyph / 2; if (isz < 4) isz = 4;
            int64_t icol = t->isdir[idx] ? (int64_t)0xFFE0B24B
                                         : list_icon_color(t->names[idx]);
            cssc_video_fillrect(v, ix, ry + (row_h - isz) / 2, isz, isz, icol);
        }
        cssc_video_draw_text(v, ix + glyph + 2, ry + 2, t->names[idx], col, t->scale);
    }
}

#define TERM_MAX_LINES 5000
#define TERM_COL_ECHO  ((int64_t)0xFFC74DE0)
#define TERM_COL_OK    ((int64_t)0xFF5FE0A0)
#define TERM_COL_ERR   ((int64_t)0xFFFF5FB0)
#define TERM_COL_SYS   ((int64_t)0xFF8A84A0)

static void term_grow(cssc_gui_terminal* t, int need) {
    if (need <= t->cap) return;
    int nc = t->cap ? t->cap : 64;
    while (nc < need) nc *= 2;
    char** nl = (char**)realloc(t->lines, (size_t)nc * sizeof(char*));
    int64_t* ncol = (int64_t*)realloc(t->line_col, (size_t)nc * sizeof(int64_t));
    if (nl) t->lines = nl;
    if (ncol) t->line_col = ncol;
    if (nl && ncol) t->cap = nc;
}
static void term_add_line(cssc_gui_terminal* t, const char* s, int64_t col) {
    term_grow(t, t->nlines + 1);
    if (t->cap < t->nlines + 1) return;
    t->lines[t->nlines] = gui_strdup(s ? s : "");
    t->line_col[t->nlines] = col;
    t->nlines++;
    if (t->nlines > TERM_MAX_LINES) {
        int drop = t->nlines - TERM_MAX_LINES;
        for (int i = 0; i < drop; i++) free(t->lines[i]);
        memmove(t->lines, t->lines + drop, (size_t)(t->nlines - drop) * sizeof(char*));
        memmove(t->line_col, t->line_col + drop,
                (size_t)(t->nlines - drop) * sizeof(int64_t));
        t->nlines -= drop;
    }
    t->stick = 1;
}
static void term_clear_lines(cssc_gui_terminal* t) {
    for (int i = 0; i < t->nlines; i++) free(t->lines[i]);
    t->nlines = 0; t->top = 0; t->stick = 1;
}
static int term_is_cssc_cmd(const char* w) {
    static const char* const c[] = {"build", "run", "hsim", "module", "doc",
        "help", "settings", "new", "flash", "test", "convert", "release",
        "update", "configure", "introspect", "workspace", "vscode",
        "analyze", "diagnostics", 0};
    for (int i = 0; c[i]; i++) if (!strcmp(w, c[i])) return 1;
    return 0;
}

static char term_cp_to_ascii(unsigned int cp) {
    if (cp < 0x80) return (char)cp;
    switch (cp) {
        case 0x2192: return '>';
        case 0x2190: return '<';
        case 0x2191: return '^';
        case 0x2193: return 'v';
        case 0x2713: case 0x2714: return '+';
        case 0x2717: case 0x2718: case 0x00D7: return 'x';
        case 0x26A0: return '!';
        case 0x2022: case 0x00B7: return '*';
        case 0x2500: case 0x2501: case 0x2504: return '-';
        case 0x2502: case 0x2503: return '|';
        case 0x250C: case 0x2514: case 0x2510: case 0x2518: return '+';
        case 0x2026: return '.';
        case 0x2018: case 0x2019: return '\'';
        case 0x201C: case 0x201D: return '"';
        case 0x2013: case 0x2014: return '-';
        default: return '?';
    }
}

static int64_t term_sgr_color(int code, int64_t def) {
    switch (code) {
        case 30: case 90: return (int64_t)0xFF6A6A6A;
        case 31: case 91: return (int64_t)0xFFFF5FB0;
        case 32: case 92: return (int64_t)0xFF5FE0A0;
        case 33: case 93: return (int64_t)0xFFE0C860;
        case 34: case 94: return (int64_t)0xFF6AA8FF;
        case 35:          return (int64_t)0xFFC74DE0;
        case 95:          return (int64_t)0xFFE49BFF;
        case 36: case 96: return (int64_t)0xFF5FD8E8;
        case 37: case 97: return (int64_t)0xFFEAEAEA;
        default: return def;
    }
}

static void term_emit_ch(cssc_gui_terminal* t, char ch) {
    if (t->partial_len < (int)sizeof(t->partial) - 1) t->partial[t->partial_len++] = ch;
}

static void term_finalize_line(cssc_gui_terminal* t) {
    t->partial[t->partial_len] = 0;
    int64_t col = t->line_col_set ? t->line_col_use : t->fg;
    term_add_line(t, t->partial, col);
    t->partial_len = 0;
    t->line_col_set = 0;
}

static int64_t term_xterm256(int n) {
    if (n < 0) n = 0; if (n > 255) n = 255;
    if (n < 16) {
        static const int codes[16] = { 30,31,32,33,34,35,36,37, 90,91,92,93,94,95,96,97 };
        return term_sgr_color(codes[n], (int64_t)0xFFEAEAEA);
    }
    if (n >= 232) {
        int v = 8 + (n - 232) * 10;
        return (int64_t)(0xFF000000u | ((unsigned)v << 16) | ((unsigned)v << 8) | (unsigned)v);
    }
    n -= 16;
    int r = (n / 36) % 6, g = (n / 6) % 6, b = n % 6;
    int rr = r ? r * 40 + 55 : 0, gg = g ? g * 40 + 55 : 0, bb = b ? b * 40 + 55 : 0;
    return (int64_t)(0xFF000000u | ((unsigned)rr << 16) | ((unsigned)gg << 8) | (unsigned)bb);
}

static void term_apply_sgr(cssc_gui_terminal* t) {
    t->esc_buf[t->esc_len] = 0;
    int params[16]; int np = 0;
    int num = 0, have = 0;
    for (int i = 0; ; i++) {
        char ch = t->esc_buf[i];
        if (ch >= '0' && ch <= '9') { num = num * 10 + (ch - '0'); have = 1; }
        else {
            if (np < 16) params[np++] = have ? num : 0;
            num = 0; have = 0;
            if (ch == 0) break;
        }
    }
    for (int i = 0; i < np; i++) {
        int code = params[i];
        if (code == 38 || code == 48) {
            int64_t rgb = -1;
            if (i + 1 < np && params[i + 1] == 2 && i + 4 < np) {
                int r = params[i + 2] & 0xFF, g = params[i + 3] & 0xFF, b = params[i + 4] & 0xFF;
                rgb = (int64_t)(0xFF000000u | ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b);
                i += 4;
            } else if (i + 1 < np && params[i + 1] == 5 && i + 2 < np) {
                rgb = term_xterm256(params[i + 2]);
                i += 2;
            } else { break; }
            if (code == 38 && rgb != -1 && !t->line_col_set) {
                t->line_col_use = rgb; t->line_col_set = 1;
            }
            continue;
        }
        if (code != 0 && !t->line_col_set) {
            int64_t c = term_sgr_color(code, t->fg);
            if (c != t->fg) { t->line_col_use = c; t->line_col_set = 1; }
        }
    }
}

static void term_feed_byte(cssc_gui_terminal* t, unsigned char c) {
    if (t->esc_st == 1) {
        if (c == '[')      { t->esc_st = 2; t->esc_len = 0; }
        else if (c == ']') { t->esc_st = 3; }
        else               { t->esc_st = 0; }
        return;
    }
    if (t->esc_st == 2) {
        if (c >= 0x40 && c <= 0x7E) {
            if (c == 'm') term_apply_sgr(t);
            else if (c == 'J') {
                t->esc_buf[t->esc_len] = 0;
                int n = 0;
                for (int i = 0; t->esc_buf[i] >= '0' && t->esc_buf[i] <= '9'; i++)
                    n = n * 10 + (t->esc_buf[i] - '0');
                if (n >= 2) { t->partial_len = 0; term_clear_lines(t); }
            }
            t->esc_st = 0;
        }
        else if (t->esc_len < (int)sizeof(t->esc_buf) - 1) t->esc_buf[t->esc_len++] = (char)c;
        return;
    }
    if (t->esc_st == 3) {
        if (c == 0x07)      t->esc_st = 0;
        else if (c == 0x1b) t->esc_st = 1;
        return;
    }
    if (c == 0x1b) { t->esc_st = 1; return; }
    if (c == '\r') return;
    if (c == '\n') { term_finalize_line(t); t->utf_need = 0; return; }
    if (t->utf_need > 0) {
        if (c >= 0x80 && c < 0xC0) {
            t->utf_cp = (t->utf_cp << 6) | (c & 0x3F);
            if (--t->utf_need == 0) term_emit_ch(t, term_cp_to_ascii(t->utf_cp));
            return;
        }
        t->utf_need = 0;
    }
    if (c < 0x80) { term_emit_ch(t, (char)c); return; }
    if (c >= 0xC0 && c < 0xE0) { t->utf_cp = c & 0x1F; t->utf_need = 1; return; }
    if (c >= 0xE0 && c < 0xF0) { t->utf_cp = c & 0x0F; t->utf_need = 2; return; }
    if (c >= 0xF0 && c < 0xF8) { t->utf_cp = c & 0x07; t->utf_need = 3; return; }
    term_emit_ch(t, '?');
}

static void term_spawn(cssc_gui_terminal* t, const char* cmd) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
    HANDLE rd = NULL, wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        term_add_line(t, "terminal: pipe creation failed", TERM_COL_ERR);
        return;
    }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    HANDLE in_rd = NULL, in_wr = NULL;
    if (!CreatePipe(&in_rd, &in_wr, &sa, 0)) {
        CloseHandle(rd); CloseHandle(wr);
        term_add_line(t, "terminal: stdin pipe creation failed", TERM_COL_ERR);
        return;
    }
    SetHandleInformation(in_wr, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr; si.hStdError = wr; si.hStdInput = in_rd;

    SetEnvironmentVariableA("PYTHONUNBUFFERED", "1");
    SetEnvironmentVariableA("PYTHONIOENCODING", "utf-8");
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    char cmdline[4200];
    snprintf(cmdline, sizeof(cmdline), "%s", cmd);
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, t->cwd[0] ? t->cwd : NULL, &si, &pi);
    CloseHandle(wr);
    CloseHandle(in_rd);
    if (ok) {
        CloseHandle(pi.hThread);
        t->proc = pi.hProcess;
        t->pipe_read = rd;
        t->stdin_write = in_wr;
        t->running = 1;
        t->partial_len = 0;
        t->esc_st = 0; t->esc_len = 0; t->utf_need = 0;
        t->line_col_set = 0;
    } else {
        CloseHandle(rd);
        CloseHandle(in_wr);
        term_add_line(t, "terminal: failed to start command", TERM_COL_ERR);
    }
#else
    (void)cmd;
    term_add_line(t, "terminal: subprocess unsupported on this platform", TERM_COL_ERR);
#endif
}

static void term_poll(cssc_gui_terminal* t) {
#ifdef _WIN32
    if (!t->running || !t->pipe_read) return;
    char buf[4096];
    DWORD avail = 0;
    while (PeekNamedPipe((HANDLE)t->pipe_read, NULL, 0, NULL, &avail, NULL) && avail > 0) {
        DWORD toread = avail < sizeof(buf) ? avail : (DWORD)sizeof(buf);
        DWORD got = 0;
        if (!ReadFile((HANDLE)t->pipe_read, buf, toread, &got, NULL) || got == 0) break;
        for (DWORD i = 0; i < got; i++) term_feed_byte(t, (unsigned char)buf[i]);
    }
    if (WaitForSingleObject((HANDLE)t->proc, 0) == WAIT_OBJECT_0) {
        if (t->partial_len > 0) term_finalize_line(t);
        DWORD code = 0; GetExitCodeProcess((HANDLE)t->proc, &code);
        char msg[64]; snprintf(msg, sizeof(msg), "[exit %lu]", (unsigned long)code);
        term_add_line(t, msg, code == 0 ? TERM_COL_OK : TERM_COL_ERR);
        CloseHandle((HANDLE)t->proc); CloseHandle((HANDLE)t->pipe_read);
        if (t->stdin_write) CloseHandle((HANDLE)t->stdin_write);
        t->proc = NULL; t->pipe_read = NULL; t->stdin_write = NULL; t->running = 0;
    }
#else
    (void)t;
#endif
}

static void term_interrupt(cssc_gui_terminal* t) {
#ifdef _WIN32
    if (!t->running || !t->proc) return;
    TerminateProcess((HANDLE)t->proc, 1);
    if (t->partial_len > 0) term_finalize_line(t);
    term_add_line(t, "^X interrupted", TERM_COL_ERR);
    CloseHandle((HANDLE)t->proc);
    if (t->pipe_read) CloseHandle((HANDLE)t->pipe_read);
    if (t->stdin_write) CloseHandle((HANDLE)t->stdin_write);
    t->proc = NULL; t->pipe_read = NULL; t->stdin_write = NULL; t->running = 0;
#else
    (void)t;
#endif
}

static void term_hist_push(cssc_gui_terminal* t, const char* cmd) {
    if (!cmd[0]) return;
    if (t->nhist >= t->cap_hist) {
        int nc = t->cap_hist ? t->cap_hist * 2 : 32;
        char** nh = (char**)realloc(t->hist, (size_t)nc * sizeof(char*));
        if (!nh) return;
        t->hist = nh; t->cap_hist = nc;
    }
    t->hist[t->nhist++] = gui_strdup(cmd);
    t->hist_pos = t->nhist;
}

static void term_submit(cssc_gui_terminal* t) {
    t->input[t->input_len] = 0;

    if (t->running && t->stdin_write) {

        for (int i = 0; i < t->input_len &&
                        t->partial_len < (int)sizeof(t->partial) - 1; i++)
            t->partial[t->partial_len++] = t->input[i];
        t->partial[t->partial_len] = 0;
        term_add_line(t, t->partial, TERM_COL_SYS);
        t->partial_len = 0;
#ifdef _WIN32
        char linebuf[1100];
        int ln = snprintf(linebuf, sizeof(linebuf), "%s\n", t->input);
        DWORD wrote = 0;
        WriteFile((HANDLE)t->stdin_write, linebuf, (DWORD)ln, &wrote, NULL);
        FlushFileBuffers((HANDLE)t->stdin_write);
#endif
        t->input_len = 0; t->input_cur = 0;
        return;
    }
    char echo[1200];
    snprintf(echo, sizeof(echo), "%s> %s", t->cwd, t->input);
    term_add_line(t, echo, TERM_COL_ECHO);
    term_hist_push(t, t->input);

    const char* p = t->input;
    while (*p == ' ') p++;
    char tok[64]; int tn = 0;
    while (*p && *p != ' ' && tn < 63) tok[tn++] = *p++;
    tok[tn] = 0;
    while (*p == ' ') p++;
    if (tok[0] == 0) { t->input_len = 0; t->input_cur = 0; return; }
    if (!strcmp(tok, "clear") || !strcmp(tok, "cls")) {
        term_clear_lines(t);
    } else if (!strcmp(tok, "cpall") || !strcmp(tok, "cpallc")) {

        size_t total = 1;
        for (int i = 0; i < t->nlines; i++) total += strlen(t->lines[i]) + 1;
        char* buf = (char*)malloc(total);
        if (buf) {
            size_t o = 0;
            for (int i = 0; i < t->nlines; i++) {
                size_t l = strlen(t->lines[i]);
                memcpy(buf + o, t->lines[i], l); o += l;
                buf[o++] = '\n';
            }
            buf[o] = 0;
            gui_clipboard_set(buf);
            free(buf);
        }
        if (!strcmp(tok, "cpallc")) term_clear_lines(t);
        term_add_line(t, "copied", TERM_COL_SYS);
    } else if (!strcmp(tok, "cd")) {
#ifdef _WIN32
        char save[1024]; GetCurrentDirectoryA(sizeof(save), save);
        SetCurrentDirectoryA(t->cwd);
        if (*p && SetCurrentDirectoryA(p)) {
            GetCurrentDirectoryA(sizeof(t->cwd), t->cwd);
        } else if (!*p) {

            term_add_line(t, t->cwd, t->fg);
        } else {
            term_add_line(t, "cd: no such directory", TERM_COL_ERR);
        }
        SetCurrentDirectoryA(save);
#endif
    } else if (term_is_cssc_cmd(tok)) {
        char cmd[2200];
        snprintf(cmd, sizeof(cmd), "cmd.exe /d /c cssc %s", t->input);
        term_spawn(t, cmd);
    } else {

        char cmd[2200];
        snprintf(cmd, sizeof(cmd),
                 "powershell -NoProfile -ExecutionPolicy Bypass -Command %s",
                 t->input);
        term_spawn(t, cmd);
    }
    t->input_len = 0; t->input_cur = 0;
}

static int term_hit(cssc_gui_terminal* t, int64_t mx, int64_t my,
                    int* line, int* col) {
    int64_t glyph = 8 * t->scale;
    int64_t line_h = glyph + 4;
    int64_t input_h = line_h + 4;
    int64_t iy = t->y + t->h - input_h;
    if (mx < t->x || mx >= t->x + t->w || my < t->y + 4 || my >= iy) return 0;
    if (t->nlines == 0) return 0;
    int r = (int)((my - (t->y + 4)) / line_h);
    int li = t->top + r;
    if (li < 0) li = 0;
    if (li >= t->nlines) li = t->nlines - 1;
    int c = (int)((mx - (t->x + 6)) / glyph);
    if (c < 0) c = 0;
    int ll = (int)strlen(t->lines[li]);
    if (c > ll) c = ll;
    *line = li; *col = c;
    return 1;
}
static void term_sel_norm(cssc_gui_terminal* t, int* sl, int* sc, int* el, int* ec) {
    if (t->sa_l < t->se_l || (t->sa_l == t->se_l && t->sa_c <= t->se_c)) {
        *sl = t->sa_l; *sc = t->sa_c; *el = t->se_l; *ec = t->se_c;
    } else {
        *sl = t->se_l; *sc = t->se_c; *el = t->sa_l; *ec = t->sa_c;
    }
}
static char* term_selected_str(cssc_gui_terminal* t) {
    if (!t->sel_active || t->nlines == 0) return NULL;
    int sl, sc, el, ec; term_sel_norm(t, &sl, &sc, &el, &ec);
    if (sl < 0) sl = 0;
    if (el >= t->nlines) el = t->nlines - 1;
    if (sl > el) return NULL;
    if (sl == el) {
        int ll = (int)strlen(t->lines[sl]);
        if (ec > ll) ec = ll; if (sc > ll) sc = ll;
        char* o = (char*)malloc((size_t)(ec - sc) + 1);
        if (!o) return NULL;
        memcpy(o, t->lines[sl] + sc, (size_t)(ec - sc)); o[ec - sc] = 0;
        return o;
    }
    int fl = (int)strlen(t->lines[sl]); if (sc > fl) sc = fl;
    size_t total = (size_t)(fl - sc) + 1;
    for (int i = sl + 1; i < el; i++) total += strlen(t->lines[i]) + 1;
    total += (size_t)ec;
    char* o = (char*)malloc(total + 1);
    if (!o) return NULL;
    size_t off = 0;
    memcpy(o + off, t->lines[sl] + sc, (size_t)(fl - sc)); off += (size_t)(fl - sc);
    o[off++] = '\n';
    for (int i = sl + 1; i < el; i++) {
        int l = (int)strlen(t->lines[i]);
        memcpy(o + off, t->lines[i], (size_t)l); off += (size_t)l;
        o[off++] = '\n';
    }
    int lc = (int)strlen(t->lines[el]); if (ec > lc) ec = lc;
    memcpy(o + off, t->lines[el], (size_t)ec); off += (size_t)ec;
    o[off] = 0;
    return o;
}

CSSC_GUI_EXPORT void* cssc_gui_terminal_new(void* screen) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)calloc(1, sizeof(cssc_gui_terminal));
    if (!t) return NULL;
    t->kind = GW_TERMINAL;
    t->screen = (cssc_gui_screen*)screen;
    t->x = 20; t->y = 20; t->w = 600; t->h = 200;
    t->scale = 2;
    t->bg = (int64_t)0xFF0B0910;
    t->fg = (int64_t)0xFFCFC8E0;
    t->visible = 1;
    t->stick = 1;
#ifdef _WIN32
    GetCurrentDirectoryA(sizeof(t->cwd), t->cwd);
#endif
    term_add_line(t, "CSSC terminal - type build/run/hsim/module (CSSC), or ls/cd/git (shell).",
                  TERM_COL_SYS);
    return t;
}
CSSC_GUI_EXPORT void cssc_gui_terminal_setrect(void* p, int64_t x, int64_t y,
                                               int64_t w, int64_t h) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (t) { t->x = x; t->y = y; t->w = w; t->h = h; }
}
CSSC_GUI_EXPORT void cssc_gui_terminal_setcwd(void* p, const char* dir) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (!t || !dir) return;
    size_t n = strlen(dir); if (n >= sizeof(t->cwd)) n = sizeof(t->cwd) - 1;
    memcpy(t->cwd, dir, n); t->cwd[n] = 0;
}
CSSC_GUI_EXPORT void cssc_gui_terminal_setfocus(void* p, int64_t f) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) t->focused = f ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_terminal_setscale(void* p, int64_t s) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) t->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_terminal_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (t) { t->bg = bg & 0xFFFFFFFF; t->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT void cssc_gui_terminal_setvisible(void* p, int64_t vis) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) t->visible = vis ? 1 : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_terminal_isrunning(void* p) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; return t ? (int64_t)t->running : 0;
}

CSSC_GUI_EXPORT void cssc_gui_terminal_run(void* p, const char* cmd) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (!t || !cmd) return;
    size_t n = strlen(cmd); if (n >= sizeof(t->input)) n = sizeof(t->input) - 1;
    memcpy(t->input, cmd, n); t->input[n] = 0; t->input_len = (int)n;
    term_submit(t);
}

CSSC_GUI_EXPORT void cssc_gui_terminal_write(void* p, const char* text) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (t) term_add_line(t, text ? text : "", t->fg);
}
CSSC_GUI_EXPORT void cssc_gui_terminal_clear(void* p) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) term_clear_lines(t);
}

CSSC_GUI_EXPORT int64_t cssc_gui_terminal_update(void* tp, void* sp) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)tp;
    if (!t) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : t->screen;
    if (!s) return 0;
    term_poll(t);
    if (!t->visible) return 0;

    int thl, thc;
    if (s->down && !s->prev_down && !s->input_captured) {
        if (term_hit(t, s->mx, s->my, &thl, &thc)) {
            t->sa_l = thl; t->sa_c = thc; t->se_l = thl; t->se_c = thc;
            t->sel_active = 0; t->dragging = 1;
        }
    } else if (s->down && t->dragging) {
        if (term_hit(t, s->mx, s->my, &thl, &thc)) {
            t->se_l = thl; t->se_c = thc;
            if (thl != t->sa_l || thc != t->sa_c) t->sel_active = 1;
        }
    } else if (!s->down) {
        t->dragging = 0;
    }
    if (t->focused && !s->input_captured) {
#ifdef _WIN32

        int altc = ((GetKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState('C') & 0x8000)) ? 1 : 0;
        if (altc && !t->alt_c_prev) term_clear_lines(t);
        t->alt_c_prev = altc;
#endif
        int64_t c;
        while ((c = cssc_video_poll_char(s->vid)) != 0) {
            if (c == 13 || c == 10) {
                term_submit(t);
            } else if (c == 3) {
                char* sel = term_selected_str(t);
                if (sel) { gui_clipboard_set(sel); free(sel); }
            } else if (c == 24) {
                term_interrupt(t);
            } else if (c == 8) {
                if (t->input_cur > 0) {
                    memmove(t->input + t->input_cur - 1, t->input + t->input_cur,
                            (size_t)(t->input_len - t->input_cur) + 1);
                    t->input_cur--; t->input_len--;
                }
            } else if (c >= 32 && c < 127 && t->input_len < (int)sizeof(t->input) - 1) {
                memmove(t->input + t->input_cur + 1, t->input + t->input_cur,
                        (size_t)(t->input_len - t->input_cur) + 1);
                t->input[t->input_cur] = (char)c;
                t->input_cur++; t->input_len++;
            }
        }
        int64_t k;
        while ((k = cssc_video_poll_key(s->vid)) != 0) {
            if (k == 0x25 && t->input_cur > 0) t->input_cur--;
            else if (k == 0x27 && t->input_cur < t->input_len) t->input_cur++;
            else if (k == 0x24) t->input_cur = 0;
            else if (k == 0x23) t->input_cur = t->input_len;
            else if (k == 0x26) {
                if (t->nhist > 0 && t->hist_pos > 0) {
                    t->hist_pos--;
                    const char* h = t->hist[t->hist_pos];
                    size_t n = strlen(h); if (n >= sizeof(t->input)) n = sizeof(t->input) - 1;
                    memcpy(t->input, h, n); t->input[n] = 0;
                    t->input_len = (int)n; t->input_cur = (int)n;
                }
            } else if (k == 0x28) {
                if (t->hist_pos < t->nhist - 1) {
                    t->hist_pos++;
                    const char* h = t->hist[t->hist_pos];
                    size_t n = strlen(h); if (n >= sizeof(t->input)) n = sizeof(t->input) - 1;
                    memcpy(t->input, h, n); t->input[n] = 0;
                    t->input_len = (int)n; t->input_cur = (int)n;
                } else {
                    t->hist_pos = t->nhist; t->input[0] = 0;
                    t->input_len = 0; t->input_cur = 0;
                }
            }
        }
        int64_t wh = cssc_video_wheel(s->vid);
        if (wh) {
            t->top -= (int)wh * 3;
            t->stick = 0;
            if (t->top < 0) t->top = 0;
        }
    }
    return 0;
}

CSSC_GUI_EXPORT void cssc_gui_terminal_draw(void* tp) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)tp;
    if (!t || !t->visible || !t->screen) return;
    void* v = t->screen->vid;
    cssc_video_fillrect(v, t->x, t->y, t->w, t->h, t->bg);
    cssc_video_draw_rect(v, t->x, t->y, t->w, t->h,
                         t->focused ? (int64_t)0xC0C74DE0 : (int64_t)0x50FFFFFF);
    int64_t glyph = 8 * t->scale;
    int64_t line_h = glyph + 4;
    int input_h = (int)line_h + 4;
    int visible_rows = (int)((t->h - 6 - input_h) / line_h);
    if (visible_rows < 1) visible_rows = 1;
    int has_partial = t->partial_len > 0 ? 1 : 0;
    int total = t->nlines + has_partial;
    int max_top = total - visible_rows; if (max_top < 0) max_top = 0;
    if (t->stick) t->top = max_top;
    if (t->top > max_top) t->top = max_top;
    if (t->top < 0) t->top = 0;
    int max_cols = (int)((t->w - 12) / glyph); if (max_cols < 1) max_cols = 1;
    if (max_cols > 511) max_cols = 511;
    int ssl = 0, ssc = 0, sel_e_l = 0, sel_e_c = 0;
    if (t->sel_active) term_sel_norm(t, &ssl, &ssc, &sel_e_l, &sel_e_c);
    char sub[512];
    for (int r = 0; r < visible_rows; ++r) {
        int li = t->top + r;
        if (li >= total) break;
        int64_t ly = t->y + 4 + (int64_t)r * line_h;
        const char* line; int64_t lcol;
        if (li < t->nlines) { line = t->lines[li]; lcol = t->line_col[li]; }
        else { t->partial[t->partial_len] = 0; line = t->partial; lcol = t->fg; }
        int slen = (int)strlen(line);
        if (t->sel_active && li < t->nlines && li >= ssl && li <= sel_e_l) {
            int cstart = (li == ssl) ? ssc : 0;
            int cend = (li == sel_e_l) ? sel_e_c : slen;
            if (cstart < 0) cstart = 0;
            if (cend > slen) cend = slen;
            if (cend > max_cols) cend = max_cols;
            if (cstart > max_cols) cstart = max_cols;
            if (cend > cstart)
                cssc_video_fillrect(v, t->x + 6 + (int64_t)cstart * glyph, ly - 1,
                                    (int64_t)(cend - cstart) * glyph, line_h,
                                    (int64_t)0x804D6AA8);
        }
        int cnt = slen; if (cnt > max_cols) cnt = max_cols;
        if (cnt > 0) { memcpy(sub, line, (size_t)cnt); sub[cnt] = 0; } else sub[0] = 0;
        cssc_video_draw_text(v, t->x + 6, ly, sub, lcol, t->scale);
    }

    int64_t iy = t->y + t->h - input_h;
    cssc_video_fillrect(v, t->x + 1, iy, t->w - 2, input_h, (int64_t)0xFF141019);
    cssc_video_draw_text(v, t->x + 6, iy + 2, t->running ? "*" : ">",
                         TERM_COL_ECHO, t->scale);
    int in_cols = max_cols - 2; if (in_cols < 1) in_cols = 1;
    int ifrom = 0;
    if (t->input_cur > in_cols) ifrom = t->input_cur - in_cols;
    int icnt = t->input_len - ifrom; if (icnt > in_cols) icnt = in_cols;
    if (icnt > 0) { memcpy(sub, t->input + ifrom, (size_t)icnt); sub[icnt] = 0; } else sub[0] = 0;
    int64_t itx = t->x + 6 + glyph + 4;
    cssc_video_draw_text(v, itx, iy + 2, sub, t->fg, t->scale);
    if (t->focused) {
        int64_t cx = itx + (int64_t)(t->input_cur - ifrom) * glyph;
        cssc_video_fillrect(v, cx, iy + 2, 2, glyph, TERM_COL_ECHO);
    }
}

static int64_t menu_title_x(cssc_gui_menu* m, int idx) {
    int64_t glyph = 8 * m->scale;
    int64_t tx = m->x + 12;
    for (int i = 0; i < idx; i++)
        tx += (int64_t)strlen(m->titles[i]) * glyph + 20;
    return tx;
}
static int64_t menu_title_w(cssc_gui_menu* m, int idx) {
    return (int64_t)strlen(m->titles[idx]) * (8 * m->scale) + 20;
}
static int64_t menu_dropdown_w(cssc_gui_menu* m, int menu1) {
    int64_t glyph = 8 * m->scale;
    int64_t w = 160;
    for (int i = 0; i < m->n_items; i++)
        if (m->item_menu[i] == menu1) {
            int64_t iw = (int64_t)strlen(m->item_label[i]) * glyph + 32;
            if (iw > w) w = iw;
        }
    return w;
}

#define DBG_COL_BG    ((int64_t)0xF00A0710)
#define DBG_COL_HEAD  ((int64_t)0xFF5FD8E8)
#define DBG_COL_NAME  ((int64_t)0xFFEAEAEA)
#define DBG_COL_ADDR  ((int64_t)0xFF8A84A0)
#define DBG_COL_TYPE  ((int64_t)0xFF6AA8FF)
#define DBG_COL_VAL   ((int64_t)0xFF5FE0A0)
#define DBG_COL_DEAD  ((int64_t)0xFF6A6A6A)
#define DBG_COL_FREE  ((int64_t)0xFFE0C860)
#define DBG_COL_READ  ((int64_t)0xFFC74DE0)
#define DBG_COL_ERR   ((int64_t)0xFFFF5FB0)

typedef struct {
    char     name[64];
    char     region[16];
    char     type[24];
    char     value[192];
    uint64_t addr;
    int      line;
    int      live;
    int      stamp;
} dbg_alloc;

typedef struct {
    int              kind;
    cssc_gui_screen* screen;

    void*  proc; void* pipe_read; void* stdin_write; int running;
    char   partial[16384]; int partial_len;

    dbg_alloc* allocs; int nallocs, cap_allocs;

    char** log; int64_t* log_col; int nlog, cap_log, log_top;
    char   logpartial[4096]; int logpartial_len;

    int  ip_line; char ip_scope[64]; int stepcount;
    int  crashed; int crash_line; char crash_msg[256];
    int  ended; int exit_code;

    int  active;
    int  focused;
    int  tab;
    int  alloc_top;
    int  ip_follow;
    char probe[80]; int probe_len;
    char last_read[300];
    char title[520];
    int64_t scale;
} cssc_gui_debugger;

static int dbg_json_str(const char* line, const char* key, char* out, int outsz) {
    if (outsz > 0) out[0] = 0;
    char pat[72];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char* p = strstr(line, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return 0;
    p++;
    int i = 0;
    while (*p && *p != '"') {
        char c = *p;
        if (c == '\\' && p[1]) {
            p++;
            switch (*p) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                default:  c = *p;   break;
            }
        }
        if (i < outsz - 1) out[i++] = c;
        p++;
    }
    if (outsz > 0) out[i] = 0;
    return 1;
}

static uint64_t dbg_json_u64(const char* line, const char* key) {
    char pat[72];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char* p = strstr(line, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t') p++;
    if (*p != ':') return 0;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '"') p++;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) return strtoull(p, NULL, 16);
    return strtoull(p, NULL, 10);
}

static int dbg_json_bool_true(const char* line, const char* key) {
    char pat[72];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char* p = strstr(line, pat);
    if (!p) return 0;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    return strncmp(p, "true", 4) == 0;
}

static void dbg_log(cssc_gui_debugger* d, const char* s, int64_t col) {
    if (d->nlog >= d->cap_log) {
        int nc = d->cap_log ? d->cap_log * 2 : 128;
        char** nl = (char**)realloc(d->log, (size_t)nc * sizeof(char*));
        if (!nl) return;
        d->log = nl;
        int64_t* ncol = (int64_t*)realloc(d->log_col, (size_t)nc * sizeof(int64_t));
        if (!ncol) return;
        d->log_col = ncol;
        d->cap_log = nc;
    }
    d->log[d->nlog] = gui_strdup(s ? s : "");
    d->log_col[d->nlog] = col;
    d->nlog++;
}

static void dbg_log_output(cssc_gui_debugger* d, const char* text) {
    for (const char* p = text; *p; p++) {
        char c = *p;
        if (c == '\r') continue;
        if (c == '\n') {
            d->logpartial[d->logpartial_len] = 0;
            dbg_log(d, d->logpartial, (int64_t)0xFFCFC8E0);
            d->logpartial_len = 0;
        } else if (d->logpartial_len < (int)sizeof(d->logpartial) - 1) {
            d->logpartial[d->logpartial_len++] = c;
        }
    }
}

static int dbg_find_alloc(cssc_gui_debugger* d, const char* name) {
    for (int i = 0; i < d->nallocs; i++)
        if (strcmp(d->allocs[i].name, name) == 0) return i;
    return -1;
}

static void dbg_upsert_alloc(cssc_gui_debugger* d, const char* name,
                             const char* region, const char* type,
                             const char* value, uint64_t addr, int line) {
    int idx = dbg_find_alloc(d, name);
    if (idx < 0) {
        if (d->nallocs >= d->cap_allocs) {
            int nc = d->cap_allocs ? d->cap_allocs * 2 : 64;
            dbg_alloc* na = (dbg_alloc*)realloc(d->allocs, (size_t)nc * sizeof(dbg_alloc));
            if (!na) return;
            d->allocs = na; d->cap_allocs = nc;
        }
        idx = d->nallocs++;
        memset(&d->allocs[idx], 0, sizeof(dbg_alloc));
    }
    dbg_alloc* a = &d->allocs[idx];
    snprintf(a->name,   sizeof(a->name),   "%s", name);
    snprintf(a->region, sizeof(a->region), "%s", region);
    snprintf(a->type,   sizeof(a->type),   "%s", type);
    snprintf(a->value,  sizeof(a->value),  "%s", value);
    a->addr = addr; a->line = line; a->live = 1;
    a->stamp = ++d->stepcount;
}

static void dbg_parse_line(cssc_gui_debugger* d, const char* line) {
    char ev[24];
    if (!dbg_json_str(line, "ev", ev, sizeof(ev))) return;
    if (strcmp(ev, "alloc") == 0 || strcmp(ev, "write") == 0) {
        char name[64], region[16], type[24], value[192];
        dbg_json_str(line, "name",   name,   sizeof(name));
        dbg_json_str(line, "region", region, sizeof(region));
        dbg_json_str(line, "type",   type,   sizeof(type));
        dbg_json_str(line, "value",  value,  sizeof(value));
        uint64_t addr = dbg_json_u64(line, "addr");
        int ln = (int)dbg_json_u64(line, "line");
        dbg_upsert_alloc(d, name, region, type, value, addr, ln);
    } else if (strcmp(ev, "free") == 0) {
        char name[64]; dbg_json_str(line, "name", name, sizeof(name));
        int idx = dbg_find_alloc(d, name);
        if (idx >= 0) d->allocs[idx].live = 0;
        char msg[160];
        snprintf(msg, sizeof(msg), "- freed  %s   (line %d)",
                 name, (int)dbg_json_u64(line, "line"));
        dbg_log(d, msg, DBG_COL_FREE);
    } else if (strcmp(ev, "step") == 0) {
        d->ip_line = (int)dbg_json_u64(line, "line");
        dbg_json_str(line, "scope", d->ip_scope, sizeof(d->ip_scope));
    } else if (strcmp(ev, "output") == 0) {
        char text[2048]; dbg_json_str(line, "text", text, sizeof(text));
        dbg_log_output(d, text);
    } else if (strcmp(ev, "crash") == 0) {
        d->crashed = 1;
        d->crash_line = (int)dbg_json_u64(line, "line");
        dbg_json_str(line, "detail", d->crash_msg, sizeof(d->crash_msg));
        char msg[320];
        snprintf(msg, sizeof(msg), "CRASH  line %d:  %s", d->crash_line, d->crash_msg);
        dbg_log(d, msg, DBG_COL_ERR);
    } else if (strcmp(ev, "end") == 0) {
        if (d->logpartial_len > 0) {
            d->logpartial[d->logpartial_len] = 0;
            dbg_log(d, d->logpartial, (int64_t)0xFFCFC8E0);
            d->logpartial_len = 0;
        }
        d->ended = 1;
        d->exit_code = (int)dbg_json_u64(line, "exit");
        char msg[64];
        snprintf(msg, sizeof(msg), "[program ended  exit %d]", d->exit_code);
        dbg_log(d, msg, d->exit_code == 0 ? DBG_COL_VAL : DBG_COL_ERR);
    } else if (strcmp(ev, "read_result") == 0) {
        char addr[24]; dbg_json_str(line, "addr", addr, sizeof(addr));
        if (dbg_json_bool_true(line, "found")) {
            char name[64], type[24], value[192], raw[160];
            dbg_json_str(line, "name",  name,  sizeof(name));
            dbg_json_str(line, "type",  type,  sizeof(type));
            dbg_json_str(line, "value", value, sizeof(value));
            dbg_json_str(line, "raw",   raw,   sizeof(raw));
            snprintf(d->last_read, sizeof(d->last_read),
                     "%s   %s %s = %s   raw[%s]", addr, type, name, value, raw);
        } else {
            snprintf(d->last_read, sizeof(d->last_read),
                     "%s   <no live allocation at this address>", addr);
        }
        dbg_log(d, d->last_read, DBG_COL_READ);
    }
}

static void dbg_feed_byte(cssc_gui_debugger* d, char c) {
    if (c == '\n') {
        d->partial[d->partial_len] = 0;
        if (d->partial_len > 0) dbg_parse_line(d, d->partial);
        d->partial_len = 0;
    } else if (c != '\r') {
        if (d->partial_len < (int)sizeof(d->partial) - 1)
            d->partial[d->partial_len++] = c;
        else
            d->partial_len = 0;
    }
}

static void dbg_spawn(cssc_gui_debugger* d, const char* cmd) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
    HANDLE rd = NULL, wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) { dbg_log(d, "debugger: pipe failed", DBG_COL_ERR); return; }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    HANDLE in_rd = NULL, in_wr = NULL;
    if (!CreatePipe(&in_rd, &in_wr, &sa, 0)) { CloseHandle(rd); CloseHandle(wr); return; }
    SetHandleInformation(in_wr, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFOA si; ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = wr; si.hStdError = wr; si.hStdInput = in_rd;
    SetEnvironmentVariableA("PYTHONUNBUFFERED", "1");
    SetEnvironmentVariableA("PYTHONIOENCODING", "utf-8");
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    char cmdline[4200];
    snprintf(cmdline, sizeof(cmdline), "%s", cmd);
    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, NULL, &si, &pi);
    CloseHandle(wr); CloseHandle(in_rd);
    if (ok) {
        CloseHandle(pi.hThread);
        d->proc = pi.hProcess; d->pipe_read = rd; d->stdin_write = in_wr;
        d->running = 1; d->partial_len = 0;
    } else {
        CloseHandle(rd); CloseHandle(in_wr);
        dbg_log(d, "debugger: failed to start pctrace", DBG_COL_ERR);
    }
#else
    (void)cmd; dbg_log(d, "debugger: unsupported platform", DBG_COL_ERR);
#endif
}

static void dbg_drain(cssc_gui_debugger* d) {
#ifdef _WIN32
    if (!d->pipe_read) return;
    char buf[8192];
    DWORD avail = 0;
    while (PeekNamedPipe((HANDLE)d->pipe_read, NULL, 0, NULL, &avail, NULL) && avail > 0) {
        DWORD toread = avail < sizeof(buf) ? avail : (DWORD)sizeof(buf);
        DWORD got = 0;
        if (!ReadFile((HANDLE)d->pipe_read, buf, toread, &got, NULL) || got == 0) break;
        for (DWORD i = 0; i < got; i++) dbg_feed_byte(d, buf[i]);
    }
#else
    (void)d;
#endif
}

static void dbg_poll(cssc_gui_debugger* d) {
#ifdef _WIN32
    if (!d->running || !d->pipe_read) return;
    dbg_drain(d);

    if (d->proc && WaitForSingleObject((HANDLE)d->proc, 0) == WAIT_OBJECT_0) {
        dbg_drain(d);
        CloseHandle((HANDLE)d->proc);
        if (d->pipe_read) CloseHandle((HANDLE)d->pipe_read);
        if (d->stdin_write) CloseHandle((HANDLE)d->stdin_write);
        d->proc = NULL; d->pipe_read = NULL; d->stdin_write = NULL; d->running = 0;
    }
#else
    (void)d;
#endif
}

static void dbg_send_read(cssc_gui_debugger* d, const char* addr) {
#ifdef _WIN32
    if (!d->running || !d->stdin_write) return;
    char linebuf[128];
    int ln = snprintf(linebuf, sizeof(linebuf), "read %s\n", addr);
    DWORD wrote = 0;
    WriteFile((HANDLE)d->stdin_write, linebuf, (DWORD)ln, &wrote, NULL);
    FlushFileBuffers((HANDLE)d->stdin_write);
#else
    (void)d; (void)addr;
#endif
}

static void dbg_stop_proc(cssc_gui_debugger* d) {
#ifdef _WIN32
    if (d->stdin_write) {
        DWORD wrote = 0;
        WriteFile((HANDLE)d->stdin_write, "quit\n", 5, &wrote, NULL);
        FlushFileBuffers((HANDLE)d->stdin_write);
    }
    if (d->proc) {
        WaitForSingleObject((HANDLE)d->proc, 200);
        TerminateProcess((HANDLE)d->proc, 0);
        CloseHandle((HANDLE)d->proc);
    }
    if (d->pipe_read) CloseHandle((HANDLE)d->pipe_read);
    if (d->stdin_write) CloseHandle((HANDLE)d->stdin_write);
    d->proc = NULL; d->pipe_read = NULL; d->stdin_write = NULL; d->running = 0;
#else
    (void)d;
#endif
}

static void dbg_reset(cssc_gui_debugger* d) {
    for (int i = 0; i < d->nlog; i++) free(d->log[i]);
    d->nlog = 0; d->log_top = 0; d->logpartial_len = 0;
    d->nallocs = 0;
    d->partial_len = 0;
    d->ip_line = 0; d->ip_scope[0] = 0; d->stepcount = 0;
    d->crashed = 0; d->crash_line = 0; d->crash_msg[0] = 0;
    d->ended = 0; d->exit_code = 0;
    d->tab = 0; d->alloc_top = 0; d->probe_len = 0; d->probe[0] = 0;
    d->last_read[0] = 0;
}

static void dbg_rect(cssc_gui_debugger* d, int64_t W, int64_t H,
                     int64_t* x, int64_t* y, int64_t* w, int64_t* h) {
    int64_t rw, rh;
    if (d->focused) { rw = W * 85 / 100; rh = H * 82 / 100; }
    else            { rw = 400;          rh = 150; }
    if (rw > W - 32) rw = W - 32;
    if (rh > H - 32) rh = H - 32;
    if (rw < 120) rw = 120;
    if (rh < 60)  rh = 60;
    *w = rw; *h = rh;
    *x = W - rw - 16;
    *y = H - rh - 16;
}

CSSC_GUI_EXPORT void* cssc_gui_debugger_new(void* screen) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)calloc(1, sizeof(cssc_gui_debugger));
    if (!d) return NULL;
    d->kind = GW_DEBUGGER;
    d->screen = (cssc_gui_screen*)screen;
    d->scale = 2;
    d->ip_follow = 1;
    return d;
}

CSSC_GUI_EXPORT void cssc_gui_debugger_start(void* p, const char* cmd, const char* title) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p; if (!d) return;
    if (d->running) dbg_stop_proc(d);
    dbg_reset(d);
    d->active = 1;
    d->focused = 1;
    snprintf(d->title, sizeof(d->title), "%s", title ? title : "");
    dbg_log(d, "CSSC Debugger  -  memtrace session started", DBG_COL_VAL);

    char full[4300];
    snprintf(full, sizeof(full), "cmd.exe /d /c cssc %s", cmd ? cmd : "");
    dbg_spawn(d, full);
}

CSSC_GUI_EXPORT int64_t cssc_gui_debugger_update(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active) return 0;
    dbg_poll(d);

    if (d->screen) d->screen->cssc_modal = d->focused ? 1 : 0;
    return 1;
}

CSSC_GUI_EXPORT int64_t cssc_gui_debugger_active(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    return (d && d->active) ? 1 : 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_debugger_focused(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    return (d && d->active && d->focused) ? 1 : 0;
}

CSSC_GUI_EXPORT void cssc_gui_debugger_click(void* p, int64_t mx, int64_t my, int64_t clicked) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active || !clicked || !d->screen) return;
    int64_t x, y, w, h;
    dbg_rect(d, d->screen->w, d->screen->h, &x, &y, &w, &h);
    d->focused = (mx >= x && mx < x + w && my >= y && my < y + h) ? 1 : 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_debugger_ipline(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active || !d->ip_follow) return 0;
    return (int64_t)d->ip_line;
}

CSSC_GUI_EXPORT void cssc_gui_debugger_close(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p; if (!d) return;
    dbg_stop_proc(d);
    d->active = 0;
    if (d->screen) d->screen->cssc_modal = 0;
}

static void dbg_handle_vk(cssc_gui_debugger* d, int vk) {
    if (vk == 27) {
        dbg_stop_proc(d);
        d->active = 0;
        if (d->screen) d->screen->cssc_modal = 0;
        return;
    }
    if (vk == 112) { d->ip_follow = d->ip_follow ? 0 : 1; return; }
    if (vk == 9)  { d->tab = d->tab ? 0 : 1; return; }
    if (vk == 8)  { if (d->probe_len > 0) d->probe[--d->probe_len] = 0; return; }
    if (vk == 13) {
        if (d->probe_len > 0) { d->probe[d->probe_len] = 0; dbg_send_read(d, d->probe); }
        return;
    }
    if (vk == 38) {
        if (d->tab == 0) { if (d->alloc_top > 0) d->alloc_top--; }
        else { if (d->log_top > 0) d->log_top--; }
        return;
    }
    if (vk == 40) {
        if (d->tab == 0) d->alloc_top++; else d->log_top++;
        return;
    }
}

CSSC_GUI_EXPORT void cssc_gui_debugger_key(void* p, int64_t vk) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p; if (!d || !d->active) return;
    dbg_handle_vk(d, (int)vk);
}

CSSC_GUI_EXPORT void cssc_gui_debugger_char(void* p, int64_t chi) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p; if (!d || !d->active) return;
    int c = (int)chi;
    if (c == 27 || c == 13 || c == 8 || c == 9) { dbg_handle_vk(d, c); return; }

    int okc = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F') || c == 'x' || c == 'X';
    if (!okc) return;
    if (d->probe_len < (int)sizeof(d->probe) - 1) {
        d->probe[d->probe_len++] = (char)c;
        d->probe[d->probe_len] = 0;
    }
}

CSSC_GUI_EXPORT void cssc_gui_debugger_draw(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active || !d->screen) return;
    void* vid = d->screen->vid;
    int64_t W = d->screen->w, H = d->screen->h;
    int64_t px, py, pw, ph;
    dbg_rect(d, W, H, &px, &py, &pw, &ph);

    if (!d->focused) {

        int64_t sch = 8;
        cssc_video_fillrect(vid, px, py, pw, ph, DBG_COL_BG);
        cssc_video_fillrect(vid, px, py, pw, 2, (int64_t)0x60FFFFFF);
        cssc_video_draw_rect(vid, px, py, pw, ph, (int64_t)0x40FFFFFF);
        int64_t scy = py + 8;
        cssc_video_draw_text(vid, px + 10, scy, "CSSC DEBUGGER", (int64_t)0xFFE49BFF, 2);
        scy += 22;
        const char* sst = d->crashed ? "CRASHED" : (d->ended ? "ended"
                          : (d->running ? "running" : "idle"));
        char m0[128];
        snprintf(m0, sizeof(m0), "[%s]   allocs %d", sst, d->nallocs);
        cssc_video_draw_text(vid, px + 10, scy, m0,
                             d->crashed ? DBG_COL_ERR : DBG_COL_ADDR, 1);
        scy += sch + 6;
        char m1[96];
        snprintf(m1, sizeof(m1), "IP line %d  %s", d->ip_line,
                 d->ip_scope[0] ? d->ip_scope : "-");
        cssc_video_draw_text(vid, px + 10, scy, m1, DBG_COL_HEAD, 1);
        scy += sch + 6;
        cssc_video_draw_text(vid, px + 10, scy, "click to expand", DBG_COL_ADDR, 1);
        return;
    }

    int64_t sc = d->scale > 0 ? d->scale : 2;
    int64_t ch = 8 * sc, cw = 8 * sc;

    cssc_video_fillrect(vid, 0, 0, W, H, (int64_t)0xC8000000);
    cssc_video_fillrect(vid, px, py, pw, ph, DBG_COL_BG);
    cssc_video_fillrect(vid, px, py, pw, 2, (int64_t)0x60FFFFFF);
    cssc_video_draw_rect(vid, px, py, pw, ph, (int64_t)0x40FFFFFF);

    int64_t cy = py + 14;
    cssc_video_draw_text(vid, px + 16, cy, "CSSC  DEBUGGER", (int64_t)0xFFE49BFF, sc + 1);
    const char* st = d->crashed ? "CRASHED" : (d->ended ? "ended"
                     : (d->running ? "running" : "idle"));
    char sb[600];
    snprintf(sb, sizeof(sb), "%s   [%s]", d->title, st);
    cssc_video_draw_text(vid, px + 16 + 15 * (8 * (sc + 1)), cy, sb,
                         d->crashed ? DBG_COL_ERR : DBG_COL_ADDR, sc);
    cy += 8 * (sc + 1) + 12;

    char ipbuf[160];
    snprintf(ipbuf, sizeof(ipbuf), "IP   line %d    scope %s",
             d->ip_line, d->ip_scope[0] ? d->ip_scope : "-");
    cssc_video_draw_text(vid, px + 16, cy, ipbuf, DBG_COL_HEAD, sc);
    const char* mode = d->ip_follow ? "F1: FOLLOW IP" : "F1: FREE CAM";
    cssc_video_draw_text(vid, px + pw - 16 - (int64_t)strlen(mode) * cw, cy, mode,
                         d->ip_follow ? DBG_COL_VAL : DBG_COL_ADDR, sc);
    cy += ch + 8;

    if (d->crashed) {
        char cb[340];
        snprintf(cb, sizeof(cb), "CRASH  line %d:  %s", d->crash_line, d->crash_msg);
        cssc_video_fillrect(vid, px + 8, cy - 2, pw - 16, ch + 6, (int64_t)0x30FF5FB0);
        cssc_video_draw_text(vid, px + 16, cy, cb, DBG_COL_ERR, sc);
        cy += ch + 10;
    }

    char t0[48];
    snprintf(t0, sizeof(t0), "[ Allocations %d ]", d->nallocs);
    const char* t1 = "[ Output ]";
    cssc_video_draw_text(vid, px + 16, cy, t0, d->tab == 0 ? DBG_COL_NAME : DBG_COL_ADDR, sc);
    cssc_video_draw_text(vid, px + 16 + (int64_t)strlen(t0) * cw + 24, cy, t1,
                         d->tab == 1 ? DBG_COL_NAME : DBG_COL_ADDR, sc);
    cy += ch + 6;
    cssc_video_fillrect(vid, px + 8, cy, pw - 16, 1, (int64_t)0x30FFFFFF);
    cy += 8;

    int64_t content_y = cy;
    int64_t footer_h = ch * 3 + 30;
    int64_t content_h = (py + ph) - content_y - footer_h;
    int64_t rowh = ch + 6;
    int64_t rows = content_h > 0 ? content_h / rowh : 1;
    if (rows < 1) rows = 1;

    if (d->tab == 0) {
        int64_t maxtop = (int64_t)d->nallocs - (rows - 1);
        if (maxtop < 0) maxtop = 0;
        if (d->alloc_top > maxtop) d->alloc_top = (int)maxtop;
        if (d->alloc_top < 0) d->alloc_top = 0;
        int64_t hx = px + 16;
        cssc_video_draw_text(vid, hx,             content_y, "NAME",    DBG_COL_HEAD, sc);
        cssc_video_draw_text(vid, hx + 14 * cw,   content_y, "REGION",  DBG_COL_HEAD, sc);
        cssc_video_draw_text(vid, hx + 22 * cw,   content_y, "ADDRESS", DBG_COL_HEAD, sc);
        cssc_video_draw_text(vid, hx + 36 * cw,   content_y, "TYPE",    DBG_COL_HEAD, sc);
        cssc_video_draw_text(vid, hx + 45 * cw,   content_y, "VALUE",   DBG_COL_HEAD, sc);
        int64_t ry = content_y + rowh;
        for (int64_t r = 1; r < rows; r++) {
            int idx = d->alloc_top + (int)(r - 1);
            if (idx >= d->nallocs) break;
            dbg_alloc* a = &d->allocs[idx];
            int64_t nc = a->live ? DBG_COL_NAME : DBG_COL_DEAD;
            int64_t mc = a->live ? DBG_COL_ADDR : DBG_COL_DEAD;
            int64_t tc = a->live ? DBG_COL_TYPE : DBG_COL_DEAD;
            int64_t vc = a->live ? DBG_COL_VAL  : DBG_COL_DEAD;
            char ab[24]; snprintf(ab, sizeof(ab), "0x%llx", (unsigned long long)a->addr);
            char vb[64]; snprintf(vb, sizeof(vb), "%.40s", a->value);
            cssc_video_draw_text(vid, hx,           ry, a->name,   nc, sc);
            cssc_video_draw_text(vid, hx + 14 * cw, ry, a->region, mc, sc);
            cssc_video_draw_text(vid, hx + 22 * cw, ry, ab,        mc, sc);
            cssc_video_draw_text(vid, hx + 36 * cw, ry, a->type,   tc, sc);
            cssc_video_draw_text(vid, hx + 45 * cw, ry, vb,        vc, sc);
            ry += rowh;
        }
    } else {
        int64_t maxtop = (int64_t)d->nlog - rows;
        if (maxtop < 0) maxtop = 0;
        if (d->log_top > maxtop) d->log_top = (int)maxtop;
        if (d->log_top < 0) d->log_top = 0;
        int64_t ry = content_y;
        for (int64_t r = 0; r < rows; r++) {
            int idx = d->log_top + (int)r;
            if (idx >= d->nlog) break;
            cssc_video_draw_text(vid, px + 16, ry, d->log[idx], d->log_col[idx], sc);
            ry += rowh;
        }
    }

    int64_t by = py + ph - footer_h + 6;
    cssc_video_fillrect(vid, px + 8, by, pw - 16, ch + 8, (int64_t)0x30101820);
    char pbuf[120];
    snprintf(pbuf, sizeof(pbuf), "probe>  %s_", d->probe);
    cssc_video_draw_text(vid, px + 16, by + 4, pbuf, DBG_COL_VAL, sc);
    if (d->last_read[0])
        cssc_video_draw_text(vid, px + 16, by + rowh + 4, d->last_read, DBG_COL_READ, sc);
    cssc_video_draw_text(vid, px + 16, py + ph - ch - 6,
        "Tab: switch tab   F1: follow / free-cam   type hex + Enter: probe address   Esc: close",
        DBG_COL_ADDR, sc >= 2 ? sc - 1 : 1);
}

CSSC_GUI_EXPORT void* cssc_gui_menu_new(void* screen) {
    cssc_gui_menu* m = (cssc_gui_menu*)calloc(1, sizeof(cssc_gui_menu));
    if (!m) return NULL;
    m->kind = GW_MENU;
    m->screen = (cssc_gui_screen*)screen;
    m->x = 0; m->y = 0; m->w = 1220; m->h = 30;
    m->scale = 2;
    m->bg = (int64_t)0xFF1A1424;
    m->fg = (int64_t)0xFFCFC8E0;
    m->visible = 1;
    return m;
}
CSSC_GUI_EXPORT void cssc_gui_menu_setrect(void* p, int64_t x, int64_t y,
                                           int64_t w, int64_t h) {
    cssc_gui_menu* m = (cssc_gui_menu*)p;
    if (m) { m->x = x; m->y = y; m->w = w; m->h = h; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_menu_addmenu(void* p, const char* title) {
    cssc_gui_menu* m = (cssc_gui_menu*)p;
    if (!m || m->n_titles >= 16) return 0;
    m->titles[m->n_titles] = gui_strdup(title ? title : "");
    m->n_titles++;
    return m->n_titles;
}
CSSC_GUI_EXPORT void cssc_gui_menu_additem(void* p, int64_t menu1,
                                           const char* label, int64_t action) {
    cssc_gui_menu* m = (cssc_gui_menu*)p;
    if (!m || m->n_items >= 128) return;
    m->item_label[m->n_items] = gui_strdup(label ? label : "");
    m->item_menu[m->n_items] = (int)menu1;
    m->item_action[m->n_items] = action;
    m->n_items++;
}
CSSC_GUI_EXPORT void cssc_gui_menu_setscale(void* p, int64_t s) {
    cssc_gui_menu* m = (cssc_gui_menu*)p; if (m) m->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_menu_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_menu* m = (cssc_gui_menu*)p;
    if (m) { m->bg = bg & 0xFFFFFFFF; m->fg = fg & 0xFFFFFFFF; }
}

CSSC_GUI_EXPORT void cssc_gui_menu_setrighttext(void* p, const char* text) {
    cssc_gui_menu* m = (cssc_gui_menu*)p;
    if (!m) return;
    if (m->right_text) { free(m->right_text); m->right_text = NULL; }
    if (text && text[0]) m->right_text = gui_strdup(text);
    if (m->right_color == 0) m->right_color = (int64_t)0xFF8FA6B8;
}
CSSC_GUI_EXPORT void cssc_gui_menu_setrightcolor(void* p, int64_t argb) {
    cssc_gui_menu* m = (cssc_gui_menu*)p; if (m) m->right_color = argb;
}
CSSC_GUI_EXPORT int64_t cssc_gui_menu_isopen(void* p) {
    cssc_gui_menu* m = (cssc_gui_menu*)p; return m ? (int64_t)(m->open != 0) : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_menu_action(void* p) {
    cssc_gui_menu* m = (cssc_gui_menu*)p; return m ? m->last_action : 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_menu_update(void* mp, void* sp) {
    cssc_gui_menu* m = (cssc_gui_menu*)mp;
    if (!m) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : m->screen;
    if (!s) return 0;
    m->last_action = 0;
    if (s->input_captured) return 0;
    if (!(s->down && !s->prev_down)) return 0;
    int64_t mx = s->mx, my = s->my;
    int64_t glyph = 8 * m->scale;
    int64_t row_h = glyph + 10;

    if (my >= m->y && my < m->y + m->h) {
        for (int i = 0; i < m->n_titles; i++) {
            int64_t tx = menu_title_x(m, i), tw = menu_title_w(m, i);
            if (mx >= tx - 8 && mx < tx + tw - 8) {
                m->open = (m->open == i + 1) ? 0 : (i + 1);
                return 0;
            }
        }
        m->open = 0;
        return 0;
    }

    if (m->open) {
        int64_t dx = menu_title_x(m, m->open - 1) - 8;
        int64_t dw = menu_dropdown_w(m, m->open);
        int64_t dy = m->y + m->h;
        int row = 0;
        for (int i = 0; i < m->n_items; i++) {
            if (m->item_menu[i] != m->open) continue;
            int64_t ry = dy + (int64_t)row * row_h;
            if (mx >= dx && mx < dx + dw && my >= ry && my < ry + row_h) {
                m->last_action = m->item_action[i];
                m->open = 0;
                return 0;
            }
            row++;
        }
        m->open = 0;
    }
    return 0;
}

CSSC_GUI_EXPORT void cssc_gui_menu_draw(void* mp) {
    cssc_gui_menu* m = (cssc_gui_menu*)mp;
    if (!m || !m->visible || !m->screen) return;
    void* v = m->screen->vid;
    int64_t glyph = 8 * m->scale;
    cssc_video_fillrect(v, m->x, m->y, m->w, m->h, m->bg);
    for (int i = 0; i < m->n_titles; i++) {
        int64_t tx = menu_title_x(m, i), tw = menu_title_w(m, i);
        if (m->open == i + 1)
            cssc_video_fillrect(v, tx - 8, m->y, tw, m->h, (int64_t)0x60C74DE0);
        cssc_video_draw_text(v, tx, m->y + (m->h - glyph) / 2, m->titles[i],
                             m->fg, m->scale);
    }
    if (m->right_text && m->right_text[0]) {
        int64_t tw = (int64_t)strlen(m->right_text) * glyph;
        int64_t rx = m->x + m->w - tw - 12;
        cssc_video_draw_text(v, rx, m->y + (m->h - glyph) / 2, m->right_text,
                             m->right_color ? m->right_color : (int64_t)0xFF8FA6B8, m->scale);
    }
    if (m->open) {
        int64_t row_h = glyph + 10;
        int64_t dx = menu_title_x(m, m->open - 1) - 8;
        int64_t dw = menu_dropdown_w(m, m->open);
        int64_t dy = m->y + m->h;
        int count = 0;
        for (int i = 0; i < m->n_items; i++) if (m->item_menu[i] == m->open) count++;
        int64_t dh = (int64_t)count * row_h + 6;
        cssc_video_fillrect(v, dx, dy, dw, dh, (int64_t)0xF01A2230);
        cssc_video_draw_rect(v, dx, dy, dw, dh, (int64_t)0xC0C74DE0);
        int row = 0;
        for (int i = 0; i < m->n_items; i++) {
            if (m->item_menu[i] != m->open) continue;
            int64_t ry = dy + 3 + (int64_t)row * row_h;
            cssc_video_draw_text(v, dx + 14, ry + 5, m->item_label[i], m->fg, m->scale);
            row++;
        }
    }
}

CSSC_GUI_EXPORT void* cssc_gui_prompt_new(void* screen) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)calloc(1, sizeof(cssc_gui_prompt));
    if (!p) return NULL;
    p->kind = GW_PROMPT;
    p->screen = (cssc_gui_screen*)screen;
    p->scale = 2;
    p->bg = (int64_t)0xF01A2230;
    p->fg = (int64_t)0xFFE6E2F0;
    p->visible = 1;
    return p;
}
CSSC_GUI_EXPORT void cssc_gui_prompt_open(void* pp, const char* label) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp;
    if (!p) return;
    const char* l = label ? label : "";
    size_t n = strlen(l); if (n >= sizeof(p->label)) n = sizeof(p->label) - 1;
    memcpy(p->label, l, n); p->label[n] = 0;
    p->input[0] = 0; p->input_len = 0; p->input_cur = 0;
    p->active = 1; p->result = 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_prompt_isopen(void* pp) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp; return p ? (int64_t)p->active : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_prompt_result(void* pp) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp;
    if (!p) return 0;
    int r = p->result; p->result = 0; return r;
}
CSSC_GUI_EXPORT void* cssc_gui_prompt_text(void* pp) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp;
    if (!p) return cssc_string_lit("", 0);
    return cssc_string_lit(p->input, strlen(p->input));
}
CSSC_GUI_EXPORT void cssc_gui_prompt_setscale(void* pp, int64_t s) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp; if (p) p->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_prompt_setcolor(void* pp, int64_t bg, int64_t fg) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp;
    if (p) { p->bg = bg & 0xFFFFFFFF; p->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_prompt_update(void* pp, void* sp) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp;
    if (!p || !p->active) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : p->screen;
    if (!s) return 0;
    int64_t c;
    while ((c = cssc_video_poll_char(s->vid)) != 0) {
        if (c == 13 || c == 10) { p->result = 1; p->active = 0; }
        else if (c == 27) { p->result = 2; p->active = 0; }
        else if (c == 8) {
            if (p->input_cur > 0) {
                memmove(p->input + p->input_cur - 1, p->input + p->input_cur,
                        (size_t)(p->input_len - p->input_cur) + 1);
                p->input_cur--; p->input_len--;
            }
        } else if (c >= 32 && c < 127 && p->input_len < (int)sizeof(p->input) - 1) {
            memmove(p->input + p->input_cur + 1, p->input + p->input_cur,
                    (size_t)(p->input_len - p->input_cur) + 1);
            p->input[p->input_cur] = (char)c; p->input_cur++; p->input_len++;
        }
    }
    int64_t k;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        if (k == 0x25 && p->input_cur > 0) p->input_cur--;
        else if (k == 0x27 && p->input_cur < p->input_len) p->input_cur++;
        else if (k == 0x24) p->input_cur = 0;
        else if (k == 0x23) p->input_cur = p->input_len;
    }
    return 0;
}
CSSC_GUI_EXPORT void cssc_gui_prompt_draw(void* pp) {
    cssc_gui_prompt* p = (cssc_gui_prompt*)pp;
    if (!p || !p->visible || !p->active || !p->screen) return;
    void* v = p->screen->vid;
    int64_t W = p->screen->w, H = p->screen->h;
    cssc_video_fillrect(v, 0, 0, W, H, (int64_t)0x90000000);
    int64_t bw = 620, bh = 132;
    int64_t bx = (W - bw) / 2, by = (H - bh) / 2;
    cssc_video_fillrect(v, bx, by, bw, bh, p->bg);
    cssc_video_draw_rect(v, bx, by, bw, bh, (int64_t)0xC0C74DE0);
    int64_t glyph = 8 * p->scale;
    cssc_video_draw_text(v, bx + 18, by + 16, p->label, (int64_t)0xFFC74DE0, p->scale);
    int64_t fx = bx + 18, fy = by + 52, fw = bw - 36, fh = glyph + 12;
    cssc_video_fillrect(v, fx, fy, fw, fh, (int64_t)0xFF0B0910);
    cssc_video_draw_rect(v, fx, fy, fw, fh, (int64_t)0x60FFFFFF);
    int max_cols = (int)((fw - 12) / glyph); if (max_cols < 1) max_cols = 1;
    int ifrom = p->input_cur > max_cols ? p->input_cur - max_cols : 0;
    int icnt = p->input_len - ifrom; if (icnt > max_cols) icnt = max_cols;
    char sub[512];
    if (icnt > 0 && icnt < 512) { memcpy(sub, p->input + ifrom, (size_t)icnt); sub[icnt] = 0; }
    else sub[0] = 0;
    cssc_video_draw_text(v, fx + 6, fy + 6, sub, p->fg, p->scale);
    int64_t cx = fx + 6 + (int64_t)(p->input_cur - ifrom) * glyph;
    cssc_video_fillrect(v, cx, fy + 6, 2, glyph, (int64_t)0xFFC74DE0);
    cssc_video_draw_text(v, bx + 18, by + bh - 24, "Enter = OK    Esc = Cancel",
                         (int64_t)0xFF8A84A0, 1);
}

static const char* tabs_base(const char* p) {
    const char* b = strrchr(p, '/');
    return b ? b + 1 : p;
}
static int64_t tabs_w(cssc_gui_tabs* t, int i) {
    int64_t glyph = 8 * t->scale;
    return (int64_t)strlen(tabs_base(t->paths[i])) * glyph + 44;
}
CSSC_GUI_EXPORT void* cssc_gui_tabs_new(void* screen) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)calloc(1, sizeof(cssc_gui_tabs));
    if (!t) return NULL;
    t->kind = GW_TABS;
    t->screen = (cssc_gui_screen*)screen;
    t->x = 0; t->y = 0; t->w = 1220; t->h = 28;
    t->scale = 2; t->active = -1;
    t->bg = (int64_t)0xFF141019;
    t->fg = (int64_t)0xFFB8C0CC;
    t->visible = 1;
    return t;
}
CSSC_GUI_EXPORT void cssc_gui_tabs_setrect(void* p, int64_t x, int64_t y,
                                           int64_t w, int64_t h) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; if (t) { t->x = x; t->y = y; t->w = w; t->h = h; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_indexof(void* p, const char* path) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p;
    if (!t || !path) return 0;
    for (int i = 0; i < t->n_tabs; i++) if (!strcmp(t->paths[i], path)) return i + 1;
    return 0;
}
CSSC_GUI_EXPORT void cssc_gui_tabs_add(void* p, const char* path) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p;
    if (!t || !path || !path[0]) return;
    for (int i = 0; i < t->n_tabs; i++)
        if (!strcmp(t->paths[i], path)) { t->active = i; return; }
    if (t->n_tabs >= 64) {
        free(t->paths[0]);
        for (int i = 0; i < t->n_tabs - 1; i++) t->paths[i] = t->paths[i + 1];
        t->n_tabs--;
    }
    t->paths[t->n_tabs] = gui_strdup(path);
    t->active = t->n_tabs;
    t->n_tabs++;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_count(void* p) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; return t ? (int64_t)t->n_tabs : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_active(void* p) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p;
    return (t && t->n_tabs && t->active >= 0) ? (int64_t)(t->active + 1) : 0;
}
CSSC_GUI_EXPORT void* cssc_gui_tabs_activepath(void* p) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p;
    if (!t || t->active < 0 || t->active >= t->n_tabs) return cssc_string_lit("", 0);
    return cssc_string_lit(t->paths[t->active], strlen(t->paths[t->active]));
}
CSSC_GUI_EXPORT void* cssc_gui_tabs_pathof(void* p, int64_t i) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; int idx = (int)i - 1;
    if (!t || idx < 0 || idx >= t->n_tabs) return cssc_string_lit("", 0);
    return cssc_string_lit(t->paths[idx], strlen(t->paths[idx]));
}
CSSC_GUI_EXPORT void cssc_gui_tabs_setactive(void* p, int64_t i) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; if (!t) return;
    int idx = (int)i - 1;
    if (idx >= 0 && idx < t->n_tabs) t->active = idx;
}
CSSC_GUI_EXPORT void cssc_gui_tabs_remove(void* p, int64_t i) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; if (!t) return;
    int idx = (int)i - 1;
    if (idx < 0 || idx >= t->n_tabs) return;
    free(t->paths[idx]);
    for (int k = idx; k < t->n_tabs - 1; k++) t->paths[k] = t->paths[k + 1];
    t->n_tabs--;
    if (t->active >= t->n_tabs) t->active = t->n_tabs - 1;
    else if (t->active > idx) t->active--;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_clicked(void* p) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; return t ? t->switch_hit : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_closerequested(void* p) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; return t ? t->close_hit : 0;
}
CSSC_GUI_EXPORT void cssc_gui_tabs_setscale(void* p, int64_t s) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p; if (t) t->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_tabs_setcolor(void* p, int64_t bg, int64_t fg) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)p;
    if (t) { t->bg = bg & 0xFFFFFFFF; t->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT int64_t cssc_gui_tabs_update(void* tp, void* sp) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)tp; if (!t) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : t->screen; if (!s) return 0;
    t->switch_hit = 0; t->close_hit = 0;
    if (s->input_captured) return 0;
    if (!(s->down && !s->prev_down)) return 0;
    if (s->my < t->y || s->my >= t->y + t->h) return 0;
    int64_t tx = t->x;
    for (int i = 0; i < t->n_tabs; i++) {
        int64_t tw = tabs_w(t, i);
        if (s->mx >= tx && s->mx < tx + tw) {
            if (s->mx >= tx + tw - 22) t->close_hit = i + 1;
            else t->switch_hit = i + 1;
            return 0;
        }
        tx += tw;
    }
    return 0;
}
CSSC_GUI_EXPORT void cssc_gui_tabs_draw(void* tp) {
    cssc_gui_tabs* t = (cssc_gui_tabs*)tp;
    if (!t || !t->visible || !t->screen) return;
    void* v = t->screen->vid;
    int64_t glyph = 8 * t->scale;
    cssc_video_fillrect(v, t->x, t->y, t->w, t->h, t->bg);
    int64_t tx = t->x;
    for (int i = 0; i < t->n_tabs; i++) {
        int64_t tw = tabs_w(t, i);
        if (tx >= t->x + t->w) break;
        int act = (i == t->active);
        cssc_video_fillrect(v, tx, t->y, tw - 1, t->h,
                            act ? (int64_t)0xFF0B0910 : (int64_t)0xFF1A1424);
        if (act) cssc_video_fillrect(v, tx, t->y, tw - 1, 2, (int64_t)0xFFC74DE0);
        int64_t col = act ? (int64_t)0xFFFFFFFF : t->fg;
        cssc_video_draw_text(v, tx + 10, t->y + (t->h - glyph) / 2,
                             tabs_base(t->paths[i]), col, t->scale);
        cssc_video_draw_text(v, tx + tw - 18, t->y + (t->h - glyph) / 2, "x",
                             (int64_t)0xFF8A84A0, t->scale);
        tx += tw;
    }
}

static int browser_has_parent(const char* dir) {
    return (int)strlen(dir) > 3;
}
static void browser_goup(char* dir) {
    int n = (int)strlen(dir);
    while (n > 1 && (dir[n - 1] == '/' || dir[n - 1] == '\\')) dir[--n] = 0;

    char* pf = strrchr(dir, '/');
    char* pb = strrchr(dir, '\\');
    char* p = pf > pb ? pf : pb;
    if (!p) return;
    int idx = (int)(p - dir);
    if (idx == 0) { dir[1] = 0; return; }
    if (idx == 2 && dir[1] == ':') { dir[3] = 0; }
    else dir[idx] = 0;
}
static void browser_clear(cssc_gui_browser* b) {
    for (int i = 0; i < b->n; i++) free(b->names[i]);
    b->n = 0;
}
static void browser_addrow(cssc_gui_browser* b, const char* name, int isd) {
    if (b->n >= 512) return;
    b->names[b->n] = gui_strdup(name);
    b->isdir[b->n] = isd;
    b->n++;
}
static void browser_scan(cssc_gui_browser* b) {
    browser_clear(b);
    b->sel = 0; b->top = 0;
    if (browser_has_parent(b->dir)) browser_addrow(b, "..", 1);
    char** dn = NULL; int nd = 0, cd = 0;
    char** fn = NULL; int nf = 0, cf = 0;
#ifdef _WIN32
    char pat[560]; snprintf(pat, sizeof(pat), "%s\\*", b->dir);
    WIN32_FIND_DATAA fd; HANDLE h = FindFirstFileA(pat, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            const char* n = fd.cFileName;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) tree_push(&dn, &nd, &cd, n);
            else tree_push(&fn, &nf, &cf, n);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(b->dir);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            const char* n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            char full[560]; snprintf(full, sizeof(full), "%s/%s", b->dir, n);
            struct stat st;
            int isd = (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) ? 1 : 0;
            if (isd) tree_push(&dn, &nd, &cd, n); else tree_push(&fn, &nf, &cf, n);
        }
        closedir(d);
    }
#endif
    if (nd > 1) qsort(dn, nd, sizeof(char*), tree_namecmp);
    if (nf > 1) qsort(fn, nf, sizeof(char*), tree_namecmp);
    for (int i = 0; i < nd; i++) { browser_addrow(b, dn[i], 1); free(dn[i]); }
    for (int i = 0; i < nf; i++) { browser_addrow(b, fn[i], 0); free(fn[i]); }
    free(dn); free(fn);
}

static void browser_ensure_icons(cssc_gui_browser* b) {
    if (b->ico_loaded || !b->icondir[0]) return;
    if (b->ico_folder)  cssc_sprite_free(b->ico_folder);
    if (b->ico_cssc)    cssc_sprite_free(b->ico_cssc);
    if (b->ico_md)      cssc_sprite_free(b->ico_md);
    if (b->ico_ini)     cssc_sprite_free(b->ico_ini);
    if (b->ico_file)    cssc_sprite_free(b->ico_file);
    if (b->ico_exe)     cssc_sprite_free(b->ico_exe);
    if (b->ico_csscu)   cssc_sprite_free(b->ico_csscu);
    if (b->ico_project) cssc_sprite_free(b->ico_project);
    b->ico_folder  = tree_load_icon(b->icondir, "folder.png");
    b->ico_cssc    = tree_load_icon(b->icondir, "cssc_file.png");
    b->ico_md      = tree_load_icon(b->icondir, "markdown.png");
    b->ico_ini     = tree_load_icon(b->icondir, "ini.png");
    b->ico_file    = tree_load_icon(b->icondir, "file.png");
    b->ico_exe     = tree_load_icon(b->icondir, "exe.png");
    b->ico_csscu   = tree_load_icon(b->icondir, "csscu.png");
    b->ico_project = tree_load_icon(b->icondir, "cssc.project.png");
    b->ico_loaded = 1;
}
static void* browser_pick_icon(cssc_gui_browser* b, int idx) {
    if (b->isdir[idx]) return b->ico_folder;
    const char* nm = b->names[idx];
    if (b->ico_project && (!strcmp(nm, "cssc.proj") || !strcmp(nm, "cssc.cproject")))
        return b->ico_project;
    const char* dot = strrchr(nm, '.');
    if (dot) {
        if (!strcmp(dot, ".cssc")) return b->ico_cssc;
        if (!strcmp(dot, ".md"))   return b->ico_md;
        if (b->ico_exe && !strcmp(dot, ".exe"))     return b->ico_exe;
        if (b->ico_csscu && !strcmp(dot, ".csscu")) return b->ico_csscu;
        if (b->ico_project && (!strcmp(dot, ".proj") || !strcmp(dot, ".cproject")))
            return b->ico_project;
        if (!strcmp(dot, ".ini") || !strcmp(dot, ".toml") || !strcmp(dot, ".json"))
            return b->ico_ini;
    }
    return b->ico_file;
}
static void browser_enter(cssc_gui_browser* b, int idx) {
    if (idx < 0 || idx >= b->n) return;
    if (!strcmp(b->names[idx], "..")) { browser_goup(b->dir); browser_scan(b); return; }
    if (b->isdir[idx]) {
        char nd[600]; snprintf(nd, sizeof(nd), "%s/%s", b->dir, b->names[idx]);
        size_t nn = strlen(nd);
        if (nn < sizeof(b->dir)) { memcpy(b->dir, nd, nn); b->dir[nn] = 0; }
        browser_scan(b);
    }
}
CSSC_GUI_EXPORT void* cssc_gui_browser_new(void* screen) {
    cssc_gui_browser* b = (cssc_gui_browser*)calloc(1, sizeof(cssc_gui_browser));
    if (!b) return NULL;
    b->kind = GW_BROWSER;
    b->screen = (cssc_gui_screen*)screen;
    b->scale = 2;
    b->bg = (int64_t)0xF01A2230;
    b->fg = (int64_t)0xFFE6E2F0;
    b->visible = 1;
    return b;
}
CSSC_GUI_EXPORT void cssc_gui_browser_open(void* bp, const char* startdir, int64_t mode) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp;
    if (!b) return;
    const char* d = (startdir && startdir[0]) ? startdir : ".";
    size_t n = strlen(d); if (n >= sizeof(b->dir)) n = sizeof(b->dir) - 1;
    memcpy(b->dir, d, n); b->dir[n] = 0;
    b->mode = (int)mode; b->active = 1; b->result = 0; b->chosen[0] = 0;
    browser_scan(b);
}
CSSC_GUI_EXPORT int64_t cssc_gui_browser_isopen(void* bp) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp; return b ? (int64_t)b->active : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_browser_result(void* bp) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp;
    if (!b) return 0;
    int r = b->result; b->result = 0; return r;
}
CSSC_GUI_EXPORT void* cssc_gui_browser_chosen(void* bp) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp;
    if (!b) return cssc_string_lit("", 0);
    return cssc_string_lit(b->chosen, strlen(b->chosen));
}
CSSC_GUI_EXPORT void cssc_gui_browser_setscale(void* bp, int64_t s) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp; if (b) b->scale = s > 0 ? s : 1;
}
CSSC_GUI_EXPORT void cssc_gui_browser_setcolor(void* bp, int64_t bg, int64_t fg) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp;
    if (b) { b->bg = bg & 0xFFFFFFFF; b->fg = fg & 0xFFFFFFFF; }
}
CSSC_GUI_EXPORT void cssc_gui_browser_seticondir(void* bp, const char* dir) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp; if (!b) return;
    const char* d = dir ? dir : "";
    size_t n = strlen(d);
    if (n >= sizeof(b->icondir)) n = sizeof(b->icondir) - 1;
    memcpy(b->icondir, d, n); b->icondir[n] = 0;
    b->ico_loaded = 0;
}
static void browser_confirm(cssc_gui_browser* b) {
    if (b->mode == 1) {
        snprintf(b->chosen, sizeof(b->chosen), "%s", b->dir);
        b->result = 1; b->active = 0;
    } else if (b->sel >= 0 && b->sel < b->n && !b->isdir[b->sel]) {
        snprintf(b->chosen, sizeof(b->chosen), "%s/%s", b->dir, b->names[b->sel]);
        b->result = 1; b->active = 0;
    }
}
CSSC_GUI_EXPORT int64_t cssc_gui_browser_update(void* bp, void* sp) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp;
    if (!b || !b->active) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : b->screen;
    if (!s) return 0;
    int64_t glyph = 8 * b->scale;
    int64_t row_h = glyph + 6;
    int64_t W = s->w, H = s->h;
    int64_t bw = 760, bh = 520;
    int64_t bx = (W - bw) / 2, by = (H - bh) / 2;
    int64_t ly = by + 72, lh = bh - 72 - 56;
    int vis = (int)(lh / row_h); if (vis < 1) vis = 1;
    int64_t c;
    while ((c = cssc_video_poll_char(s->vid)) != 0) {
        if (c == 27) { b->result = 2; b->active = 0; return 0; }
        if (c == 13) {
            if (b->sel >= 0 && b->sel < b->n &&
                (b->isdir[b->sel] || !strcmp(b->names[b->sel], "..")))
                browser_enter(b, b->sel);
            else
                browser_confirm(b);
            return 0;
        }
    }
    int64_t k;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        if (k == 0x26 && b->sel > 0) b->sel--;
        else if (k == 0x28 && b->sel < b->n - 1) b->sel++;
    }
    int64_t wh = cssc_video_wheel(s->vid);
    if (wh) { b->top -= (int)wh * 3; if (b->top < 0) b->top = 0; }
    if (s->down && !s->prev_down) {
        int64_t mx = s->mx, my = s->my;
        int64_t byb = by + bh - 44;
        if (my >= byb && my < byb + 32) {
            if (mx >= bx + bw - 150 && mx < bx + bw - 16) browser_confirm(b);
            else if (mx >= bx + bw - 300 && mx < bx + bw - 160) { b->result = 2; b->active = 0; }
            return 0;
        }
        if (mx >= bx + 16 && mx < bx + bw - 16 && my >= ly && my < ly + lh) {
            int idx = b->top + (int)((my - ly) / row_h);
            if (idx >= 0 && idx < b->n) {
                b->sel = idx;
                if (b->isdir[idx]) browser_enter(b, idx);
            }
            return 0;
        }
    }
    if (b->sel < b->top) b->top = b->sel;
    if (b->sel >= b->top + vis) b->top = b->sel - vis + 1;
    if (b->top < 0) b->top = 0;
    return 0;
}
CSSC_GUI_EXPORT void cssc_gui_browser_draw(void* bp) {
    cssc_gui_browser* b = (cssc_gui_browser*)bp;
    if (!b || !b->visible || !b->active || !b->screen) return;
    void* v = b->screen->vid;
    int64_t W = b->screen->w, H = b->screen->h;
    cssc_video_fillrect(v, 0, 0, W, H, (int64_t)0x90000000);
    int64_t glyph = 8 * b->scale;
    int64_t row_h = glyph + 6;
    int64_t bw = 760, bh = 520;
    int64_t bx = (W - bw) / 2, by = (H - bh) / 2;
    cssc_video_fillrect(v, bx, by, bw, bh, b->bg);
    cssc_video_draw_rect(v, bx, by, bw, bh, (int64_t)0xC0C74DE0);
    cssc_video_draw_text(v, bx + 16, by + 14,
                         b->mode == 1 ? "Open Folder" : "Open File",
                         (int64_t)0xFFC74DE0, b->scale);
    cssc_video_draw_text(v, bx + 16, by + 44, b->dir, (int64_t)0xFF8A94A6, 1);
    int64_t ly = by + 72, lw = bw - 32, lh = bh - 72 - 56;
    cssc_video_fillrect(v, bx + 16, ly, lw, lh, (int64_t)0xFF0B0910);
    int vis = (int)(lh / row_h); if (vis < 1) vis = 1;
    int maxc = (int)((lw - 44) / glyph); if (maxc < 1) maxc = 1; if (maxc > 200) maxc = 200;
    char sub[256];
    browser_ensure_icons(b);
    for (int r = 0; r < vis; r++) {
        int idx = b->top + r;
        if (idx >= b->n) break;
        int64_t ry = ly + (int64_t)r * row_h;
        if (idx == b->sel) cssc_video_fillrect(v, bx + 16, ry, lw, row_h, (int64_t)0x60C74DE0);
        void* icon = b->ico_loaded ? browser_pick_icon(b, idx) : NULL;
        if (icon) {
            gui_blit_sprite(v, icon, bx + 24, ry + (row_h - glyph) / 2, b->scale);
        } else {
            int64_t isz = glyph / 2; if (isz < 4) isz = 4;
            int64_t icol = b->isdir[idx] ? (int64_t)0xFFE0B24B : list_icon_color(b->names[idx]);
            cssc_video_fillrect(v, bx + 24, ry + (row_h - isz) / 2, isz, isz, icol);
        }
        int nlen = (int)strlen(b->names[idx]); if (nlen > maxc) nlen = maxc;
        memcpy(sub, b->names[idx], (size_t)nlen); sub[nlen] = 0;
        cssc_video_draw_text(v, bx + 24 + glyph + 2, ry + 3, sub, (int64_t)0xFFE6E2F0, b->scale);
    }
    int64_t byb = by + bh - 44;
    cssc_video_fillrect(v, bx + bw - 300, byb, 140, 32, (int64_t)0xFF2A2233);
    cssc_video_draw_rect(v, bx + bw - 300, byb, 140, 32, (int64_t)0x60FFFFFF);
    cssc_video_draw_text(v, bx + bw - 300 + 22, byb + 8, "Cancel", (int64_t)0xFFB8C0CC, b->scale);
    cssc_video_fillrect(v, bx + bw - 150, byb, 134, 32, (int64_t)0x60C74DE0);
    cssc_video_draw_rect(v, bx + bw - 150, byb, 134, 32, (int64_t)0xC0C74DE0);
    cssc_video_draw_text(v, bx + bw - 150 + 18, byb + 8,
                         b->mode == 1 ? "Select" : "Open", (int64_t)0xFFFFFFFF, b->scale);
}
