

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define CSSC_INI_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
    #define CSSC_INI_EXPORT __attribute__((visibility("default")))
#else
    #define CSSC_INI_EXPORT
#endif

extern void* cssc_string_lit(const char* data, size_t len);

static char* ini_trim(char* s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = 0;
    return s;
}

static int ini_read_all(const char* path, char** out, size_t* out_len) {
    *out = NULL;
    *out_len = 0;
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 0; }
    rewind(f);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return 0; }
    size_t read = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[read] = '\0';
    *out = buf;
    *out_len = read;
    return 1;
}

static int ini_write_all(const char* path, const char* data, size_t len) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    size_t w = fwrite(data, 1, len, f);
    fclose(f);
    return w == len ? 1 : 0;
}

static int ini_line_is_section(const char* line, const char* section) {
    const char* p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '[') return 0;
    p++;
    const char* end = strchr(p, ']');
    if (!end) return 0;
    size_t nlen = (size_t)(end - p);
    size_t slen = strlen(section);
    if (nlen != slen) return 0;
    return memcmp(p, section, slen) == 0 ? 1 : 0;
}

static int ini_line_matches_key(const char* line, const char* key) {
    const char* p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == 0 || *p == ';' || *p == '#' || *p == '[') return 0;
    const char* eq = strchr(p, '=');
    if (!eq) return 0;
    const char* kend = eq;
    while (kend > p && isspace((unsigned char)*(kend - 1))) kend--;
    size_t klen = (size_t)(kend - p);
    size_t xlen = strlen(key);
    if (klen != xlen) return 0;
    return memcmp(p, key, xlen) == 0 ? 1 : 0;
}

static const char* ini_value_after_eq(const char* line) {
    const char* eq = strchr(line, '=');
    if (!eq) return NULL;
    eq++;
    while (*eq && (*eq == ' ' || *eq == '\t')) eq++;
    return eq;
}

