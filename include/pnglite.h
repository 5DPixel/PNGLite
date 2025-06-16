/*
 * MIT License

 Copyright (c) 2025 Jude Routledge

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#ifndef PNGLITE_H
#define PNGLITE_H

#define PNGLITE_VERSION 1

enum pnglite_format {
    PNGLITE_default,
    PNGLITE_rgba,
};

typedef unsigned char pnglite_uc;
typedef unsigned short pnglite_us;
typedef unsigned int pnglite_uint32;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef PNG_BUILD_DLL
    #ifdef __GNUC__
      #define PNGLITE_API __attribute__ ((dllexport))
    #else
      #define PNGLITE_API __declspec(dllexport)
    #endif
  #elif defined(PNGLITE_DLL)
    #ifdef __GNUC__
      #define PNGLITE_API __attribute__ ((dllimport))
    #else
      #define PNGLITE_API __declspec(dllimport)
    #endif
  #else
    #define PNGLITE_API
  #endif
#else
  #if __GNUC__ >= 4
    #define PNGLITE_API __attribute__ ((visibility ("default")))
  #else
    #define PNGLITE_API
  #endif
#endif

#ifdef PNGLITE_STATIC
  #define PNGLITE_API
#endif

#ifdef PNGLITE_STATIC
#define PNGLITEDEF static
#else
#define PNGLITEDEF PNGLITE_API extern
#endif

#ifndef PNGLITE_NO_STDIO
#include <stdio.h>
#endif

#ifndef PNGLITE_ASSERT
#include <assert.h>
#define PNGLITE_ASSERT(x) assert(x)
#endif

#ifndef PNGLITE_MALLOC
#include <stdlib.h>
#define PNGLITE_MALLOC(size) malloc(size)
#define PNGLITE_REALLOC(ptr, size) realloc(ptr, size)
#define PNGLITE_FREE(ptr) free(ptr)
#endif

#ifndef _MSC_VER
   #ifdef __cplusplus
   #define pnglite_inline inline
   #else
   #define pnglite_inline
   #endif
#else
   #define pnglite_inline __forceinline
#endif

#include <stddef.h>
#include <string.h>

typedef struct pnglite_io_ctx {
    int      (*read)  (void *user,char *data,int size);
    void     (*skip)  (void *user,int n);
    int      (*eof)   (void *user);
} pnglite_io_ctx_t;

typedef struct pnglite_ctx {
    pnglite_io_ctx_t io_ctx;

    pnglite_uc* image_data;
    pnglite_uc* image_data_end;
} pnglite_ctx_t;

typedef struct pnglite__result_info {
    pnglite_uint32 bits_per_channel;
    pnglite_uint32 num_channels;
    pnglite_uint32 channel_order;
} pnglite__result_info_t;

typedef struct pnglite__chunk {
    pnglite_uint32 length;
    pnglite_uint32 type;
} pnglite__chunk_t;

typedef struct pnglite__png {
    pnglite_ctx_t *ctx;
    pnglite_uc *idata, *expanded, *out;
    pnglite_uint32 depth;
} pnglite__png_t;

enum {
    PNGLITE_ORDER_RGB,
    PNGLITE_ORDER_BGR,
};

static const char *pnglite__failure_reason;

static void pnglite__ctx_init(pnglite_ctx_t *ctx, pnglite_uc *buffer, pnglite_uint32 len);
static pnglite__chunk_t pnglite__get_chunk_header(pnglite_ctx_t *ctx);
static int pnglite__check_png_header(pnglite_ctx_t *ctx);
static int pnglite__err(const char *reason);
static void pnglite__load_main(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite__result_info_t *result_info);

pnglite_inline static pnglite_uc pnglite__get8(pnglite_ctx_t *ctx);

PNGLITEDEF pnglite_uc* pnglite_load_from_memory(pnglite_uc* buffer, pnglite_uint32 len, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components);

#ifdef __cplusplus
}
#endif

#endif
