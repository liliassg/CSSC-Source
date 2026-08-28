/*
 * cssc_host_gui.c — native runtime for the CSSC gui:: retro-glass framework.
 *
 * Mirrors the interpreter (cssl_gui_native.py) method-for-method so a compiled
 * GUI renders pixel-identically to an interpreted one:
 *   - Glassy panels go through cssc_video_fillrect (host_extras), whose /255
 *     alpha compositor equals the interpreter's _blend.
 *   - Text goes through cssc_video_draw_text (host_extras 8x8 ROM), equal to
 *     the interpreter's _text.
 *   - Screens wrap a cssc_video window; opening one plays the CSSC watermark
 *     (inside cssc_video_begin), exactly like every other CSSC window.
 *
 * Handles are raw pointers threaded through typed slots by the compiler; each
 * widget struct starts with an `int kind` tag so a Screen can render/update a
 * heterogeneous child list.
 */
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

/* Provided by cssc_host_game.c (window) + cssc_host_extras.c (draw). */
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
extern void    cssc_video_draw_text_styled(void* v, int64_t x, int64_t y,
                                    const char* text, int64_t argb, int64_t scale,
                                    int64_t style);
extern int64_t cssc_video_poll_char(void* v);
extern int64_t cssc_video_poll_key(void* v);
extern int64_t cssc_video_wheel(void* v);
/* Provided by the transembly runtime.ll. */
extern void* cssc_string_lit(const char* data, size_t len);
/* Provided by cssc_host_game.c — sprite loader (BMP + PNG) + pixel access,
 * reused for the file-tree's per-type 8x8 icons. We read pixels and paint them
 * through cssc_video_fillrect (the video's own compositor) rather than
 * cssc_sprite_draw*, because those target a cssc_framebuffer_t whose pixel
 * pointer sits at a different offset than the live video's backbuffer. */
extern void*   cssc_sprite_load(const char* path);
extern int64_t cssc_sprite_width(void* p);
extern int64_t cssc_sprite_height(void* p);
extern int64_t cssc_sprite_get_pixel(void* p, int64_t x, int64_t y);
extern void    cssc_sprite_free(void* p);

enum { GW_TEXT = 1, GW_BUTTON = 2, GW_TOOLBAR = 3, GW_TEXTBOX = 4,
       GW_EDITOR = 5, GW_LIST = 6, GW_TREE = 7, GW_TERMINAL = 8, GW_MENU = 9,
       GW_PROMPT = 10, GW_TABS = 11, GW_BROWSER = 12, GW_DEBUGGER = 13 };

typedef struct cssc_gui_screen {
    void*    vid;                 /* cssc_video_t* */
    int64_t  w, h;
    int64_t  mx, my, down, prev_down, rdown, prev_rdown;
    int64_t  ctrl, shift, alt;    /* live modifier-key state */
    int64_t  input_captured;      /* an open overlay (menu) owns this frame's click */
    int64_t  cssc_modal;          /* a host-drawn overlay (e.g. color picker) owns input */
    int64_t  hk_prev_b, hk_prev_j; /* Ctrl+B / Ctrl+J edge state for hotkey() */
    int64_t  hk_prev_caret;        /* Alt+^ edge state for hotkey() (color picker) */
    int64_t  dk_prev_plus, dk_prev_minus, dk_prev_hash; /* + / - / # edge state for dbgKey() */
    int64_t  hk_prev_f1, hk_prev_f2, hk_prev_f5, hk_prev_f6, hk_prev_f9, hk_prev_f10, hk_prev_f11; /* F-key edge state for funcKey() */
    void**   widgets;
    int      n_widgets, cap_widgets;
    int64_t  last_cb;
    int64_t  fs_on;                /* 1 while borderless-fullscreen */
    int64_t  fs_style;             /* saved GWL_STYLE to restore */
    int64_t  fs_rx, fs_ry, fs_rw, fs_rh; /* saved windowed rect */
    int64_t  fs_vw, fs_vh;         /* saved backbuffer dims to restore */
} cssc_gui_screen;

typedef struct { int64_t scale; int64_t color; } cssc_gui_font;

typedef struct {
    int              kind;        /* GW_TEXT */
    cssc_gui_screen* screen;
    char*            text;
    int64_t          x, y, scale, color;
    int              visible;
} cssc_gui_text;

typedef struct {
    int              kind;        /* GW_BUTTON */
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            label;
    int64_t          scale, base, hover, text_color, cb_id;
    int              hovered, pressed, visible;
} cssc_gui_button;

typedef struct {
    int               kind;       /* GW_TOOLBAR */
    cssc_gui_screen*  screen;
    int64_t           x, y, w, h, orient, spacing;
    cssc_gui_button** items;
    int               n_items, cap_items;
    char*             right_text;  /* right-aligned status text (e.g. CSSC version) */
    int64_t           right_color;
} cssc_gui_toolbar;

typedef struct {
    int              kind;        /* GW_TEXTBOX */
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            buf;
    int              len, cap, cursor;
    int64_t          scale, bg, fg, border;
    int              focused, visible;
} cssc_gui_textbox;

typedef struct {
    int              kind;        /* GW_EDITOR */
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
    int              dbl_fired;   /* one-shot: set on a double-click, read+cleared by the host */
    int              click_fired; /* one-shot: set on ANY editor click (host clears a held debug highlight) */
    int              rclick_fired; /* one-shot: set on a right-click over a word (host opens the semantic menu) */
    /* Semantic double-click: last scanned declaration of a token. Filled by
     * cssc_gui_editor_scandecl, read via the decl* accessors. */
    int              decl_isvar, decl_bits, decl_isauto;
    char             decl_type[64];
    char             decl_base[32];   /* type without the <...> generic part */
    int              follow;   /* 1 = re-center on caret this frame (set on caret moves,
                                  NOT on wheel — lets wheel scroll independently) */
    /* Ownership map from sema.cssc (scope-aware alloc<->dealloc + leak), parsed
     * from setOwnership() output. Lines are 1-indexed (as sema emits). When any
     * entries are present the editor highlights the cursor's variable group from
     * this map instead of the local ed_alloc_var scan. */
    int*             own_aline;  int* own_afreed; int* own_aleaked; int own_na;
    int*             own_dline;  int* own_dtarget; int own_nd;
    int              own_cap_a, own_cap_d;
    /* Completion popup — a caret-anchored candidate list filtered client-side
     * from one sema_complete fetch. cmp_line/cmp_start = the token being
     * completed (the empty-prefix marker position); the live filter prefix is
     * lines[cmp_line][cmp_start..cur_col]. cmp_filt indexes cmp_items. */
    char**           cmp_items; int cmp_n, cmp_cap;
    int*             cmp_filt;  int cmp_nf, cmp_capf;
    int              cmp_open, cmp_sel, cmp_top;
    int              cmp_start, cmp_line;
    int              cmp_req, cmp_pending;
    /* Hover box — sema_hover info for the word under a stationary mouse. hov_ws/
     * hov_we track the hovered word span so the box stays put over that word and
     * closes when the mouse moves to another word / off. hov_text is the display
     * content (set by the host from sema_hover). */
    char*            hov_text;
    int              hov_line, hov_col, hov_ws, hov_we;
    int              hov_mx, hov_my, hov_dwell;
    int              hov_open, hov_req;
    /* Diagnostics (sema_diag) -> squiggly underlines. dg_line is 1-based, dg_col
     * 0-based (as sema emits); dg_sev 1=error 2=warning 3=info. */
    int*             dg_line; int* dg_col; int* dg_sev; int dg_n, dg_cap;
    /* STICKY diagnostics (F1 analyze) -> persistent per-line marks + hover text,
     * kept until F2 clears them (independent of the debounced live squiggles).
     * sd_line 1-based, sd_sev 1=error 2=warn 3=info, sd_msg = the human message. */
    int*             sd_line; int* sd_sev; char** sd_msg; int sd_n, sd_cap, sd_on;
    /* Signature help — the callee signature while the caret is inside fn(args|).
     * sig_open_line/col track the enclosing call's '(' for change detection;
     * sig_arg is the active argument (top-level comma count) for the highlight. */
    char*            sig_text;
    int              sig_open, sig_req;
    int              sig_line, sig_col;
    int              sig_open_line, sig_open_col, sig_arg;
    /* F5 Debugger instruction pointer: 1-based source line to highlight red
     * (like an analyzer mark), 0 = none. Set by cssc_gui_editor_setipline. */
    int              ip_line;
    /* F10 autoclean review marks: 1-based lines the cleanup INSERTED, drawn with
     * a green wash so the author reviews each; cleared by F2. */
    int              clean_marks[512];
    int              clean_nmarks;
} cssc_gui_editor;

typedef struct {
    int              kind;        /* GW_LIST */
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
    int              kind;        /* GW_TREE — filesystem-backed explorer */
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char             root[520];
    char**           paths;       /* full path per visible row */
    char**           names;       /* basename per visible row */
    int*             depths;      /* indent level per row */
    int*             isdir;       /* 1 folder / 0 file per row */
    long long*       sizes;       /* file size in bytes per row (-1 for dirs) */
    int              nrows, cap;
    int              selected, top, right_hit;
    int64_t          scale, bg, fg, sel_bg, sel_fg;
    int              focused, visible;
    char**           fold_paths;  /* path-keyed fold state (survives rebuild) */
    int*             fold_open;
    int              fold_n, fold_cap;
    char             icondir[520];   /* dir holding the type icons, "" = none */
    void*            ico_folder; void* ico_cssc; void* ico_md;
    void*            ico_ini;    void* ico_file;
    void*            ico_exe;    void* ico_csscu; void* ico_project;
    void*            ico_arrow_open; void* ico_arrow_closed;  /* folder expand state */
    /* data-driven per-extension icons: `<ext>.png` in the icon dir is used for
     * any `.<ext>` file automatically (drop a new png in, no C change needed). */
    struct { char ext[24]; void* spr; } ico_ext[48];
    int              ico_ext_n;
    int              ico_loaded;     /* 0 = (re)load on next draw */
    /* drag-and-drop move: press-drag a row onto a folder row to move it. */
    int              drag_on, drag_idx, drag_active, drag_dx, drag_dy;
    int              drop_ready;
    int              resizing;    /* dragging the right edge to resize the panel */
    char             drop_src[520], drop_dst[520];
} cssc_gui_tree;

typedef struct {
    int              kind;        /* GW_TERMINAL — scrollback + input + subproc */
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char**           lines;       /* scrollback text */
    int64_t*         line_col;    /* per-line colour */
    int              nlines, cap, top, stick;
    int              sa_l, sa_c, se_l, se_c, sel_active, dragging;
    char             input[1024];
    int              input_len, input_cur;
    char             cwd[1024];
    char**           hist;        /* command history */
    int              nhist, cap_hist, hist_pos;
    int64_t          scale, bg, fg;
    int              focused, visible;
    void*            proc;        /* HANDLE — running child, else NULL */
    void*            pipe_read;   /* HANDLE — child stdout/stderr read end */
    void*            stdin_write; /* HANDLE — child stdin write end (interactive) */
    int              running;
    char             partial[4096];
    int              partial_len;
    int              alt_c_prev;   /* Alt+C (clear) edge-detect state */
    /* output filter: ANSI SGR (colour) + UTF-8 decode so the piped CLI
     * output renders cleanly on the ASCII 8x8 font (no `â†'` byte soup, no
     * raw `\x1b[..m` escapes) and picks up the toolchain's purple palette. */
    int              esc_st;        /* 0 none, 1 saw ESC, 2 inside CSI */
    char             esc_buf[48];   /* CSI parameter bytes */
    int              esc_len;
    int64_t          line_col_use;  /* first SGR fg seen on the current line */
    int              line_col_set;  /* 1 once a non-reset fg applied this line */
    unsigned int     utf_cp;        /* UTF-8 codepoint accumulator */
    int              utf_need;      /* remaining continuation bytes */
    /* F5 console-debugger IP marker: `pctrace --console` writes an invisible
     * SOH line STX file SOH sequence on every step; we strip it from the visible
     * stream and expose ip_line/ip_file so the editor follows the executing line
     * (and switches source file across #load boundaries). */
    int              ipm_st;        /* 1 while capturing a marker */
    char             ipm_buf[1200];
    int              ipm_len;
    int              ip_line;       /* current instruction-pointer source line */
    char             ip_file[1024]; /* absolute path of the file it sits in */
    int              input_locked;  /* while a debug trace runs: swallow typed
                                     * input (only Ctrl+X interrupt / Ctrl+C copy
                                     * pass) so the transport keys own the panel */
    int              input_wanted;  /* the traced program is blocked in cssc::input
                                     * (an I1/I0 marker) — temporarily lift the lock
                                     * so the answer can be typed, then re-lock */
} cssc_gui_terminal;

typedef struct {
    int              kind;        /* GW_MENU — top menu bar + dropdowns */
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            titles[16];
    int              n_titles;
    char*            item_label[128];
    int              item_menu[128];      /* 1-based owning menu */
    int64_t          item_action[128];
    int              n_items;
    int              open;                /* 1-based open menu, 0 none */
    int64_t          last_action;         /* action clicked this frame */
    int64_t          scale, bg, fg;
    int              visible;
    char*            right_text;          /* right-aligned status text (CSSC version) */
    int64_t          right_color;
} cssc_gui_menu;

typedef struct {
    int              kind;        /* GW_PROMPT — modal single-line text input */
    cssc_gui_screen* screen;
    char             label[256];
    char             input[1024];
    int              input_len, input_cur;
    int              active;      /* 1 while showing */
    int              result;      /* one-shot: 1 submitted, 2 cancelled, 0 none */
    int64_t          scale, bg, fg;
    int              visible;
} cssc_gui_prompt;

typedef struct {
    int              kind;        /* GW_TABS — open-file tab strip */
    cssc_gui_screen* screen;
    int64_t          x, y, w, h;
    char*            paths[64];
    int              n_tabs, active;      /* active is 0-based */
    int64_t          switch_hit;          /* 1-based tab clicked to switch, else 0 */
    int64_t          close_hit;           /* 1-based tab whose X was clicked, else 0 */
    int64_t          scale, bg, fg;
    int              visible;
} cssc_gui_tabs;

typedef struct {
    int              kind;        /* GW_BROWSER — modal file/folder picker */
    cssc_gui_screen* screen;
    char             dir[520];
    char*            names[512];
    int              isdir[512];
    int              n, sel, top;
    int              mode;        /* 0 = pick file, 1 = pick folder */
    int              active;
    int              result;      /* one-shot: 1 ok, 2 cancel, 0 none */
    char             chosen[600];
    int64_t          scale, bg, fg;
    int              visible;
    char             icondir[520];   /* asset icon dir ("" = colored-square fallback) */
    void*            ico_folder; void* ico_cssc; void* ico_md; void* ico_ini;
    void*            ico_file;   void* ico_exe;  void* ico_csscu; void* ico_project;
    int              ico_loaded;
} cssc_gui_browser;

/* ---- helpers ------------------------------------------------------------ */
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
    /* Mouse (and modifiers) are read GLOBALLY via GetCursorPos/GetAsyncKeyState,
     * so when the IDE is NOT the active window (e.g. a terminal is on top and the
     * user drags it) the buttons/cursor would otherwise bleed in and move files /
     * select text. Treat it as "up and off-screen" unless we are foreground. */
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

/* The signature CSSC glass panel — identical colours + order to the
 * interpreter's _panel, drawn with the /255 alpha fillrect. */
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

