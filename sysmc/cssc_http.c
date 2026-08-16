/* cssc_http.c — HTTP backends.
 *
 * Win32 host uses WinHTTP (already linked via -lwinhttp in the build).
 * ESP32 uses esp_http_client. ESP8266 uses ESP8266HTTPClient.
 * Linux uses libcurl. Arduino AVR has no networking — stub returns
 * an error result.
 *
 * Result memory ownership: every successful call hands the caller a
 * heap-allocated body + error string. The compiler emits a paired
 * cssc_http_result_free(&r) at the end of the result's scope.
 */

#include "cssc_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>     /* tzset() for cssc_http_set_timezone */

static char* _http_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

static CsscHttpResult _http_err(int code, const char* msg) {
    CsscHttpResult r = {0};
    r.status = code;
    r.body = _http_strdup("");
    r.body_len = 0;
    r.error = _http_strdup(msg ? msg : "unknown error");
    return r;
}

#if defined(_WIN32) && !defined(CSSC_EMBEDDED)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

/* Convert URL into host/path/scheme via WinHttpCrackUrl. Returns 0 on
 * success, fills out_host (wide), out_path (wide), out_port, out_https. */
static int _http_split_url(const char* url, wchar_t* host, size_t host_n,
                           wchar_t* path, size_t path_n,
                           int* out_port, int* out_https) {
    wchar_t wurl[2048] = {0};
    int wlen = MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 2048);
    if (wlen <= 0) return -1;
    URL_COMPONENTSW uc = {0};
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host; uc.dwHostNameLength = (DWORD)host_n;
    uc.lpszUrlPath  = path; uc.dwUrlPathLength  = (DWORD)path_n;
    uc.dwSchemeLength = 1;  /* request scheme parsing */
    if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) return -1;
    *out_port  = uc.nPort;
    *out_https = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? 1 : 0;
    return 0;
}

static CsscHttpResult _http_winhttp(const char* method, const char* url,
                                     const char* body, size_t body_len) {
    CsscHttpResult result = {0};
    wchar_t host[256] = {0}, path[1024] = {0};
    int port = 0, https = 0;
    if (_http_split_url(url, host, 256, path, 1024, &port, &https) != 0) {
        return _http_err(0, "URL parse failed");
    }
    HINTERNET hSession = WinHttpOpen(L"cssc-network-http/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return _http_err(0, "WinHttpOpen failed");
    HINTERNET hConnect = WinHttpConnect(hSession, host, (INTERNET_PORT)port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return _http_err(0, "WinHttpConnect failed"); }
    wchar_t wmethod[16] = {0};
    MultiByteToWideChar(CP_UTF8, 0, method ? method : "GET", -1, wmethod, 16);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, wmethod, path, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        https ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return _http_err(0, "WinHttpOpenRequest failed");
    }
    BOOL ok = WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        (LPVOID)body, (DWORD)body_len, (DWORD)body_len, 0);
    if (!ok) {
        DWORD err = GetLastError();
        char errbuf[64]; snprintf(errbuf, 64, "WinHttpSendRequest err=%lu", err);
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return _http_err(0, errbuf);
    }
    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return _http_err(0, "WinHttpReceiveResponse failed");
    }
    DWORD status_code = 0; DWORD scs = sizeof(status_code);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &scs, WINHTTP_NO_HEADER_INDEX);
    /* Read body into a growable buffer. */
    size_t cap = 4096, len = 0;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return _http_err((int)status_code, "out of memory");
    }
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
        if (len + avail + 1 > cap) {
            while (len + avail + 1 > cap) cap *= 2;
            char* nb = (char*)realloc(buf, cap);
            if (!nb) { free(buf); buf = NULL; break; }
            buf = nb;
        }
        DWORD got = 0;
        WinHttpReadData(hRequest, buf + len, avail, &got);
        if (got == 0) break;
        len += got;
    }
    if (buf) buf[len] = '\0';
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
    result.status = (int32_t)status_code;
    result.body = buf ? buf : _http_strdup("");
    result.body_len = len;
    result.error = _http_strdup("");
    return result;
}

