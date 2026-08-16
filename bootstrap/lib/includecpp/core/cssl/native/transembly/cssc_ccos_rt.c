

#include "../cssc_fmt_f64.h"

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed   long long  i64;
typedef unsigned long long  usize;

#define COM1 0x3F8

void cssc_outb(u64 port, u64 val) { __asm__ __volatile__("outb %0, %1" :: "a"((u8)val),  "Nd"((u16)port)); }
void cssc_outw(u64 port, u64 val) { __asm__ __volatile__("outw %0, %1" :: "a"((u16)val), "Nd"((u16)port)); }
void cssc_outl(u64 port, u64 val) { __asm__ __volatile__("outl %0, %1" :: "a"((u32)val), "Nd"((u16)port)); }

u64 cssc_inb(u64 port) { u8  r; __asm__ __volatile__("inb %1, %0" : "=a"(r) : "Nd"((u16)port)); return (u64)r; }
u64 cssc_inw(u64 port) { u16 r; __asm__ __volatile__("inw %1, %0" : "=a"(r) : "Nd"((u16)port)); return (u64)r; }
u64 cssc_inl(u64 port) { u32 r; __asm__ __volatile__("inl %1, %0" : "=a"(r) : "Nd"((u16)port)); return (u64)r; }

void cssc_poke8 (u64 a, u64 v) { *(volatile u8  *)a = (u8 )v; }
void cssc_poke16(u64 a, u64 v) { *(volatile u16 *)a = (u16)v; }
void cssc_poke32(u64 a, u64 v) { *(volatile u32 *)a = (u32)v; }
void cssc_poke64(u64 a, u64 v) { *(volatile u64 *)a = (u64)v; }

u64 cssc_peek8 (u64 a) { return (u64)*(volatile u8  *)a; }
u64 cssc_peek16(u64 a) { return (u64)*(volatile u16 *)a; }
u64 cssc_peek32(u64 a) { return (u64)*(volatile u32 *)a; }
u64 cssc_peek64(u64 a) { return (u64)*(volatile u64 *)a; }

void cssc_memfill32(u64 addr, u64 val, u64 count) {
    volatile u32 *p = (volatile u32 *)addr;
    u32 v = (u32)val;
    for (u64 i = 0; i < count; i++) p[i] = v;
}
void cssc_memcopy(u64 dst, u64 src, u64 count) {
    volatile u32 *d = (volatile u32 *)dst;
    const volatile u32 *s = (const volatile u32 *)src;
    for (u64 i = 0; i < count; i++) d[i] = s[i];
}

extern void *cssc_obj_alloc(i64 size);
extern void  cssc_obj_free(void *p);
u64  cssc_alloc_raw(u64 n) { return (u64)cssc_obj_alloc((i64)n); }

void cssc_free_raw(u64 p) { cssc_obj_free((void *)p); }

u64 cssc_read_cs(void) { u64 v; __asm__ __volatile__("mov %%cs, %0" : "=r"(v)); return v & 0xFFFF; }

void *cssc_isr_frame = 0;
u64 cssc_isr_frame_get(void) { return (u64)cssc_isr_frame; }
u64 cssc_read_rsp(void) { u64 v; __asm__ __volatile__("mov %%rsp, %0" : "=r"(v)); return v; }

i64 __cssc_bitcast_ptr_i64(void *p) { return (i64)(u64)p; }

