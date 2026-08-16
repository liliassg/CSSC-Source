

typedef unsigned char       u8;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed   long long  i64;

#define PAGE            4096ull
#define MAX_FRAMES      (1ull << 20)

#define HEAP_CAP        (512ull * 1024 * 1024)
#define HDR             16u
#define BLK_MAGIC       0xCC05B10Cu

#define LIMINE_MEMMAP_USABLE 0

extern u64 cssc_memmap_count(void);
extern u64 cssc_memmap_base(u64 i);
extern u64 cssc_memmap_length(u64 i);
extern u64 cssc_memmap_type(u64 i);
extern u64 cssc_hhdm_offset(void);

static u8  frame_bitmap[MAX_FRAMES / 8];
static u64 total_frames = 0;
static u64 free_frames  = 0;
static u64 usable_bytes = 0;

static void bm_set(u64 f)   { frame_bitmap[f >> 3] |= (u8)(1u << (f & 7u)); }
static void bm_clear(u64 f) { frame_bitmap[f >> 3] &= (u8)~(1u << (f & 7u)); }
static int  bm_test(u64 f)  { return (frame_bitmap[f >> 3] >> (f & 7u)) & 1u; }

typedef struct { u64 size; u32 free; u32 magic; } hdr_t;

static u64 heap_base = 0;
static u64 heap_end  = 0;
static u64 heap_used = 0;

static u8  fallback_arena[8ull * 1024 * 1024] __attribute__((aligned(16)));

static u64 align16(u64 x) { return (x + 15u) & ~15ull; }

static void heap_init(u64 base_virt, u64 len) {
    heap_base = base_virt;
    heap_end  = base_virt + len;
    heap_used = 0;
    hdr_t *h = (hdr_t *)base_virt;
    h->size  = len - HDR;
    h->free  = 1;
    h->magic = BLK_MAGIC;
}

void cssc_mem_init(void) {

    for (u64 i = 0; i < sizeof(frame_bitmap); i++) frame_bitmap[i] = 0xFF;

    u64 best_base = 0, best_len = 0;
    u64 n = cssc_memmap_count();
    for (u64 i = 0; i < n; i++) {
        if (cssc_memmap_type(i) != LIMINE_MEMMAP_USABLE) continue;
        u64 base = cssc_memmap_base(i);
        u64 len  = cssc_memmap_length(i);
        usable_bytes += len;
        u64 first = (base + PAGE - 1) / PAGE;
        u64 last  = (base + len) / PAGE;
        for (u64 f = first; f < last && f < MAX_FRAMES; f++) {
            bm_clear(f);
            free_frames++;
            if (f + 1 > total_frames) total_frames = f + 1;
        }
        if (len > best_len) { best_len = len; best_base = base; }
    }

    u64 hhdm = cssc_hhdm_offset();
    if (best_len >= (1ull << 20) && hhdm != 0) {

        u64 hlen = best_len / 2;
        if (hlen > HEAP_CAP) hlen = HEAP_CAP;
        hlen &= ~(PAGE - 1);
        u64 hbase_phys = (best_base + PAGE - 1) & ~(PAGE - 1);
        u64 first = hbase_phys / PAGE;
        u64 last  = (hbase_phys + hlen) / PAGE;
        for (u64 f = first; f < last && f < MAX_FRAMES; f++) {
            if (!bm_test(f)) { bm_set(f); if (free_frames) free_frames--; }
        }
        heap_init(hhdm + hbase_phys, hlen);
    } else {

        heap_init((u64)fallback_arena, sizeof(fallback_arena));
    }
}

void *cssc_obj_alloc(i64 size) {
    if (heap_base == 0) heap_init((u64)fallback_arena, sizeof(fallback_arena));
    u64 need = align16((u64)size);
    if (need == 0) need = 16;
    u64 cur = heap_base;
    while (cur + HDR <= heap_end) {
        hdr_t *h = (hdr_t *)cur;
        if (h->free && h->size >= need) {
            if (h->size >= need + HDR + 16u) {
                u64 nxt = cur + HDR + need;
                hdr_t *nh = (hdr_t *)nxt;
                nh->size  = h->size - need - HDR;
                nh->free  = 1;
                nh->magic = BLK_MAGIC;
                h->size   = need;
            }
            h->free = 0;
            heap_used += h->size + HDR;
            return (void *)(cur + HDR);
        }
        cur = cur + HDR + h->size;
    }
    return (void *)0;
}

void cssc_obj_free(void *p) {
    if (!p) return;
    hdr_t *h = (hdr_t *)((u64)p - HDR);
    if (h->magic != BLK_MAGIC || h->free) return;
    h->free = 1;
    heap_used -= h->size + HDR;

    u64 cur = heap_base;
    while (cur + HDR <= heap_end) {
        hdr_t *c = (hdr_t *)cur;
        u64 nxt = cur + HDR + c->size;
        if (c->free && nxt + HDR <= heap_end) {
            hdr_t *nb = (hdr_t *)nxt;
            if (nb->free && nb->magic == BLK_MAGIC) {
                c->size += HDR + nb->size;
                continue;
            }
        }
        cur = nxt;
    }
}

u64 cssc_mem_usable(void)   { return usable_bytes; }
u64 cssc_heap_used(void)    { return heap_used; }
u64 cssc_frames_free(void)  { return free_frames; }
u64 cssc_frames_total(void) { return total_frames; }

