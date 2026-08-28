/* ==========================================================================
 * cssc_rt_host.c -- HOST validation runtime for the self-hosted Transembly
 * compiler (CCOS project). Re-exports the SAME `cssc_*` ABI as the resident
 * CCOS kernel runtime `cssc_ccos_rt.c`, but backed by the host libc:
 *     serial_putc  -> fputc(stdout)
 *     cssc_obj_alloc -> malloc
 * so that machine code emitted by our compiler (compiler/x86.cssc) can be
 * loaded into executable memory on the host and RUN, with its cssc_* calls
 * resolved to these symbols -- the fast host-first behavioral-parity gate
 * (see harness/run.py). The string ABI is byte-identical:
 *     struct cssc_str { u32 refcount; u32 size; char data[]; }  (data at +8)
 * fmt_i64 / fmt_f64 are copied verbatim so numeric output matches the kernel
 * (and therefore matches `cssc run` for the parity comparison).
 *
 * CALLING CONVENTION (critical): our emitted code + the CCOS runtime use the
 * System V AMD64 ABI (args rdi/rsi/rdx/rcx/r8/r9). Windows uses Microsoft x64
 * (args rcx/rdx/r8/r9). So every runtime function our SysV code calls is
 * marked SYSV (__attribute__((sysv_abi))) on Windows so the host DLL matches
 * the CCOS convention and our backend can always emit SysV. The three
 * ctypes-facing entry points (cssc_rtsym, cssc_rt_call_entry, cssc_rt_flush)
 * stay in the platform-default (MS x64) convention so Python/ctypes can call
 * them; cssc_rt_call_entry is the MS-x64 -> SysV trampoline that invokes the
 * emitted program's entry. On CCOS everything is natively SysV (freestanding),
 * so SYSV is a no-op there and cssc_call_addr is the trivial trampoline.
 *
 * Build (host DLL, clang):
 *   clang -shared -O2 cssc_rt_host.c -o cssc_rt_host.dll   (or .so on posix)
 *
 * Symbol resolution: the compiler records each runtime call target by a fixed
 * small SYMBOL ID; `cssc_rtsym(id)` returns the function address. The harness
 * (or, on CCOS, the compiler itself) patches the emitted `mov rax, imm64`
 * placeholder with that address. Keep the id table APPEND-ONLY and identical
 * to the CCOS-side `cssc_rtsym` so one relocation scheme serves both targets.
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "cssc_fmt_f64.h" /* shared shortest-round-trip f64 formatter */

/* Platform file-system + timing headers for the `stdio` module rtsyms (73-82).
 * Pulled in once, up front, with the lean/no-minmax guards so nothing collides
 * with the runtime's lowercase identifiers. This also provides Sleep() for the
 * cssc::sleep rtsym below (no separate forward-decl needed). */
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

/* ---- backing primitives (the only host-specific pieces) ------------------ */
static SYSV void serial_putc(char c) { fputc((unsigned char)c, stdout); }
static SYSV void serial_puts(const char *s) { while (*s) serial_putc(*s++); }

SYSV void *cssc_obj_alloc(i64 size) { return malloc((size_t)size); }
SYSV void  cssc_obj_free(void *p)   { free(p); }
SYSV u64   cssc_alloc_raw(u64 n)    { return (u64)cssc_obj_alloc((i64)n); }
SYSV void  cssc_free_raw(u64 p)     { cssc_obj_free((void *)p); }

/* ---- host I/O + raw-memory primitives the SELF-HOSTED COMPILER is written in.
 * The reference toolchain compiles these to its own runtime; when OUR compiler
 * emits them (rtsyms 42-46) the host build (harness/run.py) resolves them here so
 * a compiler-compiled-by-our-compiler (stage-2) can read stdin, write its TXB1
 * container, fill its tables, and parse float literals. */
SYSV u64  cssc_host_read(u64 buf, u64 max) { return (u64)fread((void *)buf, 1, (size_t)max, stdin); }
/* Module/#load file read, split into two 2-arg calls so it fits the runtime-call
 * machinery: cssc::modPath(ptr,len) records the path span, cssc::modRead(buf,max)
 * opens it and reads into buf (returns bytes read, or -1 if it won't open). */
static char g_mod_path[1024];
SYSV void cssc_mod_path(u64 ptr, u64 len) {          /* #load[path]: path used verbatim */
    u64 i;
    if (len >= sizeof(g_mod_path)) len = sizeof(g_mod_path) - 1;
    for (i = 0; i < len; i++) g_mod_path[i] = ((char *)ptr)[i];
    g_mod_path[len] = 0;
}
SYSV void cssc_mod_path_inc(u64 ptr, u64 len) {      /* #include('name'): module/<strip cssc.>.cssc */
    const char *pre = "module/", *suf = ".cssc";
    char *name = (char *)ptr; u64 nl = len, i, k = 0;
    if (nl > 5 && name[0]=='c' && name[1]=='s' && name[2]=='s' && name[3]=='c' && name[4]=='.') { name += 5; nl -= 5; }
    for (i = 0; pre[i]; i++) g_mod_path[k++] = pre[i];
    for (i = 0; i < nl && k < sizeof(g_mod_path) - 8; i++) g_mod_path[k++] = name[i];
    for (i = 0; suf[i]; i++) g_mod_path[k++] = suf[i];
    g_mod_path[k] = 0;
}
SYSV i64 cssc_mod_read(u64 buf, u64 max) {
    /* `module/<name>.cssc` is resolved relative to the CWD first (the dev/parity
     * workflow runs from the transembly dir). If that misses, fall back to
     * $CSSC_MODULE_DIR/module/<name>.cssc so a shipped toolchain / a build from
     * an arbitrary project dir still finds the bundled CSSC modules. */
    FILE *f = fopen(g_mod_path, "rb");
    if (!f) {
        const char *base = getenv("CSSC_MODULE_DIR");
        if (base && base[0]) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", base, g_mod_path);
            f = fopen(full, "rb");
        }
    }
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
/* Strict-typing fault (e.g. assigning a float to an int variable). Flush stdout
 * FIRST so the program's preceding output is observed, then fault -- matching the
 * reference, which prints its output and then raises the type error. */
SYSV void cssc_trap(void) { fflush(stdout); __builtin_trap(); }
/* Assign a float to a strict int variable: the reference truncates a WHOLE value
 * (`n = n/2` on an even n) but raises a type error on a FRACTIONAL one. `bits` is
 * the f64 reinterpreted as i64 (our float SSAs live in GP regs). */
SYSV i64 cssc_ftoi_checked(i64 bits) {
    union { i64 i; double d; } u; u.i = bits;
    i64 t = (i64)u.d;
    if ((double)t != u.d) { fflush(stdout); __builtin_trap(); }
    return t;
}
/* Bit-EXACT #stack/#heap[int|bool, N] guard -- CSSC's embedded contract (no
 * hidden operations): a scalar init value that needs more bytes than the
 * declared N-bit slot is a HARD runtime error, never a silent widening. Byte-
 * for-byte the interpreter's model (_encode_value): a full sign byte is always
 * reserved, so nbytes = bitlen(|v|)/8 + 1, max_bytes = N/8; overflow when
 * nbytes > max_bytes, reported as `<nbytes*8> > <N> bits`. Returns the value
 * unchanged on success so the caller may chain the checked value. */
SYSV i64 cssc_bit_check(i64 value, i64 max_bits, i64 is_heap) {
    unsigned long long mag = value < 0 ? (unsigned long long)(-value)
                                       : (unsigned long long)value;
    int bl = 0;
    while (mag) { bl++; mag >>= 1; }
    long long nbytes = (long long)(bl / 8) + 1;
    long long max_bytes = max_bits / 8;
    if (nbytes > max_bytes) {
        const char *d = is_heap ? "#heap" : "#stack";
        fflush(stdout);
        fprintf(stderr,
                "cssc: fatal error: CSSC Runtime Error: %s: value exceeds bit "
                "limit (%lld > %lld bits)\n"
                "  Hint: Increase bit limit or reduce value size\n",
                d, (long long)(nbytes * 8), (long long)max_bits);
        fflush(stderr);
        exit(1);
    }
    return value;
}
SYSV u64 cssc_str_to_f(u64 ptr, u64 len) {
    char tmp[128]; u64 n = len < 127 ? len : 127;
    for (u64 i = 0; i < n; i++) tmp[i] = ((const char *)ptr)[i];
    tmp[n] = 0;
    double d = strtod(tmp, (char **)0);
    u64 bits; __builtin_memcpy(&bits, &d, 8);
    return bits;
}

/* ---- string heap kind (layout MUST match cssc_ccos_rt.c) ----------------- */
typedef struct { u32 refcount; u32 size; char data[]; } cssc_str;

/* DEFAULT (MS-x64) ABI, NOT SYSV: the gui/game/extras host C (compiled MS-ABI)
 * calls cssc_string_lit directly (e.g. cssc_gui_text_text), so it must match
 * their ABI. rt_host's own SYSV functions calling it are bridged by the compiler
 * (SysV->MS), and the SysV emitted code reaches it via the cssc_rt_string_lit
 * wrapper below (rtsym 3). */
void *cssc_string_lit(const char *src, i64 len) {
    u64 n = (u64)len;
    cssc_str *s = (cssc_str *)cssc_obj_alloc((i64)(8 + n + 1));
    s->refcount = 1;
    s->size = (u32)n;
    for (u64 i = 0; i < n; i++) s->data[i] = src[i];
    s->data[n] = 0;
    return s;
}
/* SysV entry for rtsym 3 (the emitted SysV code calls this; it forwards to the
 * MS-ABI cssc_string_lit -- the compiler emits the SysV->MS bridge). */
SYSV void *cssc_rt_string_lit(const char *src, i64 len) { return cssc_string_lit(src, len); }
SYSV void  cssc_string_free(void *s) { cssc_obj_free(s); }
SYSV u32   cssc_string_size(void *s) { return ((cssc_str *)s)->size; }
/* Same value, but returned as a full i64 so the compiler's `select` length
 * compare (a signed 64-bit `cursor < len`) can't be fooled by stale high bits
 * of a u32 return. rtsym 41 points here, not at cssc_string_size. */
SYSV i64   cssc_string_len_i64(void *s) { return (i64)((cssc_str *)s)->size; }
/* `select` over a STRING iterates its characters -- the interpreter does
 * list("abc") = ['a','b','c'], a list of fresh 1-char strings. This returns the
 * i-th character as a freshly-allocated 1-char cssc_str (refcount 1), so the
 * loop body can compare it (`char != "%"`), concat it, or copy it. Out of range
 * yields an empty string rather than trapping. */
SYSV void *cssc_string_char_at(void *s, i64 i) {
    cssc_str *x = (cssc_str *)s;
    if (!x || i < 0 || (u64)i >= x->size) return cssc_string_lit("", 0);
    return cssc_string_lit(&x->data[i], 1);
}
/* cssc::chr(n) -> a one-byte string with value n&0xFF; cssc::ord(s) -> the byte
 * value (0-255) of s's first byte, 0 if empty. The primitive int<->byte pair for
 * building/parsing binary buffers from pure CSSC. */
