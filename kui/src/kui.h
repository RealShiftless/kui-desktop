#pragma once

#include "kui/kui.h"
#include "webview.h"

extern webview_t* gView;

static char* kui_b64_encode(const unsigned char* data, size_t len);
extern unsigned char* kui_b64_decode(const char* b64, size_t* out_len);
