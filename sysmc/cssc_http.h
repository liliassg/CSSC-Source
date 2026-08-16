/* cssc_http.h — HTTP/HTTPS client ABI for native CSSC builds.
 *
 * One API across:
 *   CSSC_ESP32     — ESP-HTTP-Client (esp_http_client.h)
 *   CSSC_ESP8266   — ESP8266HTTPClient (Arduino-core)
 *   CSSC_ARDUINO   — bare-metal AVR has no networking; stub returns
 *                    error 0 / "no network on this board"
 *   CSSC_LINUX     — libcurl
 *   _WIN32 host    — WinHTTP
 *
 * Every call returns a CsscHttpResult with the same shape so the
 * generated code path doesn't need to branch per platform.
 */

#ifndef CSSC_HTTP_H
#define CSSC_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
  #ifdef CSSC_RUNTIME_EXPORTS
    #define CSSC_HTTP_API __declspec(dllexport)
  #else
    #define CSSC_HTTP_API __declspec(dllimport)
  #endif
#else
  #define CSSC_HTTP_API
#endif

typedef struct {
    int32_t  status;        /* HTTP code; 0 if request errored before reaching server */
    char*    body;          /* heap-allocated, free via cssc_http_result_free */
    size_t   body_len;
    char*    error;         /* "" on success; heap-allocated message on failure */
    /* Headers omitted in the C ABI surface — too costly per-call on
     * embedded. The interpreter still exposes them. */
} CsscHttpResult;

CSSC_HTTP_API CsscHttpResult cssc_http_get(const char* url);
CSSC_HTTP_API CsscHttpResult cssc_http_post(const char* url,
                                             const char* body, size_t body_len);
CSSC_HTTP_API CsscHttpResult cssc_http_request(const char* method,
                                                const char* url,
                                                const char* body, size_t body_len);
CSSC_HTTP_API void           cssc_http_result_free(CsscHttpResult* r);

/* Convenience wrappers used by the codegen for `#get` / `#post`:
 * perform the request, materialise a CsscVal map matching the
 * interpreter's response shape ({status, body, ok, error, headers=null}),
 * and free the underlying CsscHttpResult before returning. */
#include "cssc_runtime.h"
CSSC_HTTP_API CsscVal cssc_http_get_to_val(CsscVal url);
CSSC_HTTP_API CsscVal cssc_http_post_to_val(CsscVal url, CsscVal body);

/* WiFi STA connect. Blocks up to ~20 seconds for the join, returns
 * true on success / false on timeout. Only needed on embedded —
 * desktop builds always return true (the OS handles networking). */
CSSC_HTTP_API CsscVal cssc_http_wifi_connect(CsscVal ssid, CsscVal password);

/* SNTP wall-clock sync. `server` is a CsscVal string (e.g.
 * "pool.ntp.org"). Returns a bool: true on success (system clock
 * is now real wall-time, all time builtins return live values),
 * false if WiFi isn't connected / DNS failed / SNTP timed out.
 *
 * On Arduino-core (ESP8266/ESP32) this calls configTime() which
 * uses the SDK's built-in SNTP client — no extra UDP code in our
 * binary. On Win32/Linux this is a no-op-success because the OS
 * already maintains wall-clock time independently. */
CSSC_HTTP_API CsscVal cssc_http_ntp_sync(CsscVal server);

/* Set the timezone offset from UTC in seconds, applied to all
 * subsequent localtime() calls (date/datetime/detime/sdetime).
 * Examples: Berlin winter +3600, Berlin summer +7200, UTC 0,
 * New York winter -18000. */
CSSC_HTTP_API CsscVal cssc_http_set_timezone(CsscVal offset_seconds);

#ifdef __cplusplus
}
#endif
#endif