SYSV void *cssc_chr(i64 n) {
    char c = (char)(n & 0xFF);
    return cssc_string_lit(&c, 1);
}
SYSV i64 cssc_ord(void *s) {
    cssc_str *x = (cssc_str *)s;
    if (!x || x->size == 0) return 0;
    return (i64)(unsigned char)x->data[0];
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
/* String methods (match the interpreter's str.upper()/lower()/replace()). Each
 * returns a FRESH cssc_str* (refcount 1). ASCII-only case folding, matching the
 * reference (Python str.upper/lower on ASCII). rtsym ids 103-107. */
/* When gui is linked (CSSC_GUI_HOST), cssc_host_extras.c defines these 5 string
 * helpers too; exclude our copies to avoid a multiple-definition link error and
 * declare them extern so the rtsym switch (103-107) resolves to extras'
 * (behaviour-identical). The default (non-gui / --sh) DLL keeps our copies. */
#ifndef CSSC_GUI_HOST
SYSV void *cssc_string_upper(void *s) {
    cssc_str *x = (cssc_str *)s;
    cssc_str *r = (cssc_str *)cssc_obj_alloc((i64)(8 + (u64)x->size + 1));
    r->refcount = 1; r->size = x->size;
    for (u32 i = 0; i < x->size; i++) {
        char c = x->data[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        r->data[i] = c;
    }
    r->data[x->size] = 0;
    return r;
}
SYSV void *cssc_string_lower(void *s) {
    cssc_str *x = (cssc_str *)s;
    cssc_str *r = (cssc_str *)cssc_obj_alloc((i64)(8 + (u64)x->size + 1));
    r->refcount = 1; r->size = x->size;
    for (u32 i = 0; i < x->size; i++) {
        char c = x->data[i];
        if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
        r->data[i] = c;
    }
    r->data[x->size] = 0;
    return r;
}
/* replace ALL non-overlapping occurrences of `from` with `to` (Python
 * str.replace semantics). An empty `from` returns a copy (no infinite loop). */
SYSV void *cssc_string_replace(void *s, void *from, void *to) {
    cssc_str *x = (cssc_str *)s, *f = (cssc_str *)from, *t = (cssc_str *)to;
    if (!f || f->size == 0) return cssc_string_copy(s);
    /* count occurrences first so we can size the result exactly. */
    u64 count = 0;
    for (u64 i = 0; i + f->size <= x->size; ) {
        u64 j = 0;
        while (j < f->size && x->data[i + j] == f->data[j]) j++;
        if (j == f->size) { count++; i += f->size; } else { i++; }
    }
    u64 outn = (u64)x->size + count * ((u64)t->size - (u64)f->size);
    cssc_str *r = (cssc_str *)cssc_obj_alloc((i64)(8 + outn + 1));
    r->refcount = 1; r->size = (u32)outn;
    u64 w = 0;
    for (u64 i = 0; i < x->size; ) {
        if (i + f->size <= x->size) {
            u64 j = 0;
            while (j < f->size && x->data[i + j] == f->data[j]) j++;
            if (j == f->size) {
                for (u32 k = 0; k < t->size; k++) r->data[w++] = t->data[k];
                i += f->size;
                continue;
            }
        }
        r->data[w++] = x->data[i++];
    }
    r->data[outn] = 0;
    return r;
}
/* substring containment -> 1/0 (str.contains / `in`). */
SYSV i64 cssc_string_contains(void *s, void *sub) {
    cssc_str *x = (cssc_str *)s, *n = (cssc_str *)sub;
    if (!n || n->size == 0) return 1;
    if (!x || x->size < n->size) return 0;
    for (u64 i = 0; i + n->size <= x->size; i++) {
        u64 j = 0;
        while (j < n->size && x->data[i + j] == n->data[j]) j++;
        if (j == n->size) return 1;
    }
    return 0;
}
/* trim leading+trailing ASCII whitespace (str.strip) -> fresh cssc_str*. */
SYSV void *cssc_string_trim(void *s) {
    cssc_str *x = (cssc_str *)s;
    u64 a = 0, b = x->size;
    while (a < b) { char c = x->data[a]; if (c==' '||c=='\t'||c=='\n'||c=='\r') a++; else break; }
    while (b > a) { char c = x->data[b-1]; if (c==' '||c=='\t'||c=='\n'||c=='\r') b--; else break; }
    return cssc_string_lit(&x->data[a], (i64)(b - a));
}
#else
extern void *cssc_string_upper(void *s);
extern void *cssc_string_lower(void *s);
extern void *cssc_string_replace(void *s, void *from, void *to);
extern i64   cssc_string_contains(void *s, void *sub);
extern void *cssc_string_trim(void *s);
#endif
/* Content equality for two heap strings -> 1/0. `==` on strings must compare
 * TEXT, not pointers (two equal strings are separate allocations). */
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

/* ---- number/bool -> string (verbatim layout from cssc_ccos_rt.c) --------- */
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
/* SHORTEST ROUND-TRIP formatting (Dragon4 + Python-repr presentation) lives in
 * cssc_fmt_f64.h so the host and the CCOS kernel runtime share ONE implementation.
 * The previous integer-part + 6-fixed-digit loop could never match the reference
 * (which prints `2.0`, `1.6`, `0.3333333333333333`, `1e-05`). */
static SYSV i64 fmt_f64(char *buf, double x) {
    return (i64)cssc_fmt_f64_shortest(buf, x);
}
SYSV void *cssc_float_to_str(double x) { char b[48]; i64 l = fmt_f64(b, x); return cssc_string_lit(b, l); }
/* vector<auto> read: render a stored cell (raw i64) to its string form per the
 * per-element kind tag (0 int / 1 string / 2 float-bits / 3 bool), so the caller
 * can print a heterogeneous element without a static type. */
SYSV void *cssc_auto_to_str(i64 val, i64 type) {
    if (type == 1) { if (val) cssc_retain((void *)val); return (void *)val; }
    if (type == 2) { union { i64 i; double d; } u; u.i = val; return cssc_float_to_str(u.d); }
    if (type == 3) return cssc_bool_to_str((int)val);
    return cssc_int_to_str(val);
}

/* ---- output routed to stdout (matches kernel's COM1 routing) ------------- */
SYSV void cssc_print_newline(void) { serial_putc('\n'); }
SYSV void cssc_out_int(i64 n)      { char b[24]; i64 l = fmt_i64(b, n); for (i64 i=0;i<l;i++) serial_putc(b[i]); fflush(stdout); }
SYSV void cssc_out_float(double f) { char b[48]; i64 l = fmt_f64(b, f); for (i64 i=0;i<l;i++) serial_putc(b[i]); }
SYSV void cssc_out_bool(int bb)    { serial_puts(bb ? "true" : "false"); }
SYSV void cssc_out_str(const char *s, i64 len) { for (i64 i=0;i<len;i++) serial_putc(s[i]); }
SYSV void cssc_out_string(void *s) { cssc_str *x = (cssc_str *)s; if (!x) { serial_puts("0x0"); return; } for (u32 i=0;i<x->size;i++) serial_putc(x->data[i]); }
SYSV void cssc_print_int(i64 n)       { cssc_out_int(n);    serial_putc('\n'); }
SYSV void cssc_print_float(double f)  { cssc_out_float(f);  serial_putc('\n'); }
SYSV void cssc_print_bool(int bb)     { cssc_out_bool(bb);  serial_putc('\n'); }
SYSV void cssc_print_str(const char *s, i64 len) { cssc_out_str(s, len); serial_putc('\n'); }
SYSV void cssc_print_string(void *s)  { cssc_out_string(s); serial_putc('\n'); }
/* Reading a name that was `#delete`d yields the literal text `0x0` -- the
 * UNBOUND state, which the reference distinguishes from a variable that merely
 * holds the value 0 (that one prints `0`). See docs/cssc-ownership.md section 6.
 * Type-independent on purpose: the compiler branches on the `alive` cell and
 * calls this instead of whichever typed printer it would otherwise use. */
SYSV void cssc_out_null(void)   { serial_puts("0x0"); }
SYSV void cssc_print_null(void) { serial_puts("0x0"); serial_putc('\n'); }

/* ==========================================================================
 * M7 containers -- growable vector of raw 64-bit cells. An element is just an
 * i64: an integer, an f64 bit pattern, or a heap pointer (string/vector). The
 * COMPILER tracks which, exactly as it does for slots, so the runtime stays
 * type-agnostic. Out-of-range reads return 0 rather than trapping (warn-not-
 * reject). MUST stay in lock-step with the copy in cssc_ccos_rt.c.
 * ========================================================================== */
/* `types` runs parallel to `data`: for a homogeneous vector every entry is 0
 * (the compiler already knows the element kind statically); for `vector<auto>`
 * each push records the element's kind (0 int / 1 string / 2 float / 3 bool) so
 * a later read can dispatch the right print at runtime. */
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
    if (v->len == v->cap) {                       /* grow: allocate, copy, free */
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

/* Remove and return the first / last element. Empty -> 0 (warn-not-reject).
 * pop_front shifts left; vectors here are short, so a memmove-style loop is
 * cheaper than maintaining a head index. */
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

/* Map with heap-string keys. Linear probe over parallel key/value arrays --
 * CSSC maps in practice hold a handful of entries, and a linear scan keeps the
 * runtime free of hashing/allocation policy. Keys compare by CONTENT
 * (cssc_string_eq), so a runtime-built key finds a literal-inserted one.
 * Missing key -> 0 (warn-not-reject). Lock-step with cssc_ccos_rt.c. */
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

/* Tear down a container. The runtime cannot know whether a 64-bit cell is an
 * integer or a string pointer, so the COMPILER passes elemIsStr and we release
 * the elements only when told to. Map keys are always strings. */
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

/* ---- container stringify: `{e0, e1, ...}` / `{k0: v0, ...}` -----------------
 * Byte-identical to the reference interpreter's list/dict repr (`, ` separator,
 * NO quotes on string elements, `key: value` map pairs, empty -> `{}`). Used by
 * cssc::out/outln on a vector or map. A small doubling char buffer avoids the
 * O(n^2) concat chain; each element is rendered via cssc_auto_to_str. */
static void cssc__sb_put(char **buf, i64 *len, i64 *cap, const char *s, i64 n) {
    if (*len + n + 1 > *cap) {
        i64 nc = *cap ? *cap : 32;
        while (*len + n + 1 > nc) nc *= 2;
        char *nb = (char *)cssc_obj_alloc(nc);
        for (i64 i = 0; i < *len; i++) nb[i] = (*buf)[i];
        if (*buf) cssc_obj_free(*buf);
        *buf = nb; *cap = nc;
    }
    for (i64 i = 0; i < n; i++) (*buf)[(*len)++] = s[i];
}
/* elemKind: 0 int / 1 string / 2 float / 3 bool applies to every element; 6 =
 * vector<auto>, use the per-element types[] tag. */
SYSV void *cssc_vec_to_str(void *p, i64 elemKind) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return cssc_string_lit("0x0", 3);
    char *buf = 0; i64 len = 0, cap = 0;
    cssc__sb_put(&buf, &len, &cap, "{", 1);
    for (i64 i = 0; i < v->len; i++) {
        if (i) cssc__sb_put(&buf, &len, &cap, ", ", 2);
        i64 k = (elemKind == 6) ? v->types[i] : elemKind;
        cssc_str *es = (cssc_str *)cssc_auto_to_str(v->data[i], k);
        if (es) { cssc__sb_put(&buf, &len, &cap, es->data, es->size); cssc_release(es); }
    }
    cssc__sb_put(&buf, &len, &cap, "}", 1);
    void *r = cssc_string_lit(buf, len);
    if (buf) cssc_obj_free(buf);
    return r;
}
/* Map keys are always strings; valKind types the values (0 int / 1 string /
 * 2 float / 3 bool). */
SYSV void *cssc_map_to_str(void *p, i64 valKind) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return cssc_string_lit("0x0", 3);
    char *buf = 0; i64 len = 0, cap = 0;
    cssc__sb_put(&buf, &len, &cap, "{", 1);
    for (i64 i = 0; i < m->len; i++) {
        if (i) cssc__sb_put(&buf, &len, &cap, ", ", 2);
        cssc_str *ks = (cssc_str *)m->keys[i];
        if (ks) cssc__sb_put(&buf, &len, &cap, ks->data, ks->size);
        cssc__sb_put(&buf, &len, &cap, ": ", 2);
        cssc_str *vs = (cssc_str *)cssc_auto_to_str(m->vals[i], valKind);
        if (vs) { cssc__sb_put(&buf, &len, &cap, vs->data, vs->size); cssc_release(vs); }
    }
    cssc__sb_put(&buf, &len, &cap, "}", 1);
    void *r = cssc_string_lit(buf, len);
    if (buf) cssc_obj_free(buf);
    return r;
}

/* Deep copy of a container -- this is the `&x` operator. Spec 2.5: `&` is
 * `cssc::copy`, a RECURSIVE deep copy, so element strings are copied too and
 * the result shares nothing with the source. Measured against the reference in
 * docs/cssc-ownership.md (p_copy_vec / p_copy_map): mutating the copy must
 * leave the original untouched. elemIsStr / valIsStr come from the compiler --
 * the only side that knows a 64-bit cell's static type. */
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
/* Values of a map as a fresh vector, in INSERTION ORDER (map entries are a
 * linear array appended at len), so `select (map) ?v { … }` iterates the values
 * exactly like the interpreter's `list(map.values())`. Strings are copied when
 * valIsStr so the snapshot owns them. */
SYSV void *cssc_map_values(void *p, i64 valIsStr) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return cssc_vec_new(4);
    i64 cap = m->len < 4 ? 4 : m->len;
    cssc_vec *c = (cssc_vec *)cssc_vec_new(cap);
    for (i64 i = 0; i < m->len; i++) {
        i64 cell = m->vals[i];
        c->data[i] = (valIsStr && cell) ? (i64)cssc_string_copy((void *)cell) : cell;
        c->types[i] = valIsStr ? 1 : 0;
    }
    c->len = m->len;
    return c;
}