#define MMIO_WINDOW_BASE  0xFFFFC00000000000ull
#define PTE_P             0x001ull
#define PTE_RW            0x002ull
#define PTE_PWT           0x008ull
#define PTE_PCD           0x010ull
#define PTE_PS            0x080ull
#define PTE_ADDR_MASK     0x000FFFFFFFFFF000ull

static u64 pt_alloc_frame(void) {
    for (u64 f = 1; f < total_frames; f++) {
        if (!bm_test(f)) {
            bm_set(f);
            if (free_frames) free_frames--;
            u64 phys = f * PAGE;
            u64 *v = (u64 *)(cssc_hhdm_offset() + phys);
            for (int i = 0; i < 512; i++) v[i] = 0;
            return phys;
        }
    }
    return 0;
}

static u64 pt_next(u64 *entry) {
    if (*entry & PTE_P) {
        if (*entry & PTE_PS) return 0;
        return *entry & PTE_ADDR_MASK;
    }
    u64 tbl = pt_alloc_frame();
    if (!tbl) return 0;
    *entry = tbl | PTE_P | PTE_RW;
    return tbl;
}

static u64 mmio_vcursor = 0;

static u64 mmio_pick_window(u64 hhdm, u64 pml4_phys) {
    u64 *t4 = (u64 *)(hhdm + pml4_phys);
    for (u64 s = 384; s <= 510; s++) {
        if (!(t4[s] & PTE_P))
            return 0xFFFF000000000000ull | (s << 39);
    }
    for (u64 s = 256; s < 384; s++) {
        if (!(t4[s] & PTE_P))
            return 0xFFFF000000000000ull | (s << 39);
    }
    return MMIO_WINDOW_BASE;
}

u64 cssc_map_mmio(u64 phys, u64 size) {
    u64 hhdm = cssc_hhdm_offset();
    if (hhdm == 0 || size == 0) return 0;
    u64 cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    u64 pml4_phys = cr3 & PTE_ADDR_MASK;

    if (mmio_vcursor == 0)
        mmio_vcursor = mmio_pick_window(hhdm, pml4_phys);

    u64 base  = phys & ~(PAGE - 1);
    u64 pages = (size + (phys - base) + PAGE - 1) / PAGE;
    u64 vbase = mmio_vcursor;
    mmio_vcursor += pages * PAGE;

    for (u64 p = 0; p < pages; p++) {
        u64 v  = vbase + p * PAGE;
        u64 ph = base  + p * PAGE;
        u64 i4 = (v >> 39) & 0x1FF, i3 = (v >> 30) & 0x1FF;
        u64 i2 = (v >> 21) & 0x1FF, i1 = (v >> 12) & 0x1FF;

        u64 *t4 = (u64 *)(hhdm + pml4_phys);
        u64 pdpt = pt_next(&t4[i4]);            if (!pdpt) return 0;
        u64 *t3 = (u64 *)(hhdm + pdpt);
        u64 pd   = pt_next(&t3[i3]);            if (!pd)   return 0;
        u64 *t2 = (u64 *)(hhdm + pd);
        u64 pt   = pt_next(&t2[i2]);            if (!pt)   return 0;
        u64 *t1 = (u64 *)(hhdm + pt);
        t1[i1] = ph | PTE_P | PTE_RW | PTE_PWT | PTE_PCD;
    }

    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
    return vbase + (phys - base);
}

#define NX_BIT (1ull << 63)
void cssc_make_exec(u64 addr, u64 size) {
    u64 hhdm = cssc_hhdm_offset();
    if (hhdm == 0 || size == 0) return;
    u64 cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    u64 pml4_phys = cr3 & PTE_ADDR_MASK;
    u64 base  = addr & ~(PAGE - 1);
    u64 pages = (size + (addr - base) + PAGE - 1) / PAGE;
    for (u64 p = 0; p < pages; p++) {
        u64 v  = base + p * PAGE;
        u64 i4 = (v >> 39) & 0x1FF, i3 = (v >> 30) & 0x1FF;
        u64 i2 = (v >> 21) & 0x1FF, i1 = (v >> 12) & 0x1FF;
        u64 *t4 = (u64 *)(hhdm + pml4_phys);
        if (!(t4[i4] & PTE_P)) continue;
        u64 *t3 = (u64 *)(hhdm + (t4[i4] & PTE_ADDR_MASK));
        if (!(t3[i3] & PTE_P)) continue;
        if (t3[i3] & PTE_PS) { t3[i3] = (t3[i3] & ~NX_BIT) | PTE_RW; continue; }
        u64 *t2 = (u64 *)(hhdm + (t3[i3] & PTE_ADDR_MASK));
        if (!(t2[i2] & PTE_P)) continue;
        if (t2[i2] & PTE_PS) { t2[i2] = (t2[i2] & ~NX_BIT) | PTE_RW; continue; }
        u64 *t1 = (u64 *)(hhdm + (t2[i2] & PTE_ADDR_MASK));
        if (!(t1[i1] & PTE_P)) continue;
        t1[i1] = (t1[i1] & ~NX_BIT) | PTE_RW;
    }
    __asm__ volatile ("mov %0, %%cr3" :: "r"(cr3) : "memory");
}
