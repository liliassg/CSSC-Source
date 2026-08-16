

#include "cssc_allocwatcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

static int64_t _aw_string_bytes(CsscVal v) {
    if (CSSC_TYPE(v) != CSSC_TYPE_STRING || !v.data.ptr) return 0;
    return (int64_t)strlen((const char*)v.data.ptr);
}

static uint32_t _aw_estimate_used_bits(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_NULL:   return 0;
        case CSSC_TYPE_BOOL:   return 1;
        case CSSC_TYPE_INT: {

            int64_t x = v.data.i < 0 ? -v.data.i : v.data.i;
            uint32_t n = 0;
            while (x) { n++; x >>= 1; }
            if (v.data.i < 0) n++;
            return n < 8 ? 8 : n;
        }
        case CSSC_TYPE_FLOAT:  return 64;
        case CSSC_TYPE_STRING: {
            int64_t bytes = _aw_string_bytes(v);
            return (uint32_t)(bytes < 0 ? 0 : bytes * 8);
        }
        case CSSC_TYPE_VECTOR:

            return (uint32_t)(cssc_vector_size(v) * 32 + 32);
        case CSSC_TYPE_MAP:
            return (uint32_t)(cssc_map_size(v) * 64 + 32);
        case CSSC_TYPE_BIND:
            return (uint32_t)(cssc_bind_size(v) * 64 + 32);
        default:

            return 0;
    }
}

#define AW_MAX_ROWS 256

typedef struct {
    char     name[40];
    char     region;
    uint8_t  type_tag;
    uint32_t cap_bits;
    uint32_t used_bits;
} _AWRow;

typedef struct {
    _AWRow   rows[AW_MAX_ROWS];
    int      row_count;
    uint64_t total_cap_bits;
    uint64_t total_used_bits;
    uint32_t live_count;
    uint32_t total_allocs;
    uint32_t total_frees;
} _AWSnap;

static char _aw_region(CsscVal v) {
    if (CSSC_HAS_FLAG(v, CSSC_FLAG_STACK)) return 'S';
    if (CSSC_HAS_FLAG(v, CSSC_FLAG_HEAP))  return 'H';
    if (CSSC_HAS_FLAG(v, CSSC_FLAG_AUTO))  return 'A';
    return '?';
}

static bool _aw_walk_cb(const char* name, CsscVal value,
                        int32_t frame_idx, bool is_private,
                        void* userdata) {
    (void)frame_idx; (void)is_private;
    _AWSnap* snap = (_AWSnap*)userdata;
    char region = _aw_region(value);
    if (region == '?') return true;
    if (snap->row_count >= AW_MAX_ROWS) return false;
    uint32_t cap = CSSC_ALLOC_BITS(value);
    uint32_t used = _aw_estimate_used_bits(value);
    if (cap > 0 && used > cap) used = cap;
    _AWRow* r = &snap->rows[snap->row_count++];
    strncpy(r->name, name ? name : "", sizeof(r->name) - 1);
    r->name[sizeof(r->name) - 1] = '\0';
    r->region   = region;
    r->type_tag = (uint8_t)CSSC_TYPE(value);
    r->cap_bits = cap;
    r->used_bits = used;
    snap->total_cap_bits  += cap;
    snap->total_used_bits += used;
    snap->live_count++;
    return true;
}

static void _aw_collect(CsscScopeStack* scope, _AWSnap* snap) {
    snap->row_count = 0;
    snap->total_cap_bits = 0;
    snap->total_used_bits = 0;
    snap->live_count = 0;
    if (!scope) return;

    cssc_scope_walk(scope, _aw_walk_cb, snap);
}

static const char* _aw_type_name(uint8_t tag) {
    switch (tag) {
        case CSSC_TYPE_INT:    return "int";
        case CSSC_TYPE_FLOAT:  return "float";
        case CSSC_TYPE_BOOL:   return "bool";
        case CSSC_TYPE_STRING: return "string";
        case CSSC_TYPE_VECTOR: return "vector";
        case CSSC_TYPE_MAP:    return "map";
        case CSSC_TYPE_BIND:   return "bind";
        case CSSC_TYPE_FUNCTION: return "function";
        case CSSC_TYPE_SECTOR: return "sector";
        case CSSC_TYPE_OBJECT: return "object";
        default:               return "?";
    }
}

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static const wchar_t* _AW_WND_CLASS = L"CsscAllocWatcherWnd";
#define _AW_WM_REFRESH (WM_USER + 1)

