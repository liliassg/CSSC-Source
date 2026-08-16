/*
 * CSSC Native Runtime — Header
 * =============================
 * Core type system, memory management, builtins, and scope stack
 * for compiled CSSC executables.
 *
 * Every compiled .exe links against cssc_runtime.lib which provides
 * this entire API. The ASMH hotload .dll also links against it.
 *
 * Architecture:
 *   - Tagged union value system (CsscVal) with 16-byte representation
 *   - Reference-counted heap objects (strings, vectors, maps, binds)
 *   - Stack-based scope chain for sector/object isolation
 *   - Hash map for variable lookup within each scope frame
 *
 * (c) 2026 Lilias Hatterscheidt — IncludeCPP / CSSeries
 */

#ifndef CSSC_RUNTIME_H
#define CSSC_RUNTIME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef _WIN32
    #ifdef CSSC_RUNTIME_EXPORTS
        #define CSSC_API __declspec(dllexport)
    #else
        #define CSSC_API __declspec(dllimport)
    #endif
#else
    #define CSSC_API
#endif

/* Portable "the compiler may see this as unused on some target" marker.
 * Used to silence -Wunused-function on helpers that only have callers
 * inside a platform-gated #ifdef (e.g. Win32-only HTTP, ESP-only MMIO). */
#if defined(__GNUC__) || defined(__clang__)
    #define CSSC_UNUSED __attribute__((unused))
#else
    #define CSSC_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 1. TYPE SYSTEM — Tagged Union
 * =========================================================================
 * Every CSSC value is a 16-byte tagged union:
 *   [8 bytes tag + flags] [8 bytes payload]
 *
 * The tag encodes the type AND metadata (const, method-bound, region).
 * The payload is either an immediate value (int/float/bool) or a pointer
 * to a heap-allocated object (string, vector, map, sector, object).
 */

/* Type tags — stored in the low 8 bits of the tag field */
typedef enum {
    CSSC_TYPE_NULL     = 0,
    CSSC_TYPE_INT      = 1,
    CSSC_TYPE_FLOAT    = 2,
    CSSC_TYPE_BOOL     = 3,
    CSSC_TYPE_STRING   = 4,
    CSSC_TYPE_VECTOR   = 5,
    CSSC_TYPE_MAP      = 6,
    CSSC_TYPE_BIND     = 7,
    CSSC_TYPE_FUNCTION = 8,
    CSSC_TYPE_SECTOR   = 9,
    CSSC_TYPE_OBJECT   = 10,
    CSSC_TYPE_POINTER  = 11,   /* CsscPointer — name-based live reference */
    CSSC_TYPE_MATRIX   = 12,
    CSSC_TYPE_ITERATOR = 13,
    CSSC_TYPE_MODULE   = 14,   /* #load/#include result */
    CSSC_TYPE_METHOD   = 15,   /* CsscMethod definition */
    CSSC_TYPE_BINDING  = 16,   /* CsscMethodBinding wrapper */
    CSSC_TYPE_CONSOLE  = 17,   /* sys.console kernel console handle */
    CSSC_TYPE_MAX      = 18
} CsscTypeTag;

/* Flag bits — stored in bits 8-31 of the tag field */
#define CSSC_FLAG_CONST      (1 << 8)   /* immutable after first set */
#define CSSC_FLAG_HEAP       (1 << 9)   /* allocated on heap (auto-free) */
#define CSSC_FLAG_STACK      (1 << 10)  /* allocated on stack (manual #delete) */
#define CSSC_FLAG_AUTO       (1 << 11)  /* auto-resizing stack */
#define CSSC_FLAG_METHOD     (1 << 12)  /* has method binding */
#define CSSC_FLAG_FREED      (1 << 13)  /* has been #deleted */
#define CSSC_FLAG_SCOPE_ALIAS (1 << 14) /* entry is a #req[outer] *local; alias —
                                          data.ptr is the interned target name.
                                          scope_get/set follow the alias instead
                                          of returning/storing the raw value. */

/* The core value type — 16 bytes, fits in an SSE register pair */
typedef struct {
    uint64_t tag;       /* low 8 bits = type, bits 8-31 = flags, bits 32-63 = alloc_bits */
    union {
        int64_t    i;           /* CSSC_TYPE_INT */
        double     f;           /* CSSC_TYPE_FLOAT */
        bool       b;           /* CSSC_TYPE_BOOL */
        void*      ptr;         /* all heap-allocated types (string, vector, map, etc.) */
        uint64_t   raw;         /* raw access for move/copy */
    } data;
} CsscVal;

/* Tag extraction macros */
#define CSSC_TYPE(v)       ((CsscTypeTag)((v).tag & 0xFF))
#define CSSC_FLAGS(v)      ((v).tag & 0xFFFFFF00ULL)
#define CSSC_ALLOC_BITS(v) ((uint32_t)((v).tag >> 32))
#define CSSC_IS_NULL(v)    (CSSC_TYPE(v) == CSSC_TYPE_NULL)
#define CSSC_IS_INT(v)     (CSSC_TYPE(v) == CSSC_TYPE_INT)
#define CSSC_IS_FLOAT(v)   (CSSC_TYPE(v) == CSSC_TYPE_FLOAT)
#define CSSC_IS_BOOL(v)    (CSSC_TYPE(v) == CSSC_TYPE_BOOL)
#define CSSC_IS_STRING(v)  (CSSC_TYPE(v) == CSSC_TYPE_STRING)
#define CSSC_IS_VECTOR(v)  (CSSC_TYPE(v) == CSSC_TYPE_VECTOR)
#define CSSC_IS_MAP(v)     (CSSC_TYPE(v) == CSSC_TYPE_MAP)
#define CSSC_IS_NUMERIC(v) (CSSC_IS_INT(v) || CSSC_IS_FLOAT(v))
#define CSSC_HAS_FLAG(v,f) (((v).tag & (f)) != 0)

/* =========================================================================
 * 2. VALUE CONSTRUCTORS
 * ========================================================================= */

CSSC_API CsscVal cssc_null(void);
CSSC_API CsscVal cssc_int(int64_t value);
CSSC_API CsscVal cssc_float(double value);
CSSC_API CsscVal cssc_bool(bool value);
CSSC_API CsscVal cssc_string(const char* str);
CSSC_API CsscVal cssc_string_len(const char* str, size_t len);
CSSC_API CsscVal cssc_string_owned(char* str);  /* takes ownership, no copy */
CSSC_API CsscVal cssc_vector(size_t initial_capacity);
CSSC_API CsscVal cssc_map(size_t initial_capacity);
CSSC_API CsscVal cssc_bind(void);

/* =========================================================================
 * 3. VALUE ACCESS — Extract native values (no copy for heap types)
 * ========================================================================= */