CSSC_INI_EXPORT int64_t cssc_ini_exists(const char* path) {
    if (!path) return 0;
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

CSSC_INI_EXPORT void* cssc_ini_read(const char* path, const char* section,
                                      const char* key, const char* fallback) {
    if (!path || !section || !key) {
        const char* fb = fallback ? fallback : "";
        return cssc_string_lit(fb, strlen(fb));
    }
    char* buf = NULL;
    size_t len = 0;
    if (!ini_read_all(path, &buf, &len)) {
        const char* fb = fallback ? fallback : "";
        return cssc_string_lit(fb, strlen(fb));
    }
    int in_section = 0;
    char* line_start = buf;
    for (char* p = buf; ; ++p) {
        int at_end = (*p == '\0');
        int at_nl  = (*p == '\n');
        if (!at_end && !at_nl) continue;
        char saved = *p;
        *p = '\0';

        char* comment = line_start;
        while (*comment) {
            if (*comment == ';' || *comment == '#') { *comment = '\0'; break; }
            comment++;
        }
        size_t sl = strlen(line_start);
        if (sl > 0 && line_start[sl - 1] == '\r') line_start[sl - 1] = '\0';
        char* trimmed = ini_trim(line_start);
        if (*trimmed == '[') {
            in_section = ini_line_is_section(trimmed, section);
        } else if (in_section && ini_line_matches_key(trimmed, key)) {
            const char* val = ini_value_after_eq(trimmed);
            if (!val) val = "";

            size_t vlen = strlen(val);
            while (vlen > 0 && isspace((unsigned char)val[vlen - 1])) vlen--;
            void* r = cssc_string_lit(val, vlen);
            free(buf);
            return r;
        }
        if (at_end) break;
        *p = saved;
        line_start = p + 1;
    }
    free(buf);
    const char* fb = fallback ? fallback : "";
    return cssc_string_lit(fb, strlen(fb));
}

CSSC_INI_EXPORT int64_t cssc_ini_has_key(const char* path, const char* section,
                                           const char* key) {
    if (!path || !section || !key) return 0;
    char* buf = NULL;
    size_t len = 0;
    if (!ini_read_all(path, &buf, &len)) return 0;
    int in_section = 0;
    int64_t found = 0;
    char* line_start = buf;
    for (char* p = buf; ; ++p) {
        int at_end = (*p == '\0');
        int at_nl  = (*p == '\n');
        if (!at_end && !at_nl) continue;
        char saved = *p;
        *p = '\0';
        char* comment = line_start;
        while (*comment) {
            if (*comment == ';' || *comment == '#') { *comment = '\0'; break; }
            comment++;
        }
        size_t sl = strlen(line_start);
        if (sl > 0 && line_start[sl - 1] == '\r') line_start[sl - 1] = '\0';
        char* trimmed = ini_trim(line_start);
        if (*trimmed == '[') {
            in_section = ini_line_is_section(trimmed, section);
        } else if (in_section && ini_line_matches_key(trimmed, key)) {
            found = 1;
            break;
        }
        if (at_end) break;
        *p = saved;
        line_start = p + 1;
    }
    free(buf);
    return found;
}

CSSC_INI_EXPORT int64_t cssc_ini_write(const char* path, const char* section,
                                         const char* key, const char* value) {
    if (!path || !section || !key) return 0;
    if (!value) value = "";
    char* buf = NULL;
    size_t buf_len = 0;
    (void)ini_read_all(path, &buf, &buf_len);

    size_t out_cap = buf_len + strlen(section) + strlen(key) + strlen(value)
                      + 64;
    char* out = (char*)malloc(out_cap);
    if (!out) { free(buf); return 0; }
    size_t out_len = 0;
    int in_target_section = 0;
    int wrote_key = 0;
    int saw_target_section = 0;
    int need_section_trailing_nl = 0;
    char* line_start = buf;

    for (char* p = buf; buf; ++p) {
        int at_end = (*p == '\0');
        int at_nl  = (*p == '\n');
        if (!at_end && !at_nl) continue;
        size_t line_len = (size_t)(p - line_start);

        char* line_copy = (char*)malloc(line_len + 1);
        if (!line_copy) { free(out); free(buf); return 0; }
        memcpy(line_copy, line_start, line_len);
        line_copy[line_len] = '\0';

        char cls[1024];
        size_t cls_len = line_len;
        if (cls_len >= sizeof(cls)) cls_len = sizeof(cls) - 1;
        memcpy(cls, line_copy, cls_len);
        cls[cls_len] = '\0';
        if (cls_len > 0 && cls[cls_len - 1] == '\r') cls[cls_len - 1] = '\0';

        for (char* c = cls; *c; ++c) {
            if (*c == ';' || *c == '#') { *c = '\0'; break; }
        }
        char* trimmed = ini_trim(cls);

        int is_section = (*trimmed == '[');
        int is_target  = is_section && ini_line_is_section(trimmed, section);
        int is_key_match = in_target_section
                           && ini_line_matches_key(trimmed, key);

        if (is_section) {

            if (in_target_section && !wrote_key) {
                int n = snprintf(out + out_len, out_cap - out_len,
                                 "%s=%s\n", key, value);
                if (n < 0 || (size_t)n >= out_cap - out_len) {
                    free(line_copy); free(out); free(buf); return 0;
                }
                out_len += (size_t)n;
                wrote_key = 1;
            }
            in_target_section = is_target;
            if (is_target) saw_target_section = 1;
        }

        if (is_key_match && in_target_section) {

            int n = snprintf(out + out_len, out_cap - out_len,
                             "%s=%s", key, value);
            if (n < 0 || (size_t)n >= out_cap - out_len) {
                free(line_copy); free(out); free(buf); return 0;
            }
            out_len += (size_t)n;
            wrote_key = 1;
        } else {

            if (out_len + line_len >= out_cap) {
                free(line_copy); free(out); free(buf); return 0;
            }
            memcpy(out + out_len, line_copy, line_len);
            out_len += line_len;
        }
        free(line_copy);
        if (at_nl) {
            if (out_len + 1 >= out_cap) { free(out); free(buf); return 0; }
            out[out_len++] = '\n';
        }
        if (at_end) break;
        line_start = p + 1;
    }

    if (!saw_target_section) {

        need_section_trailing_nl = (out_len > 0 && out[out_len - 1] != '\n');
        size_t hdr_need = strlen(section) + strlen(key) + strlen(value) + 8;
        if (out_len + hdr_need >= out_cap) {
            size_t new_cap = out_cap * 2 + hdr_need;
            char* grown = (char*)realloc(out, new_cap);
            if (!grown) { free(out); free(buf); return 0; }
            out = grown;
            out_cap = new_cap;
        }
        int n = snprintf(out + out_len, out_cap - out_len,
                         "%s[%s]\n%s=%s\n",
                         need_section_trailing_nl ? "\n" : "",
                         section, key, value);
        if (n < 0) { free(out); free(buf); return 0; }
        out_len += (size_t)n;
        wrote_key = 1;
    } else if (!wrote_key) {

        if (out_len + strlen(key) + strlen(value) + 4 >= out_cap) {
            size_t new_cap = out_cap * 2 + strlen(key) + strlen(value) + 4;
            char* grown = (char*)realloc(out, new_cap);
            if (!grown) { free(out); free(buf); return 0; }
            out = grown;
            out_cap = new_cap;
        }
        int n = snprintf(out + out_len, out_cap - out_len,
                         "%s%s=%s\n",
                         (out_len > 0 && out[out_len - 1] != '\n') ? "\n" : "",
                         key, value);
        if (n < 0) { free(out); free(buf); return 0; }
        out_len += (size_t)n;
        wrote_key = 1;
    }

    int ok = ini_write_all(path, out, out_len);
    free(out);
    free(buf);
    return ok ? 1 : 0;
}

CSSC_INI_EXPORT int64_t cssc_ini_delete(const char* path, const char* section,
                                          const char* key) {
    if (!path || !section || !key) return 0;
    char* buf = NULL;
    size_t buf_len = 0;
    if (!ini_read_all(path, &buf, &buf_len)) return 0;
    char* out = (char*)malloc(buf_len + 1);
    if (!out) { free(buf); return 0; }
    size_t out_len = 0;
    int in_section = 0;
    int removed = 0;
    char* line_start = buf;
    for (char* p = buf; ; ++p) {
        int at_end = (*p == '\0');
        int at_nl  = (*p == '\n');
        if (!at_end && !at_nl) continue;
        size_t line_len = (size_t)(p - line_start);
        char* line_copy = (char*)malloc(line_len + 1);
        if (!line_copy) { free(out); free(buf); return 0; }
        memcpy(line_copy, line_start, line_len);
        line_copy[line_len] = '\0';
        char cls[1024];
        size_t cls_len = line_len;
        if (cls_len >= sizeof(cls)) cls_len = sizeof(cls) - 1;
        memcpy(cls, line_copy, cls_len);
        cls[cls_len] = '\0';
        if (cls_len > 0 && cls[cls_len - 1] == '\r') cls[cls_len - 1] = '\0';
        for (char* c = cls; *c; ++c) {
            if (*c == ';' || *c == '#') { *c = '\0'; break; }
        }
        char* trimmed = ini_trim(cls);

        if (*trimmed == '[') {
            in_section = ini_line_is_section(trimmed, section);
            memcpy(out + out_len, line_copy, line_len);
            out_len += line_len;
        } else if (in_section && ini_line_matches_key(trimmed, key)) {
            removed = 1;

            free(line_copy);
            if (at_end) break;
            line_start = p + 1;
            continue;
        } else {
            memcpy(out + out_len, line_copy, line_len);
            out_len += line_len;
        }
        free(line_copy);
        if (at_nl) out[out_len++] = '\n';
        if (at_end) break;
        line_start = p + 1;
    }
    if (removed) {
        ini_write_all(path, out, out_len);
    }
    free(out);
    free(buf);
    return removed;
}
