
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "../cssc_fmt_f64.h"

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  include <direct.h>
#  include <windows.h>
#  define cssc_getcwd _getcwd
#  define cssc_mkdir(p) _mkdir(p)
#else
#  include <unistd.h>
#  include <dirent.h>
#  include <sys/stat.h>
#  include <time.h>
#  define cssc_getcwd getcwd
#  define cssc_mkdir(p) mkdir((p), 0777)
#endif

typedef unsigned char      u8;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef long long          i64;

#if defined(_WIN32)
#  define CSSC_EXPORT __declspec(dllexport)
#  define SYSV        __attribute__((sysv_abi))
#else
#  define CSSC_EXPORT __attribute__((visibility("default")))
#  define SYSV
#endif

static SYSV void serial_putc(char c) { fputc((unsigned char)c, stdout); }
static SYSV void serial_puts(const char *s) { while (*s) serial_putc(*s++); }

SYSV void *cssc_obj_alloc(i64 size) { return malloc((size_t)size); }
SYSV void  cssc_obj_free(void *p)   { free(p); }
SYSV u64   cssc_alloc_raw(u64 n)    { return (u64)cssc_obj_alloc((i64)n); }
SYSV void  cssc_free_raw(u64 p)     { cssc_obj_free((void *)p); }

SYSV u64  cssc_host_read(u64 buf, u64 max) { return (u64)fread((void *)buf, 1, (size_t)max, stdin); }

static char g_mod_path[1024];
SYSV void cssc_mod_path(u64 ptr, u64 len) {
    u64 i;
    if (len >= sizeof(g_mod_path)) len = sizeof(g_mod_path) - 1;
    for (i = 0; i < len; i++) g_mod_path[i] = ((char *)ptr)[i];
    g_mod_path[len] = 0;
}
SYSV void cssc_mod_path_inc(u64 ptr, u64 len) {
    const char *pre = "module/", *suf = ".cssc";
    char *name = (char *)ptr; u64 nl = len, i, k = 0;
    if (nl > 5 && name[0]=='c' && name[1]=='s' && name[2]=='s' && name[3]=='c' && name[4]=='.') { name += 5; nl -= 5; }
    for (i = 0; pre[i]; i++) g_mod_path[k++] = pre[i];
    for (i = 0; i < nl && k < sizeof(g_mod_path) - 8; i++) g_mod_path[k++] = name[i];
    for (i = 0; suf[i]; i++) g_mod_path[k++] = suf[i];
    g_mod_path[k] = 0;
}
SYSV i64 cssc_mod_read(u64 buf, u64 max) {
    FILE *f = fopen(g_mod_path, "rb");
    if (!f) return -1;
    size_t nr = fread((void *)buf, 1, (size_t)max, f);
    fclose(f);
    return (i64)nr;
}
SYSV void cssc_print_raw(u64 ptr, u64 len) { fwrite((const void *)ptr, 1, (size_t)len, stdout); fflush(stdout); }
SYSV void cssc_memfill32(u64 ptr, u64 val, u64 count) {
    u32 *p = (u32 *)ptr; u32 v = (u32)val;
    for (u64 i = 0; i < count; i++) p[i] = v;
}
SYSV void cssc_dbg(i64 n) { fprintf(stderr, "DBG %lld\n", (long long)n); fflush(stderr); }

SYSV void cssc_trap(void) { fflush(stdout); __builtin_trap(); }

SYSV i64 cssc_ftoi_checked(i64 bits) {
    union { i64 i; double d; } u; u.i = bits;
    i64 t = (i64)u.d;
    if ((double)t != u.d) { fflush(stdout); __builtin_trap(); }
    return t;
}
SYSV u64 cssc_str_to_f(u64 ptr, u64 len) {
    char tmp[128]; u64 n = len < 127 ? len : 127;
    for (u64 i = 0; i < n; i++) tmp[i] = ((const char *)ptr)[i];
    tmp[n] = 0;
    double d = strtod(tmp, (char **)0);
    u64 bits; __builtin_memcpy(&bits, &d, 8);
    return bits;
}

typedef struct { u32 refcount; u32 size; char data[]; } cssc_str;

SYSV void *cssc_string_lit(const char *src, i64 len) {
    u64 n = (u64)len;
    cssc_str *s = (cssc_str *)cssc_obj_alloc((i64)(8 + n + 1));
    s->refcount = 1;
    s->size = (u32)n;
    for (u64 i = 0; i < n; i++) s->data[i] = src[i];
    s->data[n] = 0;
    return s;
}
SYSV void  cssc_string_free(void *s) { cssc_obj_free(s); }
SYSV u32   cssc_string_size(void *s) { return ((cssc_str *)s)->size; }

SYSV i64   cssc_string_len_i64(void *s) { return (i64)((cssc_str *)s)->size; }

