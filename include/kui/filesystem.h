#pragma once

#include <stdio.h>

#define FILE_STREAM_CAP 256

typedef struct {
    FILE *fp;
    int   in_use;
} KuiStream;