#include "kui/kui.h"
#include "webview.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cjson/cJSON.h"

#define DEFAULT_TITLE  "KUI Window"
#define DEFAULT_WIDTH  900
#define DEFAULT_HEIGHT 600

#define KUI_VERSION_PREFIX "dev"
#define KUI_VERSION_MAJOR 1
#define KUI_VERSION_MINOR 1
#define KUI_VERSION_PATCH 1

#define PAGE_SHELL_SIZE 96

static webview_t gView   = NULL;
static KuiState  gState  = KUI_STATE_NONE;

static const KuiResource* gPrelude;

KuiState kui_get_state(void) { return gState; }


/* ----- Helper ----- */
static void set_html(webview_t view,
                               const unsigned char *data, size_t len) {
    if (!view || !data || !len) return;
    char *buf = (char*)malloc(len + 1);
    if (!buf) return;
    memcpy(buf, data, len);
    buf[len] = '\0';
    webview_set_html(view, buf);
    free(buf);
}

static void js_eval(webview_t *view, const unsigned char *data, size_t len) {
    if (view == NULL || data == NULL || len == 0) return;

    char *buf = (char*)malloc(len + 1);
    if (!buf) return;

    memcpy(buf, data, len);
    buf[len] = '\0';

    webview_eval(view, buf);

    free(buf);
}


static void reply_ok(const char* seq, const char* json) {
    // status=0 => success in webview
    webview_return(gView, seq, 0, json);
}
static void reply_err(const char* seq, const char* msg) {
    // basic JSON string escaper for quotes/backslashes
    char buf[512];
    size_t k = 0;
    buf[k++] = '{'; buf[k++] = '\"'; buf[k++] = 'e'; buf[k++] = 'r'; buf[k++] = 'r';
    buf[k++] = 'o'; buf[k++] = 'r'; buf[k++] = '\"'; buf[k++] = ':'; buf[k++] = '\"';
    for (const char* p = msg; *p && k + 2 < sizeof(buf); ++p) {
        if (*p == '\"' || *p == '\\') buf[k++]='\\';
        buf[k++] = *p;
    }
    buf[k++] = '\"'; buf[k++] = '}';
    buf[k] = '\0';
    webview_return(gView, seq, 1, buf); // status!=0 => error
}

static char* kui_b64_encode(const unsigned char* data, size_t len) {
    static const char* TBL =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_len = 4 * ((len + 2) / 3);
    char* out = (char*)malloc(out_len + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        unsigned octet_a = i < len ? data[i++] : 0;
        unsigned octet_b = i < len ? data[i++] : 0;
        unsigned octet_c = i < len ? data[i++] : 0;

        unsigned triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        out[j++] = TBL[(triple >> 18) & 0x3F];
        out[j++] = TBL[(triple >> 12) & 0x3F];
        out[j++] = (i > len + 1) ? '=' : TBL[(triple >> 6) & 0x3F];
        out[j++] = (i > len)     ? '=' : TBL[triple & 0x3F];
    }

    out[out_len] = '\0';
    return out;
}


/* ----- JS Callbacks ----- */
static void __kui_version_cb(const char* seq, const char* req, void* user) {
    (void)req; (void)user;

    const char* pre  = KUI_VERSION_PREFIX;
    const char* dash = (pre && pre[0]) ? "-" : "";

    char json[64];
    snprintf(json, sizeof json,
             "{\"version\":\"%s%s%d.%d.%d\"}",
             pre, dash, KUI_VERSION_MAJOR, KUI_VERSION_MINOR, KUI_VERSION_PATCH);

    webview_return(gView, seq, 0, json);
}

static void __kui_resolve_cb(const char* seq, const char* req, void* user) {
    (void)user;
    cJSON* root = cJSON_Parse(req);
    if (!root) {
        webview_return(gView, seq, 1, "{\"error\":\"bad json\"}");
        return;
    }

    cJSON* arg0 = cJSON_GetArrayItem(root, 0);
    if (!cJSON_IsObject(arg0)) {
        cJSON_Delete(root);
        webview_return(gView, seq, 1, "{\"error\":\"bad args\"}");
        return;
    }

    const cJSON* url_node = cJSON_GetObjectItemCaseSensitive(arg0, "url");
    const char* url = cJSON_GetStringValue(url_node);
    if (!url) {
        cJSON_Delete(root);
        webview_return(gView, seq, 1, "{\"error\":\"missing url\"}");
        return;
    }

    const char* path = url;
    if (!strncasecmp(url, "kui:/", 5)) path = url + 5;

    const KuiResource* r = kui_resource_find(path);
    if (!r) {
        cJSON_Delete(root);
        webview_return(gView, seq, 1, "{\"error\":\"not found\"}");
        return;
    }

    char* b64 = kui_b64_encode(r->data, r->size);
    if (!b64) {
        cJSON_Delete(root);
        webview_return(gView, seq, 1, "{\"error\":\"oom\"}");
        return;
    }

    cJSON* resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "mime", kui_resource_mime(path));
    cJSON_AddStringToObject(resp, "b64", b64);

    char* out = cJSON_PrintUnformatted(resp);
    webview_return(gView, seq, 0, out);

    cJSON_free(out);
    cJSON_Delete(resp);
    cJSON_Delete(root);
    free(b64);
}