SYSV void *cssc_string_char_at(void *s, i64 i) {
    cssc_str *x = (cssc_str *)s;
    if (!x || i < 0 || (u64)i >= x->size) return cssc_string_lit("", 0);
    return cssc_string_lit(&x->data[i], 1);
}
SYSV void *cssc_string_data(void *s) { return (void *)((cssc_str *)s)->data; }
SYSV void *cssc_string_concat(void *a, void *b) {
    cssc_str *x = (cssc_str *)a, *y = (cssc_str *)b;
    u64 tot = (u64)x->size + (u64)y->size;
    cssc_str *r = (cssc_str *)cssc_obj_alloc((i64)(8 + tot + 1));
    r->refcount = 1; r->size = (u32)tot;
    for (u32 i = 0; i < x->size; i++) r->data[i] = x->data[i];
    for (u32 i = 0; i < y->size; i++) r->data[x->size + i] = y->data[i];
    r->data[tot] = 0;
    return r;
}
SYSV void *cssc_string_copy(void *s) {
    cssc_str *x = (cssc_str *)s;
    return cssc_string_lit(x->data, (i64)x->size);
}

SYSV i64 cssc_string_eq(void *a, void *b) {
    cssc_str *x = (cssc_str *)a, *y = (cssc_str *)b;
    if (x == y) return 1;
    if (!x || !y) return 0;
    if (x->size != y->size) return 0;
    for (u32 i = 0; i < x->size; i++) if (x->data[i] != y->data[i]) return 0;
    return 1;
}
SYSV void  cssc_retain(void *s)  { if (s) ((cssc_str *)s)->refcount++; }
SYSV void  cssc_release(void *s) { if (s) { cssc_str *x=(cssc_str*)s; if (x->refcount) x->refcount--; if (x->refcount==0) cssc_obj_free(s); } }

static SYSV i64 fmt_i64(char *buf, i64 v) {
    int neg = 0; u64 u; char tmp[24]; int ti = 0, bi = 0;
    if (v < 0) { neg = 1; u = (u64)(-(v + 1)) + 1u; } else { u = (u64)v; }
    if (u == 0) tmp[ti++] = '0';
    while (u) { tmp[ti++] = (char)('0' + (int)(u % 10u)); u /= 10u; }
    if (neg) buf[bi++] = '-';
    while (ti) buf[bi++] = tmp[--ti];
    return bi;
}
SYSV void *cssc_int_to_str(i64 n)  { char b[24]; i64 l = fmt_i64(b, n); return cssc_string_lit(b, l); }
SYSV void *cssc_bool_to_str(int b) { return b ? cssc_string_lit("true", 4) : cssc_string_lit("false", 5); }

static SYSV i64 fmt_f64(char *buf, double x) {
    return (i64)cssc_fmt_f64_shortest(buf, x);
}
SYSV void *cssc_float_to_str(double x) { char b[48]; i64 l = fmt_f64(b, x); return cssc_string_lit(b, l); }

SYSV void *cssc_auto_to_str(i64 val, i64 type) {
    if (type == 1) { if (val) cssc_retain((void *)val); return (void *)val; }
    if (type == 2) { union { i64 i; double d; } u; u.i = val; return cssc_float_to_str(u.d); }
    if (type == 3) return cssc_bool_to_str((int)val);
    return cssc_int_to_str(val);
}

SYSV void cssc_print_newline(void) { serial_putc('\n'); }
SYSV void cssc_out_int(i64 n)      { char b[24]; i64 l = fmt_i64(b, n); for (i64 i=0;i<l;i++) serial_putc(b[i]); fflush(stdout); }
SYSV void cssc_out_float(double f) { char b[48]; i64 l = fmt_f64(b, f); for (i64 i=0;i<l;i++) serial_putc(b[i]); }
SYSV void cssc_out_bool(int bb)    { serial_puts(bb ? "true" : "false"); }
SYSV void cssc_out_str(const char *s, i64 len) { for (i64 i=0;i<len;i++) serial_putc(s[i]); }
SYSV void cssc_out_string(void *s) { cssc_str *x = (cssc_str *)s; for (u32 i=0;i<x->size;i++) serial_putc(x->data[i]); }
SYSV void cssc_print_int(i64 n)       { cssc_out_int(n);    serial_putc('\n'); }
SYSV void cssc_print_float(double f)  { cssc_out_float(f);  serial_putc('\n'); }
SYSV void cssc_print_bool(int bb)     { cssc_out_bool(bb);  serial_putc('\n'); }
SYSV void cssc_print_str(const char *s, i64 len) { cssc_out_str(s, len); serial_putc('\n'); }
SYSV void cssc_print_string(void *s)  { cssc_out_string(s); serial_putc('\n'); }

SYSV void cssc_out_null(void)   { serial_puts("0x0"); }
SYSV void cssc_print_null(void) { serial_puts("0x0"); serial_putc('\n'); }

typedef struct { i64 len; i64 cap; i64 *data; i64 *types; } cssc_vec;