#elif defined(CSSC_ESP32) && defined(__has_include) && __has_include("esp_http_client.h")
#define CSSC_HTTP_HAS_ESP_IDF 1

#include "esp_http_client.h"
#include "esp_log.h"
/* esp_sntp is part of lwIP — also required for cssc::ntp_sync. */
#if defined(__has_include) && __has_include("esp_sntp.h")
  #include "esp_sntp.h"
#elif defined(__has_include) && __has_include("lwip/apps/sntp.h")
  #include "lwip/apps/sntp.h"
#endif
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static esp_err_t _http_evt_handler(esp_http_client_event_t *evt) {
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        /* Body chunks are accumulated by user_data buffer below. */
        char** out = (char**)evt->user_data;
        size_t cur = *out ? strlen(*out) : 0;
        char* nb = (char*)realloc(*out, cur + evt->data_len + 1);
        if (nb) {
            memcpy(nb + cur, evt->data, evt->data_len);
            nb[cur + evt->data_len] = '\0';
            *out = nb;
        }
    }
    return ESP_OK;
}

static CsscHttpResult _http_esp32(const char* method, const char* url,
                                   const char* body, size_t body_len) {
    CsscHttpResult r = {0};
    char* body_buf = NULL;
    esp_http_client_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.url = url;
    cfg.event_handler = _http_evt_handler;
    cfg.user_data = &body_buf;
    cfg.timeout_ms = 30000;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return _http_err(0, "esp_http_client_init failed");
    esp_http_client_method_t m = HTTP_METHOD_GET;
    if (method && strcmp(method, "POST") == 0) m = HTTP_METHOD_POST;
    else if (method && strcmp(method, "PUT") == 0) m = HTTP_METHOD_PUT;
    else if (method && strcmp(method, "DELETE") == 0) m = HTTP_METHOD_DELETE;
    else if (method && strcmp(method, "PATCH") == 0) m = HTTP_METHOD_PATCH;
    esp_http_client_set_method(client, m);
    if (body && body_len > 0) {
        esp_http_client_set_post_field(client, body, body_len);
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        if (body_buf) free(body_buf);
        return _http_err(0, esp_err_to_name(err));
    }
    r.status = (int32_t)esp_http_client_get_status_code(client);
    r.body = body_buf ? body_buf : _http_strdup("");
    r.body_len = body_buf ? strlen(body_buf) : 0;
    r.error = _http_strdup("");
    esp_http_client_cleanup(client);
    return r;
}

#elif defined(CSSC_ESP8266) && defined(__has_include) && __has_include(<ESP8266WiFi.h>)
#define CSSC_HTTP_HAS_ESP8266_WIFI 1

/* Arduino-core path uses ESP8266HTTPClient — pulled by the user's
 * build environment. Surface kept minimal because the AVR/ESP8266
 * preprocessors don't like complex C++ in our wrapper. */
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

static CsscHttpResult _http_esp8266(const char* method, const char* url,
                                     const char* body, size_t body_len) {
    CsscHttpResult r = {0};
    WiFiClient client;
    HTTPClient http;
    if (!http.begin(client, url)) return _http_err(0, "HTTPClient.begin failed");
    int code;
    if (method && strcmp(method, "POST") == 0) {
        code = http.POST((uint8_t*)body, body_len);
    } else if (method && strcmp(method, "PUT") == 0) {
        code = http.PUT((uint8_t*)body, body_len);
    } else {
        code = http.GET();
    }
    r.status = code > 0 ? (int32_t)code : 0;
    if (code > 0) {
        String s = http.getString();
        r.body = _http_strdup(s.c_str());
        r.body_len = s.length();
        r.error = _http_strdup("");
    } else {
        r.body = _http_strdup("");
        r.error = _http_strdup(http.errorToString(code).c_str());
    }
    http.end();
    return r;
}

#elif defined(CSSC_LINUX) && defined(__has_include) && __has_include(<curl/curl.h>)
#define CSSC_HTTP_HAS_LIBCURL 1

#include <curl/curl.h>