/* cssc::sleep(ms): sleep the process for `ms` milliseconds. Uses Win32 Sleep
 * (from <windows.h>, included up top) / nanosleep (POSIX, <time.h> up top). */
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

/* cssc.math builtins (rtsyms 57-68). Floats live as f64 BIT PATTERNS in i64 gp
 * cells; the op-7 CALL passes them in rdi/rsi as raw integers and reads the
 * result from rax -- so each libm wrapper is bits-in / bits-out (reinterpret,
 * call libm, reinterpret). `sqrt/sin/.../abs/pow/min/max` return a FLOAT (the
 * compiler marks the result via txSetFloat); `floor/ceil` return an INT (the
 * reference oracle: `m::floor(3.7)` -> 3). Args are float-promoted by the
 * compiler before the call. */
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

/* Deterministic seeded RNG (rtsyms 108-109): splitmix64 hash of the seed, so
 * the same seed always yields the same value -- reproducible across the
 * interpreter, this runtime and transembly (honours CSSC's determinism). */
static u64 cssc_splitmix64(u64 seed) {
    u64 z = seed + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
/* random(seed) -> float in [0,1) (returned as its f64 bit pattern, gp-reg ABI) */
SYSV u64 cssc_math_random(i64 seed) {
    u64 h = cssc_splitmix64((u64)seed);
    double d = (double)(h >> 11) * (1.0 / 9007199254740992.0);
    return cssc_d2b(d);
}
/* randomint(seed, lo, hi) -> int in [lo, hi] */
SYSV i64 cssc_math_randomint(i64 seed, i64 lo, i64 hi) {
    if (hi < lo) { i64 t = lo; lo = hi; hi = t; }
    u64 h = cssc_splitmix64((u64)seed);
    return lo + (i64)(h % (u64)(hi - lo + 1));
}

/* cssc.paths string builtins (rtsyms 69-72): a cssc_str* in, a fresh cssc_str*
 * out. Match Windows os.path (ntpath): drive-aware split ('C:/x' -> dir 'C:/'),
 * splitext leading-dot rule ('.hidden' has no ext). Separators: '/' and '\\'. */
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
    if (cut < 0) return cssc_string_lit(d, start);      /* no sep -> drive (or "") */
    int e = cut + 1;                                    /* include the sep, then strip */
    while (e > start + 1 && (d[e - 1] == '/' || d[e - 1] == '\\')) e--;
    return cssc_string_lit(d, e);
}
static int paths_dot(const char *d, int n, int b) {     /* last '.' in basename, or -1 */
    int dot = -1;
    for (int i = n - 1; i >= b; i--) if (d[i] == '.') { dot = i; break; }
    if (dot < 0) return -1;
    for (int i = b; i < dot; i++) if (d[i] != '.') return dot;  /* has a non-dot before it */
    return -1;                                          /* leading/all dots -> no ext */
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

/* cssc.peek nullable-string coercion (rtsym 83): a vector<string> element read
 * out of bounds is null; the interpreter coerces a null in a STRING context to ""
 * (not `0x0`). Returns a fresh owned string: a copy of `p` when alive, else "". */
SYSV void *cssc_nullable_str(void *p, i64 alive) {
    if (alive != 0) { if (p != 0) { return cssc_string_copy(p); } }
    return cssc_string_lit("", 0);
}

/* cssc.peek bounds_check (rtsym 84): clamp index into [0, len-1] exactly like the
 * interpreter's max(0, min(index, len-1)). Empty collection (len 0) -> 0. */
SYSV i64 cssc_bounds_check(i64 i, i64 len) {
    i64 hi = len - 1;
    i64 c = i;
    if (c > hi) { c = hi; }
    if (c < 0)  { c = 0; }
    return c;
}

/* cssc.peek search family (rtsyms 85-89) over a vector. `ek` is the element kind
 * (txGetElem: 0=int, 1=string, 2=float, 3=bool) selecting how an element compares
 * to `val`: string -> content equality, float -> double equality, else raw i64. */
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
/* sum of numeric elements. float vector -> f64-bits sum (tagged float by the
 * compiler); int/bool -> integer sum (bool counts trues, matching Python). */
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
/* min/max of numeric elements (rtsyms 90/91). float -> f64-bits, int/bool -> raw.
 * Empty / string vector returns 0 (a safe value): the compiler pairs these with an
 * alive bit so an empty (or non-numeric) collection reads back as null. */
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
/* cssc.peek slice family (rtsyms 94/95): return a NEW vector, preserving each
 * element's runtime type. rtsym 94 is Python-slice semantics (negative wrap then
 * clamp to [0,len]) for take/skip/take_last/skip_last/peek_range/peek_ahead; 96 is
 * a NON-wrapping clamp (negatives -> 0) for the max(0,..)-based window helpers. */
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
/* take_last/skip_last (rtsyms 97/98): the reference's `coll[-n:] if n else []` and
 * `coll[:-n] if n else list(coll)` -- n==0 is special (empty / whole). */
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
/* window/around/behind/cpeek (rtsyms 99-102): the reference's max(0,..)/min(len,..)
 * windowing. Encapsulated in C so the dispatch stays a single call. */
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

/* equals_next/equals_prev (rtsyms 92/93): element at i equals its neighbour, with
 * the reference's boundary rule (idx+1<len / idx-1>=0, else false). */
SYSV i64 cssc_vec_equals_next(void *p, i64 i, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v || i < 0 || i + 1 >= v->len) return 0;
    return cssc_peek_eq(v->data[i], v->data[i + 1], ek);
}
SYSV i64 cssc_vec_equals_prev(void *p, i64 i, i64 ek) {
    cssc_vec *v = (cssc_vec *)p; if (!v || i - 1 < 0 || i >= v->len) return 0;
    return cssc_peek_eq(v->data[i], v->data[i - 1], ek);
}

/* ==========================================================================
 * `stdio` module builtins (rtsyms 73-82). Ported byte-for-byte from the native
 * backend's cssc_stdio.c so `--sh`-compiled programs match `cssc run` exactly.
 * Each takes cssc_str* argument(s); the path is that string's inline, NUL-
 * terminated data buffer. Predicates return raw i64 1/0 (native-canonical
 * formatting prints these as 1/0, keeping `if (io::exists(x) != 0)` identical).
 * read/cwd/listdir return a fresh cssc_str*. Paths are used verbatim -- both
 * engines run from the same cwd in the harness, so relative paths resolve to
 * the same file. ======================================================== */
static const char *stdio_cpath(void *p) { return p ? ((cssc_str *)p)->data : ""; }

/* Recursively delete a directory tree -- matches shutil.rmtree. 1=gone, 0=fail. */
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
/* ---- sys:: module process arguments ---------------------------------------
 * The stage-1 loader (run.py / selfcarry) publishes the process argv via
 * cssc_rt_set_args BEFORE calling the entry. The sys module exposes the USER
 * args only -- argv[1:] -- exactly like the reference interpreter
 * (`sys.argv[1:]`), so the +1 program-name skip is baked into every accessor.
 * arg(i) -> heap cssc_str or NULL(0x0) past the end; arg_int/arg_float mirror
 * Python int()/float() (whole-string parse, else 0/0.0); arg_bool mirrors
 * `str(v).lower() not in {false,0,no,off,""}`. Byte-identical to CsscSysArgsModule
 * and the native cssc_sys_* accessors. */
static int    cssc_g_argc = 0;
static char **cssc_g_argv = 0;
CSSC_EXPORT void cssc_rt_set_args(int argc, char **argv) { cssc_g_argc = argc; cssc_g_argv = argv; }
/* RAW command-line access (cssc::argc / cssc::argStr builtins). Unlike the sys
 * module's user-args window, these expose the FULL argv (argv[0] included) -- the
 * self-hosted compiler's own main.cssc uses them to read its `--free`/`--obj`
 * flag at argv[1]. Byte-for-byte the s0/native cssc_host_argc / cssc_host_argstr. */
SYSV i64 cssc_host_argc(void) { return (i64)cssc_g_argc; }
SYSV i64 cssc_host_argstr(i64 idx, i64 buf, i64 max) {
    if (idx < 0 || idx >= (i64)cssc_g_argc || !cssc_g_argv) return 0;
    const char *s = cssc_g_argv[idx];
    if (!s) return 0;
    i64 n = 0; while (s[n]) n++;
    if (n > max) n = max;
    char *dst = (char *)buf;
    for (i64 i = 0; i < n; i++) dst[i] = s[i];
    return n;
}
static const char *cssc_sys__cstr(i64 idx) {
    i64 real = idx + 1;                                  /* user index -> argv, skip [0] */
    if (idx < 0 || real >= (i64)cssc_g_argc) return 0;
    return cssc_g_argv ? cssc_g_argv[real] : 0;
}
static int cssc_sys__ws(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
SYSV i64 cssc_sys_argc(void) { i64 n = (i64)cssc_g_argc - 1; return n > 0 ? n : 0; }
SYSV void *cssc_sys_arg(i64 idx) {
    const char *s = cssc_sys__cstr(idx);
    if (!s) return 0;                                    /* out of range -> None/0x0 */
    i64 n = 0; while (s[n]) n++;
    return cssc_string_lit(s, n);
}
SYSV i64 cssc_sys_has_arg(i64 idx) {
    return (idx >= 0 && idx + 1 < (i64)cssc_g_argc) ? 1 : 0;
}
SYSV i64 cssc_sys_arg_int(i64 idx) {
    const char *s = cssc_sys__cstr(idx);
    if (!s) return 0;
    while (cssc_sys__ws(*s)) s++;
    int neg = 0;
    if (*s == '+' || *s == '-') { neg = (*s == '-'); s++; }
    if (*s < '0' || *s > '9') return 0;                  /* no digit -> int() error -> 0 */
    i64 v = 0; int prev_digit = 0;
    while (*s) {
        if (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); prev_digit = 1; s++; }
        else if (*s == '_' && prev_digit && s[1] >= '0' && s[1] <= '9') { prev_digit = 0; s++; }
        else break;                                      /* Python: single _ only BETWEEN digits */
    }
    while (cssc_sys__ws(*s)) s++;
    if (*s != 0) return 0;                               /* trailing junk -> error -> 0 */
    return neg ? -v : v;
}
/* Returns the f64 BITS in RAX (not a real double in XMM0): the compiler keeps
 * float SSAs in GP registers and op-7 CALL captures RAX, exactly like the
 * cssc_math_* wrappers (bits-out via cssc_d2b). */