SYSV void *cssc_vec_new(i64 cap) {
    if (cap < 4) cap = 4;
    cssc_vec *v = (cssc_vec *)cssc_obj_alloc((i64)sizeof(cssc_vec));
    v->len = 0; v->cap = cap;
    v->data = (i64 *)cssc_obj_alloc(cap * 8);
    v->types = (i64 *)cssc_obj_alloc(cap * 8);
    return v;
}
SYSV void cssc_vec_push(void *p, i64 x) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return;
    if (v->len == v->cap) {
        i64 nc = v->cap * 2;
        i64 *nd = (i64 *)cssc_obj_alloc(nc * 8);
        i64 *nt = (i64 *)cssc_obj_alloc(nc * 8);
        for (i64 i = 0; i < v->len; i++) { nd[i] = v->data[i]; nt[i] = v->types[i]; }
        cssc_obj_free(v->data); cssc_obj_free(v->types);
        v->data = nd; v->types = nt; v->cap = nc;
    }
    v->types[v->len] = 0;
    v->data[v->len++] = x;
}
SYSV void cssc_vec_push_typed(void *p, i64 x, i64 type) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return;
    if (v->len == v->cap) {
        i64 nc = v->cap * 2;
        i64 *nd = (i64 *)cssc_obj_alloc(nc * 8);
        i64 *nt = (i64 *)cssc_obj_alloc(nc * 8);
        for (i64 i = 0; i < v->len; i++) { nd[i] = v->data[i]; nt[i] = v->types[i]; }
        cssc_obj_free(v->data); cssc_obj_free(v->types);
        v->data = nd; v->types = nt; v->cap = nc;
    }
    v->types[v->len] = type;
    v->data[v->len++] = x;
}
SYSV i64 cssc_vec_type_at(void *p, i64 i) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || i < 0 || i >= v->len) return 0;
    return v->types[i];
}
SYSV i64 cssc_vec_at(void *p, i64 i) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || i < 0 || i >= v->len) return 0;
    return v->data[i];
}
SYSV i64 cssc_vec_size(void *p) { cssc_vec *v = (cssc_vec *)p; return v ? v->len : 0; }
SYSV void cssc_vec_set(void *p, i64 i, i64 x) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || i < 0 || i >= v->len) return;
    v->data[i] = x;
}

SYSV i64 cssc_vec_pop_front(void *p) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || v->len == 0) return 0;
    i64 r = v->data[0];
    for (i64 i = 1; i < v->len; i++) { v->data[i - 1] = v->data[i]; v->types[i - 1] = v->types[i]; }
    v->len--;
    return r;
}
SYSV i64 cssc_vec_pop_back(void *p) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || v->len == 0) return 0;
    v->len--;
    return v->data[v->len];
}

typedef struct { i64 len; i64 cap; void **keys; i64 *vals; } cssc_map;

SYSV void *cssc_map_new(i64 cap) {
    if (cap < 4) cap = 4;
    cssc_map *m = (cssc_map *)cssc_obj_alloc((i64)sizeof(cssc_map));
    m->len = 0; m->cap = cap;
    m->keys = (void **)cssc_obj_alloc(cap * 8);
    m->vals = (i64 *)cssc_obj_alloc(cap * 8);
    return m;
}
SYSV void cssc_map_set(void *p, void *k, i64 v) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return;
    for (i64 i = 0; i < m->len; i++)
        if (cssc_string_eq(m->keys[i], k)) { m->vals[i] = v; return; }
    if (m->len == m->cap) {
        i64 nc = m->cap * 2;
        void **nk = (void **)cssc_obj_alloc(nc * 8);
        i64 *nv = (i64 *)cssc_obj_alloc(nc * 8);
        for (i64 i = 0; i < m->len; i++) { nk[i] = m->keys[i]; nv[i] = m->vals[i]; }
        cssc_obj_free(m->keys); cssc_obj_free(m->vals);
        m->keys = nk; m->vals = nv; m->cap = nc;
    }
    m->keys[m->len] = k; m->vals[m->len] = v; m->len++;
}
SYSV i64 cssc_map_get(void *p, void *k) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return 0;
    for (i64 i = 0; i < m->len; i++)
        if (cssc_string_eq(m->keys[i], k)) return m->vals[i];
    return 0;
}
SYSV i64 cssc_map_size(void *p) { cssc_map *m = (cssc_map *)p; return m ? m->len : 0; }
SYSV i64 cssc_map_has(void *p, void *k) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return 0;
    for (i64 i = 0; i < m->len; i++) if (cssc_string_eq(m->keys[i], k)) return 1;
    return 0;
}

SYSV void cssc_vec_free(void *p, i64 elemIsStr) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return;
    if (elemIsStr) for (i64 i = 0; i < v->len; i++) cssc_release((void *)v->data[i]);
    cssc_obj_free(v->data);
    cssc_obj_free(v->types);
    cssc_obj_free(v);
}
SYSV void cssc_map_free(void *p, i64 valIsStr) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return;
    for (i64 i = 0; i < m->len; i++) {
        cssc_release(m->keys[i]);
        if (valIsStr) cssc_release((void *)m->vals[i]);
    }
    cssc_obj_free(m->keys);
    cssc_obj_free(m->vals);
    cssc_obj_free(m);
}

