

#include "cssc_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    uc.dwSchemeLength = 1;
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

    (void)method; (void)url; (void)body; (void)body_len;
    return _http_err(0, "no network stack on this Arduino board");
#elif defined(CSSC_EMBEDDED)

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

CSSC_HTTP_API CsscVal cssc_http_wifi_connect(CsscVal ssid, CsscVal password) {
#if defined(CSSC_HTTP_HAS_ESP8266_WIFI) || defined(CSSC_HTTP_HAS_ESP_IDF)
    const char* s = (CSSC_TYPE(ssid) == CSSC_TYPE_STRING && ssid.data.ptr)
                       ? ((CsscString*)ssid.data.ptr)->data : "";
    const char* p = (CSSC_TYPE(password) == CSSC_TYPE_STRING && password.data.ptr)
                       ? ((CsscString*)password.data.ptr)->data : "";
    if (!*s) return cssc_bool(false);

    WiFi.persistent(false);

    WiFi.disconnect(true);
    delay(50);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(s, p);

    for (int i = 0; i < 200; i++) {
        wl_status_t st = WiFi.status();
        if (st == WL_CONNECTED) return cssc_bool(true);
        if (st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {

            return cssc_bool(false);
        }
        delay(100);
    }

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return cssc_bool(false);
#else
    (void)ssid; (void)password;
    return cssc_bool(true);
#endif
}

CSSC_HTTP_API CsscVal cssc_http_set_timezone(CsscVal offset_seconds) {
    long off = 0;
    if (CSSC_TYPE(offset_seconds) == CSSC_TYPE_INT) off = (long)offset_seconds.data.i;
    else if (CSSC_TYPE(offset_seconds) == CSSC_TYPE_FLOAT) off = (long)offset_seconds.data.f;
#if defined(CSSC_HTTP_HAS_ESP8266_WIFI) || defined(CSSC_HTTP_HAS_ESP_IDF)

    configTime(off, 0, "pool.ntp.org");
#else

    char tz[32];
    long abs_off = off < 0 ? -off : off;
    char sign = off < 0 ? '+' : '-';
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

CSSC_HTTP_API CsscVal cssc_http_ntp_sync(CsscVal server) {
    const char* srv = (CSSC_TYPE(server) == CSSC_TYPE_STRING && server.data.ptr)
                          ? ((CsscString*)server.data.ptr)->data
                          : "pool.ntp.org";
#if defined(CSSC_HTTP_HAS_ESP_IDF)

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*)srv);
    sntp_init();
    for (int i = 0; i < 100; i++) {
        if ((time_t)time(NULL) > 1700000000) return cssc_bool(true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    return cssc_bool(false);
#elif defined(CSSC_HTTP_HAS_ESP8266_WIFI)

    configTime(0, 0, srv);

    for (int i = 0; i < 100; i++) {
        if ((time_t)time(NULL) > 1700000000) return cssc_bool(true);
        delay(100);
    }
    return cssc_bool(false);
#elif defined(_WIN32) || (!defined(CSSC_EMBEDDED))

    (void)srv;
    return cssc_bool(true);
#else

    (void)srv;
    return cssc_bool(false);
#endif
}
