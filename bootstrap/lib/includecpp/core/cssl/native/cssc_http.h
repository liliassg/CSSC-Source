

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
    int32_t  status;
    char*    body;
    size_t   body_len;
    char*    error;

} CsscHttpResult;

CSSC_HTTP_API CsscHttpResult cssc_http_get(const char* url);
CSSC_HTTP_API CsscHttpResult cssc_http_post(const char* url,
                                             const char* body, size_t body_len);
CSSC_HTTP_API CsscHttpResult cssc_http_request(const char* method,
                                                const char* url,
                                                const char* body, size_t body_len);
CSSC_HTTP_API void           cssc_http_result_free(CsscHttpResult* r);

#include "cssc_runtime.h"
CSSC_HTTP_API CsscVal cssc_http_get_to_val(CsscVal url);
CSSC_HTTP_API CsscVal cssc_http_post_to_val(CsscVal url, CsscVal body);

CSSC_HTTP_API CsscVal cssc_http_wifi_connect(CsscVal ssid, CsscVal password);

CSSC_HTTP_API CsscVal cssc_http_ntp_sync(CsscVal server);

CSSC_HTTP_API CsscVal cssc_http_set_timezone(CsscVal offset_seconds);

#ifdef __cplusplus
}
#endif
#endif