SYSV void *cssc_vec_copy(void *p, i64 elemIsStr) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return cssc_vec_new(4);
    cssc_vec *c = (cssc_vec *)cssc_vec_new(v->cap);
    for (i64 i = 0; i < v->len; i++) {
        i64 cell = v->data[i];
        c->data[i] = (elemIsStr && cell) ? (i64)cssc_string_copy((void *)cell)
                                         : cell;
        c->types[i] = v->types[i];
    }
    c->len = v->len;
    return c;
}
SYSV void *cssc_map_copy(void *p, i64 valIsStr) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return cssc_map_new(4);
    cssc_map *c = (cssc_map *)cssc_map_new(m->cap);
    for (i64 i = 0; i < m->len; i++) {
        c->keys[i] = m->keys[i] ? cssc_string_copy(m->keys[i]) : 0;
        i64 cell = m->vals[i];
        c->vals[i] = (valIsStr && cell) ? (i64)cssc_string_copy((void *)cell)
                                        : cell;
    }
    c->len = m->len;
    return c;
}

SYSV void cssc_rt_sleep_ms(u64 ms) {
#if defined(_WIN32)
    Sleep((unsigned int)ms);
#else
    struct timespec ts;
    ts.tv_sec  = (long)(ms / 1000);
    ts.tv_nsec = (long)((ms % 1000) * 1000000L);
    nanosleep(&ts, 0);
#endif
}

static double cssc_b2d(u64 b) { double d; __builtin_memcpy(&d, &b, 8); return d; }
static u64    cssc_d2b(double d) { u64 b; __builtin_memcpy(&b, &d, 8); return b; }
SYSV u64 cssc_math_sqrt(u64 x)        { return cssc_d2b(sqrt(cssc_b2d(x))); }
SYSV u64 cssc_math_sin(u64 x)         { return cssc_d2b(sin(cssc_b2d(x))); }
SYSV u64 cssc_math_cos(u64 x)         { return cssc_d2b(cos(cssc_b2d(x))); }
SYSV u64 cssc_math_tan(u64 x)         { return cssc_d2b(tan(cssc_b2d(x))); }
SYSV u64 cssc_math_log(u64 x)         { return cssc_d2b(log(cssc_b2d(x))); }
SYSV u64 cssc_math_exp(u64 x)         { return cssc_d2b(exp(cssc_b2d(x))); }
SYSV u64 cssc_math_abs(u64 x)         { return cssc_d2b(fabs(cssc_b2d(x))); }
SYSV u64 cssc_math_floor(u64 x)       { return (u64)(i64)floor(cssc_b2d(x)); }
SYSV u64 cssc_math_ceil(u64 x)        { return (u64)(i64)ceil(cssc_b2d(x)); }
SYSV u64 cssc_math_pow(u64 x, u64 y)  { return cssc_d2b(pow(cssc_b2d(x), cssc_b2d(y))); }
SYSV u64 cssc_math_min(u64 x, u64 y)  { double a = cssc_b2d(x), b = cssc_b2d(y); return cssc_d2b(a < b ? a : b); }
SYSV u64 cssc_math_max(u64 x, u64 y)  { double a = cssc_b2d(x), b = cssc_b2d(y); return cssc_d2b(a > b ? a : b); }

static int paths_drive(const char *d, int n) { return (n >= 2 && d[1] == ':') ? 2 : 0; }
static int paths_bstart(const char *d, int n, int start) {
    for (int i = n - 1; i >= start; i--) if (d[i] == '/' || d[i] == '\\') return i + 1;
    return start;
}
SYSV void *cssc_paths_basename(void *p) {
    cssc_str *s = (cssc_str *)p; if (!s) return cssc_string_lit("", 0);
    const char *d = s->data; int n = (int)s->size;
    int b = paths_bstart(d, n, paths_drive(d, n));
    return cssc_string_lit(d + b, n - b);
}
SYSV void *cssc_paths_dirname(void *p) {
    cssc_str *s = (cssc_str *)p; if (!s) return cssc_string_lit("", 0);
    const char *d = s->data; int n = (int)s->size, start = paths_drive(d, n);
    int cut = -1;
    for (int i = n - 1; i >= start; i--) if (d[i] == '/' || d[i] == '\\') { cut = i; break; }
    if (cut < 0) return cssc_string_lit(d, start);
    int e = cut + 1;
    while (e > start + 1 && (d[e - 1] == '/' || d[e - 1] == '\\')) e--;
    return cssc_string_lit(d, e);
}
static int paths_dot(const char *d, int n, int b) {
    int dot = -1;
    for (int i = n - 1; i >= b; i--) if (d[i] == '.') { dot = i; break; }
    if (dot < 0) return -1;
    for (int i = b; i < dot; i++) if (d[i] != '.') return dot;
    return -1;
}
SYSV void *cssc_paths_ext(void *p) {
    cssc_str *s = (cssc_str *)p; if (!s) return cssc_string_lit("", 0);
    const char *d = s->data; int n = (int)s->size;
    int b = paths_bstart(d, n, paths_drive(d, n));
    int dot = paths_dot(d, n, b);
    if (dot < 0) return cssc_string_lit("", 0);
    return cssc_string_lit(d + dot, n - dot);
}
SYSV void *cssc_paths_stem(void *p) {
    cssc_str *s = (cssc_str *)p; if (!s) return cssc_string_lit("", 0);
    const char *d = s->data; int n = (int)s->size;
    int b = paths_bstart(d, n, paths_drive(d, n));
    int dot = paths_dot(d, n, b);
    if (dot < 0) return cssc_string_lit(d + b, n - b);
    return cssc_string_lit(d + b, dot - b);
}