/* ---- Screen ------------------------------------------------------------- */
CSSC_GUI_EXPORT void* cssc_gui_screen_new(int64_t w, int64_t h, int64_t fps) {
    cssc_gui_screen* s = (cssc_gui_screen*)calloc(1, sizeof(cssc_gui_screen));
    if (!s) return NULL;
    s->w = w > 0 ? w : 1;
    s->h = h > 0 ? h : 1;
    s->vid = cssc_video_new(s->w, s->h, fps > 0 ? fps : 60);
    cssc_video_begin(s->vid);     /* opens the window -> plays the watermark */
#ifdef _WIN32
    /* Center the window on the primary monitor's work area at startup. */
    HWND hwnd = (HWND)cssc_video_hwnd(s->vid);
    if (hwnd) {
        RECT wr; GetWindowRect(hwnd, &wr);
        int ww = (int)(wr.right - wr.left), wh = (int)(wr.bottom - wr.top);
        RECT wa;
        if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0)) {
            int cx = (int)wa.left + ((int)(wa.right - wa.left) - ww) / 2;
            int cy = (int)wa.top  + ((int)(wa.bottom - wa.top) - wh) / 2;
            if (cx < (int)wa.left) cx = (int)wa.left;
            if (cy < (int)wa.top)  cy = (int)wa.top;
            SetWindowPos(hwnd, NULL, cx, cy, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
#endif
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
/* Borderless fullscreen toggle (F11). Enter: save the windowed style + rect +
 * backbuffer dims, switch to a WS_POPUP covering the monitor, and resize the
 * backbuffer to match (the blit is 1:1). Exit: restore everything. width()/
 * height() report the new size, so the app re-layouts to fill the screen. */
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
/* A host-drawn modal overlay (e.g. the IDE color picker) sets this so the
 * widgets beneath it (editor caret, tree, tabs) ignore this frame's input
 * while it is up. Set 1 while open, 0 when closed. */
CSSC_GUI_EXPORT void cssc_gui_screen_setmodal(void* p, int64_t on) {
    cssc_gui_screen* s = (cssc_gui_screen*)p; if (s) s->cssc_modal = on ? 1 : 0;
}
/* Edge-triggered global hotkeys via GetAsyncKeyState (independent of the
/* 1 when the screen's window is the foreground window. The global
 * GetAsyncKeyState-based hotkeys/funcKeys must gate on this so the IDE does not
 * react to keys while the user is in another app (window on top). */
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
/* WM_CHAR queue, so they fire regardless of which widget has focus):
 * returns 1 = Ctrl+B, 2 = Ctrl+J, 0 = none. One event per key-press. */
CSSC_GUI_EXPORT int64_t cssc_gui_screen_hotkey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return 0;
#ifdef _WIN32
    int ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 1 : 0;
    int alt  = (GetAsyncKeyState(VK_MENU) & 0x8000) ? 1 : 0;
    int b = (ctrl && (GetAsyncKeyState('B') & 0x8000)) ? 1 : 0;
    int j = (ctrl && (GetAsyncKeyState('J') & 0x8000)) ? 1 : 0;
    /* Alt+^ opens the color picker. `^` is VK_OEM_5 (0xDC) on DE and the
     * top-left OEM key; also accept VK_OEM_3 (0xC0) which some layouts report
     * for that physical key. It's a dead key, so poll it here, not via WM_CHAR. */
    int caret = (alt && ((GetAsyncKeyState(0xDC) & 0x8000) ||
                         (GetAsyncKeyState(0xC0) & 0x8000))) ? 1 : 0;
    if (!screen_focused(s)) {          /* not the active window -> ignore, no edge */
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

/* Edge-triggered debugger transport keys, polled via key-STATE (not the WM_CHAR
 * queue) so they work as global controls while the terminal has focus for the
 * traced program's input: returns 1 = '+' (faster), 2 = '-' (slower),
 * 3 = '#' (pause toggle), 0 = none. Accepts numpad +/- and the DE-layout OEM
 * keys. One event per physical press. */
CSSC_GUI_EXPORT int64_t cssc_gui_screen_dbgkey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return 0;
#ifdef _WIN32
    int plus  = ((GetAsyncKeyState(VK_ADD) & 0x8000) ||
                 (GetAsyncKeyState(0xBB) & 0x8000)) ? 1 : 0;   /* VK_OEM_PLUS */
    int minus = ((GetAsyncKeyState(VK_SUBTRACT) & 0x8000) ||
                 (GetAsyncKeyState(0xBD) & 0x8000)) ? 1 : 0;   /* VK_OEM_MINUS */
    int hash  = (GetAsyncKeyState(0xBF) & 0x8000) ? 1 : 0;     /* VK_OEM_2 = '#' on DE */
    if (!screen_focused(s)) {
        s->dk_prev_plus = plus; s->dk_prev_minus = minus; s->dk_prev_hash = hash;
        return 0;
    }
    int64_t r = 0;
    if (plus && !s->dk_prev_plus)        r = 1;
    else if (minus && !s->dk_prev_minus) r = 2;
    else if (hash && !s->dk_prev_hash)   r = 3;
    s->dk_prev_plus = plus; s->dk_prev_minus = minus; s->dk_prev_hash = hash;
    return r;
#else
    return 0;
#endif
}

/* Edge-triggered function keys (independent of focus / the WM_CHAR queue):
 * returns 1 = F1, 5 = F5, 6 = F6, 9 = F9, 11 = F11, 0 = none. One per press. */
CSSC_GUI_EXPORT int64_t cssc_gui_screen_funckey(void* p) {
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (!s) return 0;
#ifdef _WIN32
    int f1 = (GetAsyncKeyState(VK_F1) & 0x8000) ? 1 : 0;
    int f2 = (GetAsyncKeyState(VK_F2) & 0x8000) ? 1 : 0;
    int f5 = (GetAsyncKeyState(VK_F5) & 0x8000) ? 1 : 0;
    int f6 = (GetAsyncKeyState(VK_F6) & 0x8000) ? 1 : 0;
    int f9 = (GetAsyncKeyState(VK_F9) & 0x8000) ? 1 : 0;
    int f10 = (GetAsyncKeyState(VK_F10) & 0x8000) ? 1 : 0;
    int f11 = (GetAsyncKeyState(VK_F11) & 0x8000) ? 1 : 0;
    if (!screen_focused(s)) {          /* not the active window -> ignore, no edge */
        s->hk_prev_f1 = f1; s->hk_prev_f2 = f2; s->hk_prev_f5 = f5;
        s->hk_prev_f6 = f6; s->hk_prev_f9 = f9; s->hk_prev_f10 = f10; s->hk_prev_f11 = f11;
        return 0;
    }
    int64_t r = 0;
    if (f1 && !s->hk_prev_f1) r = 1;
    else if (f2 && !s->hk_prev_f2) r = 2;
    else if (f5 && !s->hk_prev_f5) r = 5;
    else if (f6 && !s->hk_prev_f6) r = 6;
    else if (f9 && !s->hk_prev_f9) r = 9;
    else if (f10 && !s->hk_prev_f10) r = 10;
    else if (f11 && !s->hk_prev_f11) r = 11;
    s->hk_prev_f1 = f1;
    s->hk_prev_f2 = f2;
    s->hk_prev_f5 = f5;
    s->hk_prev_f6 = f6;
    s->hk_prev_f9 = f9;
    s->hk_prev_f10 = f10;
    s->hk_prev_f11 = f11;
    return r;
#else
    return 0;
#endif
}

/* Level-triggered: 1 while function key n (1..12) is physically held. Lets the
 * app distinguish a short tap (funcKey edge) from a long press (still held N
 * frames later) — used for the F5/F6 version-select overlays. */
CSSC_GUI_EXPORT int64_t cssc_gui_screen_fkeydown(void* p, int64_t n) {
#ifdef _WIN32
    cssc_gui_screen* s = (cssc_gui_screen*)p;
    if (n < 1 || n > 12) return 0;
    if (s && !screen_focused(s)) return 0;   /* not the active window -> ignore */
    return (GetAsyncKeyState((int)(VK_F1 + (n - 1))) & 0x8000) ? 1 : 0;
#else
    (void)p; (void)n; return 0;
#endif
}
/* Copy text to the system clipboard (used by the Controls overlay's copy). */
static void gui_clipboard_set(const char* text);   /* defined below */
CSSC_GUI_EXPORT void cssc_gui_screen_clipboardset(void* p, const char* text) {
    (void)p;
    if (text) gui_clipboard_set(text);
}
/* ---- Screen: editor input (text, nav keys, wheel) ----------------------- */
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

/* ---- Screen: direct drawing (so an editor can paint its own surface) ----- */
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

/* --- screen icons: a PNG drawn at a position (LSP status badge), and the
 * window icon. A small path->sprite cache keeps per-frame drawicon cheap. --- */
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
    /* 32-bit top-down ARGB DIB from the sprite pixels (already 0xAARRGGBB). */
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
    /* all-zero AND mask -> the color's alpha channel governs transparency. */
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

/* forward decls for the dispatch inside update/render */
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
    if (s->cssc_modal) s->input_captured = 1;   /* host overlay (color picker) owns input */
    /* Pass 1 — overlays (prompt modal, then menus) update first so they claim
     * this frame's input before any widget beneath them can react to it. */
    for (int i = 0; i < s->n_widgets; ++i) {
        if (*(int*)s->widgets[i] == GW_PROMPT) {
            cssc_gui_prompt* pr = (cssc_gui_prompt*)s->widgets[i];
            if (pr->active) s->input_captured = 1;   /* modal — capture first */
            cssc_gui_prompt_update(pr, s);
        } else if (*(int*)s->widgets[i] == GW_BROWSER) {
            cssc_gui_browser* br = (cssc_gui_browser*)s->widgets[i];
            if (br->active) s->input_captured = 1;   /* modal — capture first */
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
    /* Pass 2 — every other widget. They skip input when captured. */
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

/* ---- Font --------------------------------------------------------------- */
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

/* ---- Text --------------------------------------------------------------- */
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

/* ---- Button ------------------------------------------------------------- */
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

/* ---- Toolbar ------------------------------------------------------------ */
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
/* Right-aligned status text drawn inside the bar (e.g. the live CSSC version).
 * Passing "" clears it. Colour defaults to a muted glass tint on first set. */
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
        int64_t tx = t->x + t->w - tw - 12;      /* 12px right margin */
        int64_t ty = t->y + (t->h - glyph) / 2;
        cssc_video_draw_text(t->screen->vid, tx, ty, t->right_text,
                             t->right_color ? t->right_color : (int64_t)0xFF8FA6B8, scale);
    }
}

/* ---- TextBox (editable single-line field) ------------------------------- */
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
        /* single-line: enter (13/10) is ignored */
    }
    int64_t k;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        if (k == 0x25) { if (t->cursor > 0) t->cursor--; }      /* LEFT */
        else if (k == 0x27) { if (t->cursor < t->len) t->cursor++; } /* RIGHT */
        else if (k == 0x24) t->cursor = 0;                      /* HOME */
        else if (k == 0x23) t->cursor = t->len;                 /* END */
        else if (k == 0x2E) tb_delete(t);                       /* DELETE */
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

/* ---- Editor (multi-line code surface: line buffer + cursor + scroll) ----- */
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
/* Select the word (or run of like characters) under (line,col). */
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
    /* Select the whole word but put the CARET at its START (a), anchoring the
     * selection at the end (b). Double-click then leaves the cursor at the very
     * beginning of the variable, not somewhere after it. `selectedText` still
     * returns the full word (the selection range is normalized on read). */
    e->sel_line = line; e->sel_col = b;
    e->cur_line = line; e->cur_col = a;
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
/* Ctrl+Down duplicates the current line. When that line is a
 * `#scanp(fn, type, N) name;` declaration, the copy almost always means "declare
 * the NEXT positional parameter", so the duplicate's arg index N is bumped to
 * N+1. Returns a malloc'd rewritten line (caller frees) or NULL to fall back to a
 * verbatim copy. Commas inside <>/()/[]/{} (e.g. `map<string,int>`) are skipped
 * so the 3rd TOP-LEVEL argument is the one located. */
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

/* Cell metrics + gutter width — kept identical between update (hit-test) and
 * draw so a click lands on the glyph it visually points at. */
static void ed_metrics(cssc_gui_editor* e, int64_t* glyph, int64_t* line_h,
                       int64_t* gutter_w, int64_t* text_x) {
    int64_t g = 8 * e->scale;
    *glyph = g;
    *line_h = g + 4 * e->scale;
    int digits = 1, nn = e->nlines; while (nn >= 10) { nn /= 10; digits++; }
    *gutter_w = e->gutter_on ? (int64_t)(digits + 1) * g + 8 : 0;
    *text_x = e->x + *gutter_w + 6;
}
/* Map a client point to a (line,col). Returns 1 if the point is inside the
 * editor rect. col rounds to the nearest gap so clicks feel natural. */
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
/* Order the selection anchor + caret into start<=end. */
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

/* ---- clipboard (Win32 CF_TEXT) ------------------------------------------ */
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
static char* gui_clipboard_get(void) {   /* malloc'd copy or NULL */
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

/* ---- doc serialization + undo/redo (full-document snapshots) ------------- */
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
            if (len > 0 && start[len - 1] == '\r') len--;   /* drop CR from CRLF */
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
/* Record a snapshot before an edit; consecutive char-typing coalesces into
 * one undo unit (op==1). */
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
static char* ed_selected_str(cssc_gui_editor* e) {   /* malloc'd, or NULL */
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

/* ---- completion popup helpers ------------------------------------------- */
static void ed_cmp_free_items(cssc_gui_editor* e) {
    for (int i = 0; i < e->cmp_n; i++) free(e->cmp_items[i]);
    e->cmp_n = 0;
}
static void ed_cmp_close(cssc_gui_editor* e) {
    e->cmp_open = 0; e->cmp_pending = 0;
    e->cmp_sel = 0; e->cmp_top = 0; e->cmp_nf = 0;
}
/* case-insensitive: does `item` start with the first `plen` bytes of `pfx`?
 * (mirrors sema's txCompStartsWith so client filtering == server filtering) */
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
/* Rebuild cmp_filt from cmp_items using the live prefix on cmp_line. */
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
/* Directive name -> the bracket that canonically follows it (snippet), else 0. */
static char ed_cmp_snippet_bracket(const char* label) {
    static const char* brk[] = {"#stack","#heap","#auto","#delete","#req",
                                 "#free","#load","#unload","#depend", 0};
    static const char* par[] = {"#scanp","#include","#define","#cdefine",
                                 "#DEFINE", 0};
    for (int i = 0; brk[i]; i++) if (!strcmp(label, brk[i])) return '[';
    for (int i = 0; par[i]; i++) if (!strcmp(label, par[i])) return '(';
    return 0;
}
/* Insert the selected candidate, replacing the current token. For directives
 * that take a bracket, add the pair (caret inside) and re-arm the completion
 * so the argument context (e.g. #delete[<here>]) fetches automatically. */
static void ed_cmp_accept(cssc_gui_editor* e) {
    if (!e->cmp_open || e->cmp_nf <= 0) return;
    if (e->cmp_line != e->cur_line) { ed_cmp_close(e); return; }
    const char* label = e->cmp_items[e->cmp_filt[e->cmp_sel]];
    ed_pre_edit(e, 2);
    char* line = e->lines[e->cur_line];
    int ll = (int)strlen(line);
    int pstart = e->cmp_start; if (pstart > ll) pstart = ll; if (pstart < 0) pstart = 0;
    int pend = pstart;
    if (pend < ll && line[pend] == '#') pend++;      /* directive token starts with '#' */
    while (pend < ll && ed_isword(line[pend])) pend++;
    memmove(line + pstart, line + pend, (size_t)(ll - pend) + 1);
    e->cur_col = pstart;
    /* module name at a bare `#include(` / `#depend(` (no quote) -> wrap in "" */
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
/* After a printable char (cc) was inserted, decide whether to (re)fetch or just
 * refilter the popup, and where the completed token starts. */
static void ed_cmp_on_type(cssc_gui_editor* e, char cc) {
    if (e->language != 1) { ed_cmp_close(e); return; }
    char* ln = e->lines[e->cur_line];
    int cur = e->cur_col;
    char b2 = cur >= 2 ? ln[cur - 2] : 0;
    int startc = cur;
    while (startc > 0 && ed_isword(ln[startc - 1])) startc--;
    int trig = 0, reqStart = startc;
    if (cc == '.')                    { trig = 1; reqStart = cur; }
    else if (cc == '#')               { trig = 1; reqStart = cur - 1; } /* token has '#' */
    else if (cc == ':' && b2 == ':')  { trig = 1; reqStart = cur; }
    else if (cc == '>' && b2 == '-')  { trig = 1; reqStart = cur; }
    else if (cc == '[' || cc == '(') {
        int bpos = cur - 1;                          /* smart-bracket left caret here */
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
/* ---- signature-help helpers ---- */
static void ed_sig_close(cssc_gui_editor* e) {
    if (e->sig_text) { free(e->sig_text); e->sig_text = NULL; }
    e->sig_open = 0;
    e->sig_open_line = -1; e->sig_open_col = -1; e->sig_arg = 0;
}
/* If the caret sits inside a call fn(args|...), (re)arm signature help. Fetches
 * only when the enclosing call changes; the active arg (top-level comma count to
 * the caret) updates the highlight without a re-fetch. */
static void ed_sig_scan(cssc_gui_editor* e) {
    if (e->language != 1) { ed_sig_close(e); return; }
    if (e->cmp_open) { ed_sig_close(e); return; }
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
        e->sig_req = 1;                       /* new call -> fetch signature */
    } else {
        e->sig_arg = activeArg;               /* same call -> just move the highlight */
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
            if (len > 0 && start[len - 1] == '\r') len--;   /* drop CR from CRLF */
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
/* F5 Debugger: highlight `line` (1-based) red as the instruction pointer, or
 * pass 0 to clear. The editor auto-scrolls to keep the IP in view (IP-follow;
 * the overlay's F1 toggle drives whether a live line is passed in). */
/* Set (or clear, line=0) the red IP marker. PURE state — never scrolls: the
 * view only tracks the IP when the IDE explicitly calls revealIp() (gated on
 * the debugger's ipFollow), so free-cam / post-stop browsing is never yanked. */
CSSC_GUI_EXPORT void cssc_gui_editor_setipline(void* p, int64_t line) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (!e) return;
    e->ip_line = (int)line;
}
/* Scroll/caret the current IP line into view (draw() reveals e->cur_line when
 * e->follow is set). Called each frame while actively following. */
CSSC_GUI_EXPORT void cssc_gui_editor_revealip(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (!e) return;
    if (e->ip_line > 0) {
        int target = e->ip_line - 1;
        if (target >= 0 && target < e->nlines) {
            e->cur_line = target;
            if (e->cur_col < 0) e->cur_col = 0;
            e->follow = 1;
        }
    }
}
/* Pixel position of the caret's top-left, matching cssc_gui_editor_draw exactly
 * (uses the last-drawn top_line/left_col so an anchored popup lines up with what
 * is on screen). caretPixelY is the top of the caret line; add lineHeight() to
 * drop a popup just below it. */
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
/* --- completion popup public surface (driven by the workspace) --- */
/* 1 (once) when the editor wants the host to fetch completions via sema. */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_completereq(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int r = e->cmp_req; e->cmp_req = 0; return r;
}
/* 1 while the popup is showing (so the host can suppress hover, etc.). */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_completeactive(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    return (e && e->cmp_open) ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_editor_completecancel(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (e) { ed_cmp_free_items(e); ed_cmp_close(e); }
}
/* The buffer text with a 0x04 marker inserted at the completion token start —
 * fed to sema_complete for the full (empty-prefix) context list. */
CSSC_GUI_EXPORT void* cssc_gui_editor_completionquery(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || e->nlines == 0) return cssc_string_lit("", 0);
    int cl = e->cmp_line; if (cl < 0) cl = 0; if (cl >= e->nlines) cl = e->nlines - 1;
    int cll = (int)strlen(e->lines[cl]);
    int cs = e->cmp_start; if (cs < 0) cs = 0; if (cs > cll) cs = cll;
    if (cs < cll && e->lines[cl][cs] == '#') cs++;   /* sema marker sits AFTER the '#' */
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
/* Load the newline-separated candidate list from sema and open the popup. */
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
/* --- hover box public surface --- */
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
/* Buffer text with a 0x04 marker at the hovered word — fed to sema_hover/sig. */
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
/* Show the hover box with `text` (empty/blank closes it). */
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
/* Load sema_diag output ("line:col:severity:code" per line) as squiggle spans. */
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
/* Free the sticky-diagnostic table. */
static void ed_sticky_clear(cssc_gui_editor* e) {
    if (!e) return;
    for (int i = 0; i < e->sd_n; i++) { if (e->sd_msg[i]) free(e->sd_msg[i]); }
    e->sd_n = 0;
    e->sd_on = 0;
}
/* F1: load sema_diag output ("line:col:sev:CODE:message") as PERSISTENT per-line
 * marks + hover messages. Unlike setdiagnostics (squiggles, no text, refreshed
 * every debounce), these stay until clearStickyDiag() (F2). */
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
                if (field == 4) {                 /* rest of the line is the message */
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
/* F10 autoclean: mark the given comma-separated 1-based line numbers with the
 * green review wash (the lines the cleanup just inserted). */
CSSC_GUI_EXPORT void cssc_gui_editor_setcleanmarks(void* p, const char* csv) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (!e) return;
    e->clean_nmarks = 0;
    if (!csv) return;
    int n = 0;
    for (const char* s = csv; *s; s++) {
        if (*s >= '0' && *s <= '9') {
            n = n * 10 + (*s - '0');
        } else if (n > 0) {
            if (e->clean_nmarks < 512) e->clean_marks[e->clean_nmarks++] = n;
            n = 0;
        }
    }
    if (n > 0 && e->clean_nmarks < 512) e->clean_marks[e->clean_nmarks++] = n;
}
CSSC_GUI_EXPORT void cssc_gui_editor_clearcleanmarks(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; if (e) e->clean_nmarks = 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_stickycount(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? (int64_t)e->sd_n : 0;
}
/* --- signature help public surface --- */
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
/* Buffer text with a 0x04 marker at the caret inside the call — fed to sema_sig. */
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
/* Show the signature box. Stores only the FIRST line (the `fn(...)` signature);
 * sema's trailing `active:N` line is ignored (the editor tracks the active arg). */
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
/* Insert `s` at the caret (splitting on '\n'); advances the caret past it. One
 * undo entry covers the whole string. Used by the IDE's color picker to drop a
 * hex code at the cursor. */
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
/* Highlight every occurrence of `term` magenta WITHOUT moving the caret or
 * touching the selection. Returns the match count. Empty term clears it. Used
 * by double-click, which must keep the word it just selected marked in place. */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_markall(void* p, const char* term) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    if (e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
    if (!term || !term[0]) return 0;
    e->search = gui_strdup(term);
    e->search_len = (int)strlen(term);
    int count = 0;
    for (int i = 0; i < e->nlines; ++i) {
        const char* hit = e->lines[i];
        while ((hit = strstr(hit, e->search)) != NULL) { count++; hit += e->search_len; }
    }
    return count;
}
/* Set the active find term; every occurrence is highlighted magenta while it
 * stays set, and the caret JUMPS to the first match (Find behaviour). Returns
 * the total match count. Empty term clears the search. */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_search(void* p, const char* term) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    int64_t count = cssc_gui_editor_markall(p, term);
    if (e->search) {
        for (int i = 0; i < e->nlines; ++i) {
            const char* hit = strstr(e->lines[i], e->search);
            if (hit) { e->cur_line = i; e->cur_col = (int)(hit - e->lines[i]);
                       e->sel_active = 0; ed_clamp(e); break; }
        }
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
/* Replace every occurrence of `find` with `repl` across the whole document.
 * Recorded as a single undo step. Returns how many were replaced. */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_replaceall(void* p, const char* find,
                                                   const char* repl) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !find || !find[0]) return 0;
    if (!repl) repl = "";
    int flen = (int)strlen(find), rlen = (int)strlen(repl);
    /* Count first so we only snapshot for undo when something changes. */
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
    int r = e->save_req; e->save_req = 0;   /* one-shot */
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
/* One-shot: 1 on the frame a double-click landed on a word (the host then opens
 * the semantic menu + highlights matches). Cleared on read. */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_doubleclicked(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !e->dbl_fired) return 0;
    e->dbl_fired = 0;
    return 1;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_clicked(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !e->click_fired) return 0;
    e->click_fired = 0;
    return 1;
}
/* One-shot: 1 on the frame a right-click landed on a word (the host then opens
 * the semantic menu for the token under the cursor). Cleared on read. */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_rightclicked(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e || !e->rclick_fired) return 0;
    e->rclick_fired = 0;
    return 1;
}
/* Scan the buffer for the declaration of `word` — `#stack[T,N] word`,
 * `#heap[T,N] word`, or `#auto[T] word` — and stash type/bits/auto for the
 * semantic double-click menu. Returns 1 if a declaration was found (i.e. the
 * token is a variable), 0 otherwise. Line-local declarations (the CSSC norm). */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_scandecl(void* p, const char* word) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return 0;
    e->decl_isvar = 0; e->decl_bits = 0; e->decl_isauto = 0;
    e->decl_type[0] = 0; e->decl_base[0] = 0;
    if (!word || !word[0]) return 0;
    size_t wl = strlen(word);
    for (int li = 0; li < e->nlines; li++) {
        const char* h = strstr(e->lines[li], "#");
        while (h) {
            int kind = 0;                 /* 1 = stack/heap (typed+bits), 2 = auto */
            const char* after = NULL;
            if (strncmp(h, "#stack[", 7) == 0) { kind = 1; after = h + 7; }
            else if (strncmp(h, "#heap[", 6) == 0) { kind = 1; after = h + 6; }
            else if (strncmp(h, "#auto[", 6) == 0) { kind = 2; after = h + 6; }
            if (kind) {
                char type[64]; int ti = 0;
                const char* q = after;
                while (*q && *q != ',' && *q != ']' && ti < 63) {
                    if (*q != ' ') type[ti++] = *q;
                    q++;
                }
                type[ti] = 0;
                int bits = 0;
                if (kind == 1 && *q == ',') {
                    q++;
                    while (*q == ' ') q++;
                    while (*q >= '0' && *q <= '9') { bits = bits * 10 + (*q - '0'); q++; }
                }
                while (*q && *q != ']') q++;
                if (*q == ']') q++;
                while (*q == ' ') q++;
                if (strncmp(q, word, wl) == 0) {
                    char nc = q[wl];
                    int boundary = !((nc >= 'A' && nc <= 'Z') || (nc >= 'a' && nc <= 'z')
                                     || (nc >= '0' && nc <= '9') || nc == '_');
                    if (boundary) {
                        e->decl_isvar = 1;
                        e->decl_bits = bits;
                        e->decl_isauto = (kind == 2) ? 1 : 0;
                        snprintf(e->decl_type, sizeof(e->decl_type), "%s", type);
                        int bi = 0;
                        while (type[bi] && type[bi] != '<' && bi < 31) {
                            e->decl_base[bi] = type[bi]; bi++;
                        }
                        e->decl_base[bi] = 0;
                        return 1;
                    }
                }
            }
            h = strstr(h + 1, "#");
        }
    }
    return 0;
}
CSSC_GUI_EXPORT void* cssc_gui_editor_decltype(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    const char* t = (e && e->decl_type[0]) ? e->decl_type : "";
    return cssc_string_lit(t, strlen(t));
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_declbits(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? e->decl_bits : 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_editor_declisauto(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p; return e ? e->decl_isauto : 0;
}
CSSC_GUI_EXPORT void* cssc_gui_editor_declbase(void* p) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    const char* b = (e && e->decl_base[0]) ? e->decl_base : "";
    return cssc_string_lit(b, strlen(b));
}
/* Bits a value of `base` type would occupy — for the "test limit" check.
 * string = 8 * byte-length; bool = 8; float = 64; int-like = bit-count of the
 * parsed decimal/hex magnitude (+1 for a leading '-'). */
CSSC_GUI_EXPORT int64_t cssc_gui_editor_valuebits(void* p, const char* base,
                                                  const char* input) {
    (void)p;
    if (!base || !input) return 0;
    if (strcmp(base, "string") == 0) return (int64_t)strlen(input) * 8;
    if (strcmp(base, "bool") == 0) return 8;
    if (strcmp(base, "float") == 0) return 64;
    const char* s = input;
    while (*s == ' ') s++;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    unsigned long long v = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (*s) {
            char c = *s++; int d;
            if (c >= '0' && c <= '9') d = c - '0';
            else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
            else break;
            v = v * 16 + (unsigned)d;
        }
    } else {
        while (*s >= '0' && *s <= '9') { v = v * 10 + (unsigned)(*s - '0'); s++; }
    }
    int bits = 0; unsigned long long t = v;
    while (t) { bits++; t >>= 1; }
    if (bits == 0) bits = 1;
    if (neg) bits++;
    return bits;
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
/* Editor language by extension: 1 = CSSC (.cssc), 2 = x86 assembly (.asm/.s),
 * 3 = cssc.cproject config, 0 = plain text. Drives which highlighter runs. */
CSSC_GUI_EXPORT void cssc_gui_editor_setlangforpath(void* p, const char* path) {
    cssc_gui_editor* e = (cssc_gui_editor*)p;
    if (!e) return;
    int lang = 0;
    if (path) {
        const char* dot = strrchr(path, '.');
        if (dot && !strcmp(dot, ".cssc")) lang = 1;
        else if (dot && (!strcmp(dot, ".asm") || !strcmp(dot, ".s"))) lang = 2;
        else if (dot && !strcmp(dot, ".cproject")) lang = 3;
    }
    e->language = lang;
}

/* Read the next whitespace-separated integer in s[*pos..len); advances *pos.
 * Returns -999999 when no digits were found. */
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

/* ============================ ADHELPER =====================================
 * "adhelper" = the alloc<->dealloc highlighter. This is the single place that
 * feeds the cursor-driven allocation/deallocation markers in the editor:
 * magenta = freed group, orange-red (0xE0704A) = a live allocation with no
 * matching #delete (leak). The scope-aware map is produced by sema (sema_own)
 * and parsed here; drawing lives in cssc_gui_editor_draw under the "adhelper"
 * comment. Grep "adhelper" to find every touch-point (parse here + draw there).
 * Load: lines "A <line> <col> <scope> <freed> <leaked>" and
 * "D <line> <col> <targetAllocLine>". Empty/NULL clears it (falls back to the
 * local ed_alloc_var per-line scan). #stack/#heap/#auto + result-captures +
 * #req &x copies are all tracked (sema records them as owning slots).
 * ========================================================================== */
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
            (void)own_read_int(text, le, &pos);   /* col */
            (void)own_read_int(text, le, &pos);   /* scope */
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
            (void)own_read_int(text, le, &pos);   /* col */
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

    /* Mouse — click positions the caret, drag extends a selection. Handled
     * regardless of focus so a click into the editor doubles as focus-gain
     * (the host toggles keyboard focus by region on the same frame). */
    int ml, mc;
    if (e->dbl_timer > 0) e->dbl_timer--;
    if (s->down && !s->prev_down && !s->input_captured) {
        if (ed_hittest(e, s->mx, s->my, &ml, &mc)) {
            if (e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
            ed_cmp_close(e);
            e->click_fired = 1;         /* host clears any held (stopped) debug highlight */
            e->follow = 1;
            int is_dbl = (e->dbl_timer > 0 && ml == e->dbl_line &&
                          (mc - e->dbl_col <= 1 && e->dbl_col - mc <= 1));
            e->cur_line = ml; e->cur_col = mc;
            if (is_dbl) {
                ed_select_word(e, ml, mc);
                e->dragging = 0;
                e->dbl_fired = 1;      /* host: open the semantic menu + highlight matches */
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

    /* Right-click — positions the caret over the clicked word, selects it (so
     * selectedText/scanDecl see the token), and fires the one-shot the host
     * reads to open the semantic menu. Distinct from double-click (mark-all). */
    if (s->rdown && !s->prev_rdown && !s->input_captured) {
        if (ed_hittest(e, s->mx, s->my, &ml, &mc)) {
            if (e->search) { free(e->search); e->search = NULL; e->search_len = 0; }
            ed_cmp_close(e);
            e->follow = 1;
            e->cur_line = ml; e->cur_col = mc;
            ed_select_word(e, ml, mc);
            e->dragging = 0;
            e->rclick_fired = 1;
        }
    }

    /* Hover: a stationary mouse (button up) dwelling over a CSSC word requests
     * sema hover info. The box stays over that word and closes on move/off. */
    {
        int overed = ((e->language == 1) && !s->input_captured && !e->cmp_open &&
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
        /* Completion popup steals Enter/Tab (accept) and Esc (dismiss). */
        if (e->cmp_open && e->cmp_nf > 0 && (c == 13 || c == 10 || c == 9)) {
            ed_cmp_accept(e); continue;
        }
        if (e->cmp_open && c == 27) { ed_cmp_close(e); continue; }
        if (c == 8) {                                       /* backspace */
            ed_pre_edit(e, 2);
            if (e->sel_active) { ed_delete_selection(e); }
            else {
                char prev = e->cur_col > 0 ? line[e->cur_col - 1] : 0;
                char next = line[e->cur_col];
                if ((prev == '(' && next == ')') || (prev == '[' && next == ']') ||
                    (prev == '{' && next == '}') || (prev == '"' && next == '"')) {
                    ed_delete(e); ed_backspace(e);          /* nuke empty pair */
                } else ed_backspace(e);
            }
            if (e->cmp_open) {
                if (e->cur_line != e->cmp_line || e->cur_col < e->cmp_start)
                    ed_cmp_close(e);
                else ed_cmp_refilter(e);
            }
        } else if (c == 13 || c == 10) {                    /* enter — auto-indent */
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
        } else if (c == 9) {                                /* tab / shift+tab */
            ed_pre_edit(e, 2);
            if (shift) ed_dedent_block(e);
            else if (e->sel_active) ed_indent_block(e);
            else { ed_insert_char(e, ' '); ed_insert_char(e, ' ');
                   ed_insert_char(e, ' '); ed_insert_char(e, ' '); }
        } else if (c == 1) {                                /* Ctrl+A — select all */
            ed_select_all(e);
        } else if (c == 3) {                                /* Ctrl+C — copy */
            char* sel = ed_selected_str(e);
            if (sel) { gui_clipboard_set(sel); free(sel); }
        } else if (c == 24) {                               /* Ctrl+X — cut */
            char* sel = ed_selected_str(e);
            if (sel) { gui_clipboard_set(sel); free(sel);
                       ed_pre_edit(e, 2); ed_delete_selection(e); }
        } else if (c == 22) {                               /* Ctrl+V — paste */
            char* clip = gui_clipboard_get();
            if (clip) { ed_pre_edit(e, 2);
                        if (e->sel_active) ed_delete_selection(e);
                        ed_insert_str(e, clip); free(clip); }
        } else if (c == 26) {                               /* Ctrl+Z — undo */
            ed_undo(e); ed_cmp_close(e);
        } else if (c == 25) {                               /* Ctrl+Y — redo */
            ed_redo(e); ed_cmp_close(e);
        } else if (c == 19) {                               /* Ctrl+S — save */
            e->save_req = 1;
        } else if (c >= 32 && c < 127) {                    /* printable + smart brackets */
            if (e->sel_active) { ed_pre_edit(e, 2); ed_delete_selection(e); }
            else ed_pre_edit(e, 1);
            line = e->lines[e->cur_line];
            char cc = (char)c;
            char at = line[e->cur_col];
            if (cc == '(' || cc == '[' || cc == '{') {
                char closer = cc == '(' ? ')' : cc == '[' ? ']' : '}';
                /* Smart wrap: typing '(' DIRECTLY before a bare word closes the
                 * paren AFTER that word, turning a fused `typeof|variables` into
                 * `typeof(variables)` (caret left just inside, before the word).
                 * Only for '(' and only when the next char begins an identifier;
                 * every other case keeps the plain `(|)` auto-pair. */
                int aw_word = ((at >= 'a' && at <= 'z') || (at >= 'A' && at <= 'Z')
                               || at == '_');
                if (cc == '(' && aw_word) {
                    int wend = e->cur_col;
                    while (line[wend]) {
                        char wc = line[wend];
                        if ((wc >= 'a' && wc <= 'z') || (wc >= 'A' && wc <= 'Z')
                                || (wc >= '0' && wc <= '9') || wc == '_') wend++;
                        else break;
                    }
                    ed_insert_char(e, cc);          /* '(' — cur_col now after it */
                    int caret = e->cur_col;         /* inside, before the word */
                    e->cur_col = wend + 1;          /* word shifted right by the '(' */
                    ed_insert_char(e, closer);      /* ')' after the word */
                    e->cur_col = caret;
                } else {
                    ed_insert_char(e, cc); ed_insert_char(e, closer); e->cur_col--;
                }
            } else if (cc == '"') {
                if (at == '"') e->cur_col++;
                else { ed_insert_char(e, '"'); ed_insert_char(e, '"'); e->cur_col--; }
            } else if ((cc == ')' || cc == ']' || cc == '}') && at == cc) {
                e->cur_col++;                               /* overtype closer */
            } else {
                ed_insert_char(e, cc);
            }
            ed_cmp_on_type(e, cc);                          /* (re)arm / filter completion */
        }
    }
    int64_t k;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        e->follow = 1;
        /* Completion popup: Up/Down/PgUp/PgDn move the selection; Left/Right/
         * Home/End dismiss it and fall through to normal caret motion. */
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
        /* Ctrl +/- zoom the editor font; Ctrl+Alt +/- resets to the default (2).
         * 0xBB/0xBD = OEM '='/'-' (so Ctrl+= and Ctrl++ both zoom in), 0x6B/0x6D
         * = numpad +/-. */
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
        if (k == 0x28 && (int)s->ctrl) {           /* Ctrl+Down — duplicate line (smart #scanp) */
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
            if (k == 0x25) {                                   /* LEFT */
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
            } else if (k == 0x27) {                            /* RIGHT */
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
            } else if (k == 0x26) { if (e->cur_line > 0) e->cur_line--; }             /* UP */
            else if (k == 0x28) { if (e->cur_line < e->nlines - 1) e->cur_line++; }   /* DOWN */
            else if (k == 0x24) e->cur_col = 0;                                        /* HOME */
            else if (k == 0x23) e->cur_col = (int)strlen(e->lines[e->cur_line]);       /* END */
            else if (k == 0x21) e->cur_line -= 10;                                     /* PGUP */
            else if (k == 0x22) e->cur_line += 10;                                     /* PGDN */
            if (shift) e->sel_active = (e->sel_line != e->cur_line || e->sel_col != e->cur_col) ? 1 : 0;
            else e->sel_active = 0;
            e->last_op = 2;
            ed_clamp(e);
        } else if (k == 0x2E) {                                /* DELETE */
            ed_pre_edit(e, 2);
            if (e->sel_active) ed_delete_selection(e); else ed_delete(e);
            ed_clamp(e);
        }
    }
    /* Wheel scrolls the editor whenever the mouse is OVER it — independent of
     * keyboard focus, so the code stays scrollable while a debug trace holds the
     * terminal focused. Read (which consumes the delta) only when hovered so the
     * hovered widget wins regardless of widget update order. */
    if (s->mx >= e->x && s->mx < e->x + e->w &&
        s->my >= e->y && s->my < e->y + e->h) {
        int64_t wh = cssc_video_wheel(s->vid);
        if (wh) {
            e->top_line -= (int)wh * 3;
            if (e->top_line < 0) e->top_line = 0;
            if (e->top_line >= e->nlines) e->top_line = e->nlines - 1;
        }
    }
    ed_sig_scan(e);                /* signature help: caret inside a call? */
    return 0;
}

/* ---- CSSC syntax highlighter (token kinds + brand colours) -------------- */
enum { TK_IDENT, TK_KW, TK_TYPE, TK_NS, TK_FUNC, TK_DIR,
       TK_STR, TK_NUM, TK_COMMENT, TK_PUNCT, TK_COPY, TK_REF, TK_WS,
       TK_FREE, TK_VIS, TK_GENERIC, TK_LABEL, TK_MUTIER, TK_COUNT };
/* Editable syntax palette: one ARGB per token kind. Seeded with the brand
 * defaults; the color Designer (View->Design) overwrites entries here so the
 * highlighter picks up user choices live. Index by token kind. */
static int64_t g_tk_palette[TK_COUNT] = {
    (int64_t)0xFFE6E2F0,  /* TK_IDENT   — white       */
    (int64_t)0xFFE060C0,  /* TK_KW      — magenta      */
    (int64_t)0xFF4EC9B0,  /* TK_TYPE    — teal         */
    (int64_t)0xFF4EC9B0,  /* TK_NS      — teal         */
    (int64_t)0xFF4DA6FF,  /* TK_FUNC    — blue         */
    (int64_t)0xFFC77DFF,  /* TK_DIR     — purple       */
    (int64_t)0xFFF0A0C0,  /* TK_STR     — rose         */
    (int64_t)0xFFB5CEA8,  /* TK_NUM     — soft green   */
    (int64_t)0xFF6E7A6A,  /* TK_COMMENT — grey         */
    (int64_t)0xFFB0AAC0,  /* TK_PUNCT   — muted        */
    (int64_t)0xFFFFD700,  /* TK_COPY    — gold (&)      */
    (int64_t)0xFFFFD700,  /* TK_REF     — gold (*)      */
    (int64_t)0xFFE6E2F0,  /* TK_WS      — (unused)      */
    (int64_t)0xFFFF9E64,  /* TK_FREE    — free block: warm orange   */
    (int64_t)0xFFE5C07B,  /* TK_VIS     — private:/public: amber     */
    (int64_t)0xFF9CDCFE,  /* TK_GENERIC — <> params: light blue      */
    (int64_t)0xFFD7BA7D,  /* TK_LABEL   — object labels: sand        */
    (int64_t)0xFF90EE90,  /* TK_MUTIER  — % mutier: light green       */
};
static int64_t tk_color(int t) {
    if (t >= 0 && t < TK_COUNT) return g_tk_palette[t];
    return g_tk_palette[TK_IDENT];
}
/* Color Designer hook: overwrite a token kind's colour live. The screen arg is
 * unused (the palette is process-global) but keeps the gui::Screen method shape.
 * Out-of-range kinds are ignored. */
CSSC_GUI_EXPORT void cssc_gui_screen_set_syntax_color(void* p, int64_t kind, int64_t argb) {
    (void)p;
    if (kind >= 0 && kind < TK_COUNT) g_tk_palette[(int)kind] = argb;
}
/* Number of editable token kinds the Designer lists. */
CSSC_GUI_EXPORT int64_t cssc_gui_screen_syntax_count(void* p) { (void)p; return (int64_t)TK_COUNT; }
/* Current colour of a token kind (so the Designer shows what's active). */
CSSC_GUI_EXPORT int64_t cssc_gui_screen_syntax_color(void* p, int64_t kind) {
    (void)p; return (kind >= 0 && kind < TK_COUNT) ? g_tk_palette[(int)kind] : 0;
}
/* Palette persistence: %APPDATA%/CSSC/syntax_palette.bin (raw int64 array). Saved
 * by the Designer's Save button, loaded once at IDE start so custom colours stick. */
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
/* Track /* … *​/ block-comment state across lines. Returns 1 if the line ENDS
 * inside a block comment. `in` = whether it STARTS inside one. A `//` line
 * comment ends the scan (its rest can't open/close a block). */
static int csc_scan_block(const char* line, int in) {
    int n = (int)strlen(line), i = 0;
    while (i < n) {
        if (in) {
            if (line[i] == '*' && i + 1 < n && line[i + 1] == '/') { in = 0; i += 2; continue; }
            i++;
        } else {
            if (line[i] == '/' && i + 1 < n && line[i + 1] == '*') { in = 1; i += 2; continue; }
            if (line[i] == '/' && i + 1 < n && line[i + 1] == '/') return in;
            if (line[i] == '"') { i++; while (i < n && line[i] != '"') { if (line[i] == '\\' && i + 1 < n) i++; i++; } if (i < n) i++; continue; }
            i++;
        }
    }
    return in;
}

/* Render a styled line comment `s` (starting at "//"). baseColor is the comment
 * colour (grey, or the prefix colour for //!,//?,//$,//%,//& / doc-grey for //d).
 * `mstart` is where content begins (2 for plain "//", 3 when a prefix/marker byte
 * follows). When `conceal`==0 (the cursor is on this line) the raw text is drawn
 * flat so the markup stays editable. When `conceal`==1 (inactive line) the markup
 * is HIDDEN and applied: the "//" + marker collapse to "//", `**bold**` renders
 * bold, `~~italic~~` italic, `\r<R>g<G>b<B>` sets a colour run (\r0g0b0 resets to
 * baseColor). True conceal — visible text reflows, so cx advances only for drawn
 * glyphs. */
/* Draw s[start..end) applying the bold, italic and colour-run styling with the
 * markup CONCEALED (the visible text reflows: cx advances only for drawn glyphs).
 * One shared parser for both line and block comments. A zero-RGB code resets to
 * baseColor. Markup: double-asterisk = bold, double-tilde = italic, backslash-r
 * R g G b B = colour. */
static void ed_draw_styled_span(void* v, int64_t x, int64_t y, const char* s,
                                int start, int end, int64_t baseColor,
                                int64_t glyph, int64_t scale) {
    int64_t cx = x, color = baseColor; int bold = 0, ital = 0;
    char run[512]; int rn = 0;
    int i = start;
    while (i < end) {
        if (s[i] == '*' && i + 1 < end && s[i + 1] == '*') {
            if (rn) { run[rn] = 0; cssc_video_draw_text_styled(v, cx, y, run, color, scale, (bold ? 1 : 0) | (ital ? 2 : 0)); cx += (int64_t)rn * glyph; rn = 0; }
            bold = !bold; i += 2;
        } else if (s[i] == '~' && i + 1 < end && s[i + 1] == '~') {
            if (rn) { run[rn] = 0; cssc_video_draw_text_styled(v, cx, y, run, color, scale, (bold ? 1 : 0) | (ital ? 2 : 0)); cx += (int64_t)rn * glyph; rn = 0; }
            ital = !ital; i += 2;
        } else if (s[i] == '\\' && i + 1 < end && s[i + 1] == 'r') {
            int j = i + 2, R = 0, G = 0, B = 0, ok = 0;
            while (j < end && s[j] >= '0' && s[j] <= '9') R = R * 10 + (s[j++] - '0');
            if (j < end && s[j] == 'g') { j++;
                while (j < end && s[j] >= '0' && s[j] <= '9') G = G * 10 + (s[j++] - '0');
                if (j < end && s[j] == 'b') { j++;
                    while (j < end && s[j] >= '0' && s[j] <= '9') B = B * 10 + (s[j++] - '0');
                    ok = 1; } }
            if (ok) {
                if (rn) { run[rn] = 0; cssc_video_draw_text_styled(v, cx, y, run, color, scale, (bold ? 1 : 0) | (ital ? 2 : 0)); cx += (int64_t)rn * glyph; rn = 0; }
                color = (R == 0 && G == 0 && B == 0) ? baseColor :
                        (int64_t)0xFF000000 | ((int64_t)(R & 255) << 16) |
                        ((int64_t)(G & 255) << 8) | (int64_t)(B & 255);
                i = j;
            } else { if (rn < 511) run[rn++] = s[i]; i++; }
        } else { if (rn < 511) run[rn++] = s[i]; i++; }
    }
    if (rn) { run[rn] = 0; cssc_video_draw_text_styled(v, cx, y, run, color, scale, (bold ? 1 : 0) | (ital ? 2 : 0)); }
}

static void ed_draw_comment_hl(void* v, int64_t x, int64_t y, const char* s,
                               int mstart, int64_t baseColor, int64_t glyph,
                               int64_t scale, int conceal) {
    if (!conceal) { cssc_video_draw_text(v, x, y, s, baseColor, scale); return; }
    cssc_video_draw_text(v, x, y, "//", baseColor, scale);   /* opener; marker hidden */
    ed_draw_styled_span(v, x + 2 * glyph, y, s, mstart, (int)strlen(s), baseColor, glyph, scale);
}

/* Draw a block-comment segment s[0..seglen) - grey base, styled + concealed on
 * inactive lines, raw on the caret line. The opener/closer delimiters carry no
 * markup so they render plainly when raw. */
static void ed_draw_block_seg(void* v, int64_t x, int64_t y, const char* s, int seglen,
                              int64_t baseColor, int64_t glyph, int64_t scale, int conceal) {
    if (!conceal) {
        char tmp[512]; int nn = seglen < 511 ? seglen : 511;
        memcpy(tmp, s, (size_t)nn); tmp[nn] = 0;
        cssc_video_draw_text(v, x, y, tmp, baseColor, scale);
        return;
    }
    ed_draw_styled_span(v, x, y, s, 0, seglen, baseColor, glyph, scale);
}

/* Draw one source line with CSSC token colours, clipped to the visible
 * column window [left_col, left_col+max_cols). `active` = 1 when the caret is on
 * this line (styled comments render raw for editing). `csc_block` threads the
 * /* … *​/ block-comment state across lines. */
static void ed_draw_line_hl(cssc_gui_editor* e, void* v, const char* line,
                            int64_t ly, int64_t text_x, int64_t glyph,
                            int max_cols, int active, int* csc_block) {
    int n = (int)strlen(line);
    int i = 0;
    char buf[512];
    int first_nonws = 0;                     /* first non-blank column (for label detection) */
    while (first_nonws < n && (line[first_nonws] == ' ' || line[first_nonws] == '\t')) first_nonws++;
    if (*csc_block) {                        /* this line opened inside a /* … *​/ block */
        int end = 0;
        while (end < n && !(line[end] == '*' && end + 1 < n && line[end + 1] == '/')) end++;
        int cend;
        if (end < n) { cend = end + 2; *csc_block = 0; } else { cend = n; }
        if (e->left_col == 0) {
            ed_draw_block_seg(v, text_x, ly, line, cend, tk_color(TK_COMMENT), glyph, e->scale, active ? 0 : 1);
        } else {
            int cs = 0, ce = cend;
            if (cs < e->left_col) cs = e->left_col;
            if (ce > e->left_col + max_cols) ce = e->left_col + max_cols;
            int cnt = ce - cs;
            if (cnt > 0 && cnt < 512) {
                memcpy(buf, line + cs, (size_t)cnt); buf[cnt] = 0;
                cssc_video_draw_text(v, text_x + (int64_t)(cs - e->left_col) * glyph, ly, buf,
                                     tk_color(TK_COMMENT), e->scale);
            }
        }
        i = cend;
    }
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
        } else if (c == '/' && i + 1 < n && line[i + 1] == '*') {
            int end = i + 2;
            while (end < n && !(line[end] == '*' && end + 1 < n && line[end + 1] == '/')) end++;
            int cend;
            if (end < n) { cend = end + 2; } else { cend = n; *csc_block = 1; }
            if (i >= e->left_col) {
                ed_draw_block_seg(v, text_x + (int64_t)(i - e->left_col) * glyph, ly,
                                  line + i, cend - i, tk_color(TK_COMMENT), glyph, e->scale,
                                  active ? 0 : 1);
            } else {
                int cs = i, ce = cend;
                if (cs < e->left_col) cs = e->left_col;
                if (ce > e->left_col + max_cols) ce = e->left_col + max_cols;
                int cnt = ce - cs;
                if (cnt > 0 && cnt < 512) {
                    memcpy(buf, line + cs, (size_t)cnt); buf[cnt] = 0;
                    cssc_video_draw_text(v, text_x + (int64_t)(cs - e->left_col) * glyph, ly, buf,
                                         tk_color(TK_COMMENT), e->scale);
                }
            }
            i = cend; continue;
        } else if (c == '/' && i + 1 < n && line[i + 1] == '/') {
            int64_t base = tk_color(TK_COMMENT);
            int mstart = 2;
            if (i + 2 < n) {
                char m = line[i + 2];
                if      (m == '!') { base = (int64_t)0xFFE0605A; mstart = 3; }   /* red    */
                else if (m == '?') { base = (int64_t)0xFF4DA6FF; mstart = 3; }   /* blue   */
                else if (m == '$') { base = (int64_t)0xFFFFD700; mstart = 3; }   /* gold   */
                else if (m == '%') { base = (int64_t)0xFFE060C0; mstart = 3; }   /* magenta*/
                else if (m == '&') { base = (int64_t)0xFF5FE0A0; mstart = 3; }   /* green  */
                else if (m == 'd') { base = (int64_t)0xFFAEB6C6; mstart = 3; }   /* docstr */
            }
            if (i >= e->left_col) {
                ed_draw_comment_hl(v, text_x + (int64_t)(i - e->left_col) * glyph, ly,
                                   line + i, mstart, base, glyph, e->scale, active ? 0 : 1);
            } else {
                int cs = i, ce = n;
                if (cs < e->left_col) cs = e->left_col;
                if (ce > e->left_col + max_cols) ce = e->left_col + max_cols;
                int cnt = ce - cs;
                if (cnt > 0 && cnt < 512) {
                    memcpy(buf, line + cs, (size_t)cnt); buf[cnt] = 0;
                    cssc_video_draw_text(v, text_x + (int64_t)(cs - e->left_col) * glyph, ly, buf,
                                         base, e->scale);
                }
            }
            i = n; continue;
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
            if (len == 4 && !strncmp(id, "free", 4)) t = TK_FREE;   /* free-block keyword */
            else if (is_vis && single_colon) t = TK_VIS;            /* private: / public: */
            else if (tk_is_kw(id, len)) t = TK_KW;
            else if (tk_is_type(id, len)) t = TK_TYPE;
            else if (j + 1 < n && line[j] == ':' && line[j + 1] == ':') t = TK_NS;
            else if (j < n && line[j] == '(') t = TK_FUNC;
            else if (single_colon && start == first_nonws) t = TK_LABEL;  /* `name:` at line start = object/sector label */
            else t = TK_IDENT;
        } else if (c == '&') {
            i++; t = TK_COPY;
        } else if (c == '*') {
            i++; t = TK_REF;
        } else if (c == '%') {
            i++; t = TK_MUTIER;              /* % mutier — light green */
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

/* Shared token-emit tail for the asm/cproject highlighters: clip [start,i) to
 * the visible column window and draw it in the token's colour. */
static void ed_emit_tok(cssc_gui_editor* e, void* v, const char* line,
                        int start, int i, int t, int64_t ly, int64_t text_x,
                        int64_t glyph, int max_cols) {
    char buf[512];
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

/* x86 GAS assembly highlighter (language 2): directives (.foo) purple,
 * registers (%reg) teal, immediates ($imm)/numbers green, labels (foo:) sand,
 * mnemonics (first word on the line) magenta, '#' comments grey, strings rose.
 * The embedded `# CSSC_RT_SRC_*` runtime-source header reads as comments. */
static void ed_draw_line_hl_asm(cssc_gui_editor* e, void* v, const char* line,
                                int64_t ly, int64_t text_x, int64_t glyph,
                                int max_cols) {
    int n = (int)strlen(line);
    int i = 0;
    int first_nonws = 0;
    while (first_nonws < n && (line[first_nonws] == ' ' || line[first_nonws] == '\t')) first_nonws++;
    while (i < n) {
        char c = line[i];
        int start = i;
        int t;
        if (c == ' ' || c == '\t') {
            while (i < n && (line[i] == ' ' || line[i] == '\t')) i++;
            continue;
        } else if (c == '#') {
            i = n; t = TK_COMMENT;
        } else if (c == '/' && i + 1 < n && line[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(line[i] == '*' && line[i + 1] == '/')) i++;
            if (i + 1 < n) i += 2; else i = n;
            t = TK_COMMENT;
        } else if (c == '"') {
            i++;
            while (i < n && line[i] != '"') { if (line[i] == '\\' && i + 1 < n) i++; i++; }
            if (i < n) i++;
            t = TK_STR;
        } else if (c == '.' && i + 1 < n && tk_isidstart(line[i + 1])) {
            i++;
            while (i < n && (tk_isident(line[i]) || line[i] == '.')) i++;
            t = TK_DIR;
        } else if (c == '%') {
            i++;
            while (i < n && tk_isident(line[i])) i++;
            t = TK_TYPE;
        } else if (c == '$') {
            i++;
            while (i < n && (tk_isident(line[i]) || line[i] == '.' || line[i] == '-')) i++;
            t = TK_NUM;
        } else if (c >= '0' && c <= '9') {
            while (i < n && (tk_isident(line[i]) || line[i] == '.')) i++;
            t = TK_NUM;
        } else if (tk_isidstart(c) || c == '@') {
            i++;
            while (i < n && (tk_isident(line[i]) || line[i] == '.' || line[i] == '@')) i++;
            int j = i; while (j < n && line[j] == ' ') j++;
            if (j < n && line[j] == ':') t = TK_LABEL;
            else if (start == first_nonws) t = TK_KW;
            else t = TK_IDENT;
        } else {
            i++; t = TK_PUNCT;
        }
        ed_emit_tok(e, v, line, start, i, t, ly, text_x, glyph, max_cols);
    }
}

/* cssc.cproject config highlighter (language 3): keys (LHS identifiers, dotted)
 * blue, true/false magenta, strings rose, numbers green, '//'+'#' comments grey,
 * '='/'+='/brackets muted. */
static void ed_draw_line_hl_cproject(cssc_gui_editor* e, void* v, const char* line,
                                     int64_t ly, int64_t text_x, int64_t glyph,
                                     int max_cols) {
    int n = (int)strlen(line);
    int i = 0;
    int seen_eq = 0;
    while (i < n) {
        char c = line[i];
        int start = i;
        int t;
        if (c == ' ' || c == '\t') {
            while (i < n && (line[i] == ' ' || line[i] == '\t')) i++;
            continue;
        } else if (c == '#') {
            i = n; t = TK_COMMENT;
        } else if (c == '/' && i + 1 < n && line[i + 1] == '/') {
            i = n; t = TK_COMMENT;
        } else if (c == '"' || c == '\'') {
            char q = c; i++;
            while (i < n && line[i] != q) { if (line[i] == '\\' && i + 1 < n) i++; i++; }
            if (i < n) i++;
            t = TK_STR;
        } else if (c >= '0' && c <= '9') {
            while (i < n && (tk_isident(line[i]) || line[i] == '.')) i++;
            t = TK_NUM;
        } else if (tk_isidstart(c)) {
            while (i < n && (tk_isident(line[i]) || line[i] == '.')) i++;
            int len = i - start;
            const char* id = line + start;
            if ((len == 4 && !strncmp(id, "true", 4)) ||
                (len == 5 && !strncmp(id, "false", 5))) t = TK_KW;
            else if (!seen_eq) t = TK_FUNC;
            else t = TK_IDENT;
        } else if (c == '+' && i + 1 < n && line[i + 1] == '=') {
            seen_eq = 1; i += 2; t = TK_PUNCT;
        } else if (c == '=') {
            seen_eq = 1; i++; t = TK_PUNCT;
        } else {
            i++; t = TK_PUNCT;
        }
        ed_emit_tok(e, v, line, start, i, t, ly, text_x, glyph, max_cols);
    }
}

/* Identify the variable a line allocates (#stack/#heap/#auto[…] NAME) or frees
 * (#delete[NAME]) — for the alloc↔dealloc cursor highlighter. Only these four
 * directives match, so #define(var)/#req[var] never false-mark. Returns
 * 1 = alloc, 2 = delete, 0 = neither; NAME copied into out. */
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

/* Draw a docstring line, interpreting inline colour codes as coloured runs:
 *   \c0..3        -> red / green / blue / purple
 *   \r<R>g<G>b<B> -> that RGB (\r0g0b0 = reset to doc-grey)
 * Codes are consumed (not shown). Monospace, so x advances per glyph. */
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

/* If a hover line reads "rgb(R, G, B)" (emitted by the sema for a hex literal),
 * return 0xRRGGBB so the hover box can paint a color swatch; else -1. */
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
    /* Horizontal auto-scroll: keep the caret column in view. */
    if (e->cur_col < e->left_col) e->left_col = e->cur_col;
    if (e->cur_col >= e->left_col + max_cols) e->left_col = e->cur_col - max_cols + 1;
    if (e->left_col < 0) e->left_col = 0;
    int sl = 0, sc = 0, el = 0, ec = 0;
    if (e->sel_active) ed_sel_norm(e, &sl, &sc, &el, &ec);
    char active[128]; active[0] = 0;
    int active_kind = 0;
    /* ---- adhelper (alloc<->dealloc highlighter) draw ---- see cssc_gui_editor_
     * setownership for the data model. Highlights the cursor's alloc/dealloc
     * group: magenta when freed, orange-red when it has no #delete (leak). */
    int useMap = (e->own_na + e->own_nd) > 0;   /* sema map loaded? */
    int cursorGid = -1;                          /* alloc line of the cursor's group */
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
    int csc_block = 0;
    if (e->language == 1) {
        int top = e->top_line; if (top > e->nlines) top = e->nlines;
        for (int pl = 0; pl < top; pl++) csc_block = csc_scan_block(e->lines[pl], csc_block);
    }
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
        if (e->ip_line == li + 1) {          /* F5 debugger instruction pointer */
            cssc_video_fillrect(v, e->x + 1, ly - 1, e->w - 2, line_h, (int64_t)0x40FF3B54);
            cssc_video_fillrect(v, e->x + 1, ly - 1, 3 * e->scale, line_h, (int64_t)0xFFFF3B54);
        }
        for (int cm = 0; cm < e->clean_nmarks; cm++) {   /* F10 autoclean insertions */
            if (e->clean_marks[cm] == li + 1) {
                cssc_video_fillrect(v, e->x + 1, ly - 1, e->w - 2, line_h, (int64_t)0x385FE0A0);
                cssc_video_fillrect(v, e->x + 1, ly - 1, 3 * e->scale, line_h, (int64_t)0xFF5FE0A0);
                break;
            }
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
            ed_draw_line_hl(e, v, line, ly, text_x, glyph, max_cols,
                            (li == e->cur_line) ? 1 : 0, &csc_block);
        } else if (e->language == 2) {
            ed_draw_line_hl_asm(e, v, line, ly, text_x, glyph, max_cols);
        } else if (e->language == 3) {
            ed_draw_line_hl_cproject(e, v, line, ly, text_x, glyph, max_cols);
        } else {
            int from = e->left_col; if (from > slen) from = slen;
            int cnt = slen - from; if (cnt > max_cols) cnt = max_cols;
            if (cnt > 0) { memcpy(sub, line + from, (size_t)cnt); sub[cnt] = 0; }
            else sub[0] = 0;
            cssc_video_draw_text(v, text_x, ly, sub, e->fg, e->scale);
        }
        /* squiggle underlines for diagnostics on this line */
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
        /* sticky F1 marks (persist until F2): a left-edge bar + faint line tint */
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
    /* ---- completion popup (glass list, anchored under the token) ---- */
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
            ay = e->y + 5 + caretRow * line_h - ph - 2;     /* flip above */
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
    /* ---- hover box (glass, magenta rim; anchored under the hovered word) ---- */
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
        int64_t swc = ed_hover_color(e->hov_text);   /* color swatch for hex literals */
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
        if (swc >= 0) {                              /* paint the color swatch, top-right */
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
    /* ---- signature box (glass, blue rim; above the caret, active arg lit) ---- */
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
    /* ---- sticky-diagnostic hover: mouse over a marked line -> its message ---- */
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

/* ---- List (scrollable, selectable item list — the file tree) ------------ */
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

/* ---- Tree (filesystem-backed explorer: folders + expand/collapse) ------- */
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
    if (n[0] == '.') return 1;                 /* hidden / .git / .vscode / … */
    for (int i = 0; sk[i]; i++) if (!strcmp(n, sk[i])) return 1;
    return 0;
}
static int tree_is_open(cssc_gui_tree* t, const char* path) {
    for (int i = 0; i < t->fold_n; i++)
        if (!strcmp(t->fold_paths[i], path)) return t->fold_open[i];
    return 0;   /* default collapsed */
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
    long long* nz = (long long*)realloc(t->sizes, (size_t)nc * sizeof(long long));
    if (np) t->paths = np;
    if (nn) t->names = nn;
    if (nd) t->depths = nd;
    if (ni) t->isdir = ni;
    if (nz) t->sizes = nz;
    if (np && nn && nd && ni && nz) t->cap = nc;
}
static void tree_emit(cssc_gui_tree* t, const char* full, const char* name,
                      int depth, int isdir) {
    tree_rows_grow(t, t->nrows + 1);
    if (t->cap < t->nrows + 1) return;
    t->paths[t->nrows] = gui_strdup(full);
    t->names[t->nrows] = gui_strdup(name);
    t->depths[t->nrows] = depth;
    t->isdir[t->nrows] = isdir;
    long long sz = -1;
    if (!isdir) {
#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if (GetFileAttributesExA(full, GetFileExInfoStandard, &fa))
            sz = ((long long)fa.nFileSizeHigh << 32) | (long long)fa.nFileSizeLow;
#else
        struct stat st;
        if (stat(full, &st) == 0) sz = (long long)st.st_size;
#endif
    }
    t->sizes[t->nrows] = sz;
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
    char** dn = NULL; int nd = 0, cd = 0;   /* subdirectories */
    char** fn = NULL; int nf = 0, cf = 0;   /* files */
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
    cssc_gui_tree* t = (cssc_gui_tree*)p;
    if (t) {
        t->x = x; t->y = y; t->h = h;
        /* Preserve a user-dragged width: once the panel has been resized, the
         * host's per-frame layout should not stomp it back to the default. */
        if (!t->resizing) t->w = w;
    }
}
CSSC_GUI_EXPORT int64_t cssc_gui_tree_width(void* p) {
    cssc_gui_tree* t = (cssc_gui_tree*)p; return t ? t->w : 0;
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
    t->ico_loaded = 0;   /* (re)load on next draw */
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
/* drag-and-drop move: 1 when a row was just dropped onto a folder. The host
 * reads dropSrc()/dropDst(), performs the move, and refresh()es. */
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
    /* Right-edge resize handle: a 6px grip at the panel's right border lets the
     * user drag the whole file panel wider/narrower. Handled before row logic so
     * an edge drag never selects a row. */
    int on_edge = (!s->input_captured &&
                   s->mx >= t->x + t->w - 4 && s->mx <= t->x + t->w + 4 &&
                   s->my >= t->y && s->my < t->y + t->h);
    if (s->down && !s->prev_down && on_edge && !t->drag_on) t->resizing = 1;
    if (t->resizing) {
        if (s->down) {
            int64_t nw = s->mx - t->x;
            if (nw < 140) nw = 140;
            if (nw > 900) nw = 900;
            t->w = nw;
        } else {
            t->resizing = 0;
        }
    }
    int inside = (!t->resizing && !s->input_captured && s->mx >= t->x && s->mx < t->x + t->w &&
                  s->my >= t->y && s->my < t->y + t->h);
    if (t->drop_ready) t->drop_ready = 0;   /* one-frame signal, cleared next update */
    if (!t->resizing && s->down && !s->prev_down && inside) {
        int row = (int)((s->my - t->y - 2) / row_h);
        int idx = t->top + row;
        if (idx >= 0 && idx < t->nrows) {
            t->selected = idx;
            t->drag_on = 1; t->drag_idx = idx; t->drag_active = 0;
            t->drag_dx = (int)s->mx; t->drag_dy = (int)s->my;
        }
    }
    if (s->down && t->drag_on) {                 /* passed the drag threshold? */
        int ax = (int)s->mx - t->drag_dx; if (ax < 0) ax = -ax;
        int ay = (int)s->my - t->drag_dy; if (ay < 0) ay = -ay;
        if (ax > 5 || ay > 5) t->drag_active = 1;
    }
    if (!s->down && s->prev_down && t->drag_on) {   /* released */
        if (t->drag_active) {                        /* a drag -> drop onto a folder */
            if (inside) {
                int row = (int)((s->my - t->y - 2) / row_h);
                int tidx = t->top + row;
                if (tidx >= 0 && tidx < t->nrows && tidx != t->drag_idx &&
                    t->isdir[tidx] && t->drag_idx < t->nrows) {
                    const char* src = t->paths[t->drag_idx];
                    const char* dst = t->paths[tidx];
                    size_t sl = strlen(src);
                    /* never move a folder into itself / its own subtree */
                    if (!(strncmp(dst, src, sl) == 0 && (dst[sl] == '/' || dst[sl] == '\\' || dst[sl] == 0))) {
                        snprintf(t->drop_src, sizeof(t->drop_src), "%s", src);
                        snprintf(t->drop_dst, sizeof(t->drop_dst), "%s", dst);
                        t->drop_ready = 1;
                    }
                }
            }
        } else {                                     /* a click -> activate the row */
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
            else if (k == 0x25) {   /* Left: collapse the folder under caret */
                if (t->selected < t->nrows && t->isdir[t->selected] &&
                    tree_is_open(t, t->paths[t->selected])) {
                    tree_set_open(t, t->paths[t->selected], 0); tree_rebuild(t);
                }
            } else if (k == 0x27) { /* Right: expand the folder under caret */
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
/* Data-driven extension icon: loads `<ext>.png` from the icon dir once and
 * caches it (NULL cached too, so a missing icon is not retried each frame).
 * `ext` is the lowercase extension without the dot, e.g. "dll". */
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
    /* the CSSC project manifest gets its own icon (matched by full name). */
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
        /* Data-driven: any `<ext>.png` the user drops in the asset dir (e.g.
         * `dll.png`) is used for `.<ext>` files automatically. */
        void* de = tree_ext_icon(t, dot);
        if (de) return de;
    }
    return t->ico_file;
}

/* Blit a loaded sprite at (x,y) scaled, skipping transparent pixels. */
static void gui_blit_sprite(void* v, void* icon, int64_t x, int64_t y, int64_t scale) {
    if (!icon) return;
    int64_t iw = cssc_sprite_width(icon), ih = cssc_sprite_height(icon);
    for (int64_t sy = 0; sy < ih; ++sy)
        for (int64_t sx = 0; sx < iw; ++sx) {
            int64_t argb = cssc_sprite_get_pixel(icon, sx, sy);
            if (((uint64_t)argb >> 24) == 0) continue;   /* transparent */
            cssc_video_fillrect(v, x + sx * scale, y + sy * scale, scale, scale, argb);
        }
}

/* Blit an icon FITTED into a box×box display area (nearest-neighbour), so an
 * 8×8 (default), 16×16, or 32×32 source all render at the same on-screen size.
 * For an 8×8 source with box = 8*scale this matches gui_blit_sprite(scale). */
static void gui_blit_sprite_fit(void* v, void* icon, int64_t x, int64_t y, int64_t box) {
    if (!icon || box <= 0) return;
    int64_t iw = cssc_sprite_width(icon), ih = cssc_sprite_height(icon);
    if (iw <= 0 || ih <= 0) return;
    for (int64_t dy = 0; dy < box; ++dy)
        for (int64_t dx = 0; dx < box; ++dx) {
            int64_t sx = dx * iw / box, sy = dy * ih / box;
            int64_t argb = cssc_sprite_get_pixel(icon, sx, sy);
            if (((uint64_t)argb >> 24) == 0) continue;   /* transparent */
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
    /* resize grip on the right edge (brighter while actively dragging) */
    cssc_video_fillrect(v, t->x + t->w - 2, t->y + 2, 2, t->h - 4,
                        t->resizing ? (int64_t)0xFFC74DE0 : (int64_t)0x40C74DE0);
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
            /* folder expand state: the arrow_opened/closed textures if present,
             * else the drawn chevron. */
            int open = tree_is_open(t, t->paths[idx]);
            void* arrow = open ? t->ico_arrow_open : t->ico_arrow_closed;
            if (t->ico_loaded && arrow) gui_blit_sprite_fit(v, arrow, bx, iy, 8 * t->scale);
            else tree_chevron(v, bx, iy, glyph, open, (int64_t)0xFF8A84A0);
        }
        int64_t ix = bx + glyph;
        void* icon = t->ico_loaded ? tree_pick_icon(t, idx) : NULL;
        if (icon) {
            /* Fit into an 8×8 (base) cell — a 16×16 / 32×32 png scales down. */
            gui_blit_sprite_fit(v, icon, ix, iy, 8 * t->scale);
        } else {
            int64_t isz = glyph / 2; if (isz < 4) isz = 4;
            int64_t icol = t->isdir[idx] ? (int64_t)0xFFE0B24B
                                         : list_icon_color(t->names[idx]);
            cssc_video_fillrect(v, ix, ry + (row_h - isz) / 2, isz, isz, icol);
        }
        {
            /* Name clipped to the panel's right edge with an ellipsis so long
             * file names never bleed out of the (resizeable) tree; a compact
             * right-aligned KB/MB size is shown too when the row is wide enough
             * to keep a readable slice of the name beside it. */
            int64_t cw = 8 * t->scale;
            int64_t namex = ix + glyph + 2;
            int64_t right = t->x + t->w - 6;
            const char* nm = t->names[idx];

            char szbuf[24]; szbuf[0] = 0; int64_t szw = 0;
            if (!t->isdir[idx] && t->sizes[idx] >= 0) {
                long long b = t->sizes[idx];
                if (b < 1024LL)
                    snprintf(szbuf, sizeof(szbuf), "%lld B", b);
                else if (b < 1024LL * 1024)
                    snprintf(szbuf, sizeof(szbuf), "%.1f KB", (double)b / 1024.0);
                else if (b < 1024LL * 1024 * 1024)
                    snprintf(szbuf, sizeof(szbuf), "%.1f MB", (double)b / (1024.0 * 1024.0));
                else
                    snprintf(szbuf, sizeof(szbuf), "%.1f GB", (double)b / (1024.0 * 1024.0 * 1024.0));
                szw = (int64_t)strlen(szbuf) * cw;
            }

            int64_t name_right = right;
            if (szbuf[0] && (right - szw - cw * 2 - namex) >= cw * 5) {
                name_right = right - szw - cw * 2;   /* 2-char gap before size */
                int64_t szcol = (idx == t->selected) ? t->sel_fg : (int64_t)0xFF8A84A0;
                cssc_video_draw_text(v, right - szw, ry + 2, szbuf, szcol, t->scale);
            }

            int64_t maxch = cw > 0 ? (name_right - namex) / cw : 0;
            if (maxch > 0) {
                int nlen = (int)strlen(nm);
                if ((int64_t)nlen <= maxch) {
                    cssc_video_draw_text(v, namex, ry + 2, nm, col, t->scale);
                } else {
                    char buf[300];
                    int keep = (int)maxch;
                    if (keep > (int)sizeof(buf) - 1) keep = (int)sizeof(buf) - 1;
                    if (keep >= 4) {
                        memcpy(buf, nm, (size_t)(keep - 3));
                        buf[keep - 3] = '.'; buf[keep - 2] = '.';
                        buf[keep - 1] = '.'; buf[keep] = 0;
                    } else {
                        memcpy(buf, nm, (size_t)keep); buf[keep] = 0;
                    }
                    cssc_video_draw_text(v, namex, ry + 2, buf, col, t->scale);
                }
            }
        }
    }
}

/* ---- Terminal (scrollback + input line + async child process) ----------- */
#define TERM_MAX_LINES 5000
/* A single scrollback line is clamped to this many bytes. A runaway program
 * (e.g. a `while` loop printing without newlines, or one very long line) can
 * otherwise hand us an enormous string; the on-screen font only ever renders a
 * few hundred columns, so anything past this is invisible weight. */
#define TERM_MAX_LINE_LEN 2048
/* When the scrollback overflows TERM_MAX_LINES we drop this many oldest lines
 * at once instead of one-per-append, so the O(n) compaction amortises to O(1)
 * under a fast producer (loop output) rather than memmoving every single line. */
#define TERM_DROP_BATCH 256
/* Upper bound on bytes drained from the child pipe per poll (per frame). An
 * infinite / very fast producer would otherwise keep `avail > 0` forever and
 * spin term_poll without returning — freezing the whole IDE (the reported
 * "loops crash the IDE"). Past the budget we stop; the OS pipe buffers the rest
 * (back-pressuring the child) and we resume next frame, keeping the UI live. */
#define TERM_POLL_BUDGET (256 * 1024)
#define TERM_COL_ECHO  ((int64_t)0xFFC74DE0)   /* command echo — magenta */
#define TERM_COL_OK    ((int64_t)0xFF5FE0A0)   /* success exit — green   */
#define TERM_COL_ERR   ((int64_t)0xFFFF5FB0)   /* failure / error — pink */
#define TERM_COL_SYS   ((int64_t)0xFF8A84A0)   /* system notes — muted   */

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
    if (!s) s = "";
    /* clamp per-line length so a runaway line can't balloon the heap */
    size_t sl = strlen(s);
    char* dup;
    if (sl > TERM_MAX_LINE_LEN) {
        dup = (char*)malloc((size_t)TERM_MAX_LINE_LEN + 1);
        if (dup) { memcpy(dup, s, (size_t)TERM_MAX_LINE_LEN); dup[TERM_MAX_LINE_LEN] = 0; }
    } else {
        dup = gui_strdup(s);
    }
    if (!dup) return;
    t->lines[t->nlines] = dup;
    t->line_col[t->nlines] = col;
    t->nlines++;
    if (t->nlines > TERM_MAX_LINES) {
        /* trim a chunk of the oldest lines (amortised O(1) under loop output) */
        int drop = t->nlines - (TERM_MAX_LINES - TERM_DROP_BATCH);
        if (drop < 1) drop = 1;
        if (drop > t->nlines) drop = t->nlines;
        for (int i = 0; i < drop; i++) free(t->lines[i]);
        memmove(t->lines, t->lines + drop, (size_t)(t->nlines - drop) * sizeof(char*));
        memmove(t->line_col, t->line_col + drop,
                (size_t)(t->nlines - drop) * sizeof(int64_t));
        t->nlines -= drop;
        t->top -= drop;                 /* keep a scrolled-up view anchored */
        if (t->top < 0) t->top = 0;
    }
    t->stick = 1;   /* new output pins the view to the bottom */
}
static void term_clear_lines(cssc_gui_terminal* t) {
    for (int i = 0; i < t->nlines; i++) free(t->lines[i]);
    t->nlines = 0; t->top = 0; t->stick = 1;
}
static int term_is_cssc_cmd(const char* w) {
    static const char* const c[] = {"build", "run", "hsim", "module", "doc",
        "help", "settings", "new", "flash", "test", "convert", "release",
        "update", "configure", "introspect", "workspace", "vscode",
        "analyze", "diagnostics", "assembly", "aseprite", "shell",
        "pctrace", "transpile", 0};
    for (int i = 0; c[i]; i++) if (!strcmp(w, c[i])) return 1;
    return 0;
}

/* Map a Unicode codepoint to the closest printable ASCII for the 8x8 font.
 * The CLI emits a handful of box/arrow/status glyphs; everything else that is
 * non-ASCII degrades to '?' rather than byte soup. */
static char term_cp_to_ascii(unsigned int cp) {
    if (cp < 0x80) return (char)cp;
    switch (cp) {
        case 0x2192: return '>';   /* → arrow */
        case 0x2190: return '<';   /* ← */
        case 0x2191: return '^';   /* ↑ */
        case 0x2193: return 'v';   /* ↓ */
        case 0x2713: case 0x2714: return '+';   /* ✓ check */
        case 0x2717: case 0x2718: case 0x00D7: return 'x';   /* ✗ / × */
        case 0x26A0: return '!';   /* ⚠ warning */
        case 0x2022: case 0x00B7: return '*';   /* • / · */
        case 0x2500: case 0x2501: case 0x2504: return '-';   /* ─ */
        case 0x2502: case 0x2503: return '|';   /* │ */
        case 0x250C: case 0x2514: case 0x2510: case 0x2518: return '+';   /* corners */
        case 0x2026: return '.';   /* … ellipsis */
        case 0x2018: case 0x2019: return '\'';  /* ‘ ’ */
        case 0x201C: case 0x201D: return '"';   /* “ ” */
        case 0x2013: case 0x2014: return '-';   /* – — */
        default: return '?';
    }
}

/* SGR foreground code -> RGBA, on the toolchain's purple identity. */
static int64_t term_sgr_color(int code, int64_t def) {
    switch (code) {
        case 30: case 90: return (int64_t)0xFF6A6A6A;   /* black/grey */
        case 31: case 91: return (int64_t)0xFFFF5FB0;   /* red   -> pink   */
        case 32: case 92: return (int64_t)0xFF5FE0A0;   /* green           */
        case 33: case 93: return (int64_t)0xFFE0C860;   /* yellow          */
        case 34: case 94: return (int64_t)0xFF6AA8FF;   /* blue            */
        case 35:          return (int64_t)0xFFC74DE0;   /* magenta         */
        case 95:          return (int64_t)0xFFE49BFF;   /* bright magenta  */
        case 36: case 96: return (int64_t)0xFF5FD8E8;   /* cyan            */
        case 37: case 97: return (int64_t)0xFFEAEAEA;   /* white           */
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

/* Map an xterm-256 palette index to 0xAARRGGBB. */
static int64_t term_xterm256(int n) {
    if (n < 0) n = 0; if (n > 255) n = 255;
    if (n < 16) {
        static const int codes[16] = { 30,31,32,33,34,35,36,37, 90,91,92,93,94,95,96,97 };
        return term_sgr_color(codes[n], (int64_t)0xFFEAEAEA);
    }
    if (n >= 232) {                      /* 24-step grayscale ramp */
        int v = 8 + (n - 232) * 10;
        return (int64_t)(0xFF000000u | ((unsigned)v << 16) | ((unsigned)v << 8) | (unsigned)v);
    }
    n -= 16;                             /* 6x6x6 colour cube */
    int r = (n / 36) % 6, g = (n / 6) % 6, b = n % 6;
    int rr = r ? r * 40 + 55 : 0, gg = g ? g * 40 + 55 : 0, bb = b ? b * 40 + 55 : 0;
    return (int64_t)(0xFF000000u | ((unsigned)rr << 16) | ((unsigned)gg << 8) | (unsigned)bb);
}

/* Parse the accumulated CSI params as SGR; first non-reset fg wins the line.
 * Handles 24-bit truecolor (38;2;R;G;B), xterm-256 (38;5;N) and legacy codes. */
static void term_apply_sgr(cssc_gui_terminal* t) {
    t->esc_buf[t->esc_len] = 0;
    int params[16]; int np = 0;
    int num = 0, have = 0;
    for (int i = 0; ; i++) {
        char ch = t->esc_buf[i];
        if (ch >= '0' && ch <= '9') { num = num * 10 + (ch - '0'); have = 1; }
        else {
            if (np < 16) params[np++] = have ? num : 0;  /* empty param counts as 0 */
            num = 0; have = 0;
            if (ch == 0) break;
        }
    }
    for (int i = 0; i < np; i++) {
        int code = params[i];
        if (code == 38 || code == 48) {                  /* extended colour */
            int64_t rgb = -1;
            if (i + 1 < np && params[i + 1] == 2 && i + 4 < np) {
                int r = params[i + 2] & 0xFF, g = params[i + 3] & 0xFF, b = params[i + 4] & 0xFF;
                rgb = (int64_t)(0xFF000000u | ((unsigned)r << 16) | ((unsigned)g << 8) | (unsigned)b);
                i += 4;
            } else if (i + 1 < np && params[i + 1] == 5 && i + 2 < np) {
                rgb = term_xterm256(params[i + 2]);
                i += 2;
            } else { break; }                            /* malformed — stop */
            if (code == 38 && rgb != -1 && !t->line_col_set) {
                t->line_col_use = rgb; t->line_col_set = 1;
            }
            continue;                                    /* 48 = background: ignored */
        }
        if (code != 0 && !t->line_col_set) {
            int64_t c = term_sgr_color(code, t->fg);
            if (c != t->fg) { t->line_col_use = c; t->line_col_set = 1; }
        }
    }
}

/* Feed one raw output byte through the ANSI + UTF-8 filter. */
static void term_feed_byte(cssc_gui_terminal* t, unsigned char c) {
    if (t->esc_st == 1) {                 /* after ESC: '[' = CSI, ']' = OSC */
        if (c == '[')      { t->esc_st = 2; t->esc_len = 0; }
        else if (c == ']') { t->esc_st = 3; }   /* OSC (e.g. window title) */
        else               { t->esc_st = 0; }
        return;
    }
    if (t->esc_st == 2) {                 /* inside CSI: accumulate to final byte */
        if (c >= 0x40 && c <= 0x7E) {
            if (c == 'm') term_apply_sgr(t);
            else if (c == 'J') {          /* erase display — 2/3 wipe scrollback */
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
    if (t->esc_st == 3) {                 /* inside OSC: discard payload to BEL/ST */
        if (c == 0x07)      t->esc_st = 0;      /* BEL terminator */
        else if (c == 0x1b) t->esc_st = 1;      /* ST (ESC \) or next sequence */
        return;
    }
    if (t->ipm_st) {                      /* inside a marker: SOH ends it */
        if (c == 0x01) {
            t->ipm_buf[t->ipm_len] = 0;
            if (t->ipm_buf[0] == 'I') {
                /* input-wait marker: I1 = program is reading (unlock), I0 = done */
                t->input_wanted = (t->ipm_buf[1] == '1') ? 1 : 0;
            } else {
                char* sep = strchr(t->ipm_buf, 0x02);   /* IP marker: line STX file */
                if (sep) {
                    *sep = 0;
                    t->ip_line = atoi(t->ipm_buf);
                    snprintf(t->ip_file, sizeof(t->ip_file), "%s", sep + 1);
                }
            }
            t->ipm_st = 0;
        } else if (t->ipm_len < (int)sizeof(t->ipm_buf) - 1) {
            t->ipm_buf[t->ipm_len++] = (char)c;
        }
        return;                           /* never shown in the scrollback */
    }
    if (c == 0x01) { t->ipm_st = 1; t->ipm_len = 0; return; }   /* SOH starts one */
    if (c == 0x1b) { t->esc_st = 1; return; }
    if (c == '\r') return;
    if (c == '\n') { term_finalize_line(t); t->utf_need = 0; return; }
    if (t->utf_need > 0) {                /* UTF-8 continuation */
        if (c >= 0x80 && c < 0xC0) {
            t->utf_cp = (t->utf_cp << 6) | (c & 0x3F);
            if (--t->utf_need == 0) term_emit_ch(t, term_cp_to_ascii(t->utf_cp));
            return;
        }
        t->utf_need = 0;                  /* broken run — fall through on c */
    }
    if (c < 0x80) { term_emit_ch(t, (char)c); return; }
    if (c >= 0xC0 && c < 0xE0) { t->utf_cp = c & 0x1F; t->utf_need = 1; return; }
    if (c >= 0xE0 && c < 0xF0) { t->utf_cp = c & 0x0F; t->utf_need = 2; return; }
    if (c >= 0xF0 && c < 0xF8) { t->utf_cp = c & 0x07; t->utf_need = 3; return; }
    term_emit_ch(t, '?');
}

/* Resolve the REAL `cssc` launcher by scanning PATH, skipping the current
 * directory — otherwise a `cssc.bat` sitting in the cwd (e.g. the self-contained
 * installer in C:\CSSC) shadows the installed CLI and every console command runs
 * the installer instead. Found once, cached. Returns "" if not on PATH (caller
 * then falls back to a bare `cssc`). */
static const char* term_cssc_launcher(void) {
    static char cached[700];
    static int  resolved = 0;
    if (resolved) return cached;
    resolved = 1;
    cached[0] = 0;
    /* The command that launched the IDE tells us WHICH CSSC toolchain to run in
     * the console — `csscd` (dev/source: the newest features) when the IDE was
     * built/started via csscd, or `cssc` (installed) otherwise. It passes the
     * resolved launcher path (or bare name) via CSSC_CONSOLE_LAUNCHER so console
     * commands use the SAME toolchain that built the IDE. */
    {
        const char* forced = getenv("CSSC_CONSOLE_LAUNCHER");
        if (forced && forced[0]) {
            snprintf(cached, sizeof(cached), "%s", forced);
            return cached;
        }
    }
#ifdef _WIN32
    const char* path = getenv("PATH");
    if (!path) return cached;
    static const char* const exts[] = { "cssc.exe", "cssc.cmd", "cssc.bat", 0 };
    char dir[700];
    const char* s = path;
    while (*s) {
        int n = 0;
        while (*s && *s != ';' && n < (int)sizeof(dir) - 1) dir[n++] = *s++;
        dir[n] = 0;
        if (*s == ';') s++;
        if (n == 0) continue;
        /* skip the cwd (and "." ) so a local cssc.bat can never win */
        char cwd[700]; GetCurrentDirectoryA(sizeof(cwd), cwd);
        if (!strcmp(dir, ".") || !_stricmp(dir, cwd)) continue;
        for (int e = 0; exts[e]; e++) {
            char cand[900];
            char sep = (n > 0 && (dir[n - 1] == '\\' || dir[n - 1] == '/')) ? 0 : '\\';
            if (sep) snprintf(cand, sizeof(cand), "%s%c%s", dir, sep, exts[e]);
            else     snprintf(cand, sizeof(cand), "%s%s", dir, exts[e]);
            DWORD at = GetFileAttributesA(cand);
            if (at != INVALID_FILE_ATTRIBUTES && !(at & FILE_ATTRIBUTE_DIRECTORY)) {
                snprintf(cached, sizeof(cached), "%s", cand);
                return cached;
            }
        }
    }
#endif
    return cached;
}

static void term_spawn(cssc_gui_terminal* t, const char* cmd) {
    /* A fresh command clears any stale debugger IP so the editor stops
     * highlighting a line from a previous run. */
    t->ip_line = 0; t->ip_file[0] = 0; t->ipm_st = 0; t->ipm_len = 0;
    t->input_wanted = 0;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa); sa.bInheritHandle = TRUE; sa.lpSecurityDescriptor = NULL;
    HANDLE rd = NULL, wr = NULL;
    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        term_add_line(t, "terminal: pipe creation failed", TERM_COL_ERR);
        return;
    }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    /* stdin pipe so programs that read input (cssc::input) work interactively:
     * child reads in_rd; the parent writes the terminal's input line to in_wr. */
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
    /* Child stdout is a pipe, so C/Python stdio switches to BLOCK buffering and
     * nothing surfaces until the child exits — which reads as "the console waits
     * for the process to finish". Force the CSSC toolchain (Python-based) to
     * flush every write so `term_poll` streams output live. The child inherits
     * this env (CreateProcess env = NULL). */
    SetEnvironmentVariableA("PYTHONUNBUFFERED", "1");
    SetEnvironmentVariableA("PYTHONIOENCODING", "utf-8");
    /* The CLI auto-disables colour on a pipe; force it on so the console renders
     * the toolchain palette (magenta brand, orange `assembly`, …) — this term
     * parses the ANSI SGR itself (term_apply_sgr). */
    SetEnvironmentVariableA("CSSC_FORCE_COLOR", "1");
    PROCESS_INFORMATION pi; ZeroMemory(&pi, sizeof(pi));
    char cmdline[4200];                       /* caller supplies the full line */
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
    int drained = 0;                 /* bytes consumed this frame (budget cap) */
    while (drained < TERM_POLL_BUDGET &&
           PeekNamedPipe((HANDLE)t->pipe_read, NULL, 0, NULL, &avail, NULL) && avail > 0) {
        DWORD toread = avail < sizeof(buf) ? avail : (DWORD)sizeof(buf);
        DWORD got = 0;
        if (!ReadFile((HANDLE)t->pipe_read, buf, toread, &got, NULL) || got == 0) break;
        for (DWORD i = 0; i < got; i++) term_feed_byte(t, (unsigned char)buf[i]);
        drained += (int)got;
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

/* Ctrl+X in the focused terminal — hard-interrupt the running child. */
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
    /* A program is running: the input line feeds ITS stdin (interactive
     * cssc::input), echoed to the scrollback — not treated as a new command. */
    if (t->running && t->stdin_write) {
        /* Echo the typed answer onto the live prompt (partial) line so an
         * interactive cssc::input reads "prompt: answer" like a real terminal,
         * then commit that combined line to the scrollback. */
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
    /* first token */
    const char* p = t->input;
    while (*p == ' ') p++;
    char tok[64]; int tn = 0;
    while (*p && *p != ' ' && tn < 63) tok[tn++] = *p++;
    tok[tn] = 0;
    while (*p == ' ') p++;                 /* p now at the argument tail */
    if (tok[0] == 0) { t->input_len = 0; t->input_cur = 0; return; }
    /* Command body passed to the CSSC launcher (defaults to the whole line).
     * An explicit `cssc `/`csscd ` prefix is stripped so `csscd build "path
     * with spaces"` routes through the quote-preserving cmd.exe branch below
     * instead of the PowerShell passthrough (which mangles quoted args). */
    const char* cmd_body = t->input;
    if (!strcmp(tok, "cssc") || !strcmp(tok, "csscd")) {
        cmd_body = p;                      /* the real command (subcmd + args) */
        int tn2 = 0;
        while (*p && *p != ' ' && tn2 < 63) tok[tn2++] = *p++;
        tok[tn2] = 0;
        while (*p == ' ') p++;
    }
    if (!strcmp(tok, "clear") || !strcmp(tok, "cls")) {
        term_clear_lines(t);
    } else if (!strcmp(tok, "cpall") || !strcmp(tok, "cpallc")) {
        /* cpall  — copy the whole console scrollback to the clipboard.
         * cpallc — copy all, then clear the console. Both echo "copied". */
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
            /* bare cd prints the cwd */
            term_add_line(t, t->cwd, t->fg);
        } else {
            term_add_line(t, "cd: no such directory", TERM_COL_ERR);
        }
        SetCurrentDirectoryA(save);
#endif
    } else if (term_is_cssc_cmd(tok)) {
        char cmd[2200];
        const char* launcher = term_cssc_launcher();
        /* Invoke the resolved launcher by ABSOLUTE, quoted path so a cwd
         * `cssc.bat` can't shadow it and paths with spaces survive intact.
         * The extra outer quotes keep cmd.exe from eating the launcher quotes
         * (cmd /c "..." rule). Falls back to bare `cssc` if none was found. */
        if (launcher && launcher[0])
            snprintf(cmd, sizeof(cmd), "cmd.exe /d /c \"\"%s\" %s\"", launcher, cmd_body);
        else
            snprintf(cmd, sizeof(cmd), "cmd.exe /d /c cssc %s", cmd_body);
        term_spawn(t, cmd);
    } else {
        /* Shell passthrough via PowerShell so ls/cat/pwd/git/... all work. */
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
/* Programmatic: dispatch a command through the same CSSC-priority path as if
 * the user typed it (Run/Compile use this — output lands in-IDE). */
CSSC_GUI_EXPORT void cssc_gui_terminal_run(void* p, const char* cmd) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (!t || !cmd) return;
    size_t n = strlen(cmd); if (n >= sizeof(t->input)) n = sizeof(t->input) - 1;
    memcpy(t->input, cmd, n); t->input[n] = 0; t->input_len = (int)n;
    term_submit(t);
}
/* Programmatic: append a raw line of text to the scrollback. */
CSSC_GUI_EXPORT void cssc_gui_terminal_write(void* p, const char* text) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (t) term_add_line(t, text ? text : "", t->fg);
}
/* Like write, but feeds the text through the SAME ANSI/UTF-8 filter as live
 * subprocess output — so embedded colour escapes (e.g. a captured `cssc`
 * command's themed output) render instead of showing raw \x1b[..m. Handles its
 * own newlines; a trailing partial line is finalized. */
CSSC_GUI_EXPORT void cssc_gui_terminal_writeansi(void* p, const char* text) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    if (!t || !text) return;
    for (const unsigned char* s = (const unsigned char*)text; *s; s++)
        term_feed_byte(t, *s);
    if (t->partial_len > 0) term_finalize_line(t);
}
/* F5 console debugger: the source line the traced program is currently on (0 =
 * none / cleared), and the file it lives in. Parsed from the invisible IP
 * markers `pctrace --console` streams; the IDE highlights this line and, when
 * the file differs, switches the editor to it (cross-#load follow). */
CSSC_GUI_EXPORT int64_t cssc_gui_terminal_ipline(void* p) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    return t ? (int64_t)t->ip_line : 0;
}
/* MUST return a real CSSC string object (length-prefixed), NOT a raw char* —
 * the caller does string == on it, which reads the object's length header; a
 * bare char* is read as garbage length -> OOB compare -> crash. */
CSSC_GUI_EXPORT void* cssc_gui_terminal_ipfile(void* p) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p;
    const char* s = (t && t->ip_file[0]) ? t->ip_file : "";
    return cssc_string_lit(s, strlen(s));
}
CSSC_GUI_EXPORT void cssc_gui_terminal_clear(void* p) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) term_clear_lines(t);
}
/* Lock/unlock typed input (the IDE sets it while an F5 debug trace runs). */
CSSC_GUI_EXPORT void cssc_gui_terminal_setinputlock(void* p, int64_t on) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) t->input_locked = on ? 1 : 0;
}
/* Kill the running child (same hard-terminate as Ctrl+X), leaving the widget
 * reusable — a free slot for the switcher's "Kill" menu. No-op when idle. */
CSSC_GUI_EXPORT void cssc_gui_terminal_stop(void* p) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)p; if (t) term_interrupt(t);
}

CSSC_GUI_EXPORT int64_t cssc_gui_terminal_update(void* tp, void* sp) {
    cssc_gui_terminal* t = (cssc_gui_terminal*)tp;
    if (!t) return 0;
    cssc_gui_screen* s = sp ? (cssc_gui_screen*)sp : t->screen;
    if (!s) return 0;
    term_poll(t);                 /* always drain output, even when hidden */
    if (!t->visible) return 0;
    /* Mouse — drag to select scrollback text (works regardless of focus). */
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
        /* Alt+C — clear the console (edge-detected; Alt combos don't emit a
         * WM_CHAR so poll them live here). */
        int altc = ((GetKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState('C') & 0x8000)) ? 1 : 0;
        if (altc && !t->alt_c_prev) term_clear_lines(t);
        t->alt_c_prev = altc;
#endif
        int64_t c;
        while ((c = cssc_video_poll_char(s->vid)) != 0) {
            if (t->input_locked && !t->input_wanted) {
                /* A debug trace owns the panel: only Ctrl+X (stop) and Ctrl+C
                 * (copy) pass; every other keystroke is swallowed so nothing is
                 * typed into the console while + / - / # drive the trace. The
                 * exception is input_wanted — the program is blocked in
                 * cssc::input, so typing is allowed until the answer is sent. */
                if (c == 24) term_interrupt(t);
                else if (c == 3) {
                    char* sel = term_selected_str(t);
                    if (sel) { gui_clipboard_set(sel); free(sel); }
                }
                continue;
            }
            if (c == 13 || c == 10) {
                term_submit(t);
            } else if (c == 3) {                    /* Ctrl+C — copy selection */
                char* sel = term_selected_str(t);
                if (sel) { gui_clipboard_set(sel); free(sel); }
            } else if (c == 24) {                   /* Ctrl+X — interrupt running command */
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
            if (k == 0x25 && t->input_cur > 0) t->input_cur--;               /* LEFT */
            else if (k == 0x27 && t->input_cur < t->input_len) t->input_cur++; /* RIGHT */
            else if (k == 0x24) t->input_cur = 0;                            /* HOME */
            else if (k == 0x23) t->input_cur = t->input_len;                 /* END */
            else if (k == 0x26) {                                           /* UP — history */
                if (t->nhist > 0 && t->hist_pos > 0) {
                    t->hist_pos--;
                    const char* h = t->hist[t->hist_pos];
                    size_t n = strlen(h); if (n >= sizeof(t->input)) n = sizeof(t->input) - 1;
                    memcpy(t->input, h, n); t->input[n] = 0;
                    t->input_len = (int)n; t->input_cur = (int)n;
                }
            } else if (k == 0x28) {                                         /* DOWN — history */
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
    }
    /* Wheel scrolls the scrollback whenever the mouse is OVER the terminal —
     * independent of focus, so the console stays scrollable while a debug trace
     * runs and the editor keeps its own wheel when hovered instead (whichever
     * widget the mouse is over consumes the delta). */
    if (s->mx >= t->x && s->mx < t->x + t->w &&
        s->my >= t->y && s->my < t->y + t->h) {
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
    /* input line at the bottom */
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

/* ---- Menu bar (top menus + dropdowns) ----------------------------------- */
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

/* ======================= F5 DEBUGGER (memtrace overlay) ====================
 * A modal overlay that spawns `csscd pctrace <file>`, parses its JSON event
 * stream (start/step/alloc/write/free/output/crash/end/read_result) and shows a
 * live allocation table + an ordered event log + the instruction pointer. The
 * IP line is mirrored to the editor as a red bar (cssc_gui_editor_setipline). A
 * hex address typed into the probe box is written to pctrace's stdin
 * (`read <addr>`) and its raw reply shown. Same process/pipe plumbing as the
 * terminal widget; the whole panel captures input (screen->cssc_modal) so the
 * IDE underneath cannot be clicked while it is open.                          */

#define DBG_COL_BG    ((int64_t)0xF00A0710)
#define DBG_COL_HEAD  ((int64_t)0xFF5FD8E8)   /* column headers — cyan   */
#define DBG_COL_NAME  ((int64_t)0xFFEAEAEA)   /* variable name — white   */
#define DBG_COL_ADDR  ((int64_t)0xFF8A84A0)   /* address / region — muted*/
#define DBG_COL_TYPE  ((int64_t)0xFF6AA8FF)   /* type — blue             */
#define DBG_COL_VAL   ((int64_t)0xFF5FE0A0)   /* value — green           */
#define DBG_COL_DEAD  ((int64_t)0xFF6A6A6A)   /* freed allocation — grey */
#define DBG_COL_FREE  ((int64_t)0xFFE0C860)   /* free note — yellow      */
#define DBG_COL_READ  ((int64_t)0xFFC74DE0)   /* probe reply — magenta   */
#define DBG_COL_ERR   ((int64_t)0xFFFF5FB0)   /* crash — pink            */

typedef struct {
    char     name[64];
    char     region[16];
    char     type[24];
    char     value[192];
    uint64_t addr;
    int      line;
    int      live;            /* 1 = allocated, 0 = freed (kept, greyed) */
    int      stamp;
} dbg_alloc;

typedef struct {
    int              kind;    /* GW_DEBUGGER */
    cssc_gui_screen* screen;
    /* child process (pctrace) */
    void*  proc; void* pipe_read; void* stdin_write; int running;
    void*  job;   /* job object owning the trace tree (cmd.exe + python) so STOP
                   * kills the WHOLE tree, not just cmd.exe (no orphaned pctrace
                   * writing to a closed pipe → the crash on stop-while-running) */
    char   partial[16384]; int partial_len;      /* JSON-line accumulator */
    /* allocation table */
    dbg_alloc* allocs; int nallocs, cap_allocs;
    /* event log (Output tab): program output + frees + probe replies + crash */
    char** log; int64_t* log_col; int nlog, cap_log, log_top;
    char   logpartial[4096]; int logpartial_len;
    /* instruction pointer */
    int  ip_line; char ip_scope[64]; int stepcount;
    /* Source FILE the IP currently sits in. A `#load`ed module's steps carry
     * their own path (the tracer's `file` field), so the overlay follows
     * execution into the module and back — the IDE loads THIS file's source and
     * highlights ip_line within it. Empty = the main program file. */
    char ip_file[1024];
    int  crashed; int crash_line; char crash_msg[256];
    int  ended; int exit_code;
    /* UI */
    int  active;
    int  held;                /* stopped but keeping the last IP highlight until the editor is clicked */
    int  focused;             /* clicked-into: draw big + own input; else small */
    int  tab;                 /* 0 = Allocations, 1 = Output */
    int  alloc_top;
    int  ip_follow;           /* F2: 1 = follow IP, 0 = free cam */
    char probe[80]; int probe_len;
    char last_read[300];
    char title[520];
    int64_t scale;
    /* Focused-panel size (resizeable): 300x300 by default, centered. Dragging
     * the bottom-right grip adjusts these; clamped in dbg_rect. */
    int  win_w, win_h;
    int  resizing;            /* 1 while the user drags the resize grip */
    int  drag_dx, drag_dy;    /* grip-grab offset so resize doesn't jump */
    /* Program stdout, mirrored to the IDE terminal so `run + debug` shows the
     * real console output (the tiny card only carries status). Drained each
     * frame by `cssc_gui_debugger_takeoutput`. */
    char pending_out[16384]; int pending_out_len;
    /* Startup progress: 1 while the tracer process is launching and no event has
     * arrived yet — the card shows an animated spinner so it never reads as hung.
     * Cleared the moment the first pctrace byte/event lands. */
    int  waiting; int wait_anim;
} cssc_gui_debugger;

/* ---- minimal JSON field extraction (flat objects only) ------------------- */
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
                default:  c = *p;   break;    /* \" \\ \/ — literal */
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

/* Hard ceiling on retained log lines. A `pctrace` run emits one `step` event
 * per executed source line plus every `output` line, so a loop-heavy program
 * produces an unbounded stream. Ring-trimming here is what keeps the debugger
 * from growing memory without limit (the OOM crash) on a real program. */
#define DBG_LOG_MAX 4000
/* ---- log + allocation table ---------------------------------------------- */
static void dbg_log(cssc_gui_debugger* d, const char* s, int64_t col) {
    if (d->nlog >= DBG_LOG_MAX) {
        /* Drop the oldest half (freeing those strings) and keep the most recent
         * lines — bounded memory, newest-relevant content preserved. */
        int drop = DBG_LOG_MAX / 2;
        for (int i = 0; i < drop; i++) free(d->log[i]);
        memmove(d->log, d->log + drop,
                (size_t)(d->nlog - drop) * sizeof(char*));
        memmove(d->log_col, d->log_col + drop,
                (size_t)(d->nlog - drop) * sizeof(int64_t));
        d->nlog -= drop;
        d->log_top = (d->log_top > drop) ? (d->log_top - drop) : 0;
    }
    if (d->nlog >= d->cap_log) {
        int nc = d->cap_log ? d->cap_log * 2 : 128;
        if (nc > DBG_LOG_MAX) nc = DBG_LOG_MAX;
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
        /* Mirror EVERY output char into pending_out for the terminal. When the
         * buffer is full, drain-half so we keep flowing rather than truncate for
         * the rest of the run (the terminal already has what was drained). */
        if (d->pending_out_len >= (int)sizeof(d->pending_out) - 1) {
            int half = (int)sizeof(d->pending_out) / 2;
            memmove(d->pending_out, d->pending_out + half,
                    (size_t)(d->pending_out_len - half));
            d->pending_out_len -= half;
        }
        d->pending_out[d->pending_out_len++] = c;
        if (c == '\n') {
            d->logpartial[d->logpartial_len] = 0;
            dbg_log(d, d->logpartial, (int64_t)0xFFCFC8E0);
            d->logpartial_len = 0;
        } else if (d->logpartial_len < (int)sizeof(d->logpartial) - 1) {
            d->logpartial[d->logpartial_len++] = c;
        }
    }
}

/* Drain the mirrored program stdout (returns "" when nothing new). The IDE
 * calls this each frame and writes the result to its terminal, so `run + debug`
 * shows the live console output while the editor tracks the IP line. */
CSSC_GUI_EXPORT const char* cssc_gui_debugger_takeoutput(void* p) {
    static char ret[16384];
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || d->pending_out_len <= 0) { ret[0] = 0; return ret; }
    int n = d->pending_out_len;
    if (n > (int)sizeof(ret) - 1) n = (int)sizeof(ret) - 1;
    memcpy(ret, d->pending_out, (size_t)n);
    ret[n] = 0;
    d->pending_out_len = 0;
    return ret;
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
        /* Bound the table: a loop that allocates uniquely-named slots would
         * otherwise grow it without limit. Existing entries still update; only
         * brand-new names past the ceiling are dropped from the live view. */
        if (d->nallocs >= 16384) return;
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

/* ---- event dispatch ------------------------------------------------------ */
static void dbg_parse_line(cssc_gui_debugger* d, const char* line) {
    char ev[24];
    if (!dbg_json_str(line, "ev", ev, sizeof(ev))) {
        /* Not an event line = raw stdout/stderr from the toolchain itself — most
         * importantly a `cssc: fatal error: … Parser/Runtime Error …` that
         * fires BEFORE any trace event (e.g. a syntax error). SHOW it, else a
         * compile/parse failure reads as an unexplained "[program ended exit 1]"
         * with no clue why. Skip blank lines and our own JSON braces. */
        const char* s = line;
        while (*s == ' ' || *s == '\t') s++;
        if (*s && *s != '{') dbg_log(d, line, DBG_COL_ERR);
        return;
    }
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
        /* Track the source file so the IDE can switch the shown source when
         * execution crosses into (or out of) a `#load`ed module. */
        dbg_json_str(line, "file", d->ip_file, sizeof(d->ip_file));
    } else if (strcmp(ev, "scope") == 0) {
        char sc[64]; dbg_json_str(line, "scope", sc, sizeof(sc));
        char sm[128];
        const char* verb = dbg_json_bool_true(line, "created")
                           ? "created scope" : "entering scope";
        snprintf(sm, sizeof(sm), "%s: %s", verb, sc);
        dbg_log(d, sm, DBG_COL_HEAD);
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
        /* Leak report: every allocation still live at end was never #delete'd.
         * List each as a red line with its source position. */
        int leaks = 0;
        for (int i = 0; i < d->nallocs; i++) if (d->allocs[i].live) leaks++;
        if (leaks > 0) {
            char lh[64];
            snprintf(lh, sizeof(lh), "LEAKS: %d allocation(s) never deleted:", leaks);
            dbg_log(d, lh, DBG_COL_ERR);
            for (int i = 0; i < d->nallocs; i++) {
                if (!d->allocs[i].live) continue;
                char lb[320];
                snprintf(lb, sizeof(lb), "  leak: %s (%s) line %d  0x%llx",
                         d->allocs[i].name, d->allocs[i].type, d->allocs[i].line,
                         (unsigned long long)d->allocs[i].addr);
                dbg_log(d, lb, DBG_COL_ERR);
            }
        } else {
            dbg_log(d, "no leaks - all allocations freed", DBG_COL_VAL);
        }
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
            d->partial_len = 0;    /* runaway line — drop it */
    }
}

/* ---- child process (pctrace) — same plumbing as the terminal widget ------ */
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
        /* Put the child in a kill-on-close job so STOP terminates the whole
         * process tree (cmd.exe AND the python pctrace it launched). Without
         * this, TerminateProcess only killed cmd.exe and left pctrace running,
         * writing into a closed pipe. Best-effort: if the job can't be created
         * the run still works (STOP falls back to terminating cmd.exe). */
        HANDLE job = CreateJobObjectA(NULL, NULL);
        if (job) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
            ZeroMemory(&jeli, sizeof(jeli));
            jeli.BasicLimitInformation.LimitFlags =
                JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                    &jeli, sizeof(jeli));
            if (!AssignProcessToJobObject(job, pi.hProcess)) {
                CloseHandle(job); job = NULL;
            }
        }
        d->job = job;
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
    /* Cap bytes consumed PER FRAME. A full-speed `pctrace` floods step events
     * faster than they can be parsed; without a budget the drain loop would run
     * until the pipe empties, stalling the render loop ("not responding"). The
     * remainder is picked up next frame — pctrace back-pressures on the pipe,
     * so nothing is lost, and the UI keeps painting the advancing IP line. */
    int budget = 512 * 1024;
    while (budget > 0
           && PeekNamedPipe((HANDLE)d->pipe_read, NULL, 0, NULL, &avail, NULL)
           && avail > 0) {
        DWORD toread = avail < sizeof(buf) ? avail : (DWORD)sizeof(buf);
        DWORD got = 0;
        if (!ReadFile((HANDLE)d->pipe_read, buf, toread, &got, NULL) || got == 0) break;
        if (got > 0) d->waiting = 0;   /* tracer is alive — stop the spinner */
        for (DWORD i = 0; i < got; i++) dbg_feed_byte(d, buf[i]);
        budget -= (int)got;
    }
#else
    (void)d;
#endif
}

static void dbg_poll(cssc_gui_debugger* d) {
#ifdef _WIN32
    if (!d->running || !d->pipe_read) return;
    dbg_drain(d);
    /* pctrace stays alive after the program ends to serve probes, so it only
     * exits once we close its stdin (dbg_stop_proc). A signalled process here
     * means it terminated on its own. */
    if (d->proc && WaitForSingleObject((HANDLE)d->proc, 0) == WAIT_OBJECT_0) {
        dbg_drain(d);
        CloseHandle((HANDLE)d->proc);
        if (d->pipe_read) CloseHandle((HANDLE)d->pipe_read);
        if (d->stdin_write) CloseHandle((HANDLE)d->stdin_write);
        if (d->job) CloseHandle((HANDLE)d->job);
        d->proc = NULL; d->pipe_read = NULL; d->stdin_write = NULL;
        d->job = NULL; d->running = 0;
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
    /* Close the READ end first so a child blocked writing into a full stdout
     * pipe (mid-flood) unblocks at once with a broken-pipe error — otherwise it
     * never reads 'quit' and we'd stall the UI thread on the wait. */
    if (d->pipe_read) { CloseHandle((HANDLE)d->pipe_read); d->pipe_read = NULL; }
    if (d->stdin_write) {
        DWORD wrote = 0;
        WriteFile((HANDLE)d->stdin_write, "quit\n", 5, &wrote, NULL);
        CloseHandle((HANDLE)d->stdin_write); d->stdin_write = NULL;
    }
    /* Kill the WHOLE tree (cmd.exe + python pctrace) — closing the kill-on-close
     * job does this atomically, so nothing is orphaned onto the dead pipe. */
    if (d->job) { CloseHandle((HANDLE)d->job); d->job = NULL; }
    if (d->proc) {
        WaitForSingleObject((HANDLE)d->proc, 50);   /* brief, non-blocking-ish */
        TerminateProcess((HANDLE)d->proc, 0);        /* belt-and-braces */
        CloseHandle((HANDLE)d->proc); d->proc = NULL;
    }
    d->running = 0;
#else
    (void)d;
#endif
}

/* STOP a run keeping the last IP highlight — defined after dbg_rect; forward-
 * declared here so the click/Esc handlers above its body can call it. */
static void dbg_stop_held(cssc_gui_debugger* d);

static void dbg_reset(cssc_gui_debugger* d) {
    for (int i = 0; i < d->nlog; i++) free(d->log[i]);
    d->nlog = 0; d->log_top = 0; d->logpartial_len = 0;
    d->nallocs = 0;
    d->partial_len = 0;
    d->ip_line = 0; d->ip_scope[0] = 0; d->stepcount = 0; d->ip_file[0] = 0;
    d->crashed = 0; d->crash_line = 0; d->crash_msg[0] = 0;
    d->ended = 0; d->exit_code = 0;
    d->tab = 0; d->alloc_top = 0; d->probe_len = 0; d->probe[0] = 0;
    d->last_read[0] = 0;
    d->waiting = 0; d->wait_anim = 0;
}

/* Minimized card size — a compact top-right status card: "CSSC Diagnose" +
 * run state + the last few program-output lines, so `run + debug` shows console
 * output without covering the editor (the red IP-line stays visible on the
 * left). Click it to expand to the full 300x300 panel. */
#define DBG_MIN_W 360
#define DBG_MIN_H 168

/* Panel geometry. Minimized = the tiny top-right card. Focused = a resizeable
 * window (default 300x300) CENTERED on screen. `win_w/win_h` carry the user's
 * dragged size; clamped to the screen here. */
static void dbg_rect(cssc_gui_debugger* d, int64_t W, int64_t H,
                     int64_t* x, int64_t* y, int64_t* w, int64_t* h) {
    if (!d->focused) {
        int64_t rw = DBG_MIN_W, rh = DBG_MIN_H;
        if (rw > W - 24) rw = W - 24;
        if (rh > H - 24) rh = H - 24;
        *w = rw; *h = rh;
        *x = W - rw - 16;   /* top-right corner */
        *y = 16;
        return;
    }
    int64_t rw = d->win_w > 0 ? d->win_w : 300;
    int64_t rh = d->win_h > 0 ? d->win_h : 300;
    if (rw < 220) rw = 220;             /* keep the header + tabs legible */
    if (rh < 160) rh = 160;
    if (rw > W - 24) rw = W - 24;
    if (rh > H - 24) rh = H - 24;
    *w = rw; *h = rh;
    *x = (W - rw) / 2;                  /* centered */
    *y = (H - rh) / 2;
}

/* ---- exports ------------------------------------------------------------- */
CSSC_GUI_EXPORT void* cssc_gui_debugger_new(void* screen) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)calloc(1, sizeof(cssc_gui_debugger));
    if (!d) return NULL;
    d->kind = GW_DEBUGGER;
    d->screen = (cssc_gui_screen*)screen;
    d->scale = 2;
    d->ip_follow = 1;
    d->win_w = 300;            /* focused panel default — resizeable */
    d->win_h = 300;
    return d;
}

/* Which CLI launcher is installed — `cssc` (shipped) or `csscd` (dev). Resolved
 * once via SearchPath (no console flash, unlike `system("cssc --version")`), so
 * the debugger's `pctrace` actually runs on a dev machine that only has csscd. */
static const char* dbg_launcher(void) {
    /* Resolve the real launcher by absolute path, skipping the cwd — a bare
     * `cssc` would run a cwd `cssc.bat` (e.g. the installer in C:\CSSC) instead
     * of pctrace, which reads as "the debugger builds then nothing happens". */
    const char* real = term_cssc_launcher();
    if (real && real[0]) return real;
    /* Fall back to a dev csscd.exe, then bare cssc. */
    static int resolved = 0;
    static char launcher[8] = "cssc";
    if (!resolved) {
        resolved = 1;
        char buf[MAX_PATH]; char* fp = NULL;
        if (SearchPathA(NULL, "csscd", ".exe", (DWORD)sizeof(buf), buf, &fp) != 0)
            snprintf(launcher, sizeof(launcher), "csscd");
    }
    return launcher;
}

CSSC_GUI_EXPORT void cssc_gui_debugger_start(void* p, const char* cmd, const char* title) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p; if (!d) return;
    if (d->running) dbg_stop_proc(d);
    dbg_reset(d);
    d->held = 0;
    d->active = 1;
    /* Open MINIMIZED (a small top-right status card), NOT focused. F5's job is
     * to run the file and light up the current IP line in the editor — a big
     * modal overlay that hid the code was the whole complaint. Click the card
     * to expand it (probe memory / read output); Esc closes. */
    d->focused = 0;
    snprintf(d->title, sizeof(d->title), "%s", title ? title : "");
    {
        /* Show the file being traced (basename) so F5 reads like a build step. */
        const char* base = title ? title : "";
        for (const char* q = base; *q; q++)
            if (*q == '\\' || *q == '/') base = q + 1;
        char msg[600];
        snprintf(msg, sizeof(msg), "building Debugger for %s ...", base);
        dbg_log(d, msg, DBG_COL_VAL);
    }
    /* `cmd` is a CSSC sub-command (e.g. `pctrace "file"`); resolve it through the
     * installed launcher (absolute, quoted — cwd-shadow + space safe). */
    char full[4300];
    const char* L = dbg_launcher();
    snprintf(full, sizeof(full), "cmd.exe /d /c \"\"%s\" %s\"", L, cmd ? cmd : "");
    dbg_spawn(d, full);
    /* The tracer is a fresh Python process: startup + parse can take a second or
     * two before the first event streams. Say so + start the spinner so the card
     * never reads as hung. `dbg_waiting` is cleared when the first event lands. */
    dbg_log(d, "starting tracer (pctrace) — waiting for first step ...", DBG_COL_VAL);
    d->waiting = 1;
    d->wait_anim = 0;
}

CSSC_GUI_EXPORT int64_t cssc_gui_debugger_update(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active) return 0;
    dbg_poll(d);
    /* Own input only while focused (big). Minimized (small) it floats in the
     * corner and the IDE stays fully interactive underneath. */
    if (d->screen) d->screen->cssc_modal = d->focused ? 1 : 0;
    /* Resize-drag: while focused, holding the bottom-right grip and dragging
     * resizes the (centered) panel — growing symmetrically from the centre so
     * it stays centred. Reads the screen's live mouse state directly (no extra
     * per-frame binding needed). */
    if (d->focused && d->screen) {
        int64_t W = d->screen->w, H = d->screen->h;
        int64_t x, y, w, h;
        dbg_rect(d, W, H, &x, &y, &w, &h);
        int mx = (int)d->screen->mx, my = (int)d->screen->my;
        int down = d->screen->down ? 1 : 0;
        int in_grip = (mx >= x + w - 22 && mx <= x + w + 4
                       && my >= y + h - 22 && my <= y + h + 4);
        if (!d->resizing && down && in_grip) d->resizing = 1;
        if (d->resizing) {
            if (down) {
                int64_t nw = 2 * (mx - W / 2);
                int64_t nh = 2 * (my - H / 2);
                if (nw < 220) nw = 220;
                if (nh < 160) nh = 160;
                if (nw > W - 24) nw = W - 24;
                if (nh > H - 24) nh = H - 24;
                d->win_w = (int)nw;
                d->win_h = (int)nh;
            } else {
                d->resizing = 0;
            }
        }
    } else if (d->resizing) {
        d->resizing = 0;
    }
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

/* Click routing from the workspace: focus (grow) when the click lands inside
 * the panel, minimize when it lands anywhere else. */
CSSC_GUI_EXPORT void cssc_gui_debugger_click(void* p, int64_t mx, int64_t my, int64_t clicked) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active || !clicked || !d->screen) return;
    int64_t x, y, w, h;
    dbg_rect(d, d->screen->w, d->screen->h, &x, &y, &w, &h);
    /* STOP button hit-test (minimized card, top-right) — works while minimized,
     * so the user can always end a run without focusing the overlay. */
    if (!d->focused) {
        int64_t stx = x + w - 52, sty = y + 6;
        if (mx >= stx && mx < stx + 44 && my >= sty && my < sty + 18) {
            dbg_stop_held(d);
            return;
        }
    }
    d->focused = (mx >= x && mx < x + w && my >= y && my < y + h) ? 1 : 0;
}

/* Current instruction-pointer source line — the editor highlights it red. The
 * highlight is shown whenever a trace is active OR stopped-but-held, EVEN in
 * free-cam (F2 off): toggling follow off must not blank the marker, only stop
 * the view from auto-tracking it. 0 = no highlight (debugger fully inactive). */
CSSC_GUI_EXPORT int64_t cssc_gui_debugger_ipline(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d) return 0;
    if (!d->active && !d->held) return 0;
    return (int64_t)d->ip_line;
}
/* 1 while the view should LOCK onto the IP (actively following): active, follow
 * on, and not stopped-held. The IDE calls revealIp() each frame only while this
 * is 1 — so F2-off (free-cam) and post-stop leave the user free to scroll while
 * the red marker stays put. */
CSSC_GUI_EXPORT int64_t cssc_gui_debugger_ipfollow(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d) return 0;
    return (d->active && d->ip_follow && !d->held) ? 1 : 0;
}
/* Absolute path of the source file the IP currently sits in — the main program
 * normally, or a `#load`ed module's own path while execution is inside it. The
 * IDE compares this against the file shown in the editor and, when it differs,
 * loads THIS file's source before highlighting ip_line. Only auto-switches
 * while ACTIVELY following (not free-cam, not after a stop) so the user's own
 * file browsing is never yanked away. */