CSSC_API int64_t     cssc_to_int(CsscVal v);
CSSC_API double      cssc_to_float(CsscVal v);
CSSC_API bool        cssc_to_bool(CsscVal v);
CSSC_API const char* cssc_to_cstr(CsscVal v);       /* borrowed pointer, DO NOT free */
CSSC_API size_t      cssc_strlen(CsscVal v);
CSSC_API bool        cssc_is_truthy(CsscVal v);
CSSC_API const char* cssc_typeof_str(CsscVal v);     /* "int", "float", "string", etc. */

/* =========================================================================
 * 4. HEAP OBJECT — Reference-counted string/vector/map internals
 * ========================================================================= */

/* All heap-allocated CSSC objects share this header */
typedef struct {
    uint32_t refcount;
    uint32_t type;       /* CsscTypeTag */
    uint32_t capacity;   /* allocated capacity (elements for vector/map, bytes for string) */
    uint32_t length;     /* used length */
} CsscHeapHeader;

/* String internals: header + char data[] (null-terminated) */
typedef struct {
    CsscHeapHeader header;
    char data[];         /* flexible array member */
} CsscString;

/* Vector internals: header + CsscVal items[] */
typedef struct {
    CsscHeapHeader header;
    CsscVal* items;      /* heap-allocated array of CsscVal */
} CsscVector;

/* Map entry: key-value pair */
typedef struct {
    char*    key;        /* heap-allocated key string */
    CsscVal  value;
    uint32_t hash;       /* cached key hash */
    bool     occupied;
} CsscMapEntry;

/* Map internals: open-addressing hash map */
typedef struct {
    CsscHeapHeader header;
    CsscMapEntry* buckets;
    uint32_t bucket_count;
} CsscMap;

/* Bind internals: array of (CsscVal, CsscVal) pairs with semicolon separation */
typedef struct {
    CsscHeapHeader header;
    CsscVal* pairs;     /* flat array: [key0, val0, key1, val1, ...] */
} CsscBind;

/* Reference counting */
CSSC_API void cssc_retain(CsscVal v);
CSSC_API void cssc_release(CsscVal v);
CSSC_API CsscVal cssc_copy(CsscVal v);              /* deep copy */

/* v6 mirror live-ref helper — dereferences an i64-shaped slot address.
 * `addr == 0` means "no live ref" and returns 0 (the snapshot path
 * handles the actual snapshot value separately). */
CSSC_API uint64_t cssc_load_i64_at(uint64_t addr);

/* =========================================================================
 * 5. STRING OPERATIONS
 * ========================================================================= */

CSSC_API CsscVal cssc_string_concat(CsscVal a, CsscVal b);
CSSC_API CsscVal cssc_string_repeat(CsscVal s, int64_t n);
CSSC_API CsscVal cssc_string_substr(CsscVal s, int64_t start, int64_t length);
CSSC_API CsscVal cssc_string_upper(CsscVal s);
CSSC_API CsscVal cssc_string_lower(CsscVal s);
CSSC_API CsscVal cssc_string_trim(CsscVal s);
CSSC_API CsscVal cssc_string_replace(CsscVal s, CsscVal old_str, CsscVal new_str);
CSSC_API CsscVal cssc_string_split(CsscVal s, CsscVal separator);
CSSC_API int64_t cssc_string_indexof(CsscVal s, CsscVal sub);
CSSC_API bool    cssc_string_contains(CsscVal s, CsscVal sub);
CSSC_API bool    cssc_string_startswith(CsscVal s, CsscVal prefix);
CSSC_API bool    cssc_string_endswith(CsscVal s, CsscVal suffix);
CSSC_API CsscVal cssc_string_char_at(CsscVal s, int64_t index);
CSSC_API CsscVal cssc_string_set_char(CsscVal s, int64_t index, CsscVal ch);
CSSC_API CsscVal cssc_string_reverse(CsscVal s);
CSSC_API bool    cssc_string_isdigit(CsscVal s);
CSSC_API bool    cssc_string_isalpha(CsscVal s);

/* =========================================================================
 * 6. VECTOR OPERATIONS
 * ========================================================================= */

CSSC_API void     cssc_vector_push(CsscVal vec, CsscVal item);
CSSC_API CsscVal  cssc_vector_pop(CsscVal vec);
CSSC_API CsscVal  cssc_vector_get(CsscVal vec, int64_t index);
CSSC_API void     cssc_vector_set(CsscVal vec, int64_t index, CsscVal item);
CSSC_API int64_t  cssc_vector_size(CsscVal vec);
CSSC_API void     cssc_vector_clear(CsscVal vec);
CSSC_API void     cssc_vector_erase(CsscVal vec, int64_t index);
CSSC_API void     cssc_vector_insert(CsscVal vec, int64_t index, CsscVal item);
CSSC_API CsscVal  cssc_vector_first(CsscVal vec);
CSSC_API CsscVal  cssc_vector_last(CsscVal vec);
CSSC_API CsscVal  cssc_vector_slice(CsscVal vec, int64_t start, int64_t end);
CSSC_API CsscVal  cssc_vector_sort(CsscVal vec);
CSSC_API CsscVal  cssc_vector_reverse(CsscVal vec);
CSSC_API bool     cssc_vector_contains(CsscVal vec, CsscVal item);
CSSC_API int64_t  cssc_vector_indexof(CsscVal vec, CsscVal item);

/* =========================================================================
 * 7. MAP OPERATIONS
 * ========================================================================= */

CSSC_API void     cssc_map_set(CsscVal map, const char* key, CsscVal value);
CSSC_API CsscVal  cssc_map_get(CsscVal map, const char* key);
CSSC_API bool     cssc_map_has(CsscVal map, const char* key);
CSSC_API void     cssc_map_remove(CsscVal map, const char* key);
CSSC_API int64_t  cssc_map_size(CsscVal map);
CSSC_API CsscVal  cssc_map_keys(CsscVal map);
CSSC_API CsscVal  cssc_map_values(CsscVal map);
CSSC_API void     cssc_map_clear(CsscVal map);

/* =========================================================================
 * 8. BIND OPERATIONS
 * ========================================================================= */

CSSC_API void     cssc_bind_add(CsscVal bind, CsscVal key, CsscVal value);
CSSC_API CsscVal  cssc_bind_get_key(CsscVal bind, int64_t pair_index);
CSSC_API CsscVal  cssc_bind_get_value(CsscVal bind, int64_t pair_index);
CSSC_API int64_t  cssc_bind_size(CsscVal bind);
/* Append every entry of a map (or another bind) to the end of `bind`.
 * A bind is conceptually a chain of single-entry maps; addmap extends the
 * chain by one map's worth of pairs in a single call. */
CSSC_API void     cssc_bind_addmap(CsscVal bind, CsscVal map);

