/*
 * CSSC Native Runtime — Implementation
 * ======================================
 * Complete C runtime for compiled CSSC executables.
 *
 * Sections:
 *   1. Memory Management
 *   2. String Interning
 *   3. Value Constructors
 *   4. Value Access
 *   5. Reference Counting
 *   6. String Operations
 *   7. Vector Operations
 *   8. Map Operations
 *   9. Bind Operations
 *  10. Arithmetic & Comparison
 *  11. Type Coercion
 *  12. Scope Stack
 *  13. Sector & Object
 *  14. Function
 *  15. Builtin Functions (cssc::)
 *  16. Error Handling
 *  17. ASMH Hotloading Bridge
 *  18. Format Utilities
 *  19. Global State
 *
 * (c) 2026 Lilias Hatterscheidt — IncludeCPP / CSSeries
 */

#ifndef CSSC_RUNTIME_EXPORTS
#define CSSC_RUNTIME_EXPORTS
#endif
#include "cssc_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #include <winhttp.h>
    #define cssc_getcwd _getcwd
#elif defined(CSSC_EMBEDDED)
    /* Embedded targets (ESP32 / ESP8266 / Arduino AVR) have neither
     * unistd.h getcwd nor dlfcn.h dynamic loading. Provide a stub —
     * scripts that touch CWD or .cobj loading at runtime won't work
     * on a microcontroller anyway, so the stub is the right answer. */
    static char* _cssc_embedded_getcwd_stub(char* buf, size_t n) {
        if (buf && n > 0) buf[0] = '\0';
        return buf;
    }
    #define cssc_getcwd _cssc_embedded_getcwd_stub

    /* ── POSIX → Arduino-core forwarding for bare-metal builds ────
     * cssc_runtime.c has 40+ `#else (POSIX)` branches that call
     * clock_gettime / nanosleep / pthread_* / realpath / stat — these
     * don't exist (or aren't reliable) on AVR/ESP-IDF/Tasmota libcs.
     * Rather than #ifdef-ing every callsite, we forward the calls
     * to Arduino-core equivalents (delay/millis/micros/yield) so they
     * are REAL operations, not silent no-ops:
     *
     *   nanosleep / cssc::sleep   →  delay(ms)        (yields WDT + Wi-Fi)
     *   clock_gettime             →  micros()         (real wall time)
     *   pthread_*                 →  no-op (single-threaded MCU)
     *   realpath / mkdir          →  no-op (no filesystem path)
     *
     * The pthread_/realpath/mkdir set still no-ops because there's no
     * meaningful semantic on a microcontroller; everything timing- or
     * I/O-related forwards to a real call so scripts behave correctly. */
    #include <time.h>      /* struct timespec is in <time.h>; the
                            * xtensa libcs provide it but not
                            * clock_gettime. */
    #ifndef CLOCK_MONOTONIC
      #define CLOCK_MONOTONIC 1
    #endif
    /* Arduino-framework reachability: when our build pipeline includes
     * <Arduino.h> (always for ESP8266/ESP32/AVR via cssc build) we
     * forward sleep/time to delay()/millis()/micros() so the runtime
     * actually yields and reports real time. Critical for embedded:
     * the ESP8266 watchdog resets the chip after ~3s of synchronous
     * code without yield(), so silent no-op sleeps brick the script. */
    #if defined(__has_include) && __has_include(<Arduino.h>)
      #include <Arduino.h>
      #define CSSC_EMB_HAS_ARDUINO 1
    #endif
    /* AVR-specific watchdog header (needed by cssc_builtin_reboot on
     * atmega328p / atmega2560 / etc.). Pull at file scope — avr-gcc
     * doesn't accept `#include` from inside a function body, and the
     * header is small (~40 lines) so the cost on non-AVR builds is
     * zero anyway since it's gated on __AVR__. */
    #if defined(__AVR__) && defined(__has_include) && __has_include(<avr/wdt.h>)
      #include <avr/wdt.h>
    #endif
    /* avr-libc has strtol (32-bit long) but no strtoll (64-bit). Map
     * our `strtoll` callers down to `strtol` on AVR so the same
     * cssc_to_int / cssc_builtin_input code compiles for both
     * 8-bit and 32-bit Arduino-flavours without per-call #ifdef
     * branches scattered through the file. */
    #if defined(__AVR__)
      #define strtoll(s, e, b) strtol((s), (e), (b))
    #endif
    static inline int _cssc_emb_clock_gettime(int c, struct timespec* ts) {
        (void)c;
        if (!ts) return 0;
      #ifdef CSSC_EMB_HAS_ARDUINO
        /* micros() wraps every ~70 minutes on ESP8266 — for short
         * deltas (which is what cssc::time / cssc::detime are used
         * for) this is fine; for absolute wall-clock the user should
         * pull NTP via the http module. */
        unsigned long us = micros();
        ts->tv_sec  = (time_t)(us / 1000000UL);
        ts->tv_nsec = (long)((us % 1000000UL) * 1000UL);
      #else
        ts->tv_sec = 0; ts->tv_nsec = 0;
      #endif
        return 0;
    }
    #define clock_gettime(c, ts) _cssc_emb_clock_gettime((int)(c), (ts))
    static inline int _cssc_emb_nanosleep(const struct timespec* req,
                                          struct timespec* rem) {
        (void)rem;
        if (!req) return 0;
      #ifdef CSSC_EMB_HAS_ARDUINO
        /* Convert to milliseconds and call delay() — yields to the
         * watchdog + Wi-Fi background tasks. For sub-ms requests we
         * still call delay(0) which lets the WDT scheduler tick. */
        unsigned long ms = (unsigned long)req->tv_sec * 1000UL
                         + (unsigned long)(req->tv_nsec / 1000000L);
        if (ms == 0 && req->tv_nsec > 0) ms = 1;  /* round up sub-ms */
        delay(ms);
      #else
        (void)req;
      #endif
        return 0;
    }
    #define nanosleep _cssc_emb_nanosleep
    /* pthread_* shims — types are int handles, ops are no-ops.
     * Many embedded toolchains (newlib on xtensa-esp32) already
     * declare these via <sys/_pthreadtypes.h> pulled by <stdio.h>;
     * skip our typedefs when they exist to avoid redefinition. */
    #if !(defined(__has_include) && __has_include(<sys/_pthreadtypes.h>))
      typedef int pthread_t;
      typedef int pthread_mutex_t;
      typedef int pthread_attr_t;
      typedef int pthread_mutexattr_t;
    #endif
    static inline int _cssc_emb_pthread_create(pthread_t* t,
        const pthread_attr_t* a, void* (*f)(void*), void* arg) {
        (void)t; (void)a; (void)f; (void)arg; return 0;
    }
    static inline int _cssc_emb_pthread_join(pthread_t t, void** rv) {
        (void)t; if (rv) *rv = 0; return 0;
    }
    static inline int _cssc_emb_pthread_mutex_init(pthread_mutex_t* m,
        const pthread_mutexattr_t* a) { (void)m; (void)a; return 0; }
    static inline int _cssc_emb_pthread_mutex_destroy(pthread_mutex_t* m) {
        (void)m; return 0;
    }
    static inline int _cssc_emb_pthread_mutex_lock(pthread_mutex_t* m) {
        (void)m; return 0;
    }
    static inline int _cssc_emb_pthread_mutex_unlock(pthread_mutex_t* m) {
        (void)m; return 0;
    }
    #define pthread_create        _cssc_emb_pthread_create
    #define pthread_join          _cssc_emb_pthread_join
    #define pthread_mutex_init    _cssc_emb_pthread_mutex_init
    #define pthread_mutex_destroy _cssc_emb_pthread_mutex_destroy
    #define pthread_mutex_lock    _cssc_emb_pthread_mutex_lock
    #define pthread_mutex_unlock  _cssc_emb_pthread_mutex_unlock
    /* realpath returns NULL on failure — a safe default for the stub. */
    static inline char* _cssc_emb_realpath(const char* path, char* resolved) {
        (void)path; if (resolved) resolved[0] = '\0'; return resolved;
    }
    #define realpath _cssc_emb_realpath
    /* mkdir is referenced in two places without the embedded guard
     * I added earlier; provide a stub that returns 0 (success). */
    static inline int _cssc_emb_mkdir(const char* p, unsigned m) {
        (void)p; (void)m; return 0;
    }
    #ifndef mkdir
      #define mkdir _cssc_emb_mkdir
    #endif
#else
    #include <unistd.h>
    #include <dlfcn.h>
    #define cssc_getcwd getcwd
#endif

/* Cross-platform shims — these symbols are Win32-named throughout the
 * code but the actual values exist on every platform. We expose them
 * unconditionally so the .obj-loading and tmp-path code compiles for
 * Linux + embedded without #ifdef noise everywhere it's used. */
#ifndef MAX_PATH
  #define MAX_PATH 260
#endif

#if !defined(_WIN32)
  #if defined(CSSC_EMBEDDED)
    /* Bare-metal MCUs have no PID — return 0 as a stable fingerprint. */
    static inline unsigned long _cssc_get_pid(void) { return 0UL; }
  #else
    /* POSIX has getpid() in <unistd.h>, already included above. */
    static inline unsigned long _cssc_get_pid(void) { return (unsigned long)getpid(); }
  #endif
  #define GetCurrentProcessId _cssc_get_pid
#endif

/* =========================================================================
 * 0. EMBEDDED SIZING — knobs that govern static-table footprint
 * =========================================================================
 *
 * CSSC's runtime philosophy: minimal, bit-precise, only what the script
 * actually uses gets compiled. The static tables below default to sizes
 * tuned for desktop builds (kilobytes of headroom). On bare-metal MCUs
 * (ESP8266 has 80KB RAM total) those defaults instantly blow the budget
 * — `g_intern_table[4096]` alone weighs ~64KB.
 *
 * For `CSSC_EMBEDDED` builds we shrink every static table to the bare
 * minimum a typical IoT script touches: a handful of strings, a couple
 * of catch frames, a few hex scopes. Scripts that actually need more
 * can override these via -DMAX_DAEMONS=N etc. on the build flags.
 *
 * Win32-only subsystems (.cobj DLL loader, watermark thread, native
 * sound) are gated by `#if defined(_WIN32) && !defined(CSSC_EMBEDDED)`
 * at their definition sites so the corresponding code & globals are
 * dropped entirely on embedded.
 */
#ifdef CSSC_EMBEDDED
  #ifndef INTERN_TABLE_SIZE
    /* 32 slots × 12 bytes = 384 B. Most embedded scripts never intern
     * more than a dozen distinct strings (sensor names, format
     * specifiers). Tested at 32: zero collisions on a 200-line script.
     * Was 4096 (49 KB) on host, then 64 (768 B), now 32 (384 B). */
    #define INTERN_TABLE_SIZE      32
  #endif
  #ifndef CSSC_CATCH_STACK_MAX
    #define CSSC_CATCH_STACK_MAX   8     /* was 64; jmp_buf ~104B/slot */
  #endif
  #ifndef CSSC_LAST_ERROR_MAX
    #define CSSC_LAST_ERROR_MAX    256   /* was 2048 */
  #endif
  #ifndef MAX_HEX_VARS
    #define MAX_HEX_VARS           8     /* was 256 */
  #endif
  #ifndef MAX_HEX_SCOPES
    #define MAX_HEX_SCOPES         4     /* was 128 */
  #endif
  #ifndef MAX_DEFERRED
    #define MAX_DEFERRED           4     /* was 64 */
  #endif
  #ifndef MAX_DAEMONS
    #define MAX_DAEMONS            2     /* was 32 */
  #endif
  #ifndef MAX_COBJ_LOADED
    /* .cobj loader is Win32-only; on embedded the table is unreachable
     * but we keep the symbol so the cleanup hook still links. Min=1
     * because zero-length arrays are a constraint violation in C. */
    #define MAX_COBJ_LOADED        1
  #endif
#else
  #ifndef CSSC_LAST_ERROR_MAX
    #define CSSC_LAST_ERROR_MAX    2048
  #endif
#endif

/* =========================================================================
 * 1. MEMORY MANAGEMENT
 * ========================================================================= */

static size_t g_total_allocated = 0;

CSSC_API void* cssc_alloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        cssc_panic("out of memory");
    }
    g_total_allocated += size;
    return ptr;
}

CSSC_API void* cssc_realloc(void* ptr, size_t new_size) {
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size > 0) {
        cssc_panic("out of memory on realloc");
    }
    return new_ptr;
}

CSSC_API void cssc_free(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}

/* =========================================================================
 * 2. STRING INTERNING
 * ========================================================================= */

#ifndef INTERN_TABLE_SIZE
#define INTERN_TABLE_SIZE 4096
#endif

typedef struct {
    char*    str;
    uint32_t hash;
    bool     occupied;
} InternEntry;

static InternEntry g_intern_table[INTERN_TABLE_SIZE];
static bool g_intern_initialized = false;

CSSC_API uint32_t cssc_hash_string(const char* str) {
    uint32_t hash = 5381;
    if (!str) return 0;
    while (*str) {
        hash = ((hash << 5) + hash) + (unsigned char)*str;
        str++;
    }
    return hash;
}

CSSC_API const char* cssc_intern(const char* str) {
    if (!str) return NULL;
    if (!g_intern_initialized) {
        memset(g_intern_table, 0, sizeof(g_intern_table));
        g_intern_initialized = true;
    }
    uint32_t hash = cssc_hash_string(str);
    uint32_t idx = hash % INTERN_TABLE_SIZE;
    for (uint32_t i = 0; i < INTERN_TABLE_SIZE; i++) {
        uint32_t probe = (idx + i) % INTERN_TABLE_SIZE;
        if (!g_intern_table[probe].occupied) {
            size_t len = strlen(str);
            char* copy = (char*)cssc_alloc(len + 1);
            memcpy(copy, str, len + 1);
            g_intern_table[probe].str = copy;
            g_intern_table[probe].hash = hash;
            g_intern_table[probe].occupied = true;
            return g_intern_table[probe].str;
        }
        if (g_intern_table[probe].hash == hash && strcmp(g_intern_table[probe].str, str) == 0) {
            return g_intern_table[probe].str;
        }
    }
    /* Table full — just strdup without interning */
    size_t len = strlen(str);
    char* copy = (char*)cssc_alloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

CSSC_API bool cssc_intern_eq(const char* a, const char* b) {
    return a == b;
}

/* =========================================================================
 * 3. VALUE CONSTRUCTORS
 * ========================================================================= */

CSSC_API CsscVal cssc_null(void) {
    CsscVal v;
    v.tag = CSSC_TYPE_NULL;
    v.data.raw = 0;
    return v;
}

CSSC_API CsscVal cssc_int(int64_t value) {
    CsscVal v;
    v.tag = CSSC_TYPE_INT;
    v.data.i = value;
    return v;
}

CSSC_API CsscVal cssc_float(double value) {
    CsscVal v;
    v.tag = CSSC_TYPE_FLOAT;
    v.data.f = value;
    return v;
}

CSSC_API CsscVal cssc_bool(bool value) {
    CsscVal v;
    v.tag = CSSC_TYPE_BOOL;
    v.data.b = value;
    return v;
}

/* Sentinel for arena-owned CsscHeapHeader.refcount: cssc_release skips
 * such objects entirely — the arena owns them and will bulk-free at
 * scope_pop. Without this sentinel, a user calling cssc_release on a
 * string/vector built via cssc_string_concat (now arena-backed) would
 * double-free the block. Defined here so cssc_string and downstream
 * allocators can write the sentinel directly. */
#define CSSC_REFCOUNT_ARENA  0xFFFFFFFFu

CSSC_API CsscVal cssc_string(const char* str) {
    if (!str) return cssc_null();
    size_t len = strlen(str);
    /* Try arena first — string literals in expression statements are
     * the dominant allocation pattern in CSSC scripts, and they leak
     * heap blocks per call when allocated outside arena. By preferring
     * arena (per-frame bulk-free), tight loops like
     *   `display.text(0, 0, "CSSC Embedded", ...)`
     * no longer drift the heap downward. Strings stored in long-lived
     * scope entries (sector members, object slots) remain valid because
     * scope_pop bulk-frees the arena synchronously; if the user binds
     * a string into a sector that outlives the creating scope, they
     * should explicitly cssc_copy() — that path goes via heap. */
    extern CsscScopeStack* cssc_global_scope(void);
    CsscScopeStack* scope = cssc_global_scope();
    CsscString* s = scope ? (CsscString*)cssc_frame_arena_alloc(
                              scope, sizeof(CsscString) + len + 1) : NULL;
    if (s) {
        s->header.refcount = CSSC_REFCOUNT_ARENA;
    } else {
        /* Pre-runtime-init or arena OOM — heap fallback. */
        s = (CsscString*)cssc_alloc(sizeof(CsscString) + len + 1);
        s->header.refcount = 1;
    }
    s->header.type = CSSC_TYPE_STRING;
    s->header.capacity = (uint32_t)(len + 1);
    s->header.length = (uint32_t)len;
    memcpy(s->data, str, len + 1);
    CsscVal v;
    v.tag = CSSC_TYPE_STRING;
    v.data.ptr = s;
    return v;
}

CSSC_API CsscVal cssc_string_len(const char* str, size_t len) {
    CsscString* s = (CsscString*)cssc_alloc(sizeof(CsscString) + len + 1);
    s->header.refcount = 1;
    s->header.type = CSSC_TYPE_STRING;
    s->header.capacity = (uint32_t)(len + 1);
    s->header.length = (uint32_t)len;
    if (str) memcpy(s->data, str, len);
    s->data[len] = '\0';
    CsscVal v;
    v.tag = CSSC_TYPE_STRING;
    v.data.ptr = s;
    return v;
}

CSSC_API CsscVal cssc_string_owned(char* str) {
    if (!str) return cssc_null();
    size_t len = strlen(str);
    CsscString* s = (CsscString*)cssc_alloc(sizeof(CsscString) + len + 1);
    s->header.refcount = 1;
    s->header.type = CSSC_TYPE_STRING;
    s->header.capacity = (uint32_t)(len + 1);
    s->header.length = (uint32_t)len;
    memcpy(s->data, str, len + 1);
    cssc_free(str);
    CsscVal v;
    v.tag = CSSC_TYPE_STRING;
    v.data.ptr = s;
    return v;
}

CSSC_API CsscVal cssc_vector(size_t initial_capacity) {
    if (initial_capacity < 8) initial_capacity = 8;
    CsscVector* vec = (CsscVector*)cssc_alloc(sizeof(CsscVector));
    vec->header.refcount = 1;
    vec->header.type = CSSC_TYPE_VECTOR;
    vec->header.capacity = (uint32_t)initial_capacity;
    vec->header.length = 0;
    vec->items = (CsscVal*)cssc_alloc(sizeof(CsscVal) * initial_capacity);
    memset(vec->items, 0, sizeof(CsscVal) * initial_capacity);
    CsscVal v;
    v.tag = CSSC_TYPE_VECTOR;
    v.data.ptr = vec;
    return v;
}

CSSC_API CsscVal cssc_map(size_t initial_capacity) {
    if (initial_capacity < 16) initial_capacity = 16;
    /* Round up to power of 2 for efficient modulo */
    size_t cap = 16;
    while (cap < initial_capacity) cap <<= 1;
    CsscMap* map = (CsscMap*)cssc_alloc(sizeof(CsscMap));
    map->header.refcount = 1;
    map->header.type = CSSC_TYPE_MAP;
    map->header.capacity = 0;
    map->header.length = 0;
    map->bucket_count = (uint32_t)cap;
    map->buckets = (CsscMapEntry*)cssc_alloc(sizeof(CsscMapEntry) * cap);
    memset(map->buckets, 0, sizeof(CsscMapEntry) * cap);
    CsscVal v;
    v.tag = CSSC_TYPE_MAP;
    v.data.ptr = map;
    return v;
}

CSSC_API CsscVal cssc_bind(void) {
    CsscBind* bind = (CsscBind*)cssc_alloc(sizeof(CsscBind));
    bind->header.refcount = 1;
    bind->header.type = CSSC_TYPE_BIND;
    bind->header.capacity = 8;
    bind->header.length = 0;
    bind->pairs = (CsscVal*)cssc_alloc(sizeof(CsscVal) * 16); /* 8 pairs = 16 CsscVals */
    CsscVal v;
    v.tag = CSSC_TYPE_BIND;
    v.data.ptr = bind;
    return v;
}

/* =========================================================================
 * 4. VALUE ACCESS
 * ========================================================================= */

CSSC_API int64_t cssc_to_int(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_INT:    return v.data.i;
        case CSSC_TYPE_FLOAT:  return (int64_t)v.data.f;
        case CSSC_TYPE_BOOL:   return v.data.b ? 1 : 0;
        case CSSC_TYPE_STRING: {
            const char* s = ((CsscString*)v.data.ptr)->data;
            char* end;
            errno = 0;
            long long val = strtoll(s, &end, 0);
            if (errno == 0 && end != s) return (int64_t)val;
            return 0;
        }
        default: return 0;
    }
}

CSSC_API double cssc_to_float(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_INT:    return (double)v.data.i;
        case CSSC_TYPE_FLOAT:  return v.data.f;
        case CSSC_TYPE_BOOL:   return v.data.b ? 1.0 : 0.0;
        case CSSC_TYPE_STRING: {
            const char* s = ((CsscString*)v.data.ptr)->data;
            char* end;
            double val = strtod(s, &end);
            if (end != s) return val;
            return 0.0;
        }
        default: return 0.0;
    }
}

CSSC_API bool cssc_to_bool(CsscVal v) {
    return cssc_is_truthy(v);
}

/* Manual integer formatter — works on every libc including ESP8266
 * newlib (where %lld and %f printf paths require linker flags that
 * we can't assume). Writes into `buf` (≥24 bytes), returns its end
 * (one past the last digit). */
static char* _cssc_fmt_i64(char* buf, int64_t v) {
    if (v < 0) { *buf++ = '-'; v = -v; }
    char tmp[24]; int n = 0;
    if (v == 0) tmp[n++] = '0';
    else while (v > 0) { tmp[n++] = (char)('0' + (v % 10)); v /= 10; }
    while (n > 0) *buf++ = tmp[--n];
    *buf = '\0';
    return buf;
}

/* double → string. Buffer needs ≥40 bytes.
 *
 * On hosted targets we use the SHARED SHORTEST-ROUND-TRIP formatter
 * (native/cssc_fmt_f64.h, Dragon4 + Python-repr presentation) so the native
 * backends print floats exactly like the reference interpreter: `2.0`, `1.6`,
 * `0.3333333333333333`, `1e-05`. It is libc-free and allocation-free, but its
 * fixed-size bignum costs ~800 bytes of stack across the five working values —
 * fine on a host, too much for a 2 KB-RAM AVR. Embedded builds therefore keep
 * the compact 6-fractional-digit formatter below, which is also why this path
 * avoids libc %g (absent on ESP8266, or ~20 KB of float-printf). */
#if !defined(CSSC_EMBEDDED)
#include "cssc_fmt_f64.h"
static void _cssc_fmt_f64(char* buf, double v) {
    long long n = cssc_fmt_f64_shortest(buf, v);
    buf[n] = '\0';
}
#else
static void _cssc_fmt_f64(char* buf, double v) {
    if (v != v) { strcpy(buf, "nan"); return; }
    if (v < 0) { *buf++ = '-'; v = -v; }
    int64_t whole = (int64_t)v;
    double frac = v - (double)whole;
    char* p = _cssc_fmt_i64(buf, whole);
    if (frac > 0.0) {
        *p++ = '.';
        /* Emit 6 fractional digits then trim trailing zeros. */
        char fdigits[8]; int fn = 0;
        for (int i = 0; i < 6; i++) {
            frac *= 10.0;
            int d = (int)frac;
            if (d < 0) d = 0; else if (d > 9) d = 9;
            fdigits[fn++] = (char)('0' + d);
            frac -= (double)d;
        }
        while (fn > 1 && fdigits[fn - 1] == '0') fn--;
        for (int i = 0; i < fn; i++) *p++ = fdigits[i];
        *p = '\0';
    }
}
#endif /* CSSC_EMBEDDED */

CSSC_API const char* cssc_to_cstr(CsscVal v) {
    if (CSSC_IS_STRING(v) && v.data.ptr) {
        return ((CsscString*)v.data.ptr)->data;
    }
    /* Non-string values used to silently coerce to "" — that masked
     * bugs where scripts like `display.text(0, 0, time, ...)` drew
     * an empty string instead of the formatted number. We now format
     * every tag into a small static buffer ring so chained calls in
     * the same C statement (e.g. multi-arg display.text) don't
     * clobber each other's results.
     *
     * On embedded we deliberately avoid printf-%f / %g / %lld which
     * require -u _printf_float / -u _printf_long_long linker flags
     * the user's PIO build doesn't pass. Hand-rolled formatters
     * above keep this hot path libc-independent. */
    static char  _g_cstr_bufs[4][48];
    static int   _g_cstr_idx = 0;
    char* buf = _g_cstr_bufs[_g_cstr_idx];
    _g_cstr_idx = (_g_cstr_idx + 1) & 3;
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_NULL:
            return "null";
        case CSSC_TYPE_INT:
            _cssc_fmt_i64(buf, v.data.i);
            return buf;
        case CSSC_TYPE_FLOAT:
            _cssc_fmt_f64(buf, v.data.f);
            return buf;
        case CSSC_TYPE_BOOL:
            return v.data.b ? "true" : "false";
        case CSSC_TYPE_VECTOR: {
            char* p = buf;
            const char* lab = "[vec len=";
            while (*lab) *p++ = *lab++;
            int64_t n = v.data.ptr ? (int64_t)((CsscVector*)v.data.ptr)->header.length : 0;
            p = _cssc_fmt_i64(p, n);
            *p++ = ']'; *p = '\0';
            return buf;
        }
        case CSSC_TYPE_MAP: {
            char* p = buf;
            const char* lab = "[map len=";
            while (*lab) *p++ = *lab++;
            int64_t n = v.data.ptr ? (int64_t)((CsscMap*)v.data.ptr)->header.length : 0;
            p = _cssc_fmt_i64(p, n);
            *p++ = ']'; *p = '\0';
            return buf;
        }
        default: {
            /* Object/sector/function/pointer/etc. — short type tag
             * label, no pointer printout (avoids %p which also needs
             * libc linker flags on some embedded toolchains). */
            const char* tn = cssc_typeof_str(v);
            char* p = buf;
            *p++ = '[';
            while (*tn) *p++ = *tn++;
            *p++ = ']'; *p = '\0';
            return buf;
        }
    }
}

CSSC_API size_t cssc_strlen(CsscVal v) {
    if (CSSC_IS_STRING(v) && v.data.ptr) {
        return ((CsscString*)v.data.ptr)->header.length;
    }
    return 0;
}

CSSC_API bool cssc_is_truthy(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_NULL:   return false;
        case CSSC_TYPE_INT:    return v.data.i != 0;
        case CSSC_TYPE_FLOAT:  return v.data.f != 0.0;
        case CSSC_TYPE_BOOL:   return v.data.b;
        case CSSC_TYPE_STRING: {
            CsscString* s = (CsscString*)v.data.ptr;
            if (!s || s->header.length == 0) return false;
            if (s->header.length == 5 && strcmp(s->data, "false") == 0) return false;
            if (s->header.length == 1 && s->data[0] == '0') return false;
            if (s->header.length == 4 && strcmp(s->data, "null") == 0) return false;
            if (s->header.length == 4 && strcmp(s->data, "none") == 0) return false;
            return true;
        }
        case CSSC_TYPE_VECTOR:
            return v.data.ptr && ((CsscVector*)v.data.ptr)->header.length > 0;
        case CSSC_TYPE_MAP:
            return v.data.ptr && ((CsscMap*)v.data.ptr)->header.length > 0;
        default:
            return v.data.ptr != NULL;
    }
}

static const char* g_type_names[] = {
    "null", "int", "float", "bool", "string", "vector", "map", "bind",
    "function", "sector", "object", "pointer", "matrix", "iterator",
    "module", "method", "binding"
};

CSSC_API const char* cssc_typeof_str(CsscVal v) {
    CsscTypeTag t = CSSC_TYPE(v);
    if (t < CSSC_TYPE_MAX) return g_type_names[t];
    return "unknown";
}

/* =========================================================================
 * 5. REFERENCE COUNTING
 * ========================================================================= */

static CsscHeapHeader* cssc_get_header(CsscVal v) {
    if (v.data.ptr && CSSC_TYPE(v) >= CSSC_TYPE_STRING) {
        return (CsscHeapHeader*)v.data.ptr;
    }
    return NULL;
}

/* cssc_load_i64_at — read 8 bytes from an arbitrary slot address.
 *
 * Used by the v6 mirror epilogue (`_emit_mirror_epilogue_ret` in
 * cir_lower.py) for the live-ref re-fetch path. `mirror x;` records
 * the address of x's storage slot in a hidden function-local; the
 * function epilogue calls this helper at runtime to dereference it.
 *
 * Why a runtime helper instead of a CIR op: keeping the inttoptr +
 * load sequence out of the IR lets every backend (LLVM-IR for
 * x86_64 / avr / aarch64, hand-asm for xtensa) use its native
 * pointer load. The xtensa emitter inlines the equivalent
 * `l32i + l32i` sequence as a leaf function in xtensa_lx106.py;
 * the C runtime here covers host + AVR builds.
 *
 * The address argument is the raw bit pattern of a `cssc_obj_alloc`-
 * managed slot. CSSC's `#delete` zeroes the slot before releasing
 * the heap header, so a re-fetch after a delete reliably returns 0
 * (matches the interpreter's live-ref invalidation rule).
 */
CSSC_API uint64_t cssc_load_i64_at(uint64_t addr) {
    if (addr == 0u) return 0u;
    return *(uint64_t*)(uintptr_t)addr;
}

CSSC_API void cssc_retain(CsscVal v) {
    CsscHeapHeader* h = cssc_get_header(v);
    if (!h) return;
    if (h->refcount == 0xFFFFFFFFu) return;  /* arena-owned: sentinel preserved */
    h->refcount++;
}

static void cssc_release_internal(CsscVal v);

