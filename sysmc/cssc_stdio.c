/*
 * cssc_stdio.c — v6-native runtime for the `stdio` CSSC module.
 *
 * Mirrors the interpreter's stdio surface used by real CSSC programs
 * (e.g. the CCC compressor):
 *   stdio::exists(path)          -> int  (1 if a readable file exists, else 0)
 *   stdio::read(path)            -> string (heap cssc_str*; whole file, "" on miss)
 *   stdio::write(path, content)  -> int  (1 on success, 0 on I/O failure) — truncating write
 *   stdio::createfile(path)      -> int  (1 on success) — creates/truncates to empty
 *   stdio::cwd()                 -> string (heap cssc_str*; current working directory)
 *
 * Path arguments arrive as NUL-terminated `const char*` (the CSTR dispatch
 * passes the cssc_str's inline, NUL-terminated data buffer). `content`
 * arrives as the raw `cssc_str*` so a binary-safe length is available via
 * cssc_string_size (embedded NULs survive).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#if defined(_WIN32)
    #include <direct.h>
    #include <windows.h>
    #define cssc_getcwd _getcwd
    #define cssc_mkdir(p) _mkdir(p)
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #define cssc_getcwd getcwd
    #define cssc_mkdir(p) mkdir((p), 0777)
#endif

/* Recursively delete a directory tree — matches the interpreter's
 * shutil.rmtree. Returns 1 on success (tree gone), 0 on failure. */
static int cssc_rmtree(const char* path) {
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
            const char* n = fd.cFileName;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
                continue;
            char child[4096];
            size_t nl = strlen(n);
            if (pl + 1 + nl >= sizeof(child)) continue;
            memcpy(child, path, pl);
            child[pl] = '\\';
            memcpy(child + pl + 1, n, nl + 1);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                cssc_rmtree(child);
            else
                DeleteFileA(child);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return RemoveDirectoryA(path) ? 1 : 0;
#else
    DIR* d = opendir(path);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            const char* n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
                continue;
            char child[4096];
            snprintf(child, sizeof(child), "%s/%s", path, n);
            struct stat st;
            if (stat(child, &st) == 0 && S_ISDIR(st.st_mode))
                cssc_rmtree(child);
            else
                unlink(child);
        }
        closedir(d);
    }
    return rmdir(path) == 0 ? 1 : 0;
#endif
}

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define CSSC_STDIO_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
    #define CSSC_STDIO_EXPORT __attribute__((visibility("default")))
#else
    #define CSSC_STDIO_EXPORT
#endif

/* Heap-string constructor from the transembly runtime: (data, len) -> cssc_str*. */
extern void*  cssc_string_lit(const char* data, size_t len);
/* Binary-safe accessors for a cssc_str* value (transembly runtime). */
extern int64_t cssc_string_size(void* s);
extern void*   cssc_string_data(void* s);

CSSC_STDIO_EXPORT int64_t cssc_stdio_exists(const char* path) {
    if (!path) return 0;
    FILE* f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

CSSC_STDIO_EXPORT void* cssc_stdio_read(const char* path) {
    if (!path) return cssc_string_lit("", 0);
    FILE* f = fopen(path, "rb");
    if (!f) return cssc_string_lit("", 0);
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return cssc_string_lit("", 0); }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return cssc_string_lit("", 0); }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return cssc_string_lit("", 0); }
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return cssc_string_lit("", 0); }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    void* s = cssc_string_lit(buf, rd);
    free(buf);
    return s;
}

CSSC_STDIO_EXPORT int64_t cssc_stdio_write(const char* path, void* content) {
    if (!path) return 0;
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    int64_t len = content ? cssc_string_size(content) : 0;
    int ok = 1;
    if (len > 0) {
        const void* data = cssc_string_data(content);
        size_t w = fwrite(data, 1, (size_t)len, f);
        ok = (w == (size_t)len);
    }
    fclose(f);
    return ok ? 1 : 0;
}

