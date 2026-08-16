

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

#if defined(__GNUC__) || defined(__clang__)
    #define CSSC_UNUSED __attribute__((unused))
#else
    #define CSSC_UNUSED
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
    CSSC_TYPE_POINTER  = 11,
    CSSC_TYPE_MATRIX   = 12,
    CSSC_TYPE_ITERATOR = 13,
    CSSC_TYPE_MODULE   = 14,
    CSSC_TYPE_METHOD   = 15,
    CSSC_TYPE_BINDING  = 16,
    CSSC_TYPE_CONSOLE  = 17,
    CSSC_TYPE_MAX      = 18
} CsscTypeTag;

#define CSSC_FLAG_CONST      (1 << 8)
#define CSSC_FLAG_HEAP       (1 << 9)
#define CSSC_FLAG_STACK      (1 << 10)
#define CSSC_FLAG_AUTO       (1 << 11)
#define CSSC_FLAG_METHOD     (1 << 12)
#define CSSC_FLAG_FREED      (1 << 13)
#define CSSC_FLAG_SCOPE_ALIAS (1 << 14)

typedef struct {
    uint64_t tag;
    union {
        int64_t    i;
        double     f;
        bool       b;
        void*      ptr;
        uint64_t   raw;
    } data;
} CsscVal;

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

CSSC_API CsscVal cssc_null(void);
CSSC_API CsscVal cssc_int(int64_t value);
CSSC_API CsscVal cssc_float(double value);
CSSC_API CsscVal cssc_bool(bool value);
CSSC_API CsscVal cssc_string(const char* str);
CSSC_API CsscVal cssc_string_len(const char* str, size_t len);
CSSC_API CsscVal cssc_string_owned(char* str);
CSSC_API CsscVal cssc_vector(size_t initial_capacity);
CSSC_API CsscVal cssc_map(size_t initial_capacity);
CSSC_API CsscVal cssc_bind(void);

CSSC_API int64_t     cssc_to_int(CsscVal v);
CSSC_API double      cssc_to_float(CsscVal v);
CSSC_API bool        cssc_to_bool(CsscVal v);
CSSC_API const char* cssc_to_cstr(CsscVal v);
CSSC_API size_t      cssc_strlen(CsscVal v);
CSSC_API bool        cssc_is_truthy(CsscVal v);
CSSC_API const char* cssc_typeof_str(CsscVal v);

typedef struct {
    uint32_t refcount;
    uint32_t type;
    uint32_t capacity;
    uint32_t length;
} CsscHeapHeader;

typedef struct {
    CsscHeapHeader header;
    char data[];
} CsscString;

typedef struct {
    CsscHeapHeader header;
    CsscVal* items;
} CsscVector;

typedef struct {
    char*    key;
    CsscVal  value;
    uint32_t hash;
    bool     occupied;
} CsscMapEntry;

typedef struct {
    CsscHeapHeader header;
    CsscMapEntry* buckets;
    uint32_t bucket_count;
} CsscMap;

typedef struct {
    CsscHeapHeader header;
    CsscVal* pairs;
} CsscBind;

CSSC_API void cssc_retain(CsscVal v);
CSSC_API void cssc_release(CsscVal v);
CSSC_API CsscVal cssc_copy(CsscVal v);

CSSC_API uint64_t cssc_load_i64_at(uint64_t addr);

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

CSSC_API void     cssc_map_set(CsscVal map, const char* key, CsscVal value);
CSSC_API CsscVal  cssc_map_get(CsscVal map, const char* key);
CSSC_API bool     cssc_map_has(CsscVal map, const char* key);
CSSC_API void     cssc_map_remove(CsscVal map, const char* key);
CSSC_API int64_t  cssc_map_size(CsscVal map);
CSSC_API CsscVal  cssc_map_keys(CsscVal map);
CSSC_API CsscVal  cssc_map_values(CsscVal map);
CSSC_API void     cssc_map_clear(CsscVal map);