/* =========================================================================
 * 8b. #DELMEMBER — soft wipes that keep size/cap intact
 *
 * `#delmember[v]`     -> cssc_delmember_all(v)
 * `#delmember[v[i]]`  -> cssc_delmember_at(v, i)
 *
 * Polymorphic helpers — dispatch on CSSC_TYPE(target):
 *   VECTOR  -> set v[i] = null (or zero for POD vectors); _all zeros
 *              every element. Size stays.
 *   MAP     -> release key/value at slot i; _all wipes every entry.
 *              Map count stays so iteration order is preserved.
 *   BIND    -> release key+value strings at pair i; _all wipes every
 *              pair. Bind size stays.
 *   OBJECT  -> release every heap member; instance handle stays alive.
 *   STRING  -> replace contents with empty string; the heap struct
 *              survives so any aliasing slot still has a valid ptr.
 *   POD     -> no-op (no heap content to release).
 *
 * The container itself is NEVER freed — that's what `#delete[v]` /
 * `#free[v]` do. `#delmember` is the in-place reset.
 * ========================================================================= */
CSSC_API void     cssc_delmember_all(CsscVal target);
CSSC_API void     cssc_delmember_at(CsscVal target, CsscVal index);

/* =========================================================================
 * 9. ARITHMETIC & COMPARISON
 * ========================================================================= */

CSSC_API CsscVal cssc_add(CsscVal a, CsscVal b);
CSSC_API CsscVal cssc_sub(CsscVal a, CsscVal b);
CSSC_API CsscVal cssc_mul(CsscVal a, CsscVal b);
CSSC_API CsscVal cssc_div(CsscVal a, CsscVal b);
CSSC_API CsscVal cssc_mod(CsscVal a, CsscVal b);
CSSC_API CsscVal cssc_neg(CsscVal a);

CSSC_API bool cssc_eq(CsscVal a, CsscVal b);
CSSC_API bool cssc_ne(CsscVal a, CsscVal b);
CSSC_API bool cssc_lt(CsscVal a, CsscVal b);
CSSC_API bool cssc_gt(CsscVal a, CsscVal b);
CSSC_API bool cssc_le(CsscVal a, CsscVal b);
CSSC_API bool cssc_ge(CsscVal a, CsscVal b);

CSSC_API bool cssc_logical_and(CsscVal a, CsscVal b);
CSSC_API bool cssc_logical_or(CsscVal a, CsscVal b);
CSSC_API bool cssc_logical_not(CsscVal a);

/* =========================================================================
 * 10. TYPE COERCION
 * ========================================================================= */

CSSC_API CsscVal cssc_coerce(CsscVal v, CsscTypeTag target_type);
CSSC_API CsscVal cssc_to_string_val(CsscVal v);     /* any → string */
CSSC_API CsscVal cssc_to_int_val(CsscVal v);         /* any → int */
CSSC_API CsscVal cssc_to_float_val(CsscVal v);       /* any → float */

/* =========================================================================
 * 11. SCOPE STACK — Variable management for sectors/objects/#define
 * =========================================================================
 *
 * The scope stack manages nested variable scopes. Each scope frame is a
 * hash map from variable name (const char*) to CsscVal. Entering a
 * sector or object pushes a new frame; exiting pops it.
 *
 * Variable lookup walks the stack from top to bottom (inner → outer).
 * Assignment writes to the topmost frame that contains the variable,
 * or the current frame if the variable is new.
 */

/* Hash map entry for scope frame */
typedef struct {
    const char* name;    /* variable name (interned string, not freed) */
    CsscVal     value;
    uint32_t    hash;
    bool        occupied;
} CsscScopeEntry;

/* Single scope frame */
typedef struct {
    CsscScopeEntry* entries;
    uint32_t capacity;
    uint32_t count;
    uint32_t alloc_bits;   /* max bits for #stack variables in this frame */
    uint8_t  is_private;   /* 1 = private scope barrier — lookups stop here */
    uint8_t  is_borrowed;  /* 1 = entries owned by another frame (no destroy on pop) */
    uint8_t  _pad[2];
    /* Frame-scoped allocation arena for transient strings/vectors built
     * during expression evaluation. Two-tier:
     *   1. `slab` is a fixed-size bump buffer allocated once on first use
     *      of this slot. Small allocations bump-allocate from it — no
     *      malloc per call. `slab_used` is reset to 0 on scope_pop;
     *      slab buffer itself lives until runtime shutdown.
     *   2. Allocations that don't fit the slab fall back to malloc and
     *      get linked onto `arena_head`. cssc_scope_pop walks and frees
     *      this list.
     * The slab eliminates ~95% of arena malloc/free traffic on tight
     * label loops, the single biggest source of umm_malloc fragmentation
     * on ESP8266 (~30s heap-death before this fix). */
    void*    arena_head;
    uint8_t* slab;
    uint32_t slab_used;
    uint32_t slab_size;
} CsscScopeFrame;

/* Free every block linked to this frame's arena. Called from
 * cssc_scope_pop; rarely useful directly. CsscScopeStack-taking
 * cssc_frame_arena_alloc is declared further below — after the
 * full CsscScopeStack typedef — so this header stays valid C without
 * forward-declaration tricks. */
CSSC_API void    cssc_frame_arena_reset(CsscScopeFrame* frame);

/* Maximum simultaneous pending arg-links (v6 ref-by-default). 8 is
 * comfortable for typical CSSC signatures (1-3 args); the runtime
 * silently drops links past this cap with a debug log — over-cap
 * means the caller invoked a function with >8 ref args, which is
 * either a parser bug or a user pattern we should support by
 * raising the cap. */
#ifndef CSSC_MAX_PENDING_LINKS
#define CSSC_MAX_PENDING_LINKS 8
#endif

/* One entry in the per-stack pending-arg-link table. Populated by
 * `cssc_arg_link` at the CALL SITE (caller's frame is still top);
 * consumed by `cssc_scope_push` at callee entry (after the new
 * frame is on top, before any user code runs). `param_name` is the
 * callee's `#scanp` slot name; `caller_slot_name` is the caller-
 * side slot the AST identifier referred to. Both are interned. */
typedef struct {
    const char* param_name;
    const char* caller_slot_name;
} CsscPendingLink;

/* The scope stack */
typedef struct {
    CsscScopeFrame* frames;
    uint32_t depth;        /* current depth (0 = global) */
    uint32_t max_depth;    /* allocated frame slots */
    /* v6 ref-by-default pending-arg-link queue. Filled by
     * `cssc_arg_link` at call sites; consumed by `cssc_scope_push`
     * into the new top frame as alias entries. Reset on push. */
    CsscPendingLink pending_links[CSSC_MAX_PENDING_LINKS];
    uint32_t pending_link_count;
} CsscScopeStack;

/* Frame-arena: alloc heap-backed memory linked to the top frame's
 * arena. Caller treats the returned pointer as a regular heap block —
 * the only difference is it gets freed automatically on scope_pop.
 * Returns NULL on OOM. The first 8 bytes of the actual allocation
 * are reserved for the next-pointer and are NOT exposed to the caller. */