static const u8 font8x8[256][8] = {
    [' '] = {0,0,0,0,0,0,0,0},
    ['.'] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    [':'] = {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00},
    ['-'] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
    ['/'] = {0x00,0x06,0x0C,0x18,0x30,0x60,0x00,0x00},
    ['0'] = {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    ['1'] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    ['2'] = {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
    ['3'] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    ['4'] = {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00},
    ['5'] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    ['6'] = {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    ['7'] = {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    ['8'] = {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    ['9'] = {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
    ['A'] = {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00},
    ['B'] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    ['C'] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    ['D'] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    ['E'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    ['F'] = {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    ['G'] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00},
    ['H'] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    ['I'] = {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    ['J'] = {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00},
    ['K'] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    ['L'] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    ['M'] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    ['N'] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00},
    ['O'] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['P'] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    ['Q'] = {0x3C,0x66,0x66,0x66,0x66,0x3C,0x0E,0x00},
    ['R'] = {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00},
    ['S'] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    ['T'] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['U'] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    ['V'] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    ['W'] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    ['X'] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    ['Y'] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    ['Z'] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
    ['a'] = {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
    ['b'] = {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00},
    ['c'] = {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},
    ['d'] = {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00},
    ['e'] = {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00},
    ['f'] = {0x1C,0x30,0x30,0x7C,0x30,0x30,0x30,0x00},
    ['g'] = {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x7C},
    ['h'] = {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00},
    ['i'] = {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},
    ['j'] = {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x6C,0x38},
    ['k'] = {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00},
    ['l'] = {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    ['m'] = {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00},
    ['n'] = {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00},
    ['o'] = {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    ['p'] = {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60},
    ['q'] = {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06},
    ['r'] = {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00},
    ['s'] = {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00},
    ['t'] = {0x30,0x30,0x7C,0x30,0x30,0x36,0x1C,0x00},
    ['u'] = {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
    ['v'] = {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00},
    ['w'] = {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00},
    ['x'] = {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00},
    ['y'] = {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x7C},
    ['z'] = {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00},
    ['!'] = {0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x00},
    ['"'] = {0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00},
    ['#'] = {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    ['$'] = {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
    ['%'] = {0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00},
    ['&'] = {0x38,0x6C,0x38,0x76,0x6C,0x6C,0x76,0x00},
    ['\''] = {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00},
    ['('] = {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},
    [')'] = {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},
    ['*'] = {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},
    ['+'] = {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    [','] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30},
    [';'] = {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30},
    ['<'] = {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00},
    ['='] = {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    ['>'] = {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00},
    ['?'] = {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00},
    ['@'] = {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00},
    ['['] = {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00},
    ['\\'] = {0x00,0x60,0x30,0x18,0x0C,0x06,0x00,0x00},
    [']'] = {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00},
    ['^'] = {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00},
    ['_'] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF},
    ['`'] = {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00},
    ['{'] = {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00},
    ['|'] = {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    ['}'] = {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00},
    ['~'] = {0x00,0x00,0x76,0xDC,0x00,0x00,0x00,0x00},
    [228] = {0x6C,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},
    [246] = {0x6C,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    [252] = {0x6C,0x00,0x66,0x66,0x66,0x66,0x3E,0x00},
    [196] = {0x6C,0x00,0x18,0x3C,0x66,0x7E,0x66,0x00},
    [214] = {0x6C,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},
    [220] = {0x6C,0x00,0x66,0x66,0x66,0x66,0x3C,0x00},
    [223] = {0x38,0x6C,0x6C,0x78,0x6C,0x66,0x7C,0x60},
    [167] = {0x3C,0x66,0x38,0x6C,0x6C,0x1C,0x66,0x3C},
    [176] = {0x3C,0x66,0x66,0x3C,0x00,0x00,0x00,0x00},
    [181] = {0x00,0x00,0x66,0x66,0x66,0x7C,0x60,0x60},
    [128] = {0x1C,0x36,0x60,0xFC,0x60,0x36,0x1C,0x00},
};
u64 cssc_font_row(u64 c, u64 row) {
    if (c > 255 || row > 7) return 0;
    return (u64)font8x8[c][row];
}

u64 cssc_shl(u64 a, u64 b)  { return a << b; }
u64 cssc_shr(u64 a, u64 b)  { return a >> b; }
u64 cssc_band(u64 a, u64 b) { return a & b; }
u64 cssc_bor(u64 a, u64 b)  { return a | b; }
u64 cssc_bxor(u64 a, u64 b) { return a ^ b; }

void cssc_hlt(void)   { __asm__ __volatile__("hlt"); }
void cssc_cli(void)   { __asm__ __volatile__("cli" ::: "memory"); }
void cssc_sti(void)   { __asm__ __volatile__("sti" ::: "memory"); }
void cssc_pause(void) { __asm__ __volatile__("pause"); }

void cssc_lgdt(u64 addr) { __asm__ __volatile__("lgdt (%0)" :: "r"(addr) : "memory"); }
void cssc_lidt(u64 addr) { __asm__ __volatile__("lidt (%0)" :: "r"(addr) : "memory"); }
void cssc_ltr (u64 sel)  { __asm__ __volatile__("ltr %w0"   :: "r"((u16)sel)); }

u64  cssc_read_cr0(void) { u64 v; __asm__ __volatile__("mov %%cr0, %0" : "=r"(v)); return v; }
u64  cssc_read_cr2(void) { u64 v; __asm__ __volatile__("mov %%cr2, %0" : "=r"(v)); return v; }
u64  cssc_read_cr3(void) { u64 v; __asm__ __volatile__("mov %%cr3, %0" : "=r"(v)); return v; }
u64  cssc_read_cr4(void) { u64 v; __asm__ __volatile__("mov %%cr4, %0" : "=r"(v)); return v; }
void cssc_write_cr0(u64 v){ __asm__ __volatile__("mov %0, %%cr0" :: "r"(v) : "memory"); }
void cssc_write_cr3(u64 v){ __asm__ __volatile__("mov %0, %%cr3" :: "r"(v) : "memory"); }
void cssc_write_cr4(u64 v){ __asm__ __volatile__("mov %0, %%cr4" :: "r"(v) : "memory"); }

void cssc_invlpg(u64 addr){ __asm__ __volatile__("invlpg (%0)" :: "r"(addr) : "memory"); }

u64  cssc_rdmsr(u64 msr) {
    u32 lo, hi;
    __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"((u32)msr));
    return ((u64)hi << 32) | (u64)lo;
}
void cssc_wrmsr(u64 msr, u64 val) {
    __asm__ __volatile__("wrmsr" :: "c"((u32)msr), "a"((u32)val), "d"((u32)(val >> 32)));
}
u64 cssc_rdtsc(void) {
    u32 lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((u64)hi << 32) | (u64)lo;
}

static char *cssc_cap_buf = 0;
static i64   cssc_cap_cap = 0;
static i64   cssc_cap_len = 0;
void cssc_out_capture(void *buf, i64 cap) {
    cssc_cap_buf = (char *)buf; cssc_cap_cap = cap; cssc_cap_len = 0;
}
i64  cssc_out_capture_len(void) { return cssc_cap_len; }
void cssc_out_capture_end(void) { cssc_cap_buf = 0; cssc_cap_cap = 0; }

static void serial_raw_putc(char c) {
    while ((cssc_inb(COM1 + 5) & 0x20) == 0) { }
    cssc_outb(COM1, (u8)c);
}
static void serial_putc(char c) {
    if (cssc_cap_buf) {
        if (cssc_cap_len < cssc_cap_cap) cssc_cap_buf[cssc_cap_len++] = c;
        return;
    }
    if (c == '\n') serial_raw_putc('\r');
    serial_raw_putc(c);
}
static void serial_puts(const char *s) {
    while (*s) serial_putc(*s++);
}

void cssc_runtime_init(void) {
    cssc_outb(COM1 + 1, 0x00);
    cssc_outb(COM1 + 3, 0x80);
    cssc_outb(COM1 + 0, 0x01);
    cssc_outb(COM1 + 1, 0x00);
    cssc_outb(COM1 + 3, 0x03);
    cssc_outb(COM1 + 2, 0xC7);
    cssc_outb(COM1 + 4, 0x0B);
}
void cssc_runtime_shutdown(void) { }

void *memcpy(void *dst, const void *src, usize n) {
    u8 *d = (u8 *)dst; const u8 *s = (const u8 *)src;
    for (usize i = 0; i < n; i++) d[i] = s[i];
    return dst;
}
void *memset(void *dst, int c, usize n) {
    u8 *d = (u8 *)dst;
    for (usize i = 0; i < n; i++) d[i] = (u8)c;
    return dst;
}
void *memmove(void *dst, const void *src, usize n) {
    u8 *d = (u8 *)dst; const u8 *s = (const u8 *)src;
    if (d < s) { for (usize i = 0; i < n; i++) d[i] = s[i]; }
    else       { for (usize i = n; i > 0; i--) d[i - 1] = s[i - 1]; }
    return dst;
}
int memcmp(const void *a, const void *b, usize n) {
    const u8 *x = (const u8 *)a, *y = (const u8 *)b;
    for (usize i = 0; i < n; i++) { if (x[i] != y[i]) return (int)x[i] - (int)y[i]; }
    return 0;
}

extern void *cssc_obj_alloc(i64 size);
extern void  cssc_obj_free(void *p);

typedef struct { u32 refcount; u32 size; char data[]; } cssc_str;

void *cssc_string_lit(const char *src, i64 len) {
    u64 n = (u64)len;
    cssc_str *s = (cssc_str *)cssc_obj_alloc((i64)(8 + n + 1));
    s->refcount = 1;
    s->size = (u32)n;
    for (u64 i = 0; i < n; i++) s->data[i] = src[i];
    s->data[n] = 0;
    return s;
}

void  cssc_string_free(void *s) { cssc_obj_free(s); }

typedef struct { void *param; void *caller; } cssc_link;

#define CSSC_MAX_PENDING 16
#define CSSC_MAX_ALIASES 512
#define CSSC_MAX_FRAMES  256

static cssc_link cssc_pending_links[CSSC_MAX_PENDING];
static int cssc_pending_count = 0;
static cssc_link cssc_alias_frames[CSSC_MAX_ALIASES];
static int cssc_alias_frame_starts[CSSC_MAX_FRAMES];
static int cssc_alias_frame_ends[CSSC_MAX_FRAMES];
static int cssc_alias_depth = 0;
static int cssc_alias_total = 0;

void cssc_arg_link(void *param, void *caller) {
    if (cssc_pending_count >= CSSC_MAX_PENDING) return;
    cssc_pending_links[cssc_pending_count].param  = param;
    cssc_pending_links[cssc_pending_count].caller = caller;
    cssc_pending_count++;
}

void cssc_scope_push(void) {
    int d = cssc_alias_depth;
    if (d < CSSC_MAX_FRAMES) {
        int start = cssc_alias_total;
        int copied = 0;
        for (int i = 0; i < cssc_pending_count &&
                        (start + copied) < CSSC_MAX_ALIASES; i++) {
            cssc_alias_frames[start + copied] = cssc_pending_links[i];
            copied++;
        }
        cssc_alias_frame_starts[d] = start;
        cssc_alias_frame_ends[d]   = start + copied;
        cssc_alias_total           = start + copied;
    }
    cssc_alias_depth = d + 1;
    cssc_pending_count = 0;
}

void cssc_scope_pop(void) {
    if (cssc_alias_depth == 0) return;
    cssc_alias_depth--;
    if (cssc_alias_depth < CSSC_MAX_FRAMES)
        cssc_alias_total = cssc_alias_frame_starts[cssc_alias_depth];
}

void cssc_scope_delete_aliased(void *name) {
    if (cssc_alias_depth == 0) return;
    int d = cssc_alias_depth - 1;
    if (d >= CSSC_MAX_FRAMES) return;
    int start = cssc_alias_frame_starts[d];
    int end   = cssc_alias_frame_ends[d];
    for (int i = start; i < end; i++) {
        if (cssc_alias_frames[i].param == name) {
            if (cssc_alias_frames[i].caller)
                *(volatile i64 *)cssc_alias_frames[i].caller = 0;
            cssc_alias_frames[i].param = (void *)0;
            return;
        }
    }
}

i64 cssc_load_i64_at(void *slot) { return slot ? *(i64 *)slot : 0; }
i64   cssc_string_size(void *s) { return (i64)((cssc_str *)s)->size; }

void *cssc_string_char_at(void *s, i64 i) {
    cssc_str *x = (cssc_str *)s;
    if (!x || i < 0 || (u64)i >= x->size) return cssc_string_lit("", 0);
    return cssc_string_lit(&x->data[i], 1);
}
void *cssc_string_data(void *s) { return (void *)((cssc_str *)s)->data; }

void *cssc_string_concat(void *a, void *b) {
    cssc_str *sa = (cssc_str *)a, *sb = (cssc_str *)b;
    u64 na = sa->size, nb = sb->size, tot = na + nb;
    cssc_str *r = (cssc_str *)cssc_obj_alloc((i64)(8 + tot + 1));
    r->refcount = 1;
    r->size = (u32)tot;
    for (u64 i = 0; i < na; i++) r->data[i]      = sa->data[i];
    for (u64 i = 0; i < nb; i++) r->data[na + i] = sb->data[i];
    r->data[tot] = 0;
    return r;
}
void *cssc_string_copy(void *s) {
    cssc_str *x = (cssc_str *)s;
    return cssc_string_lit(x->data, (i64)x->size);
}

typedef struct { i64 len; i64 cap; i64 *data; } cssc_vec;

void *cssc_vec_new(i64 cap) {
    if (cap < 4) cap = 4;
    cssc_vec *v = (cssc_vec *)cssc_obj_alloc((i64)sizeof(cssc_vec));
    v->len = 0; v->cap = cap;
    v->data = (i64 *)cssc_obj_alloc(cap * 8);
    return v;
}
void cssc_vec_push(void *p, i64 x) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return;
    if (v->len == v->cap) {
        i64 nc = v->cap * 2;
        i64 *nd = (i64 *)cssc_obj_alloc(nc * 8);
        for (i64 i = 0; i < v->len; i++) nd[i] = v->data[i];
        cssc_obj_free(v->data);
        v->data = nd; v->cap = nc;
    }
    v->data[v->len++] = x;
}
i64 cssc_vec_at(void *p, i64 i) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || i < 0 || i >= v->len) return 0;
    return v->data[i];
}
i64 cssc_vec_size(void *p) { cssc_vec *v = (cssc_vec *)p; return v ? v->len : 0; }
void cssc_vec_set(void *p, i64 i, i64 x) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || i < 0 || i >= v->len) return;
    v->data[i] = x;
}

i64 cssc_vec_pop_front(void *p) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || v->len == 0) return 0;
    i64 r = v->data[0];
    for (i64 i = 1; i < v->len; i++) v->data[i - 1] = v->data[i];
    v->len--;
    return r;
}
i64 cssc_vec_pop_back(void *p) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v || v->len == 0) return 0;
    v->len--;
    return v->data[v->len];
}

i64 cssc_string_eq(void *a, void *b);
typedef struct { i64 len; i64 cap; void **keys; i64 *vals; } cssc_map;

void *cssc_map_new(i64 cap) {
    if (cap < 4) cap = 4;
    cssc_map *m = (cssc_map *)cssc_obj_alloc((i64)sizeof(cssc_map));
    m->len = 0; m->cap = cap;
    m->keys = (void **)cssc_obj_alloc(cap * 8);
    m->vals = (i64 *)cssc_obj_alloc(cap * 8);
    return m;
}
void cssc_map_set(void *p, void *k, i64 v) {
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
i64 cssc_map_get(void *p, void *k) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return 0;
    for (i64 i = 0; i < m->len; i++)
        if (cssc_string_eq(m->keys[i], k)) return m->vals[i];
    return 0;
}
i64 cssc_map_size(void *p) { cssc_map *m = (cssc_map *)p; return m ? m->len : 0; }
i64 cssc_map_has(void *p, void *k) {
    cssc_map *m = (cssc_map *)p;
    if (!m) return 0;
    for (i64 i = 0; i < m->len; i++) if (cssc_string_eq(m->keys[i], k)) return 1;
    return 0;
}

void cssc_release(void *s);
void cssc_vec_free(void *p, i64 elemIsStr) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return;
    if (elemIsStr) for (i64 i = 0; i < v->len; i++) cssc_release((void *)v->data[i]);
    cssc_obj_free(v->data);
    cssc_obj_free(v);
}
void cssc_map_free(void *p, i64 valIsStr) {
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

void *cssc_vec_copy(void *p, i64 elemIsStr) {
    cssc_vec *v = (cssc_vec *)p;
    if (!v) return cssc_vec_new(4);
    cssc_vec *c = (cssc_vec *)cssc_vec_new(v->cap);
    for (i64 i = 0; i < v->len; i++) {
        i64 cell = v->data[i];
        c->data[i] = (elemIsStr && cell) ? (i64)cssc_string_copy((void *)cell)
                                         : cell;
    }
    c->len = v->len;
    return c;
}
void *cssc_map_copy(void *p, i64 valIsStr) {
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

i64 cssc_string_eq(void *a, void *b) {
    cssc_str *x = (cssc_str *)a, *y = (cssc_str *)b;
    if (x == y) return 1;
    if (!x || !y) return 0;
    if (x->size != y->size) return 0;
    for (u32 i = 0; i < x->size; i++) if (x->data[i] != y->data[i]) return 0;
    return 1;
}
void cssc_retain(void *s)  { if (s) ((cssc_str *)s)->refcount++; }
void cssc_release(void *s) { if (s) { cssc_str *x = (cssc_str *)s; if (x->refcount) { if (--x->refcount == 0) cssc_string_free(s); } } }

static i64 fmt_i64(char *buf, i64 v) {

    int neg = 0;
    u64 u;
    char tmp[24];
    int  ti = 0, bi = 0;
    if (v < 0) { neg = 1; u = (u64)(-(v + 1)) + 1u; } else { u = (u64)v; }
    if (u == 0) tmp[ti++] = '0';
    while (u) { tmp[ti++] = (char)('0' + (int)(u % 10u)); u /= 10u; }
    if (neg) buf[bi++] = '-';
    while (ti) buf[bi++] = tmp[--ti];
    return bi;
}
void *cssc_int_to_str(i64 n) {
    char buf[24];
    i64 len = fmt_i64(buf, n);
    return cssc_string_lit(buf, len);
}
void *cssc_bool_to_str(int b) { return b ? cssc_string_lit("true", 4) : cssc_string_lit("false", 5); }

static i64 fmt_f64(char *buf, double x) {
    return (i64)cssc_fmt_f64_shortest(buf, x);
}
void *cssc_float_to_str(double x) {
    char buf[48];
    i64 len = fmt_f64(buf, x);
    return cssc_string_lit(buf, len);
}

typedef union { double d; u64 u; } cssc_fbits;
static double cssc__f(u64 u) { cssc_fbits b; b.u = u; return b.d; }
static u64    cssc__u(double d) { cssc_fbits b; b.d = d; return b.u; }
static double cssc__ftrunc(double x) {
    if (x != x) return x;
    if (x < 0) { double t = -x; u64 i = (u64)t; return -(double)i; }
    u64 i = (u64)x; return (double)i;
}
static double cssc__ffloor(double x) { double t = cssc__ftrunc(x); if (t > x) t -= 1.0; return t; }
static double cssc__fceil(double x)  { double t = cssc__ftrunc(x); if (t < x) t += 1.0; return t; }
static double cssc__fround(double x) { if (x < 0) return -cssc__ffloor(-x + 0.5); return cssc__ffloor(x + 0.5); }
u64 cssc_fadd(u64 a, u64 b) { return cssc__u(cssc__f(a) + cssc__f(b)); }
u64 cssc_fsub(u64 a, u64 b) { return cssc__u(cssc__f(a) - cssc__f(b)); }
u64 cssc_fmul(u64 a, u64 b) { return cssc__u(cssc__f(a) * cssc__f(b)); }
u64 cssc_fdiv(u64 a, u64 b) { return cssc__u(cssc__f(a) / cssc__f(b)); }
u64 cssc_fmod_(u64 a, u64 b) {
    double x = cssc__f(a), y = cssc__f(b);
    if (y == 0.0) { cssc_fbits n; n.u = 0x7FF8000000000000ULL; return n.u; }
    return cssc__u(x - cssc__ftrunc(x / y) * y);
}
u64 cssc_fneg(u64 a) { return a ^ 0x8000000000000000ULL; }
u64 cssc_fcmp(u64 a, u64 b) {
    double x = cssc__f(a), y = cssc__f(b);
    if (x < y) return (u64)(i64)-1;
    if (x > y) return 1;
    if (x == y) return 0;
    return 2;
}
u64 cssc_i2f(u64 i) { return cssc__u((double)(i64)i); }
u64 cssc_f2i(u64 f) { return (u64)(i64)cssc__f(f); }
u64 cssc_fabs(u64 f) { return f & 0x7FFFFFFFFFFFFFFFULL; }
u64 cssc_ffloor(u64 f) { return cssc__u(cssc__ffloor(cssc__f(f))); }
u64 cssc_fceil(u64 f)  { return cssc__u(cssc__fceil(cssc__f(f))); }
u64 cssc_ftrunc(u64 f) { return cssc__u(cssc__ftrunc(cssc__f(f))); }
u64 cssc_fround(u64 f) { return cssc__u(cssc__fround(cssc__f(f))); }
u64 cssc_fsqrt(u64 f) {
    double a = cssc__f(f);
    if (a < 0) { cssc_fbits n; n.u = 0x7FF8000000000000ULL; return n.u; }
    if (a == 0.0) return cssc__u(0.0);
    double x = a; int i; for (i = 0; i < 40; i++) { x = 0.5 * (x + a / x); }
    return cssc__u(x);
}
u64 cssc_f2str(u64 f, u64 buf, u64 cap) {
    double x = cssc__f(f);
    char *out = (char *)buf;
    i64 n = 0, c = (i64)cap;
    if (x != x) { if (c >= 3) { out[0]='n'; out[1]='a'; out[2]='n'; } return 3; }
    if (x < 0) { if (n < c) out[n++] = '-'; x = -x; }
    i64 ip = (i64)x;
    char tmp[24]; int t = 0;
    if (ip == 0) { tmp[t++] = '0'; } else { i64 v = ip; while (v > 0) { tmp[t++] = (char)('0' + (v % 10)); v /= 10; } }
    while (t > 0) { t--; if (n < c) out[n++] = tmp[t]; }
    if (n < c) out[n++] = '.';
    double frac = x - (double)ip;
    int k; for (k = 0; k < 6; k++) { frac *= 10.0; int digit = (int)frac; if (digit < 0) digit = 0; if (digit > 9) digit = 9; if (n < c) out[n++] = (char)('0' + digit); frac -= (double)digit; }
    return (u64)n;
}
u64 cssc_str2f(u64 ptr, u64 len) {
    const char *s = (const char *)ptr;
    i64 n = (i64)len, i = 0; int neg = 0;
    if (i < n && (s[i] == '-' || s[i] == '+')) { neg = (s[i] == '-'); i++; }
    double v = 0.0;
    while (i < n && s[i] >= '0' && s[i] <= '9') { v = v * 10.0 + (s[i] - '0'); i++; }
    if (i < n && s[i] == '.') { i++; double scale = 0.1; while (i < n && s[i] >= '0' && s[i] <= '9') { v += (s[i] - '0') * scale; scale *= 0.1; i++; } }
    if (neg) v = -v;
    return cssc__u(v);
}

void cssc_print_newline(void) { serial_putc('\n'); }

void cssc_out_int(i64 n)       { char b[24]; i64 l = fmt_i64(b, n); for (i64 i=0;i<l;i++) serial_putc(b[i]); }
void cssc_out_float(double f)  { char b[48]; i64 l = fmt_f64(b, f); for (i64 i=0;i<l;i++) serial_putc(b[i]); }
void cssc_out_bool(int bb)     { serial_puts(bb ? "true" : "false"); }
void cssc_out_str(const char *s, i64 len) { for (i64 i=0;i<len;i++) serial_putc(s[i]); }
void cssc_out_string(void *s)  { cssc_str *x = (cssc_str *)s; for (u32 i=0;i<x->size;i++) serial_putc(x->data[i]); }

void cssc_print_int(i64 n)       { cssc_out_int(n);   serial_putc('\n'); }
void cssc_print_float(double f)  { cssc_out_float(f); serial_putc('\n'); }
void cssc_print_bool(int bb)     { cssc_out_bool(bb); serial_putc('\n'); }
void cssc_print_str(const char *s, i64 len) { cssc_out_str(s, len); serial_putc('\n'); }
void cssc_print_string(void *s)  { cssc_out_string(s); serial_putc('\n'); }

void cssc_out_null(void)   { serial_puts("0x0"); }
void cssc_print_null(void) { serial_puts("0x0"); serial_putc('\n'); }

void cssc_panic(const char *s, i64 len) {
    serial_puts("\n*** CCOS PANIC: ");
    if (s) for (i64 i = 0; i < len; i++) serial_putc(s[i]);
    serial_putc('\n');
    cssc_cli();
    for (;;) cssc_hlt();
}
void cssc_panic_msg(void *s) {
    cssc_str *x = (cssc_str *)s;
    if (x) cssc_panic(x->data, (i64)x->size);
    else   cssc_panic((const char *)0, 0);
}

int  cssc_argc = 0;
void *cssc_argv = (void *)0;
int  cssc_sysout_value = 0;
void cssc_sysout(i64 v)       { cssc_sysout_value = (int)v; }
i64  cssc_sysarg_int(i64 idx) { (void)idx; return 0; }

void cssc_call_addr(u64 a)     { ((void (*)(void))a)(); }
i64  cssc_call_addr_i64(u64 a) { return ((i64 (*)(void))a)(); }

u64 cssc_rtsym(u64 id) {
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
        case 41: return (u64)&cssc_string_size;
        default: return 0;
    }
}