CSSC_API void cssc_release(CsscVal v) {
    CsscHeapHeader* h = cssc_get_header(v);
    if (!h) return;
    if (h->refcount == CSSC_REFCOUNT_ARENA) return;  /* arena-owned */
    if (h->refcount <= 1) {
        cssc_release_internal(v);
    } else {
        h->refcount--;
    }
}

static void cssc_release_internal(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_STRING:
            cssc_free(v.data.ptr);
            break;
        case CSSC_TYPE_VECTOR: {
            CsscVector* vec = (CsscVector*)v.data.ptr;
            for (uint32_t i = 0; i < vec->header.length; i++) {
                cssc_release(vec->items[i]);
            }
            cssc_free(vec->items);
            cssc_free(vec);
            break;
        }
        case CSSC_TYPE_MAP: {
            CsscMap* map = (CsscMap*)v.data.ptr;
            for (uint32_t i = 0; i < map->bucket_count; i++) {
                if (map->buckets[i].occupied) {
                    cssc_free((void*)map->buckets[i].key);
                    cssc_release(map->buckets[i].value);
                }
            }
            cssc_free(map->buckets);
            cssc_free(map);
            break;
        }
        case CSSC_TYPE_BIND: {
            CsscBind* bind = (CsscBind*)v.data.ptr;
            for (uint32_t i = 0; i < bind->header.length * 2; i++) {
                cssc_release(bind->pairs[i]);
            }
            cssc_free(bind->pairs);
            cssc_free(bind);
            break;
        }
        case CSSC_TYPE_OBJECT: {
            /* Run the object's `free { ... }` block before freeing its
             * members. Without this, objects stored in a sector / vector /
             * variable that go out of scope leak their free-block side
             * effects (e.g. #delete[CPUClock->clock_callbacks] never
             * fires). Mirrors the explicit `#delete[obj]` semantics. */
            cssc_object_free(v);
            cssc_free(v.data.ptr);
            break;
        }
        case CSSC_TYPE_SECTOR: {
            /* Symmetric: sector destruction tears down members (which
             * may themselves be objects → their free blocks run via the
             * OBJECT case above when cssc_frame_destroy releases them). */
            cssc_sector_free(v);
            cssc_free(v.data.ptr);
            break;
        }
        case CSSC_TYPE_FUNCTION: {
            CsscFunction* fn = (CsscFunction*)v.data.ptr;
            /* last_return is a non-owning borrow snapshot — don't
             * release. Just free the function header. */
            (void)fn;
            cssc_free(v.data.ptr);
            break;
        }
        default:
            if (v.data.ptr) cssc_free(v.data.ptr);
            break;
    }
}

CSSC_API CsscVal cssc_copy(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_NULL:
        case CSSC_TYPE_INT:
        case CSSC_TYPE_FLOAT:
        case CSSC_TYPE_BOOL:
            return v; /* immutable, no copy needed */
        case CSSC_TYPE_STRING:
            return cssc_string(cssc_to_cstr(v));
        case CSSC_TYPE_VECTOR: {
            CsscVector* src = (CsscVector*)v.data.ptr;
            CsscVal dst = cssc_vector(src->header.length > 0 ? src->header.length : 8);
            CsscVector* dvec = (CsscVector*)dst.data.ptr;
            for (uint32_t i = 0; i < src->header.length; i++) {
                dvec->items[i] = cssc_copy(src->items[i]);
            }
            dvec->header.length = src->header.length;
            return dst;
        }
        case CSSC_TYPE_MAP: {
            CsscMap* src = (CsscMap*)v.data.ptr;
            CsscVal dst = cssc_map(src->bucket_count);
            for (uint32_t i = 0; i < src->bucket_count; i++) {
                if (src->buckets[i].occupied) {
                    cssc_map_set(dst, src->buckets[i].key, cssc_copy(src->buckets[i].value));
                }
            }
            return dst;
        }
        default:
            cssc_retain(v);
            return v;
    }
}

/* =========================================================================
 * 6. STRING OPERATIONS
 * ========================================================================= */

CSSC_API CsscVal cssc_string_concat(CsscVal a, CsscVal b) {
    /* Arena-allocated result: the resulting CsscString is linked to
     * the current top scope frame's arena and gets bulk-freed when
     * that frame pops. This is the fix for the hot-loop heap-leak
     * pattern `for(...) display.text(..., x + "-" + y, ...)` which
     * previously exhausted the ESP8266 heap after ~325 iterations.
     * cssc_release is a no-op for arena-owned strings (refcount sentinel). */
    CsscVal sa = cssc_to_string_val(a);
    CsscVal sb = cssc_to_string_val(b);
    const char* ca = cssc_to_cstr(sa);
    const char* cb = cssc_to_cstr(sb);
    size_t la = strlen(ca), lb = strlen(cb);
    /* Allocate the entire CsscString + payload from arena. The 8-byte
     * arena header is prepended by cssc_frame_arena_alloc; the
     * pointer we receive is past it. */
    extern CsscScopeStack* cssc_global_scope(void);
    CsscScopeStack* scope = cssc_global_scope();
    size_t total = sizeof(CsscString) + la + lb + 1;
    CsscString* s = (CsscString*)cssc_frame_arena_alloc(scope, total);
    if (!s) {
        /* Arena OOM — fall back to heap path so script doesn't die. */
        char* buf = (char*)cssc_alloc(la + lb + 1);
        memcpy(buf, ca, la);
        memcpy(buf + la, cb, lb);
        buf[la + lb] = '\0';
        cssc_release(sa); cssc_release(sb);
        return cssc_string_owned(buf);
    }
    s->header.refcount = CSSC_REFCOUNT_ARENA;   /* never released by cssc_release */
    s->header.type = CSSC_TYPE_STRING;
    s->header.capacity = (uint32_t)(la + lb + 1);
    s->header.length = (uint32_t)(la + lb);
    memcpy(s->data, ca, la);
    memcpy(s->data + la, cb, lb);
    s->data[la + lb] = '\0';
    cssc_release(sa);
    cssc_release(sb);
    CsscVal v;
    v.tag = CSSC_TYPE_STRING;
    v.data.ptr = s;
    return v;
}

CSSC_API CsscVal cssc_string_repeat(CsscVal s, int64_t n) {
    if (n <= 0) return cssc_string("");
    const char* cs = cssc_to_cstr(s);
    size_t len = strlen(cs);
    size_t total = len * (size_t)n;
    char* buf = (char*)cssc_alloc(total + 1);
    for (int64_t i = 0; i < n; i++) {
        memcpy(buf + i * len, cs, len);
    }
    buf[total] = '\0';
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_string_substr(CsscVal s, int64_t start, int64_t length) {
    const char* cs = cssc_to_cstr(s);
    size_t slen = strlen(cs);
    if (start < 0) start = 0;
    if ((size_t)start >= slen) return cssc_string("");
    if (length < 0) length = (int64_t)slen - start;
    if ((size_t)(start + length) > slen) length = (int64_t)slen - start;
    return cssc_string_len(cs + start, (size_t)length);
}

CSSC_API CsscVal cssc_string_upper(CsscVal s) {
    const char* cs = cssc_to_cstr(s);
    size_t len = strlen(cs);
    char* buf = (char*)cssc_alloc(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = (char)toupper((unsigned char)cs[i]);
    buf[len] = '\0';
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_string_lower(CsscVal s) {
    const char* cs = cssc_to_cstr(s);
    size_t len = strlen(cs);
    char* buf = (char*)cssc_alloc(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = (char)tolower((unsigned char)cs[i]);
    buf[len] = '\0';
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_string_trim(CsscVal s) {
    const char* cs = cssc_to_cstr(s);
    while (*cs && isspace((unsigned char)*cs)) cs++;
    const char* end = cs + strlen(cs) - 1;
    while (end > cs && isspace((unsigned char)*end)) end--;
    return cssc_string_len(cs, (size_t)(end - cs + 1));
}

CSSC_API CsscVal cssc_string_replace(CsscVal s, CsscVal old_str, CsscVal new_str) {
    const char* src = cssc_to_cstr(s);
    const char* old_s = cssc_to_cstr(old_str);
    const char* new_s = cssc_to_cstr(new_str);
    size_t old_len = strlen(old_s);
    size_t new_len = strlen(new_s);
    if (old_len == 0) return cssc_string(src);
    /* Count occurrences */
    size_t count = 0;
    const char* p = src;
    while ((p = strstr(p, old_s)) != NULL) { count++; p += old_len; }
    size_t result_len = strlen(src) + count * (new_len - old_len);
    char* buf = (char*)cssc_alloc(result_len + 1);
    char* dst = buf;
    p = src;
    while (*p) {
        if (strncmp(p, old_s, old_len) == 0) {
            memcpy(dst, new_s, new_len);
            dst += new_len;
            p += old_len;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_string_split(CsscVal s, CsscVal separator) {
    const char* src = cssc_to_cstr(s);
    const char* sep = cssc_to_cstr(separator);
    size_t sep_len = strlen(sep);
    CsscVal result = cssc_vector(8);
    if (sep_len == 0) {
        /* Split into individual characters */
        size_t len = strlen(src);
        for (size_t i = 0; i < len; i++) {
            cssc_vector_push(result, cssc_string_len(src + i, 1));
        }
        return result;
    }
    const char* p = src;
    while (*p) {
        const char* found = strstr(p, sep);
        if (found) {
            cssc_vector_push(result, cssc_string_len(p, (size_t)(found - p)));
            p = found + sep_len;
        } else {
            cssc_vector_push(result, cssc_string(p));
            break;
        }
    }
    return result;
}

CSSC_API int64_t cssc_string_indexof(CsscVal s, CsscVal sub) {
    const char* cs = cssc_to_cstr(s);
    const char* csub = cssc_to_cstr(sub);
    const char* found = strstr(cs, csub);
    return found ? (int64_t)(found - cs) : -1;
}

CSSC_API bool cssc_string_contains(CsscVal s, CsscVal sub) {
    return cssc_string_indexof(s, sub) >= 0;
}

CSSC_API bool cssc_string_startswith(CsscVal s, CsscVal prefix) {
    const char* cs = cssc_to_cstr(s);
    const char* cp = cssc_to_cstr(prefix);
    size_t pl = strlen(cp);
    return strncmp(cs, cp, pl) == 0;
}

CSSC_API bool cssc_string_endswith(CsscVal s, CsscVal suffix) {
    const char* cs = cssc_to_cstr(s);
    const char* csu = cssc_to_cstr(suffix);
    size_t sl = strlen(cs), sul = strlen(csu);
    if (sul > sl) return false;
    return strcmp(cs + sl - sul, csu) == 0;
}

CSSC_API CsscVal cssc_string_char_at(CsscVal s, int64_t index) {
    const char* cs = cssc_to_cstr(s);
    size_t len = strlen(cs);
    if (index < 0 || (size_t)index >= len) return cssc_string("");
    return cssc_string_len(cs + index, 1);
}

CSSC_API CsscVal cssc_string_set_char(CsscVal s, int64_t index, CsscVal ch) {
    const char* cs = cssc_to_cstr(s);
    size_t len = strlen(cs);
    if (index < 0 || (size_t)index >= len) return cssc_copy(s);
    char* buf = (char*)cssc_alloc(len + 1);
    memcpy(buf, cs, len + 1);
    const char* c = cssc_to_cstr(ch);
    buf[index] = c[0] ? c[0] : '\0';
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_string_reverse(CsscVal s) {
    const char* cs = cssc_to_cstr(s);
    size_t len = strlen(cs);
    char* buf = (char*)cssc_alloc(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = cs[len - 1 - i];
    buf[len] = '\0';
    return cssc_string_owned(buf);
}

CSSC_API bool cssc_string_isdigit(CsscVal s) {
    const char* cs = cssc_to_cstr(s);
    if (!*cs) return false;
    while (*cs) { if (!isdigit((unsigned char)*cs)) return false; cs++; }
    return true;
}

CSSC_API bool cssc_string_isalpha(CsscVal s) {
    const char* cs = cssc_to_cstr(s);
    if (!*cs) return false;
    while (*cs) { if (!isalpha((unsigned char)*cs)) return false; cs++; }
    return true;
}

/* =========================================================================
 * 7. VECTOR OPERATIONS
 * ========================================================================= */

static void cssc_vector_grow(CsscVector* vec) {
    uint32_t new_cap = vec->header.capacity * 2;
    vec->items = (CsscVal*)cssc_realloc(vec->items, sizeof(CsscVal) * new_cap);
    memset(vec->items + vec->header.capacity, 0, sizeof(CsscVal) * (new_cap - vec->header.capacity));
    vec->header.capacity = new_cap;
}

CSSC_API void cssc_vector_push(CsscVal vec, CsscVal item) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return;
    CsscVector* v = (CsscVector*)vec.data.ptr;
    if (v->header.length >= v->header.capacity) {
        cssc_vector_grow(v);
    }
    cssc_retain(item);
    v->items[v->header.length++] = item;
}

CSSC_API CsscVal cssc_vector_pop(CsscVal vec) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return cssc_null();
    CsscVector* v = (CsscVector*)vec.data.ptr;
    if (v->header.length == 0) return cssc_null();
    v->header.length--;
    CsscVal item = v->items[v->header.length];
    v->items[v->header.length] = cssc_null();
    return item;
}

CSSC_API CsscVal cssc_vector_get(CsscVal vec, int64_t index) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return cssc_null();
    CsscVector* v = (CsscVector*)vec.data.ptr;
    if (index < 0 || (uint32_t)index >= v->header.length) return cssc_null();
    return v->items[index];
}

CSSC_API void cssc_vector_set(CsscVal vec, int64_t index, CsscVal item) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return;
    CsscVector* v = (CsscVector*)vec.data.ptr;
    if (index < 0 || (uint32_t)index >= v->header.length) return;
    cssc_release(v->items[index]);
    cssc_retain(item);
    v->items[index] = item;
}

CSSC_API int64_t cssc_vector_size(CsscVal vec) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return 0;
    return ((CsscVector*)vec.data.ptr)->header.length;
}

CSSC_API void cssc_vector_clear(CsscVal vec) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return;
    CsscVector* v = (CsscVector*)vec.data.ptr;
    for (uint32_t i = 0; i < v->header.length; i++) {
        cssc_release(v->items[i]);
    }
    v->header.length = 0;
}

CSSC_API void cssc_vector_erase(CsscVal vec, int64_t index) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return;
    CsscVector* v = (CsscVector*)vec.data.ptr;
    if (index < 0 || (uint32_t)index >= v->header.length) return;
    cssc_release(v->items[index]);
    memmove(&v->items[index], &v->items[index + 1],
            sizeof(CsscVal) * (v->header.length - index - 1));
    v->header.length--;
}

CSSC_API void cssc_vector_insert(CsscVal vec, int64_t index, CsscVal item) {
    if (!CSSC_IS_VECTOR(vec) || !vec.data.ptr) return;
    CsscVector* v = (CsscVector*)vec.data.ptr;
    if (index < 0) index = 0;
    if ((uint32_t)index > v->header.length) index = v->header.length;
    if (v->header.length >= v->header.capacity) cssc_vector_grow(v);
    memmove(&v->items[index + 1], &v->items[index],
            sizeof(CsscVal) * (v->header.length - index));
    cssc_retain(item);
    v->items[index] = item;
    v->header.length++;
}

CSSC_API CsscVal cssc_vector_first(CsscVal vec) {
    return cssc_vector_get(vec, 0);
}

CSSC_API CsscVal cssc_vector_last(CsscVal vec) {
    int64_t sz = cssc_vector_size(vec);
    return sz > 0 ? cssc_vector_get(vec, sz - 1) : cssc_null();
}

CSSC_API CsscVal cssc_vector_slice(CsscVal vec, int64_t start, int64_t end) {
    int64_t sz = cssc_vector_size(vec);
    if (start < 0) start = 0;
    if (end < 0 || end > sz) end = sz;
    CsscVal result = cssc_vector(end > start ? (size_t)(end - start) : 1);
    for (int64_t i = start; i < end; i++) {
        cssc_vector_push(result, cssc_copy(cssc_vector_get(vec, i)));
    }
    return result;
}

CSSC_API CsscVal cssc_vector_sort(CsscVal vec) {
    /* Simple insertion sort — works for CSSC's typical small arrays */
    int64_t sz = cssc_vector_size(vec);
    CsscVal result = cssc_vector(sz > 0 ? (size_t)sz : 1);
    CsscVector* rv = (CsscVector*)result.data.ptr;
    for (int64_t i = 0; i < sz; i++) {
        cssc_vector_push(result, cssc_copy(cssc_vector_get(vec, i)));
    }
    for (int64_t i = 1; i < sz; i++) {
        CsscVal key = rv->items[i];
        int64_t j = i - 1;
        while (j >= 0 && cssc_gt(rv->items[j], key)) {
            rv->items[j + 1] = rv->items[j];
            j--;
        }
        rv->items[j + 1] = key;
    }
    return result;
}

CSSC_API CsscVal cssc_vector_reverse(CsscVal vec) {
    int64_t sz = cssc_vector_size(vec);
    CsscVal result = cssc_vector(sz > 0 ? (size_t)sz : 1);
    for (int64_t i = sz - 1; i >= 0; i--) {
        cssc_vector_push(result, cssc_copy(cssc_vector_get(vec, i)));
    }
    return result;
}

CSSC_API bool cssc_vector_contains(CsscVal vec, CsscVal item) {
    return cssc_vector_indexof(vec, item) >= 0;
}

CSSC_API int64_t cssc_vector_indexof(CsscVal vec, CsscVal item) {
    int64_t sz = cssc_vector_size(vec);
    for (int64_t i = 0; i < sz; i++) {
        if (cssc_eq(cssc_vector_get(vec, i), item)) return i;
    }
    return -1;
}

/* =========================================================================
 * 8. MAP OPERATIONS
 * ========================================================================= */

static CsscMapEntry* cssc_map_find_bucket(CsscMap* map, const char* key, uint32_t hash) {
    uint32_t idx = hash & (map->bucket_count - 1);
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        uint32_t probe = (idx + i) & (map->bucket_count - 1);
        CsscMapEntry* e = &map->buckets[probe];
        if (!e->occupied) return e;
        if (e->hash == hash && strcmp(e->key, key) == 0) return e;
    }
    return NULL;
}

static void cssc_map_rehash(CsscMap* map) {
    uint32_t old_count = map->bucket_count;
    CsscMapEntry* old_buckets = map->buckets;
    map->bucket_count = old_count * 2;
    map->buckets = (CsscMapEntry*)cssc_alloc(sizeof(CsscMapEntry) * map->bucket_count);
    memset(map->buckets, 0, sizeof(CsscMapEntry) * map->bucket_count);
    for (uint32_t i = 0; i < old_count; i++) {
        if (old_buckets[i].occupied) {
            CsscMapEntry* e = cssc_map_find_bucket(map, old_buckets[i].key, old_buckets[i].hash);
            *e = old_buckets[i];
        }
    }
    cssc_free(old_buckets);
}

CSSC_API void cssc_map_set(CsscVal map_val, const char* key, CsscVal value) {
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr || !key) return;
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    if (map->header.length * 4 >= map->bucket_count * 3) {
        cssc_map_rehash(map);
    }
    uint32_t hash = cssc_hash_string(key);
    CsscMapEntry* e = cssc_map_find_bucket(map, key, hash);
    if (!e) return;
    if (e->occupied) {
        cssc_release(e->value);
        cssc_retain(value);
        e->value = value;
    } else {
        size_t klen = strlen(key);
        e->key = (char*)cssc_alloc(klen + 1);
        memcpy(e->key, key, klen + 1);
        e->hash = hash;
        cssc_retain(value);
        e->value = value;
        e->occupied = true;
        map->header.length++;
    }
}

CSSC_API CsscVal cssc_map_get(CsscVal map_val, const char* key) {
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr || !key) return cssc_null();
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    uint32_t hash = cssc_hash_string(key);
    CsscMapEntry* e = cssc_map_find_bucket(map, key, hash);
    if (e && e->occupied) return e->value;
    return cssc_null();
}

CSSC_API bool cssc_map_has(CsscVal map_val, const char* key) {
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr || !key) return false;
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    uint32_t hash = cssc_hash_string(key);
    CsscMapEntry* e = cssc_map_find_bucket(map, key, hash);
    return e && e->occupied;
}

CSSC_API void cssc_map_remove(CsscVal map_val, const char* key) {
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr || !key) return;
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    uint32_t hash = cssc_hash_string(key);
    CsscMapEntry* e = cssc_map_find_bucket(map, key, hash);
    if (e && e->occupied) {
        cssc_free((void*)e->key);
        cssc_release(e->value);
        e->occupied = false;
        e->key = NULL;
        map->header.length--;
    }
}

CSSC_API int64_t cssc_map_size(CsscVal map_val) {
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr) return 0;
    return ((CsscMap*)map_val.data.ptr)->header.length;
}

CSSC_API CsscVal cssc_map_keys(CsscVal map_val) {
    CsscVal result = cssc_vector(8);
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr) return result;
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        if (map->buckets[i].occupied) {
            cssc_vector_push(result, cssc_string(map->buckets[i].key));
        }
    }
    return result;
}

CSSC_API CsscVal cssc_map_values(CsscVal map_val) {
    CsscVal result = cssc_vector(8);
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr) return result;
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        if (map->buckets[i].occupied) {
            cssc_vector_push(result, map->buckets[i].value);
        }
    }
    return result;
}

CSSC_API void cssc_map_clear(CsscVal map_val) {
    if (!CSSC_IS_MAP(map_val) || !map_val.data.ptr) return;
    CsscMap* map = (CsscMap*)map_val.data.ptr;
    for (uint32_t i = 0; i < map->bucket_count; i++) {
        if (map->buckets[i].occupied) {
            cssc_free((void*)map->buckets[i].key);
            cssc_release(map->buckets[i].value);
            map->buckets[i].occupied = false;
        }
    }
    map->header.length = 0;
}

/* =========================================================================
 * 9. BIND OPERATIONS
 * ========================================================================= */

CSSC_API void cssc_bind_add(CsscVal bind_val, CsscVal key, CsscVal value) {
    if (CSSC_TYPE(bind_val) != CSSC_TYPE_BIND || !bind_val.data.ptr) return;
    CsscBind* bind = (CsscBind*)bind_val.data.ptr;
    if (bind->header.length >= bind->header.capacity) {
        uint32_t new_cap = bind->header.capacity * 2;
        bind->pairs = (CsscVal*)cssc_realloc(bind->pairs, sizeof(CsscVal) * new_cap * 2);
        bind->header.capacity = new_cap;
    }
    uint32_t idx = bind->header.length * 2;
    cssc_retain(key);
    cssc_retain(value);
    bind->pairs[idx] = key;
    bind->pairs[idx + 1] = value;
    bind->header.length++;
}

CSSC_API CsscVal cssc_bind_get_key(CsscVal bind_val, int64_t pair_index) {
    if (!bind_val.data.ptr) return cssc_null();
    CsscBind* bind = (CsscBind*)bind_val.data.ptr;
    if (pair_index < 0 || (uint32_t)pair_index >= bind->header.length) return cssc_null();
    return bind->pairs[pair_index * 2];
}

CSSC_API CsscVal cssc_bind_get_value(CsscVal bind_val, int64_t pair_index) {
    if (!bind_val.data.ptr) return cssc_null();
    CsscBind* bind = (CsscBind*)bind_val.data.ptr;
    if (pair_index < 0 || (uint32_t)pair_index >= bind->header.length) return cssc_null();
    return bind->pairs[pair_index * 2 + 1];
}

CSSC_API int64_t cssc_bind_size(CsscVal bind_val) {
    if (!bind_val.data.ptr) return 0;
    return ((CsscBind*)bind_val.data.ptr)->header.length;
}

CSSC_API void cssc_bind_addmap(CsscVal bind_val, CsscVal map_val) {
    if (CSSC_TYPE(bind_val) != CSSC_TYPE_BIND || !bind_val.data.ptr) return;
    /* Map source — append each occupied bucket as a (key, value) pair. */
    if (CSSC_IS_MAP(map_val) && map_val.data.ptr) {
        CsscMap* map = (CsscMap*)map_val.data.ptr;
        for (uint32_t i = 0; i < map->bucket_count; i++) {
            if (map->buckets[i].occupied) {
                cssc_bind_add(bind_val, cssc_string(map->buckets[i].key), map->buckets[i].value);
            }
        }
        return;
    }
    /* Bind source — append each existing pair. Lets users splice chains. */
    if (CSSC_TYPE(map_val) == CSSC_TYPE_BIND && map_val.data.ptr) {
        CsscBind* src = (CsscBind*)map_val.data.ptr;
        for (uint32_t i = 0; i < src->header.length; i++) {
            cssc_bind_add(bind_val, src->pairs[i * 2], src->pairs[i * 2 + 1]);
        }
    }
}

/* =========================================================================
 * 8b. #DELMEMBER — soft wipes (release heap, keep size/cap)
 *
 * Polymorphic dispatch on CsscVal's type tag. Each branch wipes the
 * "membership content" of the targeted entry/entries while preserving
 * the container's identity, count, and capacity. The container handle
 * itself remains live — re-use after #delmember is supported.
 *
 * Vector: `_at(i)` overwrites slot i with cssc_null(); `_all` overwrites
 *         every slot. POD-storage vectors (no allocated values) still
 *         see the slot zeroed.
 * Map:    `_at(i)` releases bucket i's key+value (sets occupied=false);
 *         `_all` clears the whole bucket table. Bucket count stays.
 * Bind:   `_at(i)` releases pair i's (key, value); `_all` walks every
 *         pair.
 * Object: `_all` releases every heap-typed member (string/vector/map/
 *         bind/sub-object) — POD members stay; `_at` is ill-defined
 *         and silently no-ops.
 * String: `_all` replaces the heap string contents with "" (capacity
 *         is freed and re-malloc'd to length-1); `_at(i)` zeros out
 *         the i-th byte (rare use, supports
 *         `#delmember[mystr[0]]` patterns).
 * Other:  PODs are no-ops by definition — there's no heap content.
 * ========================================================================= */
CSSC_API void cssc_delmember_all(CsscVal target) {
    if (!target.data.ptr && CSSC_TYPE(target) != CSSC_TYPE_INT
        && CSSC_TYPE(target) != CSSC_TYPE_FLOAT
        && CSSC_TYPE(target) != CSSC_TYPE_BOOL) {
        return;
    }
    int tag = CSSC_TYPE(target);
    if (tag == CSSC_TYPE_VECTOR) {
        int64_t n = cssc_vector_size(target);
        for (int64_t i = 0; i < n; i++) {
            cssc_vector_set(target, i, cssc_null());
        }
        return;
    }
    if (tag == CSSC_TYPE_MAP) {
        CsscMap* m = (CsscMap*)target.data.ptr;
        for (uint32_t i = 0; i < m->bucket_count; i++) {
            if (m->buckets[i].occupied) {
                free(m->buckets[i].key);
                m->buckets[i].key = NULL;
                m->buckets[i].value = cssc_null();
                m->buckets[i].occupied = 0;
            }
        }
        return;
    }
    if (tag == CSSC_TYPE_BIND) {
        CsscBind* b = (CsscBind*)target.data.ptr;
        for (uint32_t i = 0; i < b->header.length; i++) {
            b->pairs[i * 2]     = cssc_null();
            b->pairs[i * 2 + 1] = cssc_null();
        }
        return;
    }
    if (tag == CSSC_TYPE_OBJECT) {
        /* Walk the instance's member-scope frame; replace each heap-typed
         * entry's value with cssc_null(). The Object handle (its
         * label-method dispatch table) survives. `members` is a
         * CsscScopeFrame (hash-set of (name, value) entries indexed by
         * an internal `entries[]` array with `count` populated slots);
         * we iterate the entire capacity and skip unoccupied slots. */
        CsscObject* obj = (CsscObject*)target.data.ptr;
        if (!obj) return;
        for (uint32_t i = 0; i < obj->members.capacity; i++) {
            if (obj->members.entries[i].occupied) {
                obj->members.entries[i].value = cssc_null();
            }
        }
        return;
    }
    if (tag == CSSC_TYPE_STRING) {
        /* CsscString stores its `length`/`capacity` inside the
         * CsscHeapHeader (CsscString = { CsscHeapHeader header;
         * char data[]; }). The data buffer is allocated inline with
         * the header via cssc_alloc; resetting "all" means truncating
         * the logical length to 0 without freeing the backing buffer
         * (which is part of the same allocation). */
        CsscString* s = (CsscString*)target.data.ptr;
        if (!s) return;
        s->header.length = 0;
        if (s->header.capacity > 0) {
            s->data[0] = '\0';
        }
        return;
    }
    /* POD types — nothing to release. */
}

CSSC_API void cssc_delmember_at(CsscVal target, CsscVal index_val) {
    int tag = CSSC_TYPE(target);
    int64_t i = cssc_to_int(index_val);
    if (tag == CSSC_TYPE_VECTOR) {
        if (i < 0 || i >= cssc_vector_size(target)) return;
        cssc_vector_set(target, i, cssc_null());
        return;
    }
    if (tag == CSSC_TYPE_MAP) {
        CsscMap* m = (CsscMap*)target.data.ptr;
        if (!m || i < 0 || (uint32_t)i >= m->bucket_count) return;
        if (m->buckets[i].occupied) {
            free(m->buckets[i].key);
            m->buckets[i].key = NULL;
            m->buckets[i].value = cssc_null();
            m->buckets[i].occupied = 0;
        }
        return;
    }
    if (tag == CSSC_TYPE_BIND) {
        CsscBind* b = (CsscBind*)target.data.ptr;
        if (!b || i < 0 || (uint32_t)i >= b->header.length) return;
        b->pairs[i * 2]     = cssc_null();
        b->pairs[i * 2 + 1] = cssc_null();
        return;
    }
    if (tag == CSSC_TYPE_STRING) {
        CsscString* s = (CsscString*)target.data.ptr;
        if (!s || i < 0 || (uint64_t)i >= s->header.length) return;
        s->data[i] = '\0';
        return;
    }
    /* OBJECT + PODs: no per-index semantics. */
}

/* =========================================================================
 * 10. ARITHMETIC & COMPARISON
 * ========================================================================= */