CSSC_API void*   cssc_frame_arena_alloc(CsscScopeStack* stack, size_t size);

CSSC_API void     cssc_scope_init(CsscScopeStack* stack);
CSSC_API void     cssc_scope_destroy(CsscScopeStack* stack);
CSSC_API void     cssc_scope_push(CsscScopeStack* stack);
CSSC_API void     cssc_scope_push_private(CsscScopeStack* stack);
CSSC_API void     cssc_scope_pop(CsscScopeStack* stack);
/* Copy a named binding across a private-scope barrier — backs CSSC's
 * `#req[outer] local;` when emitted inside a private_scope. */
CSSC_API void     cssc_scope_req(CsscScopeStack* stack,
                                  const char* outer_name, const char* local_name);
/* Register `alias_name` as a true reference to `target_name`. Reads from
 * `alias_name` return target's value; writes to `alias_name` mutate
 * target's slot. Backs `#req[X] *Y;` so `Y += 1` actually mutates X.
 * Same scope-walk rules as scope_get — target must be findable upward
 * from `stack`'s current top. */
CSSC_API void     cssc_scope_alias(CsscScopeStack* stack,
                                    const char* alias_name, const char* target_name);

/* Walk every entry in every frame of the scope stack, newest frame
 * first (frame_idx counts down from stack->depth). The callback
 * receives the entry name, its CsscVal, the frame index it lives in,
 * and whether that frame is private. Return `false` from the callback
 * to stop iteration early; `true` to continue.
 *
 * This is the public, ABI-stable surface for tools like the
 * allocation watcher and external debuggers that need to enumerate
 * live variables — implementations are not allowed to break this
 * contract even if the underlying scope-frame struct layout changes. */
typedef bool (*CsscScopeWalkFn)(const char* name, CsscVal value,
                                int32_t frame_idx, bool is_private,
                                void* userdata);
CSSC_API void     cssc_scope_walk(CsscScopeStack* stack,
                                   CsscScopeWalkFn fn, void* userdata);
CSSC_API CsscVal  cssc_scope_get(CsscScopeStack* stack, const char* name);
CSSC_API void     cssc_scope_set(CsscScopeStack* stack, const char* name, CsscVal value);
CSSC_API bool     cssc_scope_has(CsscScopeStack* stack, const char* name);
CSSC_API void     cssc_scope_delete(CsscScopeStack* stack, const char* name);
/* v6 ref-by-default cross-frame delete (spec §2.5). When `name` is an
 * alias entry in the current top frame (registered by `cssc_arg_link`
 * + `cssc_scope_push`), this also deletes the underlying source slot
 * in the parent frame. Otherwise behaves identically to
 * `cssc_scope_delete`. Used by `#delete[param]` inside a `#define`
 * body so the caller's slot gets nulled when the callee deletes its
 * ref-aliased param. */
CSSC_API void     cssc_scope_delete_aliased(CsscScopeStack* stack,
                                              const char* name);
/* v6 ref-by-default arg-link recorder. Called at the CALL SITE
 * (caller's frame is top) just before the callable's prologue runs.
 * Records `(param_name, caller_slot_name)` in the stack's pending
 * queue. The very next `cssc_scope_push` consumes the queue and
 * installs each pair as an alias entry in the new top frame.
 * Silently drops over-cap entries (see CSSC_MAX_PENDING_LINKS). */
CSSC_API void     cssc_arg_link(CsscScopeStack* stack,
                                 const char* param_name,
                                 const char* caller_slot_name);
CSSC_API CsscVal* cssc_scope_get_ptr(CsscScopeStack* stack, const char* name);

/* Push an externally-owned frame as the new top-of-stack (borrowed: the frame's
 * entry-array stays owned by the caller; cssc_scope_pop will not destroy it). */
CSSC_API void     cssc_scope_push_borrowed(CsscScopeStack* stack, CsscScopeFrame* frame);

/* =========================================================================
 * 11b. HEX-KEYED FIRST-CLASS STORAGE
 *      `#stack[int, 32] 0x0AA = 1;` and `0xAEFF { … }` blocks store under
 *      stable uint64 keys, bypassing string-name lookups entirely.
 * ========================================================================= */

/* Hex-keyed scalar variable. Lives globally until cssc_hex_var_delete. */
CSSC_API void     cssc_hex_var_set(uint64_t hex_id, CsscVal value);
CSSC_API CsscVal  cssc_hex_var_get(uint64_t hex_id);
CSSC_API bool     cssc_hex_var_has(uint64_t hex_id);
CSSC_API void     cssc_hex_var_delete(uint64_t hex_id);

/* Hex-addressed persistent scope. The init function populates the frame
 * with members (vars + #define-emitted CsscVals); subsequent calls
 * cssc_hex_scope_get to retrieve the frame and dispatch into it. */
typedef void (*CsscHexScopeInitFn)(CsscScopeFrame* frame);
CSSC_API void              cssc_hex_scope_define(uint64_t hex_id, CsscHexScopeInitFn init_fn);
CSSC_API CsscScopeFrame*   cssc_hex_scope_get(uint64_t hex_id);
CSSC_API void              cssc_hex_scope_free(uint64_t hex_id);

/* =========================================================================
 * 11d. CATCH / DEBUG — try-call infrastructure for `#catch` + `#debug`
 * ========================================================================= */
#include <setjmp.h>
CSSC_API jmp_buf*   cssc_catch_push(void);
CSSC_API void       cssc_catch_pop(void);
CSSC_API const char* cssc_catch_last_error(void);
CSSC_API void       cssc_set_debug(int enabled);
extern CSSC_API int g_cssc_debug_enabled;

/* =========================================================================
 * 11c. OPENAI — per-instance OpenAIClient over WinHTTP
 *      Backs `ai::OpenAIClient(key) MyAI; MyAI.chat(prompt);` in CSSC.
 * ========================================================================= */
CSSC_API CsscVal cssc_openai_client_create(CsscVal api_key);
CSSC_API CsscVal cssc_openai_client_chat(CsscVal client, CsscVal prompt);
CSSC_API CsscVal cssc_openai_client_completion(CsscVal client, CsscVal messages_vec);
CSSC_API CsscVal cssc_openai_client_set_model(CsscVal client, CsscVal model);
CSSC_API CsscVal cssc_openai_client_set_system(CsscVal client, CsscVal text);
CSSC_API CsscVal cssc_openai_client_set_base_url(CsscVal client, CsscVal url);
CSSC_API CsscVal cssc_openai_client_set_timeout(CsscVal client, CsscVal secs);
CSSC_API CsscVal cssc_openai_client_model(CsscVal client);
CSSC_API CsscVal cssc_openai_client_has_key(CsscVal client);

/* =========================================================================
 * 12. SECTOR & OBJECT STRUCTURES
 * ========================================================================= */