typedef struct { char* buf; size_t len; size_t cap; } _BodyBuf;
static size_t _curl_write(void* ptr, size_t sz, size_t n, void* user) {
    _BodyBuf* b = (_BodyBuf*)user;
    size_t add = sz * n;
    if (b->len + add + 1 > b->cap) {
        while (b->len + add + 1 > b->cap) b->cap = b->cap ? b->cap * 2 : 4096;
        char* nb = (char*)realloc(b->buf, b->cap);
        if (!nb) return 0;
        b->buf = nb;
    }
    memcpy(b->buf + b->len, ptr, add);
    b->len += add;
    b->buf[b->len] = '\0';
    return add;
}

static CsscHttpResult _http_curl(const char* method, const char* url,
                                  const char* body, size_t body_len) {
    CsscHttpResult r = {0};
    CURL* c = curl_easy_init();
    if (!c) return _http_err(0, "curl_easy_init failed");
    _BodyBuf bb = {0};
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, _curl_write);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &bb);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "cssc-network-http/1.0");
    curl_easy_setopt(c, CURLOPT_TIMEOUT, 30L);
    if (method && strcmp(method, "GET") != 0) {
        curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, method);
    }
    if (body && body_len > 0) {
        curl_easy_setopt(c, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, (long)body_len);
    }
    CURLcode rc = curl_easy_perform(c);
    long status = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &status);
    if (rc != CURLE_OK) {
        free(bb.buf);
        curl_easy_cleanup(c);
        return _http_err((int)status, curl_easy_strerror(rc));
    }
    r.status = (int32_t)status;
    r.body = bb.buf ? bb.buf : _http_strdup("");
    r.body_len = bb.len;
    r.error = _http_strdup("");
    curl_easy_cleanup(c);
    return r;
}

#endif
/* AVR / embedded fallback handled inline in cssc_http_request below — no
 * separate function needed since there's no actual networking to do. */


/* =====================================================================
 * Dispatcher
 * ===================================================================== */

CSSC_HTTP_API CsscHttpResult cssc_http_request(const char* method,
                                                const char* url,
                                                const char* body, size_t body_len) {
    if (!url || !*url) return _http_err(0, "empty URL");
#if defined(_WIN32) && !defined(CSSC_EMBEDDED)
    return _http_winhttp(method, url, body, body_len);
#elif defined(CSSC_HTTP_HAS_ESP_IDF)
    return _http_esp32(method, url, body, body_len);
#elif defined(CSSC_HTTP_HAS_ESP8266_WIFI)
    return _http_esp8266(method, url, body, body_len);
#elif defined(CSSC_HTTP_HAS_LIBCURL)
    return _http_curl(method, url, body, body_len);
#elif defined(CSSC_ARDUINO)
    /* Bare AVR Arduino has no networking — return a clear error. */
    (void)method; (void)url; (void)body; (void)body_len;
    return _http_err(0, "no network stack on this Arduino board");
#elif defined(CSSC_EMBEDDED)
    /* Embedded target without an SDK that provides HTTP — return a
     * clean error rather than a link failure. The user can later
     * compile the generated .c through PIO/Arduino IDE for real
     * networking. */
    (void)method; (void)url; (void)body; (void)body_len;
    return _http_err(0, "no HTTP backend on this embedded target "
                       "(SDK header missing at compile time)");
#else
    return _http_err(0, "no HTTP backend compiled in");
#endif
}

CSSC_HTTP_API CsscHttpResult cssc_http_get(const char* url) {
    return cssc_http_request("GET", url, NULL, 0);
}

CSSC_HTTP_API CsscHttpResult cssc_http_post(const char* url,
                                             const char* body, size_t body_len) {
    return cssc_http_request("POST", url, body, body_len);
}

CSSC_HTTP_API void cssc_http_result_free(CsscHttpResult* r) {
    if (!r) return;
    free(r->body);   r->body = NULL;
    free(r->error);  r->error = NULL;
}

/* =====================================================================
 * Glue for the codegen — turn a CsscHttpResult into a CsscVal map.
 * ===================================================================== */