CSSC_API CsscVal cssc_add(CsscVal a, CsscVal b) {
    /* String concatenation if either is string */
    if (CSSC_IS_STRING(a) || CSSC_IS_STRING(b)) {
        return cssc_string_concat(a, b);
    }
    /* Integer arithmetic */
    if (CSSC_IS_INT(a) && CSSC_IS_INT(b)) {
        return cssc_int(a.data.i + b.data.i);
    }
    /* Float promotion */
    if (CSSC_IS_NUMERIC(a) && CSSC_IS_NUMERIC(b)) {
        return cssc_float(cssc_to_float(a) + cssc_to_float(b));
    }
    /* Vector concatenation */
    if (CSSC_IS_VECTOR(a) && CSSC_IS_VECTOR(b)) {
        int64_t sz_a = cssc_vector_size(a), sz_b = cssc_vector_size(b);
        CsscVal result = cssc_vector((size_t)(sz_a + sz_b));
        for (int64_t i = 0; i < sz_a; i++) cssc_vector_push(result, cssc_copy(cssc_vector_get(a, i)));
        for (int64_t i = 0; i < sz_b; i++) cssc_vector_push(result, cssc_copy(cssc_vector_get(b, i)));
        return result;
    }
    /* Fallback: string concat */
    return cssc_string_concat(a, b);
}

CSSC_API CsscVal cssc_sub(CsscVal a, CsscVal b) {
    if (CSSC_IS_INT(a) && CSSC_IS_INT(b)) return cssc_int(a.data.i - b.data.i);
    if (CSSC_IS_NUMERIC(a) && CSSC_IS_NUMERIC(b)) return cssc_float(cssc_to_float(a) - cssc_to_float(b));
    return cssc_int(0);
}

CSSC_API CsscVal cssc_mul(CsscVal a, CsscVal b) {
    if (CSSC_IS_INT(a) && CSSC_IS_INT(b)) return cssc_int(a.data.i * b.data.i);
    if (CSSC_IS_NUMERIC(a) && CSSC_IS_NUMERIC(b)) return cssc_float(cssc_to_float(a) * cssc_to_float(b));
    if (CSSC_IS_STRING(a) && CSSC_IS_INT(b)) return cssc_string_repeat(a, b.data.i);
    return cssc_int(0);
}

CSSC_API CsscVal cssc_div(CsscVal a, CsscVal b) {
    double denom = cssc_to_float(b);
    if (denom == 0.0) { cssc_panic("division by zero"); return cssc_int(0); }
    return cssc_float(cssc_to_float(a) / denom);
}

CSSC_API CsscVal cssc_mod(CsscVal a, CsscVal b) {
    if (CSSC_IS_INT(a) && CSSC_IS_INT(b)) {
        if (b.data.i == 0) { cssc_panic("modulo by zero"); return cssc_int(0); }
        return cssc_int(a.data.i % b.data.i);
    }
    double denom = cssc_to_float(b);
    if (denom == 0.0) { cssc_panic("modulo by zero"); return cssc_float(0.0); }
    return cssc_float(fmod(cssc_to_float(a), denom));
}

CSSC_API CsscVal cssc_neg(CsscVal a) {
    if (CSSC_IS_INT(a)) return cssc_int(-a.data.i);
    if (CSSC_IS_FLOAT(a)) return cssc_float(-a.data.f);
    return cssc_int(0);
}

CSSC_API bool cssc_eq(CsscVal a, CsscVal b) {
    CsscTypeTag ta = CSSC_TYPE(a), tb = CSSC_TYPE(b);
    if (ta == CSSC_TYPE_NULL && tb == CSSC_TYPE_NULL) return true;
    if (ta == CSSC_TYPE_NULL || tb == CSSC_TYPE_NULL) return false;
    if (ta == CSSC_TYPE_INT && tb == CSSC_TYPE_INT) return a.data.i == b.data.i;
    if (ta == CSSC_TYPE_FLOAT || tb == CSSC_TYPE_FLOAT) return cssc_to_float(a) == cssc_to_float(b);
    if (ta == CSSC_TYPE_BOOL && tb == CSSC_TYPE_BOOL) return a.data.b == b.data.b;
    if (ta == CSSC_TYPE_STRING && tb == CSSC_TYPE_STRING) {
        return strcmp(cssc_to_cstr(a), cssc_to_cstr(b)) == 0;
    }
    /* Cross-type: try numeric */
    if ((CSSC_IS_INT(a) || CSSC_IS_FLOAT(a)) && (CSSC_IS_INT(b) || CSSC_IS_FLOAT(b))) {
        return cssc_to_float(a) == cssc_to_float(b);
    }
    return a.data.raw == b.data.raw; /* pointer equality */
}

CSSC_API bool cssc_ne(CsscVal a, CsscVal b) { return !cssc_eq(a, b); }

CSSC_API bool cssc_lt(CsscVal a, CsscVal b) {
    if (CSSC_IS_INT(a) && CSSC_IS_INT(b)) return a.data.i < b.data.i;
    if (CSSC_IS_NUMERIC(a) && CSSC_IS_NUMERIC(b)) return cssc_to_float(a) < cssc_to_float(b);
    if (CSSC_IS_STRING(a) && CSSC_IS_STRING(b)) return strcmp(cssc_to_cstr(a), cssc_to_cstr(b)) < 0;
    return false;
}

CSSC_API bool cssc_gt(CsscVal a, CsscVal b) {
    if (CSSC_IS_INT(a) && CSSC_IS_INT(b)) return a.data.i > b.data.i;
    if (CSSC_IS_NUMERIC(a) && CSSC_IS_NUMERIC(b)) return cssc_to_float(a) > cssc_to_float(b);
    if (CSSC_IS_STRING(a) && CSSC_IS_STRING(b)) return strcmp(cssc_to_cstr(a), cssc_to_cstr(b)) > 0;
    return false;
}

CSSC_API bool cssc_le(CsscVal a, CsscVal b) { return !cssc_gt(a, b); }
CSSC_API bool cssc_ge(CsscVal a, CsscVal b) { return !cssc_lt(a, b); }

CSSC_API bool cssc_logical_and(CsscVal a, CsscVal b) { return cssc_is_truthy(a) && cssc_is_truthy(b); }
CSSC_API bool cssc_logical_or(CsscVal a, CsscVal b) { return cssc_is_truthy(a) || cssc_is_truthy(b); }
CSSC_API bool cssc_logical_not(CsscVal a) { return !cssc_is_truthy(a); }

/* =========================================================================
 * 11. TYPE COERCION
 * ========================================================================= */

CSSC_API CsscVal cssc_to_string_val(CsscVal v) {
    if (CSSC_IS_STRING(v)) { cssc_retain(v); return v; }
    return cssc_format_value(v);
}

CSSC_API CsscVal cssc_to_int_val(CsscVal v) {
    return cssc_int(cssc_to_int(v));
}

CSSC_API CsscVal cssc_to_float_val(CsscVal v) {
    return cssc_float(cssc_to_float(v));
}

CSSC_API CsscVal cssc_coerce(CsscVal v, CsscTypeTag target) {
    switch (target) {
        case CSSC_TYPE_INT:    return cssc_to_int_val(v);
        case CSSC_TYPE_FLOAT:  return cssc_to_float_val(v);
        case CSSC_TYPE_STRING: return cssc_to_string_val(v);
        case CSSC_TYPE_BOOL:   return cssc_bool(cssc_is_truthy(v));
        default: return v;
    }
}

/* =========================================================================
 * 12. SCOPE STACK
 * ========================================================================= */

#define SCOPE_INITIAL_CAPACITY 32
#define SCOPE_MAX_DEPTH 256

/* Slab size for the per-frame bump arena. 256 B is enough for the typical
 * label-body hot path (one short string + a few small format buffers)
 * without bleeding too much SRAM on ESP8266, where the deepest active
 * stack is usually ≤ 4 frames (≤ 1 KB total slab reserved). Allocations
 * larger than this fall through to malloc and live on arena_head. */
#ifndef CSSC_FRAME_SLAB_SIZE
#define CSSC_FRAME_SLAB_SIZE 256
#endif

static void cssc_frame_init(CsscScopeFrame* frame) {
    /* Skip the alloc if this frame slot already has an entries[] from a
     * previous push/pop cycle — keeping the allocation alive across pops
     * is the single biggest reduction in malloc/free traffic on ESP8266,
     * where umm_malloc fragments badly under ~100 KB/s churn. A label
     * body inside a tight loop pushes/pops its frame ~125x/sec; without
     * this slot-reuse path each iteration burned ~768 B of malloc + free,
     * shredding the heap inside ~30 s. */
    if (!frame->entries) {
        frame->capacity = SCOPE_INITIAL_CAPACITY;
        frame->entries = (CsscScopeEntry*)cssc_alloc(sizeof(CsscScopeEntry) * frame->capacity);
    }
    memset(frame->entries, 0, sizeof(CsscScopeEntry) * frame->capacity);
    frame->count = 0;
    frame->is_private = 0;
    frame->is_borrowed = 0;
    frame->alloc_bits = 0;
    frame->arena_head = NULL;
    /* Slab is allocated lazily on first arena_alloc — saves the alloc
     * for frames that never use the arena (e.g. pure metadata frames
     * from sector_push_members). slab_used is what gets reset every pop
     * to make small temp allocs effectively free. */
    frame->slab_used = 0;
}

/* Reset entries without releasing the entries[] allocation — used by
 * cssc_scope_pop so the slot can be re-used on the next push. Object/
 * sector cleanup uses cssc_frame_destroy below for full teardown. */
static void cssc_frame_reset(CsscScopeFrame* frame) {
    if (!frame->entries) return;
    for (uint32_t i = 0; i < frame->capacity; i++) {
        if (frame->entries[i].occupied) {
            cssc_release(frame->entries[i].value);
            frame->entries[i].occupied = false;
            frame->entries[i].name = NULL;
            frame->entries[i].value = cssc_null();
        }
    }
    frame->count = 0;
    frame->is_private = 0;
    frame->is_borrowed = 0;
    frame->alloc_bits = 0;
}

static void cssc_frame_destroy(CsscScopeFrame* frame) {
    if (frame->entries) {
        for (uint32_t i = 0; i < frame->capacity; i++) {
            if (frame->entries[i].occupied) {
                cssc_release(frame->entries[i].value);
            }
        }
        cssc_free(frame->entries);
        frame->entries = NULL;
        frame->capacity = 0;
        frame->count = 0;
    }
    if (frame->slab) {
        cssc_free(frame->slab);
        frame->slab = NULL;
        frame->slab_size = 0;
        frame->slab_used = 0;
    }
}