SYSV u64 cssc_sys_arg_float(i64 idx) {
    const char *s = cssc_sys__cstr(idx);
    if (!s) return cssc_d2b(0.0);
    while (cssc_sys__ws(*s)) s++;
    if (*s == 0) return cssc_d2b(0.0);
    char *end = 0;
    double d = strtod(s, &end);
    if (end == s) return cssc_d2b(0.0);                  /* no parse -> float() error -> 0.0 */
    while (cssc_sys__ws(*end)) end++;
    if (*end != 0) return cssc_d2b(0.0);                 /* trailing junk -> error -> 0.0 */
    return cssc_d2b(d);
}
SYSV i64 cssc_sys_arg_bool(i64 idx) {
    const char *s = cssc_sys__cstr(idx);
    if (!s) return 0;
    if (s[0] == 0) return 0;                             /* "" -> false */
    char lo[8]; int i = 0;
    while (s[i] && i < 7) { char c = s[i]; if (c >= 'A' && c <= 'Z') c = (char)(c + 32); lo[i] = c; i++; }
    lo[i] = 0;
    if (s[i] != 0) return 1;                             /* longer than any false-word -> true */
    if ((lo[0]=='f'&&lo[1]=='a'&&lo[2]=='l'&&lo[3]=='s'&&lo[4]=='e'&&lo[5]==0)
        || (lo[0]=='0'&&lo[1]==0)
        || (lo[0]=='n'&&lo[1]=='o'&&lo[2]==0)
        || (lo[0]=='o'&&lo[1]=='f'&&lo[2]=='f'&&lo[3]==0)) return 0;
    return 1;
}
/* ---- cssc.env module accessors (byte-identical to CsscEnvModule + the native
 * cssc_env_* companion). Args are cssc_str* whose inline data (NUL-terminated by
 * cssc_string_lit) is at +8, so the C string is that pointer. HOST-only: a
 * freestanding kernel has no environment, so cssc_ccos_rt.c does NOT define these
 * -> env:: in a --obj build is FS_UNSUPPORTED, which is correct. */
