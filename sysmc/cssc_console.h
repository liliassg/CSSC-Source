#ifndef CSSC_CONSOLE_H
#define CSSC_CONSOLE_H

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <psapi.h>
#  include <io.h>
#  pragma comment(lib, "psapi.lib")
#  define CC_SLEEP_MS(ms) Sleep((DWORD)(ms))
#  define CC_ISATTY(fd)   _isatty(fd)
#  define CC_FILENO(f)    _fileno(f)
#else
#  include <unistd.h>
#  include <sys/resource.h>
#  define CC_SLEEP_MS(ms) usleep((useconds_t)(ms) * 1000)
#  define CC_ISATTY(fd)   isatty(fd)
#  define CC_FILENO(f)    fileno(f)
#endif

#define CC_OUT  stderr

#define CC_ESC          "\x1b"
#define CC_RESET        CC_ESC "[0m"
#define CC_BOLD         CC_ESC "[1m"
#define CC_DIM          CC_ESC "[2m"
#define CC_UNDERLINE    CC_ESC "[4m"
#define CC_HOME         CC_ESC "[H"
#define CC_CLEAR_EOL    CC_ESC "[K"
#define CC_CLEAR_ALL    CC_ESC "[2J"
#define CC_SAVE_CUR     CC_ESC "[s"
#define CC_REST_CUR     CC_ESC "[u"
#define CC_HIDE_CUR     CC_ESC "[?25l"
#define CC_SHOW_CUR     CC_ESC "[?25h"
#define CC_ALT_SCREEN   CC_ESC "[?1049h"
#define CC_MAIN_SCREEN  CC_ESC "[?1049l"

#define CC_THEME_ACCENT     CC_ESC "[38;2;157;78;221m"
#define CC_THEME_VALUE      CC_ESC "[38;2;0;183;199m"
#define CC_THEME_SUCCESS    CC_ESC "[38;2;6;214;160m"
#define CC_THEME_WARNING    CC_ESC "[38;2;255;177;59m"
#define CC_THEME_ERROR      CC_ESC "[38;2;231;76;60m"
#define CC_THEME_FG         CC_ESC "[38;2;229;231;235m"
#define CC_THEME_DIM        CC_ESC "[38;2;90;90;109m"
#define CC_THEME_FRAME      CC_ESC "[38;2;88;73;152m"
#define CC_THEME_FRAME_HI   CC_ESC "[38;2;157;78;221m"
#define CC_THEME_HEADING    CC_ESC "[38;2;255;255;255m" CC_BOLD
#define CC_THEME_LABEL      CC_ESC "[38;2;180;180;200m"
#define CC_THEME_PIN_ON     CC_ESC "[38;2;6;214;160m" CC_BOLD
#define CC_THEME_PIN_OFF    CC_ESC "[38;2;80;80;100m"
#define CC_THEME_LED_LIT    CC_ESC "[38;2;255;213;79m" CC_BOLD
#define CC_THEME_LED_DARK   CC_ESC "[38;2;90;90;109m"
#define CC_THEME_WIRE       CC_ESC "[38;2;130;130;150m"
#define CC_THEME_FAULT      CC_ESC "[38;2;255;90;90m" CC_BOLD
#define CC_THEME_FAULT_HEAD CC_ESC "[48;2;120;30;30m" CC_ESC "[38;2;255;235;235m" CC_BOLD
#define CC_THEME_FIX        CC_ESC "[38;2;180;255;180m"
#define CC_THEME_TRACE      CC_ESC "[38;2;180;200;255m"
#define CC_THEME_PREFLIGHT  CC_ESC "[38;2;255;150;90m"

#define CC_BOX_H            "\xe2\x94\x80"  /* ─ */
#define CC_BOX_V            "\xe2\x94\x82"  /* │ */
#define CC_BOX_TL           "\xe2\x95\xad"  /* ╭ */
#define CC_BOX_TR           "\xe2\x95\xae"  /* ╮ */
#define CC_BOX_BL           "\xe2\x95\xb0"  /* ╰ */
#define CC_BOX_BR           "\xe2\x95\xaf"  /* ╯ */
#define CC_BOX_VL           "\xe2\x94\xa4"  /* ┤ */
#define CC_BOX_VR           "\xe2\x94\x9c"  /* ├ */
#define CC_DOT_ON           "\xe2\x97\x8f"  /* ● */
#define CC_DOT_OFF          "\xe2\x97\x8b"  /* ○ */
#define CC_LED_GLYPH        "\xe2\xac\xa4"  /* ⬤ */
#define CC_WIRE_GLYPH       "\xe2\x94\x80"  /* ─ */
#define CC_SPARK_LO         "\xe2\x96\x81"  /* ▁ */
#define CC_SPARK_HI         "\xe2\x96\x87"  /* ▇ */