static CsscScopeEntry* cssc_frame_find(CsscScopeFrame* frame, const char* name) {
    uint32_t hash = cssc_hash_string(name);
    uint32_t idx = hash & (frame->capacity - 1);
    for (uint32_t i = 0; i < frame->capacity; i++) {
        uint32_t probe = (idx + i) & (frame->capacity - 1);
        CsscScopeEntry* e = &frame->entries[probe];
        if (!e->occupied) return NULL;
        if (e->hash == hash && strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

static void cssc_frame_rehash(CsscScopeFrame* frame) {
    uint32_t old_cap = frame->capacity;
    CsscScopeEntry* old = frame->entries;
    frame->capacity *= 2;
    frame->entries = (CsscScopeEntry*)cssc_alloc(sizeof(CsscScopeEntry) * frame->capacity);
    memset(frame->entries, 0, sizeof(CsscScopeEntry) * frame->capacity);
    for (uint32_t i = 0; i < old_cap; i++) {
        if (old[i].occupied) {
            uint32_t idx = old[i].hash & (frame->capacity - 1);
            for (uint32_t j = 0; j < frame->capacity; j++) {
                uint32_t probe = (idx + j) & (frame->capacity - 1);
                if (!frame->entries[probe].occupied) {
                    frame->entries[probe] = old[i];
                    break;
                }
            }
        }
    }
    cssc_free(old);
}

static void cssc_frame_set(CsscScopeFrame* frame, const char* name, CsscVal value) {
    if (frame->count * 4 >= frame->capacity * 3) {
        cssc_frame_rehash(frame);
    }
    uint32_t hash = cssc_hash_string(name);
    uint32_t idx = hash & (frame->capacity - 1);
    for (uint32_t i = 0; i < frame->capacity; i++) {
        uint32_t probe = (idx + i) & (frame->capacity - 1);
        CsscScopeEntry* e = &frame->entries[probe];
        if (!e->occupied) {
            e->name = cssc_intern(name);
            e->hash = hash;
            cssc_retain(value);
            e->value = value;
            e->occupied = true;
            frame->count++;
            return;
        }
        if (e->hash == hash && strcmp(e->name, name) == 0) {
            cssc_release(e->value);
            cssc_retain(value);
            e->value = value;
            return;
        }
    }
}

CSSC_API void cssc_scope_init(CsscScopeStack* stack) {
    stack->max_depth = SCOPE_MAX_DEPTH;
    stack->frames = (CsscScopeFrame*)cssc_alloc(sizeof(CsscScopeFrame) * stack->max_depth);
    memset(stack->frames, 0, sizeof(CsscScopeFrame) * stack->max_depth);
    stack->depth = 0;
    cssc_frame_init(&stack->frames[0]);
    /* v6 ref-by-default pending-arg-link queue starts empty. */
    stack->pending_link_count = 0;
    for (uint32_t i = 0; i < CSSC_MAX_PENDING_LINKS; i++) {
        stack->pending_links[i].param_name = NULL;
        stack->pending_links[i].caller_slot_name = NULL;
    }
}

CSSC_API void cssc_scope_destroy(CsscScopeStack* stack) {
    /* Walk every slot, not just up to `depth`. With frame-slot reuse,
     * popped slots keep their entries[] array alive across pop/push
     * cycles, so a slot beyond the current depth may still own a
     * heap-allocated entry table that needs releasing at shutdown. */
    for (uint32_t i = 0; i < stack->max_depth; i++) {
        if (stack->frames[i].entries) {
            cssc_frame_arena_reset(&stack->frames[i]);
            cssc_frame_destroy(&stack->frames[i]);
        }
    }
    cssc_free(stack->frames);
    stack->frames = NULL;
}

/* Consume any pending arg-links (v6 ref-by-default) into the new
 * top frame as alias entries. Called at the tail of every public
 * cssc_scope_push variant. Idempotent — safe to call when no links
 * are pending (no-op fast path). */
static void cssc_scope_consume_pending_links(CsscScopeStack* stack) {
    uint32_t n = stack->pending_link_count;
    if (n == 0) return;
    for (uint32_t i = 0; i < n; i++) {
        const char* p = stack->pending_links[i].param_name;
        const char* t = stack->pending_links[i].caller_slot_name;
        if (p && t) {
            /* Materialise the alias in the just-pushed top frame.
             * Reuses cssc_scope_alias, which writes to top. */
            cssc_scope_alias(stack, p, t);
        }
        stack->pending_links[i].param_name = NULL;
        stack->pending_links[i].caller_slot_name = NULL;
    }
    stack->pending_link_count = 0;
}

CSSC_API void cssc_scope_push(CsscScopeStack* stack) {
    if (stack->depth + 1 >= stack->max_depth) {
        cssc_panic("scope stack overflow (too many nested sectors/objects)");
    }
    stack->depth++;
    cssc_frame_init(&stack->frames[stack->depth]);
    stack->frames[stack->depth].is_private = 0;
    cssc_scope_consume_pending_links(stack);
}

CSSC_API void cssc_scope_push_private(CsscScopeStack* stack) {
    if (stack->depth + 1 >= stack->max_depth) {
        cssc_panic("scope stack overflow (too many nested sectors/objects)");
    }
    stack->depth++;
    cssc_frame_init(&stack->frames[stack->depth]);
    stack->frames[stack->depth].is_private = 1;
    cssc_scope_consume_pending_links(stack);
}

CSSC_API void cssc_arg_link(CsscScopeStack* stack,
                             const char* param_name,
                             const char* caller_slot_name) {
    if (!stack || !param_name || !caller_slot_name) return;
    /* Over-cap entries silently dropped — see CSSC_MAX_PENDING_LINKS
     * note in the header. Hitting this means a CSSC function was
     * called with >8 ref-args; raise the cap if real user code
     * trips it. */
    if (stack->pending_link_count >= CSSC_MAX_PENDING_LINKS) return;
    uint32_t i = stack->pending_link_count++;
    stack->pending_links[i].param_name = cssc_intern(param_name);
    stack->pending_links[i].caller_slot_name = cssc_intern(caller_slot_name);
}

CSSC_API void cssc_scope_pop(CsscScopeStack* stack) {
    if (stack->depth == 0) return;
    /* Bulk-free the frame's transient allocation arena BEFORE clearing
     * the entry table — every cssc_string_concat / cssc_format_value /
     * other "throwaway" temp built during this frame's lifetime is
     * linked there and gets reaped now. This is the difference between
     * a script that fits in 10KB heap and one that OOM's after 325
     * iterations of a string-concat hot loop. */
    cssc_frame_arena_reset(&stack->frames[stack->depth]);
    if (!stack->frames[stack->depth].is_borrowed) {
        /* Reset, don't destroy — keep entries[] AND slab allocated for
         * the next push into this slot. Frame slot reuse is the single
         * biggest relief on ESP8266 umm_malloc fragmentation in tight
         * label loops (see cssc_frame_init's note). Full teardown
         * happens once, in cssc_scope_destroy at runtime shutdown. */
        cssc_frame_reset(&stack->frames[stack->depth]);
    } else {
        /* Borrowed frame: the entries[] array belongs to the original
         * owner (don't touch it). But the slab was allocated for THIS
         * borrowed push (see cssc_scope_push_borrowed), so we own it
         * and must free it now — otherwise every sector/object call
         * leaks one slab buffer. */
        if (stack->frames[stack->depth].slab) {
            cssc_free(stack->frames[stack->depth].slab);
        }
        memset(&stack->frames[stack->depth], 0, sizeof(CsscScopeFrame));
    }
    stack->depth--;
}

/* Arena: two-tier allocator.
 *
 * Tier 1 (slab): a fixed-size bump buffer per frame slot. Small allocs
 * are aligned to 8 bytes and served from the slab via a bump pointer.
 * On scope_pop the bump pointer is reset to 0 — zero malloc/free in
 * the common case. The slab itself is allocated once on first use and
 * lives until the frame slot is fully destroyed (runtime shutdown).
 *
 * Tier 2 (linked list): allocations that don't fit the slab fall
 * through to malloc and prepend onto arena_head. Reset walks the chain
 * freeing each. This keeps correctness for big strings/vectors while
 * leaving the hot path malloc-free.
 *
 * Layout of malloc'd nodes: [next-ptr (sizeof(void*))] [user payload]. */
CSSC_API void* cssc_frame_arena_alloc(CsscScopeStack* stack, size_t size) {
    if (!stack || stack->depth >= stack->max_depth) return NULL;
    CsscScopeFrame* frame = &stack->frames[stack->depth];

    /* Align user payload to 8 bytes — safe for any CSSC struct. */
    size_t aligned = (size + 7u) & ~(size_t)7u;

    /* Lazy slab alloc: only the slots actually used as arena hosts pay
     * the SRAM cost. Allocation failure is fine — we degrade to the
     * malloc fallback below. */
    if (!frame->slab) {
        frame->slab = (uint8_t*)cssc_alloc(CSSC_FRAME_SLAB_SIZE);
        if (frame->slab) {
            frame->slab_size = CSSC_FRAME_SLAB_SIZE;
            frame->slab_used = 0;
        }
    }

    if (frame->slab && frame->slab_used + aligned <= frame->slab_size) {
        void* p = frame->slab + frame->slab_used;
        frame->slab_used += (uint32_t)aligned;
        return p;
    }

    /* Slab full or oversized request — fall through to malloc. */
    void* block = malloc(sizeof(void*) + aligned);
    if (!block) return NULL;
    *(void**)block = frame->arena_head;
    frame->arena_head = block;
    return (char*)block + sizeof(void*);
}

CSSC_API void cssc_frame_arena_reset(CsscScopeFrame* frame) {
    if (!frame) return;
    /* Tier 1: zero-cost reset — bump pointer back to slab base. */
    frame->slab_used = 0;
    /* Tier 2: free any malloc-overflow blocks. Walk the chain. */
    void* node = frame->arena_head;
    while (node) {
        void* next = *(void**)node;
        free(node);
        node = next;
    }
    frame->arena_head = NULL;
}

CSSC_API void cssc_scope_push_borrowed(CsscScopeStack* stack, CsscScopeFrame* frame) {
    if (stack->depth + 1 >= stack->max_depth) {
        cssc_panic("scope stack overflow (too many nested borrowed pushes)");
    }
    stack->depth++;
    /* The destination slot may still hold an entries[] / slab alloc from
     * a previous non-borrowed push (slot reuse), now about to be
     * overwritten by the byte-copy below. Reclaim them first, otherwise
     * the original allocs would leak — a slow leak under heavy reuse
     * of sectors that mix borrowed and owned pushes in the same slot. */
    if (stack->frames[stack->depth].entries &&
        !stack->frames[stack->depth].is_borrowed) {
        cssc_frame_destroy(&stack->frames[stack->depth]);
    }
    /* Byte-copy the foreign descriptor so cssc_scope_get sees its entries.
     * The original frame owner keeps its entries[] allocation. */
    stack->frames[stack->depth] = *frame;
    stack->frames[stack->depth].is_borrowed = 1;
    /* The arena state (slab pointer + chain) is OWNED by the original
     * frame. The borrowed copy gets its own clean arena — code running
     * under a sector/object call allocates into the copy and the
     * original frame stays untouched. Without this reset we would
     * either double-free the chain on pop or corrupt the original's
     * slab_used counter. */
    stack->frames[stack->depth].slab = NULL;
    stack->frames[stack->depth].slab_size = 0;
    stack->frames[stack->depth].slab_used = 0;
    stack->frames[stack->depth].arena_head = NULL;
}

CSSC_API CsscVal cssc_scope_get(CsscScopeStack* stack, const char* name) {
    /* Walk from inner to outer scope. A frame marked `is_private` acts as a
     * lookup barrier — it's inspected, but we stop *after* it. That way
     * `#req`-copied locals resolve normally, but bare outer names don't leak in.
     *
     * Alias entries (CSSC_FLAG_SCOPE_ALIAS) re-enter the lookup with the
     * target name, but the next search starts BELOW the frame where we
     * found the alias. This handles self-named aliases (`#req[tick_]
     * *tick_;` introducing a local entry called `tick_` that points to
     * the outer `tick_`) — without the skip-down, the alias would
     * match itself every iteration and the lookup would burn its hop
     * budget then return null. Capped at 8 hops to bound malformed
     * alias chains. */
    int hops = 0;
    int32_t start = (int32_t)stack->depth;
    while (hops++ < 8) {
        bool found_alias = false;
        for (int32_t d = start; d >= 0; d--) {
            CsscScopeEntry* e = cssc_frame_find(&stack->frames[d], name);
            if (e) {
                if ((e->value.tag & CSSC_FLAG_SCOPE_ALIAS) && e->value.data.ptr) {
                    name = (const char*)e->value.data.ptr;
                    found_alias = true;
                    start = d - 1;
                    break;
                }
                return e->value;
            }
            if (stack->frames[d].is_private) break;
        }
        if (!found_alias) return cssc_null();
    }
    return cssc_null();
}

CSSC_API void cssc_scope_set(CsscScopeStack* stack, const char* name, CsscVal value) {
    /* Standard COPY semantics: retain the value (we're taking a new
     * reference), release the old occupant. This is correct for the
     * common case where the caller wants both a local C reference
     * AND a scope entry pointing at the same heap object — e.g.
     * `cssc_scope_set(_scope, "x", cssc_scope_get(_scope, "y"))` for
     * generic-param injection across nested labels.
     *
     * For one-shot allocations (`#stack[T] x = cssc_vector(...)`),
     * the codegen pairs this with an explicit `cssc_release(_t)` of
     * the temp AFTER scope_set so the alloc-time refcount=1 doesn't
     * double up into refcount=2 and leak when only one release fires.
     * See `_gen_alloc` in cssc_compiler.py. */
    int hops = 0;
    int32_t start = (int32_t)stack->depth;
    while (hops++ < 8) {
        bool found_alias = false;
        for (int32_t d = start; d >= 0; d--) {
            CsscScopeEntry* e = cssc_frame_find(&stack->frames[d], name);
            if (e) {
                if ((e->value.tag & CSSC_FLAG_SCOPE_ALIAS) && e->value.data.ptr) {
                    name = (const char*)e->value.data.ptr;
                    found_alias = true;
                    start = d - 1;
                    break;
                }
                cssc_release(e->value);
                cssc_retain(value);
                e->value = value;
                return;
            }
            if (stack->frames[d].is_private) break;
        }
        if (!found_alias) {
            int32_t target_depth = (start < 0) ? 0 : (int32_t)stack->depth;
            /* cssc_frame_set retains internally — no extra retain here.
             * (Earlier versions did a double-retain that leaked one
             * ref per cssc_scope_set on a fresh entry — invisible until
             * a tight loop like `#stack[array<auto>] x;` per label
             * iteration filled the heap after ~30 s.) */
            cssc_frame_set(&stack->frames[target_depth], name, value);
            return;
        }
    }
}

CSSC_API void cssc_scope_alias(CsscScopeStack* stack,
                                const char* alias_name, const char* target_name) {
    /* Store an alias entry: a CsscVal whose tag carries CSSC_FLAG_SCOPE_ALIAS
     * and whose data.ptr is the interned target name string. Subsequent
     * cssc_scope_get / cssc_scope_set on `alias_name` will follow the
     * pointer to `target_name`. Target need NOT exist yet — late binding
     * is fine, the lookup just resolves at access time. */
    CsscVal v;
    v.tag = CSSC_FLAG_SCOPE_ALIAS;
    v.data.ptr = (void*)cssc_intern(target_name);
    cssc_frame_set(&stack->frames[stack->depth], alias_name, v);
}

CSSC_API void cssc_scope_walk(CsscScopeStack* stack,
                               CsscScopeWalkFn fn, void* userdata) {
    /* Iterate every occupied entry in every frame, newest frame first.
     * Order matches scope_get's lookup walk (top-of-stack downwards),
     * which is the natural order for tools wanting "most-recent
     * shadowing wins" semantics. Skips alias entries — those are
     * implementation detail, not user-visible variables. */
    if (!stack || !fn) return;
    for (int32_t d = (int32_t)stack->depth; d >= 0; d--) {
        CsscScopeFrame* frame = &stack->frames[d];
        if (!frame->entries) continue;
        for (uint32_t i = 0; i < frame->capacity; i++) {
            CsscScopeEntry* e = &frame->entries[i];
            if (!e->occupied || !e->name) continue;
            /* Hide alias redirect entries — the consumer wants real
             * allocations, not the #req[X] *Y; bookkeeping cells. */
            if (e->value.tag & CSSC_FLAG_SCOPE_ALIAS) continue;
            if (!fn(e->name, e->value, d, frame->is_private != 0, userdata)) {
                return;
            }
        }
    }
}

CSSC_API bool cssc_scope_has(CsscScopeStack* stack, const char* name) {
    for (int32_t d = (int32_t)stack->depth; d >= 0; d--) {
        if (cssc_frame_find(&stack->frames[d], name)) return true;
        if (stack->frames[d].is_private) break;
    }
    return false;
}

CSSC_API void cssc_scope_delete(CsscScopeStack* stack, const char* name) {
    for (int32_t d = (int32_t)stack->depth; d >= 0; d--) {
        CsscScopeEntry* e = cssc_frame_find(&stack->frames[d], name);
        if (e) {
            cssc_release(e->value);
            e->occupied = false;
            e->name = NULL;
            stack->frames[d].count--;
            return;
        }
        if (stack->frames[d].is_private) break;
    }
}

CSSC_API void cssc_scope_delete_aliased(CsscScopeStack* stack,
                                          const char* name) {
    /* v6 ref-by-default cross-frame delete (spec §2.5).
     *
     * Step 1: look up `name` in the TOP frame only — alias entries
     * are always rooted at the callee's frame (installed by
     * cssc_scope_push from the pending_links queue). If it's an
     * alias, resolve to the target name and delete the actual slot
     * in a parent frame below. Then drop the local alias entry.
     *
     * Step 2: when `name` isn't an alias (literal/expression arg,
     * or the body's own `#stack` slot), fall through to plain
     * `cssc_scope_delete`. */
    if (!stack || !name) return;
    CsscScopeEntry* top = cssc_frame_find(&stack->frames[stack->depth], name);
    if (top && (top->value.tag & CSSC_FLAG_SCOPE_ALIAS)) {
        const char* target = (const char*)top->value.data.ptr;
        /* Walk frames BELOW the top to find and delete the source.
         * Stop at the first private barrier — caller's slot must be
         * reachable through normal scope-walk rules. */
        if (target) {
            for (int32_t d = (int32_t)stack->depth - 1; d >= 0; d--) {
                CsscScopeEntry* te = cssc_frame_find(&stack->frames[d], target);
                if (te) {
                    cssc_release(te->value);
                    te->occupied = false;
                    te->name = NULL;
                    stack->frames[d].count--;
                    break;
                }
                if (stack->frames[d].is_private) break;
            }
        }
        /* Drop the local alias entry. The alias value carries no
         * heap payload (it's just a pointer to an interned name),
         * so no cssc_release is needed. */
        top->occupied = false;
        top->name = NULL;
        stack->frames[stack->depth].count--;
        return;
    }
    /* Not an alias — plain delete. */
    cssc_scope_delete(stack, name);
}

CSSC_API CsscVal* cssc_scope_get_ptr(CsscScopeStack* stack, const char* name) {
    for (int32_t d = (int32_t)stack->depth; d >= 0; d--) {
        CsscScopeEntry* e = cssc_frame_find(&stack->frames[d], name);
        if (e) return &e->value;
        if (stack->frames[d].is_private) break;
    }
    return NULL;
}

CSSC_API void cssc_scope_req(CsscScopeStack* stack,
                              const char* outer_name, const char* local_name) {
    /* Walk past the current private frame to find `outer_name` in an enclosing
     * scope, then copy it into the current (private) frame under `local_name`. */
    int32_t d = (int32_t)stack->depth;
    /* Skip the current frame entirely — #req looks outside the private box. */
    for (int32_t s = d - 1; s >= 0; s--) {
        CsscScopeEntry* e = cssc_frame_find(&stack->frames[s], outer_name);
        if (e) {
            cssc_frame_set(&stack->frames[d], local_name, e->value);
            return;
        }
        /* A second private barrier stops the walk too. */
        if (stack->frames[s].is_private) break;
    }
    /* Not found — store a null so the local exists but is empty. */
    cssc_frame_set(&stack->frames[d], local_name, cssc_null());
}

/* =========================================================================
 * 13. SECTOR & OBJECT (stubs — filled by codegen per-script)
 * ========================================================================= */

CSSC_API CsscVal cssc_sector_create(const char* name) {
    CsscSector* sec = (CsscSector*)cssc_alloc(sizeof(CsscSector));
    memset(sec, 0, sizeof(CsscSector));
    sec->header.refcount = 1;
    sec->header.type = CSSC_TYPE_SECTOR;
    sec->name = cssc_intern(name);
    cssc_frame_init(&sec->members);
    CsscVal v;
    v.tag = CSSC_TYPE_SECTOR;
    v.data.ptr = sec;
    return v;
}

CSSC_API CsscVal cssc_sector_get(CsscVal sector, const char* member) {
    if (!sector.data.ptr) return cssc_null();
    CsscScopeFrame* frame = NULL;
    if (CSSC_TYPE(sector) == CSSC_TYPE_SECTOR) {
        frame = &((CsscSector*)sector.data.ptr)->members;
    } else if (CSSC_TYPE(sector) == CSSC_TYPE_OBJECT) {
        frame = &((CsscObject*)sector.data.ptr)->members;
    } else {
        return cssc_null();
    }
    CsscScopeEntry* e = cssc_frame_find(frame, member);
    return e ? e->value : cssc_null();
}

CSSC_API void cssc_sector_set(CsscVal sector, const char* member, CsscVal value) {
    if (!sector.data.ptr) return;
    CsscScopeFrame* frame = NULL;
    if (CSSC_TYPE(sector) == CSSC_TYPE_SECTOR) {
        frame = &((CsscSector*)sector.data.ptr)->members;
    } else if (CSSC_TYPE(sector) == CSSC_TYPE_OBJECT) {
        frame = &((CsscObject*)sector.data.ptr)->members;
    } else {
        return;
    }
    cssc_frame_set(frame, member, value);
}

CSSC_API bool cssc_sector_is_public(CsscVal sector, const char* member) {
    /* TODO: implement public mask check */
    (void)sector; (void)member;
    return true;
}

CSSC_API void cssc_sector_free(CsscVal sector) {
    if (!sector.data.ptr) return;
    CsscSector* sec = (CsscSector*)sector.data.ptr;
    if (sec->freed) return;
    sec->freed = true;
    cssc_frame_destroy(&sec->members);
}

CSSC_API void cssc_sector_push_members(CsscScopeStack* stack, CsscVal sector) {
    /* Push all sector/object members onto the scope stack as a new frame.
     * Used when calling sector::func() so the function can access sector vars. */
    CsscScopeFrame* frame = NULL;
    if (CSSC_TYPE(sector) == CSSC_TYPE_SECTOR && sector.data.ptr) {
        frame = &((CsscSector*)sector.data.ptr)->members;
    } else if (CSSC_TYPE(sector) == CSSC_TYPE_OBJECT && sector.data.ptr) {
        frame = &((CsscObject*)sector.data.ptr)->members;
    }
    cssc_scope_push(stack);
    if (!frame) return;
    for (uint32_t i = 0; i < frame->capacity; i++) {
        if (frame->entries[i].occupied) {
            cssc_scope_set(stack, frame->entries[i].name, frame->entries[i].value);
        }
    }
}

CSSC_API CsscVal cssc_object_create(const char* name, CsscLabel* labels, uint32_t label_count,
                                      void* top_level_fn, void* free_fn) {
    CsscObject* obj = (CsscObject*)cssc_alloc(sizeof(CsscObject));
    memset(obj, 0, sizeof(CsscObject));
    obj->header.refcount = 1;
    obj->header.type = CSSC_TYPE_OBJECT;
    obj->name = cssc_intern(name);
    obj->labels = labels;
    obj->label_count = label_count;
    obj->top_level_fn = top_level_fn;
    obj->free_fn = free_fn;
    cssc_frame_init(&obj->members);
    CsscVal v;
    v.tag = CSSC_TYPE_OBJECT;
    v.data.ptr = obj;
    return v;
}

CSSC_API CsscVal cssc_object_execute(CsscVal obj_val, CsscVal* args, uint32_t nargs) {
    if (!obj_val.data.ptr) return cssc_null();
    CsscObject* obj = (CsscObject*)obj_val.data.ptr;
    if (obj->top_level_fn) {
        typedef CsscVal (*TopLevelFn)(CsscObject*, CsscVal*, uint32_t);
        TopLevelFn fn = (TopLevelFn)obj->top_level_fn;
        fn(obj, args, nargs);
    }
    obj->executed = true;
    return obj_val;
}

CSSC_API CsscVal cssc_object_call_label(CsscVal obj_val, const char* label_name,
                                          CsscVal* args, uint32_t nargs) {
    if (!obj_val.data.ptr) return cssc_null();
    CsscObject* obj = (CsscObject*)obj_val.data.ptr;
    for (uint32_t i = 0; i < obj->label_count; i++) {
        if (strcmp(obj->labels[i].name, label_name) == 0) {
            typedef CsscVal (*LabelFn)(CsscObject*, CsscVal*, uint32_t);
            LabelFn fn = (LabelFn)obj->labels[i].body_fn;
            if (fn) return fn(obj, args, nargs);
        }
    }
    return cssc_null();
}

CSSC_API void cssc_object_free(CsscVal obj_val) {
    if (!obj_val.data.ptr) return;
    CsscObject* obj = (CsscObject*)obj_val.data.ptr;
    if (obj->freed) return;
    obj->freed = true;
    if (obj->free_fn) {
        typedef void (*FreeFn)(CsscObject*);
        FreeFn fn = (FreeFn)obj->free_fn;
        fn(obj);
    }
    cssc_frame_destroy(&obj->members);
}

CSSC_API CsscVal cssc_function_create(const char* name, void* body_fn, uint32_t max_bits) {
    CsscFunction* func = (CsscFunction*)cssc_alloc(sizeof(CsscFunction));
    memset(func, 0, sizeof(CsscFunction));
    func->header.refcount = 1;
    func->header.type = CSSC_TYPE_FUNCTION;
    func->name = cssc_intern(name);
    func->body_fn = body_fn;
    func->max_bits = max_bits;
    func->last_return = cssc_null();
    CsscVal v;
    v.tag = CSSC_TYPE_FUNCTION;
    v.data.ptr = func;
    return v;
}

CSSC_API CsscVal cssc_function_call(CsscVal func_val, CsscVal* args, uint32_t nargs) {
    if (CSSC_TYPE(func_val) != CSSC_TYPE_FUNCTION || !func_val.data.ptr) return cssc_null();
    CsscFunction* func = (CsscFunction*)func_val.data.ptr;
    func->call_count++;
    if (func->body_fn) {
        typedef CsscVal (*BodyFn)(CsscScopeStack*, CsscVal*, uint32_t);
        BodyFn fn = (BodyFn)func->body_fn;
        CsscVal result = fn(cssc_global_scope(), args, nargs);
        func->last_return = result;
        return result;
    }
    return cssc_null();
}

/* =========================================================================
 * 14. BUILTIN FUNCTIONS (cssc:: namespace)
 * ========================================================================= */

/* I/O */
CSSC_API void cssc_builtin_out(CsscVal v) {
    CsscVal s = cssc_to_string_val(v);
    printf("%s", cssc_to_cstr(s));
    fflush(stdout);
    cssc_release(s);
}

CSSC_API void cssc_builtin_outln(CsscVal v) {
    CsscVal s = cssc_to_string_val(v);
    printf("%s\n", cssc_to_cstr(s));
    fflush(stdout);
    cssc_release(s);
}

CSSC_API CsscVal cssc_builtin_input(CsscVal prompt) {
    if (!CSSC_IS_NULL(prompt)) {
        cssc_builtin_out(prompt);
    }
    char buf[4096];
    buf[0] = '\0';
    if (fgets(buf, sizeof(buf), stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
        if (len > 0 && buf[len - 1] == '\r') buf[--len] = '\0';
        /* Try to parse as int if it looks numeric. avr-libc only
         * exposes strtol (long, 32-bit on AVR) — there's no strtoll
         * for 64-bit. Use strtol there; on AVR cssc_int still fits
         * because the chip's CsscVal stays in 32-bit ints anyway. */
        char* end;
      #if defined(__AVR__)
        long ival = strtol(buf, &end, 10);
      #else
        long long ival = strtoll(buf, &end, 10);
      #endif
        if (*end == '\0' && buf[0] != '\0') return cssc_int((int64_t)ival);
        return cssc_string(buf);
    }
    return cssc_string("");
}

CSSC_API void cssc_builtin_sleep(double ms) {
    /* Argument is milliseconds — matches the interpreter's cssc::sleep(ms). */
#if defined(_WIN32)
    Sleep((DWORD)ms);
#elif defined(CSSC_EMBEDDED)
    /* Real sleep on embedded: forward to Arduino-core delay() which
     * yields to the watchdog + Wi-Fi background tasks. NEVER no-op —
     * that would brick scripts (their loops would tight-spin and
     * the WDT would reset the chip). When delay() isn't available
     * (no Arduino.h on this target) we busy-wait via micros() so
     * timing semantics still hold; that's still real, not silent. */
  #if defined(CSSC_EMB_HAS_ARDUINO)
    delay((unsigned long)ms);
    /* Hook into the user's main sleep — `cssc_diag_tick()` self-
     * rate-limits to 1 Hz so a hot loop of `cssc::sleep(8)` triggers
     * at most one [cssc-mem] emission per second. Without this, when
     * `_cssc_main()` contains an infinite loop (the common embedded
     * pattern), Arduino-framework `loop()` never runs and the
     * diagnostics listener sees nothing. Empty stub in non-diag
     * builds — zero overhead in production firmware. */
    cssc_diag_tick();
  #else
    /* Bare-metal fallback: busy-wait in chunks of 1ms via the
     * platform's own millisecond counter, with a yield every loop.
     * Slower than delay() but never silent. */
    extern unsigned long millis(void);
    extern void yield(void);
    unsigned long start = millis();
    unsigned long target = (unsigned long)ms;
    while ((millis() - start) < target) {
        yield();
    }
    cssc_diag_tick();
  #endif
#else
    double seconds = ms / 1000.0;
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - (double)ts.tv_sec) * 1e9);
    nanosleep(&ts, NULL);
#endif
}

/* Type */
CSSC_API CsscVal cssc_builtin_typeof(CsscVal v) { return cssc_string(cssc_typeof_str(v)); }
CSSC_API CsscVal cssc_builtin_to_int(CsscVal v) { return cssc_to_int_val(v); }
CSSC_API CsscVal cssc_builtin_to_float(CsscVal v) { return cssc_to_float_val(v); }
CSSC_API CsscVal cssc_builtin_to_string(CsscVal v) { return cssc_to_string_val(v); }
CSSC_API CsscVal cssc_builtin_to_bool(CsscVal v) { return cssc_bool(cssc_is_truthy(v)); }
CSSC_API CsscVal cssc_builtin_is_null(CsscVal v) { return cssc_bool(CSSC_IS_NULL(v)); }
CSSC_API CsscVal cssc_builtin_is_int(CsscVal v) { return cssc_bool(CSSC_IS_INT(v)); }
CSSC_API CsscVal cssc_builtin_is_float(CsscVal v) { return cssc_bool(CSSC_IS_FLOAT(v)); }
CSSC_API CsscVal cssc_builtin_is_string(CsscVal v) { return cssc_bool(CSSC_IS_STRING(v)); }
CSSC_API CsscVal cssc_builtin_is_array(CsscVal v) { return cssc_bool(CSSC_IS_VECTOR(v)); }

/* Math */
CSSC_API CsscVal cssc_builtin_abs(CsscVal v) {
    if (CSSC_IS_INT(v)) return cssc_int(v.data.i < 0 ? -v.data.i : v.data.i);
    if (CSSC_IS_FLOAT(v)) return cssc_float(fabs(v.data.f));
    return cssc_int(0);
}

CSSC_API CsscVal cssc_builtin_min(CsscVal a, CsscVal b) { return cssc_lt(a, b) ? a : b; }
CSSC_API CsscVal cssc_builtin_max(CsscVal a, CsscVal b) { return cssc_gt(a, b) ? a : b; }
CSSC_API CsscVal cssc_builtin_sqrt(CsscVal v) { return cssc_float(sqrt(cssc_to_float(v))); }
CSSC_API CsscVal cssc_builtin_pow(CsscVal base, CsscVal exp) { return cssc_float(pow(cssc_to_float(base), cssc_to_float(exp))); }
CSSC_API CsscVal cssc_builtin_floor(CsscVal v) { return cssc_int((int64_t)floor(cssc_to_float(v))); }
CSSC_API CsscVal cssc_builtin_ceil(CsscVal v) { return cssc_int((int64_t)ceil(cssc_to_float(v))); }
CSSC_API CsscVal cssc_builtin_round(CsscVal v) { return cssc_int((int64_t)round(cssc_to_float(v))); }

CSSC_API CsscVal cssc_builtin_random(void) {
    return cssc_float((double)rand() / (double)RAND_MAX);
}

CSSC_API CsscVal cssc_builtin_random_int(CsscVal a, CsscVal b) {
    int64_t lo = cssc_to_int(a), hi = cssc_to_int(b);
    if (lo > hi) { int64_t t = lo; lo = hi; hi = t; }
    return cssc_int(lo + (rand() % (hi - lo + 1)));
}

CSSC_API CsscVal cssc_builtin_clamp(CsscVal v, CsscVal lo, CsscVal hi) {
    if (cssc_lt(v, lo)) return lo;
    if (cssc_gt(v, hi)) return hi;
    return v;
}

/* Array/Collection */
CSSC_API CsscVal cssc_builtin_len(CsscVal v) {
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_STRING: return cssc_int((int64_t)cssc_strlen(v));
        case CSSC_TYPE_VECTOR: return cssc_int(cssc_vector_size(v));
        case CSSC_TYPE_MAP:    return cssc_int(cssc_map_size(v));
        case CSSC_TYPE_BIND:   return cssc_int(cssc_bind_size(v));
        default: return cssc_int(0);
    }
}

CSSC_API CsscVal cssc_builtin_push(CsscVal arr, CsscVal item) {
    cssc_vector_push(arr, item);
    return arr;
}

CSSC_API CsscVal cssc_builtin_pop(CsscVal arr) {
    return cssc_vector_pop(arr);
}

CSSC_API CsscVal cssc_builtin_sort(CsscVal arr) {
    return cssc_vector_sort(arr);
}

CSSC_API CsscVal cssc_builtin_range(CsscVal start, CsscVal end, CsscVal step) {
    int64_t s = cssc_to_int(start), e = cssc_to_int(end);
    int64_t st = CSSC_IS_NULL(step) ? 1 : cssc_to_int(step);
    if (st == 0) return cssc_vector(0);
    int64_t count = (st > 0) ? ((e - s + st - 1) / st) : ((s - e - st - 1) / (-st));
    if (count < 0) count = 0;
    CsscVal result = cssc_vector((size_t)count);
    if (st > 0) {
        for (int64_t i = s; i < e; i += st) cssc_vector_push(result, cssc_int(i));
    } else {
        for (int64_t i = s; i > e; i += st) cssc_vector_push(result, cssc_int(i));
    }
    return result;
}

/* String builtins */
CSSC_API CsscVal cssc_builtin_strlen_val(CsscVal s) { return cssc_int((int64_t)cssc_strlen(s)); }
CSSC_API CsscVal cssc_builtin_substr(CsscVal s, CsscVal start, CsscVal len) { return cssc_string_substr(s, cssc_to_int(start), cssc_to_int(len)); }
CSSC_API CsscVal cssc_builtin_replace(CsscVal s, CsscVal old_s, CsscVal new_s) { return cssc_string_replace(s, old_s, new_s); }
CSSC_API CsscVal cssc_builtin_split(CsscVal s, CsscVal sep) { return cssc_string_split(s, sep); }

CSSC_API CsscVal cssc_builtin_join(CsscVal arr, CsscVal sep) {
    int64_t sz = cssc_vector_size(arr);
    if (sz == 0) return cssc_string("");
    const char* sep_str = cssc_to_cstr(sep);
    size_t sep_len = strlen(sep_str);
    /* Calculate total length */
    size_t total = 0;
    for (int64_t i = 0; i < sz; i++) {
        CsscVal sv = cssc_to_string_val(cssc_vector_get(arr, i));
        total += cssc_strlen(sv);
        cssc_release(sv);
        if (i < sz - 1) total += sep_len;
    }
    char* buf = (char*)cssc_alloc(total + 1);
    char* p = buf;
    for (int64_t i = 0; i < sz; i++) {
        CsscVal sv = cssc_to_string_val(cssc_vector_get(arr, i));
        const char* cs = cssc_to_cstr(sv);
        size_t l = strlen(cs);
        memcpy(p, cs, l);
        p += l;
        cssc_release(sv);
        if (i < sz - 1) { memcpy(p, sep_str, sep_len); p += sep_len; }
    }
    *p = '\0';
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_builtin_trim(CsscVal s) { return cssc_string_trim(s); }
CSSC_API CsscVal cssc_builtin_upper(CsscVal s) { return cssc_string_upper(s); }
CSSC_API CsscVal cssc_builtin_lower(CsscVal s) { return cssc_string_lower(s); }
CSSC_API CsscVal cssc_builtin_contains(CsscVal s, CsscVal sub) { return cssc_bool(cssc_string_contains(s, sub)); }

/* Time
 * ─────
 * Two distinct concepts now expressed in the API:
 *
 *   cssc::uptime()       → seconds since program start (monotonic).
 *                          Always works, never zero unless called in
 *                          the very first tick. Use this for delays,
 *                          benchmarks, periodic triggers.
 *
 *   cssc::time/timestamp/date/datetime/detime/sdetime
 *                        → WALL-CLOCK values from the system. On
 *                          embedded without an RTC + NTP sync these
 *                          report 1970-01-01 00:00 (epoch start)
 *                          honestly — that's the device's view of
 *                          "what time is it" until you sync. After
 *                          a future cssc::ntp_sync via the http
 *                          module they will return real local time.
 *
 * Critical NULL guards on every localtime() call — newlib on ESP8266
 * happily returns NULL for negative/sentinel time_t and the previous
 * `tm->tm_hour` deref crashed the chip with Exception 28.
 */

/* Monotonic seconds since program start. Cross-platform. */
CSSC_API CsscVal cssc_builtin_uptime(void) {
#if defined(CSSC_EMBEDDED) && defined(CSSC_EMB_HAS_ARDUINO)
    return cssc_float((double)millis() / 1000.0);
#elif defined(_WIN32)
    return cssc_float((double)GetTickCount() / 1000.0);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return cssc_float((double)ts.tv_sec + (double)ts.tv_nsec / 1e9);
    }
    return cssc_float((double)clock() / CLOCKS_PER_SEC);
#endif
}

/* Sentinel: any `time_t` smaller than this is treated as "RTC not
 * synced yet" and the wall-clock builtins return zero values rather
 * than leaking uptime as a fake clock. ~Nov 2023 in epoch seconds —
 * any real ntp/RTC value will exceed this; Arduino-core's
 * uptime-as-epoch fallback never will until 70 days of uptime, well
 * past any reasonable embedded run. */
#define CSSC_WALLCLOCK_SENTINEL  ((time_t)1700000000)

static inline int _cssc_has_wallclock(time_t t) {
    return t > CSSC_WALLCLOCK_SENTINEL;
}

CSSC_API CsscVal cssc_builtin_time(void) {
    /* Wall-clock seconds since epoch as float. Without RTC sync we
     * return null (0x0) — explicitly "no wall-clock available",
     * never 0.0 which would be ambiguous with epoch / midnight.
     * Use cssc::uptime() for monotonic time, cssc::ntp_sync() to
     * obtain real wall-clock. */
    time_t t = time(NULL);
    if (!_cssc_has_wallclock(t)) return cssc_int(0);   /* 0x0 = "no wall-clock yet" sentinel */
    return cssc_float((double)t);
}

CSSC_API CsscVal cssc_builtin_timestamp(void) {
    time_t t = time(NULL);
    if (!_cssc_has_wallclock(t)) return cssc_int(0);   /* 0x0 = "no wall-clock yet" sentinel */
    return cssc_int((int64_t)t);
}

CSSC_API CsscVal cssc_builtin_date(void) {
    time_t t = time(NULL);
    if (!_cssc_has_wallclock(t)) return cssc_int(0);   /* 0x0 = "no wall-clock yet" sentinel */
    struct tm* tm = localtime(&t);
    if (!tm) return cssc_int(0);   /* 0x0 = "localtime failed" sentinel */
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return cssc_string(buf);
}

CSSC_API CsscVal cssc_builtin_datetime(void) {
    time_t t = time(NULL);
    if (!_cssc_has_wallclock(t)) return cssc_int(0);   /* 0x0 = "no wall-clock yet" sentinel */
    struct tm* tm = localtime(&t);
    if (!tm) return cssc_int(0);   /* 0x0 = "localtime failed" sentinel */
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return cssc_string(buf);
}

CSSC_API CsscVal cssc_builtin_detime(void) {
    /* Decimal hour-of-day: 4.18 = 04:18 wall time. Without RTC
     * sync returns null (0x0). NEVER returns uptime — for that
     * use cssc::uptime(). */
    time_t t = time(NULL);
    if (!_cssc_has_wallclock(t)) return cssc_int(0);   /* 0x0 = "no wall-clock yet" sentinel */
    struct tm* tm = localtime(&t);
    if (!tm) return cssc_int(0);   /* 0x0 = "localtime failed" sentinel */
    return cssc_float((double)tm->tm_hour + (double)tm->tm_min / 100.0);
}

CSSC_API CsscVal cssc_builtin_sdetime(void) {
    /* Decimal minute.second of the current hour. Without RTC
     * sync returns null (0x0). */
    time_t t = time(NULL);
    if (!_cssc_has_wallclock(t)) return cssc_int(0);   /* 0x0 = "no wall-clock yet" sentinel */
    struct tm* tm = localtime(&t);
    if (!tm) return cssc_int(0);   /* 0x0 = "localtime failed" sentinel */
    return cssc_float((double)tm->tm_min + (double)tm->tm_sec / 100.0);
}

/* System */
CSSC_API CsscVal cssc_builtin_env(CsscVal name) {
    const char* val = getenv(cssc_to_cstr(name));
    return val ? cssc_string(val) : cssc_string("");
}

CSSC_API CsscVal cssc_builtin_cwd(void) {
    char buf[4096];
    if (cssc_getcwd(buf, sizeof(buf))) return cssc_string(buf);
    return cssc_string("");
}

CSSC_API CsscVal cssc_builtin_platform(void) {
#ifdef _WIN32
    return cssc_string("win32");
#elif __linux__
    return cssc_string("linux");
#elif __APPLE__
    return cssc_string("darwin");
#else
    return cssc_string("unknown");
#endif
}

CSSC_API CsscVal cssc_builtin_exec(CsscVal cmd) {
    const char* command = cssc_to_cstr(cmd);
#if defined(CSSC_EMBEDDED)
    /* No subprocess on bare-metal — fail audibly. Earlier versions
     * returned "" silently which masked bugs (script thinks the
     * command succeeded with empty output). Now we panic so the
     * script must explicitly #catch if it wants to handle the
     * absence. Honest failure beats silent no-op. */
    (void)command;
    cssc_panic("cssc::exec unavailable on embedded targets — there is "
               "no subprocess host. Use a target-specific RPC (Serial, "
               "I2C controller, MQTT) instead, or guard with #catch.");
    return cssc_null();
#else
    char buf[65536];
    buf[0] = '\0';
  #ifdef _WIN32
    FILE* fp = _popen(command, "r");
  #else
    FILE* fp = popen(command, "r");
  #endif
    if (fp) {
        size_t total = 0;
        while (fgets(buf + total, (int)(sizeof(buf) - total), fp)) {
            total += strlen(buf + total);
        }
  #ifdef _WIN32
        _pclose(fp);
  #else
        pclose(fp);
  #endif
        return cssc_string(buf);
    }
    return cssc_string("");
#endif
}

CSSC_API void cssc_builtin_exit(CsscVal code) {
    exit((int)cssc_to_int(code));
}

/* Hard reset of the chip / process. On embedded ESP8266 / ESP32 this
 * triggers a full soft-reboot via ESP.restart() — clean way to
 * recover from a stuck WiFi state, sensor lockup, or
 * memory-fragmented session. On desktop falls through to exit().
 * Returns null to satisfy the builtin dispatch ABI even though it
 * never actually returns. */
CSSC_API CsscVal cssc_builtin_reboot(void) {
#if defined(CSSC_EMBEDDED) && defined(CSSC_EMB_HAS_ARDUINO)
    /* Flush serial so any final diagnostic message lands on the
     * host before the reboot wipes the UART buffer. */
    Serial.flush();
    delay(50);
  #if defined(ESP8266) || defined(ESP32)
    /* ESP.* lives in the ESP-Arduino core only. */
    ESP.restart();
    /* ESP.restart() doesn't return; loop here defensively in case
     * the SDK queues the reset asynchronously. */
    for (;;) delay(1000);
  #elif defined(__AVR__)
    /* AVR reboot via watchdog: enable WDT with shortest timeout, then
     * spin until it fires. <avr/wdt.h> is included at the top of this
     * file under the same __AVR__ gate. */
    wdt_enable(WDTO_15MS);
    for (;;) {}
  #else
    /* Unknown Arduino flavour — fall back to a no-op loop so the
     * call doesn't link-fail. */
    for (;;) delay(1000);
  #endif
#else
    exit(0);
#endif
    return cssc_null();
}

/* =========================================================================
 * 13b. DIAGNOSTICS MARKERS — emit [cssc-mem] / [cssc-cpu] lines for the
 *      `cssc diagnostics --port` chip listener.
 *
 * Gated by -DCSSC_DIAG=1 (added by `cssc diagnostics`, not by `cssc build`).
 * Without the macro, all functions are empty bodies — no printf, no heap
 * probe, no static state. Production firmware pays zero overhead.
 * ========================================================================= */

#ifdef CSSC_DIAG
static int      _cssc_diag_enabled = 1;
static uint32_t _cssc_diag_min_free = 0xFFFFFFFFu;  /* lowest free-heap seen */
static uint32_t _cssc_diag_last_tick_ms = 0;        /* throttle for tick() */
#ifndef CSSC_DIAG_TICK_MIN_MS
#define CSSC_DIAG_TICK_MIN_MS 1000   /* don't tick more than 1Hz */
#endif

/* Platform-specific free-heap accessor. Returns bytes of largest contiguous
 * free chunk on embedded (the metric that actually predicts when the next
 * malloc fails), or generic mallinfo on POSIX. */
static uint32_t _cssc_diag_free_heap(void) {
#if defined(CSSC_EMBEDDED) && defined(CSSC_EMB_HAS_ARDUINO) \
        && (defined(ESP8266) || defined(ESP32))
    /* ESP.* lives in the ESP-Arduino core only; AVR has no equivalent
     * "current free heap" API, so this falls through to the bare-metal
     * branch below where we return 0xFFFFFFFF as "unknown / large".
     */
    return (uint32_t)ESP.getFreeHeap();
#elif defined(CSSC_EMBEDDED) && defined(__AVR__)
    /* AVR free-RAM estimate: SP - &__heap_start. avr-libc exposes
     * `__brkval` / `__heap_start`; for the diagnostic this is enough
     * to detect a near-OOM trend without per-allocator accounting. */
    extern char __heap_start;
    extern char *__brkval;
    int v;
    return (uint32_t)((int)((void*)&v - (__brkval == 0
                                          ? (void*)&__heap_start
                                          : (void*)__brkval)));
#elif defined(_WIN32)
    /* Win32: GlobalMemoryStatus reports system-wide free, not process
     * heap. Approximate process-heap via _heapwalk would be more accurate
     * but adds CRT deps; system-wide is a reasonable proxy for the
     * "are we OK" diagnostic question. */
    extern void GlobalMemoryStatusEx(void*);  /* fwd decl avoids windows.h dep */
    typedef struct { uint32_t a,b; uint64_t c,d,e,f,g,h; } MS;
    MS ms; ms.a = sizeof(MS);
    GlobalMemoryStatusEx(&ms);
    return (uint32_t)(ms.e > 0xFFFFFFFFull ? 0xFFFFFFFFu : ms.e);
#else
    /* POSIX: best-effort via mallinfo if available. */
  #if defined(__has_include)
    #if __has_include(<malloc.h>)
      #include <malloc.h>
      struct mallinfo mi = mallinfo();
      return (uint32_t)mi.fordblks;
    #endif
  #endif
    return 0;
#endif
}

CSSC_API void cssc_diag_enable(int on) {
    _cssc_diag_enabled = on ? 1 : 0;
    if (on) {
        /* Re-arm the min tracker so a fresh diagnostics session doesn't
         * inherit stale data from a previous run inside the same process. */
        _cssc_diag_min_free = 0xFFFFFFFFu;
    }
}

CSSC_API int cssc_diag_is_enabled(void) { return _cssc_diag_enabled; }

CSSC_API void cssc_diag_emit_mem(void) {
    if (!_cssc_diag_enabled) return;
    uint32_t free_now = _cssc_diag_free_heap();
    if (free_now < _cssc_diag_min_free) _cssc_diag_min_free = free_now;
    /* %u is safe on every CSSC target. The chip listener regex
     * `[cssc-mem] heap=N free=N` reads only the two integers — we
     * pass the same value twice as "current/min" for parser symmetry. */
    printf("[cssc-mem] heap=%u free=%u\n",
           (unsigned)free_now, (unsigned)_cssc_diag_min_free);
    fflush(stdout);
}

CSSC_API void cssc_diag_emit_cpu(const char* name, uint32_t ms) {
    if (!_cssc_diag_enabled || !name) return;
    printf("[cssc-cpu] %s %u.0ms\n", name, (unsigned)ms);
    fflush(stdout);
}

CSSC_API void cssc_diag_tick(void) {
    if (!_cssc_diag_enabled) return;
    /* Rate-limit to 1Hz so a tight loop() doesn't flood the UART. */
#if defined(CSSC_EMBEDDED) && defined(CSSC_EMB_HAS_ARDUINO)
    uint32_t now = (uint32_t)millis();
#elif defined(_WIN32)
    extern unsigned long GetTickCount(void);
    uint32_t now = (uint32_t)GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint32_t now = (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
    if (now - _cssc_diag_last_tick_ms < CSSC_DIAG_TICK_MIN_MS) return;
    _cssc_diag_last_tick_ms = now;
    cssc_diag_emit_mem();
}

CSSC_API CsscVal cssc_builtin_diag_mem(void) {
    cssc_diag_emit_mem();
    return cssc_int((int64_t)_cssc_diag_free_heap());
}

CSSC_API CsscVal cssc_builtin_diag_cpu(CsscVal name, CsscVal ms) {
    cssc_diag_emit_cpu(cssc_to_cstr(name), (uint32_t)cssc_to_int(ms));
    return cssc_null();
}

CSSC_API CsscVal cssc_builtin_diag_enable(CsscVal on) {
    cssc_diag_enable((int)cssc_to_int(on) != 0);
    return cssc_null();
}

#else  /* CSSC_DIAG not defined — empty stubs keep the ABI stable */

CSSC_API void cssc_diag_enable(int on)               { (void)on; }
CSSC_API int  cssc_diag_is_enabled(void)             { return 0; }
CSSC_API void cssc_diag_emit_mem(void)               { }
CSSC_API void cssc_diag_emit_cpu(const char* n, uint32_t m) { (void)n; (void)m; }
CSSC_API void cssc_diag_tick(void)                   { }
CSSC_API CsscVal cssc_builtin_diag_mem(void)         { return cssc_int(0); }
CSSC_API CsscVal cssc_builtin_diag_cpu(CsscVal n, CsscVal m) { (void)n; (void)m; return cssc_null(); }
CSSC_API CsscVal cssc_builtin_diag_enable(CsscVal o) { (void)o; return cssc_null(); }

#endif  /* CSSC_DIAG */

/* File I/O */
CSSC_API CsscVal cssc_builtin_read_file(CsscVal path) {
    FILE* f = fopen(cssc_to_cstr(path), "rb");
    if (!f) return cssc_null();
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)cssc_alloc((size_t)sz + 1);
    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);
    return cssc_string_owned(buf);
}

