

#ifndef CSSC_ALLOCWATCHER_H
#define CSSC_ALLOCWATCHER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cssc_runtime.h"

#ifdef _WIN32
  #ifdef CSSC_RUNTIME_EXPORTS
    #define CSSC_AW_API __declspec(dllexport)
  #else
    #define CSSC_AW_API __declspec(dllimport)
  #endif
#else
  #define CSSC_AW_API
#endif

CSSC_AW_API void  cssc_alloc_watcher_start(CsscScopeStack* scope,
                                            const char* target_name);

CSSC_AW_API void  cssc_alloc_watcher_stop(void);

CSSC_AW_API CsscVal cssc_alloc_watcher_snapshot(void);

#ifdef __cplusplus
}
#endif

#endif