SYSV void *cssc_nullable_str(void *p, i64 alive) {
    if (alive != 0) { if (p != 0) { return cssc_string_copy(p); } }
    return cssc_string_lit("", 0);
}

SYSV i64 cssc_bounds_check(i64 i, i64 len) {
    i64 hi = len - 1;
    i64 c = i;
    if (c > hi) { c = hi; }
    if (c < 0)  { c = 0; }
    return c;
}

static int cssc_peek_eq(i64 e, i64 val, i64 ek) {
    if (ek == 1) return (int)cssc_string_eq((void *)e, (void *)val);
    if (ek == 2) { double a = cssc_b2d((u64)e), b = cssc_b2d((u64)val); return a == b; }
    return e == val;
}
SYSV i64 cssc_vec_find_index(void *p, i64 val, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v) return -1;
    for (i64 i = 0; i < v->len; i++) if (cssc_peek_eq(v->data[i], val, ek)) return i;
    return -1;
}
SYSV i64 cssc_vec_find_last_index(void *p, i64 val, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v) return -1;
    for (i64 i = v->len - 1; i >= 0; i--) if (cssc_peek_eq(v->data[i], val, ek)) return i;
    return -1;
}
SYSV i64 cssc_vec_contains(void *p, i64 val, i64 ek) {
    return cssc_vec_find_index(p, val, ek) >= 0 ? 1 : 0;
}
SYSV i64 cssc_vec_count(void *p, i64 val, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v) return 0;
    i64 n = 0;
    for (i64 i = 0; i < v->len; i++) if (cssc_peek_eq(v->data[i], val, ek)) n++;
    return n;
}

SYSV i64 cssc_vec_sum(void *p, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v) return 0;
    if (ek == 2) {
        double s = 0;
        for (i64 i = 0; i < v->len; i++) s += cssc_b2d((u64)v->data[i]);
        return (i64)cssc_d2b(s);
    }
    i64 s = 0;
    for (i64 i = 0; i < v->len; i++) s += v->data[i];
    return s;
}

SYSV i64 cssc_vec_min(void *p, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v || v->len == 0 || ek == 1) return 0;
    if (ek == 2) {
        double m = cssc_b2d((u64)v->data[0]);
        for (i64 i = 1; i < v->len; i++) { double d = cssc_b2d((u64)v->data[i]); if (d < m) m = d; }
        return (i64)cssc_d2b(m);
    }
    i64 m = v->data[0];
    for (i64 i = 1; i < v->len; i++) if (v->data[i] < m) m = v->data[i];
    return m;
}
SYSV i64 cssc_vec_max(void *p, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v || v->len == 0 || ek == 1) return 0;
    if (ek == 2) {
        double m = cssc_b2d((u64)v->data[0]);
        for (i64 i = 1; i < v->len; i++) { double d = cssc_b2d((u64)v->data[i]); if (d > m) m = d; }
        return (i64)cssc_d2b(m);
    }
    i64 m = v->data[0];
    for (i64 i = 1; i < v->len; i++) if (v->data[i] > m) m = v->data[i];
    return m;
}

SYSV void *cssc_vec_slice(void *p, i64 start, i64 end) {
    cssc_vec *v = (cssc_vec *)p;
    void *rp = cssc_vec_new(4);
    cssc_vec *r = (cssc_vec *)rp;
    if (!v) return rp;
    i64 n = v->len;
    if (start < 0) { start += n; if (start < 0) start = 0; }
    if (start > n) start = n;
    if (end < 0) { end += n; if (end < 0) end = 0; }
    if (end > n) end = n;
    for (i64 i = start; i < end; i++) cssc_vec_push_typed(rp, v->data[i], v->types[i]);
    return rp;
}
SYSV void *cssc_vec_slice_clamp(void *p, i64 start, i64 end) {
    cssc_vec *v = (cssc_vec *)p;
    void *rp = cssc_vec_new(4);
    if (!v) return rp;
    i64 n = v->len;
    if (start < 0) start = 0;
    if (start > n) start = n;
    if (end < 0) end = 0;
    if (end > n) end = n;
    for (i64 i = start; i < end; i++) cssc_vec_push_typed(rp, v->data[i], v->types[i]);
    return rp;
}
SYSV void *cssc_vec_every_nth(void *p, i64 step) {
    cssc_vec *v = (cssc_vec *)p;
    void *rp = cssc_vec_new(4);
    if (!v) return rp;
    if (step < 1) step = 1;
    for (i64 i = 0; i < v->len; i += step) cssc_vec_push_typed(rp, v->data[i], v->types[i]);
    return rp;
}

