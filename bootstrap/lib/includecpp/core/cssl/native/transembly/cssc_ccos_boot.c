

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;

#define LIMINE_COMMON_MAGIC 0xc7b1dd30df4c8b88ULL, 0x0a82e883a194f07bULL

__attribute__((used, section(".limine_requests")))
static volatile u64 limine_base_revision[3] = {
    0xf9562b2d5c95a6c8ULL, 0x6a7b384944536bdcULL, 3
};

__attribute__((used, section(".limine_requests_start")))
static volatile u64 limine_requests_start_marker[4] = {
    0xf6b8f4b39de7d1aeULL, 0xfab91a6940fcb9cfULL,
    0x785c6ed015d3e316ULL, 0x181e920a7852b9d9ULL
};

struct limine_framebuffer {
    void *address;
    u64 width, height, pitch;
    u16 bpp;
    u8 memory_model;
    u8 red_mask_size, red_mask_shift;
    u8 green_mask_size, green_mask_shift;
    u8 blue_mask_size, blue_mask_shift;
    u8 unused[7];
    u64 edid_size;
    void *edid;
    u64 mode_count;
    void *modes;
};
struct limine_framebuffer_response {
    u64 revision;
    u64 framebuffer_count;
    struct limine_framebuffer **framebuffers;
};
struct limine_framebuffer_request {
    u64 id[4];
    u64 revision;
    struct limine_framebuffer_response *response;
};
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x9d5827dcd881dd75ULL, 0xa3148604f6fab11bULL },
    .revision = 0, .response = 0
};

struct limine_hhdm_response { u64 revision; u64 offset; };
struct limine_hhdm_request  { u64 id[4]; u64 revision; struct limine_hhdm_response *response; };
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852ULL, 0x63984e959a98244bULL },
    .revision = 0, .response = 0
};

struct limine_memmap_entry { u64 base; u64 length; u64 type; };
struct limine_memmap_response { u64 revision; u64 entry_count; struct limine_memmap_entry **entries; };
struct limine_memmap_request  { u64 id[4]; u64 revision; struct limine_memmap_response *response; };
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806fULL, 0xe304acdfc50c3c62ULL },
    .revision = 0, .response = 0
};

struct limine_stack_size_response { u64 revision; };
struct limine_stack_size_request {
    u64 id[4]; u64 revision;
    struct limine_stack_size_response *response;
    u64 stack_size;
};
__attribute__((used, section(".limine_requests")))
static volatile struct limine_stack_size_request stack_size_request = {
    .id = { LIMINE_COMMON_MAGIC, 0x224ef0460a8e8926ULL, 0xe1cb0fc25f46ea3dULL },
    .revision = 0, .response = 0, .stack_size = 8ull * 1024 * 1024
};

__attribute__((used, section(".limine_requests_end")))
static volatile u64 limine_requests_end_marker[2] = {
    0xadc0e0531bb10d03ULL, 0x9572709f31764c62ULL
};

static struct limine_framebuffer *ccos_fb(void) {
    if (!framebuffer_request.response) return 0;
    if (framebuffer_request.response->framebuffer_count == 0) return 0;
    return framebuffer_request.response->framebuffers[0];
}

u64 cssc_fb_addr(void)   { struct limine_framebuffer *f = ccos_fb(); return f ? (u64)f->address : 0; }
u64 cssc_fb_width(void)  { struct limine_framebuffer *f = ccos_fb(); return f ? f->width  : 0; }
u64 cssc_fb_height(void) { struct limine_framebuffer *f = ccos_fb(); return f ? f->height : 0; }
u64 cssc_fb_pitch(void)  { struct limine_framebuffer *f = ccos_fb(); return f ? f->pitch  : 0; }
u64 cssc_fb_bpp(void)    { struct limine_framebuffer *f = ccos_fb(); return f ? (u64)f->bpp : 0; }

u64 cssc_hhdm_offset(void) { return hhdm_request.response ? hhdm_request.response->offset : 0; }

u64 cssc_memmap_count(void) { return memmap_request.response ? memmap_request.response->entry_count : 0; }
u64 cssc_memmap_base(u64 i) {
    if (!memmap_request.response || i >= memmap_request.response->entry_count) return 0;
    return memmap_request.response->entries[i]->base;
}
u64 cssc_memmap_length(u64 i) {
    if (!memmap_request.response || i >= memmap_request.response->entry_count) return 0;
    return memmap_request.response->entries[i]->length;
}
u64 cssc_memmap_type(u64 i) {
    if (!memmap_request.response || i >= memmap_request.response->entry_count) return 0;
    return memmap_request.response->entries[i]->type;
}

extern void cssc_runtime_init(void);
extern void cssc_mem_init(void);
extern void cssc_user_main(void);

static void enable_sse(void) {
    u64 cr0, cr4;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ull << 2);
    cr0 &= ~(1ull << 3);
    cr0 |=  (1ull << 1);
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(cr0));
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
    cr4 |=  (1ull << 9);
    cr4 |=  (1ull << 10);
    __asm__ __volatile__("mov %0, %%cr4" :: "r"(cr4));
    __asm__ __volatile__("fninit");
}

void kmain(void) {
    enable_sse();
    cssc_runtime_init();
    cssc_mem_init();
    cssc_user_main();
    for (;;) __asm__ __volatile__("cli; hlt");
}