CSSC_API void     cssc_bind_add(CsscVal bind, CsscVal key, CsscVal value);
CSSC_API CsscVal  cssc_bind_get_key(CsscVal bind, int64_t pair_index);
CSSC_API CsscVal  cssc_bind_get_value(CsscVal bind, int64_t pair_index);
CSSC_API int64_t  cssc_bind_size(CsscVal bind);

CSSC_API void     cssc_bind_addmap(CsscVal bind, CsscVal map);

CSSC_API void     cssc_delmember_all(CsscVal target);
CSSC_API void     cssc_delmember_at(CsscVal target, CsscVal index);

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

CSSC_API CsscVal cssc_coerce(CsscVal v, CsscTypeTag target_type);
CSSC_API CsscVal cssc_to_string_val(CsscVal v);
CSSC_API CsscVal cssc_to_int_val(CsscVal v);
CSSC_API CsscVal cssc_to_float_val(CsscVal v);

typedef struct {
    const char* name;
    CsscVal     value;
    uint32_t    hash;
    bool        occupied;
} CsscScopeEntry;

typedef struct {
    CsscScopeEntry* entries;
    uint32_t capacity;
    uint32_t count;
    uint32_t alloc_bits;
    uint8_t  is_private;
    uint8_t  is_borrowed;
    uint8_t  _pad[2];

    void*    arena_head;
    uint8_t* slab;
    uint32_t slab_used;
    uint32_t slab_size;
} CsscScopeFrame;

CSSC_API void    cssc_frame_arena_reset(CsscScopeFrame* frame);

#ifndef CSSC_MAX_PENDING_LINKS
#define CSSC_MAX_PENDING_LINKS 8
#endif

typedef struct {
    const char* param_name;
    const char* caller_slot_name;
} CsscPendingLink;

typedef struct {
    CsscScopeFrame* frames;
    uint32_t depth;
    uint32_t max_depth;

    CsscPendingLink pending_links[CSSC_MAX_PENDING_LINKS];
    uint32_t pending_link_count;
} CsscScopeStack;

CSSC_API void*   cssc_frame_arena_alloc(CsscScopeStack* stack, size_t size);

CSSC_API void     cssc_scope_init(CsscScopeStack* stack);
CSSC_API void     cssc_scope_destroy(CsscScopeStack* stack);
CSSC_API void     cssc_scope_push(CsscScopeStack* stack);
CSSC_API void     cssc_scope_push_private(CsscScopeStack* stack);
CSSC_API void     cssc_scope_pop(CsscScopeStack* stack);

CSSC_API void     cssc_scope_req(CsscScopeStack* stack,
                                  const char* outer_name, const char* local_name);

CSSC_API void     cssc_scope_alias(CsscScopeStack* stack,
                                    const char* alias_name, const char* target_name);

typedef bool (*CsscScopeWalkFn)(const char* name, CsscVal value,
                                int32_t frame_idx, bool is_private,
                                void* userdata);
CSSC_API void     cssc_scope_walk(CsscScopeStack* stack,
                                   CsscScopeWalkFn fn, void* userdata);
CSSC_API CsscVal  cssc_scope_get(CsscScopeStack* stack, const char* name);
CSSC_API void     cssc_scope_set(CsscScopeStack* stack, const char* name, CsscVal value);
CSSC_API bool     cssc_scope_has(CsscScopeStack* stack, const char* name);
CSSC_API void     cssc_scope_delete(CsscScopeStack* stack, const char* name);

CSSC_API void     cssc_scope_delete_aliased(CsscScopeStack* stack,
                                              const char* name);

CSSC_API void     cssc_arg_link(CsscScopeStack* stack,
                                 const char* param_name,
                                 const char* caller_slot_name);
CSSC_API CsscVal* cssc_scope_get_ptr(CsscScopeStack* stack, const char* name);