/* Sector: namespace with public/private access control */
typedef struct {
    CsscHeapHeader header;
    CsscScopeFrame members;        /* all member variables */
    uint64_t* public_mask;          /* bitfield: which members are public */
    const char* name;
    bool freed;
} CsscSector;

/* Object label: function pointer + param metadata */
typedef struct {
    const char* name;
    void* body_fn;                  /* compiled function pointer */
    uint32_t param_count;           /* number of required params */
    uint32_t overload_index;        /* index in overload table */
} CsscLabel;

/* Object: deferred code block with labels and vtable */
typedef struct {
    CsscHeapHeader header;
    CsscScopeFrame members;        /* Object->member storage */
    CsscLabel* labels;              /* array of labels (vtable) */
    uint32_t label_count;
    void* top_level_fn;             /* compiled top-level code */
    void* free_fn;                  /* compiled free {} block */
    const char* name;
    bool executed;
    bool freed;
    bool access_enabled;            /* private:/public: enforcement */
    uint64_t* public_label_mask;
    uint64_t* private_label_mask;
} CsscObject;

/* Function: callable with body pointer */
typedef struct {
    CsscHeapHeader header;
    void* body_fn;                  /* compiled function body */
    const char* name;
    CsscVal last_return;
    uint32_t call_count;
    uint32_t max_bits;              /* capacity of holding variable */
} CsscFunction;

CSSC_API CsscVal  cssc_sector_create(const char* name);
CSSC_API CsscVal  cssc_sector_get(CsscVal sector, const char* member);
CSSC_API void     cssc_sector_set(CsscVal sector, const char* member, CsscVal value);
CSSC_API bool     cssc_sector_is_public(CsscVal sector, const char* member);
CSSC_API void     cssc_sector_free(CsscVal sector);
CSSC_API void     cssc_sector_push_members(CsscScopeStack* stack, CsscVal sector);

CSSC_API CsscVal  cssc_object_create(const char* name, CsscLabel* labels, uint32_t label_count,
                                      void* top_level_fn, void* free_fn);
CSSC_API CsscVal  cssc_object_execute(CsscVal obj, CsscVal* args, uint32_t nargs);
CSSC_API CsscVal  cssc_object_call_label(CsscVal obj, const char* label_name,
                                          CsscVal* args, uint32_t nargs);
CSSC_API void     cssc_object_free(CsscVal obj);

CSSC_API CsscVal  cssc_function_create(const char* name, void* body_fn, uint32_t max_bits);
CSSC_API CsscVal  cssc_function_call(CsscVal func, CsscVal* args, uint32_t nargs);

/* =========================================================================
 * 13. BUILTIN FUNCTIONS (cssc:: namespace)
 * ========================================================================= */

/* I/O */
CSSC_API void     cssc_builtin_out(CsscVal v);
CSSC_API void     cssc_builtin_outln(CsscVal v);
CSSC_API CsscVal  cssc_builtin_input(CsscVal prompt);
CSSC_API void     cssc_builtin_sleep(double seconds);

/* Type */
CSSC_API CsscVal  cssc_builtin_typeof(CsscVal v);
CSSC_API CsscVal  cssc_builtin_to_int(CsscVal v);
CSSC_API CsscVal  cssc_builtin_to_float(CsscVal v);
CSSC_API CsscVal  cssc_builtin_to_string(CsscVal v);
CSSC_API CsscVal  cssc_builtin_to_bool(CsscVal v);
CSSC_API CsscVal  cssc_builtin_is_null(CsscVal v);
CSSC_API CsscVal  cssc_builtin_is_int(CsscVal v);
CSSC_API CsscVal  cssc_builtin_is_float(CsscVal v);
CSSC_API CsscVal  cssc_builtin_is_string(CsscVal v);
CSSC_API CsscVal  cssc_builtin_is_array(CsscVal v);

/* Math */
CSSC_API CsscVal  cssc_builtin_abs(CsscVal v);
CSSC_API CsscVal  cssc_builtin_min(CsscVal a, CsscVal b);
CSSC_API CsscVal  cssc_builtin_max(CsscVal a, CsscVal b);
CSSC_API CsscVal  cssc_builtin_sqrt(CsscVal v);
CSSC_API CsscVal  cssc_builtin_pow(CsscVal base, CsscVal exp);
CSSC_API CsscVal  cssc_builtin_floor(CsscVal v);
CSSC_API CsscVal  cssc_builtin_ceil(CsscVal v);
CSSC_API CsscVal  cssc_builtin_round(CsscVal v);
CSSC_API CsscVal  cssc_builtin_random(void);
CSSC_API CsscVal  cssc_builtin_random_int(CsscVal a, CsscVal b);
CSSC_API CsscVal  cssc_builtin_clamp(CsscVal v, CsscVal lo, CsscVal hi);

/* Array/Collection */
CSSC_API CsscVal  cssc_builtin_len(CsscVal v);
CSSC_API CsscVal  cssc_builtin_push(CsscVal arr, CsscVal item);
CSSC_API CsscVal  cssc_builtin_pop(CsscVal arr);
CSSC_API CsscVal  cssc_builtin_sort(CsscVal arr);
CSSC_API CsscVal  cssc_builtin_range(CsscVal start, CsscVal end, CsscVal step);

/* String */
CSSC_API CsscVal  cssc_builtin_strlen_val(CsscVal s);
CSSC_API CsscVal  cssc_builtin_substr(CsscVal s, CsscVal start, CsscVal len);
CSSC_API CsscVal  cssc_builtin_replace(CsscVal s, CsscVal old_s, CsscVal new_s);
CSSC_API CsscVal  cssc_builtin_split(CsscVal s, CsscVal sep);
CSSC_API CsscVal  cssc_builtin_join(CsscVal arr, CsscVal sep);
CSSC_API CsscVal  cssc_builtin_trim(CsscVal s);
CSSC_API CsscVal  cssc_builtin_upper(CsscVal s);
CSSC_API CsscVal  cssc_builtin_lower(CsscVal s);
CSSC_API CsscVal  cssc_builtin_contains(CsscVal s, CsscVal sub);

/* Time
 * ─────
 *   cssc_builtin_uptime    — monotonic seconds since program start.
 *                            Always live, never depends on RTC/NTP.
 *   cssc_builtin_time/...  — wall-clock variants. On embedded these
 *                            return epoch-zero values until
 *                            cssc::ntp_sync() succeeds. */
CSSC_API CsscVal  cssc_builtin_uptime(void);
/* Hard reset — embedded targets call ESP.restart(), desktop exit(0).
 * Returns cssc_null but never actually returns to the caller. */