SYSV void *cssc_vec_take_last(void *p, i64 n) {
    if (n == 0) return cssc_vec_new(4);
    cssc_vec *v = (cssc_vec *)p;
    return cssc_vec_slice(p, 0 - n, v ? v->len : 0);
}
SYSV void *cssc_vec_skip_last(void *p, i64 n) {
    cssc_vec *v = (cssc_vec *)p;
    if (n == 0) return cssc_vec_slice(p, 0, v ? v->len : 0);
    return cssc_vec_slice(p, 0, 0 - n);
}

SYSV void *cssc_vec_window(void *p, i64 idx, i64 size) {
    cssc_vec *v = (cssc_vec *)p; i64 len = v ? v->len : 0;
    i64 half = size / 2;
    i64 start = idx - half; if (start < 0) start = 0;
    i64 end = start + size; if (end > len) end = len;
    return cssc_vec_slice_clamp(p, start, end);
}
SYSV void *cssc_vec_around(void *p, i64 idx, i64 r) {
    cssc_vec *v = (cssc_vec *)p; i64 len = v ? v->len : 0;
    i64 start = idx - r; if (start < 0) start = 0;
    i64 end = idx + r + 1; if (end > len) end = len;
    return cssc_vec_slice_clamp(p, start, end);
}
SYSV void *cssc_vec_behind(void *p, i64 idx, i64 count) {
    i64 start = idx - count; if (start < 0) start = 0;
    return cssc_vec_slice_clamp(p, start, idx);
}
SYSV void *cssc_vec_cpeek(void *p, i64 idx, i64 amount) {
    cssc_vec *v = (cssc_vec *)p; i64 len = v ? v->len : 0;
    if (amount >= 0) {
        if (idx < len) return cssc_vec_slice_clamp(p, idx, idx + amount);
        return cssc_vec_new(4);
    }
    if (idx <= len) {
        i64 start = idx + amount; if (start < 0) start = 0;
        return cssc_vec_slice_clamp(p, start, idx);
    }
    return cssc_vec_new(4);
}

SYSV i64 cssc_vec_equals_next(void *p, i64 i, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v || i < 0 || i + 1 >= v->len) return 0;
    return cssc_peek_eq(v->data[i], v->data[i + 1], ek);
}
SYSV i64 cssc_vec_equals_prev(void *p, i64 i, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v || i - 1 < 0 || i >= v->len) return 0;
    return cssc_peek_eq(v->data[i], v->data[i - 1], ek);
}

static const char *stdio_cpath(void *p) { return p ? ((cssc_str *)p)->data : ""; }

static int cssc_rt_rmtree(const char *path) {
    if (!path || !path[0]) return 0;
#if defined(_WIN32)
    char pattern[4096];
    size_t pl = strlen(path);
    if (pl + 3 >= sizeof(pattern)) return 0;
    memcpy(pattern, path, pl);
    pattern[pl] = '\\'; pattern[pl + 1] = '*'; pattern[pl + 2] = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            const char *n = fd.cFileName;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            char child[4096];
            size_t nl = strlen(n);
            if (pl + 1 + nl >= sizeof(child)) continue;
            memcpy(child, path, pl);
            child[pl] = '\\';
            memcpy(child + pl + 1, n, nl + 1);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) cssc_rt_rmtree(child);
            else DeleteFileA(child);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return RemoveDirectoryA(path) ? 1 : 0;
#else
    DIR *d = opendir(path);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            const char *n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            char child[4096];
            snprintf(child, sizeof(child), "%s/%s", path, n);
            struct stat st;
            if (stat(child, &st) == 0 && S_ISDIR(st.st_mode)) cssc_rt_rmtree(child);
            else unlink(child);
        }
        closedir(d);
    }
    return rmdir(path) == 0 ? 1 : 0;
#endif
}