CSSC_STDIO_EXPORT int64_t cssc_stdio_createfile(const char* path) {
    if (!path) return 0;
    /* Create the parent directory chain first, matching the interpreter's
       `os.makedirs(dirname(path), exist_ok=True)` so
       `createfile("data/user.ini")` succeeds even when `data/` does not
       exist yet (otherwise fopen fails and this returns 0). */
    size_t len = strlen(path);
    if (len && len < 4096) {
        char tmp[4096];
        memcpy(tmp, path, len + 1);
        size_t last_sep = 0;
        for (size_t i = 0; i < len; i++) {
            if (tmp[i] == '/' || tmp[i] == '\\') last_sep = i;
        }
        if (last_sep > 0) {
            tmp[last_sep] = 0;
            for (size_t i = 1; i < last_sep; i++) {
                char c = tmp[i];
                if (c == '/' || c == '\\') {
                    tmp[i] = 0;
                    if (tmp[0]) cssc_mkdir(tmp);
                    tmp[i] = c;
                }
            }
            if (tmp[0]) cssc_mkdir(tmp);
        }
    }
    FILE* f = fopen(path, "wb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

CSSC_STDIO_EXPORT void* cssc_stdio_cwd(void) {
    char buf[4096];
    if (cssc_getcwd(buf, sizeof(buf)) == NULL) return cssc_string_lit("", 0);
    return cssc_string_lit(buf, strlen(buf));
}

/* stdio::listdir(path) -> the directory's entry names (files + subdirs, minus
   "." and ".."), as one '\n'-separated string ("" on error / empty dir). New in
   CSSC 7.0. Callers split on '\n'; the empty string yields no entries. */
CSSC_STDIO_EXPORT void* cssc_stdio_listdir(const char* path) {
    if (!path || !path[0]) return cssc_string_lit("", 0);
    char* buf = NULL;
    size_t cap = 0, len = 0;
#if defined(_WIN32)
    char pattern[4096];
    size_t pl = strlen(path);
    if (pl + 3 >= sizeof(pattern)) return cssc_string_lit("", 0);
    memcpy(pattern, path, pl);
    pattern[pl] = '\\'; pattern[pl + 1] = '*'; pattern[pl + 2] = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            const char* n = fd.cFileName;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
                continue;
            size_t nl = strlen(n);
            if (len + nl + 2 > cap) {
                cap = (len + nl + 2) * 2 + 64;
                char* nb = (char*)realloc(buf, cap);
                if (!nb) { free(buf); FindClose(h); return cssc_string_lit("", 0); }
                buf = nb;
            }
            if (len > 0) buf[len++] = '\n';
            memcpy(buf + len, n, nl); len += nl;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR* d = opendir(path);
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            const char* n = e->d_name;
            if (n[0] == '.' && (n[1] == 0 || (n[1] == '.' && n[2] == 0)))
                continue;
            size_t nl = strlen(n);
            if (len + nl + 2 > cap) {
                cap = (len + nl + 2) * 2 + 64;
                char* nb = (char*)realloc(buf, cap);
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
    void* s = cssc_string_lit(buf, len);
    free(buf);
    return s;
}

CSSC_STDIO_EXPORT int64_t cssc_stdio_removefile(const char* path) {
    if (!path) return 0;
    /* C stdlib `remove` unlinks a file on both Windows and POSIX. */
    return remove(path) == 0 ? 1 : 0;
}

CSSC_STDIO_EXPORT int64_t cssc_stdio_createdir(const char* path) {
    if (!path || !path[0]) return 0;
    /* Recursive: create each ancestor, matching os.makedirs(exist_ok=True). */
    char tmp[4096];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return 0;
    memcpy(tmp, path, len + 1);
    for (size_t i = 1; i < len; i++) {
        char c = tmp[i];
        if (c == '/' || c == '\\') {
            tmp[i] = 0;
            if (tmp[0]) cssc_mkdir(tmp);   /* ignore intermediate EEXIST */
            tmp[i] = c;
        }
    }
    if (cssc_mkdir(tmp) == 0) return 1;
    return (errno == EEXIST) ? 1 : 0;   /* already present == success */
}

CSSC_STDIO_EXPORT int64_t cssc_stdio_removedir(const char* path) {
    if (!path) return 0;
    return cssc_rmtree(path);
}

CSSC_STDIO_EXPORT int64_t cssc_stdio_move(const char* src, const char* dst) {
    if (!src || !dst) return 0;
    /* rename handles both files and directories on the same volume;
     * matches shutil.move for the common same-volume case. */
    if (rename(src, dst) == 0) return 1;
#if defined(_WIN32)
    /* Cross-directory / existing-target: fall back to a forced move. */
    if (MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
        return 1;
#endif
    return 0;
}