typedef struct {
    int   initialised;
    int   ansi_enabled;
    int   stderr_is_tty;
    int   width;
    int   height;
    int   alt_screen;
    char  app_name[64];
    unsigned long long start_ms;
} cssc_console_state_t;

static cssc_console_state_t cssc__cc = {0};

static unsigned long long cssc_now_ms(void) {
#ifdef _WIN32
    return (unsigned long long)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
#endif
}

static size_t cssc_rss_kb(void) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return (size_t)(pmc.WorkingSetSize / 1024);
    }
    return 0;
#else
    struct rusage ru;
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
#  if defined(__APPLE__)
        return (size_t)(ru.ru_maxrss / 1024);
#  else
        return (size_t)ru.ru_maxrss;
#  endif
    }
    return 0;
#endif
}

static void cssc__cc_refresh_size(void) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(h, &csbi)) {
        int w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        int hh = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        cssc__cc.width  = (w  > 0) ? w  : 100;
        cssc__cc.height = (hh > 0) ? hh : 30;
        return;
    }
#endif
    cssc__cc.width  = 100;
    cssc__cc.height = 30;
}

static int cssc_console_width(void) {
    if (!cssc__cc.initialised) cssc__cc_refresh_size();
    cssc__cc_refresh_size();
    return cssc__cc.width;
}

static int cssc_console_height(void) {
    if (!cssc__cc.initialised) cssc__cc_refresh_size();
    cssc__cc_refresh_size();
    return cssc__cc.height;
}

static int cssc_console_is_tty(void) {
    return cssc__cc.stderr_is_tty;
}

static void cssc_console_shutdown(void) {
    if (!cssc__cc.initialised) return;
    if (cssc__cc.ansi_enabled) {
        fputs(CC_RESET CC_SHOW_CUR, CC_OUT);
        if (cssc__cc.alt_screen) fputs(CC_MAIN_SCREEN, CC_OUT);
        fputs("\n", CC_OUT);
        fflush(CC_OUT);
    }
    cssc__cc.initialised = 0;
}

static void cssc__cc_atexit(void) { cssc_console_shutdown(); }

#ifdef _WIN32
static BOOL WINAPI cssc__cc_console_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT
            || ctrl_type == CTRL_CLOSE_EVENT
            || ctrl_type == CTRL_BREAK_EVENT) {
        cssc_console_shutdown();
    }
    return FALSE;
}
#else
static void cssc__cc_signal_handler(int sig) {
    (void)sig;
    cssc_console_shutdown();
    _Exit(130);
}
#endif

static void cssc_console_init(const char* app_name) {
    if (cssc__cc.initialised) return;
    cssc__cc.initialised = 1;
    if (app_name && *app_name) {
        strncpy(cssc__cc.app_name, app_name,
                sizeof(cssc__cc.app_name) - 1);
    } else {
        strncpy(cssc__cc.app_name, "cssc app",
                sizeof(cssc__cc.app_name) - 1);
    }
    cssc__cc.app_name[sizeof(cssc__cc.app_name) - 1] = '\0';
    cssc__cc.stderr_is_tty = CC_ISATTY(CC_FILENO(CC_OUT)) ? 1 : 0;
    cssc__cc.ansi_enabled  = cssc__cc.stderr_is_tty;
#ifdef _WIN32
    if (cssc__cc.ansi_enabled) {
        HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(h, &mode)) {
            if (!SetConsoleMode(h,
                    mode | 0x0004 | 0x0008)) {
                cssc__cc.ansi_enabled = 0;
            }
        }
        UINT cp = GetConsoleOutputCP();
        if (cp != CP_UTF8) SetConsoleOutputCP(CP_UTF8);
        char title[128];
        snprintf(title, sizeof(title),
                  "CSSC HSim :: %s", cssc__cc.app_name);
        wchar_t wtitle[160];
        MultiByteToWideChar(CP_UTF8, 0, title, -1,
                             wtitle, (int)(sizeof(wtitle)/sizeof(wtitle[0])));
        SetConsoleTitleW(wtitle);
        SetConsoleCtrlHandler(cssc__cc_console_handler, TRUE);
    }
#else
    signal(SIGINT,  cssc__cc_signal_handler);
    signal(SIGTERM, cssc__cc_signal_handler);
