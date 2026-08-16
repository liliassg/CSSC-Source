
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#if defined(_WIN32)
#  include <io.h>
#  include <fcntl.h>
#else
#  include <dirent.h>
#endif
#include "cssc_fmt_f64.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define CSSC_CX_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
    #define CSSC_CX_EXPORT __attribute__((visibility("default")))
#else
    #define CSSC_CX_EXPORT
#endif

static uint64_t g_heap_used = 0;
CSSC_CX_EXPORT uint64_t cssc_alloc_raw(uint64_t n) {
    void* raw = malloc((size_t)n + 16);
    if (!raw) return 0;
    *(uint64_t*)raw = n;
    g_heap_used += n + 16;
    return (uint64_t)(uintptr_t)raw + 16;
}
CSSC_CX_EXPORT void cssc_free_raw(uint64_t p) {
    if (!p) return;
    void* raw = (void*)(uintptr_t)(p - 16);
    g_heap_used -= *(uint64_t*)raw + 16;
    free(raw);
}
CSSC_CX_EXPORT uint64_t cssc_heap_used(void) { return g_heap_used; }

CSSC_CX_EXPORT void cssc_memcopy(uint64_t dst, uint64_t src, uint64_t words) {
    memmove((void*)(uintptr_t)dst, (void*)(uintptr_t)src, (size_t)words * 4u);
}
CSSC_CX_EXPORT void cssc_memfill32(uint64_t addr, uint64_t val, uint64_t count) {
    uint32_t* d = (uint32_t*)(uintptr_t)addr;
    uint32_t v = (uint32_t)val;
    for (uint64_t i = 0; i < count; i++) d[i] = v;
}

CSSC_CX_EXPORT uint64_t cssc_peek8(uint64_t a)  { return *(volatile uint8_t*)(uintptr_t)a; }
CSSC_CX_EXPORT uint64_t cssc_peek16(uint64_t a) { return *(volatile uint16_t*)(uintptr_t)a; }
CSSC_CX_EXPORT uint64_t cssc_peek32(uint64_t a) { return *(volatile uint32_t*)(uintptr_t)a; }
CSSC_CX_EXPORT uint64_t cssc_peek64(uint64_t a) { return *(volatile uint64_t*)(uintptr_t)a; }
CSSC_CX_EXPORT void cssc_poke8(uint64_t a, uint64_t v)  { *(volatile uint8_t*)(uintptr_t)a  = (uint8_t)v; }
CSSC_CX_EXPORT void cssc_poke16(uint64_t a, uint64_t v) { *(volatile uint16_t*)(uintptr_t)a = (uint16_t)v; }
CSSC_CX_EXPORT void cssc_poke32(uint64_t a, uint64_t v) { *(volatile uint32_t*)(uintptr_t)a = (uint32_t)v; }
CSSC_CX_EXPORT void cssc_poke64(uint64_t a, uint64_t v) { *(volatile uint64_t*)(uintptr_t)a = (uint64_t)v; }

CSSC_CX_EXPORT uint64_t cssc_band(uint64_t a, uint64_t b) { return a & b; }
CSSC_CX_EXPORT uint64_t cssc_bor(uint64_t a, uint64_t b)  { return a | b; }
CSSC_CX_EXPORT uint64_t cssc_bxor(uint64_t a, uint64_t b) { return a ^ b; }
CSSC_CX_EXPORT uint64_t cssc_shl(uint64_t a, uint64_t b) { return a << b; }
CSSC_CX_EXPORT uint64_t cssc_shr(uint64_t a, uint64_t b) { return a >> b; }

CSSC_CX_EXPORT void cssc_outb(uint64_t p, uint64_t v) { (void)p; (void)v; }
CSSC_CX_EXPORT void cssc_outw(uint64_t p, uint64_t v) { (void)p; (void)v; }
CSSC_CX_EXPORT void cssc_outl(uint64_t p, uint64_t v) { (void)p; (void)v; }
CSSC_CX_EXPORT uint64_t cssc_inb(uint64_t p) { (void)p; return 0; }
CSSC_CX_EXPORT uint64_t cssc_inw(uint64_t p) { (void)p; return 0; }
CSSC_CX_EXPORT uint64_t cssc_inl(uint64_t p) { (void)p; return 0; }
CSSC_CX_EXPORT uint64_t cssc_fb_addr(void)   { return 0; }
CSSC_CX_EXPORT uint64_t cssc_fb_width(void)  { return 0; }
CSSC_CX_EXPORT uint64_t cssc_fb_height(void) { return 0; }
CSSC_CX_EXPORT uint64_t cssc_fb_pitch(void)  { return 0; }
CSSC_CX_EXPORT uint64_t cssc_fb_bpp(void)    { return 0; }