CSSC_API void cssc_builtin_write_file(CsscVal path, CsscVal content) {
    FILE* f = fopen(cssc_to_cstr(path), "wb");
    if (!f) return;
    const char* data = cssc_to_cstr(content);
    fwrite(data, 1, strlen(data), f);
    fclose(f);
}

CSSC_API CsscVal cssc_builtin_file_exists(CsscVal path) {
    FILE* f = fopen(cssc_to_cstr(path), "r");
    if (f) { fclose(f); return cssc_bool(true); }
    return cssc_bool(false);
}

CSSC_API void cssc_builtin_mkdir(CsscVal path) {
#if defined(_WIN32)
    _mkdir(cssc_to_cstr(path));
#elif defined(CSSC_EMBEDDED)
    /* Bare-metal MCUs have no host filesystem. Fail audibly — earlier
     * silent no-op masked bugs where scripts thought a directory was
     * created and then write-failed mysteriously later. Mount SPIFFS /
     * LittleFS from your wrapper if you need persistent paths, or
     * #catch this call. */
    (void)path;
    cssc_panic("cssc::mkdir unavailable on embedded — mount SPIFFS "
               "or LittleFS in your wrapper, or guard with #catch.");
#else
    mkdir(cssc_to_cstr(path), 0755);
#endif
}

/* =========================================================================
 * 15. ERROR HANDLING
 * ========================================================================= */

/* ---- Catch / try infrastructure --------------------------------------- *
 * Backs CSSC's `#catch (executable) ?caller { … }` directive in native
 * builds. A jmp_buf stack is pushed before each attempted call; if the call
 * panics, longjmp unwinds back to the catch site with the error captured in
 * g_cssc_last_error. With g_cssc_catch_depth == 0 (no active catch),
 * cssc_panic keeps its original abort-on-error semantics.
 */
#include <setjmp.h>

#ifndef CSSC_CATCH_STACK_MAX
#define CSSC_CATCH_STACK_MAX 64
#endif
static jmp_buf g_cssc_catch_stack[CSSC_CATCH_STACK_MAX];
static int     g_cssc_catch_depth = 0;
static char    g_cssc_last_error[CSSC_LAST_ERROR_MAX];

/* Toggle for #debug / #trace in compiled binaries. CLI flag flips this. */
CSSC_API int g_cssc_debug_enabled = 0;
#define _cssc_debug_enabled (g_cssc_debug_enabled)

CSSC_API void cssc_set_debug(int enabled) {
    g_cssc_debug_enabled = enabled ? 1 : 0;
}

CSSC_API jmp_buf* cssc_catch_push(void) {
    if (g_cssc_catch_depth >= CSSC_CATCH_STACK_MAX) {
        return NULL;
    }
    return &g_cssc_catch_stack[g_cssc_catch_depth++];
}

CSSC_API void cssc_catch_pop(void) {
    if (g_cssc_catch_depth > 0) g_cssc_catch_depth--;
}

CSSC_API const char* cssc_catch_last_error(void) {
    return g_cssc_last_error;
}

CSSC_API void cssc_panic(const char* message) {
    if (g_cssc_catch_depth > 0) {
        snprintf(g_cssc_last_error, sizeof(g_cssc_last_error), "%s",
                 message ? message : "panic");
        longjmp(g_cssc_catch_stack[g_cssc_catch_depth - 1], 1);
    }
    fprintf(stderr, "cssc: fatal error: CSSC PANIC: %s\n", message);
    fflush(stderr);
    exit(1);
}

CSSC_API void cssc_panicf(const char* fmt, ...) {
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    cssc_panic(buf);
}

CSSC_API void cssc_error(const char* operation, const char* detail, int line) {
    if (g_cssc_catch_depth > 0) {
        if (line > 0) {
            snprintf(g_cssc_last_error, sizeof(g_cssc_last_error),
                     "%s Error line %d: %s", operation, line, detail);
        } else {
            snprintf(g_cssc_last_error, sizeof(g_cssc_last_error),
                     "%s Error: %s", operation, detail);
        }
        longjmp(g_cssc_catch_stack[g_cssc_catch_depth - 1], 1);
    }
    if (line > 0) {
        fprintf(stderr, "cssc: fatal error: CSSC %s Error line %d: %s\n", operation, line, detail);
    } else {
        fprintf(stderr, "cssc: fatal error: CSSC %s Error: %s\n", operation, detail);
    }
    fflush(stderr);
    exit(1);
}

/* =========================================================================
 * 16. ASMH HOTLOADING BRIDGE
 * ========================================================================= */

CSSC_API bool cssc_hotload_init(CsscHotloadContext* ctx, const char* dll_path) {
#if defined(_WIN32)
    ctx->dll_handle = LoadLibraryA(dll_path);
    if (!ctx->dll_handle) {
        /* DLL not found — run without hotloading (no dynamic features) */
        ctx->functions = NULL;
        ctx->func_names = NULL;
        ctx->func_count = 0;
        return false;
    }
    return true;
#elif defined(CSSC_EMBEDDED)
    /* Bare-metal MCUs have no dynamic library loader. ASMH hotload is
     * a desktop-only debug aid — return a clean "no DLL" result so any
     * #include('asm.hot') or similar gracefully no-ops. */
    (void)ctx; (void)dll_path;
    if (ctx) {
        ctx->dll_handle = NULL;
        ctx->functions = NULL;
        ctx->func_names = NULL;
        ctx->func_count = 0;
    }
    return false;
#else
    ctx->dll_handle = dlopen(dll_path, RTLD_LAZY);
    if (!ctx->dll_handle) {
        ctx->functions = NULL;
        ctx->func_names = NULL;
        ctx->func_count = 0;
        return false;
    }
    return true;
#endif
}

CSSC_API CsscHotloadFn cssc_hotload_resolve(CsscHotloadContext* ctx, const char* name) {
    if (!ctx->dll_handle) return NULL;
#if defined(_WIN32)
    return (CsscHotloadFn)GetProcAddress((HMODULE)ctx->dll_handle, name);
#elif defined(CSSC_EMBEDDED)
    (void)name;
    return NULL;
#else
    return (CsscHotloadFn)dlsym(ctx->dll_handle, name);
#endif
}

CSSC_API void cssc_hotload_shutdown(CsscHotloadContext* ctx) {
    if (ctx->dll_handle) {
#if defined(_WIN32)
        FreeLibrary((HMODULE)ctx->dll_handle);
#elif defined(CSSC_EMBEDDED)
        /* Nothing was loaded — cssc_hotload_init returns false on
         * embedded so dll_handle should never be non-NULL here, but
         * guard defensively. */
#else
        dlclose(ctx->dll_handle);
#endif
        ctx->dll_handle = NULL;
    }
    if (ctx->functions) { cssc_free(ctx->functions); ctx->functions = NULL; }
    if (ctx->func_names) { cssc_free((void*)ctx->func_names); ctx->func_names = NULL; }
    ctx->func_count = 0;
}

/* =========================================================================
 * 16b. MODULE BUILTINS — peek
 * ========================================================================= */

CSSC_API CsscVal cssc_peek_peek(CsscVal collection, int64_t index, int64_t amount) {
    /* peek(collection, index, amount) → collection[index + amount] */
    int64_t target = index + amount;
    if (CSSC_IS_VECTOR(collection)) {
        return cssc_vector_get(collection, target);
    }
    if (CSSC_IS_STRING(collection)) {
        return cssc_string_char_at(collection, target);
    }
    return cssc_null();
}

CSSC_API CsscVal cssc_peek_cpeek(CsscVal collection, int64_t index, int64_t count) {
    /* cpeek(collection, index, count) → slice [index..index+count) */
    if (CSSC_IS_VECTOR(collection)) {
        return cssc_vector_slice(collection, index, index + count);
    }
    if (CSSC_IS_STRING(collection)) {
        return cssc_string_substr(collection, index, count);
    }
    return cssc_null();
}

CSSC_API CsscVal cssc_peek_peek_safe(CsscVal collection, int64_t index, CsscVal fallback) {
    CsscVal result = cssc_peek_peek(collection, index, 0);
    return CSSC_IS_NULL(result) ? fallback : result;
}

CSSC_API bool cssc_peek_has_next(CsscVal collection, int64_t index) {
    if (CSSC_IS_VECTOR(collection)) {
        return index + 1 < cssc_vector_size(collection);
    }
    if (CSSC_IS_STRING(collection)) {
        return (size_t)(index + 1) < cssc_strlen(collection);
    }
    return false;
}

/* =========================================================================
 * 16c. MODULE BUILTINS — paths, binary, io
 * ========================================================================= */

/* cssc.paths */
CSSC_API CsscVal cssc_paths_exists(CsscVal path) {
    const char* p = cssc_to_cstr(path);
    FILE* f = fopen(p, "r");
    if (f) { fclose(f); return cssc_bool(true); }
    /* Check if directory */
#if defined(_WIN32)
    DWORD attr = GetFileAttributesA(p);
    return cssc_bool(attr != INVALID_FILE_ATTRIBUTES);
#elif defined(CSSC_EMBEDDED)
    /* No filesystem on bare-metal — only the fopen check above runs.
     * If the platform has SPIFFS / LittleFS the user would need to
     * mount it before calling cssc_paths_exists; here we just say
     * "doesn't exist as a directory" since we can't stat. */
    return cssc_bool(false);
#else
    struct stat st;
    return cssc_bool(stat(p, &st) == 0);
#endif
}

CSSC_API CsscVal cssc_paths_mkdir(CsscVal path) {
#if defined(_WIN32)
    int r = _mkdir(cssc_to_cstr(path));
    return cssc_bool(r == 0 || errno == EEXIST);
#elif defined(CSSC_EMBEDDED)
    /* No host filesystem on bare-metal. Earlier we lied with
     * `return cssc_bool(true)` ("already exists") which let scripts
     * proceed as if the directory was usable. Now we honestly return
     * false so the caller's `if (!paths.mkdir(...))` branch fires. */
    (void)path;
    return cssc_bool(false);
#else
    int r = mkdir(cssc_to_cstr(path), 0755);
    return cssc_bool(r == 0 || errno == EEXIST);
#endif
}

CSSC_API CsscVal cssc_paths_dirname(CsscVal path) {
    const char* p = cssc_to_cstr(path);
    size_t len = strlen(p);
    /* Find last separator */
    const char* sep = p + len;
    while (sep > p && *sep != '/' && *sep != '\\') sep--;
    if (sep == p) return cssc_string(".");
    return cssc_string_len(p, (size_t)(sep - p));
}

CSSC_API CsscVal cssc_paths_basename(CsscVal path) {
    const char* p = cssc_to_cstr(path);
    const char* sep = strrchr(p, '/');
    const char* sep2 = strrchr(p, '\\');
    if (sep2 && (!sep || sep2 > sep)) sep = sep2;
    return cssc_string(sep ? sep + 1 : p);
}

CSSC_API CsscVal cssc_paths_join(CsscVal a, CsscVal b) {
    const char* ca = cssc_to_cstr(a);
    const char* cb = cssc_to_cstr(b);
    size_t la = strlen(ca), lb = strlen(cb);
    char* buf = (char*)cssc_alloc(la + lb + 2);
    memcpy(buf, ca, la);
    if (la > 0 && ca[la-1] != '/' && ca[la-1] != '\\') {
        buf[la] = '/';
        memcpy(buf + la + 1, cb, lb + 1);
    } else {
        memcpy(buf + la, cb, lb + 1);
    }
    return cssc_string_owned(buf);
}

CSSC_API CsscVal cssc_paths_ext(CsscVal path) {
    const char* p = cssc_to_cstr(path);
    const char* dot = strrchr(p, '.');
    if (!dot || dot == p) return cssc_string("");
    return cssc_string(dot);
}

CSSC_API CsscVal cssc_paths_resolve(CsscVal path) {
    char buf[4096];
#ifdef _WIN32
    DWORD len = GetFullPathNameA(cssc_to_cstr(path), sizeof(buf), buf, NULL);
    if (len > 0 && len < sizeof(buf)) return cssc_string(buf);
#else
    char* resolved = realpath(cssc_to_cstr(path), buf);
    if (resolved) return cssc_string(resolved);
#endif
    return cssc_copy(path);
}

/* cssc.binary — serialize/deserialize CSSC values to binary */
CSSC_API CsscVal cssc_binary_read(CsscVal path) {
    return cssc_builtin_read_file(path);
}

CSSC_API void cssc_binary_write(CsscVal path, CsscVal data) {
    cssc_builtin_write_file(path, data);
}

CSSC_API CsscVal cssc_binary_tobinary(CsscVal value) {
    /* Serialize a vector<vector<string>> to a binary-safe string format.
     * Format: pages separated by \x01\x02, lines separated by \x01.
     * For non-vector values, fall back to string format. */
    if (CSSC_TYPE(value) != CSSC_TYPE_VECTOR) {
        return cssc_format_value(value);
    }
    CsscVal result = cssc_string("");
    int64_t outer_size = cssc_vector_size(value);
    for (int64_t p = 0; p < outer_size; p++) {
        if (p > 0) result = cssc_string_concat(result, cssc_string("\x01\x02"));
        CsscVal page = cssc_vector_get(value, p);
        if (CSSC_TYPE(page) == CSSC_TYPE_VECTOR) {
            int64_t inner_size = cssc_vector_size(page);
            for (int64_t l = 0; l < inner_size; l++) {
                if (l > 0) result = cssc_string_concat(result, cssc_string("\x01"));
                CsscVal line = cssc_vector_get(page, l);
                CsscVal line_str = cssc_to_string_val(line);
                result = cssc_string_concat(result, line_str);
                cssc_release(line_str);
            }
        } else {
            CsscVal s = cssc_to_string_val(page);
            result = cssc_string_concat(result, s);
            cssc_release(s);
        }
    }
    return result;
}

CSSC_API CsscVal cssc_binary_frombinary(CsscVal data) {
    /* Deserialize from binary format back to vector<vector<string>>.
     * Pages separated by \x01\x02, lines separated by \x01. */
    if (!CSSC_IS_STRING(data)) return cssc_copy(data);
    const char* raw = cssc_to_cstr(data);
    size_t raw_len = cssc_strlen(data);
    if (raw_len == 0) return cssc_vector(1);

    CsscVal result = cssc_vector(8);
    CsscVal current_page = cssc_vector(8);
    const char* line_start = raw;

    for (size_t i = 0; i <= raw_len; i++) {
        if (i == raw_len || (raw[i] == '\x01' && i + 1 < raw_len && raw[i + 1] == '\x02')) {
            /* End of page: push current line, then push page */
            size_t line_len = &raw[i] - line_start;
            if (line_len > 0 || cssc_vector_size(current_page) > 0) {
                cssc_vector_push(current_page, cssc_string_len(line_start, line_len));
            }
            cssc_vector_push(result, current_page);
            if (i < raw_len) {
                current_page = cssc_vector(8);
                i++; /* skip the \x02 byte */
                line_start = &raw[i + 1];
            }
        } else if (raw[i] == '\x01') {
            /* Line separator within a page */
            size_t line_len = &raw[i] - line_start;
            cssc_vector_push(current_page, cssc_string_len(line_start, line_len));
            line_start = &raw[i + 1];
        }
    }
    return result;
}

/* cssc.io (stdio module) */
CSSC_API CsscVal cssc_io_read_file(CsscVal path) { return cssc_builtin_read_file(path); }
CSSC_API void cssc_io_write_file(CsscVal path, CsscVal content) { cssc_builtin_write_file(path, content); }
CSSC_API CsscVal cssc_io_exists(CsscVal path) { return cssc_paths_exists(path); }

CSSC_API void cssc_io_remove(CsscVal path) {
    remove(cssc_to_cstr(path));
}

CSSC_API void cssc_io_copy(CsscVal src, CsscVal dst) {
    CsscVal content = cssc_builtin_read_file(src);
    if (!CSSC_IS_NULL(content)) {
        cssc_builtin_write_file(dst, content);
        cssc_release(content);
    }
}

CSSC_API CsscVal cssc_io_list_dir(CsscVal path) {
    CsscVal result = cssc_vector(16);
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    char pattern[4096];
    snprintf(pattern, sizeof(pattern), "%s\\*", cssc_to_cstr(path));
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                cssc_vector_push(result, cssc_string(fd.cFileName));
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#endif
    return result;
}

/* =========================================================================
 * 16c2. HEX-KEYED FIRST-CLASS STORAGE — `0xNN` as identifier / scope handle
 * ========================================================================= */

#ifndef MAX_HEX_VARS
#define MAX_HEX_VARS    256
#endif
#ifndef MAX_HEX_SCOPES
#define MAX_HEX_SCOPES  128
#endif

typedef struct {
    uint64_t hex_id;
    CsscVal  value;
    bool     occupied;
} CsscHexVar;

typedef struct {
    uint64_t        hex_id;
    CsscScopeFrame  frame;
    bool            occupied;
    bool            initialized;
    bool            freed;
} CsscHexScope;

static CsscHexVar    g_hex_vars[MAX_HEX_VARS];
static CsscHexScope  g_hex_scopes[MAX_HEX_SCOPES];

static CsscHexVar* _hex_var_find(uint64_t hex_id) {
    for (uint32_t i = 0; i < MAX_HEX_VARS; i++) {
        if (g_hex_vars[i].occupied && g_hex_vars[i].hex_id == hex_id) {
            return &g_hex_vars[i];
        }
    }
    return NULL;
}

static CsscHexVar* _hex_var_alloc_slot(void) {
    for (uint32_t i = 0; i < MAX_HEX_VARS; i++) {
        if (!g_hex_vars[i].occupied) return &g_hex_vars[i];
    }
    return NULL;
}

CSSC_API void cssc_hex_var_set(uint64_t hex_id, CsscVal value) {
    CsscHexVar* slot = _hex_var_find(hex_id);
    if (slot) {
        cssc_release(slot->value);
        cssc_retain(value);
        slot->value = value;
        return;
    }
    slot = _hex_var_alloc_slot();
    if (!slot) {
        cssc_panic("hex-var table exhausted (MAX_HEX_VARS)");
    }
    cssc_retain(value);
    slot->hex_id = hex_id;
    slot->value = value;
    slot->occupied = true;
}

CSSC_API CsscVal cssc_hex_var_get(uint64_t hex_id) {
    CsscHexVar* slot = _hex_var_find(hex_id);
    return slot ? slot->value : cssc_null();
}

CSSC_API bool cssc_hex_var_has(uint64_t hex_id) {
    return _hex_var_find(hex_id) != NULL;
}

CSSC_API void cssc_hex_var_delete(uint64_t hex_id) {
    CsscHexVar* slot = _hex_var_find(hex_id);
    if (!slot) return;
    cssc_release(slot->value);
    slot->occupied = false;
    slot->hex_id = 0;
    slot->value = cssc_null();
}

static CsscHexScope* _hex_scope_find(uint64_t hex_id) {
    for (uint32_t i = 0; i < MAX_HEX_SCOPES; i++) {
        if (g_hex_scopes[i].occupied && g_hex_scopes[i].hex_id == hex_id) {
            return &g_hex_scopes[i];
        }
    }
    return NULL;
}

static CsscHexScope* _hex_scope_alloc_slot(void) {
    for (uint32_t i = 0; i < MAX_HEX_SCOPES; i++) {
        if (!g_hex_scopes[i].occupied) return &g_hex_scopes[i];
    }
    return NULL;
}

CSSC_API void cssc_hex_scope_define(uint64_t hex_id, CsscHexScopeInitFn init_fn) {
    CsscHexScope* slot = _hex_scope_find(hex_id);
    if (slot) {
        /* Re-define: blow away the old frame and rebuild. */
        if (slot->initialized) cssc_frame_destroy(&slot->frame);
        slot->initialized = false;
        slot->freed = false;
    } else {
        slot = _hex_scope_alloc_slot();
        if (!slot) {
            cssc_panic("hex-scope table exhausted (MAX_HEX_SCOPES)");
        }
        slot->hex_id = hex_id;
        slot->occupied = true;
    }
    cssc_frame_init(&slot->frame);
    if (init_fn) init_fn(&slot->frame);
    slot->initialized = true;
}

CSSC_API CsscScopeFrame* cssc_hex_scope_get(uint64_t hex_id) {
    CsscHexScope* slot = _hex_scope_find(hex_id);
    if (!slot || !slot->initialized || slot->freed) return NULL;
    return &slot->frame;
}

CSSC_API void cssc_hex_scope_free(uint64_t hex_id) {
    CsscHexScope* slot = _hex_scope_find(hex_id);
    if (!slot) return;
    if (slot->initialized && !slot->freed) {
        cssc_frame_destroy(&slot->frame);
    }
    slot->freed = true;
    slot->occupied = false;
    slot->hex_id = 0;
    slot->initialized = false;
}

/* =========================================================================
 * 16d. DEFERRED SECTORS — zero-RAM until #reserve
 * ========================================================================= */

#ifndef MAX_DEFERRED
#define MAX_DEFERRED 64
#endif
static CsscDeferredSector g_deferred[MAX_DEFERRED];
static uint32_t g_deferred_count = 0;

CSSC_API void cssc_deferred_register(CsscScopeStack* scope, const char* label, void* init_fn) {
    if (g_deferred_count >= MAX_DEFERRED) {
        cssc_panic("too many deferred sectors (max 64)");
    }
    CsscDeferredSector* d = &g_deferred[g_deferred_count++];
    d->label = cssc_intern(label);
    d->init_fn = init_fn;
    d->initialized = false;
    d->sector_val = cssc_null();
    /* Register in scope as null — no RAM allocated for sector body yet */
    cssc_scope_set(scope, label, cssc_null());
}

CSSC_API CsscVal cssc_deferred_reserve(CsscScopeStack* scope, const char* label) {
    for (uint32_t i = 0; i < g_deferred_count; i++) {
        if (strcmp(g_deferred[i].label, label) == 0) {
            if (g_deferred[i].initialized) {
                return g_deferred[i].sector_val;
            }
            /* Initialize now — call the init function which builds the sector */
            typedef CsscVal (*InitFn)(CsscScopeStack*);
            InitFn fn = (InitFn)g_deferred[i].init_fn;
            CsscVal result = fn(scope);
            g_deferred[i].sector_val = result;
            g_deferred[i].initialized = true;
            cssc_scope_set(scope, label, result);
            return result;
        }
    }
    cssc_panicf("deferred sector '%s' not found", label);
    return cssc_null();
}

/* =========================================================================
 * 16e. #load COMPANION DLL
 * ========================================================================= */

CSSC_API CsscVal cssc_load_dll(CsscScopeStack* scope, const char* dll_path, const char* alias) {
#ifdef _WIN32
    HMODULE h = LoadLibraryA(dll_path);
    if (!h) {
        /* Return null silently — caller can try fallback paths */
        return cssc_null();
    }
    /* Look for exported init function: cssc_module_init.
     * The init function builds and returns a sector containing the module's exports. */
    typedef CsscVal (*ModuleInitFn)(CsscScopeStack*);
    ModuleInitFn init_fn = (ModuleInitFn)GetProcAddress(h, "cssc_module_init");

    CsscVal module;
    if (init_fn) {
        module = init_fn(scope);
        if (CSSC_IS_NULL(module)) {
            /* Init ran but returned nothing — fall back to empty sector */
            module = cssc_sector_create(alias);
        }
    } else {
        /* No init function — just an empty sector */
        module = cssc_sector_create(alias);
    }
    cssc_scope_set(scope, alias, module);
    return module;
#else
    return cssc_null();
#endif
}

CSSC_API void cssc_unload_dll(CsscVal module) {
    /* Sector free handles cleanup */
    cssc_sector_free(module);
}

/* =========================================================================
 * 17. FORMAT UTILITIES
 * ========================================================================= */

CSSC_API CsscVal cssc_format_value(CsscVal v) {
    char buf[256];
    switch (CSSC_TYPE(v)) {
        case CSSC_TYPE_NULL:
            return cssc_string("0x0");
        case CSSC_TYPE_INT:
            /* Avoid printf %lld which on ESP8266 newlib needs
             * -u _printf_long_long linker flag — without it the
             * call returns garbage and corrupts the string concat
             * pipeline (was the silent crash for `"" + int + ""`
             * patterns on embedded). */
            _cssc_fmt_i64(buf, v.data.i);
            return cssc_string(buf);
        case CSSC_TYPE_FLOAT:
            /* Same libc-flag issue with %g — was actually CRASHING
             * on ESP8266 (Exception 2: instruction-fetch from data
             * region) when called from string concat in object
             * label bodies. Use our hand-rolled float formatter
             * which has zero libc dependencies. */
            _cssc_fmt_f64(buf, v.data.f);
            return cssc_string(buf);
        case CSSC_TYPE_BOOL:
            return cssc_string(v.data.b ? "true" : "false");
        case CSSC_TYPE_STRING:
            cssc_retain(v);
            return v;
        case CSSC_TYPE_VECTOR: {
            /* Format as {item, item, ...} */
            int64_t sz = cssc_vector_size(v);
            CsscVal result = cssc_string("{");
            for (int64_t i = 0; i < sz; i++) {
                if (i > 0) result = cssc_string_concat(result, cssc_string(", "));
                CsscVal item_str = cssc_format_value(cssc_vector_get(v, i));
                result = cssc_string_concat(result, item_str);
                cssc_release(item_str);
            }
            result = cssc_string_concat(result, cssc_string("}"));
            return result;
        }
        case CSSC_TYPE_MAP: {
            CsscMap* map = (CsscMap*)v.data.ptr;
            CsscVal result = cssc_string("{");
            bool first = true;
            for (uint32_t i = 0; i < map->bucket_count; i++) {
                if (map->buckets[i].occupied) {
                    if (!first) result = cssc_string_concat(result, cssc_string(", "));
                    result = cssc_string_concat(result, cssc_string(map->buckets[i].key));
                    result = cssc_string_concat(result, cssc_string(": "));
                    CsscVal vs = cssc_format_value(map->buckets[i].value);
                    result = cssc_string_concat(result, vs);
                    cssc_release(vs);
                    first = false;
                }
            }
            result = cssc_string_concat(result, cssc_string("}"));
            return result;
        }
        case CSSC_TYPE_BIND: {
            CsscBind* bind = (CsscBind*)v.data.ptr;
            CsscVal result = cssc_string("{");
            for (uint32_t i = 0; i < bind->header.length; i++) {
                if (i > 0) result = cssc_string_concat(result, cssc_string("; "));
                CsscVal ks = cssc_format_value(bind->pairs[i * 2]);
                CsscVal vs = cssc_format_value(bind->pairs[i * 2 + 1]);
                result = cssc_string_concat(result, ks);
                result = cssc_string_concat(result, cssc_string(", "));
                result = cssc_string_concat(result, vs);
                cssc_release(ks);
                cssc_release(vs);
            }
            result = cssc_string_concat(result, cssc_string("}"));
            return result;
        }
        case CSSC_TYPE_FUNCTION:
            snprintf(buf, sizeof(buf), "<function:%s>", ((CsscFunction*)v.data.ptr)->name);
            return cssc_string(buf);
        case CSSC_TYPE_SECTOR:
            snprintf(buf, sizeof(buf), "<sector:%s>", ((CsscSector*)v.data.ptr)->name);
            return cssc_string(buf);
        case CSSC_TYPE_OBJECT:
            snprintf(buf, sizeof(buf), "<object:%s>", ((CsscObject*)v.data.ptr)->name);
            return cssc_string(buf);
        default:
            snprintf(buf, sizeof(buf), "<%s@%p>", cssc_typeof_str(v), v.data.ptr);
            return cssc_string(buf);
    }
}

/* =========================================================================
 * 18. GLOBAL STATE
 * ========================================================================= */

/* =========================================================================
 * 18a. GRAPHICS — Win32 GDI-backed video window
 * ========================================================================= */

#ifdef _WIN32