#endif
    atexit(cssc__cc_atexit);
    cssc__cc.start_ms = cssc_now_ms();
    cssc__cc_refresh_size();
    if (cssc__cc.ansi_enabled) {
        cssc__cc.alt_screen = 1;
        fputs(CC_ALT_SCREEN CC_CLEAR_ALL CC_HOME CC_HIDE_CUR, CC_OUT);
        fflush(CC_OUT);
    }
}

static void cssc_console_goto(int row, int col) {
    if (!cssc__cc.ansi_enabled) return;
    fprintf(CC_OUT, CC_ESC "[%d;%dH", row, col);
}

static void cssc_console_clear_line(int row) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, 1);
    fputs(CC_CLEAR_EOL, CC_OUT);
}

static void cssc_console_clear_box(int row, int col, int w, int h) {
    if (!cssc__cc.ansi_enabled) return;
    for (int r = 0; r < h; r++) {
        cssc_console_goto(row + r, col);
        for (int c = 0; c < w; c++) fputc(' ', CC_OUT);
    }
}

static void cssc_widget_hbar(int row, int col, int w) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, col);
    fputs(CC_THEME_FRAME, CC_OUT);
    for (int i = 0; i < w; i++) fputs(CC_BOX_H, CC_OUT);
    fputs(CC_RESET, CC_OUT);
}

static void cssc_widget_panel(int row, int col, int w, int h,
                                const char* title) {
    if (!cssc__cc.ansi_enabled || w < 4 || h < 2) return;
    cssc_console_goto(row, col);
    fputs(CC_THEME_FRAME CC_BOX_TL, CC_OUT);
    for (int i = 0; i < w - 2; i++) fputs(CC_BOX_H, CC_OUT);
    fputs(CC_BOX_TR CC_RESET, CC_OUT);
    if (title && *title) {
        int tlen = (int)strlen(title);
        int max_title = w - 6;
        if (max_title < 1) max_title = 1;
        if (tlen > max_title) tlen = max_title;
        cssc_console_goto(row, col + 2);
        fprintf(CC_OUT, CC_THEME_FRAME_HI "%s " CC_THEME_HEADING "%.*s "
                        CC_THEME_FRAME_HI "%s" CC_RESET,
                CC_BOX_VL, tlen, title, CC_BOX_VR);
    }
    for (int r = 1; r < h - 1; r++) {
        cssc_console_goto(row + r, col);
        fputs(CC_THEME_FRAME CC_BOX_V CC_RESET, CC_OUT);
        cssc_console_goto(row + r, col + w - 1);
        fputs(CC_THEME_FRAME CC_BOX_V CC_RESET, CC_OUT);
    }
    cssc_console_goto(row + h - 1, col);
    fputs(CC_THEME_FRAME CC_BOX_BL, CC_OUT);
    for (int i = 0; i < w - 2; i++) fputs(CC_BOX_H, CC_OUT);
    fputs(CC_BOX_BR CC_RESET, CC_OUT);
}

static void cssc_widget_logo(int row, int col) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, col);
    fputs(CC_THEME_ACCENT CC_BOLD "[", CC_OUT);
    fputs(CC_THEME_VALUE  "::", CC_OUT);
    fputs(CC_THEME_ACCENT "CSSC", CC_OUT);
    fputs(CC_THEME_VALUE  "::", CC_OUT);
    fputs(CC_THEME_ACCENT "]" CC_RESET, CC_OUT);
}

static void cssc_widget_metric(int row, int col,
                                 const char* label, const char* value) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, col);
    fprintf(CC_OUT, CC_THEME_LABEL "%s" CC_RESET
                    " " CC_THEME_DIM ":" CC_RESET " "
                    CC_THEME_VALUE CC_BOLD "%s" CC_RESET,
            label ? label : "", value ? value : "");
}

static void cssc_widget_status_dot(int row, int col, int state) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, col);
    if (state > 0)      fputs(CC_THEME_PIN_ON  CC_DOT_ON  CC_RESET, CC_OUT);
    else if (state < 0) fputs(CC_THEME_WARNING "?"        CC_RESET, CC_OUT);
    else                fputs(CC_THEME_PIN_OFF CC_DOT_OFF CC_RESET, CC_OUT);
}

static void cssc_widget_led(int row, int col, int lit, const char* name) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, col);
    if (lit) fputs(CC_THEME_LED_LIT CC_LED_GLYPH " " CC_RESET, CC_OUT);
    else     fputs(CC_THEME_LED_DARK CC_LED_GLYPH " " CC_RESET, CC_OUT);
    fprintf(CC_OUT, CC_THEME_LABEL "%s" CC_RESET, name ? name : "");
}