typedef union { double d; uint64_t u; } cssc_fbits;
static double  cssc__f(uint64_t u) { cssc_fbits b; b.u = u; return b.d; }
static uint64_t cssc__u(double d)  { cssc_fbits b; b.d = d; return b.u; }
static double  cssc__trunc(double x) {
    if (x != x) return x;
    if (x < 0) { double t = -x; uint64_t i = (uint64_t)t; return -(double)i; }
    uint64_t i = (uint64_t)x; return (double)i;
}
static double  cssc__floor(double x) { double t = cssc__trunc(x); if (t > x) t -= 1.0; return t; }
static double  cssc__ceil(double x)  { double t = cssc__trunc(x); if (t < x) t += 1.0; return t; }
static double  cssc__round(double x) { if (x < 0) return -cssc__floor(-x + 0.5); return cssc__floor(x + 0.5); }
CSSC_CX_EXPORT uint64_t cssc_fadd(uint64_t a, uint64_t b) { return cssc__u(cssc__f(a) + cssc__f(b)); }
CSSC_CX_EXPORT uint64_t cssc_fsub(uint64_t a, uint64_t b) { return cssc__u(cssc__f(a) - cssc__f(b)); }
CSSC_CX_EXPORT uint64_t cssc_fmul(uint64_t a, uint64_t b) { return cssc__u(cssc__f(a) * cssc__f(b)); }
CSSC_CX_EXPORT uint64_t cssc_fdiv(uint64_t a, uint64_t b) { return cssc__u(cssc__f(a) / cssc__f(b)); }
CSSC_CX_EXPORT uint64_t cssc_fmod_(uint64_t a, uint64_t b) {
    double x = cssc__f(a), y = cssc__f(b);
    if (y == 0.0) { cssc_fbits n; n.u = 0x7FF8000000000000ULL; return n.u; }
    return cssc__u(x - cssc__trunc(x / y) * y);
}
CSSC_CX_EXPORT uint64_t cssc_fneg(uint64_t a) { return a ^ 0x8000000000000000ULL; }
CSSC_CX_EXPORT uint64_t cssc_fcmp(uint64_t a, uint64_t b) {
    double x = cssc__f(a), y = cssc__f(b);
    if (x < y) return (uint64_t)(int64_t)-1;
    if (x > y) return 1;
    if (x == y) return 0;
    return 2;
}
CSSC_CX_EXPORT uint64_t cssc_i2f(uint64_t i) { return cssc__u((double)(int64_t)i); }
CSSC_CX_EXPORT uint64_t cssc_f2i(uint64_t f) { return (uint64_t)(int64_t)cssc__f(f); }
CSSC_CX_EXPORT uint64_t cssc_ffloor(uint64_t f) { return cssc__u(cssc__floor(cssc__f(f))); }
CSSC_CX_EXPORT uint64_t cssc_fceil(uint64_t f)  { return cssc__u(cssc__ceil(cssc__f(f))); }
CSSC_CX_EXPORT uint64_t cssc_ftrunc(uint64_t f) { return cssc__u(cssc__trunc(cssc__f(f))); }
CSSC_CX_EXPORT uint64_t cssc_fround(uint64_t f) { return cssc__u(cssc__round(cssc__f(f))); }
CSSC_CX_EXPORT uint64_t cssc_fabs(uint64_t f) { return f & 0x7FFFFFFFFFFFFFFFULL; }
CSSC_CX_EXPORT uint64_t cssc_fsqrt(uint64_t f) {

    double a = cssc__f(f);
    if (a < 0) { cssc_fbits n; n.u = 0x7FF8000000000000ULL; return n.u; }
    return cssc__u(__builtin_sqrt(a));
}

CSSC_CX_EXPORT uint64_t cssc_f2str(uint64_t f, uint64_t buf, uint64_t cap) {

    double x = cssc__f(f);
    char* out = (char*)(uintptr_t)buf;
    int64_t c = (int64_t)cap;
    char tmp[64];
    int64_t n = (int64_t)cssc_fmt_f64_shortest(tmp, x);
    if (n > c) n = c;
    for (int64_t i = 0; i < n; i++) out[i] = tmp[i];
    return (uint64_t)n;
}