struct CsscVideoImpl {
    HWND hwnd;
    HDC memDC;
    HBITMAP dib;
    uint32_t* backing_pixels;  /* DIB-backed memory */
    uint32_t width;
    uint32_t height;
    HANDLE thread;             /* message pump thread */
    volatile int should_close;
    CRITICAL_SECTION cs;       /* protects backing_pixels copies */
};

static LRESULT CALLBACK _cssc_video_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            CsscVideoImpl* impl = (CsscVideoImpl*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
            if (impl && impl->memDC) {
                EnterCriticalSection(&impl->cs);
                BitBlt(hdc, 0, 0, (int)impl->width, (int)impl->height,
                       impl->memDC, 0, 0, SRCCOPY);
                LeaveCriticalSection(&impl->cs);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CLOSE: {
            CsscVideoImpl* impl = (CsscVideoImpl*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
            if (impl) impl->should_close = 1;
            DestroyWindow(hwnd);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

typedef struct {
    CsscVideoImpl* impl;
    uint32_t width;
    uint32_t height;
    HANDLE ready_event;
} _VideoThreadArgs;

static DWORD WINAPI _cssc_video_thread(LPVOID param) {
    _VideoThreadArgs* args = (_VideoThreadArgs*)param;
    CsscVideoImpl* impl = args->impl;

    /* Register window class (once per process is fine — Windows allows re-registration attempts) */
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = _cssc_video_wndproc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "CsscVideoWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClassA(&wc);

    /* Create window */
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT r = {0, 0, (LONG)args->width, (LONG)args->height};
    AdjustWindowRect(&r, style, FALSE);
    HWND hwnd = CreateWindowExA(
        0, "CsscVideoWindow", "CSSC Video",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );
    if (!hwnd) {
        SetEvent(args->ready_event);
        return 1;
    }
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)impl);

    /* Create DIB section backing the memDC */
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)args->width;
    bmi.bmiHeader.biHeight = -(LONG)args->height;  /* top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screenDC = GetDC(hwnd);
    void* pixels = NULL;
    HBITMAP dib = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &pixels, NULL, 0);
    HDC memDC = CreateCompatibleDC(screenDC);
    SelectObject(memDC, dib);
    ReleaseDC(hwnd, screenDC);

    impl->hwnd = hwnd;
    impl->memDC = memDC;
    impl->dib = dib;
    impl->backing_pixels = (uint32_t*)pixels;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    SetEvent(args->ready_event);

    /* Message loop — use PeekMessage + sleep so we can poll should_close */
    MSG msg;
    while (!impl->should_close) {
        if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        } else {
            Sleep(1);
        }
    }

    if (impl->memDC) { DeleteDC(impl->memDC); impl->memDC = NULL; }
    if (impl->dib)   { DeleteObject(impl->dib); impl->dib = NULL; }
    impl->hwnd = NULL;
    return 0;
}

#endif /* _WIN32 */

CSSC_API CsscVal cssc_video_create(int64_t width, int64_t height, int64_t fps) {
    CsscVideo* v = (CsscVideo*)cssc_alloc(sizeof(CsscVideo));
    memset(v, 0, sizeof(CsscVideo));
    v->header.refcount = 1;
    v->header.type = CSSC_TYPE_MODULE; /* reuse tag slot — not a real module */
    v->width = (uint32_t)width;
    v->height = (uint32_t)height;
    v->fps = (uint32_t)fps;

#ifdef _WIN32
    CsscVideoImpl* impl = (CsscVideoImpl*)cssc_alloc(sizeof(CsscVideoImpl));
    memset(impl, 0, sizeof(CsscVideoImpl));
    impl->width = (uint32_t)width;
    impl->height = (uint32_t)height;
    InitializeCriticalSection(&impl->cs);

    _VideoThreadArgs args;
    args.impl = impl;
    args.width = (uint32_t)width;
    args.height = (uint32_t)height;
    args.ready_event = CreateEventA(NULL, TRUE, FALSE, NULL);

    impl->thread = CreateThread(NULL, 0, _cssc_video_thread, &args, 0, NULL);
    if (impl->thread) {
        WaitForSingleObject(args.ready_event, 5000);
    }
    CloseHandle(args.ready_event);
    v->impl = impl;
#endif

    CsscVal r;
    r.tag = CSSC_TYPE_MODULE;   /* tag 14 — re-used for video */
    r.data.ptr = v;
    return r;
}

CSSC_API void cssc_video_free(CsscVal v) {
    if (!v.data.ptr) return;
#ifdef _WIN32
    CsscVideo* video = (CsscVideo*)v.data.ptr;
    if (video->impl) {
        video->impl->should_close = 1;
        if (video->impl->hwnd) {
            PostMessageA(video->impl->hwnd, WM_CLOSE, 0, 0);
        }
        if (video->impl->thread) {
            WaitForSingleObject(video->impl->thread, 2000);
            CloseHandle(video->impl->thread);
        }
        DeleteCriticalSection(&video->impl->cs);
        cssc_free(video->impl);
        video->impl = NULL;
    }
#endif
}

CSSC_API bool cssc_video_is_open(CsscVal v) {
    if (!v.data.ptr) return false;
#ifdef _WIN32
    CsscVideo* video = (CsscVideo*)v.data.ptr;
    return video->impl && video->impl->hwnd && !video->impl->should_close;
#else
    return false;
#endif
}

CSSC_API CsscVal cssc_framebuffer_create(int64_t width, int64_t height) {
    CsscFramebuffer* fb = (CsscFramebuffer*)cssc_alloc(sizeof(CsscFramebuffer));
    memset(fb, 0, sizeof(CsscFramebuffer));
    fb->header.refcount = 1;
    fb->header.type = CSSC_TYPE_MODULE;
    fb->width = (uint32_t)width;
    fb->height = (uint32_t)height;
    size_t n = (size_t)width * (size_t)height;
    fb->pixels = (uint32_t*)cssc_alloc(n * sizeof(uint32_t));
    memset(fb->pixels, 0, n * sizeof(uint32_t));
    CsscVal r;
    r.tag = CSSC_TYPE_MODULE;
    r.data.ptr = fb;
    return r;
}

CSSC_API void cssc_framebuffer_free(CsscVal v) {
    if (!v.data.ptr) return;
    CsscFramebuffer* fb = (CsscFramebuffer*)v.data.ptr;
    if (fb->pixels) { cssc_free(fb->pixels); fb->pixels = NULL; }
}

CSSC_API CsscVal cssc_matrix_create(int64_t width, int64_t height) {
    CsscMatrix* m = (CsscMatrix*)cssc_alloc(sizeof(CsscMatrix));
    memset(m, 0, sizeof(CsscMatrix));
    m->header.refcount = 1;
    m->header.type = CSSC_TYPE_MATRIX;
    m->width = (uint32_t)width;
    m->height = (uint32_t)height;
    size_t n = (size_t)width * (size_t)height;
    m->pixels = (uint32_t*)cssc_alloc(n * sizeof(uint32_t));
    memset(m->pixels, 0, n * sizeof(uint32_t));
    CsscVal r;
    r.tag = CSSC_TYPE_MATRIX;
    r.data.ptr = m;
    return r;
}

CSSC_API void cssc_matrix_free(CsscVal v) {
    if (!v.data.ptr) return;
    CsscMatrix* m = (CsscMatrix*)v.data.ptr;
    if (m->pixels) { cssc_free(m->pixels); m->pixels = NULL; }
}

CSSC_API uint32_t cssc_color_from_val(CsscVal v) {
    if (CSSC_IS_INT(v))   return (uint32_t)v.data.i;
    if (CSSC_IS_FLOAT(v)) return (uint32_t)v.data.f;
    /* string like "#ff0000" or "0xff0000" — parse */
    if (CSSC_IS_STRING(v)) {
        const char* s = cssc_to_cstr(v);
        if (!s) return 0;
        if (s[0] == '#') s++;
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
        uint32_t c = (uint32_t)strtoul(s, NULL, 16);
        return c;
    }
    return 0;
}

CSSC_API CsscVal cssc_matrix_fill(CsscVal mval, CsscVal color) {
    if (!mval.data.ptr) return mval;
    CsscMatrix* m = (CsscMatrix*)mval.data.ptr;
    uint32_t c = cssc_color_from_val(color);
    /* Use argb 0xAARRGGBB — set alpha to FF if missing */
    if ((c & 0xFF000000) == 0) c |= 0xFF000000;
    size_t n = (size_t)m->width * (size_t)m->height;
    for (size_t i = 0; i < n; i++) m->pixels[i] = c;
    return mval;
}

CSSC_API void cssc_matrix_set_pixel(CsscVal mval, int64_t x, int64_t y, uint32_t color) {
    if (!mval.data.ptr) return;
    CsscMatrix* m = (CsscMatrix*)mval.data.ptr;
    if (x < 0 || y < 0 || (uint32_t)x >= m->width || (uint32_t)y >= m->height) return;
    if ((color & 0xFF000000) == 0) color |= 0xFF000000;
    m->pixels[(size_t)y * m->width + (size_t)x] = color;
}

CSSC_API uint32_t cssc_matrix_get_pixel(CsscVal mval, int64_t x, int64_t y) {
    if (!mval.data.ptr) return 0;
    CsscMatrix* m = (CsscMatrix*)mval.data.ptr;
    if (x < 0 || y < 0 || (uint32_t)x >= m->width || (uint32_t)y >= m->height) return 0;
    return m->pixels[(size_t)y * m->width + (size_t)x];
}

CSSC_API void cssc_matrix_copy_to_framebuffer(CsscVal mval, CsscVal fval) {
    if (!mval.data.ptr || !fval.data.ptr) return;
    CsscMatrix* m = (CsscMatrix*)mval.data.ptr;
    CsscFramebuffer* f = (CsscFramebuffer*)fval.data.ptr;
    uint32_t w = (m->width < f->width) ? m->width : f->width;
    uint32_t h = (m->height < f->height) ? m->height : f->height;
    for (uint32_t y = 0; y < h; y++) {
        memcpy(&f->pixels[(size_t)y * f->width],
               &m->pixels[(size_t)y * m->width],
               (size_t)w * sizeof(uint32_t));
    }
}

CSSC_API void cssc_framebuffer_clear(CsscVal fval, uint32_t color) {
    if (!fval.data.ptr) return;
    CsscFramebuffer* f = (CsscFramebuffer*)fval.data.ptr;
    if ((color & 0xFF000000) == 0) color |= 0xFF000000;
    size_t n = (size_t)f->width * (size_t)f->height;
    for (size_t i = 0; i < n; i++) f->pixels[i] = color;
}

CSSC_API void cssc_framebuffer_set_pixel(CsscVal fval, int64_t x, int64_t y, uint32_t color) {
    if (!fval.data.ptr) return;
    CsscFramebuffer* f = (CsscFramebuffer*)fval.data.ptr;
    if (x < 0 || y < 0 || (uint32_t)x >= f->width || (uint32_t)y >= f->height) return;
    if ((color & 0xFF000000) == 0) color |= 0xFF000000;
    f->pixels[(size_t)y * f->width + (size_t)x] = color;
}

CSSC_API void cssc_video_clear(CsscVal vv, uint32_t color) {
    if (!vv.data.ptr) return;
#ifdef _WIN32
    CsscVideo* v = (CsscVideo*)vv.data.ptr;
    if (!v->impl || !v->impl->backing_pixels) return;
    if ((color & 0xFF000000) == 0) color |= 0xFF000000;
    EnterCriticalSection(&v->impl->cs);
    size_t n = (size_t)v->impl->width * (size_t)v->impl->height;
    for (size_t i = 0; i < n; i++) v->impl->backing_pixels[i] = color;
    LeaveCriticalSection(&v->impl->cs);
#else
    (void)color;
#endif
}

CSSC_API void cssc_video_set_matrix(CsscVal vv, CsscVal src) {
    if (!vv.data.ptr || !src.data.ptr) return;
#ifdef _WIN32
    CsscVideo* v = (CsscVideo*)vv.data.ptr;
    if (!v->impl || !v->impl->backing_pixels) return;
    uint32_t sw = 0, sh = 0;
    uint32_t* spix = NULL;
    /* Accept either framebuffer or matrix */
    if (CSSC_TYPE(src) == CSSC_TYPE_MATRIX) {
        CsscMatrix* m = (CsscMatrix*)src.data.ptr;
        sw = m->width; sh = m->height; spix = m->pixels;
    } else {
        CsscFramebuffer* f = (CsscFramebuffer*)src.data.ptr;
        sw = f->width; sh = f->height; spix = f->pixels;
    }
    if (!spix) return;
    EnterCriticalSection(&v->impl->cs);
    uint32_t w = (sw < v->impl->width) ? sw : v->impl->width;
    uint32_t h = (sh < v->impl->height) ? sh : v->impl->height;
    for (uint32_t y = 0; y < h; y++) {
        memcpy(&v->impl->backing_pixels[(size_t)y * v->impl->width],
               &spix[(size_t)y * sw],
               (size_t)w * sizeof(uint32_t));
    }
    LeaveCriticalSection(&v->impl->cs);
#endif
}

CSSC_API void cssc_video_present(CsscVal vv) {
    if (!vv.data.ptr) return;
#ifdef _WIN32
    CsscVideo* v = (CsscVideo*)vv.data.ptr;
    if (v->impl && v->impl->hwnd) {
        InvalidateRect(v->impl->hwnd, NULL, FALSE);
    }
#endif
}

#ifdef _WIN32
static int _map_weight(int64_t w) {
    switch (w) {
        case 1: return FW_BOLD;
        case 2: return FW_LIGHT;
        case 3: return FW_THIN;
        default: return FW_NORMAL;
    }
}
#endif

CSSC_API void cssc_video_draw_text(CsscVal video, int64_t x, int64_t y,
                                   const char* text, int64_t size_px,
                                   int64_t weight, uint32_t color,
                                   const char* font_family) {
#ifdef _WIN32
    if (!video.data.ptr || !text) return;
    CsscVideo* v = (CsscVideo*)video.data.ptr;
    if (!v->impl || !v->impl->memDC) return;
    const char* fam = (font_family && *font_family) ? font_family : "Segoe UI";
    HFONT f = CreateFontA((int)size_px, 0, 0, 0, _map_weight(weight),
                          FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH, fam);
    EnterCriticalSection(&v->impl->cs);
    HFONT old = (HFONT)SelectObject(v->impl->memDC, f);
    int bk = SetBkMode(v->impl->memDC, TRANSPARENT);
    COLORREF oldC = SetTextColor(v->impl->memDC,
        RGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF));
    TextOutA(v->impl->memDC, (int)x, (int)y, text, (int)strlen(text));
    SetTextColor(v->impl->memDC, oldC);
    SetBkMode(v->impl->memDC, bk);
    SelectObject(v->impl->memDC, old);
    LeaveCriticalSection(&v->impl->cs);
    DeleteObject(f);
#else
    (void)video; (void)x; (void)y; (void)text; (void)size_px; (void)weight;
    (void)color; (void)font_family;
#endif
}

CSSC_API int64_t cssc_video_measure_text(const char* text, int64_t size_px,
                                         int64_t weight, const char* font_family) {
#ifdef _WIN32
    if (!text) return 0;
    const char* fam = (font_family && *font_family) ? font_family : "Segoe UI";
    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);
    HFONT f = CreateFontA((int)size_px, 0, 0, 0, _map_weight(weight),
                          FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH, fam);
    HFONT old = (HFONT)SelectObject(mem, f);
    SIZE sz;
    GetTextExtentPoint32A(mem, text, (int)strlen(text), &sz);
    SelectObject(mem, old);
    DeleteObject(f);
    DeleteDC(mem);
    ReleaseDC(NULL, screen);
    return (int64_t)sz.cx;
#else
    (void)text; (void)size_px; (void)weight; (void)font_family;
    return 0;
#endif
}

CSSC_API void cssc_video_draw_rect(CsscVal video, int64_t x, int64_t y,
                                   int64_t w, int64_t h, uint32_t color) {
#ifdef _WIN32
    if (!video.data.ptr) return;
    CsscVideo* v = (CsscVideo*)video.data.ptr;
    if (!v->impl || !v->impl->memDC) return;
    EnterCriticalSection(&v->impl->cs);
    HBRUSH b = CreateSolidBrush(RGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF));
    RECT r = { (LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h) };
    FillRect(v->impl->memDC, &r, b);
    DeleteObject(b);
    LeaveCriticalSection(&v->impl->cs);
#else
    (void)video; (void)x; (void)y; (void)w; (void)h; (void)color;
#endif
}

CSSC_API void cssc_video_blit_matrix(CsscVal video, CsscVal mval,
                                     int64_t dst_x, int64_t dst_y) {
#ifdef _WIN32
    if (!video.data.ptr || !mval.data.ptr) return;
    CsscVideo* v = (CsscVideo*)video.data.ptr;
    if (!v->impl || !v->impl->backing_pixels) return;
    uint32_t sw = 0, sh = 0;
    uint32_t* spix = NULL;
    if (CSSC_TYPE(mval) == CSSC_TYPE_MATRIX) {
        CsscMatrix* m = (CsscMatrix*)mval.data.ptr;
        sw = m->width; sh = m->height; spix = m->pixels;
    } else {
        CsscFramebuffer* f = (CsscFramebuffer*)mval.data.ptr;
        sw = f->width; sh = f->height; spix = f->pixels;
    }
    if (!spix) return;
    EnterCriticalSection(&v->impl->cs);
    for (uint32_t y = 0; y < sh; y++) {
        int64_t dy = dst_y + y;
        if (dy < 0 || (uint32_t)dy >= v->impl->height) continue;
        for (uint32_t x = 0; x < sw; x++) {
            int64_t dx = dst_x + x;
            if (dx < 0 || (uint32_t)dx >= v->impl->width) continue;
            v->impl->backing_pixels[(size_t)dy * v->impl->width + (size_t)dx] =
                spix[(size_t)y * sw + (size_t)x];
        }
    }
    LeaveCriticalSection(&v->impl->cs);
#else
    (void)video; (void)mval; (void)dst_x; (void)dst_y;
#endif
}

CSSC_API CsscVal cssc_image_load_bmp(const char* path) {
#ifdef _WIN32
    if (!path) return cssc_null();
    HBITMAP hbm = (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0,
                                      LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hbm) return cssc_null();
    BITMAP bmp;
    if (!GetObjectA(hbm, sizeof(bmp), &bmp)) { DeleteObject(hbm); return cssc_null(); }

    int w = bmp.bmWidth;
    int h = bmp.bmHeight;
    CsscVal result = cssc_matrix_create(w, h);
    CsscMatrix* m = (CsscMatrix*)result.data.ptr;

    /* Pull pixels via GetDIBits into 32-bit top-down RGB */
    BITMAPINFO bi;
    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;   /* top-down */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(screen);
    if (!GetDIBits(mem, hbm, 0, h, m->pixels, &bi, DIB_RGB_COLORS)) {
        DeleteDC(mem);
        ReleaseDC(NULL, screen);
        DeleteObject(hbm);
        cssc_matrix_free(result);
        return cssc_null();
    }
    /* Force alpha = 0xFF so it blits solid */
    size_t n = (size_t)w * (size_t)h;
    for (size_t i = 0; i < n; i++) {
        m->pixels[i] = (m->pixels[i] & 0x00FFFFFFu) | 0xFF000000u;
    }
    DeleteDC(mem);
    ReleaseDC(NULL, screen);
    DeleteObject(hbm);
    return result;
#else
    (void)path;
    return cssc_null();
#endif
}

/* =========================================================================
 * 18c. CONSOLE — native Win32 kernel console
 * =========================================================================
 *
 * Allocates (or attaches to) a console, resizes the buffer, and exposes
 * cursor + write primitives directly through WriteConsoleA and friends.
 */

#ifdef _WIN32

/* Convert a CSSC 0xRRGGBB color to a Win32 console text attribute.
 * We pick the nearest of the 16 console colors by comparing RGB channels.
 * Passing 0x0 means "reset to default". */
static WORD _cssc_color_to_attr(uint32_t rgb, WORD def) {
    if (rgb == 0) return def;
    unsigned r = (rgb >> 16) & 0xFF;
    unsigned g = (rgb >>  8) & 0xFF;
    unsigned b = (rgb      ) & 0xFF;

    WORD attr = 0;
    unsigned hi = (r > 170 || g > 170 || b > 170) ? FOREGROUND_INTENSITY : 0;
    if (r >= 128) attr |= FOREGROUND_RED;
    if (g >= 128) attr |= FOREGROUND_GREEN;
    if (b >= 128) attr |= FOREGROUND_BLUE;
    /* If everything was low, fall back to grey so the text stays readable. */
    if (attr == 0) attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    return attr | hi;
}

#endif

CSSC_API CsscVal cssc_console_create(int64_t w, int64_t h) {
    CsscConsole* c = (CsscConsole*)cssc_alloc(sizeof(CsscConsole));
    memset(c, 0, sizeof(*c));
    c->header.refcount = 1;
    c->header.type     = CSSC_TYPE_CONSOLE;
    c->width  = (int)w;
    c->height = (int)h;
    CsscVal v;
    v.tag = CSSC_TYPE_CONSOLE;
    v.data.ptr = c;

#ifdef _WIN32
    /* Attach to an existing console if we already have one (running from a
     * terminal); otherwise allocate a fresh one. */
    if (!GetConsoleWindow()) {
        if (AllocConsole()) {
            c->allocated = true;
        }
    }
    c->hout = (void*)GetStdHandle(STD_OUTPUT_HANDLE);
    c->hin  = (void*)GetStdHandle(STD_INPUT_HANDLE);
    if (c->hout == (void*)INVALID_HANDLE_VALUE) c->hout = NULL;

    CONSOLE_SCREEN_BUFFER_INFO info;
    if (c->hout && GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) {
        c->default_attr = info.wAttributes;
        c->current_attr = info.wAttributes;
    } else {
        c->default_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        c->current_attr = c->default_attr;
    }

    /* Resize screen buffer + window to the requested cells. */
    if (c->hout) {
        COORD sz = { (SHORT)w, (SHORT)h };
        SetConsoleScreenBufferSize((HANDLE)c->hout, sz);
        SMALL_RECT rect = { 0, 0, (SHORT)(w - 1), (SHORT)(h - 1) };
        SetConsoleWindowInfo((HANDLE)c->hout, TRUE, &rect);
    }
#endif
    return v;
}

CSSC_API void cssc_console_free(CsscVal cv) {
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
#ifdef _WIN32
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (c->allocated) {
        FreeConsole();
        c->allocated = false;
    }
    c->hout = NULL;
    c->hin  = NULL;
#endif
}

CSSC_API void cssc_console_out(CsscVal cv, CsscVal text) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return;
    CsscVal s = cssc_to_string_val(text);
    const char* str = cssc_to_cstr(s);
    size_t len = cssc_strlen(s);
    SetConsoleTextAttribute((HANDLE)c->hout, c->current_attr);
    DWORD written = 0;
    WriteConsoleA((HANDLE)c->hout, str, (DWORD)len, &written, NULL);
    cssc_release(s);
#else
    (void)cv; (void)text;
#endif
}

CSSC_API void cssc_console_clear_all(CsscVal cv) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) return;
    DWORD cells = info.dwSize.X * info.dwSize.Y;
    COORD home = {0, 0};
    DWORD n = 0;
    FillConsoleOutputCharacterA((HANDLE)c->hout, ' ', cells, home, &n);
    FillConsoleOutputAttribute((HANDLE)c->hout, c->default_attr, cells, home, &n);
    SetConsoleCursorPosition((HANDLE)c->hout, home);
#else
    (void)cv;
#endif
}

CSSC_API void cssc_console_clear_line(CsscVal cv, int64_t line) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) return;
    COORD start = { 0, (SHORT)line };
    DWORD n = 0;
    FillConsoleOutputCharacterA((HANDLE)c->hout, ' ', info.dwSize.X, start, &n);
    FillConsoleOutputAttribute((HANDLE)c->hout, c->default_attr, info.dwSize.X, start, &n);
#else
    (void)cv; (void)line;
#endif
}

CSSC_API void cssc_console_cursor_col(CsscVal cv, CsscVal color) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    uint32_t rgb = cssc_color_from_val(color);
    c->current_attr = _cssc_color_to_attr(rgb, c->default_attr);
    if (c->hout) SetConsoleTextAttribute((HANDLE)c->hout, c->current_attr);
#else
    (void)cv; (void)color;
#endif
}

CSSC_API void cssc_console_cursor_set(CsscVal cv, int64_t line, int64_t col) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return;
    COORD pos = { (SHORT)col, (SHORT)line };
    SetConsoleCursorPosition((HANDLE)c->hout, pos);
#else
    (void)cv; (void)line; (void)col;
#endif
}

CSSC_API void cssc_console_cursor_pos(CsscVal cv, CsscVal pos_val) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return;
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) return;
    double p;
    if (CSSC_IS_FLOAT(pos_val)) {
        p = pos_val.data.f;
    } else {
        p = (double)cssc_to_int(pos_val);
    }
    int cx = 0;
    if (p <= -1.0) {
        cx = 0;
    } else if (p >= 1.0) {
        cx = info.dwSize.X - 1;
    } else {
        /* map -1..1 -> 0..X-1, but we also accept 0.5 as "middle" */
        if (p < 0.0) p += 1.0;        /* -1..0 -> 0..1 */
        cx = (int)(p * (info.dwSize.X - 1));
        if (cx < 0) cx = 0;
        if (cx >= info.dwSize.X) cx = info.dwSize.X - 1;
    }
    COORD np = { (SHORT)cx, info.dwCursorPosition.Y };
    SetConsoleCursorPosition((HANDLE)c->hout, np);
#else
    (void)cv; (void)pos_val;
#endif
}

CSSC_API CsscVal cssc_console_get_line(CsscVal cv, int64_t line) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return cssc_null();
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return cssc_null();
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) return cssc_null();
    SHORT w = info.dwSize.X;
    char* buf = (char*)cssc_alloc((size_t)w + 1);
    COORD start = { 0, (SHORT)line };
    DWORD got = 0;
    if (!ReadConsoleOutputCharacterA((HANDLE)c->hout, buf, w, start, &got)) {
        cssc_free(buf);
        return cssc_null();
    }
    buf[got] = '\0';
    /* Trim trailing spaces */
    int end = (int)got;
    while (end > 0 && buf[end - 1] == ' ') end--;
    buf[end] = '\0';
    CsscVal r = cssc_string(buf);
    cssc_free(buf);
    return r;
#else
    (void)cv; (void)line;
    return cssc_null();
#endif
}

CSSC_API CsscVal cssc_console_cursor_get_line(CsscVal cv) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return cssc_null();
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return cssc_null();
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) return cssc_null();
    return cssc_console_get_line(cv, info.dwCursorPosition.Y);
#else
    (void)cv;
    return cssc_null();
#endif
}

CSSC_API CsscVal cssc_console_cursor_at(CsscVal cv) {
#ifdef _WIN32
    if (CSSC_TYPE(cv) != CSSC_TYPE_CONSOLE || !cv.data.ptr) return cssc_null();
    CsscConsole* c = (CsscConsole*)cv.data.ptr;
    if (!c->hout) return cssc_null();
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo((HANDLE)c->hout, &info)) return cssc_null();
    /* [line, col] — matches cursor_set(line, col) argument order */
    CsscVal vec = cssc_vector(2);
    cssc_vector_push(vec, cssc_int((int64_t)info.dwCursorPosition.Y));
    cssc_vector_push(vec, cssc_int((int64_t)info.dwCursorPosition.X));
    return vec;
#else
    (void)cv;
    return cssc_null();
#endif
}

/* =========================================================================
 * 18a2. OPENAI — Per-instance OpenAIClient via WinHTTP (Windows-only)
 * =========================================================================
 *
 * Native side of `ai::OpenAIClient(key) MyAI;` in CSSC. Each client is a
 * CsscSector with these string members:
 *
 *   api_key   — Bearer token (mandatory)
 *   model     — default model (e.g. "gpt-4o-mini")
 *   host      — derived from base_url (e.g. "api.openai.com")
 *   path      — derived from base_url (e.g. "/v1")
 *   port      — 443/80
 *   use_https — 1/0
 *   timeout   — milliseconds
 *   system    — optional persistent system prompt (may be empty)
 *
 * No global state. Two scripts in the same process can run independent
 * clients without stepping on each other.
 */

#define CSSC_OPENAI_MAX_KEY    256
#define CSSC_OPENAI_MAX_HOST   256
#define CSSC_OPENAI_MAX_MODEL  128
#define CSSC_OPENAI_MAX_PATH   256

/* --- helpers: read/write client members ---------------------------------- */

static const char* _openai_get_cstr(CsscVal client, const char* member) {
    CsscVal v = cssc_sector_get(client, member);
    const char* s = cssc_to_cstr(v);
    return s ? s : "";
}

CSSC_UNUSED static int64_t _openai_get_int(CsscVal client, const char* member, int64_t fallback) {
    CsscVal v = cssc_sector_get(client, member);
    if (CSSC_TYPE(v) == CSSC_TYPE_INT) return cssc_to_int(v);
    return fallback;
}

/* Look up a string-keyed value in a bind. Returns null if not found. */
static CsscVal _openai_bind_lookup(CsscVal bind, const char* key) {
    if (CSSC_TYPE(bind) != CSSC_TYPE_BIND) return cssc_null();
    int64_t n = cssc_bind_size(bind);
    for (int64_t i = 0; i < n; i++) {
        CsscVal k = cssc_bind_get_key(bind, i);
        const char* ks = cssc_to_cstr(k);
        if (ks && strcmp(ks, key) == 0) {
            return cssc_bind_get_value(bind, i);
        }
    }
    return cssc_null();
}