static void cssc_widget_wire(int row, int col, int len) {
    if (!cssc__cc.ansi_enabled) return;
    cssc_console_goto(row, col);
    fputs(CC_THEME_WIRE, CC_OUT);
    for (int i = 0; i < len; i++) fputs(CC_WIRE_GLYPH, CC_OUT);
    fputs(CC_RESET, CC_OUT);
}

static void cssc_widget_progress(int row, int col, int width, int pct) {
    if (!cssc__cc.ansi_enabled || width < 4) return;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int fill = (width - 2) * pct / 100;
    cssc_console_goto(row, col);
    fputs(CC_THEME_FRAME "[" CC_RESET, CC_OUT);
    for (int i = 0; i < width - 2; i++) {
        if (i < fill) fputs(CC_THEME_VALUE  "=" CC_RESET, CC_OUT);
        else          fputs(CC_THEME_DIM    "." CC_RESET, CC_OUT);
    }
    fputs(CC_THEME_FRAME "]" CC_RESET, CC_OUT);
}

static int cssc__visible_len(const char* s) {
    int n = 0;
    int in_esc = 0;
    for (const char* p = s; *p; p++) {
        if (*p == '\x1b') { in_esc = 1; continue; }
        if (in_esc) { if (*p == 'm') in_esc = 0; continue; }
        if (((unsigned char)*p & 0xC0) == 0x80) continue;
        n++;
    }
    return n;
}

static void cssc_widget_corner_hud(const char* fmt, ...) {
    if (!cssc__cc.ansi_enabled || !fmt) return;
    char buf[320];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    int w = cssc_console_width();
    int visible = cssc__visible_len(buf);
    int col = w - visible;
    if (col < 1) col = 1;
    fputs(CC_SAVE_CUR, CC_OUT);
    cssc_console_goto(1, col);
    fputs(buf, CC_OUT);
    fputs(CC_RESET CC_REST_CUR, CC_OUT);
    fflush(CC_OUT);
}

static void cssc_widget_runtime_hud(int extras_cnt,
                                      const char* const* extra_labels,
                                      const char* const* extra_values) {
    if (!cssc__cc.ansi_enabled) return;
    unsigned long long elapsed = cssc_now_ms() - cssc__cc.start_ms;
    size_t rss_kb = cssc_rss_kb();
    char buf[480];
    int n = snprintf(buf, sizeof(buf),
                      CC_THEME_DIM "[" CC_THEME_ACCENT "CSSC" CC_THEME_DIM "] "
                      CC_THEME_LABEL "up"  CC_RESET " "
                      CC_THEME_VALUE "%llu.%03llus" CC_RESET " "
                      CC_THEME_DIM "|" CC_RESET " "
                      CC_THEME_LABEL "rss" CC_RESET " "
                      CC_THEME_VALUE "%zu KB" CC_RESET,
                      elapsed / 1000ULL, elapsed % 1000ULL, rss_kb);
    if (n < 0) return;
    for (int i = 0; i < extras_cnt && i < 6 && n < (int)sizeof(buf); i++) {
        if (!extra_labels || !extra_values) break;
        int added = snprintf(buf + n, sizeof(buf) - n,
                              " " CC_THEME_DIM "|" CC_RESET " "
                              CC_THEME_LABEL "%s" CC_RESET " "
                              CC_THEME_VALUE "%s" CC_RESET,
                              extra_labels[i] ? extra_labels[i] : "",
                              extra_values[i] ? extra_values[i] : "");
        if (added < 0) break;
        n += added;
    }
    cssc_widget_corner_hud("%s", buf);
}

typedef struct {
    int          severity;
    int          cause_code;
    const char*  kind;
    const char*  message;
    const char*  suggestion;
    const char*  source_file;
    int          source_line;
    int          target_pin;
    unsigned long long t_ms;
} cssc_fault_t;