CSSC_CX_EXPORT uint64_t cssc_str2f(uint64_t ptr, uint64_t len) {
    const char* s = (const char*)(uintptr_t)ptr;
    int64_t n = (int64_t)len, i = 0; int neg = 0;
    if (i < n && (s[i] == '-' || s[i] == '+')) { neg = (s[i] == '-'); i++; }
    double v = 0.0;
    while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10.0 + (s[i] - '0'); i++; }
    if (i < n && s[i] == '.') { i++; double scale = 0.1; while (i < n && s[i] >= '0' && s[i] <= '9') { v += (s[i] - '0') * scale; scale *= 0.1; i++; } }
    if (neg) v = -v;
    return cssc__u(v);
}

CSSC_CX_EXPORT uint64_t cssc_host_read(uint64_t addr, uint64_t maxn) {
    size_t n = fread((void*)(uintptr_t)addr, 1, (size_t)maxn, stdin);
    return (uint64_t)n;
}
CSSC_CX_EXPORT void cssc_host_write(uint64_t addr, uint64_t n) {
#if defined(_WIN32)

    static int mode_set = 0;
    if (!mode_set) { _setmode(_fileno(stdout), _O_BINARY); mode_set = 1; }
#endif
    fwrite((void*)(uintptr_t)addr, 1, (size_t)n, stdout);
    fflush(stdout);
}

CSSC_CX_EXPORT void cssc_host_err(uint64_t addr, uint64_t n) {
    fwrite((void*)(uintptr_t)addr, 1, (size_t)n, stderr);
    fflush(stderr);
}

CSSC_CX_EXPORT uint64_t cssc_os_read_file(uint64_t namePtr, uint64_t nameLen,
                                          uint64_t bufPtr, uint64_t bufMax) {
    char nm[1024];
    uint64_t nl = nameLen; if (nl > 1023) nl = 1023;
    const char* np = (const char*)(uintptr_t)namePtr;
    for (uint64_t i = 0; i < nl; i++) nm[i] = np[i];
    nm[nl] = 0;
    FILE* fp = fopen(nm, "rb");
    if (!fp) return (uint64_t)-1;
    size_t n = fread((void*)(uintptr_t)bufPtr, 1, (size_t)bufMax, fp);
    fclose(fp);
    return (uint64_t)n;
}

CSSC_CX_EXPORT uint64_t cssc_os_list_dir(uint64_t namePtr, uint64_t nameLen,
                                         uint64_t bufPtr, uint64_t bufMax) {
    char nm[1024];
    uint64_t nl = nameLen; if (nl > 1023) nl = 1023;
    const char* np = (const char*)(uintptr_t)namePtr;
    for (uint64_t i = 0; i < nl; i++) nm[i] = np[i];
    nm[nl] = 0;
    char* out = (char*)(uintptr_t)bufPtr;
    uint64_t len = 0;
#if defined(_WIN32)

    char pattern[1200];
    size_t pl = strlen(nm);
    if (pl + 3 >= sizeof(pattern)) return 0;
    memcpy(pattern, nm, pl);
    pattern[pl] = '\\'; pattern[pl + 1] = '*'; pattern[pl + 2] = 0;
    struct _finddata_t fd;
    intptr_t h = _findfirst(pattern, &fd);
    if (h != -1) {
        do {
            const char* n = fd.name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            size_t el = strlen(n);
            if (len + el + 1 > bufMax) break;
            if (len > 0) out[len++] = '\n';
            memcpy(out + len, n, el); len += el;
        } while (_findnext(h, &fd) == 0);
        _findclose(h);
    }
#else
    DIR* d = opendir(nm);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            const char* n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            size_t el = strlen(n);
            if (len + el + 1 > bufMax) break;
            if (len > 0) out[len++] = '\n';
            memcpy(out + len, n, el); len += el;
        }
        closedir(d);
    }
#endif
    return len;
}