CSSC_API void     cssc_scope_push_borrowed(CsscScopeStack* stack, CsscScopeFrame* frame);

CSSC_API void     cssc_hex_var_set(uint64_t hex_id, CsscVal value);
CSSC_API CsscVal  cssc_hex_var_get(uint64_t hex_id);
CSSC_API bool     cssc_hex_var_has(uint64_t hex_id);
CSSC_API void     cssc_hex_var_delete(uint64_t hex_id);

typedef void (*CsscHexScopeInitFn)(CsscScopeFrame* frame);
CSSC_API void              cssc_hex_scope_define(uint64_t hex_id, CsscHexScopeInitFn init_fn);
CSSC_API CsscScopeFrame*   cssc_hex_scope_get(uint64_t hex_id);
CSSC_API void              cssc_hex_scope_free(uint64_t hex_id);

#include <setjmp.h>
CSSC_API jmp_buf*   cssc_catch_push(void);
CSSC_API void       cssc_catch_pop(void);
CSSC_API const char* cssc_catch_last_error(void);
CSSC_API void       cssc_set_debug(int enabled);
extern CSSC_API int g_cssc_debug_enabled;

CSSC_API CsscVal cssc_openai_client_create(CsscVal api_key);
CSSC_API CsscVal cssc_openai_client_chat(CsscVal client, CsscVal prompt);
CSSC_API CsscVal cssc_openai_client_completion(CsscVal client, CsscVal messages_vec);
CSSC_API CsscVal cssc_openai_client_set_model(CsscVal client, CsscVal model);
CSSC_API CsscVal cssc_openai_client_set_system(CsscVal client, CsscVal text);
CSSC_API CsscVal cssc_openai_client_set_base_url(CsscVal client, CsscVal url);
CSSC_API CsscVal cssc_openai_client_set_timeout(CsscVal client, CsscVal secs);
CSSC_API CsscVal cssc_openai_client_model(CsscVal client);
CSSC_API CsscVal cssc_openai_client_has_key(CsscVal client);

typedef struct {
    CsscHeapHeader header;
    CsscScopeFrame members;
    uint64_t* public_mask;
    const char* name;
    bool freed;
} CsscSector;

typedef struct {
    const char* name;
    void* body_fn;
    uint32_t param_count;
    uint32_t overload_index;
} CsscLabel;

typedef struct {
    CsscHeapHeader header;
    CsscScopeFrame members;
    CsscLabel* labels;
    uint32_t label_count;
    void* top_level_fn;
    void* free_fn;
    const char* name;
    bool executed;
    bool freed;
    bool access_enabled;
    uint64_t* public_label_mask;
    uint64_t* private_label_mask;
} CsscObject;

