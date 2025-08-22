#include "filesystem.h"

#include "cjson/cJSON.h"
#include "webview.h"
#include "kui.h"

#include <stdlib.h>

static KuiStream g_streams[256];  // cap open files

static int alloc_handle(void) {
    for (int i = 1; i < 256; i++) {
        if (!g_streams[i].in_use) {
            g_streams[i].in_use = 1;
            return i;
        }
    }
    return 0; // 0=invalid
}

static void free_handle(int h) {
    if (h > 0 && h < 256) {
        g_streams[h].in_use = 0;
        g_streams[h].fp = NULL;
    }
}

static FILE* get_fp(int h) {
    return (h > 0 && h < 256 && g_streams[h].in_use) ? g_streams[h].fp : NULL;
}

// Normalize modes ("r","rb","w","wb", ...)
static const char* normalize_mode(const char* m) {
    return m && strstr(m, "b") ? m : m;
}

// Base64 helpers you already have (or reuse your existing ones)
//extern char* kui_b64_encode(const unsigned char* data, size_t len);

// ---- RPC: __fs_open({ path, mode }) -> { handle } ----
static void __fs_open_cb(const char* seq, const char* req, void* user) {
    cJSON* j = cJSON_Parse(req);
    const cJSON* jarg = j ? cJSON_GetArrayItem(j, 0) : NULL;
    const char* path = jarg ? cJSON_GetObjectItemCaseSensitive(jarg, "path")->valuestring : NULL;
    const char* mode = jarg ? cJSON_GetObjectItemCaseSensitive(jarg, "mode")->valuestring : "rb";

    if (!path) {
        webview_return(gView, seq, 1, "{\"error\":\"missing path\"}");
        goto done;
    }

    // TODO: security: restrict path to app dir or require a file dialog result
    FILE* fp = fopen(path, normalize_mode(mode));
    if (!fp) {
        webview_return(gView, seq, 1, "{\"error\":\"open failed\"}");
        goto done;
    }

    int h = alloc_handle();
    if (!h) {
        fclose(fp);
        webview_return(gView, seq, 1, "{\"error\":\"too many open files\"}");
        goto done;
    }
    g_streams[h].fp = fp;

    char buf[64];
    snprintf(buf, sizeof buf, "{\"handle\":%d}", h);
    webview_return(gView, seq, 0, buf);

done:
    if (j) cJSON_Delete(j);
}

// ---- __fs_read({ handle, max }) -> { b64 }  (chunked) ----
static void __fs_read_cb(const char* seq, const char* req, void* user) {
    cJSON* j = cJSON_Parse(req);
    const cJSON* a = j ? cJSON_GetArrayItem(j, 0) : NULL;
    int h = a ? cJSON_GetObjectItemCaseSensitive(a, "handle")->valueint : 0;
    size_t max = a && cJSON_GetObjectItemCaseSensitive(a, "max")
        ? (size_t)cJSON_GetObjectItemCaseSensitive(a, "max")->valuedouble
        : 64 * 1024;

    FILE* fp = get_fp(h);
    if (!fp) {
        webview_return(gView, seq, 1, "{\"error\":\"bad handle\"}");
        goto done;
    }

    if (max > 4 * 1024 * 1024) max = 4 * 1024 * 1024; // safety cap
    unsigned char* buf = (unsigned char*)malloc(max);
    size_t n = buf ? fread(buf, 1, max, fp) : 0;

    if (!buf) {
        webview_return(gView, seq, 1, "{\"error\":\"oom\"}");
        goto done;
    }

    char* b64 = kui_b64_encode(buf, n);
    free(buf);
    if (!b64) {
        webview_return(gView, seq, 1, "{\"error\":\"b64 fail\"}");
        goto done;
    }

    cJSON* out = cJSON_CreateObject();
    cJSON_AddStringToObject(out, "b64", b64);
    cJSON_AddNumberToObject(out, "n", (double)n);
    char* s = cJSON_PrintUnformatted(out);
    webview_return(gView, seq, 0, s);

    cJSON_free(s);
    cJSON_Delete(out);
    free(b64);

done:
    if (j) cJSON_Delete(j);
}