CSSC_API CsscVal  cssc_builtin_reboot(void);
CSSC_API CsscVal  cssc_builtin_time(void);
CSSC_API CsscVal  cssc_builtin_timestamp(void);
CSSC_API CsscVal  cssc_builtin_date(void);
CSSC_API CsscVal  cssc_builtin_datetime(void);
CSSC_API CsscVal  cssc_builtin_detime(void);
CSSC_API CsscVal  cssc_builtin_sdetime(void);

/* System */
CSSC_API CsscVal  cssc_builtin_env(CsscVal name);
CSSC_API CsscVal  cssc_builtin_cwd(void);
CSSC_API CsscVal  cssc_builtin_platform(void);
CSSC_API CsscVal  cssc_builtin_exec(CsscVal cmd);
CSSC_API void     cssc_builtin_exit(CsscVal code);

/* File I/O */
CSSC_API CsscVal  cssc_builtin_read_file(CsscVal path);
CSSC_API void     cssc_builtin_write_file(CsscVal path, CsscVal content);
CSSC_API CsscVal  cssc_builtin_file_exists(CsscVal path);
CSSC_API void     cssc_builtin_mkdir(CsscVal path);

/* =========================================================================
 * 13b. DIAGNOSTICS MARKERS
 * =========================================================================
 * Emit lines to stdout / Serial that the `cssc diagnostics --port COMx`
 * chip listener parses into MEMORY / CPU / GENERAL tabs:
 *
 *   [cssc-mem] heap=<bytes-free>  min=<lowest-since-boot>
 *   [cssc-cpu] <fn-name> <ms>ms
 *
 * - cssc_diag_emit_mem() pulls free-heap from the platform (ESP.getFreeHeap
 *   on Arduino-ESP, mallinfo on POSIX, GlobalMemoryStatus on Win32) and
 *   tracks the all-time minimum.
 * - cssc_diag_emit_cpu() is a thin print — caller measures the duration.
 * - cssc_diag_tick() is the auto-pulse fired from the Arduino-framework
 *   loop() once per second; emits mem snapshot when diagnostics enabled.
 * - cssc_diag_enable(1/0) toggles emission. Default on under -DCSSC_DIAG,
 *   off otherwise. */
CSSC_API void     cssc_diag_emit_mem(void);
CSSC_API void     cssc_diag_emit_cpu(const char* name, uint32_t ms);
CSSC_API void     cssc_diag_tick(void);
CSSC_API void     cssc_diag_enable(int on);
CSSC_API int      cssc_diag_is_enabled(void);

/* User-facing builtins routed through cssc::diag_mem() / cssc::diag_cpu()
 * / cssc::diag_enable(). All return CsscVal — null for the void-effect
 * variants — so the codegen dispatcher can stuff them into the same
 * "expression result" slot as any other ns_call. */
CSSC_API CsscVal  cssc_builtin_diag_mem(void);
CSSC_API CsscVal  cssc_builtin_diag_cpu(CsscVal name, CsscVal ms);
CSSC_API CsscVal  cssc_builtin_diag_enable(CsscVal on);

/* =========================================================================
 * 14. ERROR HANDLING
 * ========================================================================= */

/* #panic — triggers a fatal runtime error with message */
CSSC_API void cssc_panic(const char* message);
CSSC_API void cssc_panicf(const char* fmt, ...);

/* Runtime error with source location */
CSSC_API void cssc_error(const char* operation, const char* detail, int line);

/* =========================================================================
 * 15. ASMH HOTLOADING BRIDGE
 * =========================================================================
 * These functions are used by the main .exe to load and call into the
 * hotload .dll at runtime. The DLL exports named functions that the
 * compiled code calls via function pointers resolved at startup.
 */

typedef CsscVal (*CsscHotloadFn)(CsscScopeStack* scope, CsscVal* args, uint32_t nargs);

typedef struct {
    void*   dll_handle;           /* LoadLibrary handle */
    CsscHotloadFn* functions;     /* resolved function pointers */
    const char**   func_names;    /* function name table */
    uint32_t       func_count;
} CsscHotloadContext;

CSSC_API bool cssc_hotload_init(CsscHotloadContext* ctx, const char* dll_path);
CSSC_API CsscHotloadFn cssc_hotload_resolve(CsscHotloadContext* ctx, const char* name);
CSSC_API void cssc_hotload_shutdown(CsscHotloadContext* ctx);

/* =========================================================================
 * 15b. MODULE BUILTINS — peek, etc.
 * ========================================================================= */

/* peek module: look-ahead into collections */
CSSC_API CsscVal  cssc_peek_peek(CsscVal collection, int64_t index, int64_t amount);
CSSC_API CsscVal  cssc_peek_cpeek(CsscVal collection, int64_t index, int64_t count);
CSSC_API CsscVal  cssc_peek_peek_safe(CsscVal collection, int64_t index, CsscVal fallback);
CSSC_API bool     cssc_peek_has_next(CsscVal collection, int64_t index);

/* =========================================================================
 * 15c. MODULE BUILTINS — paths, binary, io, env, math, etc.
 * ========================================================================= */

/* cssc.paths module */
CSSC_API CsscVal  cssc_paths_exists(CsscVal path);
CSSC_API CsscVal  cssc_paths_mkdir(CsscVal path);
CSSC_API CsscVal  cssc_paths_dirname(CsscVal path);
CSSC_API CsscVal  cssc_paths_basename(CsscVal path);
CSSC_API CsscVal  cssc_paths_join(CsscVal a, CsscVal b);
CSSC_API CsscVal  cssc_paths_ext(CsscVal path);
CSSC_API CsscVal  cssc_paths_resolve(CsscVal path);

/* cssc.binary module */
CSSC_API CsscVal  cssc_binary_read(CsscVal path);
CSSC_API void     cssc_binary_write(CsscVal path, CsscVal data);
CSSC_API CsscVal  cssc_binary_tobinary(CsscVal value);
CSSC_API CsscVal  cssc_binary_frombinary(CsscVal data);

/* cssc.io module (stdio) */
CSSC_API CsscVal  cssc_io_read_file(CsscVal path);
CSSC_API void     cssc_io_write_file(CsscVal path, CsscVal content);
CSSC_API CsscVal  cssc_io_exists(CsscVal path);
CSSC_API void     cssc_io_remove(CsscVal path);
CSSC_API void     cssc_io_copy(CsscVal src, CsscVal dst);
CSSC_API CsscVal  cssc_io_list_dir(CsscVal path);

/* Deferred sector support — zero-RAM until #reserve */
typedef struct {
    const char* label;           /* ?label name */
    void* init_fn;               /* compiled sector init function */
    bool initialized;            /* false until #reserve[label] */
    CsscVal sector_val;          /* null until initialized */
} CsscDeferredSector;

CSSC_API void     cssc_deferred_register(CsscScopeStack* scope, const char* label, void* init_fn);
CSSC_API CsscVal  cssc_deferred_reserve(CsscScopeStack* scope, const char* label);