static CsscVal _http_result_to_map(CsscHttpResult* r) {
    CsscVal m = cssc_map(8);
    cssc_map_set(m, "status", cssc_int((int64_t)r->status));
    cssc_map_set(m, "body",   cssc_string(r->body ? r->body : ""));
    cssc_map_set(m, "ok",     cssc_bool(r->status >= 200 && r->status < 300));
    cssc_map_set(m, "error",  cssc_string(r->error ? r->error : ""));
    return m;
}

CSSC_HTTP_API CsscVal cssc_http_get_to_val(CsscVal url) {
    const char* u = (CSSC_TYPE(url) == CSSC_TYPE_STRING && url.data.ptr)
                        ? (const char*)url.data.ptr : "";
    CsscHttpResult r = cssc_http_get(u);
    CsscVal v = _http_result_to_map(&r);
    cssc_http_result_free(&r);
    return v;
}

CSSC_HTTP_API CsscVal cssc_http_post_to_val(CsscVal url, CsscVal body) {
    const char* u = (CSSC_TYPE(url) == CSSC_TYPE_STRING && url.data.ptr)
                        ? (const char*)url.data.ptr : "";
    const char* b = (CSSC_TYPE(body) == CSSC_TYPE_STRING && body.data.ptr)
                        ? (const char*)body.data.ptr : "";
    size_t bn = b ? strlen(b) : 0;
    CsscHttpResult r = cssc_http_post(u, b, bn);
    CsscVal v = _http_result_to_map(&r);
    cssc_http_result_free(&r);
    return v;
}

/* WiFi STA connect — blocks until joined or 20s timeout. Embedded-only;
 * on desktop this is a no-op-success since the OS handles networking.
 *
 * Cold-boot robustness: we disconnect first to clear any stale SDK
 * state, disable persistent (flash-writing) credentials to avoid
 * sector-write crashes mid-connect, and put a small delay between
 * mode() and begin() so the SDK has time to set up the STA driver.
 * Without those guards the cold-boot WiFi.begin() path triggered
 * Exception (0) IllegalInstruction in the Wi-Fi MAC firmware
 * (~0x40100c1c) on ~5% of boots. */
CSSC_HTTP_API CsscVal cssc_http_wifi_connect(CsscVal ssid, CsscVal password) {
#if defined(CSSC_HTTP_HAS_ESP8266_WIFI) || defined(CSSC_HTTP_HAS_ESP_IDF)
    const char* s = (CSSC_TYPE(ssid) == CSSC_TYPE_STRING && ssid.data.ptr)
                       ? ((CsscString*)ssid.data.ptr)->data : "";
    const char* p = (CSSC_TYPE(password) == CSSC_TYPE_STRING && password.data.ptr)
                       ? ((CsscString*)password.data.ptr)->data : "";
    if (!*s) return cssc_bool(false);
    /* Don't write credentials to flash (default behaviour) — that's
     * a wear hazard and the trigger for the cold-boot crash. */
    WiFi.persistent(false);
    /* Force a clean state. ESP8266 boots with whatever Wi-Fi mode
     * was last in flash; if that's STA and a previous join is still
     * cached, the next begin() can race with the lingering state. */
    WiFi.disconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(s, p);
    /* Wait up to 20 seconds for the join. delay() inside the loop
     * yields to the WDT + Wi-Fi background tasks. */
    for (int i = 0; i < 200; i++) {
        wl_status_t st = WiFi.status();
        if (st == WL_CONNECTED) return cssc_bool(true);
        if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
            /* Hard-fail states — disconnect and power off the radio
             * so background retransmits don't fight with user-code
             * I2C / SPI / display ops. Without this teardown the
             * stuck STA driver kept hammering the SDK and crashed
             * the chip during the next display.show() heap alloc. */
            /* WiFi connect failed. We deliberately do NOT touch the
             * radio further — every disconnect / mode-change / sleep
             * sequence we tried (forceSleepBegin, mode(WIFI_OFF),
             * disconnect(true)) left I2C in a state that hard-WDT'd
             * during the next display.show(). Leaving the SDK to its
             * own bounded retry behaviour is the only path that
             * keeps user code stable after a failed join. The script
             * can call cssc::reboot() if a clean retry is required. */
            return cssc_bool(false);
        }
        delay(100);
    }
    /* Timeout — same teardown reasoning. */
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return cssc_bool(false);
#else
    (void)ssid; (void)password;
    return cssc_bool(true);