/* ----- API Func ----- */
KuiResult kui_init(KuiArgs args) {
    if (gState != KUI_STATE_NONE) return KUI_ALREADY_INITIALIZED;

    const char* title = args.title ? args.title : DEFAULT_TITLE;
    int w = (args.width  > 0) ? args.width  : DEFAULT_WIDTH;
    int h = (args.height > 0) ? args.height : DEFAULT_HEIGHT;
    int debug = (args.debug > 0 ? args.debug : 0);

    gView = webview_create(debug, NULL);
    if (!gView) return KUI_WEBVIEW_FAILED;

    
    webview_set_title(gView, title);
    webview_set_size(gView, w, h, WEBVIEW_HINT_NONE);

    // Bind native fns first (they persist across navigations)
    webview_bind(gView, "__kui_version", __kui_version_cb, NULL);
    webview_bind(gView, "__kui_resolve", __kui_resolve_cb, NULL);

    // Get the prelude
    gPrelude = kui_resource_find("js/prelude.js");
    
    // Load the default page
    const KuiResource* default_page = kui_resource_find("html/getting_started.html");
    kui_set_page(default_page);

    gState = KUI_STATE_INITIALIZED;
    return KUI_OK;
}

KuiResult kui_run(void) {
    if (gState == KUI_STATE_NONE)     return KUI_NOT_INITIALIZED;
    if (gState == KUI_STATE_RUNNING)  return KUI_ALREADY_RUNNING;
    if (gState == KUI_STATE_DISPOSED) return KUI_DISPOSED;
    if (!gView)                       return KUI_NOT_INITIALIZED;

    gState = KUI_STATE_RUNNING;
    webview_run(gView);

    webview_destroy(gView);
    gView  = NULL;
    gState = KUI_STATE_DISPOSED;
    return KUI_OK;
}

static char* esc_script_n(const char* js, size_t n, size_t* out_len) {
    size_t extra = 0;
    for (size_t i = 0; i + 8 <= n; ++i) {
        if (js[i]=='<' && js[i+1]=='/' &&
            ((js[i+2]|32)=='s') && ((js[i+3]|32)=='c') &&
            ((js[i+4]|32)=='r') && ((js[i+5]|32)=='i') &&
            ((js[i+6]|32)=='p') && ((js[i+7]|32)=='t')) {
            extra += 1; // for '\' in <\/script>
        }
    }
    char* out = (char*)malloc(n + extra + 1);
    if (!out) return NULL;
    char* p = out;
    for (size_t i = 0; i < n; ) {
        if (i + 8 <= n && js[i]=='<' && js[i+1]=='/' &&
            ((js[i+2]|32)=='s') && ((js[i+3]|32)=='c') &&
            ((js[i+4]|32)=='r') && ((js[i+5]|32)=='i') &&
            ((js[i+6]|32)=='p') && ((js[i+7]|32)=='t')) {
            *p++ = '<'; *p++ = '\\'; *p++ = '/';  // <\/
            i += 2;                               // keep copying "script..."
            continue;
        }
        *p++ = js[i++];
    }
    *p = '\0';
    if (out_len) *out_len = (size_t)(p - out);
    return out;
}

static char* build_page_n(const char* body, size_t body_len,
                          const char* prelude, size_t prelude_len) {
    size_t esc_len = 0;
    char* esc = esc_script_n(prelude ? prelude : "", prelude ? prelude_len : 0, &esc_len);
    if (!esc && (prelude_len != 0)) return NULL;

    const char* head = "<!doctype html><html><head><meta charset=\"utf-8\"><script>";
    const char* mid  = "</script></head><body>";
    const char* tail = "</body></html>";

    size_t total = strlen(head) + esc_len + strlen(mid) + body_len + strlen(tail) + 1;
    char* buf = (char*)malloc(total);
    if (!buf) { free(esc); return NULL; }

    char* p = buf;
    memcpy(p, head, strlen(head)); p += strlen(head);
    if (esc_len) { memcpy(p, esc, esc_len); p += esc_len; }
    memcpy(p, mid, strlen(mid)); p += strlen(mid);
    if (body_len) { memcpy(p, body, body_len); p += body_len; }
    memcpy(p, tail, strlen(tail)); p += strlen(tail);
    *p = '\0';

    free(esc);
    return buf;
}

KuiResult kui_set_page(const KuiResource* page) {
    if (!page) return KUI_INVALID_RESOURCE;

    set_html(gView, page->data, page->size);
    js_eval(gView, gPrelude->data, gPrelude->size);   // ensure this copies; if not, keep html alive appropriately
    return KUI_OK;
}