static char cssc_g_mod_path[1024];
CSSC_CX_EXPORT void cssc_mod_path(uint64_t ptr, uint64_t len) {
    if (len > 1023) len = 1023;
    memcpy(cssc_g_mod_path, (const void*)(uintptr_t)ptr, (size_t)len);
    cssc_g_mod_path[len] = 0;
}
CSSC_CX_EXPORT void cssc_mod_path_inc(uint64_t ptr, uint64_t len) {
    const char* name = (const char*)(uintptr_t)ptr; uint64_t nl = len; size_t k = 0; size_t i;
    if (nl > 5 && name[0]=='c'&&name[1]=='s'&&name[2]=='s'&&name[3]=='c'&&name[4]=='.') { name += 5; nl -= 5; }
    const char* pre = "module/"; const char* suf = ".cssc";
    for (i = 0; pre[i]; i++) cssc_g_mod_path[k++] = pre[i];
    for (i = 0; i < nl && k < 1015; i++) cssc_g_mod_path[k++] = name[i];
    for (i = 0; suf[i]; i++) cssc_g_mod_path[k++] = suf[i];
    cssc_g_mod_path[k] = 0;
}
CSSC_CX_EXPORT int64_t cssc_mod_read(uint64_t bufPtr, uint64_t bufMax) {
    FILE* fp = fopen(cssc_g_mod_path, "rb");
    if (!fp) return -1;
    size_t n = fread((void*)(uintptr_t)bufPtr, 1, (size_t)bufMax, fp);
    fclose(fp);
    return (int64_t)n;
}

extern int   cssc_argc;
extern void* cssc_argv;
CSSC_CX_EXPORT uint64_t cssc_host_argc(void) {
    return (uint64_t)(int64_t)cssc_argc;
}
CSSC_CX_EXPORT uint64_t cssc_host_argstr(uint64_t idx, uint64_t bufPtr, uint64_t bufMax) {
    if ((int64_t)idx < 0 || (int64_t)idx >= (int64_t)cssc_argc) return 0;
    char** av = (char**)cssc_argv;
    if (!av) return 0;
    const char* s = av[(size_t)idx];
    if (!s) return 0;
    uint64_t n = 0; while (s[n]) n++;
    if (n > bufMax) n = bufMax;
    char* dst = (char*)(uintptr_t)bufPtr;
    for (uint64_t i = 0; i < n; i++) dst[i] = s[i];
    return n;
}

#if defined(_WIN32)
extern void Sleep(unsigned int);
extern int  Beep(unsigned int, unsigned int);
extern int  GetDiskFreeSpaceExA(const char*, unsigned long long*, unsigned long long*, unsigned long long*);
extern int  _getpid(void);
#define CSSC_POPEN _popen
#define CSSC_PCLOSE _pclose
#else
#include <unistd.h>
#define CSSC_POPEN popen
#define CSSC_PCLOSE pclose
#endif

CSSC_CX_EXPORT uint64_t cssc_os_time(void) {
    return cssc__u((double)time(0));
}
CSSC_CX_EXPORT uint64_t cssc_os_timestamp(void) {
    return (uint64_t)(int64_t)time(0);
}
CSSC_CX_EXPORT uint64_t cssc_os_clock(void) {
    return cssc__u((double)clock() / (double)CLOCKS_PER_SEC);
}
CSSC_CX_EXPORT uint64_t cssc_os_detime(void) {
    time_t t = time(0);
    struct tm* lt = localtime(&t);
    if (!lt) return cssc__u(0.0);
    return cssc__u((double)lt->tm_hour + (double)lt->tm_min / 100.0);
}
CSSC_CX_EXPORT uint64_t cssc_os_sdetime(void) {
    time_t t = time(0);
    struct tm* lt = localtime(&t);
    if (!lt) return cssc__u(0.0);
    return cssc__u((double)lt->tm_min + (double)lt->tm_sec / 100.0);
}
CSSC_CX_EXPORT void cssc_os_sleep(uint64_t ms) {
#if defined(_WIN32)
    Sleep((unsigned int)ms);
#else
    struct timespec ts; ts.tv_sec = (long)(ms / 1000); ts.tv_nsec = (long)((ms % 1000) * 1000000L); nanosleep(&ts, 0);
#endif
}
CSSC_CX_EXPORT uint64_t cssc_os_getenv(uint64_t namePtr, uint64_t nameLen, uint64_t bufPtr, uint64_t bufMax) {
    char nm[256]; uint64_t nl = nameLen; if (nl > 255) nl = 255;
    const char* np = (const char*)(uintptr_t)namePtr;
    for (uint64_t i = 0; i < nl; i++) nm[i] = np[i];
    nm[nl] = 0;
    const char* v = getenv(nm);
    if (!v) return 0;
    uint64_t n = 0; while (v[n]) n++;
    if (n > bufMax) n = bufMax;
    char* dst = (char*)(uintptr_t)bufPtr;
    for (uint64_t i = 0; i < n; i++) dst[i] = v[i];
    return n;
}
CSSC_CX_EXPORT uint64_t cssc_os_system(uint64_t cmdPtr, uint64_t cmdLen) {
    char* cmd = (char*)malloc((size_t)cmdLen + 1);
    if (!cmd) return (uint64_t)(int64_t)-1;
    const char* cp = (const char*)(uintptr_t)cmdPtr;
    for (uint64_t i = 0; i < cmdLen; i++) cmd[i] = cp[i];
    cmd[cmdLen] = 0;
    int rc = system(cmd);
    free(cmd);
    return (uint64_t)(int64_t)rc;
}