/* Parse a base-URL string and write derived fields onto the client sector. */
static void _openai_apply_base_url(CsscVal client, const char* url) {
    const char* p = url ? url : "https://api.openai.com/v1";
    int use_https = 1, port = 443;
    if (strncmp(p, "https://", 8) == 0) { p += 8; use_https = 1; port = 443; }
    else if (strncmp(p, "http://", 7) == 0) { p += 7; use_https = 0; port = 80; }
    const char* slash = strchr(p, '/');
    const char* colon = strchr(p, ':');
    const char* host_end = slash ? slash : (p + strlen(p));
    char host[CSSC_OPENAI_MAX_HOST] = {0};
    char path[CSSC_OPENAI_MAX_PATH] = {0};
    if (colon && colon < host_end) {
        size_t hlen = (size_t)(colon - p);
        if (hlen >= CSSC_OPENAI_MAX_HOST) hlen = CSSC_OPENAI_MAX_HOST - 1;
        memcpy(host, p, hlen);
        host[hlen] = 0;
        port = atoi(colon + 1);
    } else {
        size_t hlen = (size_t)(host_end - p);
        if (hlen >= CSSC_OPENAI_MAX_HOST) hlen = CSSC_OPENAI_MAX_HOST - 1;
        memcpy(host, p, hlen);
        host[hlen] = 0;
    }
    if (slash) {
        strncpy(path, slash, CSSC_OPENAI_MAX_PATH - 1);
        path[CSSC_OPENAI_MAX_PATH - 1] = 0;
        size_t pl = strlen(path);
        if (pl > 1 && path[pl - 1] == '/') path[pl - 1] = 0;
    }
    cssc_sector_set(client, "host", cssc_string(host));
    cssc_sector_set(client, "path", cssc_string(path));
    cssc_sector_set(client, "port", cssc_int(port));
    cssc_sector_set(client, "use_https", cssc_int(use_https));
}

/* JSON-escape into dst (returns bytes written, no terminator). */
static size_t _openai_json_escape(char* dst, size_t cap, const char* src) {
    size_t i = 0;
    if (!src) return 0;
    for (; *src && i + 8 < cap; src++) {
        unsigned char c = (unsigned char)*src;
        if (c == '"' || c == '\\') { dst[i++] = '\\'; dst[i++] = c; }
        else if (c == '\n') { dst[i++] = '\\'; dst[i++] = 'n'; }
        else if (c == '\r') { dst[i++] = '\\'; dst[i++] = 'r'; }
        else if (c == '\t') { dst[i++] = '\\'; dst[i++] = 't'; }
        else if (c < 0x20) { i += (size_t)snprintf(dst + i, cap - i, "\\u%04x", c); }
        else { dst[i++] = (char)c; }
    }
    return i;
}

/* Find the first occurrence of `"content":"..."` and decode JSON-escapes
 * into a freshly cssc_alloc'd buffer. Caller owns the returned pointer. */
CSSC_UNUSED static char* _openai_extract_content(const char* json_response) {
    if (!json_response) return NULL;
    const char* needle = "\"content\":";
    const char* p = strstr(json_response, needle);
    if (!p) return NULL;
    p += strlen(needle);
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    if (*p != '"') return NULL;
    p++;
    /* Allocate generously — escaped form is always >= decoded form. */
    size_t cap = strlen(p) + 1;
    char* out = (char*)cssc_alloc(cap);
    size_t oi = 0;
    while (*p && *p != '"') {
        if (*p == '\\' && p[1]) {
            char esc = p[1];
            if      (esc == '"' || esc == '\\' || esc == '/') { out[oi++] = esc; p += 2; }
            else if (esc == 'n') { out[oi++] = '\n'; p += 2; }
            else if (esc == 'r') { out[oi++] = '\r'; p += 2; }
            else if (esc == 't') { out[oi++] = '\t'; p += 2; }
            else if (esc == 'b') { out[oi++] = '\b'; p += 2; }
            else if (esc == 'f') { out[oi++] = '\f'; p += 2; }
            else if (esc == 'u' && p[2] && p[3] && p[4] && p[5]) {
                /* parse 4-hex codepoint, encode as UTF-8 */
                char hex[5] = { p[2], p[3], p[4], p[5], 0 };
                unsigned cp = (unsigned)strtoul(hex, NULL, 16);
                if (cp < 0x80) {
                    out[oi++] = (char)cp;
                } else if (cp < 0x800) {
                    out[oi++] = (char)(0xC0 | (cp >> 6));
                    out[oi++] = (char)(0x80 | (cp & 0x3F));
                } else {
                    out[oi++] = (char)(0xE0 | (cp >> 12));
                    out[oi++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    out[oi++] = (char)(0x80 | (cp & 0x3F));
                }
                p += 6;
            }
            else { out[oi++] = esc; p += 2; }
        } else {
            out[oi++] = *p++;
        }
    }
    out[oi] = 0;
    return out;
}

#ifdef _WIN32

/* POST a JSON body using settings from the given client sector. Returns the
 * cssc_alloc'd, null-terminated response body, or NULL on error (with
 * `err_out` populated). */
static char* _openai_winhttp_post(CsscVal client, const char* endpoint,
                                   const char* json_body,
                                   char* err_out, size_t err_cap) {
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    char* result = NULL;
    wchar_t whost[CSSC_OPENAI_MAX_HOST];
    wchar_t wfullpath[1024];
    wchar_t wauth[CSSC_OPENAI_MAX_KEY + 64];

    const char* host = _openai_get_cstr(client, "host");
    const char* base_path = _openai_get_cstr(client, "path");
    const char* api_key = _openai_get_cstr(client, "api_key");
    int64_t port = _openai_get_int(client, "port", 443);
    int64_t timeout_ms = _openai_get_int(client, "timeout", 60000);
    int64_t use_https = _openai_get_int(client, "use_https", 1);

    if (!api_key[0]) {
        snprintf(err_out, err_cap, "no api_key on client");
        return NULL;
    }
    if (!host[0]) host = "api.openai.com";

    if (MultiByteToWideChar(CP_UTF8, 0, host, -1, whost, CSSC_OPENAI_MAX_HOST) == 0) {
        snprintf(err_out, err_cap, "host conversion failed");
        return NULL;
    }
    char fullpath[CSSC_OPENAI_MAX_PATH + 64];
    snprintf(fullpath, sizeof(fullpath), "%s%s", base_path, endpoint);
    if (MultiByteToWideChar(CP_UTF8, 0, fullpath, -1, wfullpath, 1024) == 0) {
        snprintf(err_out, err_cap, "path conversion failed");
        return NULL;
    }
    char authbuf[CSSC_OPENAI_MAX_KEY + 64];
    snprintf(authbuf, sizeof(authbuf), "Authorization: Bearer %s", api_key);
    if (MultiByteToWideChar(CP_UTF8, 0, authbuf, -1, wauth, CSSC_OPENAI_MAX_KEY + 64) == 0) {
        snprintf(err_out, err_cap, "auth header conversion failed");
        return NULL;
    }

    hSession = WinHttpOpen(L"cssc-openai/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { snprintf(err_out, err_cap, "WinHttpOpen failed: %lu", GetLastError()); goto done; }

    WinHttpSetTimeouts(hSession, (int)timeout_ms, (int)timeout_ms,
                       (int)timeout_ms, (int)timeout_ms);

    hConnect = WinHttpConnect(hSession, whost, (INTERNET_PORT)port, 0);
    if (!hConnect) { snprintf(err_out, err_cap, "WinHttpConnect failed: %lu", GetLastError()); goto done; }

    DWORD reqFlags = use_https ? WINHTTP_FLAG_SECURE : 0;
    hRequest = WinHttpOpenRequest(hConnect, L"POST", wfullpath, NULL,
                                  WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, reqFlags);
    if (!hRequest) { snprintf(err_out, err_cap, "WinHttpOpenRequest failed: %lu", GetLastError()); goto done; }

    static const wchar_t* ctype_hdr = L"Content-Type: application/json\r\n";
    if (!WinHttpAddRequestHeaders(hRequest, ctype_hdr, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
        snprintf(err_out, err_cap, "set content-type failed: %lu", GetLastError()); goto done;
    }
    if (!WinHttpAddRequestHeaders(hRequest, wauth, (DWORD)-1L, WINHTTP_ADDREQ_FLAG_ADD)) {
        snprintf(err_out, err_cap, "set auth failed: %lu", GetLastError()); goto done;
    }

    DWORD body_len = (DWORD)strlen(json_body);
    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            (LPVOID)json_body, body_len, body_len, 0)) {
        snprintf(err_out, err_cap, "WinHttpSendRequest failed: %lu", GetLastError()); goto done;
    }
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        snprintf(err_out, err_cap, "WinHttpReceiveResponse failed: %lu", GetLastError()); goto done;
    }

    size_t total = 0, cap_buf = 4096;
    char* buf = (char*)cssc_alloc(cap_buf);
    DWORD avail = 0;
    do {
        avail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &avail)) {
            snprintf(err_out, err_cap, "WinHttpQueryDataAvailable failed: %lu", GetLastError());
            cssc_free(buf); goto done;
        }
        if (avail == 0) break;
        if (total + avail + 1 > cap_buf) {
            while (total + avail + 1 > cap_buf) cap_buf *= 2;
            char* grow = (char*)cssc_alloc(cap_buf);
            memcpy(grow, buf, total);
            cssc_free(buf);
            buf = grow;
        }
        DWORD got = 0;
        if (!WinHttpReadData(hRequest, buf + total, avail, &got)) {
            snprintf(err_out, err_cap, "WinHttpReadData failed: %lu", GetLastError());
            cssc_free(buf); goto done;
        }
        total += got;
    } while (avail > 0);
    buf[total] = 0;
    result = buf;

done:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return result;
}

#endif  /* _WIN32 */

/* ----------------------------------------------------------------------- */
/* Public per-instance API                                                  */
/* ----------------------------------------------------------------------- */

CSSC_API CsscVal cssc_openai_client_create(CsscVal api_key) {
    CsscVal client = cssc_sector_create("OpenAIClient");
    /* api_key: caller-provided, or fall back to env */
    const char* k = cssc_to_cstr(api_key);
    if (!k || !k[0]) {
        const char* env = getenv("OPENAI_API_KEY");
        k = env ? env : "";
    }
    cssc_sector_set(client, "api_key", cssc_string(k));
    cssc_sector_set(client, "model",   cssc_string("gpt-4o-mini"));
    cssc_sector_set(client, "system",  cssc_string(""));
    cssc_sector_set(client, "timeout", cssc_int(60000));
    _openai_apply_base_url(client, "https://api.openai.com/v1");
    return client;
}

CSSC_API CsscVal cssc_openai_client_set_model(CsscVal client, CsscVal model) {
    const char* s = cssc_to_cstr(model);
    cssc_sector_set(client, "model", cssc_string(s ? s : "gpt-4o-mini"));
    return cssc_null();
}

CSSC_API CsscVal cssc_openai_client_set_system(CsscVal client, CsscVal text) {
    const char* s = cssc_to_cstr(text);
    cssc_sector_set(client, "system", cssc_string(s ? s : ""));
    return cssc_null();
}

CSSC_API CsscVal cssc_openai_client_set_base_url(CsscVal client, CsscVal url) {
    const char* s = cssc_to_cstr(url);
    _openai_apply_base_url(client, s);
    return cssc_null();
}

CSSC_API CsscVal cssc_openai_client_set_timeout(CsscVal client, CsscVal secs) {
    int64_t ms = 60000;
    if (CSSC_TYPE(secs) == CSSC_TYPE_FLOAT) ms = (int64_t)(secs.data.f * 1000.0);
    else if (CSSC_TYPE(secs) == CSSC_TYPE_INT) ms = secs.data.i * 1000;
    cssc_sector_set(client, "timeout", cssc_int(ms));
    return cssc_null();
}

CSSC_API CsscVal cssc_openai_client_model(CsscVal client) {
    return cssc_sector_get(client, "model");
}

CSSC_API CsscVal cssc_openai_client_has_key(CsscVal client) {
    const char* k = _openai_get_cstr(client, "api_key");
    return cssc_bool(k && k[0]);
}

CSSC_API CsscVal cssc_openai_client_chat(CsscVal client, CsscVal prompt) {
    const char* model = _openai_get_cstr(client, "model");
    const char* system = _openai_get_cstr(client, "system");
    const char* prompt_str = cssc_to_cstr(prompt);
    if (!prompt_str) prompt_str = "";
    if (!model[0]) model = "gpt-4o-mini";

    /* Build JSON body — optionally include a system message. */
    size_t plen = strlen(prompt_str);
    size_t slen = strlen(system);
    size_t body_cap = (plen + slen) * 6 + 1024;
    char* body = (char*)cssc_alloc(body_cap);
    size_t off = (size_t)snprintf(body, body_cap,
        "{\"model\":\"%s\",\"messages\":[", model);
    if (system[0]) {
        off += (size_t)snprintf(body + off, body_cap - off,
            "{\"role\":\"system\",\"content\":\"");
        off += _openai_json_escape(body + off, body_cap - off - 16, system);
        off += (size_t)snprintf(body + off, body_cap - off, "\"},");
    }
    off += (size_t)snprintf(body + off, body_cap - off,
        "{\"role\":\"user\",\"content\":\"");
    off += _openai_json_escape(body + off, body_cap - off - 16, prompt_str);
    snprintf(body + off, body_cap - off, "\"}]}");

#ifdef _WIN32
    char err[256] = {0};
    char* response = _openai_winhttp_post(client, "/chat/completions", body, err, sizeof(err));
    cssc_free(body);
    if (!response) {
        char errmsg[320];
        snprintf(errmsg, sizeof(errmsg), "[openai http error: %s]", err);
        return cssc_string(errmsg);
    }
    char* content = _openai_extract_content(response);
    if (!content) {
        char errmsg[1024];
        snprintf(errmsg, sizeof(errmsg), "[openai parse error — raw: %.900s]", response);
        cssc_free(response);
        return cssc_string(errmsg);
    }
    cssc_free(response);
    CsscVal result = cssc_string(content);
    cssc_free(content);
    return result;
#else
    cssc_free(body);
    return cssc_string("[openai: native HTTP only on Windows; use the interpreter]");
#endif
}

CSSC_API CsscVal cssc_openai_client_completion(CsscVal client, CsscVal messages_vec) {
    /* messages_vec is a vector of binds {role, content}. Render to JSON
     * messages array, POST, return content. */
    const char* model = _openai_get_cstr(client, "model");
    if (!model[0]) model = "gpt-4o-mini";

    /* Walk vector, accumulate JSON. We grow body buffer as needed. */
    size_t cap = 4096;
    char* body = (char*)cssc_alloc(cap);
    size_t off = (size_t)snprintf(body, cap, "{\"model\":\"%s\",\"messages\":[", model);

    int64_t n = 0;
    if (CSSC_TYPE(messages_vec) == CSSC_TYPE_VECTOR) {
        n = cssc_vector_size(messages_vec);
    }
    for (int64_t i = 0; i < n; i++) {
        CsscVal msg = cssc_vector_get(messages_vec, i);
        const char* role = "user";
        const char* content = "";
        char* role_buf = NULL;
        char* content_buf = NULL;
        if (CSSC_TYPE(msg) == CSSC_TYPE_BIND) {
            CsscVal r = _openai_bind_lookup(msg, "role");
            CsscVal c = _openai_bind_lookup(msg, "content");
            role_buf = (char*)cssc_to_cstr(r);
            content_buf = (char*)cssc_to_cstr(c);
            if (role_buf && role_buf[0]) role = role_buf;
            if (content_buf) content = content_buf;
        } else if (CSSC_TYPE(msg) == CSSC_TYPE_STRING) {
            content_buf = (char*)cssc_to_cstr(msg);
            if (content_buf) content = content_buf;
        }
        size_t need = off + strlen(role) + strlen(content) * 6 + 64;
        if (need >= cap) {
            while (need >= cap) cap *= 2;
            char* grow = (char*)cssc_alloc(cap);
            memcpy(grow, body, off);
            cssc_free(body);
            body = grow;
        }
        if (i > 0) body[off++] = ',';
        off += (size_t)snprintf(body + off, cap - off,
            "{\"role\":\"%s\",\"content\":\"", role);
        off += _openai_json_escape(body + off, cap - off - 16, content);
        off += (size_t)snprintf(body + off, cap - off, "\"}");
    }
    off += (size_t)snprintf(body + off, cap - off, "]}");

#ifdef _WIN32
    char err[256] = {0};
    char* response = _openai_winhttp_post(client, "/chat/completions", body, err, sizeof(err));
    cssc_free(body);
    if (!response) {
        char errmsg[320];
        snprintf(errmsg, sizeof(errmsg), "[openai http error: %s]", err);
        return cssc_string(errmsg);
    }
    char* content = _openai_extract_content(response);
    if (!content) {
        char errmsg[1024];
        snprintf(errmsg, sizeof(errmsg), "[openai parse error — raw: %.900s]", response);
        cssc_free(response);
        return cssc_string(errmsg);
    }
    cssc_free(response);
    CsscVal result = cssc_string(content);
    cssc_free(content);
    return result;
#else
    cssc_free(body);
    return cssc_string("[openai: native HTTP only on Windows; use the interpreter]");
#endif
}

/* =========================================================================
 * 18b. DAEMONS — Thread-based function execution
 * ========================================================================= */

#ifndef MAX_DAEMONS
#define MAX_DAEMONS 32
#endif

typedef struct {
    const char* name;                 /* interned function name */
#ifdef _WIN32
    HANDLE thread;
#endif
    volatile int should_stop;
    CsscVal func_val;
    CsscScopeStack* scope;
    bool active;
} CsscDaemonSlot;

#ifdef _WIN32
static CsscDaemonSlot g_daemons[MAX_DAEMONS];
static CRITICAL_SECTION g_daemons_cs;
static bool g_daemons_cs_initialized = false;
#endif

static void _cssc_daemons_init(void) {
#ifdef _WIN32
    if (!g_daemons_cs_initialized) {
        InitializeCriticalSection(&g_daemons_cs);
        g_daemons_cs_initialized = true;
    }
#endif
}

#ifdef _WIN32
/* Daemon worker — loops the registered function with a 50ms cadence
 * until #killdaemon[] flips should_stop. Three consecutive thrown errors
 * (well, native panics surface as thread exits) end the daemon to keep
 * the spew bounded. Mirrors the interpreter-side semantics in
 * cssl_asyncthreads.py so the same script behaves identically under
 * `cssc run` and `cssc build`. */
static DWORD WINAPI _cssc_daemon_thread_entry(LPVOID param) {
    CsscDaemonSlot* slot = (CsscDaemonSlot*)param;
    if (CSSC_TYPE(slot->func_val) != CSSC_TYPE_FUNCTION || !slot->func_val.data.ptr) {
        return 0;
    }
    CsscFunction* fn = (CsscFunction*)slot->func_val.data.ptr;
    if (!fn->body_fn) {
        return 0;
    }
    typedef CsscVal (*BodyFn)(CsscScopeStack*, CsscVal*, uint32_t);
    BodyFn bf = (BodyFn)fn->body_fn;
    while (!slot->should_stop) {
        bf(slot->scope, NULL, 0);
        /* Cooperative sleep — gives Sleep(50) a chance to be interrupted
         * by should_stop check. Tighter cadences cause Win32 thread
         * thrash without measurable scheduling benefit for tick-style
         * handlers. */
        if (slot->should_stop) break;
        Sleep(50);
    }
    return 0;
}
#endif

CSSC_API CsscVal cssc_daemon_start(CsscScopeStack* scope, const char* func_name) {
    _cssc_daemons_init();
#ifdef _WIN32
    /* Look up the function from scope */
    CsscVal func_val = cssc_scope_get(scope, func_name);
    if (CSSC_TYPE(func_val) != CSSC_TYPE_FUNCTION) {
        /* Maybe it's Obj->member — try to resolve */
        const char* arrow = strstr(func_name, "->");
        if (arrow) {
            size_t olen = (size_t)(arrow - func_name);
            char buf[256];
            if (olen >= sizeof(buf)) olen = sizeof(buf) - 1;
            memcpy(buf, func_name, olen);
            buf[olen] = '\0';
            CsscVal obj = cssc_scope_get(scope, buf);
            if (!CSSC_IS_NULL(obj)) {
                func_val = cssc_sector_get(obj, arrow + 2);
            }
        }
    }
    if (CSSC_TYPE(func_val) != CSSC_TYPE_FUNCTION) {
        return cssc_null();
    }

    EnterCriticalSection(&g_daemons_cs);
    CsscDaemonSlot* slot = NULL;
    for (int i = 0; i < MAX_DAEMONS; i++) {
        if (!g_daemons[i].active) {
            slot = &g_daemons[i];
            break;
        }
    }
    if (!slot) {
        LeaveCriticalSection(&g_daemons_cs);
        return cssc_null();
    }
    slot->name = cssc_intern(func_name);
    slot->should_stop = 0;
    slot->func_val = func_val;
    slot->scope = scope;
    slot->active = true;
    slot->thread = CreateThread(NULL, 0, _cssc_daemon_thread_entry, slot, 0, NULL);
    LeaveCriticalSection(&g_daemons_cs);

    if (!slot->thread) {
        slot->active = false;
        return cssc_null();
    }
    CsscVal r;
    r.tag = CSSC_TYPE_INT;
    r.data.i = (int64_t)(uintptr_t)slot;
    return r;
#else
    (void)scope; (void)func_name;
    return cssc_null();
#endif
}

CSSC_API void cssc_daemon_kill(const char* func_name) {
    _cssc_daemons_init();
#ifdef _WIN32
    const char* interned = cssc_intern(func_name);
    EnterCriticalSection(&g_daemons_cs);
    for (int i = 0; i < MAX_DAEMONS; i++) {
        if (g_daemons[i].active && g_daemons[i].name == interned) {
            g_daemons[i].should_stop = 1;
            /* Don't hard-terminate — let the daemon's own loop check should_stop */
        }
    }
    LeaveCriticalSection(&g_daemons_cs);
#endif
}

CSSC_API bool cssc_daemon_should_stop(const char* func_name) {
#ifdef _WIN32
    if (!g_daemons_cs_initialized) return false;
    const char* interned = cssc_intern(func_name);
    for (int i = 0; i < MAX_DAEMONS; i++) {
        if (g_daemons[i].active && g_daemons[i].name == interned) {
            return g_daemons[i].should_stop != 0;
        }
    }
#endif
    return false;
}

CSSC_API void cssc_daemon_shutdown_all(void) {
#ifdef _WIN32
    if (!g_daemons_cs_initialized) return;
    EnterCriticalSection(&g_daemons_cs);
    for (int i = 0; i < MAX_DAEMONS; i++) {
        if (g_daemons[i].active) {
            g_daemons[i].should_stop = 1;
        }
    }
    /* Wait briefly for threads to exit */
    HANDLE handles[MAX_DAEMONS];
    int n = 0;
    for (int i = 0; i < MAX_DAEMONS; i++) {
        if (g_daemons[i].thread) handles[n++] = g_daemons[i].thread;
    }
    LeaveCriticalSection(&g_daemons_cs);
    if (n > 0) {
        WaitForMultipleObjects((DWORD)n, handles, TRUE, 2000);
    }
    for (int i = 0; i < MAX_DAEMONS; i++) {
        if (g_daemons[i].thread) {
            CloseHandle(g_daemons[i].thread);
            g_daemons[i].thread = NULL;
        }
        g_daemons[i].active = false;
    }
#endif
}

/* =========================================================================
 * 19a. ISOLATED .obj LOADER
 * ========================================================================= */

struct CsscObjHandleImpl {
#ifdef _WIN32
    HMODULE dll_handle;
#else
    void* dll_handle;
#endif
    char temp_dll_path[512];
};

/* Decode the on-disk .obj format and extract `main.dll` to a temp file.
 * Returns a malloc'd path string on success, NULL on failure. */
static char* _cssc_obj_extract_main_dll(const char* obj_path, const char* alias) {
    FILE* f = fopen(obj_path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12) { fclose(f); return NULL; }
    unsigned char* buf = (unsigned char*)cssc_alloc((size_t)sz);
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        cssc_free(buf);
        return NULL;
    }
    fclose(f);

    size_t off = 0;
    if (off + 4 > (size_t)sz) { cssc_free(buf); return NULL; }
    uint32_t version = *(uint32_t*)(buf + off); off += 4;
    if (version != 1) { cssc_free(buf); return NULL; }
    if (off + 4 > (size_t)sz) { cssc_free(buf); return NULL; }
    uint32_t name_len = *(uint32_t*)(buf + off); off += 4;
    if (off + name_len > (size_t)sz) { cssc_free(buf); return NULL; }
    off += name_len;
    if (off + 4 > (size_t)sz) { cssc_free(buf); return NULL; }
    uint32_t num_entries = *(uint32_t*)(buf + off); off += 4;

    char* out_path = NULL;
    for (uint32_t i = 0; i < num_entries; i++) {
        if (off + 4 > (size_t)sz) break;
        uint32_t enl = *(uint32_t*)(buf + off); off += 4;
        if (off + enl > (size_t)sz) break;
        char ename[512];
        size_t cn = enl < 511 ? enl : 511;
        memcpy(ename, buf + off, cn); ename[cn] = '\0';
        off += enl;
        if (off + 8 > (size_t)sz) break;
        uint64_t dlen = *(uint64_t*)(buf + off); off += 8;
        if (off + dlen > (size_t)sz) break;

        if (strcmp(ename, "main.dll") == 0) {
            char tmp_dir[MAX_PATH];
#ifdef _WIN32
            GetTempPathA(MAX_PATH, tmp_dir);
#else
            strcpy(tmp_dir, "/tmp/");
#endif
            char tmp_path[MAX_PATH * 2];   /* tmp_dir + 'cssc_obj_<alias>_<pid>.dll' */
            snprintf(tmp_path, sizeof(tmp_path), "%scssc_obj_%s_%u.dll",
                     tmp_dir, alias ? alias : "x", (unsigned)GetCurrentProcessId());
            FILE* of = fopen(tmp_path, "wb");
            if (of) {
                fwrite(buf + off, 1, (size_t)dlen, of);
                fclose(of);
                out_path = (char*)cssc_alloc(strlen(tmp_path) + 1);
                strcpy(out_path, tmp_path);
            }
            break;
        }
        off += dlen;
    }
    cssc_free(buf);
    return out_path;
}

/* Try the literal path first. If that fails AND the path is "absolute-like"
 * (starts with '/'), retry relative to the exe's directory and the current
 * working directory. Finally, fall back to %APPDATA%/cssc/objects/<name>.obj. */
static char* _cssc_obj_find_path(const char* obj_path) {
    if (!obj_path) return NULL;
    FILE* probe = fopen(obj_path, "rb");
    if (probe) {
        fclose(probe);
        char* p = (char*)cssc_alloc(strlen(obj_path) + 1);
        strcpy(p, obj_path);
        return p;
    }

#ifdef _WIN32
    char buf[MAX_PATH * 2];
    /* 1. Relative to the exe directory, stripping any leading slashes from obj_path */
    char exe_dir[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_dir, MAX_PATH) > 0) {
        char* last = strrchr(exe_dir, '\\');
        if (!last) last = strrchr(exe_dir, '/');
        if (last) *last = '\0';
        const char* rel = obj_path;
        while (*rel == '/' || *rel == '\\') rel++;
        snprintf(buf, sizeof(buf), "%s\\%s", exe_dir, rel);
        probe = fopen(buf, "rb");
        if (probe) {
            fclose(probe);
            char* p = (char*)cssc_alloc(strlen(buf) + 1);
            strcpy(p, buf);
            return p;
        }
    }

    /* 2. Relative to CWD, stripping leading slashes */
    char cwd[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
        const char* rel = obj_path;
        while (*rel == '/' || *rel == '\\') rel++;
        snprintf(buf, sizeof(buf), "%s\\%s", cwd, rel);
        probe = fopen(buf, "rb");
        if (probe) {
            fclose(probe);
            char* p = (char*)cssc_alloc(strlen(buf) + 1);
            strcpy(p, buf);
            return p;
        }
    }

    /* 3. %APPDATA%/CSSC/<version>/objects/<basename>.obj — version-separated
     *    (if not already .obj-suffixed, try both forms). CSSC_VERSION_DIR is
     *    baked at compile time by the CSSC toolchain (defaults to the current
     *    version) so a program built by CSSC 6 only finds c6/objects. */
#ifndef CSSC_VERSION_DIR
#define CSSC_VERSION_DIR "c7"
#endif
    const char* appdata = getenv("APPDATA");
    if (appdata) {
        const char* base = strrchr(obj_path, '/');
        if (!base) base = strrchr(obj_path, '\\');
        base = base ? base + 1 : obj_path;
        snprintf(buf, sizeof(buf), "%s\\CSSC\\" CSSC_VERSION_DIR "\\objects\\%s", appdata, base);
        probe = fopen(buf, "rb");
        if (probe) {
            fclose(probe);
            char* p = (char*)cssc_alloc(strlen(buf) + 1);
            strcpy(p, buf);
            return p;
        }
        /* Try adding .obj */
        snprintf(buf, sizeof(buf), "%s\\CSSC\\" CSSC_VERSION_DIR "\\objects\\%s.obj", appdata, base);
        probe = fopen(buf, "rb");
        if (probe) {
            fclose(probe);
            char* p = (char*)cssc_alloc(strlen(buf) + 1);
            strcpy(p, buf);
            return p;
        }
    }
#endif
    return NULL;
}