CSSC_GUI_EXPORT const char* cssc_gui_debugger_ipfile(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d) return "";
    if (!d->active || !d->ip_follow || d->held) return "";
    return d->ip_file;
}
/* STOP: halt the trace but KEEP the last IP line highlighted in the editor and
 * close the overlay. The highlight persists (via `held`) until the user clicks
 * the editor, which calls cssc_gui_debugger_clearheld. */
static void dbg_stop_held(cssc_gui_debugger* d) {
    if (!d) return;
    d->held = (d->ip_line > 0) ? 1 : 0;
    dbg_stop_proc(d);
    d->active = 0;
    if (d->screen) d->screen->cssc_modal = 0;
}
CSSC_GUI_EXPORT int64_t cssc_gui_debugger_held(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    return (d && d->held) ? 1 : 0;
}
CSSC_GUI_EXPORT void cssc_gui_debugger_clearheld(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (d) { d->held = 0; d->ip_line = 0; }
}

CSSC_GUI_EXPORT void cssc_gui_debugger_close(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p; if (!d) return;
    dbg_stop_proc(d);
    d->active = 0;
    if (d->screen) d->screen->cssc_modal = 0;
}

/* Central key dispatch — shared by handleKey (virtual keys / arrows) and
 * handleChar (control codes that arrive as characters: Esc/Enter/Tab/Bksp). */