typedef struct {
    HWND     hwnd;
    HANDLE   thread;
    DWORD    thread_id;
    int      should_stop;

    CRITICAL_SECTION snap_lock;
    _AWSnap  snap;
    char     target_name[64];
    DWORD    started_at_ms;
    CsscScopeStack* scope;
} _AWState;

static _AWState* _g_aw = NULL;

static HFONT _aw_make_font(int size, BOOL bold) {
    return CreateFontW(size, 0, 0, 0,
                       bold ? FW_BOLD : FW_NORMAL,
                       FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
}

static LRESULT CALLBACK _aw_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        HBRUSH bg = CreateSolidBrush(RGB(0x0c, 0x0c, 0x0c));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);
        if (!_g_aw) { EndPaint(hwnd, &ps); return 0; }
        EnterCriticalSection(&_g_aw->snap_lock);
        _AWSnap snap = _g_aw->snap;
        char target_buf[64];
        strncpy(target_buf, _g_aw->target_name, sizeof(target_buf) - 1);
        target_buf[sizeof(target_buf) - 1] = '\0';
        DWORD started_ms = _g_aw->started_at_ms;
        LeaveCriticalSection(&_g_aw->snap_lock);

        RECT status_rc = rc;
        status_rc.bottom = status_rc.top + 28;
        HBRUSH sbg = CreateSolidBrush(RGB(0x1a, 0x1a, 0x1a));
        FillRect(hdc, &status_rc, sbg);
        DeleteObject(sbg);

        HFONT logo_font = CreateFontW(16, 0, 0, 0, 900, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        HFONT prev_logo = (HFONT)SelectObject(hdc, logo_font);
        SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
        RECT logo_rc = { rc.left + 4, rc.top + 4, rc.left + 60, rc.top + 22 };
        DrawTextW(hdc, L"CSSC", 4, &logo_rc,
                  DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
        SelectObject(hdc, prev_logo);
        DeleteObject(logo_font);
        SetBkMode(hdc, TRANSPARENT);
        HFONT font_status = _aw_make_font(15, TRUE);
        HFONT old_font = (HFONT)SelectObject(hdc, font_status);
        SetTextColor(hdc, RGB(0x88, 0xdd, 0xff));
        DWORD elapsed_ms = GetTickCount() - started_ms;
        double cap_kib  = snap.total_cap_bits  / 8.0 / 1024.0;
        double used_kib = snap.total_used_bits / 8.0 / 1024.0;
        double fill = snap.total_cap_bits ?
            (double)snap.total_used_bits * 100.0 / (double)snap.total_cap_bits : 0.0;
        wchar_t status_buf[400];
        _snwprintf_s(status_buf, 400, _TRUNCATE,
            L" %hs | t+%6.2fs | live=%u | used %llu / %llu bits  "
            L"(%.2f / %.2f KiB, %5.1f%% full)  allocs=%u frees=%u",
            target_buf,
            (double)elapsed_ms / 1000.0,
            snap.live_count,
            (unsigned long long)snap.total_used_bits,
            (unsigned long long)snap.total_cap_bits,
            used_kib, cap_kib, fill,
            snap.total_allocs, snap.total_frees);
        RECT pad = status_rc;

        pad.left += 60; pad.top += 5; pad.right -= 8;
        DrawTextW(hdc, status_buf, -1, &pad,
                  DT_LEFT | DT_TOP | DT_NOPREFIX | DT_SINGLELINE);
        SelectObject(hdc, old_font);
        DeleteObject(font_status);

        HFONT font_body = _aw_make_font(14, FALSE);
        old_font = (HFONT)SelectObject(hdc, font_body);
        SetTextColor(hdc, RGB(0xdd, 0xdd, 0xdd));
        wchar_t body_buf[8192];
        int len = 0;
        len += _snwprintf_s(body_buf + len, 8192 - len, _TRUNCATE,
            L"-- live allocations --------------------------------\n"
            L"region name                 type     used /     cap fill%%\n");
        for (int i = 0; i < snap.row_count; i++) {
            const _AWRow* r = &snap.rows[i];
            double pct = r->cap_bits ? (double)r->used_bits * 100.0 / (double)r->cap_bits : 0.0;
            len += _snwprintf_s(body_buf + len, 8192 - len, _TRUNCATE,
                L" %hc     %-20.20hs %-8.8hs %5u / %7u %5.1f%%\n",
                r->region, r->name, _aw_type_name(r->type_tag),
                r->used_bits, r->cap_bits, pct);
            if (len >= 7800) break;
        }
        if (snap.row_count == 0) {
            _snwprintf_s(body_buf + len, 8192 - len, _TRUNCATE, L"  (no live allocations)\n");
        }
        RECT body_rc = rc;
        body_rc.top = status_rc.bottom + 8;
        body_rc.left += 8; body_rc.right -= 8; body_rc.bottom -= 8;
        DrawTextW(hdc, body_buf, -1, &body_rc,
                  DT_LEFT | DT_TOP | DT_NOPREFIX | DT_EXPANDTABS);
        SelectObject(hdc, old_font);
        DeleteObject(font_body);
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == _AW_WM_REFRESH) {
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    if (msg == WM_TIMER) {
        if (_g_aw && !_g_aw->should_stop) {
            _AWSnap fresh;
            memset(&fresh, 0, sizeof(fresh));
            _aw_collect(_g_aw->scope, &fresh);
            EnterCriticalSection(&_g_aw->snap_lock);

            uint32_t prev_live = _g_aw->snap.live_count;
            fresh.total_allocs = _g_aw->snap.total_allocs;
            fresh.total_frees  = _g_aw->snap.total_frees;
            if (fresh.live_count > prev_live)
                fresh.total_allocs += (fresh.live_count - prev_live);
            else if (fresh.live_count < prev_live)
                fresh.total_frees  += (prev_live - fresh.live_count);
            _g_aw->snap = fresh;
            LeaveCriticalSection(&_g_aw->snap_lock);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    if (msg == WM_CLOSE) {
        DestroyWindow(hwnd);
        return 0;
    }
    if (msg == WM_DESTROY) {
        if (_g_aw) _g_aw->should_stop = 1;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI _aw_thread_entry(LPVOID param) {
    _AWState* state = (_AWState*)param;
    HINSTANCE hinst = GetModuleHandleW(NULL);
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = _aw_wnd_proc;
    wc.hInstance = hinst;
    wc.lpszClassName = _AW_WND_CLASS;

    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(IDC_ARROW));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    state->hwnd = CreateWindowExW(
        0, _AW_WND_CLASS, L"cssc::allocwatcher",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 480,
        NULL, NULL, hinst, NULL);
    if (!state->hwnd) return 1;

    SetTimer(state->hwnd, 1, 100, NULL);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (state->should_stop) {
            PostMessageW(state->hwnd, WM_CLOSE, 0, 0);
            state->should_stop = 0;
        }
    }
    return 0;
}

CSSC_AW_API void cssc_alloc_watcher_start(CsscScopeStack* scope,
                                          const char* target_name) {
    if (_g_aw) cssc_alloc_watcher_stop();
    _g_aw = (_AWState*)calloc(1, sizeof(_AWState));
    if (!_g_aw) return;
    InitializeCriticalSection(&_g_aw->snap_lock);
    _g_aw->scope = scope;
    _g_aw->started_at_ms = GetTickCount();
    if (target_name && *target_name) {
        strncpy(_g_aw->target_name, target_name, sizeof(_g_aw->target_name) - 1);
    } else {
        strcpy(_g_aw->target_name, "<global>");
    }
    fprintf(stderr, "[allocwatcher] started, watching %s\n", _g_aw->target_name);
    fflush(stderr);
    _g_aw->thread = CreateThread(NULL, 0, _aw_thread_entry, _g_aw, 0, &_g_aw->thread_id);
}

CSSC_AW_API void cssc_alloc_watcher_stop(void) {
    if (!_g_aw) return;
    DWORD elapsed = GetTickCount() - _g_aw->started_at_ms;
    fprintf(stderr,
            "[allocwatcher] stopped after %.2fs - %u alloc / %u free\n",
            (double)elapsed / 1000.0,
            _g_aw->snap.total_allocs, _g_aw->snap.total_frees);
    fflush(stderr);
    _g_aw->should_stop = 1;
    if (_g_aw->hwnd) PostMessageW(_g_aw->hwnd, WM_CLOSE, 0, 0);
    if (_g_aw->thread) {
        WaitForSingleObject(_g_aw->thread, 1500);
        CloseHandle(_g_aw->thread);
    }
    DeleteCriticalSection(&_g_aw->snap_lock);
    free(_g_aw);
    _g_aw = NULL;
}

CSSC_AW_API CsscVal cssc_alloc_watcher_snapshot(void) {
    CsscVal m = cssc_map(8);
    if (!_g_aw) {
        cssc_map_set(m, "active", cssc_bool(false));
        cssc_map_set(m, "live_count", cssc_int(0));
        cssc_map_set(m, "total_capacity_bits", cssc_int(0));
        cssc_map_set(m, "total_used_bits", cssc_int(0));
        cssc_map_set(m, "fill_pct", cssc_float(0.0));
        cssc_map_set(m, "allocs", cssc_int(0));
        cssc_map_set(m, "frees", cssc_int(0));
        cssc_map_set(m, "elapsed_s", cssc_float(0.0));
        return m;
    }
    EnterCriticalSection(&_g_aw->snap_lock);
    _AWSnap snap = _g_aw->snap;
    DWORD started = _g_aw->started_at_ms;
    LeaveCriticalSection(&_g_aw->snap_lock);
    double fill = snap.total_cap_bits ?
        (double)snap.total_used_bits * 100.0 / (double)snap.total_cap_bits : 0.0;
    cssc_map_set(m, "active", cssc_bool(true));
    cssc_map_set(m, "live_count", cssc_int(snap.live_count));
    cssc_map_set(m, "total_capacity_bits", cssc_int((int64_t)snap.total_cap_bits));
    cssc_map_set(m, "total_used_bits", cssc_int((int64_t)snap.total_used_bits));
    cssc_map_set(m, "fill_pct", cssc_float(fill));
    cssc_map_set(m, "allocs", cssc_int(snap.total_allocs));
    cssc_map_set(m, "frees", cssc_int(snap.total_frees));
    cssc_map_set(m, "elapsed_s", cssc_float((double)(GetTickCount() - started) / 1000.0));
    return m;
}

#elif defined(CSSC_EMBEDDED) && !defined(CSSC_LINUX)

CSSC_AW_API void cssc_alloc_watcher_start(CsscScopeStack* scope,
                                          const char* target_name) {
    (void)scope; (void)target_name;
}
CSSC_AW_API void cssc_alloc_watcher_stop(void) {}
CSSC_AW_API CsscVal cssc_alloc_watcher_snapshot(void) {
    CsscVal m = cssc_map(8);
    cssc_map_set(m, "active", cssc_bool(false));
    cssc_map_set(m, "live_count", cssc_int(0));
    cssc_map_set(m, "total_capacity_bits", cssc_int(0));
    cssc_map_set(m, "total_used_bits", cssc_int(0));
    cssc_map_set(m, "fill_pct", cssc_float(0.0));
    cssc_map_set(m, "allocs", cssc_int(0));
    cssc_map_set(m, "frees", cssc_int(0));
    cssc_map_set(m, "elapsed_s", cssc_float(0.0));
    return m;
}

#else

#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    pthread_t thread;
    volatile int should_stop;
    char target_name[64];
    struct timespec started_at;
    CsscScopeStack* scope;
    pthread_mutex_t snap_lock;
    _AWSnap snap;
} _AWState;

static _AWState* _g_aw = NULL;

static double _aw_elapsed_s(struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start.tv_sec) +
           (now.tv_nsec - start.tv_nsec) / 1e9;
}

static void _aw_dump_stderr(_AWState* s) {
    pthread_mutex_lock(&s->snap_lock);
    _AWSnap snap = s->snap;
    pthread_mutex_unlock(&s->snap_lock);
    double fill = snap.total_cap_bits ?
        (double)snap.total_used_bits * 100.0 / (double)snap.total_cap_bits : 0.0;
    fprintf(stderr,
        "[allocwatcher] %s | t+%.2fs | live=%u | used %llu / %llu bits "
        "(%.1f%% full) | allocs=%u frees=%u\n",
        s->target_name, _aw_elapsed_s(s->started_at),
        snap.live_count,
        (unsigned long long)snap.total_used_bits,
        (unsigned long long)snap.total_cap_bits,
        fill, snap.total_allocs, snap.total_frees);
    fflush(stderr);
}

static void* _aw_thread_entry(void* param) {
    _AWState* state = (_AWState*)param;
    while (!state->should_stop) {
        _AWSnap fresh;
        memset(&fresh, 0, sizeof(fresh));
        _aw_collect(state->scope, &fresh);
        pthread_mutex_lock(&state->snap_lock);
        uint32_t prev_live = state->snap.live_count;
        fresh.total_allocs = state->snap.total_allocs;
        fresh.total_frees  = state->snap.total_frees;
        if (fresh.live_count > prev_live)
            fresh.total_allocs += (fresh.live_count - prev_live);
        else if (fresh.live_count < prev_live)
            fresh.total_frees  += (prev_live - fresh.live_count);
        state->snap = fresh;
        pthread_mutex_unlock(&state->snap_lock);
        _aw_dump_stderr(state);
        sleep(1);
    }
    return NULL;
}

CSSC_AW_API void cssc_alloc_watcher_start(CsscScopeStack* scope,
                                          const char* target_name) {
    if (_g_aw) cssc_alloc_watcher_stop();
    _g_aw = (_AWState*)calloc(1, sizeof(_AWState));
    if (!_g_aw) return;
    pthread_mutex_init(&_g_aw->snap_lock, NULL);
    _g_aw->scope = scope;
    clock_gettime(CLOCK_MONOTONIC, &_g_aw->started_at);
    if (target_name && *target_name) {
        strncpy(_g_aw->target_name, target_name, sizeof(_g_aw->target_name) - 1);
    } else {
        strcpy(_g_aw->target_name, "<global>");
    }
    fprintf(stderr, "[allocwatcher] started, watching %s\n", _g_aw->target_name);
    pthread_create(&_g_aw->thread, NULL, _aw_thread_entry, _g_aw);
}

CSSC_AW_API void cssc_alloc_watcher_stop(void) {
    if (!_g_aw) return;
    fprintf(stderr,
            "[allocwatcher] stopped after %.2fs - %u alloc / %u free\n",
            _aw_elapsed_s(_g_aw->started_at),
            _g_aw->snap.total_allocs, _g_aw->snap.total_frees);
    _g_aw->should_stop = 1;
    pthread_join(_g_aw->thread, NULL);
    pthread_mutex_destroy(&_g_aw->snap_lock);
    free(_g_aw);
    _g_aw = NULL;
}

CSSC_AW_API CsscVal cssc_alloc_watcher_snapshot(void) {
    CsscVal m = cssc_map(8);
    if (!_g_aw) {
        cssc_map_set(m, "active", cssc_bool(false));
        return m;
    }
    pthread_mutex_lock(&_g_aw->snap_lock);
    _AWSnap snap = _g_aw->snap;
    pthread_mutex_unlock(&_g_aw->snap_lock);
    double fill = snap.total_cap_bits ?
        (double)snap.total_used_bits * 100.0 / (double)snap.total_cap_bits : 0.0;
    cssc_map_set(m, "active", cssc_bool(true));
    cssc_map_set(m, "live_count", cssc_int(snap.live_count));
    cssc_map_set(m, "total_capacity_bits", cssc_int((int64_t)snap.total_cap_bits));
    cssc_map_set(m, "total_used_bits", cssc_int((int64_t)snap.total_used_bits));
    cssc_map_set(m, "fill_pct", cssc_float(fill));
    cssc_map_set(m, "allocs", cssc_int(snap.total_allocs));
    cssc_map_set(m, "frees", cssc_int(snap.total_frees));
    cssc_map_set(m, "elapsed_s", cssc_float(_aw_elapsed_s(_g_aw->started_at)));
    return m;
}

#endif