/* #load companion DLL support */
typedef struct {
    void*       dll_handle;      /* LoadLibrary handle */
    const char* path;            /* DLL file path */
    bool        started;         /* true after global scope executed */
    CsscVal     module_val;      /* the module-as-sector value */
    void*       init_fn;         /* DLL exported init function */
} CsscLoadedDll;

CSSC_API CsscVal  cssc_load_dll(CsscScopeStack* scope, const char* dll_path, const char* alias);
CSSC_API void     cssc_unload_dll(CsscVal module);

/* =========================================================================
 * 16. MEMORY MANAGEMENT
 * ========================================================================= */

/* Allocator interface — all CSSC heap allocations go through this */
CSSC_API void* cssc_alloc(size_t size);
CSSC_API void* cssc_realloc(void* ptr, size_t new_size);
CSSC_API void  cssc_free(void* ptr);

/* String interning — frequently used strings are interned for fast comparison */
CSSC_API const char* cssc_intern(const char* str);
CSSC_API bool cssc_intern_eq(const char* a, const char* b);  /* pointer comparison for interned */

/* =========================================================================
 * 17. FORMAT / CONVERSION UTILITIES
 * ========================================================================= */

/* Format a CsscVal to a human-readable string (for cssc::out, error messages) */
CSSC_API CsscVal cssc_format_value(CsscVal v);

/* Hash function for map keys */
CSSC_API uint32_t cssc_hash_string(const char* str);

/* =========================================================================
 * 18a. GRAPHICS — Video window, framebuffer, matrix (Windows GDI-backed)
 * =========================================================================
 *
 * CSSC video model:
 *   #video[w, h, fps] name       — creates a window (HWND) on Windows
 *   #framebuffer[w, h] name      — pixel buffer (RGBA32)
 *   #matrix[w, h] name           — 2D array of pixels, also RGBA32
 *
 * Rendering pipeline:
 *   matrix -> framebuffer -> video window (present)
 */

typedef struct {
    CsscHeapHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;   /* ARGB32, size = width * height */
} CsscFramebuffer;

typedef struct {
    CsscHeapHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;   /* ARGB32 */
} CsscMatrix;

/* Forward declarations for platform types (opaque pointers) */
typedef struct CsscVideoImpl CsscVideoImpl;

typedef struct {
    CsscHeapHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    CsscVideoImpl* impl;   /* platform-specific state: HWND, HDC, thread */
} CsscVideo;

/* Constructors */
CSSC_API CsscVal cssc_video_create(int64_t width, int64_t height, int64_t fps);
CSSC_API CsscVal cssc_framebuffer_create(int64_t width, int64_t height);
CSSC_API CsscVal cssc_matrix_create(int64_t width, int64_t height);

/* Free */
CSSC_API void cssc_video_free(CsscVal v);
CSSC_API void cssc_framebuffer_free(CsscVal v);
CSSC_API void cssc_matrix_free(CsscVal v);

/* Matrix operations */
CSSC_API CsscVal cssc_matrix_fill(CsscVal matrix, CsscVal color);   /* fills in place, returns same */
CSSC_API void cssc_matrix_set_pixel(CsscVal matrix, int64_t x, int64_t y, uint32_t color);
CSSC_API uint32_t cssc_matrix_get_pixel(CsscVal matrix, int64_t x, int64_t y);
CSSC_API void cssc_matrix_copy_to_framebuffer(CsscVal matrix, CsscVal fb);

/* Framebuffer operations (same as matrix, different type) */
CSSC_API void cssc_framebuffer_clear(CsscVal fb, uint32_t color);
CSSC_API void cssc_framebuffer_set_pixel(CsscVal fb, int64_t x, int64_t y, uint32_t color);

/* Video operations */
CSSC_API void cssc_video_clear(CsscVal video, uint32_t color);
CSSC_API void cssc_video_set_matrix(CsscVal video, CsscVal fb_or_matrix);
CSSC_API void cssc_video_present(CsscVal video);
CSSC_API bool cssc_video_is_open(CsscVal video);

/* Draw UTF-8 text directly into a video's backing DC.
 *   x, y         — top-left of the text, in video pixels
 *   size_px      — pixel height of the font cell
 *   weight       — 0=normal, 1=bold, 2=light, 3=thin
 *   color        — 0xRRGGBB
 *   font_family  — NULL or empty = Segoe UI */
CSSC_API void cssc_video_draw_text(CsscVal video, int64_t x, int64_t y,
                                   const char* text, int64_t size_px,
                                   int64_t weight, uint32_t color,
                                   const char* font_family);

/* Measure a text string with the given font parameters. Returns a small
 * CsscMap-like pair; for simplicity this helper just returns the width
 * in pixels (height = size_px is assumed by the caller). */
CSSC_API int64_t cssc_video_measure_text(const char* text, int64_t size_px,
                                         int64_t weight, const char* font_family);

/* Draw a filled rectangle into the video's backing DC. Useful for panels,
 * translucent overlays are done by alpha-blending the color before passing. */
CSSC_API void cssc_video_draw_rect(CsscVal video, int64_t x, int64_t y,
                                   int64_t w, int64_t h, uint32_t color);

/* Copy a matrix into a region of a video's backing DC (for background images,
 * sprites, icons). Clips to the video dimensions. */
CSSC_API void cssc_video_blit_matrix(CsscVal video, CsscVal matrix,
                                     int64_t dst_x, int64_t dst_y);

/* Load a 24- or 32-bit Windows BMP file into a CsscMatrix.
 * Returns cssc_null() on any failure. */
CSSC_API CsscVal cssc_image_load_bmp(const char* path);

/* =========================================================================
 * 18c. CONSOLE — native Win32 kernel console wrapper
 * =========================================================================
 *
 * Created via `#console[w, h] name;` after `#include('sys.console')`.
 * Uses AllocConsole / GetStdHandle — no Python shim, no new PowerShell.
 *
 * Lines longer than `w` are written verbatim — no clipping, no wrapping
 * performed by us; whatever the Win32 console does with them is what you get.
 */

typedef struct {
    CsscHeapHeader header;
    int     width;
    int     height;
    void*   hout;            /* HANDLE on Windows; opaque otherwise */
    void*   hin;
    uint16_t default_attr;   /* original attributes at create time */
    uint16_t current_attr;   /* attrs used by next out/cout */
    bool    allocated;       /* whether we called AllocConsole ourselves */
} CsscConsole;

CSSC_API CsscVal cssc_console_create(int64_t w, int64_t h);
CSSC_API void    cssc_console_free(CsscVal c);