static void dbg_handle_vk(cssc_gui_debugger* d, int vk) {
    if (vk == 27) {                                                   /* Esc -> stop, keep the IP highlight */
        dbg_stop_held(d);
        return;
    }
    if (vk == 113) { d->ip_follow = d->ip_follow ? 0 : 1; return; }   /* F2 follow/free */
    if (vk == 9)  { d->tab = d->tab ? 0 : 1; return; }                /* Tab switch */
    if (vk == 8)  { if (d->probe_len > 0) d->probe[--d->probe_len] = 0; return; }
    if (vk == 13) {                                                   /* Enter -> probe */
        if (d->probe_len > 0) { d->probe[d->probe_len] = 0; dbg_send_read(d, d->probe); }
        return;
    }
    if (vk == 38) {                                                   /* Up */
        if (d->tab == 0) { if (d->alloc_top > 0) d->alloc_top--; }
        else { if (d->log_top > 0) d->log_top--; }
        return;
    }
    if (vk == 40) {                                                   /* Down */
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
    /* the probe box accepts a hex address only */
    int okc = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F') || c == 'x' || c == 'X';
    if (!okc) return;
    if (d->probe_len < (int)sizeof(d->probe) - 1) {
        d->probe[d->probe_len++] = (char)c;
        d->probe[d->probe_len] = 0;
    }
}

/* Draw `s` at (x,y) but never past `right`: if it wouldn't fit, truncate and
 * append "..." so nothing bleeds out of the (small, resizeable) panel. Resize
 * the panel wider and more of each line shows — exactly the requested UX. */
static void dbg_text_clip(void* vid, int64_t x, int64_t y, const char* s,
                          int64_t col, int64_t sc, int64_t right) {
    if (!s || x >= right) return;
    int64_t cw = 8 * sc;
    int64_t maxch = (right - x) / cw;
    if (maxch <= 0) return;
    int n = (int)strlen(s);
    if ((int64_t)n <= maxch) { cssc_video_draw_text(vid, x, y, s, col, sc); return; }
    char buf[600];
    if (maxch > (int64_t)sizeof(buf) - 1) maxch = (int64_t)sizeof(buf) - 1;
    if (maxch >= 4) {
        int keep = (int)maxch - 3;
        memcpy(buf, s, (size_t)keep);
        buf[keep] = '.'; buf[keep + 1] = '.'; buf[keep + 2] = '.'; buf[keep + 3] = 0;
    } else {
        memcpy(buf, s, (size_t)maxch);
        buf[maxch] = 0;
    }
    cssc_video_draw_text(vid, x, y, buf, col, sc);
}

CSSC_GUI_EXPORT void cssc_gui_debugger_draw(void* p) {
    cssc_gui_debugger* d = (cssc_gui_debugger*)p;
    if (!d || !d->active || !d->screen) return;
    void* vid = d->screen->vid;
    int64_t W = d->screen->w, H = d->screen->h;
    int64_t px, py, pw, ph;
    dbg_rect(d, W, H, &px, &py, &pw, &ph);

    if (!d->focused) {
        /* Minimized: a compact status card — title + run state + IP line + the
         * last few program-output lines. Non-modal, top-right, so the editor and
         * its red IP-line stay visible. Click it to expand to the full panel. */
        int64_t right = px + pw - 10;
        cssc_video_fillrect(vid, px, py, pw, ph, DBG_COL_BG);
        cssc_video_fillrect(vid, px, py, pw, 2, (int64_t)0x60FFFFFF);
        cssc_video_draw_rect(vid, px, py, pw, ph, (int64_t)0x40FFFFFF);
        int64_t dot = d->crashed ? DBG_COL_ERR
                    : (d->ended ? DBG_COL_ADDR
                    : (d->running ? DBG_COL_VAL : DBG_COL_ADDR));
        int64_t cy2 = py + 8;
        cssc_video_fillrect(vid, px + 10, cy2 + 3, 8, 8, dot);
        cssc_video_draw_text(vid, px + 26, cy2, "CSSC Diagnose",
                             (int64_t)0xFFE49BFF, 2);
        /* STOP button (top-right of the card) — halts the trace, keeps the last
         * IP line highlighted, closes the card. */
        int64_t stx = px + pw - 52, sty = py + 6;
        cssc_video_fillrect(vid, stx, sty, 44, 18, (int64_t)0x50FF3B54);
        cssc_video_draw_rect(vid, stx, sty, 44, 18, (int64_t)0xC0FF3B54);
        cssc_video_draw_text(vid, stx + 8, sty + 3, "STOP", (int64_t)0xFFFF9BB0, 1);
        cy2 += 22;
        char m0[160];
        if (d->waiting) {
            /* Spinner while pctrace boots (fresh Python: parse + first event can
             * take a second) so the card never reads as hung. */
            static const char spin[4] = { '|', '/', '-', '\\' };
            d->wait_anim++;
            snprintf(m0, sizeof(m0), "%c  starting tracer ...   click to expand",
                     spin[(d->wait_anim >> 2) & 3]);
            dbg_text_clip(vid, px + 10, cy2, m0, DBG_COL_VAL, 1, right);
        } else {
            const char* sst = d->crashed ? "CRASHED" : (d->ended ? "ended"
                              : (d->running ? "running" : "idle"));
            snprintf(m0, sizeof(m0), "[%s]  IP line %d   click to expand",
                     sst, d->ip_line);
            dbg_text_clip(vid, px + 10, cy2, m0,
                          d->crashed ? DBG_COL_ERR : DBG_COL_ADDR, 1, right);
        }
        cy2 += 14;
        cssc_video_fillrect(vid, px + 8, cy2, pw - 16, 1, (int64_t)0x30FFFFFF);
        cy2 += 6;
        /* Last program-output lines (the Output tab's tail) so stdout is
         * visible at a glance without expanding. */
        int64_t rowh = 8 + 5;
        int64_t maxrows = ((py + ph) - cy2 - 6) / rowh;
        if (maxrows < 0) maxrows = 0;
        int64_t startln = (int64_t)d->nlog - maxrows;
        if (startln < 0) startln = 0;
        for (int64_t li = startln; li < d->nlog; li++) {
            dbg_text_clip(vid, px + 10, cy2, d->log[li], d->log_col[li], 1, right);
            cy2 += rowh;
        }
        return;
    }

    /* Compact text (scale 1) so the small default panel is useful; resize wider
     * to reveal more (dbg_text_clip handles the ellipsis). */
    int64_t sc = 1;
    int64_t ch = 8 * sc, cw = 8 * sc;
    int64_t right = px + pw - 10;        /* content right edge for clipping */

    cssc_video_fillrect(vid, 0, 0, W, H, (int64_t)0x28000000);          /* light modal tint — keep code + IP line readable */
    cssc_video_fillrect(vid, px, py, pw, ph, DBG_COL_BG);
    cssc_video_fillrect(vid, px, py, pw, 2, (int64_t)0x60FFFFFF);
    cssc_video_draw_rect(vid, px, py, pw, ph, (int64_t)0x40FFFFFF);

    int64_t cy = py + 12;
    cssc_video_draw_text(vid, px + 12, cy, "CSSC  DEBUGGER", (int64_t)0xFFE49BFF, sc + 1);
    const char* st = d->crashed ? "CRASHED" : (d->ended ? "ended"
                     : (d->running ? "running" : "idle"));
    dbg_text_clip(vid, px + 12 + 15 * (8 * (sc + 1)), cy, st,
                  d->crashed ? DBG_COL_ERR : DBG_COL_ADDR, sc, right);
    cy += 8 * (sc + 1) + 10;

    char ipbuf[160];
    snprintf(ipbuf, sizeof(ipbuf), "IP line %d   %s   scope %s",
             d->ip_line, d->ip_follow ? "[FOLLOW F2]" : "[FREE F2]",
             d->ip_scope[0] ? d->ip_scope : "-");
    dbg_text_clip(vid, px + 12, cy, ipbuf, DBG_COL_HEAD, sc, right);
    cy += ch + 8;

    if (d->crashed) {
        char cb[340];
        snprintf(cb, sizeof(cb), "CRASH  line %d:  %s", d->crash_line, d->crash_msg);
        cssc_video_fillrect(vid, px + 8, cy - 2, pw - 16, ch + 6, (int64_t)0x30FF5FB0);
        dbg_text_clip(vid, px + 12, cy, cb, DBG_COL_ERR, sc, right);
        cy += ch + 10;
    }

    char t0[48];
    snprintf(t0, sizeof(t0), "[ Allocs %d ]", d->nallocs);
    const char* t1 = "[ Output ]";
    dbg_text_clip(vid, px + 12, cy, t0, d->tab == 0 ? DBG_COL_NAME : DBG_COL_ADDR, sc, right);
    dbg_text_clip(vid, px + 12 + (int64_t)strlen(t0) * cw + 16, cy, t1,
                  d->tab == 1 ? DBG_COL_NAME : DBG_COL_ADDR, sc, right);
    cy += ch + 6;
    cssc_video_fillrect(vid, px + 8, cy, pw - 16, 1, (int64_t)0x30FFFFFF);
    cy += 8;

    int64_t content_y = cy;
    int64_t footer_h = ch * 3 + 28;
    int64_t content_h = (py + ph) - content_y - footer_h;
    int64_t rowh = ch + 5;
    int64_t rows = content_h > 0 ? content_h / rowh : 1;
    if (rows < 1) rows = 1;

    if (d->tab == 0) {
        int64_t maxtop = (int64_t)d->nallocs - (rows - 1);
        if (maxtop < 0) maxtop = 0;
        if (d->alloc_top > maxtop) d->alloc_top = (int)maxtop;
        if (d->alloc_top < 0) d->alloc_top = 0;
        /* Compact columns; each field clips at the next column (no bleed) and
         * the whole row clips at `right`. Resize wider to see full values. */
        int64_t cxName = px + 12;
        int64_t cxReg  = cxName + 13 * cw;
        int64_t cxAddr = cxReg  + 7  * cw;
        int64_t cxType = cxAddr + 13 * cw;
        int64_t cxVal  = cxType + 8  * cw;
        dbg_text_clip(vid, cxName, content_y, "NAME",    DBG_COL_HEAD, sc, cxReg  - cw);
        dbg_text_clip(vid, cxReg,  content_y, "REGION",  DBG_COL_HEAD, sc, cxAddr - cw);
        dbg_text_clip(vid, cxAddr, content_y, "ADDRESS", DBG_COL_HEAD, sc, cxType - cw);
        dbg_text_clip(vid, cxType, content_y, "TYPE",    DBG_COL_HEAD, sc, cxVal  - cw);
        dbg_text_clip(vid, cxVal,  content_y, "VALUE",   DBG_COL_HEAD, sc, right);
        int64_t ry = content_y + rowh;
        for (int64_t r = 1; r < rows; r++) {
            int idx = d->alloc_top + (int)(r - 1);
            if (idx >= d->nallocs) break;
            dbg_alloc* a = &d->allocs[idx];
            /* Still-live at program end == a leak -> name in red. */
            int64_t nc = a->live ? (d->ended ? DBG_COL_ERR : DBG_COL_NAME) : DBG_COL_DEAD;
            int64_t mc = a->live ? DBG_COL_ADDR : DBG_COL_DEAD;
            int64_t tc = a->live ? DBG_COL_TYPE : DBG_COL_DEAD;
            int64_t vc = a->live ? DBG_COL_VAL  : DBG_COL_DEAD;
            char ab[24]; snprintf(ab, sizeof(ab), "0x%llx", (unsigned long long)a->addr);
            dbg_text_clip(vid, cxName, ry, a->name,   nc, sc, cxReg  - cw);
            dbg_text_clip(vid, cxReg,  ry, a->region, mc, sc, cxAddr - cw);
            dbg_text_clip(vid, cxAddr, ry, ab,        mc, sc, cxType - cw);
            dbg_text_clip(vid, cxType, ry, a->type,   tc, sc, cxVal  - cw);
            dbg_text_clip(vid, cxVal,  ry, a->value,  vc, sc, right);
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
            dbg_text_clip(vid, px + 12, ry, d->log[idx], d->log_col[idx], sc, right);
            ry += rowh;
        }
    }

    int64_t by = py + ph - footer_h + 6;
    cssc_video_fillrect(vid, px + 8, by, pw - 16, ch + 8, (int64_t)0x30101820);
    char pbuf[120];
    snprintf(pbuf, sizeof(pbuf), "probe>  %s_", d->probe);
    dbg_text_clip(vid, px + 12, by + 4, pbuf, DBG_COL_VAL, sc, right);
    if (d->last_read[0])
        dbg_text_clip(vid, px + 12, by + rowh + 4, d->last_read, DBG_COL_READ, sc, right);
    dbg_text_clip(vid, px + 12, py + ph - ch - 6,
        "Tab: tabs   F1: follow   hex+Enter: probe   Esc: close   drag corner: resize",
        DBG_COL_ADDR, sc, right);

    /* Resize grip — three diagonal ticks in the bottom-right corner. */
    int64_t gx = px + pw - 4, gy = py + ph - 4;
    for (int64_t i = 1; i <= 3; i++) {
        int64_t off = i * 4;
        cssc_video_fillrect(vid, gx - off, gy - 2, off, 2, (int64_t)0x60FFFFFF);
    }
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
    return m->n_titles;                     /* 1-based menu index */
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
/* Right-aligned status text on the menu bar (e.g. the live CSSC version). "" clears. */
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
    if (s->input_captured) return 0;             /* a modal prompt owns input */
    if (!(s->down && !s->prev_down)) return 0;   /* only act on click down-edge */
    int64_t mx = s->mx, my = s->my;
    int64_t glyph = 8 * m->scale;
    int64_t row_h = glyph + 10;
    /* click on a top-level title? */
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
    /* click in the open dropdown? */
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
        m->open = 0;   /* click outside the dropdown closes it */
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

/* ---- Prompt (modal single-line text input) ------------------------------ */
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
    cssc_video_fillrect(v, 0, 0, W, H, (int64_t)0x90000000);   /* dim backdrop */
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

/* ---- Tabs (open-file tab strip) ----------------------------------------- */
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
        if (!strcmp(t->paths[i], path)) { t->active = i; return; }   /* dedup */
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
            if (s->mx >= tx + tw - 22) t->close_hit = i + 1;   /* the close X */
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

/* ---- Browser (modal file/folder picker) --------------------------------- */
static int browser_has_parent(const char* dir) {
    return (int)strlen(dir) > 3;      /* longer than a drive root "C:/" */
}
static void browser_goup(char* dir) {
    int n = (int)strlen(dir);
    while (n > 1 && (dir[n - 1] == '/' || dir[n - 1] == '\\')) dir[--n] = 0;
    /* last path separator of EITHER kind -- Windows dirs use '\\' (e.g.
     * "C:\CSSC"), so a '/'-only search left `..` a no-op there. */
    char* pf = strrchr(dir, '/');
    char* pb = strrchr(dir, '\\');
    char* p = pf > pb ? pf : pb;
    if (!p) return;
    int idx = (int)(p - dir);
    if (idx == 0) { dir[1] = 0; return; }             /* "/x" -> "/" */
    if (idx == 2 && dir[1] == ':') { dir[3] = 0; }    /* "C:/x" -> "C:/" */
    else dir[idx] = 0;                                 /* ".../a/b" -> ".../a" */
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
/* asset icons (mirrors the file-tree): lazily load type icons from the icon dir
 * so the picker shows the user's assets instead of colored-square fallbacks. */
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
    if (b->isdir[idx]) return b->ico_folder;              /* ".." included -> folder */
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
    b->ico_loaded = 0;   /* (re)load on next draw */
}
static void browser_confirm(cssc_gui_browser* b) {
    if (b->mode == 1) {                       /* folder mode: choose current dir */
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
    int sel_moved = 0;
    while ((k = cssc_video_poll_key(s->vid)) != 0) {
        if (k == 0x26 && b->sel > 0) { b->sel--; sel_moved = 1; }
        else if (k == 0x28 && b->sel < b->n - 1) { b->sel++; sel_moved = 1; }
    }
    int64_t wh = cssc_video_wheel(s->vid);
    if (wh) { b->top -= (int)wh * 3; }              /* free wheel scroll */
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
                if (b->isdir[idx]) browser_enter(b, idx);   /* click a folder = enter */
            }
            return 0;
        }
    }
    if (sel_moved) {                               /* keep the cursor visible only after arrow-key nav */
        if (b->sel < b->top) b->top = b->sel;
        if (b->sel >= b->top + vis) b->top = b->sel - vis + 1;
    }
    int maxtop = b->n - vis; if (maxtop < 0) maxtop = 0;
    if (b->top > maxtop) b->top = maxtop;
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
    browser_ensure_icons(b);                   /* load the user's asset icons once */
    for (int r = 0; r < vis; r++) {
        int idx = b->top + r;
        if (idx >= b->n) break;
        int64_t ry = ly + (int64_t)r * row_h;
        if (idx == b->sel) cssc_video_fillrect(v, bx + 16, ry, lw, row_h, (int64_t)0x60C74DE0);
        void* icon = b->ico_loaded ? browser_pick_icon(b, idx) : NULL;
        if (icon) {
            gui_blit_sprite_fit(v, icon, bx + 24, ry + (row_h - glyph) / 2, 8 * b->scale);
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