typedef struct {
    CsscHeapHeader header;
    void* body_fn;
    const char* name;
    CsscVal last_return;
    uint32_t call_count;
    uint32_t max_bits;
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

CSSC_API void     cssc_builtin_out(CsscVal v);
CSSC_API void     cssc_builtin_outln(CsscVal v);
CSSC_API CsscVal  cssc_builtin_input(CsscVal prompt);
CSSC_API void     cssc_builtin_sleep(double seconds);

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

CSSC_API CsscVal  cssc_builtin_len(CsscVal v);
CSSC_API CsscVal  cssc_builtin_push(CsscVal arr, CsscVal item);
CSSC_API CsscVal  cssc_builtin_pop(CsscVal arr);
CSSC_API CsscVal  cssc_builtin_sort(CsscVal arr);
CSSC_API CsscVal  cssc_builtin_range(CsscVal start, CsscVal end, CsscVal step);

CSSC_API CsscVal  cssc_builtin_strlen_val(CsscVal s);
CSSC_API CsscVal  cssc_builtin_substr(CsscVal s, CsscVal start, CsscVal len);
CSSC_API CsscVal  cssc_builtin_replace(CsscVal s, CsscVal old_s, CsscVal new_s);
CSSC_API CsscVal  cssc_builtin_split(CsscVal s, CsscVal sep);
CSSC_API CsscVal  cssc_builtin_join(CsscVal arr, CsscVal sep);
CSSC_API CsscVal  cssc_builtin_trim(CsscVal s);
CSSC_API CsscVal  cssc_builtin_upper(CsscVal s);
CSSC_API CsscVal  cssc_builtin_lower(CsscVal s);
CSSC_API CsscVal  cssc_builtin_contains(CsscVal s, CsscVal sub);

CSSC_API CsscVal  cssc_builtin_uptime(void);

CSSC_API CsscVal  cssc_builtin_reboot(void);
CSSC_API CsscVal  cssc_builtin_time(void);
CSSC_API CsscVal  cssc_builtin_timestamp(void);
CSSC_API CsscVal  cssc_builtin_date(void);
CSSC_API CsscVal  cssc_builtin_datetime(void);
CSSC_API CsscVal  cssc_builtin_detime(void);
CSSC_API CsscVal  cssc_builtin_sdetime(void);

CSSC_API CsscVal  cssc_builtin_env(CsscVal name);
CSSC_API CsscVal  cssc_builtin_cwd(void);
CSSC_API CsscVal  cssc_builtin_platform(void);
CSSC_API CsscVal  cssc_builtin_exec(CsscVal cmd);
CSSC_API void     cssc_builtin_exit(CsscVal code);

CSSC_API CsscVal  cssc_builtin_read_file(CsscVal path);
CSSC_API void     cssc_builtin_write_file(CsscVal path, CsscVal content);
CSSC_API CsscVal  cssc_builtin_file_exists(CsscVal path);
CSSC_API void     cssc_builtin_mkdir(CsscVal path);

CSSC_API void     cssc_diag_emit_mem(void);
CSSC_API void     cssc_diag_emit_cpu(const char* name, uint32_t ms);
CSSC_API void     cssc_diag_tick(void);
CSSC_API void     cssc_diag_enable(int on);
CSSC_API int      cssc_diag_is_enabled(void);

CSSC_API CsscVal  cssc_builtin_diag_mem(void);
CSSC_API CsscVal  cssc_builtin_diag_cpu(CsscVal name, CsscVal ms);
CSSC_API CsscVal  cssc_builtin_diag_enable(CsscVal on);

CSSC_API void cssc_panic(const char* message);
CSSC_API void cssc_panicf(const char* fmt, ...);

CSSC_API void cssc_error(const char* operation, const char* detail, int line);

typedef CsscVal (*CsscHotloadFn)(CsscScopeStack* scope, CsscVal* args, uint32_t nargs);

typedef struct {
    void*   dll_handle;
    CsscHotloadFn* functions;
    const char**   func_names;
    uint32_t       func_count;
} CsscHotloadContext;

CSSC_API bool cssc_hotload_init(CsscHotloadContext* ctx, const char* dll_path);
CSSC_API CsscHotloadFn cssc_hotload_resolve(CsscHotloadContext* ctx, const char* name);
CSSC_API void cssc_hotload_shutdown(CsscHotloadContext* ctx);

CSSC_API CsscVal  cssc_peek_peek(CsscVal collection, int64_t index, int64_t amount);
CSSC_API CsscVal  cssc_peek_cpeek(CsscVal collection, int64_t index, int64_t count);
CSSC_API CsscVal  cssc_peek_peek_safe(CsscVal collection, int64_t index, CsscVal fallback);
CSSC_API bool     cssc_peek_has_next(CsscVal collection, int64_t index);

CSSC_API CsscVal  cssc_paths_exists(CsscVal path);
CSSC_API CsscVal  cssc_paths_mkdir(CsscVal path);
CSSC_API CsscVal  cssc_paths_dirname(CsscVal path);
CSSC_API CsscVal  cssc_paths_basename(CsscVal path);
CSSC_API CsscVal  cssc_paths_join(CsscVal a, CsscVal b);
CSSC_API CsscVal  cssc_paths_ext(CsscVal path);
CSSC_API CsscVal  cssc_paths_resolve(CsscVal path);

CSSC_API CsscVal  cssc_binary_read(CsscVal path);
CSSC_API void     cssc_binary_write(CsscVal path, CsscVal data);
CSSC_API CsscVal  cssc_binary_tobinary(CsscVal value);
CSSC_API CsscVal  cssc_binary_frombinary(CsscVal data);

CSSC_API CsscVal  cssc_io_read_file(CsscVal path);
CSSC_API void     cssc_io_write_file(CsscVal path, CsscVal content);
CSSC_API CsscVal  cssc_io_exists(CsscVal path);
CSSC_API void     cssc_io_remove(CsscVal path);
CSSC_API void     cssc_io_copy(CsscVal src, CsscVal dst);
CSSC_API CsscVal  cssc_io_list_dir(CsscVal path);

typedef struct {
    const char* label;
    void* init_fn;
    bool initialized;
    CsscVal sector_val;
} CsscDeferredSector;

CSSC_API void     cssc_deferred_register(CsscScopeStack* scope, const char* label, void* init_fn);
CSSC_API CsscVal  cssc_deferred_reserve(CsscScopeStack* scope, const char* label);

typedef struct {
    void*       dll_handle;
    const char* path;
    bool        started;
    CsscVal     module_val;
    void*       init_fn;
} CsscLoadedDll;

CSSC_API CsscVal  cssc_load_dll(CsscScopeStack* scope, const char* dll_path, const char* alias);
CSSC_API void     cssc_unload_dll(CsscVal module);

CSSC_API void* cssc_alloc(size_t size);
CSSC_API void* cssc_realloc(void* ptr, size_t new_size);
CSSC_API void  cssc_free(void* ptr);

CSSC_API const char* cssc_intern(const char* str);
CSSC_API bool cssc_intern_eq(const char* a, const char* b);

CSSC_API CsscVal cssc_format_value(CsscVal v);

CSSC_API uint32_t cssc_hash_string(const char* str);

typedef struct {
    CsscHeapHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;
} CsscFramebuffer;

typedef struct {
    CsscHeapHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;
} CsscMatrix;

typedef struct CsscVideoImpl CsscVideoImpl;

typedef struct {
    CsscHeapHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    CsscVideoImpl* impl;
} CsscVideo;

CSSC_API CsscVal cssc_video_create(int64_t width, int64_t height, int64_t fps);
CSSC_API CsscVal cssc_framebuffer_create(int64_t width, int64_t height);
CSSC_API CsscVal cssc_matrix_create(int64_t width, int64_t height);

CSSC_API void cssc_video_free(CsscVal v);
CSSC_API void cssc_framebuffer_free(CsscVal v);
CSSC_API void cssc_matrix_free(CsscVal v);

CSSC_API CsscVal cssc_matrix_fill(CsscVal matrix, CsscVal color);
CSSC_API void cssc_matrix_set_pixel(CsscVal matrix, int64_t x, int64_t y, uint32_t color);
CSSC_API uint32_t cssc_matrix_get_pixel(CsscVal matrix, int64_t x, int64_t y);
CSSC_API void cssc_matrix_copy_to_framebuffer(CsscVal matrix, CsscVal fb);

CSSC_API void cssc_framebuffer_clear(CsscVal fb, uint32_t color);
CSSC_API void cssc_framebuffer_set_pixel(CsscVal fb, int64_t x, int64_t y, uint32_t color);

CSSC_API void cssc_video_clear(CsscVal video, uint32_t color);
CSSC_API void cssc_video_set_matrix(CsscVal video, CsscVal fb_or_matrix);
CSSC_API void cssc_video_present(CsscVal video);
CSSC_API bool cssc_video_is_open(CsscVal video);

CSSC_API void cssc_video_draw_text(CsscVal video, int64_t x, int64_t y,
                                   const char* text, int64_t size_px,
                                   int64_t weight, uint32_t color,
                                   const char* font_family);

CSSC_API int64_t cssc_video_measure_text(const char* text, int64_t size_px,
                                         int64_t weight, const char* font_family);

CSSC_API void cssc_video_draw_rect(CsscVal video, int64_t x, int64_t y,
                                   int64_t w, int64_t h, uint32_t color);

CSSC_API void cssc_video_blit_matrix(CsscVal video, CsscVal matrix,
                                     int64_t dst_x, int64_t dst_y);

CSSC_API CsscVal cssc_image_load_bmp(const char* path);

typedef struct {
    CsscHeapHeader header;
    int     width;
    int     height;
    void*   hout;
    void*   hin;
    uint16_t default_attr;
    uint16_t current_attr;
    bool    allocated;
} CsscConsole;

CSSC_API CsscVal cssc_console_create(int64_t w, int64_t h);
CSSC_API void    cssc_console_free(CsscVal c);

CSSC_API void    cssc_console_out(CsscVal c, CsscVal text);
CSSC_API void    cssc_console_clear_all(CsscVal c);
CSSC_API void    cssc_console_clear_line(CsscVal c, int64_t line);
CSSC_API void    cssc_console_cursor_col(CsscVal c, CsscVal color);
CSSC_API void    cssc_console_cursor_set(CsscVal c, int64_t line, int64_t col);
CSSC_API void    cssc_console_cursor_pos(CsscVal c, CsscVal pos);
CSSC_API CsscVal cssc_console_get_line(CsscVal c, int64_t line);
CSSC_API CsscVal cssc_console_cursor_get_line(CsscVal c);

CSSC_API CsscVal cssc_console_cursor_at(CsscVal c);

CSSC_API uint32_t cssc_color_from_val(CsscVal v);

typedef struct CsscDaemonImpl CsscDaemonImpl;

CSSC_API CsscVal cssc_daemon_start(CsscScopeStack* scope, const char* func_name);
CSSC_API void    cssc_daemon_kill(const char* func_name);
CSSC_API bool    cssc_daemon_should_stop(const char* func_name);
CSSC_API void    cssc_daemon_shutdown_all(void);

CSSC_API void cssc_runtime_init(void);

CSSC_API void cssc_runtime_shutdown(void);

CSSC_API CsscScopeStack* cssc_global_scope(void);

CSSC_API void cssc_global_scope_set(CsscScopeStack* external);

typedef struct CsscObjHandleImpl CsscObjHandleImpl;

typedef struct {
    CsscHeapHeader header;
    const char* alias;
    const char* project_name;
    CsscVal      module_sector;
    CsscObjHandleImpl* impl;
} CsscObjHandle;

CSSC_API CsscVal cssc_obj_load(CsscScopeStack* host_scope, const char* obj_path, const char* alias);

CSSC_API void cssc_obj_unload(CsscVal handle_val);

CSSC_API CsscVal cssc_cobj_load(CsscScopeStack* host_scope,
                                const char* cobj_path, const char* alias);

CSSC_API void cssc_watermark_show_once(void);
CSSC_API void cssc_watermark_dismiss(void);
CSSC_API bool cssc_watermark_is_showing(void);

CSSC_API bool cssc_sound_play_file(const char* path, bool async);
CSSC_API bool cssc_sound_play_memory(const void* data, uint32_t size, bool async);
CSSC_API void cssc_sound_stop(void);

CSSC_API CsscVal cssc_obj_asset_read(const char* obj_path, const char* entry_name);

CSSC_API void cssc_intro_play(const char* title,
                              const char* subtitle,
                              int32_t duration_ms,
                              const void* wav_data,
                              uint32_t wav_size);

#ifdef __cplusplus
}
#endif

#endif