CSSC_API void    cssc_console_out(CsscVal c, CsscVal text);
CSSC_API void    cssc_console_clear_all(CsscVal c);
CSSC_API void    cssc_console_clear_line(CsscVal c, int64_t line);
CSSC_API void    cssc_console_cursor_col(CsscVal c, CsscVal color);
CSSC_API void    cssc_console_cursor_set(CsscVal c, int64_t line, int64_t col);
CSSC_API void    cssc_console_cursor_pos(CsscVal c, CsscVal pos);     /* int or float */
CSSC_API CsscVal cssc_console_get_line(CsscVal c, int64_t line);
CSSC_API CsscVal cssc_console_cursor_get_line(CsscVal c);
/* Returns a 2-element vector [line, col] of the current cursor position,
 * or cssc_null() if the console is invalid. */
CSSC_API CsscVal cssc_console_cursor_at(CsscVal c);

/* Generic color-from-value helper (int or hex int expected) */
CSSC_API uint32_t cssc_color_from_val(CsscVal v);

/* =========================================================================
 * 18b. DAEMONS — Background threads for CSSC function execution
 * =========================================================================
 *
 * #daemon[func_var] name        — launches `func_var` in a new thread
 * #killdaemon[func_var]         — requests the thread to exit
 *
 * Daemon functions receive the global scope and run until they naturally
 * finish or the 'killed' flag is checked in their loop body.
 */

typedef struct CsscDaemonImpl CsscDaemonImpl;

CSSC_API CsscVal cssc_daemon_start(CsscScopeStack* scope, const char* func_name);
CSSC_API void    cssc_daemon_kill(const char* func_name);
CSSC_API bool    cssc_daemon_should_stop(const char* func_name);   /* daemon's own check */
CSSC_API void    cssc_daemon_shutdown_all(void);

/* =========================================================================
 * 19. GLOBAL STATE
 * ========================================================================= */

/* Initialize the runtime (called once at program start) */
CSSC_API void cssc_runtime_init(void);

/* Shutdown the runtime (called once at program end) */
CSSC_API void cssc_runtime_shutdown(void);

/* Global scope stack (used by generated code) */
CSSC_API CsscScopeStack* cssc_global_scope(void);

/* DLL/module support: when a module is loaded into a host exe, the module's
 * own g_scope is disconnected from the host's. Calling cssc_global_scope_set
 * at the start of cssc_module_init points the DLL's cssc_global_scope() at
 * the host's scope, so variable lookups and scope pushes work across the
 * DLL boundary. */
CSSC_API void cssc_global_scope_set(CsscScopeStack* external);

/* =========================================================================
 * 19a. ISOLATED OBJECT (.obj) SUPPORT — #depend directive backend
 * =========================================================================
 *
 * A .obj is an isolated CSSC program bundled into a single file. Unlike
 * modules (which share the host scope), .obj files are loaded with their
 * own runtime/scope. The host communicates with them only via the sector
 * returned by their init function.
 *
 * Binary format (produced by includecpp/.../cssc_obj_format.py):
 *   uint32 version (=1)
 *   uint32 name_len, bytes name
 *   uint32 num_entries
 *   per entry: uint32 nlen, bytes name, uint64 dlen, bytes data
 *
 * Entry "main.dll" is extracted to a temp file and loaded. The DLL's
 * cssc_module_init() is called with a fresh scope and its return value
 * (a sector) is published to the caller under the chosen alias.
 */

typedef struct CsscObjHandleImpl CsscObjHandleImpl;

typedef struct {
    CsscHeapHeader header;
    const char* alias;
    const char* project_name;
    CsscVal      module_sector;    /* the sector returned by the .obj's init */
    CsscObjHandleImpl* impl;
} CsscObjHandle;

/* Load a .obj from disk, extract main.dll, call its init.
 * Returns the module's sector (which is also registered under `alias` in
 * the calling host's scope). */
CSSC_API CsscVal cssc_obj_load(CsscScopeStack* host_scope, const char* obj_path, const char* alias);

/* Unload (releases extracted DLL, closes handles). */
CSSC_API void cssc_obj_unload(CsscVal handle_val);

/* =========================================================================
 * 19f. .cobj native-code library loader
 * =========================================================================
 *
 * A .cobj wraps a DLL that exposes `cssc_cobj_init(scope)` and
 * `cssc_cobj_cleanup(scope)`. The host calls init to populate its scope,
 * and the runtime registers cleanup to fire at runtime shutdown (reverse
 * order of load).
 */
CSSC_API CsscVal cssc_cobj_load(CsscScopeStack* host_scope,
                                const char* cobj_path, const char* alias);

/* =========================================================================
 * 19b. WATERMARK — CSSeries Engine intro animation
 * =========================================================================
 *
 * When the host instantiates a sector from a .obj-loaded dependency, the
 * runtime triggers a threaded 6-second watermark intro (once per process).
 * Caller code continues executing immediately; the watermark thread
 * displays the animation in parallel and closes itself when done or when
 * cssc_watermark_dismiss() is invoked.
 */
CSSC_API void cssc_watermark_show_once(void);
CSSC_API void cssc_watermark_dismiss(void);
CSSC_API bool cssc_watermark_is_showing(void);

/* =========================================================================
 * 19c. SOUND — simple WAV playback (Win32 PlaySound-based)
 * =========================================================================
 *
 * cssc_sound_play_file(path, async)       — play from disk
 * cssc_sound_play_memory(buf, size, sync) — play from in-memory WAV bytes
 * cssc_sound_stop()                       — stop all playback
 *
 * Designed for UI feedback and short cues (startup stingers, click sounds).
 * For long-form music use a streaming API (not provided here).
 */
CSSC_API bool cssc_sound_play_file(const char* path, bool async);
CSSC_API bool cssc_sound_play_memory(const void* data, uint32_t size, bool async);
CSSC_API void cssc_sound_stop(void);

/* =========================================================================
 * 19d. .obj ASSET READING — pull bytes from a bundled asset entry
 * =========================================================================
 *
 * CSSC modules can reach into their own (or any) .obj bundle to read
 * embedded asset bytes (audio, images, fonts). Returns a CsscVal string
 * whose backing holds the raw bytes, length = cssc_strlen(result).
 * Returns cssc_null() on failure.
 */
CSSC_API CsscVal cssc_obj_asset_read(const char* obj_path, const char* entry_name);

/* =========================================================================
 * 19e. CUSTOM ANIMATED INTRO — configurable Apple-style splash
 * =========================================================================
 *
 * Shows a single intro window with:
 *  - Animated reveal of `title` (letter-by-letter, ease-out)
 *  - Shine sweep during mid-phase
 *  - Small italic `subtitle` below
 *  - Fade in / hold / fade out, total = duration_ms
 *  - Optional WAV audio from memory (buf/size) played in parallel
 *
 * Non-blocking: returns immediately, intro runs on its own thread.
 * Idempotent once-per-process: subsequent calls are no-ops.
 */
CSSC_API void cssc_intro_play(const char* title,
                              const char* subtitle,
                              int32_t duration_ms,
                              const void* wav_data,
                              uint32_t wav_size);

#ifdef __cplusplus
}
#endif

#endif /* CSSC_RUNTIME_H */