SYSV i64 cssc_rt_stdio_exists(void *path) {
    const char *p = stdio_cpath(path);
    FILE *f = fopen(p, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}
SYSV void *cssc_rt_stdio_read(void *path) {
    const char *p = stdio_cpath(path);
    FILE *f = fopen(p, "rb");
    if (!f) return cssc_string_lit("", 0);
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return cssc_string_lit("", 0); }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return cssc_string_lit("", 0); }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return cssc_string_lit("", 0); }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return cssc_string_lit("", 0); }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    void *s = cssc_string_lit(buf, (i64)rd);
    free(buf);
    return s;
}
SYSV i64 cssc_rt_stdio_write(void *path, void *content) {
    const char *p = stdio_cpath(path);
    FILE *f = fopen(p, "wb");
    if (!f) return 0;
    int ok = 1;
    if (content) {
        cssc_str *c = (cssc_str *)content;
        if (c->size > 0) {
            size_t w = fwrite(c->data, 1, (size_t)c->size, f);
            ok = (w == (size_t)c->size);
        }
    }
    fclose(f);
    return ok ? 1 : 0;
}
SYSV i64 cssc_rt_stdio_createfile(void *path) {
    const char *pin = stdio_cpath(path);
    size_t len = strlen(pin);
    if (len && len < 4096) {
        char tmp[4096];
        memcpy(tmp, pin, len + 1);
        size_t last_sep = 0;
        for (size_t i = 0; i < len; i++)
            if (tmp[i] == '/' || tmp[i] == '\\') last_sep = i;
        if (last_sep > 0) {
            tmp[last_sep] = 0;
            for (size_t i = 1; i < last_sep; i++) {
                char c = tmp[i];
                if (c == '/' || c == '\\') { tmp[i] = 0; if (tmp[0]) cssc_mkdir(tmp); tmp[i] = c; }
            }
            if (tmp[0]) cssc_mkdir(tmp);
        }
    }
    FILE *f = fopen(pin, "wb");
    if (!f) return 0;
    fclose(f);
    return 1;
}
SYSV void *cssc_rt_stdio_cwd(void) {
    char buf[4096];
    if (cssc_getcwd(buf, sizeof(buf)) == NULL) return cssc_string_lit("", 0);
    return cssc_string_lit(buf, (i64)strlen(buf));
}
SYSV void *cssc_rt_stdio_listdir(void *path) {
    const char *pin = stdio_cpath(path);
    if (!pin[0]) return cssc_string_lit("", 0);
    char *buf = NULL;
    size_t cap = 0, len = 0;
#if defined(_WIN32)
    char pattern[4096];
    size_t pl = strlen(pin);
    if (pl + 3 >= sizeof(pattern)) return cssc_string_lit("", 0);
    memcpy(pattern, pin, pl);
    pattern[pl] = '\\'; pattern[pl + 1] = '*'; pattern[pl + 2] = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            const char *n = fd.cFileName;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            size_t nl = strlen(n);
            if (len + nl + 2 > cap) {
                cap = (len + nl + 2) * 2 + 64;
                char *nb = (char *)realloc(buf, cap);
                if (!nb) { free(buf); FindClose(h); return cssc_string_lit("", 0); }
                buf = nb;
            }
            if (len > 0) buf[len++] = '\n';
            memcpy(buf + len, n, nl); len += nl;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(pin);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            const char *n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0))) continue;
            size_t nl = strlen(n);
            if (len + nl + 2 > cap) {
                cap = (len + nl + 2) * 2 + 64;
                char *nb = (char *)realloc(buf, cap);
                if (!nb) { free(buf); closedir(d); return cssc_string_lit("", 0); }
                buf = nb;
            }
            if (len > 0) buf[len++] = '\n';
            memcpy(buf + len, n, nl); len += nl;
        }
        closedir(d);
    }