// ---- __fs_write({ handle, b64 }) -> { n } ----
static void __fs_write_cb(const char* seq, const char* req, void* user) {
    cJSON* j = cJSON_Parse(req);
    const cJSON* a = j ? cJSON_GetArrayItem(j, 0) : NULL;
    int h = a ? cJSON_GetObjectItemCaseSensitive(a, "handle")->valueint : 0;
    const char* b64 = a ? cJSON_GetObjectItemCaseSensitive(a, "b64")->valuestring : NULL;

    FILE* fp = get_fp(h);
    if (!fp || !b64) {
        webview_return(gView, seq, 1, "{\"error\":\"args\"}");
        goto done;
    }

    size_t len = 0;
    unsigned char* bytes = kui_b64_decode(b64, &len);
    if (!bytes) {
        webview_return(gView, seq, 1, "{\"error\":\"b64 decode\"}");
        goto done;
    }

    size_t n = fwrite(bytes, 1, len, fp);
    free(bytes);

    char buf[48];
    snprintf(buf, sizeof buf, "{\"n\":%zu}", n);
    webview_return(gView, seq, 0, buf);

done:
    if (j) cJSON_Delete(j);
}

// ---- __fs_seek({ handle, offset, whence }) -> { pos } ----
// Use 64-bit offsets for big files: fseeko/ftello on POSIX, _fseeki64/_ftelli64 on Windows
static long long tell64(FILE* fp) {
#if defined(_WIN32)
    return _ftelli64(fp);
#else
    return (long long)ftello(fp);
#endif
}

static int seek64(FILE* fp, long long off, int wh) {
#if defined(_WIN32)
    return _fseeki64(fp, off, wh);
#else
    return fseeko(fp, (off_t)off, wh);
#endif
}

static void __fs_seek_cb(const char* seq, const char* req, void* user) {
    cJSON* j = cJSON_Parse(req);
    const cJSON* a = j ? cJSON_GetArrayItem(j, 0) : NULL;
    int h = a ? cJSON_GetObjectItemCaseSensitive(a, "handle")->valueint : 0;
    long long off = a ? (long long)cJSON_GetObjectItemCaseSensitive(a, "offset")->valuedouble : 0;
    int wh = a ? (int)cJSON_GetObjectItemCaseSensitive(a, "whence")->valueint : 0;

    FILE* fp = get_fp(h);
    if (!fp || seek64(fp, off, wh)) {
        webview_return(gView, seq, 1, "{\"error\":\"seek\"}");
        goto done;
    }

    long long pos = tell64(fp);
    char out[64];
    snprintf(out, sizeof out, "{\"pos\":%lld}", pos);
    webview_return(gView, seq, 0, out);

done:
    if (j) cJSON_Delete(j);
}

// ---- __fs_close({ handle }) ----
static void __fs_close_cb(const char* seq, const char* req, void* user) {
    cJSON* j = cJSON_Parse(req);
    const cJSON* a = j ? cJSON_GetArrayItem(j, 0) : NULL;
    int h = a ? cJSON_GetObjectItemCaseSensitive(a, "handle")->valueint : 0;

    FILE* fp = get_fp(h);
    if (!fp) {
        webview_return(gView, seq, 1, "{\"error\":\"bad handle\"}");
        goto done;
    }

    fclose(fp);
    free_handle(h);
    webview_return(gView, seq, 0, "{\"ok\":true}");

done:
    if (j) cJSON_Delete(j);
}

void fs_init() {
    webview_bind(gView, "__fs_open",  __fs_open_cb,  NULL);
    webview_bind(gView, "__fs_read",  __fs_read_cb,  NULL);
    webview_bind(gView, "__fs_write", __fs_write_cb, NULL);
    webview_bind(gView, "__fs_seek",  __fs_seek_cb,  NULL);
    webview_bind(gView, "__fs_close", __fs_close_cb, NULL);
}
