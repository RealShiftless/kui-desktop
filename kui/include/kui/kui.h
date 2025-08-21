#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "kui/resource.h"
#include "kui/api.h"

typedef enum {
    KUI_OK = 0,
    KUI_WEBVIEW_FAILED,
    KUI_ALREADY_INITIALIZED,
    KUI_ALREADY_RUNNING,
    KUI_NOT_INITIALIZED,
    KUI_DISPOSED,
    KUI_UNHANDLED_SCOPE,
    KUI_INVALID_RESOURCE,
    KUI_OOM
} KuiResult;

typedef enum {
    KUI_STATE_NONE = 0,
    KUI_STATE_INITIALIZED,
    KUI_STATE_RUNNING,
    KUI_STATE_DISPOSED
} KuiState;

typedef struct {
    const char* title;  // optional
    int width;          // <=0 -> default
    int height;         // <=0 -> default
} KuiArgs;

typedef enum {
    KUI_SCOPE_INTERNAL
    //KUI_SCOPE_USER  -- Commented out for now as i don't support user resources, yet
} KuiScope;

KUI_API KuiState  kui_get_state(void);
KUI_API KuiResult kui_init(KuiArgs args);
KUI_API KuiResult kui_run(void);

KUI_API KuiResult kui_set_page(const KuiResource* resource);

#ifdef __cplusplus
}
#endif