static void cssc_widget_fault_panel(int row, int col, int w, int h,
                                      const cssc_fault_t* faults,
                                      int n_faults,
                                      int total_faults) {
    if (!cssc__cc.ansi_enabled || w < 12 || h < 3) return;
    cssc_console_clear_box(row, col, w, h);
    char title[64];
    if (total_faults > 0) {
        snprintf(title, sizeof(title), "faults  %d total", total_faults);
    } else {
        snprintf(title, sizeof(title), "faults");
    }
    cssc_widget_panel(row, col, w, h, title);
    int inner_top = row + 1;
    int inner_max = h - 2;
    if (n_faults == 0 || faults == NULL) {
        cssc_console_goto(inner_top, col + 3);
        fputs(CC_THEME_SUCCESS CC_DOT_ON CC_RESET
              "  " CC_THEME_DIM "no faults — chip happy" CC_RESET,
              CC_OUT);
        return;
    }
    int rows_per_fault = 3;
    int max_shown = inner_max / rows_per_fault;
    if (max_shown < 1) max_shown = 1;
    int start = (n_faults > max_shown) ? (n_faults - max_shown) : 0;
    int drawn = 0;
    for (int i = start; i < n_faults && drawn < max_shown; i++) {
        const cssc_fault_t* f = &faults[i];
        int r = inner_top + drawn * rows_per_fault;
        const char* sev_tag = (f->severity >= 2) ? "FAULT"
                              : (f->severity == 1) ? "WARN "
                              : "INFO ";
        const char* sev_col = (f->severity >= 2) ? CC_THEME_FAULT_HEAD
                              : (f->severity == 1) ? CC_THEME_WARNING
                              : CC_THEME_DIM;
        cssc_console_goto(r, col + 3);
        fprintf(CC_OUT, "%s %s %s "
                        CC_THEME_LABEL "%-22.22s" CC_RESET
                        "  " CC_THEME_DIM "cause" CC_RESET " "
                        CC_THEME_VALUE "%d" CC_RESET,
                sev_col, sev_tag, CC_RESET, f->kind ? f->kind : "?",
                f->cause_code);
        cssc_console_goto(r + 1, col + 3);
        fprintf(CC_OUT, "  " CC_THEME_FAULT "%.*s" CC_RESET,
                w - 6, f->message ? f->message : "");
        cssc_console_goto(r + 2, col + 3);
        if (f->source_file && *f->source_file) {
            fprintf(CC_OUT,
                    "  " CC_THEME_TRACE "src" CC_RESET " "
                    CC_THEME_VALUE "%s" CC_RESET CC_THEME_DIM ":" CC_RESET
                    CC_THEME_VALUE "%d" CC_RESET,
                    f->source_file, f->source_line);
        } else {
            fputs("  " CC_THEME_DIM "src: <unknown>" CC_RESET, CC_OUT);
        }
        if (f->suggestion && *f->suggestion) {
            fprintf(CC_OUT, "  " CC_THEME_DIM "fix" CC_RESET " "
                            CC_THEME_FIX "%.60s" CC_RESET,
                    f->suggestion);
        }
        drawn++;
    }
}

static void cssc_widget_fault_inline_print(const cssc_fault_t* f) {
    if (!f) return;
    fputs("\n", CC_OUT);
    fprintf(CC_OUT,
            "%s %s %s  " CC_THEME_LABEL "%s" CC_RESET
            "  " CC_THEME_DIM "cause" CC_RESET " "
            CC_THEME_VALUE "%d" CC_RESET "\n",
            (f->severity >= 2) ? CC_THEME_FAULT_HEAD : CC_THEME_WARNING,
            (f->severity >= 2) ? "FAULT" : "WARN ",
            CC_RESET, f->kind ? f->kind : "?", f->cause_code);
    fprintf(CC_OUT, "    " CC_THEME_FAULT "%s" CC_RESET "\n",
            f->message ? f->message : "");
    if (f->source_file && *f->source_file) {
        fprintf(CC_OUT,
                "    " CC_THEME_TRACE "src" CC_RESET " "
                CC_THEME_VALUE "%s" CC_RESET CC_THEME_DIM ":" CC_RESET
                CC_THEME_VALUE "%d" CC_RESET "\n",
                f->source_file, f->source_line);
    }
    if (f->suggestion && *f->suggestion) {
        fprintf(CC_OUT, "    " CC_THEME_DIM "fix" CC_RESET " "
                        CC_THEME_FIX "%s" CC_RESET "\n",
                f->suggestion);
    }
    fflush(CC_OUT);
}

static void cssc_widget_footer(const char* hint) {
    if (!cssc__cc.ansi_enabled) return;
    int h = cssc_console_height();
    int w = cssc_console_width();
    cssc_console_clear_line(h);
    cssc_console_goto(h, 2);
    fprintf(CC_OUT, CC_THEME_DIM "Ctrl-C to exit  " CC_THEME_VALUE "%s" CC_RESET,
            hint ? hint : "");
    (void)w;
    fflush(CC_OUT);
}

#endif