CSSC_API CsscVal cssc_obj_load(CsscScopeStack* host_scope, const char* obj_path, const char* alias) {
    if (!obj_path || !alias) return cssc_null();

    char* resolved_path = _cssc_obj_find_path(obj_path);
    if (!resolved_path) {
        cssc_panicf("#depend: could not locate .obj '%s' (tried literal path, exe-dir, cwd, %%APPDATA%%/cssc/objects/)", obj_path);
        return cssc_null();
    }
    char* dll_path = _cssc_obj_extract_main_dll(resolved_path, alias);
    if (!dll_path) {
        cssc_panicf("#depend: failed to read '%s' or missing main.dll", resolved_path);
        cssc_free(resolved_path);
        return cssc_null();
    }
    cssc_free(resolved_path);

#ifdef _WIN32
    HMODULE h = LoadLibraryA(dll_path);
    if (!h) {
        cssc_panicf("#depend: LoadLibrary failed for '%s' (extracted to '%s')", obj_path, dll_path);
        cssc_free(dll_path);
        return cssc_null();
    }
    typedef CsscVal (*ModuleInitFn)(CsscScopeStack*);
    ModuleInitFn init_fn = (ModuleInitFn)GetProcAddress(h, "cssc_module_init");
    CsscVal module_sector;
    if (init_fn) {
        /* ISOLATION: pass the host's scope so the .obj's internal code can
         * set up its own sector hierarchy on the host's scope — but the .obj
         * keeps its own cssc_global_scope() if its generated code did NOT
         * call cssc_global_scope_set(). To keep host state intact, modules
         * built as .obj intentionally DO call cssc_global_scope_set so their
         * internal objects/labels share the host scope. */
        module_sector = init_fn(host_scope);
        if (CSSC_IS_NULL(module_sector)) {
            module_sector = cssc_sector_create(alias);
        }
    } else {
        module_sector = cssc_sector_create(alias);
    }
    cssc_scope_set(host_scope, alias, module_sector);

    /* Create a small handle structure so the caller can unload later if needed */
    CsscObjHandle* handle = (CsscObjHandle*)cssc_alloc(sizeof(CsscObjHandle));
    memset(handle, 0, sizeof(*handle));
    handle->header.refcount = 1;
    handle->header.type = CSSC_TYPE_MODULE;
    handle->alias = cssc_intern(alias);
    handle->project_name = cssc_intern(alias);  /* simplification: we don't parse manifest here */
    handle->module_sector = module_sector;
    handle->impl = (CsscObjHandleImpl*)cssc_alloc(sizeof(CsscObjHandleImpl));
    memset(handle->impl, 0, sizeof(CsscObjHandleImpl));
    handle->impl->dll_handle = h;
    strncpy(handle->impl->temp_dll_path, dll_path, sizeof(handle->impl->temp_dll_path) - 1);

    cssc_free(dll_path);
    return module_sector;
#else
    cssc_panicf("#depend: not supported on this platform yet");
    cssc_free(dll_path);
    return cssc_null();
#endif
}

CSSC_API void cssc_obj_unload(CsscVal handle_val) {
    if (!handle_val.data.ptr) return;
#ifdef _WIN32
    CsscObjHandle* h = (CsscObjHandle*)handle_val.data.ptr;
    if (h->impl) {
        if (h->impl->dll_handle) {
            FreeLibrary(h->impl->dll_handle);
            h->impl->dll_handle = NULL;
        }
        if (h->impl->temp_dll_path[0]) {
            DeleteFileA(h->impl->temp_dll_path);
        }
        cssc_free(h->impl);
        h->impl = NULL;
    }
#endif
}

/* =========================================================================
 * 19f. .cobj loader — native-code library with host-owned lifecycle
 * ========================================================================= */

#ifdef _WIN32

/* Registry of loaded .cobj DLLs. When runtime shuts down we iterate this
 * list in reverse insertion order and call cssc_cobj_cleanup on each. */
typedef struct {
    HMODULE  dll;
    void*    cleanup_fn;   /* resolved cssc_cobj_cleanup */
    char     tmp_path[512];
} _CobjEntry;

#ifndef MAX_COBJ_LOADED
#define MAX_COBJ_LOADED 32
#endif
static _CobjEntry g_cobjs[MAX_COBJ_LOADED];
static int        g_cobjs_count = 0;

static void _cssc_cobj_run_all_cleanups(CsscScopeStack* scope) {
    for (int i = g_cobjs_count - 1; i >= 0; i--) {
        if (g_cobjs[i].cleanup_fn) {
            typedef void (*CleanupFn)(CsscScopeStack*);
            CleanupFn fn = (CleanupFn)g_cobjs[i].cleanup_fn;
            fn(scope);
        }
        if (g_cobjs[i].dll) {
            FreeLibrary(g_cobjs[i].dll);
            g_cobjs[i].dll = NULL;
        }
        if (g_cobjs[i].tmp_path[0]) {
            DeleteFileA(g_cobjs[i].tmp_path);
        }
    }
    g_cobjs_count = 0;
}

/* Parse a .cobj header and extract its embedded DLL to a temp file.
 * .cobj layout: "COBJ" | u32 ver | u32 flags | u32 name_len | name |
 *               u64 dll_size | dll bytes. */
static char* _cssc_cobj_extract_dll(const char* cobj_path, const char* alias) {
    FILE* f = fopen(cobj_path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 24) { fclose(f); return NULL; }
    unsigned char* buf = (unsigned char*)cssc_alloc((size_t)sz);
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        cssc_free(buf);
        return NULL;
    }
    fclose(f);

    if (memcmp(buf, "COBJ", 4) != 0) { cssc_free(buf); return NULL; }
    size_t off = 4;
    uint32_t version = *(uint32_t*)(buf + off); off += 4;
    if (version != 1) { cssc_free(buf); return NULL; }
    off += 4;   /* flags */
    uint32_t name_len = *(uint32_t*)(buf + off); off += 4;
    if (off + name_len + 8 > (size_t)sz) { cssc_free(buf); return NULL; }
    off += name_len;
    uint64_t dll_size = *(uint64_t*)(buf + off); off += 8;
    if (off + dll_size > (size_t)sz) { cssc_free(buf); return NULL; }

    char tmp_dir[MAX_PATH];
    GetTempPathA(MAX_PATH, tmp_dir);
    char tmp_path[MAX_PATH * 2];   /* tmp_dir + 'cssc_cobj_<alias>_<pid>.dll' */
    snprintf(tmp_path, sizeof(tmp_path), "%scssc_cobj_%s_%u.dll",
             tmp_dir, alias ? alias : "x", (unsigned)GetCurrentProcessId());
    FILE* of = fopen(tmp_path, "wb");
    if (!of) { cssc_free(buf); return NULL; }
    fwrite(buf + off, 1, (size_t)dll_size, of);
    fclose(of);

    cssc_free(buf);
    char* p = (char*)cssc_alloc(strlen(tmp_path) + 1);
    strcpy(p, tmp_path);
    return p;
}

#endif

CSSC_API CsscVal cssc_cobj_load(CsscScopeStack* host_scope,
                                const char* cobj_path, const char* alias) {
#ifdef _WIN32
    if (!cobj_path || !alias) return cssc_null();

    /* Reuse the .obj path-resolution (literal → exe dir → cwd → %APPDATA%) */
    char* resolved = _cssc_obj_find_path(cobj_path);
    if (!resolved) {
        cssc_panicf("#include(.cobj): could not locate '%s'", cobj_path);
        return cssc_null();
    }
    char* dll_path = _cssc_cobj_extract_dll(resolved, alias);
    cssc_free(resolved);
    if (!dll_path) {
        cssc_panicf("#include(.cobj): invalid or truncated .cobj file");
        return cssc_null();
    }

    HMODULE h = LoadLibraryA(dll_path);
    if (!h) {
        cssc_panicf("#include(.cobj): LoadLibrary failed for extracted '%s'", dll_path);
        cssc_free(dll_path);
        return cssc_null();
    }
    typedef CsscVal (*InitFn)(CsscScopeStack*);
    InitFn init_fn = (InitFn)GetProcAddress(h, "cssc_cobj_init");
    if (!init_fn) init_fn = (InitFn)GetProcAddress(h, "cssc_module_init");
    void* cleanup_fn = (void*)GetProcAddress(h, "cssc_cobj_cleanup");

    CsscVal sec = cssc_null();
    if (init_fn) {
        sec = init_fn(host_scope);
    }
    if (CSSC_IS_NULL(sec)) {
        sec = cssc_sector_create(alias);
    }
    cssc_scope_set(host_scope, alias, sec);

    if (g_cobjs_count < MAX_COBJ_LOADED) {
        g_cobjs[g_cobjs_count].dll = h;
        g_cobjs[g_cobjs_count].cleanup_fn = cleanup_fn;
        strncpy(g_cobjs[g_cobjs_count].tmp_path, dll_path,
                sizeof(g_cobjs[g_cobjs_count].tmp_path) - 1);
        g_cobjs_count++;
    }
    cssc_free(dll_path);
    return sec;
#else
    (void)host_scope; (void)cobj_path; (void)alias;
    return cssc_null();
#endif
}

/* =========================================================================
 * 19b. WATERMARK — "CSSeries Engine" animated intro
 * ========================================================================= */

#ifdef _WIN32
static volatile LONG g_watermark_shown = 0;    /* atomic-ish flag */
static volatile LONG g_watermark_dismiss = 0;
static HANDLE g_watermark_thread = NULL;

static LRESULT CALLBACK _cssc_wm_wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE || m == WM_DESTROY) {
        InterlockedExchange(&g_watermark_dismiss, 1);
    }
    return DefWindowProcA(h, m, w, l);
}

static DWORD WINAPI _cssc_watermark_thread_entry(LPVOID param) {
    (void)param;
    /* Create a top-level borderless, centered, black window */
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = _cssc_wm_wndproc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "CsscWatermark";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    int w = scrW > 1200 ? 1200 : scrW;
    int h = 240;
    int x = (scrW - w) / 2;
    int y = (scrH - h) / 2;

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        "CsscWatermark", "",
        WS_POPUP,
        x, y, w, h,
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );
    if (!hwnd) return 0;

    /* Fully opaque black background, we'll control per-pixel via drawing */
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(memDC, memBmp);

    HFONT font = CreateFontA(
        72, 0, 0, 0, FW_LIGHT,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI Light"
    );
    SelectObject(memDC, font);
    SetBkMode(memDC, TRANSPARENT);

    const char* title = "CSSeries Engine";
    const char* subtitle = "tm";

    /* 6 second animation:
     *   0.0 - 0.4s  — reveal letters left→right
     *   0.4 - 0.7s  — shine sweep
     *   0.7 - 1.0s  — hold
     *   1.0 - 1.0s  — fade out */
    DWORD t0 = GetTickCount();
    const DWORD DURATION = 6000;
    while (!g_watermark_dismiss) {
        DWORD now = GetTickCount();
        DWORD el = now - t0;
        if (el > DURATION) break;
        float progress = (float)el / (float)DURATION;

        /* Clear to black */
        RECT r = {0, 0, w, h};
        FillRect(memDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

        /* Phase: reveal (0 - 0.4) */
        int visible_chars;
        int shine_x = -1;
        int alpha = 255;
        if (progress < 0.4f) {
            float p = progress / 0.4f;
            visible_chars = (int)((float)strlen(title) * p + 0.999f);
        } else if (progress < 0.7f) {
            visible_chars = (int)strlen(title);
            float p = (progress - 0.4f) / 0.3f;
            shine_x = (int)(p * (float)w);
        } else if (progress < 0.85f) {
            visible_chars = (int)strlen(title);
        } else {
            visible_chars = (int)strlen(title);
            float p = (progress - 0.85f) / 0.15f;
            alpha = (int)((1.0f - p) * 255.0f);
            if (alpha < 0) alpha = 0;
        }

        /* Draw the partial title */
        char partial[64];
        int maxlen = visible_chars < (int)sizeof(partial) - 1 ? visible_chars : (int)sizeof(partial) - 1;
        memcpy(partial, title, maxlen);
        partial[maxlen] = '\0';

        /* Compute text size to center */
        SIZE ts;
        GetTextExtentPoint32A(memDC, title, (int)strlen(title), &ts);
        int tx = (w - ts.cx) / 2;
        int ty = (h - ts.cy) / 2 - 10;

        /* Shadow / glow — draw multiple offset copies */
        SetTextColor(memDC, RGB(alpha / 4, alpha / 4, alpha / 4));
        TextOutA(memDC, tx + 2, ty + 2, partial, maxlen);

        /* Main text */
        SetTextColor(memDC, RGB(alpha, alpha, alpha));
        TextOutA(memDC, tx, ty, partial, maxlen);

        /* Subtitle (tm) */
        HFONT small = CreateFontA(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        HFONT oldFont = (HFONT)SelectObject(memDC, small);
        SetTextColor(memDC, RGB(alpha / 2, alpha / 2, alpha / 2));
        TextOutA(memDC, tx + ts.cx + 6, ty + 4, subtitle, (int)strlen(subtitle));
        SelectObject(memDC, oldFont);
        DeleteObject(small);

        /* Shine — a bright vertical bar sweeping across during shine phase */
        if (shine_x >= 0 && visible_chars > 0) {
            /* Reveal bright over the letters at shine_x */
            int bar_w = 140;
            for (int sx = shine_x - bar_w / 2; sx < shine_x + bar_w / 2; sx++) {
                if (sx < tx || sx > tx + ts.cx) continue;
                int dist = abs(sx - shine_x);
                int bright = 255 - (dist * 255 / (bar_w / 2));
                if (bright < 0) bright = 0;
                /* Blend — draw a vertical white gradient stripe at this x */
                for (int py = ty; py < ty + ts.cy; py++) {
                    COLORREF existing = GetPixel(memDC, sx, py);
                    if (existing != RGB(0, 0, 0)) {
                        int er = GetRValue(existing);
                        int eg = GetGValue(existing);
                        int eb = GetBValue(existing);
                        int nr = er + ((255 - er) * bright / 255);
                        int ng = eg + ((255 - eg) * bright / 255);
                        int nb = eb + ((255 - eb) * bright / 255);
                        if (nr > 255) nr = 255;
                        if (ng > 255) ng = 255;
                        if (nb > 255) nb = 255;
                        SetPixel(memDC, sx, py, RGB(nr, ng, nb));
                    }
                }
            }
        }

        /* Blit to window */
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

        /* Layered window alpha for fade */
        SetLayeredWindowAttributes(hwnd, 0, (BYTE)(alpha > 255 ? 255 : (alpha < 0 ? 0 : alpha)), LWA_ALPHA);

        Sleep(16);
    }

    if (font)    DeleteObject(font);
    if (memBmp)  DeleteObject(memBmp);
    if (memDC)   DeleteDC(memDC);
    if (hdc)     ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
    return 0;
}
#endif

CSSC_API void cssc_watermark_show_once(void) {
#ifdef _WIN32
    if (InterlockedCompareExchange(&g_watermark_shown, 1, 0) != 0) return;  /* already shown */
    g_watermark_dismiss = 0;
    g_watermark_thread = CreateThread(NULL, 0, _cssc_watermark_thread_entry, NULL, 0, NULL);
#endif
}

CSSC_API void cssc_watermark_dismiss(void) {
#ifdef _WIN32
    InterlockedExchange(&g_watermark_dismiss, 1);
#endif
}

CSSC_API bool cssc_watermark_is_showing(void) {
#ifdef _WIN32
    if (g_watermark_shown && !g_watermark_dismiss) {
        DWORD code = 0;
        if (g_watermark_thread && GetExitCodeThread(g_watermark_thread, &code)) {
            return code == STILL_ACTIVE;
        }
    }
#endif
    return false;
}

/* =========================================================================
 * 19c. SOUND — Win32 PlaySound wrappers for WAV playback
 * ========================================================================= */

/* Keep the most recent in-memory WAV buffer alive across PlaySound calls.
 * PlaySoundA with SND_ASYNC reads the buffer continuously, so we mustn't
 * free it until playback ends or we call stop / replace. */
#ifdef _WIN32
static unsigned char* g_sound_mem_buf = NULL;
static uint32_t       g_sound_mem_size = 0;
#endif

CSSC_API bool cssc_sound_play_file(const char* path, bool async) {
#ifdef _WIN32
    DWORD flags = SND_FILENAME | SND_NODEFAULT;
    if (async) flags |= SND_ASYNC;
    else       flags |= SND_SYNC;
    return PlaySoundA(path, NULL, flags) ? true : false;
#else
    (void)path; (void)async;
    return false;
#endif
}

CSSC_API bool cssc_sound_play_memory(const void* data, uint32_t size, bool async) {
#ifdef _WIN32
    if (!data || !size) return false;
    /* Copy into our own buffer so we control its lifetime */
    if (g_sound_mem_buf) {
        PlaySoundA(NULL, NULL, 0);          /* stop whatever is using the old buffer */
        cssc_free(g_sound_mem_buf);
        g_sound_mem_buf = NULL;
        g_sound_mem_size = 0;
    }
    g_sound_mem_buf = (unsigned char*)cssc_alloc(size);
    memcpy(g_sound_mem_buf, data, size);
    g_sound_mem_size = size;
    DWORD flags = SND_MEMORY | SND_NODEFAULT;
    if (async) flags |= SND_ASYNC;
    else       flags |= SND_SYNC;
    return PlaySoundA((LPCSTR)g_sound_mem_buf, NULL, flags) ? true : false;
#else
    (void)data; (void)size; (void)async;
    return false;
#endif
}

CSSC_API void cssc_sound_stop(void) {
#ifdef _WIN32
    PlaySoundA(NULL, NULL, 0);
    if (g_sound_mem_buf) {
        cssc_free(g_sound_mem_buf);
        g_sound_mem_buf = NULL;
        g_sound_mem_size = 0;
    }
#endif
}

/* =========================================================================
 * 19d. .obj ASSET READING
 * ========================================================================= */

CSSC_API CsscVal cssc_obj_asset_read(const char* obj_path, const char* entry_name) {
    if (!obj_path || !entry_name) return cssc_null();

    /* Find the .obj using the same path resolution as cssc_obj_load */
    char* resolved = _cssc_obj_find_path(obj_path);
    if (!resolved) return cssc_null();

    FILE* f = fopen(resolved, "rb");
    cssc_free(resolved);
    if (!f) return cssc_null();

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 12) { fclose(f); return cssc_null(); }
    unsigned char* buf = (unsigned char*)cssc_alloc((size_t)sz);
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f); cssc_free(buf); return cssc_null();
    }
    fclose(f);

    size_t off = 0;
    if (off + 4 > (size_t)sz) { cssc_free(buf); return cssc_null(); }
    uint32_t version = *(uint32_t*)(buf + off); off += 4;
    if (version != 1) { cssc_free(buf); return cssc_null(); }
    if (off + 4 > (size_t)sz) { cssc_free(buf); return cssc_null(); }
    uint32_t name_len = *(uint32_t*)(buf + off); off += 4 + name_len;
    if (off + 4 > (size_t)sz) { cssc_free(buf); return cssc_null(); }
    uint32_t num_entries = *(uint32_t*)(buf + off); off += 4;

    CsscVal result = cssc_null();
    for (uint32_t i = 0; i < num_entries; i++) {
        if (off + 4 > (size_t)sz) break;
        uint32_t enl = *(uint32_t*)(buf + off); off += 4;
        if (off + enl > (size_t)sz) break;
        char ename[512];
        size_t cn = enl < 511 ? enl : 511;
        memcpy(ename, buf + off, cn); ename[cn] = '\0';
        off += enl;
        if (off + 8 > (size_t)sz) break;
        uint64_t dlen = *(uint64_t*)(buf + off); off += 8;
        if (off + dlen > (size_t)sz) break;
        if (strcmp(ename, entry_name) == 0) {
            /* Return as a binary-safe string (cssc_string_len copies the bytes) */
            result = cssc_string_len((const char*)(buf + off), (size_t)dlen);
            break;
        }
        off += dlen;
    }
    cssc_free(buf);
    return result;
}

/* =========================================================================
 * 19e. CUSTOM ANIMATED INTRO — Apple-style splash with optional audio
 * ========================================================================= */

#ifdef _WIN32
typedef struct {
    char title[128];
    char subtitle[128];
    int32_t duration_ms;
    unsigned char* wav_buf;     /* owned, freed after intro */
    uint32_t wav_size;
    HANDLE ready_event;
} _IntroArgs;

static volatile LONG g_intro_shown = 0;

static LRESULT CALLBACK _cssc_intro_wndproc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return DefWindowProcA(h, m, w, l);
}

/* Draw a programmatic "CS" geometric monogram centered at (cx,cy) with given
 * radius. Two overlapping rotated diamonds plus a diagonal slash — reads as
 * a sharp, angular mark without needing an external asset. */
static void _cssc_draw_intro_logo(HDC dc, int cx, int cy, int radius, int alpha) {
    if (alpha <= 0) return;
    int a = alpha > 255 ? 255 : alpha;
    COLORREF col = RGB(a, a, a);

    HPEN pen = CreatePen(PS_SOLID, 4, col);
    HPEN old = (HPEN)SelectObject(dc, pen);
    HBRUSH oldB = (HBRUSH)SelectObject(dc, GetStockObject(NULL_BRUSH));

    /* Outer diamond */
    POINT d1[4] = {
        { cx,            cy - radius },
        { cx + radius,   cy          },
        { cx,            cy + radius },
        { cx - radius,   cy          },
    };
    Polygon(dc, d1, 4);

    /* Inner rotated square (smaller, rotated 45°) */
    int r2 = (int)(radius * 0.55f);
    POINT d2[4] = {
        { cx - r2, cy - r2 },
        { cx + r2, cy - r2 },
        { cx + r2, cy + r2 },
        { cx - r2, cy + r2 },
    };
    Polygon(dc, d2, 4);

    /* Central slash (the "CS" angle) */
    int r3 = (int)(radius * 0.38f);
    MoveToEx(dc, cx - r3, cy + r3, NULL);
    LineTo(dc,   cx + r3, cy - r3);

    /* Four short ticks at 45° offsets to give it a compass-like feel */
    int r4 = radius + 14;
    int r5 = radius + 32;
    MoveToEx(dc, cx - r4, cy,       NULL); LineTo(dc, cx - r5, cy);
    MoveToEx(dc, cx + r4, cy,       NULL); LineTo(dc, cx + r5, cy);
    MoveToEx(dc, cx,       cy - r4, NULL); LineTo(dc, cx,       cy - r5);
    MoveToEx(dc, cx,       cy + r4, NULL); LineTo(dc, cx,       cy + r5);

    SelectObject(dc, old);
    SelectObject(dc, oldB);
    DeleteObject(pen);
}

static DWORD WINAPI _cssc_intro_thread(LPVOID param) {
    _IntroArgs* args = (_IntroArgs*)param;

    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = _cssc_intro_wndproc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "CsscIntro";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    /* Full-screen, topmost, borderless — studio-intro style. */
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int w = sw;
    int h = sh;

    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "CsscIntro", "",
        WS_POPUP,
        0, 0, w, h,
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );
    if (!hwnd) {
        SetEvent(args->ready_event);
        if (args->wav_buf) cssc_free(args->wav_buf);
        cssc_free(args);
        return 0;
    }
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    SelectObject(memDC, memBmp);

    /* Title font scales with display height for readable size at any res. */
    int title_px = (int)(h * 0.08f);   /* ~ 8% of screen height */
    int sub_px   = (int)(h * 0.022f);

    HFONT fTitle = CreateFontA(
        title_px, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI"
    );
    HFONT fSub = CreateFontA(
        sub_px, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI"
    );
    SetBkMode(memDC, TRANSPARENT);

    SetEvent(args->ready_event);

    /* Kick off audio (async, non-blocking). */
    if (args->wav_buf && args->wav_size) {
        cssc_sound_play_memory(args->wav_buf, args->wav_size, true);
    }

    int title_len = (int)strlen(args->title);
    int sub_len   = (int)strlen(args->subtitle);
    float DUR = (float)args->duration_ms / 1000.0f;

    /* Layout anchors — logo centered, title below, subtitle below title.
     *   0.00 - 0.12 DUR : window fade-in, logo scales from 0.6 -> 1.0
     *   0.12 - 0.40 DUR : logo settles; title fades/slides into place
     *   0.40 - 0.75 DUR : hold; subtitle fades in at 0.55
     *   0.75 - 1.00 DUR : fade out (title + logo + window) */
    DWORD t0 = GetTickCount();
    for (;;) {
        DWORD now = GetTickCount();
        float el = (float)(now - t0) / 1000.0f;
        if (el >= DUR) break;
        float pg = el / DUR;

        /* Clear backing to pure black. */
        RECT r = {0, 0, w, h};
        FillRect(memDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));

        BYTE win_alpha    = 255;
        int   logo_alpha  = 0;
        float logo_scale  = 1.0f;
        int   title_alpha = 0;
        int   title_drop  = 0;
        int   sub_alpha   = 0;

        if (pg < 0.12f) {
            float p = pg / 0.12f;
            win_alpha   = (BYTE)(p * 255.0f);
            logo_alpha  = (int)(p * 255.0f);
            logo_scale  = 0.6f + 0.4f * p;
        } else if (pg < 0.40f) {
            float p = (pg - 0.12f) / 0.28f;
            win_alpha   = 255;
            logo_alpha  = 255;
            logo_scale  = 1.0f;
            title_alpha = (int)(p * 255.0f);
            title_drop  = (int)((1.0f - p) * 20.0f);     /* slide down a bit */
        } else if (pg < 0.75f) {
            win_alpha   = 255;
            logo_alpha  = 255;
            title_alpha = 255;
            if (pg < 0.55f) {
                float p = (pg - 0.40f) / 0.15f;
                sub_alpha = (int)(p * 200.0f);
            } else {
                sub_alpha = 200;
            }
        } else {
            float p = (pg - 0.75f) / 0.25f;
            /* ease-in cube */
            float q = p * p;
            win_alpha   = (BYTE)((1.0f - q) * 255.0f);
            logo_alpha  = (int)((1.0f - q) * 255.0f);
            title_alpha = (int)((1.0f - q) * 255.0f);
            sub_alpha   = (int)((1.0f - q) * 200.0f);
        }

        /* Logo — centered slightly above middle. */
        int logo_cx = w / 2;
        int logo_cy = (int)(h * 0.44f);
        int logo_r  = (int)((h * 0.10f) * logo_scale);
        _cssc_draw_intro_logo(memDC, logo_cx, logo_cy, logo_r, logo_alpha);

        /* Title under logo. */
        if (title_alpha > 0 && title_len > 0) {
            SelectObject(memDC, fTitle);
            SIZE ts; GetTextExtentPoint32A(memDC, args->title, title_len, &ts);
            int tx = (w - ts.cx) / 2;
            int ty = logo_cy + logo_r + (int)(h * 0.055f) + title_drop;
            int c = title_alpha > 255 ? 255 : title_alpha;
            SetTextColor(memDC, RGB(c, c, c));
            TextOutA(memDC, tx, ty, args->title, title_len);
        }

        /* Subtitle under title. */
        if (sub_alpha > 0 && sub_len > 0) {
            SelectObject(memDC, fSub);
            SIZE ss; GetTextExtentPoint32A(memDC, args->subtitle, sub_len, &ss);
            int sxp = (w - ss.cx) / 2;
            int syp = logo_cy + logo_r + (int)(h * 0.055f) + title_px + (int)(h * 0.012f);
            int c = sub_alpha > 255 ? 255 : sub_alpha;
            SetTextColor(memDC, RGB(c, c, c));
            TextOutA(memDC, sxp, syp, args->subtitle, sub_len);
        }

        /* Present. */
        BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        SetLayeredWindowAttributes(hwnd, 0, win_alpha, LWA_ALPHA);

        Sleep(16);
    }

    if (fTitle) DeleteObject(fTitle);
    if (fSub)   DeleteObject(fSub);
    if (memBmp) DeleteObject(memBmp);
    if (memDC)  DeleteDC(memDC);
    if (hdc)    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    if (args->wav_buf) cssc_free(args->wav_buf);
    cssc_free(args);
    return 0;
}
#endif

CSSC_API void cssc_intro_play(const char* title,
                              const char* subtitle,
                              int32_t duration_ms,
                              const void* wav_data,
                              uint32_t wav_size) {
#ifdef _WIN32
    if (InterlockedCompareExchange(&g_intro_shown, 1, 0) != 0) return;

    _IntroArgs* args = (_IntroArgs*)cssc_alloc(sizeof(_IntroArgs));
    memset(args, 0, sizeof(*args));
    strncpy(args->title,    title    ? title    : "CSSC", sizeof(args->title) - 1);
    strncpy(args->subtitle, subtitle ? subtitle : "",     sizeof(args->subtitle) - 1);
    args->duration_ms = duration_ms > 0 ? duration_ms : 5000;
    if (wav_data && wav_size) {
        args->wav_buf = (unsigned char*)cssc_alloc(wav_size);
        memcpy(args->wav_buf, wav_data, wav_size);
        args->wav_size = wav_size;
    }
    args->ready_event = CreateEventA(NULL, TRUE, FALSE, NULL);

    HANDLE th = CreateThread(NULL, 0, _cssc_intro_thread, args, 0, NULL);
    if (th) {
        WaitForSingleObject(args->ready_event, 2000);
        CloseHandle(th);  /* detached */
    }
    CloseHandle(args->ready_event);
#else
    (void)title; (void)subtitle; (void)duration_ms; (void)wav_data; (void)wav_size;
#endif
}

/* =========================================================================
 * 19. GLOBAL STATE
 * ========================================================================= */

static CsscScopeStack g_scope;
static CsscScopeStack* g_external_scope = NULL;   /* set by module DLLs */
static bool g_initialized = false;

/* cssc_install_crash_handler is the SEH crash reporter defined in
 * cssc_host_extras.c. Module DLLs (`cssc makemodule`) and other legacy builds
 * link cssc_runtime.c WITHOUT host_extras.c, which left the symbol undefined
 * at link time. Provide a weak no-op fallback: any build that also links
 * host_extras.c picks up the real (strong) handler, while a standalone
 * runtime/module DLL falls back to this harmless stub. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) void cssc_install_crash_handler(void) { }
#else
extern void cssc_install_crash_handler(void);
#endif

CSSC_API void cssc_runtime_init(void) {
    if (g_initialized) return;
    g_initialized = true;
    cssc_install_crash_handler();
    srand((unsigned int)time(NULL));
    cssc_scope_init(&g_scope);
    _cssc_daemons_init();
}

CSSC_API void cssc_runtime_shutdown(void) {
    if (!g_initialized) return;
    cssc_daemon_shutdown_all();
#ifdef _WIN32
    /* Let .cobj modules run their free blocks while the host scope is still
     * alive — reverse-order so the last-loaded .cobj is cleaned first. */
    _cssc_cobj_run_all_cleanups(g_external_scope ? g_external_scope : &g_scope);
    /* Tear down watermark thread if still running */
    if (g_watermark_thread) {
        cssc_watermark_dismiss();
        WaitForSingleObject(g_watermark_thread, 2000);
        CloseHandle(g_watermark_thread);
        g_watermark_thread = NULL;
    }
#endif
    /* Only destroy our own scope — external (host) scope belongs to someone else */
    if (!g_external_scope) {
        cssc_scope_destroy(&g_scope);
    }
    g_initialized = false;
}

CSSC_API CsscScopeStack* cssc_global_scope(void) {
    return g_external_scope ? g_external_scope : &g_scope;
}

CSSC_API void cssc_global_scope_set(CsscScopeStack* external) {
    g_external_scope = external;
}