CSSC_CX_EXPORT uint64_t cssc_os_popen(uint64_t cmdPtr, uint64_t cmdLen, uint64_t bufPtr, uint64_t bufMax) {
    char* cmd = (char*)malloc((size_t)cmdLen + 1);
    if (!cmd) return 0;
    const char* cp = (const char*)(uintptr_t)cmdPtr;
    for (uint64_t i = 0; i < cmdLen; i++) cmd[i] = cp[i];
    cmd[cmdLen] = 0;
    FILE* fp = CSSC_POPEN(cmd, "r");
    free(cmd);
    if (!fp) return 0;
    char* dst = (char*)(uintptr_t)bufPtr;
    size_t n = fread(dst, 1, (size_t)bufMax, fp);
    CSSC_PCLOSE(fp);
    while (n > 0) { char c = dst[n - 1]; if (c == '\n' || c == '\r' || c == ' ' || c == '\t') n--; else break; }
    return (uint64_t)n;
}
CSSC_CX_EXPORT uint64_t cssc_os_pid(void) {
#if defined(_WIN32)
    return (uint64_t)(int64_t)_getpid();
#else
    return (uint64_t)(int64_t)getpid();
#endif
}
CSSC_CX_EXPORT uint64_t cssc_os_beep(uint64_t freq, uint64_t dur) {
#if defined(_WIN32)
    return (uint64_t)(int64_t)Beep((unsigned int)freq, (unsigned int)dur);
#else
    (void)freq; (void)dur; return 0;
#endif
}

CSSC_CX_EXPORT uint64_t cssc_os_diskusage(uint64_t pathPtr, uint64_t pathLen, uint64_t which) {
#if defined(_WIN32)
    char p[1024]; uint64_t pl = pathLen; if (pl > 1023) pl = 1023;
    const char* pp = (const char*)(uintptr_t)pathPtr;
    for (uint64_t i = 0; i < pl; i++) p[i] = pp[i];
    p[pl] = 0;
    unsigned long long freeAvail = 0, total = 0, totalFree = 0;
    if (!GetDiskFreeSpaceExA(p[0] ? p : ".", &freeAvail, &total, &totalFree)) return 0;
    if (which == 0) return total;
    if (which == 1) return totalFree;
    return total - totalFree;
#else
    (void)pathPtr; (void)pathLen; (void)which; return 0;
#endif
}
CSSC_CX_EXPORT uint64_t cssc_os_kill(uint64_t pid) {
    char cmd[64];
    int n = snprintf(cmd, sizeof(cmd), "taskkill /F /PID %llu >nul 2>&1", (unsigned long long)pid);
    if (n <= 0) return 0;
    return (system(cmd) == 0) ? 1 : 0;
}

CSSC_CX_EXPORT uint64_t cssc_os_readline(uint64_t bufPtr, uint64_t bufMax) {
    char* dst = (char*)(uintptr_t)bufPtr;
    uint64_t n = 0;
    for (;;) {
        int ch = fgetc(stdin);
        if (ch == EOF) break;
        if (ch == '\n') break;
        if (ch == '\r') continue;
        if (n < bufMax) { dst[n++] = (char)ch; }
    }
    return n;
}

CSSC_CX_EXPORT uint64_t cssc_os_strftime(uint64_t fmtPtr, uint64_t fmtLen, uint64_t bufPtr, uint64_t bufMax) {
    char fmt[128]; uint64_t fl = fmtLen; if (fl > 127) fl = 127;
    const char* fp = (const char*)(uintptr_t)fmtPtr;
    for (uint64_t i = 0; i < fl; i++) fmt[i] = fp[i];
    fmt[fl] = 0;
    time_t t = time(0);
    struct tm* lt = localtime(&t);
    if (!lt) return 0;
    char tmp[256];
    size_t n = strftime(tmp, sizeof(tmp), fmt, lt);
    if ((uint64_t)n > bufMax) n = (size_t)bufMax;
    char* dst = (char*)(uintptr_t)bufPtr;
    for (size_t i = 0; i < n; i++) dst[i] = tmp[i];
    return (uint64_t)n;
}