#endif
    if (!buf) return cssc_string_lit("", 0);
    void *s = cssc_string_lit(buf, (i64)len);
    free(buf);
    return s;
}
SYSV i64 cssc_rt_stdio_removefile(void *path) {
    return remove(stdio_cpath(path)) == 0 ? 1 : 0;
}
SYSV i64 cssc_rt_stdio_createdir(void *path) {
    const char *pin = stdio_cpath(path);
    if (!pin[0]) return 0;
    char tmp[4096];
    size_t len = strlen(pin);
    if (len >= sizeof(tmp)) return 0;
    memcpy(tmp, pin, len + 1);
    for (size_t i = 1; i < len; i++) {
        char c = tmp[i];
        if (c == '/' || c == '\\') { tmp[i] = 0; if (tmp[0]) cssc_mkdir(tmp); tmp[i] = c; }
    }
    if (cssc_mkdir(tmp) == 0) return 1;
    return (errno == EEXIST) ? 1 : 0;
}
SYSV i64 cssc_rt_stdio_removedir(void *path) {
    return cssc_rt_rmtree(stdio_cpath(path));
}
SYSV i64 cssc_rt_stdio_move(void *src, void *dst) {
    const char *s = stdio_cpath(src), *d = stdio_cpath(dst);
    if (rename(s, d) == 0) return 1;
#if defined(_WIN32)
    if (MoveFileExA(s, d, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) return 1;
#endif
    return 0;
}

CSSC_EXPORT u64 cssc_rtsym(u64 id) {
    switch (id) {
        case 0:  return (u64)&cssc_print_int;
        case 1:  return (u64)&cssc_print_str;
        case 2:  return (u64)&cssc_out_string;
        case 3:  return (u64)&cssc_string_lit;
        case 4:  return (u64)&cssc_string_concat;
        case 5:  return (u64)&cssc_alloc_raw;
        case 6:  return (u64)&cssc_out_int;
        case 7:  return (u64)&cssc_print_string;
        case 8:  return (u64)&cssc_print_newline;
        case 9:  return (u64)&cssc_print_bool;
        case 10: return (u64)&cssc_out_bool;
        case 11: return (u64)&cssc_int_to_str;
        case 12: return (u64)&cssc_bool_to_str;
        case 13: return (u64)&cssc_string_free;
        case 14: return (u64)&cssc_string_copy;
        case 15: return (u64)&cssc_retain;
        case 16: return (u64)&cssc_release;
        case 17: return (u64)&cssc_out_str;
        case 18: return (u64)&cssc_float_to_str;
        case 19: return (u64)&cssc_string_eq;
        case 20: return (u64)&cssc_print_float;
        case 21: return (u64)&cssc_out_float;
        case 22: return (u64)&cssc_vec_new;
        case 23: return (u64)&cssc_vec_push;
        case 24: return (u64)&cssc_vec_at;
        case 25: return (u64)&cssc_vec_size;
        case 26: return (u64)&cssc_vec_set;
        case 27: return (u64)&cssc_map_new;
        case 28: return (u64)&cssc_map_set;
        case 29: return (u64)&cssc_map_get;
        case 30: return (u64)&cssc_map_size;
        case 31: return (u64)&cssc_map_has;
        case 32: return (u64)&cssc_vec_pop_front;
        case 33: return (u64)&cssc_vec_pop_back;
        case 34: return (u64)&cssc_vec_free;
        case 35: return (u64)&cssc_map_free;
        case 36: return (u64)&cssc_vec_copy;
        case 37: return (u64)&cssc_map_copy;
        case 38: return (u64)&cssc_print_null;
        case 39: return (u64)&cssc_out_null;
        case 40: return (u64)&cssc_string_char_at;
        case 41: return (u64)&cssc_string_len_i64;
        case 42: return (u64)&cssc_host_read;
        case 43: return (u64)&cssc_free_raw;
        case 44: return (u64)&cssc_print_raw;
        case 45: return (u64)&cssc_memfill32;
        case 46: return (u64)&cssc_str_to_f;
        case 47: return (u64)&cssc_dbg;
        case 48: return (u64)&cssc_vec_push_typed;
        case 49: return (u64)&cssc_vec_type_at;
        case 50: return (u64)&cssc_auto_to_str;
        case 51: return (u64)&cssc_trap;
        case 52: return (u64)&cssc_ftoi_checked;
        case 53: return (u64)&cssc_mod_path;
        case 54: return (u64)&cssc_mod_read;
        case 55: return (u64)&cssc_mod_path_inc;
        case 56: return (u64)&cssc_rt_sleep_ms;
        case 57: return (u64)&cssc_math_sqrt;
        case 58: return (u64)&cssc_math_sin;
        case 59: return (u64)&cssc_math_cos;
        case 60: return (u64)&cssc_math_tan;
        case 61: return (u64)&cssc_math_log;
        case 62: return (u64)&cssc_math_exp;
        case 63: return (u64)&cssc_math_abs;
        case 64: return (u64)&cssc_math_floor;
        case 65: return (u64)&cssc_math_ceil;
        case 66: return (u64)&cssc_math_pow;
        case 67: return (u64)&cssc_math_min;
        case 68: return (u64)&cssc_math_max;
        case 69: return (u64)&cssc_paths_dirname;
        case 70: return (u64)&cssc_paths_basename;
        case 71: return (u64)&cssc_paths_ext;
        case 72: return (u64)&cssc_paths_stem;
        case 73: return (u64)&cssc_rt_stdio_write;
        case 74: return (u64)&cssc_rt_stdio_read;
        case 75: return (u64)&cssc_rt_stdio_exists;
        case 76: return (u64)&cssc_rt_stdio_createdir;
        case 77: return (u64)&cssc_rt_stdio_createfile;
        case 78: return (u64)&cssc_rt_stdio_removefile;
        case 79: return (u64)&cssc_rt_stdio_removedir;
        case 80: return (u64)&cssc_rt_stdio_move;
        case 81: return (u64)&cssc_rt_stdio_cwd;
        case 82: return (u64)&cssc_rt_stdio_listdir;
        case 83: return (u64)&cssc_nullable_str;
        case 84: return (u64)&cssc_bounds_check;
        case 85: return (u64)&cssc_vec_find_index;
        case 86: return (u64)&cssc_vec_find_last_index;
        case 87: return (u64)&cssc_vec_contains;
        case 88: return (u64)&cssc_vec_count;
        case 89: return (u64)&cssc_vec_sum;
        case 90: return (u64)&cssc_vec_min;
        case 91: return (u64)&cssc_vec_max;
        case 92: return (u64)&cssc_vec_equals_next;
        case 93: return (u64)&cssc_vec_equals_prev;
        case 94: return (u64)&cssc_vec_slice;
        case 95: return (u64)&cssc_vec_every_nth;
        case 96: return (u64)&cssc_vec_slice_clamp;
        case 97: return (u64)&cssc_vec_take_last;
        case 98: return (u64)&cssc_vec_skip_last;
        case 99: return (u64)&cssc_vec_window;
        case 100: return (u64)&cssc_vec_around;
        case 101: return (u64)&cssc_vec_behind;
        case 102: return (u64)&cssc_vec_cpeek;
        default: return 0;
    }
}

CSSC_EXPORT void cssc_rt_call_entry(u64 fn) { ((void (SYSV *)(void))fn)(); }

CSSC_EXPORT void cssc_rt_flush(void) { fflush(stdout); }