#if defined(_WIN32)
extern int _putenv_s(const char *, const char *);
extern char **_environ;
#define CSSC_RT_ENVIRON _environ
static int cssc_rt_env_set(const char *n, const char *v) { return _putenv_s(n, v); }
static int cssc_rt_env_del(const char *n) { return _putenv_s(n, ""); }
#else
extern int setenv(const char *, const char *, int);
extern int unsetenv(const char *);
extern char **environ;
#define CSSC_RT_ENVIRON environ
static int cssc_rt_env_set(const char *n, const char *v) { return setenv(n, v, 1); }
static int cssc_rt_env_del(const char *n) { return unsetenv(n); }
#endif
static const char *cssc_env__cstr(u64 s) { return s ? (const char *)(s + 8) : ""; }
static int cssc_env__ws(char c) { return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v'; }
SYSV void *cssc_env_get(u64 nameStr) {
    const char *v = getenv(cssc_env__cstr(nameStr));
    if (!v) v = "";
    i64 n = 0; while (v[n]) n++;
    return cssc_string_lit(v, n);
}
SYSV void *cssc_env_get2(u64 nameStr, u64 defStr) {
    const char *v = getenv(cssc_env__cstr(nameStr));
    if (!v) v = cssc_env__cstr(defStr);
    i64 n = 0; while (v[n]) n++;
    return cssc_string_lit(v, n);
}
SYSV i64 cssc_env_set(u64 nameStr, u64 valStr) {
    cssc_rt_env_set(cssc_env__cstr(nameStr), cssc_env__cstr(valStr));
    return 1;
}
SYSV i64 cssc_env_has(u64 nameStr) { return getenv(cssc_env__cstr(nameStr)) ? 1 : 0; }
SYSV i64 cssc_env_delete(u64 nameStr) {
    const char *n = cssc_env__cstr(nameStr);
    if (!getenv(n)) return 0;
    cssc_rt_env_del(n);
    return 1;
}
SYSV i64 cssc_env_count(void) {
    i64 c = 0; char **e = CSSC_RT_ENVIRON;
    if (e) while (e[c]) c++;
    return c;
}
SYSV i64 cssc_env_get_int(u64 nameStr) {
    const char *v = getenv(cssc_env__cstr(nameStr));
    if (!v) return 0;
    const char *s = v;
    while (cssc_env__ws(*s)) s++;
    int neg = 0;
    if (*s == '+' || *s == '-') { neg = (*s == '-'); s++; }
    if (*s < '0' || *s > '9') return 0;
    i64 r = 0; int prev = 0;
    while (*s) {
        if (*s >= '0' && *s <= '9') { r = r * 10 + (*s - '0'); prev = 1; s++; }
        else if (*s == '_' && prev && s[1] >= '0' && s[1] <= '9') { prev = 0; s++; }
        else break;
    }
    while (cssc_env__ws(*s)) s++;
    if (*s != 0) return 0;
    return neg ? -r : r;
}
SYSV i64 cssc_env_get_bool(u64 nameStr) {
    const char *v = getenv(cssc_env__cstr(nameStr));
    if (!v || !v[0]) return 0;
    char lo[8]; int i = 0;
    while (v[i] && i < 7) { char c = v[i]; if (c >= 'A' && c <= 'Z') c = (char)(c + 32); lo[i] = c; i++; }
    lo[i] = 0;
    if (v[i] != 0) return 0;
    if ((lo[0]=='t'&&lo[1]=='r'&&lo[2]=='u'&&lo[3]=='e'&&lo[4]==0)
        || (lo[0]=='1'&&lo[1]==0)
        || (lo[0]=='y'&&lo[1]=='e'&&lo[2]=='s'&&lo[3]==0)) return 1;
    return 0;
}
/* sys::args -> the user command-line args as a vector<string> (skips argv[0]).
 * Each arg becomes a heap cssc_str the vector adopts, tagged as a string element
 * (types[]=1) so per-element reads and stringify see text, not a raw pointer.
 * Same user-arg window as the scalar accessors. */
SYSV void *cssc_sys_args(void) {
    void *v = cssc_vec_new(4);
    i64 n = (i64)cssc_g_argc;
    for (i64 i = 1; i < n; i++) {
        const char *s = cssc_g_argv ? cssc_g_argv[i] : 0;
        if (!s) s = "";
        i64 len = 0; while (s[len]) len++;
        void *str = cssc_string_lit(s, len);
        cssc_vec_push_typed(v, (i64)(intptr_t)str, 1);   /* type 1 = string */
    }
    return v;
}
/* stdio::sizeof(path) -> file size in bytes, -1 if missing/error. Mirrors the
   native cssc_stdio_sizeof so `--sh` builds match `cssc run`. New in CSSC 7.0. */
SYSV i64 cssc_rt_stdio_sizeof(void *path) {
    const char *p = stdio_cpath(path);
    if (!p[0]) return -1;
    FILE *f = fopen(p, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long sz = ftell(f);
    fclose(f);
    return sz < 0 ? -1 : (i64)sz;
}
/* stdio::absolute(path) -> absolute/normalized form ("" cwd for empty). */
SYSV void *cssc_rt_stdio_absolute(void *path) {
    const char *p = stdio_cpath(path);
    char buf[4096];
    if (!p[0]) {
        if (cssc_getcwd(buf, sizeof(buf)) == NULL) return cssc_string_lit("", 0);
        return cssc_string_lit(buf, (i64)strlen(buf));
    }
#if defined(_WIN32)
    if (GetFullPathNameA(p, (DWORD)sizeof(buf), buf, NULL) == 0)
        return cssc_string_lit(p, (i64)strlen(p));
    return cssc_string_lit(buf, (i64)strlen(buf));
#else
    if (realpath(p, buf) == NULL) return cssc_string_lit(p, (i64)strlen(p));
    return cssc_string_lit(buf, (i64)strlen(buf));
#endif
}
/* stdio::setcwd(path) -> 1 on success, 0 on error. */
SYSV i64 cssc_rt_stdio_setcwd(void *path) {
    const char *p = stdio_cpath(path);
    if (!p[0]) return 0;
#if defined(_WIN32)
    return SetCurrentDirectoryA(p) ? 1 : 0;
#else
    return chdir(p) == 0 ? 1 : 0;
#endif
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

/* ==========================================================================
 * Runtime symbol table -- the CANONICAL id->function map shared by the host
 * harness and (mirrored) the CCOS `cssc_rtsym`. APPEND ONLY; never renumber.
 * The compiler emits `mov rax, imm64=<placeholder>` for a runtime call and
 * records (operand_offset, sym_id); the loader patches imm64 = cssc_rtsym(id).
 * (Default/MS-x64 convention -- called by Python/ctypes.)
 * ========================================================================== */
#ifdef CSSC_GUI_HOST
/* gui rtsym targets: the SysV wrappers in cssc_gui_glue.c (bridge to the MS-ABI
 * gui runtime). Only present in the gui-enabled DLL. */
extern void  *cssc_rt_gui_screen_new(i64 w, i64 h, i64 fps);
extern void   cssc_rt_gui_screen_clear(void *s, i64 argb);
extern void   cssc_rt_gui_screen_present(void *s);
extern i64    cssc_rt_gui_screen_isopen(void *s);
extern void   cssc_rt_gui_screen_close(void *s);
extern i64    cssc_rt_gui_screen_width(void *s);
extern i64    cssc_rt_gui_screen_height(void *s);
extern void   cssc_rt_gui_screen_fillrect(void *s, i64 x, i64 y, i64 w, i64 h, i64 argb);
extern void   cssc_rt_gui_screen_drawrect(void *s, i64 x, i64 y, i64 w, i64 h, i64 argb);
extern void   cssc_rt_gui_screen_drawtext(void *s, i64 x, i64 y, void *str, i64 argb, i64 scale);
extern i64    cssc_rt_gui_screen_fbchecksum(void *s);
/* generated gui externs */
/* GUIGEN-RTEXTERNS-BEGIN (generated by harness/gui_gen.py -- do not hand-edit) */
extern void *cssc_rt_cssc_gui_font_new(i64);
extern void cssc_rt_cssc_gui_font_setscale(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_font_scale(void *s);
extern void cssc_rt_cssc_gui_font_setcolor(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_font_color(void *s);
extern i64 cssc_rt_cssc_gui_font_measure(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_font_height(void *s);
extern void *cssc_rt_cssc_gui_text_new(void *);
extern void cssc_rt_cssc_gui_text_settext(void *s, void * a0);
extern void * cssc_rt_cssc_gui_text_text(void *s);
extern void cssc_rt_cssc_gui_text_setpos(void *s, i64 a0, i64 a1);
extern i64 cssc_rt_cssc_gui_text_x(void *s);
extern i64 cssc_rt_cssc_gui_text_y(void *s);
extern void cssc_rt_cssc_gui_text_setcolor(void *s, i64 a0);
extern void cssc_rt_cssc_gui_text_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_text_setfont(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_text_measure(void *s);
extern void cssc_rt_cssc_gui_text_draw(void *s);
extern void cssc_rt_cssc_gui_text_hide(void *s);
extern void cssc_rt_cssc_gui_text_show(void *s);
extern void *cssc_rt_cssc_gui_button_new(void *);
extern void cssc_rt_cssc_gui_button_setlabel(void *s, void * a0);
extern void * cssc_rt_cssc_gui_button_label(void *s);
extern void cssc_rt_cssc_gui_button_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern void cssc_rt_cssc_gui_button_position(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_button_size(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_button_setcolor(void *s, i64 a0);
extern void cssc_rt_cssc_gui_button_sethovercolor(void *s, i64 a0);
extern void cssc_rt_cssc_gui_button_settextcolor(void *s, i64 a0);
extern void cssc_rt_cssc_gui_button_onclick(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_button_update(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_button_ishovered(void *s);
extern i64 cssc_rt_cssc_gui_button_ispressed(void *s);
extern void cssc_rt_cssc_gui_button_draw(void *s);
extern void cssc_rt_cssc_gui_button_hide(void *s);
extern void cssc_rt_cssc_gui_button_show(void *s);
extern void *cssc_rt_cssc_gui_toolbar_new(void *);
extern void cssc_rt_cssc_gui_toolbar_add(void *s, void * a0);
extern void cssc_rt_cssc_gui_toolbar_setpos(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_toolbar_setsize(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_toolbar_setorientation(void *s, i64 a0);
extern void cssc_rt_cssc_gui_toolbar_setspacing(void *s, i64 a0);
extern void cssc_rt_cssc_gui_toolbar_setrighttext(void *s, void * a0);
extern void cssc_rt_cssc_gui_toolbar_setrightcolor(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_toolbar_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_toolbar_draw(void *s);
extern void *cssc_rt_cssc_gui_textbox_new(void *);
extern void cssc_rt_cssc_gui_textbox_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern void cssc_rt_cssc_gui_textbox_settext(void *s, void * a0);
extern void * cssc_rt_cssc_gui_textbox_text(void *s);
extern i64 cssc_rt_cssc_gui_textbox_length(void *s);
extern void cssc_rt_cssc_gui_textbox_setfocus(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_textbox_focused(void *s);
extern void cssc_rt_cssc_gui_textbox_setcolor(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_textbox_setscale(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_textbox_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_textbox_draw(void *s);
extern void *cssc_rt_cssc_gui_editor_new(void *);
extern void cssc_rt_cssc_gui_editor_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern void cssc_rt_cssc_gui_editor_settext(void *s, void * a0);
extern void * cssc_rt_cssc_gui_editor_text(void *s);
extern i64 cssc_rt_cssc_gui_editor_linecount(void *s);
extern i64 cssc_rt_cssc_gui_editor_cursorline(void *s);
extern i64 cssc_rt_cssc_gui_editor_cursorcol(void *s);
extern i64 cssc_rt_cssc_gui_editor_revision(void *s);
extern i64 cssc_rt_cssc_gui_editor_caretpixelx(void *s);
extern i64 cssc_rt_cssc_gui_editor_caretpixely(void *s);
extern i64 cssc_rt_cssc_gui_editor_lineheight(void *s);
extern i64 cssc_rt_cssc_gui_editor_completereq(void *s);
extern i64 cssc_rt_cssc_gui_editor_completeactive(void *s);
extern void cssc_rt_cssc_gui_editor_completecancel(void *s);
extern void * cssc_rt_cssc_gui_editor_completionquery(void *s);
extern void cssc_rt_cssc_gui_editor_setcompletions(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_editor_hoverreq(void *s);
extern i64 cssc_rt_cssc_gui_editor_hoveractive(void *s);
extern void cssc_rt_cssc_gui_editor_hovercancel(void *s);
extern void * cssc_rt_cssc_gui_editor_hoverquery(void *s);
extern void cssc_rt_cssc_gui_editor_sethover(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_setdiagnostics(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_setstickydiag(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_clearstickydiag(void *s);
extern void cssc_rt_cssc_gui_editor_setcleanmarks(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_clearcleanmarks(void *s);
extern void cssc_rt_cssc_gui_editor_setipline(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_revealip(void *s);
extern i64 cssc_rt_cssc_gui_editor_stickycount(void *s);
extern i64 cssc_rt_cssc_gui_editor_sigreq(void *s);
extern i64 cssc_rt_cssc_gui_editor_sigactive(void *s);
extern void cssc_rt_cssc_gui_editor_sigcancel(void *s);
extern void * cssc_rt_cssc_gui_editor_sigquery(void *s);
extern void cssc_rt_cssc_gui_editor_setsignature(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_gotoline(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_setcursor(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_editor_insert(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_editor_hasselection(void *s);
extern void * cssc_rt_cssc_gui_editor_selectedtext(void *s);
extern i64 cssc_rt_cssc_gui_editor_doubleclicked(void *s);
extern i64 cssc_rt_cssc_gui_editor_rightclicked(void *s);
extern i64 cssc_rt_cssc_gui_editor_clicked(void *s);
extern i64 cssc_rt_cssc_gui_editor_scandecl(void *s, void * a0);
extern void * cssc_rt_cssc_gui_editor_decltype(void *s);
extern void * cssc_rt_cssc_gui_editor_declbase(void *s);
extern i64 cssc_rt_cssc_gui_editor_declbits(void *s);
extern i64 cssc_rt_cssc_gui_editor_declisauto(void *s);
extern i64 cssc_rt_cssc_gui_editor_valuebits(void *s, void * a0, void * a1);
extern i64 cssc_rt_cssc_gui_editor_saverequested(void *s);
extern void cssc_rt_cssc_gui_editor_undo(void *s);
extern void cssc_rt_cssc_gui_editor_redo(void *s);
extern void cssc_rt_cssc_gui_editor_selectall(void *s);
extern i64 cssc_rt_cssc_gui_editor_search(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_editor_markall(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_clearsearch(void *s);
extern i64 cssc_rt_cssc_gui_editor_searchcount(void *s);
extern i64 cssc_rt_cssc_gui_editor_replaceall(void *s, void * a0, void * a1);
extern void cssc_rt_cssc_gui_editor_setfocus(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_setcolor(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_editor_setgutter(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_setvisible(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_setlanguage(void *s, i64 a0);
extern void cssc_rt_cssc_gui_editor_setlangforpath(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_setownership(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_editor_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_editor_draw(void *s);
extern void *cssc_rt_cssc_gui_list_new(void *);
extern void cssc_rt_cssc_gui_list_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern void cssc_rt_cssc_gui_list_add(void *s, void * a0);
extern void cssc_rt_cssc_gui_list_addat(void *s, void * a0, i64 a1);
extern void cssc_rt_cssc_gui_list_clear(void *s);
extern i64 cssc_rt_cssc_gui_list_count(void *s);
extern i64 cssc_rt_cssc_gui_list_selected(void *s);
extern i64 cssc_rt_cssc_gui_list_rightclicked(void *s);
extern void * cssc_rt_cssc_gui_list_selectedtext(void *s);
extern void cssc_rt_cssc_gui_list_setselected(void *s, i64 a0);
extern void cssc_rt_cssc_gui_list_setfocus(void *s, i64 a0);
extern void cssc_rt_cssc_gui_list_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_list_setcolor(void *s, i64 a0, i64 a1);
extern i64 cssc_rt_cssc_gui_list_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_list_draw(void *s);
extern void *cssc_rt_cssc_gui_tree_new(void *);
extern void cssc_rt_cssc_gui_tree_setroot(void *s, void * a0);
extern void cssc_rt_cssc_gui_tree_seticondir(void *s, void * a0);
extern void cssc_rt_cssc_gui_tree_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern i64 cssc_rt_cssc_gui_tree_width(void *s);
extern void cssc_rt_cssc_gui_tree_refresh(void *s);
extern i64 cssc_rt_cssc_gui_tree_count(void *s);
extern i64 cssc_rt_cssc_gui_tree_selected(void *s);
extern void * cssc_rt_cssc_gui_tree_selectedpath(void *s);
extern void * cssc_rt_cssc_gui_tree_selectedname(void *s);
extern i64 cssc_rt_cssc_gui_tree_selectedisdir(void *s);
extern i64 cssc_rt_cssc_gui_tree_rightclicked(void *s);
extern i64 cssc_rt_cssc_gui_tree_dropready(void *s);
extern void * cssc_rt_cssc_gui_tree_dropsrc(void *s);
extern void * cssc_rt_cssc_gui_tree_dropdst(void *s);
extern void cssc_rt_cssc_gui_tree_setfocus(void *s, i64 a0);
extern void cssc_rt_cssc_gui_tree_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_tree_setcolor(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_tree_setvisible(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_tree_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_tree_draw(void *s);
extern void *cssc_rt_cssc_gui_terminal_new(void *);
extern void cssc_rt_cssc_gui_terminal_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern void cssc_rt_cssc_gui_terminal_setcwd(void *s, void * a0);
extern void cssc_rt_cssc_gui_terminal_setfocus(void *s, i64 a0);
extern void cssc_rt_cssc_gui_terminal_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_terminal_setcolor(void *s, i64 a0, i64 a1);
extern i64 cssc_rt_cssc_gui_terminal_isrunning(void *s);
extern void cssc_rt_cssc_gui_terminal_run(void *s, void * a0);
extern void cssc_rt_cssc_gui_terminal_write(void *s, void * a0);
extern void cssc_rt_cssc_gui_terminal_writeansi(void *s, void * a0);
extern void cssc_rt_cssc_gui_terminal_clear(void *s);
extern i64 cssc_rt_cssc_gui_terminal_ipline(void *s);
extern void * cssc_rt_cssc_gui_terminal_ipfile(void *s);
extern void cssc_rt_cssc_gui_terminal_setinputlock(void *s, i64 a0);
extern void cssc_rt_cssc_gui_terminal_stop(void *s);
extern void cssc_rt_cssc_gui_terminal_setvisible(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_terminal_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_terminal_draw(void *s);
extern void *cssc_rt_cssc_gui_menu_new(void *);
extern void cssc_rt_cssc_gui_menu_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern i64 cssc_rt_cssc_gui_menu_addmenu(void *s, void * a0);
extern void cssc_rt_cssc_gui_menu_additem(void *s, i64 a0, void * a1, i64 a2);
extern void cssc_rt_cssc_gui_menu_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_menu_setcolor(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_menu_setrighttext(void *s, void * a0);
extern void cssc_rt_cssc_gui_menu_setrightcolor(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_menu_isopen(void *s);
extern i64 cssc_rt_cssc_gui_menu_action(void *s);
extern i64 cssc_rt_cssc_gui_menu_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_menu_draw(void *s);
extern void *cssc_rt_cssc_gui_prompt_new(void *);
extern void cssc_rt_cssc_gui_prompt_open(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_prompt_isopen(void *s);
extern i64 cssc_rt_cssc_gui_prompt_result(void *s);
extern void * cssc_rt_cssc_gui_prompt_text(void *s);
extern void cssc_rt_cssc_gui_prompt_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_prompt_setcolor(void *s, i64 a0, i64 a1);
extern i64 cssc_rt_cssc_gui_prompt_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_prompt_draw(void *s);
extern void *cssc_rt_cssc_gui_tabs_new(void *);
extern void cssc_rt_cssc_gui_tabs_setrect(void *s, i64 a0, i64 a1, i64 a2, i64 a3);
extern void cssc_rt_cssc_gui_tabs_add(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_tabs_indexof(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_tabs_count(void *s);
extern i64 cssc_rt_cssc_gui_tabs_active(void *s);
extern void * cssc_rt_cssc_gui_tabs_activepath(void *s);
extern void * cssc_rt_cssc_gui_tabs_pathof(void *s, i64 a0);
extern void cssc_rt_cssc_gui_tabs_setactive(void *s, i64 a0);
extern void cssc_rt_cssc_gui_tabs_remove(void *s, i64 a0);
extern i64 cssc_rt_cssc_gui_tabs_clicked(void *s);
extern i64 cssc_rt_cssc_gui_tabs_closerequested(void *s);
extern void cssc_rt_cssc_gui_tabs_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_tabs_setcolor(void *s, i64 a0, i64 a1);
extern i64 cssc_rt_cssc_gui_tabs_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_tabs_draw(void *s);
extern void *cssc_rt_cssc_gui_browser_new(void *);
extern void cssc_rt_cssc_gui_browser_open(void *s, void * a0, i64 a1);
extern i64 cssc_rt_cssc_gui_browser_isopen(void *s);
extern i64 cssc_rt_cssc_gui_browser_result(void *s);
extern void * cssc_rt_cssc_gui_browser_chosen(void *s);
extern void cssc_rt_cssc_gui_browser_setscale(void *s, i64 a0);
extern void cssc_rt_cssc_gui_browser_setcolor(void *s, i64 a0, i64 a1);
extern void cssc_rt_cssc_gui_browser_seticondir(void *s, void * a0);
extern i64 cssc_rt_cssc_gui_browser_update(void *s, void * a0);
extern void cssc_rt_cssc_gui_browser_draw(void *s);
extern void *cssc_rt_cssc_gui_debugger_new(void *);
extern void cssc_rt_cssc_gui_debugger_start(void *s, void * a0, void * a1);
extern i64 cssc_rt_cssc_gui_debugger_update(void *s);
extern i64 cssc_rt_cssc_gui_debugger_active(void *s);
extern i64 cssc_rt_cssc_gui_debugger_focused(void *s);
extern void cssc_rt_cssc_gui_debugger_click(void *s, i64 a0, i64 a1, i64 a2);
extern void cssc_rt_cssc_gui_debugger_draw(void *s);
extern i64 cssc_rt_cssc_gui_debugger_ipline(void *s);
extern i64 cssc_rt_cssc_gui_debugger_ipfollow(void *s);
extern void * cssc_rt_cssc_gui_debugger_ipfile(void *s);
extern void * cssc_rt_cssc_gui_debugger_takeoutput(void *s);
extern i64 cssc_rt_cssc_gui_debugger_held(void *s);
extern void cssc_rt_cssc_gui_debugger_clearheld(void *s);
extern void cssc_rt_cssc_gui_debugger_close(void *s);
extern void cssc_rt_cssc_gui_debugger_key(void *s, i64 a0);
extern void cssc_rt_cssc_gui_debugger_char(void *s, i64 a0);
/* GUIGEN-RTEXTERNS-END */
/* video:: module wrappers (cssc_gui_glue.c) */
extern void *cssc_rt_video_new(i64, i64, i64);
extern void cssc_rt_video_clear(void *p, i64 argb);
extern void cssc_rt_video_pixel(void *p, i64 x, i64 y, i64 argb);
extern i64 cssc_rt_video_get_pixel(void *p, i64 x, i64 y);
extern void cssc_rt_video_fillrect(void *p, i64 x, i64 y, i64 w, i64 h, i64 argb);
extern void cssc_rt_video_draw_rect(void *p, i64 x, i64 y, i64 w, i64 h, i64 argb);
extern void cssc_rt_video_draw_text(void *p, i64 x, i64 y, void *str, i64 argb, i64 scale);
extern void cssc_rt_video_present(void *p);
extern i64 cssc_rt_video_checksum(void *p);
#endif

CSSC_EXPORT u64 cssc_rtsym(u64 id) {
    switch (id) {
        case 0:  return (u64)&cssc_print_int;
        case 1:  return (u64)&cssc_print_str;
        case 2:  return (u64)&cssc_out_string;
        case 3:  return (u64)&cssc_rt_string_lit;
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
        case 20: return (u64)&cssc_print_float;   /* arg in XMM0, not RDI */
        case 21: return (u64)&cssc_out_float;     /* arg in XMM0, not RDI */
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
        case 42: return (u64)&cssc_host_read;      /* self-host: read stdin  */
        case 43: return (u64)&cssc_free_raw;       /* self-host: free block  */
        case 44: return (u64)&cssc_print_raw;      /* self-host: write bytes */
        case 45: return (u64)&cssc_memfill32;      /* self-host: fill dwords */
        case 46: return (u64)&cssc_str_to_f;       /* self-host: str -> f64  */
        case 47: return (u64)&cssc_dbg;            /* self-host: stderr trace */
        case 48: return (u64)&cssc_vec_push_typed; /* vector<auto>: push value+kind */
        case 49: return (u64)&cssc_vec_type_at;    /* vector<auto>: kind of elem i  */
        case 50: return (u64)&cssc_auto_to_str;    /* vector<auto>: cell -> string  */
        case 51: return (u64)&cssc_trap;           /* strict-typing fault (flush+trap) */
        case 52: return (u64)&cssc_ftoi_checked;   /* int = float: whole->int / frac->fault */
        case 53: return (u64)&cssc_mod_path;       /* module/#load: set path span         */
        case 54: return (u64)&cssc_mod_read;       /* module/#load: read set-path into buf */
        case 55: return (u64)&cssc_mod_path_inc;   /* #include: name -> module/<name>.cssc */
        case 56: return (u64)&cssc_rt_sleep_ms;     /* cssc::sleep(ms): process sleep      */
        case 57: return (u64)&cssc_math_sqrt;       /* cssc.math: sqrt  (float)            */
        case 58: return (u64)&cssc_math_sin;        /* cssc.math: sin   (float)            */
        case 59: return (u64)&cssc_math_cos;        /* cssc.math: cos   (float)            */
        case 60: return (u64)&cssc_math_tan;        /* cssc.math: tan   (float)            */
        case 61: return (u64)&cssc_math_log;        /* cssc.math: log   (float)            */
        case 62: return (u64)&cssc_math_exp;        /* cssc.math: exp   (float)            */
        case 63: return (u64)&cssc_math_abs;        /* cssc.math: abs   (float)            */
        case 64: return (u64)&cssc_math_floor;      /* cssc.math: floor (int)              */
        case 65: return (u64)&cssc_math_ceil;       /* cssc.math: ceil  (int)              */
        case 66: return (u64)&cssc_math_pow;        /* cssc.math: pow   (float, 2-arg)     */
        case 67: return (u64)&cssc_math_min;        /* cssc.math: min   (float, 2-arg)     */
        case 68: return (u64)&cssc_math_max;        /* cssc.math: max   (float, 2-arg)     */
        case 69: return (u64)&cssc_paths_dirname;   /* cssc.paths: dirname (string)        */
        case 70: return (u64)&cssc_paths_basename;  /* cssc.paths: basename (string)       */
        case 71: return (u64)&cssc_paths_ext;       /* cssc.paths: ext    (string)         */
        case 72: return (u64)&cssc_paths_stem;      /* cssc.paths: stem   (string)         */
        case 73: return (u64)&cssc_rt_stdio_write;      /* stdio: write(path, content) -> int  */
        case 74: return (u64)&cssc_rt_stdio_read;       /* stdio: read(path) -> string         */
        case 75: return (u64)&cssc_rt_stdio_exists;     /* stdio: exists(path) -> int          */
        case 76: return (u64)&cssc_rt_stdio_createdir;  /* stdio: createdir(path) -> int       */
        case 77: return (u64)&cssc_rt_stdio_createfile; /* stdio: createfile(path) -> int      */
        case 78: return (u64)&cssc_rt_stdio_removefile; /* stdio: removefile(path) -> int      */
        case 79: return (u64)&cssc_rt_stdio_removedir;  /* stdio: removedir(path) -> int       */
        case 80: return (u64)&cssc_rt_stdio_move;       /* stdio: move(src, dst) -> int        */
        case 81: return (u64)&cssc_rt_stdio_cwd;        /* stdio: cwd() -> string (0-arg)      */
        case 82: return (u64)&cssc_rt_stdio_listdir;    /* stdio: listdir(path) -> string      */
        case 83: return (u64)&cssc_nullable_str;        /* cssc.peek: null-in-string -> ""      */
        case 84: return (u64)&cssc_bounds_check;        /* cssc.peek: clamp idx to [0,len-1]   */
        case 85: return (u64)&cssc_vec_find_index;      /* cssc.peek: find_index(v,val,ek)     */
        case 86: return (u64)&cssc_vec_find_last_index; /* cssc.peek: find_last_index          */
        case 87: return (u64)&cssc_vec_contains;        /* cssc.peek: contains -> 1/0          */
        case 88: return (u64)&cssc_vec_count;           /* cssc.peek: count occurrences        */
        case 89: return (u64)&cssc_vec_sum;             /* cssc.peek: sum (int/float bits)     */
        case 90: return (u64)&cssc_vec_min;             /* cssc.peek: min (int/float bits)     */
        case 91: return (u64)&cssc_vec_max;             /* cssc.peek: max (int/float bits)     */
        case 92: return (u64)&cssc_vec_equals_next;     /* cssc.peek: equals_next -> bool      */
        case 93: return (u64)&cssc_vec_equals_prev;     /* cssc.peek: equals_prev -> bool      */
        case 94: return (u64)&cssc_vec_slice;           /* cssc.peek: slice (Python wrap)      */
        case 95: return (u64)&cssc_vec_every_nth;       /* cssc.peek: every_nth -> vector      */
        case 96: return (u64)&cssc_vec_slice_clamp;     /* cssc.peek: slice (max0 clamp)       */
        case 97: return (u64)&cssc_vec_take_last;       /* cssc.peek: take_last -> vector      */
        case 98: return (u64)&cssc_vec_skip_last;       /* cssc.peek: skip_last -> vector      */
        case 99: return (u64)&cssc_vec_window;          /* cssc.peek: peek_window -> vector    */
        case 100: return (u64)&cssc_vec_around;         /* cssc.peek: peek_around -> vector    */
        case 101: return (u64)&cssc_vec_behind;         /* cssc.peek: peek_behind -> vector    */
        case 102: return (u64)&cssc_vec_cpeek;          /* cssc.peek: cpeek -> vector          */
        case 103: return (u64)&cssc_string_upper;       /* str.upper()   -> string             */
        case 104: return (u64)&cssc_string_lower;       /* str.lower()   -> string             */
        case 105: return (u64)&cssc_string_replace;     /* str.replace(from,to) -> string      */
        case 106: return (u64)&cssc_string_contains;    /* str.contains(sub) -> bool           */
        case 107: return (u64)&cssc_string_trim;        /* str.trim()    -> string             */
        case 108: return (u64)&cssc_math_random;        /* cssc.math: random(seed) -> float    */
        case 109: return (u64)&cssc_math_randomint;     /* cssc.math: randomint(seed,lo,hi)    */
        case 110: return (u64)&cssc_chr;                /* cssc::chr(n) -> 1-byte string        */
        case 111: return (u64)&cssc_ord;                /* cssc::ord(s) -> first byte value     */
        /* Core-extra rtsyms live at 4096+ (ABOVE the gui block 112-4095) so the
         * gui-widget generator can grow its ids without colliding, and run.py's
         * gui-DLL detection ([112,4095]) never mistakes a core sym for gui. */
        case 4096: return (u64)&cssc_bit_check;         /* #stack/#heap[int,N] bit-exact guard  */
        case 4097: return (u64)&cssc_rt_stdio_sizeof;   /* stdio: sizeof(path) -> i64 bytes/-1  */
        case 4098: return (u64)&cssc_rt_stdio_absolute; /* stdio: absolute(path) -> string      */
        case 4099: return (u64)&cssc_rt_stdio_setcwd;   /* stdio: setcwd(path) -> 1/0           */
        case 4100: return (u64)&cssc_sys_argc;          /* sys::argc() -> user arg count       */
        case 4101: return (u64)&cssc_sys_arg;           /* sys::arg(i) -> string / 0x0         */
        case 4102: return (u64)&cssc_sys_has_arg;       /* sys::has_arg(i) -> bool             */
        case 4103: return (u64)&cssc_sys_arg_int;       /* sys::arg_int(i) -> i64              */
        case 4104: return (u64)&cssc_sys_arg_float;     /* sys::arg_float(i) -> f64 (XMM0)     */
        case 4105: return (u64)&cssc_sys_arg_bool;      /* sys::arg_bool(i) -> bool            */
        case 4106: return (u64)&cssc_sys_args;          /* sys::args -> vector<string>         */
        case 4107: return (u64)&cssc_vec_to_str;        /* vector -> "{e0, e1, ...}"           */
        case 4108: return (u64)&cssc_map_to_str;        /* map -> "{k0: v0, ...}"              */
        case 4109: return (u64)&cssc_env_get;           /* env::get(name) -> string            */
        case 4110: return (u64)&cssc_env_get2;          /* env::get(name,def) -> string        */
        case 4111: return (u64)&cssc_env_set;           /* env::set(name,val) -> 1             */
        case 4112: return (u64)&cssc_env_has;           /* env::has(name) -> bool              */
        case 4113: return (u64)&cssc_env_delete;        /* env::delete(name) -> bool           */
        case 4114: return (u64)&cssc_env_count;         /* env::count() -> int                 */
        case 4115: return (u64)&cssc_env_get_int;       /* env::get_int(name) -> int           */
        case 4116: return (u64)&cssc_env_get_bool;      /* env::get_bool(name) -> bool         */
        case 4117: return (u64)&cssc_host_argc;          /* cssc::argc() -> RAW arg count       */
        case 4118: return (u64)&cssc_host_argstr;        /* cssc::argStr(i,buf,max) -> len      */
        case 4119: return (u64)&cssc_map_values;         /* map -> vector<value> (insertion ord) */
#ifdef CSSC_GUI_HOST
        case 112: return (u64)&cssc_rt_gui_screen_new;        /* gui::Screen(w,h[,fps]) -> handle */
        case 113: return (u64)&cssc_rt_gui_screen_clear;      /* screen->clear(argb)             */
        case 114: return (u64)&cssc_rt_gui_screen_present;    /* screen->present()               */
        case 115: return (u64)&cssc_rt_gui_screen_isopen;     /* screen->isOpen() -> int         */
        case 116: return (u64)&cssc_rt_gui_screen_close;      /* screen->close()                 */
        case 117: return (u64)&cssc_rt_gui_screen_width;      /* screen->width() -> int          */
        case 118: return (u64)&cssc_rt_gui_screen_height;     /* screen->height() -> int         */
        case 119: return (u64)&cssc_rt_gui_screen_fillrect;   /* screen->fillRect(x,y,w,h,argb)  */
        case 120: return (u64)&cssc_rt_gui_screen_drawrect;   /* screen->drawRect(x,y,w,h,argb)  */
        case 121: return (u64)&cssc_rt_gui_screen_drawtext;   /* screen->drawText(x,y,s,argb,sc) */
        case 122: return (u64)&cssc_rt_gui_screen_fbchecksum; /* screen->fbChecksum() -> int     */
        /* generated gui cases */
        /* GUIGEN-RTCASES-BEGIN (generated by harness/gui_gen.py -- do not hand-edit) */
        case 130: return (u64)&cssc_rt_cssc_gui_font_new;
        case 131: return (u64)&cssc_rt_cssc_gui_font_setscale;
        case 132: return (u64)&cssc_rt_cssc_gui_font_scale;
        case 133: return (u64)&cssc_rt_cssc_gui_font_setcolor;
        case 134: return (u64)&cssc_rt_cssc_gui_font_color;
        case 135: return (u64)&cssc_rt_cssc_gui_font_measure;
        case 136: return (u64)&cssc_rt_cssc_gui_font_height;
        case 137: return (u64)&cssc_rt_cssc_gui_text_new;
        case 138: return (u64)&cssc_rt_cssc_gui_text_settext;
        case 139: return (u64)&cssc_rt_cssc_gui_text_text;
        case 140: return (u64)&cssc_rt_cssc_gui_text_setpos;
        case 141: return (u64)&cssc_rt_cssc_gui_text_x;
        case 142: return (u64)&cssc_rt_cssc_gui_text_y;
        case 143: return (u64)&cssc_rt_cssc_gui_text_setcolor;
        case 144: return (u64)&cssc_rt_cssc_gui_text_setscale;
        case 145: return (u64)&cssc_rt_cssc_gui_text_setfont;
        case 146: return (u64)&cssc_rt_cssc_gui_text_measure;
        case 147: return (u64)&cssc_rt_cssc_gui_text_draw;
        case 148: return (u64)&cssc_rt_cssc_gui_text_hide;
        case 149: return (u64)&cssc_rt_cssc_gui_text_show;
        case 150: return (u64)&cssc_rt_cssc_gui_button_new;
        case 151: return (u64)&cssc_rt_cssc_gui_button_setlabel;
        case 152: return (u64)&cssc_rt_cssc_gui_button_label;
        case 153: return (u64)&cssc_rt_cssc_gui_button_setrect;
        case 154: return (u64)&cssc_rt_cssc_gui_button_position;
        case 155: return (u64)&cssc_rt_cssc_gui_button_size;
        case 156: return (u64)&cssc_rt_cssc_gui_button_setcolor;
        case 157: return (u64)&cssc_rt_cssc_gui_button_sethovercolor;
        case 158: return (u64)&cssc_rt_cssc_gui_button_settextcolor;
        case 159: return (u64)&cssc_rt_cssc_gui_button_onclick;
        case 160: return (u64)&cssc_rt_cssc_gui_button_update;
        case 161: return (u64)&cssc_rt_cssc_gui_button_ishovered;
        case 162: return (u64)&cssc_rt_cssc_gui_button_ispressed;
        case 163: return (u64)&cssc_rt_cssc_gui_button_draw;
        case 164: return (u64)&cssc_rt_cssc_gui_button_hide;
        case 165: return (u64)&cssc_rt_cssc_gui_button_show;
        case 166: return (u64)&cssc_rt_cssc_gui_toolbar_new;
        case 167: return (u64)&cssc_rt_cssc_gui_toolbar_add;
        case 168: return (u64)&cssc_rt_cssc_gui_toolbar_setpos;
        case 169: return (u64)&cssc_rt_cssc_gui_toolbar_setsize;
        case 170: return (u64)&cssc_rt_cssc_gui_toolbar_setorientation;
        case 171: return (u64)&cssc_rt_cssc_gui_toolbar_setspacing;
        case 172: return (u64)&cssc_rt_cssc_gui_toolbar_setrighttext;
        case 173: return (u64)&cssc_rt_cssc_gui_toolbar_setrightcolor;
        case 174: return (u64)&cssc_rt_cssc_gui_toolbar_update;
        case 175: return (u64)&cssc_rt_cssc_gui_toolbar_draw;
        case 176: return (u64)&cssc_rt_cssc_gui_textbox_new;
        case 177: return (u64)&cssc_rt_cssc_gui_textbox_setrect;
        case 178: return (u64)&cssc_rt_cssc_gui_textbox_settext;
        case 179: return (u64)&cssc_rt_cssc_gui_textbox_text;
        case 180: return (u64)&cssc_rt_cssc_gui_textbox_length;
        case 181: return (u64)&cssc_rt_cssc_gui_textbox_setfocus;
        case 182: return (u64)&cssc_rt_cssc_gui_textbox_focused;
        case 183: return (u64)&cssc_rt_cssc_gui_textbox_setcolor;
        case 184: return (u64)&cssc_rt_cssc_gui_textbox_setscale;
        case 185: return (u64)&cssc_rt_cssc_gui_textbox_update;
        case 186: return (u64)&cssc_rt_cssc_gui_textbox_draw;
        case 187: return (u64)&cssc_rt_cssc_gui_editor_new;
        case 188: return (u64)&cssc_rt_cssc_gui_editor_setrect;
        case 189: return (u64)&cssc_rt_cssc_gui_editor_settext;
        case 190: return (u64)&cssc_rt_cssc_gui_editor_text;
        case 191: return (u64)&cssc_rt_cssc_gui_editor_linecount;
        case 192: return (u64)&cssc_rt_cssc_gui_editor_cursorline;
        case 193: return (u64)&cssc_rt_cssc_gui_editor_cursorcol;
        case 194: return (u64)&cssc_rt_cssc_gui_editor_revision;
        case 195: return (u64)&cssc_rt_cssc_gui_editor_caretpixelx;
        case 196: return (u64)&cssc_rt_cssc_gui_editor_caretpixely;
        case 197: return (u64)&cssc_rt_cssc_gui_editor_lineheight;
        case 198: return (u64)&cssc_rt_cssc_gui_editor_completereq;
        case 199: return (u64)&cssc_rt_cssc_gui_editor_completeactive;
        case 200: return (u64)&cssc_rt_cssc_gui_editor_completecancel;
        case 201: return (u64)&cssc_rt_cssc_gui_editor_completionquery;
        case 202: return (u64)&cssc_rt_cssc_gui_editor_setcompletions;
        case 203: return (u64)&cssc_rt_cssc_gui_editor_hoverreq;
        case 204: return (u64)&cssc_rt_cssc_gui_editor_hoveractive;
        case 205: return (u64)&cssc_rt_cssc_gui_editor_hovercancel;
        case 206: return (u64)&cssc_rt_cssc_gui_editor_hoverquery;
        case 207: return (u64)&cssc_rt_cssc_gui_editor_sethover;
        case 208: return (u64)&cssc_rt_cssc_gui_editor_setdiagnostics;
        case 209: return (u64)&cssc_rt_cssc_gui_editor_setstickydiag;
        case 210: return (u64)&cssc_rt_cssc_gui_editor_clearstickydiag;
        case 211: return (u64)&cssc_rt_cssc_gui_editor_setcleanmarks;
        case 212: return (u64)&cssc_rt_cssc_gui_editor_clearcleanmarks;
        case 213: return (u64)&cssc_rt_cssc_gui_editor_setipline;
        case 214: return (u64)&cssc_rt_cssc_gui_editor_revealip;
        case 215: return (u64)&cssc_rt_cssc_gui_editor_stickycount;
        case 216: return (u64)&cssc_rt_cssc_gui_editor_sigreq;
        case 217: return (u64)&cssc_rt_cssc_gui_editor_sigactive;
        case 218: return (u64)&cssc_rt_cssc_gui_editor_sigcancel;
        case 219: return (u64)&cssc_rt_cssc_gui_editor_sigquery;
        case 220: return (u64)&cssc_rt_cssc_gui_editor_setsignature;
        case 221: return (u64)&cssc_rt_cssc_gui_editor_gotoline;
        case 222: return (u64)&cssc_rt_cssc_gui_editor_setcursor;
        case 223: return (u64)&cssc_rt_cssc_gui_editor_insert;
        case 224: return (u64)&cssc_rt_cssc_gui_editor_hasselection;
        case 225: return (u64)&cssc_rt_cssc_gui_editor_selectedtext;
        case 226: return (u64)&cssc_rt_cssc_gui_editor_doubleclicked;
        case 227: return (u64)&cssc_rt_cssc_gui_editor_rightclicked;
        case 228: return (u64)&cssc_rt_cssc_gui_editor_clicked;
        case 229: return (u64)&cssc_rt_cssc_gui_editor_scandecl;
        case 230: return (u64)&cssc_rt_cssc_gui_editor_decltype;
        case 231: return (u64)&cssc_rt_cssc_gui_editor_declbase;
        case 232: return (u64)&cssc_rt_cssc_gui_editor_declbits;
        case 233: return (u64)&cssc_rt_cssc_gui_editor_declisauto;
        case 234: return (u64)&cssc_rt_cssc_gui_editor_valuebits;
        case 235: return (u64)&cssc_rt_cssc_gui_editor_saverequested;
        case 236: return (u64)&cssc_rt_cssc_gui_editor_undo;
        case 237: return (u64)&cssc_rt_cssc_gui_editor_redo;
        case 238: return (u64)&cssc_rt_cssc_gui_editor_selectall;
        case 239: return (u64)&cssc_rt_cssc_gui_editor_search;
        case 240: return (u64)&cssc_rt_cssc_gui_editor_markall;
        case 241: return (u64)&cssc_rt_cssc_gui_editor_clearsearch;
        case 242: return (u64)&cssc_rt_cssc_gui_editor_searchcount;
        case 243: return (u64)&cssc_rt_cssc_gui_editor_replaceall;
        case 244: return (u64)&cssc_rt_cssc_gui_editor_setfocus;
        case 245: return (u64)&cssc_rt_cssc_gui_editor_setscale;
        case 246: return (u64)&cssc_rt_cssc_gui_editor_setcolor;
        case 247: return (u64)&cssc_rt_cssc_gui_editor_setgutter;
        case 248: return (u64)&cssc_rt_cssc_gui_editor_setvisible;
        case 249: return (u64)&cssc_rt_cssc_gui_editor_setlanguage;
        case 250: return (u64)&cssc_rt_cssc_gui_editor_setlangforpath;
        case 251: return (u64)&cssc_rt_cssc_gui_editor_setownership;
        case 252: return (u64)&cssc_rt_cssc_gui_editor_update;
        case 253: return (u64)&cssc_rt_cssc_gui_editor_draw;
        case 254: return (u64)&cssc_rt_cssc_gui_list_new;
        case 255: return (u64)&cssc_rt_cssc_gui_list_setrect;
        case 256: return (u64)&cssc_rt_cssc_gui_list_add;
        case 257: return (u64)&cssc_rt_cssc_gui_list_addat;
        case 258: return (u64)&cssc_rt_cssc_gui_list_clear;
        case 259: return (u64)&cssc_rt_cssc_gui_list_count;
        case 260: return (u64)&cssc_rt_cssc_gui_list_selected;
        case 261: return (u64)&cssc_rt_cssc_gui_list_rightclicked;
        case 262: return (u64)&cssc_rt_cssc_gui_list_selectedtext;
        case 263: return (u64)&cssc_rt_cssc_gui_list_setselected;
        case 264: return (u64)&cssc_rt_cssc_gui_list_setfocus;
        case 265: return (u64)&cssc_rt_cssc_gui_list_setscale;
        case 266: return (u64)&cssc_rt_cssc_gui_list_setcolor;
        case 267: return (u64)&cssc_rt_cssc_gui_list_update;
        case 268: return (u64)&cssc_rt_cssc_gui_list_draw;
        case 269: return (u64)&cssc_rt_cssc_gui_tree_new;
        case 270: return (u64)&cssc_rt_cssc_gui_tree_setroot;
        case 271: return (u64)&cssc_rt_cssc_gui_tree_seticondir;
        case 272: return (u64)&cssc_rt_cssc_gui_tree_setrect;
        case 273: return (u64)&cssc_rt_cssc_gui_tree_width;
        case 274: return (u64)&cssc_rt_cssc_gui_tree_refresh;
        case 275: return (u64)&cssc_rt_cssc_gui_tree_count;
        case 276: return (u64)&cssc_rt_cssc_gui_tree_selected;
        case 277: return (u64)&cssc_rt_cssc_gui_tree_selectedpath;
        case 278: return (u64)&cssc_rt_cssc_gui_tree_selectedname;
        case 279: return (u64)&cssc_rt_cssc_gui_tree_selectedisdir;
        case 280: return (u64)&cssc_rt_cssc_gui_tree_rightclicked;
        case 281: return (u64)&cssc_rt_cssc_gui_tree_dropready;
        case 282: return (u64)&cssc_rt_cssc_gui_tree_dropsrc;
        case 283: return (u64)&cssc_rt_cssc_gui_tree_dropdst;
        case 284: return (u64)&cssc_rt_cssc_gui_tree_setfocus;
        case 285: return (u64)&cssc_rt_cssc_gui_tree_setscale;
        case 286: return (u64)&cssc_rt_cssc_gui_tree_setcolor;
        case 287: return (u64)&cssc_rt_cssc_gui_tree_setvisible;
        case 288: return (u64)&cssc_rt_cssc_gui_tree_update;
        case 289: return (u64)&cssc_rt_cssc_gui_tree_draw;
        case 290: return (u64)&cssc_rt_cssc_gui_terminal_new;
        case 291: return (u64)&cssc_rt_cssc_gui_terminal_setrect;
        case 292: return (u64)&cssc_rt_cssc_gui_terminal_setcwd;
        case 293: return (u64)&cssc_rt_cssc_gui_terminal_setfocus;
        case 294: return (u64)&cssc_rt_cssc_gui_terminal_setscale;
        case 295: return (u64)&cssc_rt_cssc_gui_terminal_setcolor;
        case 296: return (u64)&cssc_rt_cssc_gui_terminal_isrunning;
        case 297: return (u64)&cssc_rt_cssc_gui_terminal_run;
        case 298: return (u64)&cssc_rt_cssc_gui_terminal_write;
        case 299: return (u64)&cssc_rt_cssc_gui_terminal_writeansi;
        case 300: return (u64)&cssc_rt_cssc_gui_terminal_clear;
        case 301: return (u64)&cssc_rt_cssc_gui_terminal_ipline;
        case 302: return (u64)&cssc_rt_cssc_gui_terminal_ipfile;
        case 303: return (u64)&cssc_rt_cssc_gui_terminal_setinputlock;
        case 304: return (u64)&cssc_rt_cssc_gui_terminal_stop;
        case 305: return (u64)&cssc_rt_cssc_gui_terminal_setvisible;
        case 306: return (u64)&cssc_rt_cssc_gui_terminal_update;
        case 307: return (u64)&cssc_rt_cssc_gui_terminal_draw;
        case 308: return (u64)&cssc_rt_cssc_gui_menu_new;
        case 309: return (u64)&cssc_rt_cssc_gui_menu_setrect;
        case 310: return (u64)&cssc_rt_cssc_gui_menu_addmenu;
        case 311: return (u64)&cssc_rt_cssc_gui_menu_additem;
        case 312: return (u64)&cssc_rt_cssc_gui_menu_setscale;
        case 313: return (u64)&cssc_rt_cssc_gui_menu_setcolor;
        case 314: return (u64)&cssc_rt_cssc_gui_menu_setrighttext;
        case 315: return (u64)&cssc_rt_cssc_gui_menu_setrightcolor;
        case 316: return (u64)&cssc_rt_cssc_gui_menu_isopen;
        case 317: return (u64)&cssc_rt_cssc_gui_menu_action;
        case 318: return (u64)&cssc_rt_cssc_gui_menu_update;
        case 319: return (u64)&cssc_rt_cssc_gui_menu_draw;
        case 320: return (u64)&cssc_rt_cssc_gui_prompt_new;
        case 321: return (u64)&cssc_rt_cssc_gui_prompt_open;
        case 322: return (u64)&cssc_rt_cssc_gui_prompt_isopen;
        case 323: return (u64)&cssc_rt_cssc_gui_prompt_result;
        case 324: return (u64)&cssc_rt_cssc_gui_prompt_text;
        case 325: return (u64)&cssc_rt_cssc_gui_prompt_setscale;
        case 326: return (u64)&cssc_rt_cssc_gui_prompt_setcolor;
        case 327: return (u64)&cssc_rt_cssc_gui_prompt_update;
        case 328: return (u64)&cssc_rt_cssc_gui_prompt_draw;
        case 329: return (u64)&cssc_rt_cssc_gui_tabs_new;
        case 330: return (u64)&cssc_rt_cssc_gui_tabs_setrect;
        case 331: return (u64)&cssc_rt_cssc_gui_tabs_add;
        case 332: return (u64)&cssc_rt_cssc_gui_tabs_indexof;
        case 333: return (u64)&cssc_rt_cssc_gui_tabs_count;
        case 334: return (u64)&cssc_rt_cssc_gui_tabs_active;
        case 335: return (u64)&cssc_rt_cssc_gui_tabs_activepath;
        case 336: return (u64)&cssc_rt_cssc_gui_tabs_pathof;
        case 337: return (u64)&cssc_rt_cssc_gui_tabs_setactive;
        case 338: return (u64)&cssc_rt_cssc_gui_tabs_remove;
        case 339: return (u64)&cssc_rt_cssc_gui_tabs_clicked;
        case 340: return (u64)&cssc_rt_cssc_gui_tabs_closerequested;
        case 341: return (u64)&cssc_rt_cssc_gui_tabs_setscale;
        case 342: return (u64)&cssc_rt_cssc_gui_tabs_setcolor;
        case 343: return (u64)&cssc_rt_cssc_gui_tabs_update;
        case 344: return (u64)&cssc_rt_cssc_gui_tabs_draw;
        case 345: return (u64)&cssc_rt_cssc_gui_browser_new;
        case 346: return (u64)&cssc_rt_cssc_gui_browser_open;
        case 347: return (u64)&cssc_rt_cssc_gui_browser_isopen;
        case 348: return (u64)&cssc_rt_cssc_gui_browser_result;
        case 349: return (u64)&cssc_rt_cssc_gui_browser_chosen;
        case 350: return (u64)&cssc_rt_cssc_gui_browser_setscale;
        case 351: return (u64)&cssc_rt_cssc_gui_browser_setcolor;
        case 352: return (u64)&cssc_rt_cssc_gui_browser_seticondir;
        case 353: return (u64)&cssc_rt_cssc_gui_browser_update;
        case 354: return (u64)&cssc_rt_cssc_gui_browser_draw;
        case 355: return (u64)&cssc_rt_cssc_gui_debugger_new;
        case 356: return (u64)&cssc_rt_cssc_gui_debugger_start;
        case 357: return (u64)&cssc_rt_cssc_gui_debugger_update;
        case 358: return (u64)&cssc_rt_cssc_gui_debugger_active;
        case 359: return (u64)&cssc_rt_cssc_gui_debugger_focused;
        case 360: return (u64)&cssc_rt_cssc_gui_debugger_click;
        case 361: return (u64)&cssc_rt_cssc_gui_debugger_draw;
        case 362: return (u64)&cssc_rt_cssc_gui_debugger_ipline;
        case 363: return (u64)&cssc_rt_cssc_gui_debugger_ipfollow;
        case 364: return (u64)&cssc_rt_cssc_gui_debugger_ipfile;
        case 365: return (u64)&cssc_rt_cssc_gui_debugger_takeoutput;
        case 366: return (u64)&cssc_rt_cssc_gui_debugger_held;
        case 367: return (u64)&cssc_rt_cssc_gui_debugger_clearheld;
        case 368: return (u64)&cssc_rt_cssc_gui_debugger_close;
        case 369: return (u64)&cssc_rt_cssc_gui_debugger_key;
        case 370: return (u64)&cssc_rt_cssc_gui_debugger_char;
        /* GUIGEN-RTCASES-END */
        /* video:: module (rtsyms 500-508, gui block -> gui DLL) */
        case 500: return (u64)&cssc_rt_video_new;
        case 501: return (u64)&cssc_rt_video_clear;
        case 502: return (u64)&cssc_rt_video_pixel;
        case 503: return (u64)&cssc_rt_video_get_pixel;
        case 504: return (u64)&cssc_rt_video_fillrect;
        case 505: return (u64)&cssc_rt_video_draw_rect;
        case 506: return (u64)&cssc_rt_video_draw_text;
        case 507: return (u64)&cssc_rt_video_present;
        case 508: return (u64)&cssc_rt_video_checksum;
#endif
        default: return 0;
    }
}

/* MS-x64 -> SysV trampoline: ctypes calls this (MS x64) with the address of
 * the emitted program's entry; it invokes the entry under the SysV ABI our
 * backend emits, so callee-saved-register expectations never mismatch. */
CSSC_EXPORT void cssc_rt_call_entry(u64 fn) { ((void (SYSV *)(void))fn)(); }

/* Harness flushes host stdout after running the emitted program so its output
 * is observable regardless of libc buffering. */
CSSC_EXPORT void cssc_rt_flush(void) { fflush(stdout); }