#endif
}

/* Set the libc timezone for all subsequent localtime() calls. On
 * Arduino-core ESP8266 this also re-runs the SNTP config so any
 * subsequent ntp_sync uses the new offset. */
CSSC_HTTP_API CsscVal cssc_http_set_timezone(CsscVal offset_seconds) {
    long off = 0;
    if (CSSC_TYPE(offset_seconds) == CSSC_TYPE_INT) off = (long)offset_seconds.data.i;
    else if (CSSC_TYPE(offset_seconds) == CSSC_TYPE_FLOAT) off = (long)offset_seconds.data.f;
#if defined(CSSC_HTTP_HAS_ESP8266_WIFI) || defined(CSSC_HTTP_HAS_ESP_IDF)
    /* configTime treats the first arg as seconds-east-of-UTC; the
     * second is DST offset which we leave at 0 (caller bakes any
     * DST adjustment into the offset). Forwards directly to SNTP
     * configuration even before a server is set — when the script
     * later calls ntp_sync it'll use this offset to compute local
     * time from the synced UTC epoch. */
    configTime(off, 0, "pool.ntp.org");
#else
    /* POSIX: setenv("TZ", "UTC<sign><HH:MM>") then tzset(). */
    char tz[32];
    long abs_off = off < 0 ? -off : off;
    char sign = off < 0 ? '+' : '-';   /* POSIX TZ sign is reversed */
    snprintf(tz, sizeof(tz), "UTC%c%ld:%02ld", sign,
             abs_off / 3600, (abs_off % 3600) / 60);
  #ifdef _WIN32
    _putenv(tz);
  #else
    setenv("TZ", tz, 1);
  #endif
    tzset();
#endif
    return cssc_bool(true);
}

/* SNTP wall-clock sync. Per-target backend: ESP-IDF / Arduino-core /
 * desktop. Returns true once the system clock is set so that
 * subsequent time(NULL) / cssc::detime / etc. report real wall-clock
 * values. False on WiFi-down / DNS-fail / SNTP-timeout — the script
 * can re-call later or fall back to cssc::uptime(). */
CSSC_HTTP_API CsscVal cssc_http_ntp_sync(CsscVal server) {
    const char* srv = (CSSC_TYPE(server) == CSSC_TYPE_STRING && server.data.ptr)
                          ? ((CsscString*)server.data.ptr)->data
                          : "pool.ntp.org";
#if defined(CSSC_HTTP_HAS_ESP_IDF)
    /* ESP-IDF v4+: lwIP SNTP module is in the SDK and was included
     * at file scope above. We poll time(NULL) > a sentinel epoch
     * (~Nov 2023) to detect a successful first sync. */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)srv);
    sntp_init();
    for (int i = 0; i < 100; i++) {
        if ((time_t)time(NULL) > 1700000000) return cssc_bool(true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    return cssc_bool(false);
#elif defined(CSSC_HTTP_HAS_ESP8266_WIFI)
    /* Arduino-core (ESP8266 + ESP32): configTime() runs SNTP via the
     * SDK's built-in client. We pass timezone=0/dst=0 so time(NULL)
     * returns UTC seconds — the user can localise with detime which
     * applies the timezone via standard libc localtime(). */
    configTime(0, 0, srv);
    /* Wait up to 10s for the OS to receive a packet. Without WiFi
     * connected this loop just times out; the script then sees false
     * and can show an offline indicator. */
    for (int i = 0; i < 100; i++) {
        if ((time_t)time(NULL) > 1700000000) return cssc_bool(true);
        delay(100);
    }
    return cssc_bool(false);
#elif defined(_WIN32) || (!defined(CSSC_EMBEDDED))
    /* Desktop: the OS already keeps wall-clock time. We don't push
     * it around — return success immediately so scripts written for
     * embedded "if (ntp_sync) { ... }" gates run on host too. */
    (void)srv;
    return cssc_bool(true);
#else
    /* Bare-metal AVR / no networking: honest false. */
    (void)srv;
    return cssc_bool(false);
#endif
}
