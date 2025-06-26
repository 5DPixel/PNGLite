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

#include <stdint.h>
#include <string.h>

typedef unsigned char pnglite_uc;
typedef unsigned short pnglite_us;
typedef uint32_t pnglite_uint32;

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

#define PNGLITE__CHUNK_TYPE(a,b,c,d)  (((unsigned) (a) << 24) + ((unsigned) (b) << 16) + ((unsigned) (c) << 8) + (unsigned) (d))

#define PNGLITE__ZFAST_BITS 9
#define PNGLITE__ZFAST_MASK ((1 << PNGLITE__ZFAST_BITS) - 1)

#define PNGLITE__BYTECAST(x) ((pnglite_uc)((x) & 255))

#include <stddef.h>

typedef struct pnglite_ctx {
    pnglite_uc* image_data;
    pnglite_uc* image_data_start;
    pnglite_uc* image_data_end;

    pnglite_uc* image_data_original;
    pnglite_uc* image_data_original_end;

    pnglite_uint32 image_x, image_y, image_n;
    int image_out_n;
    pnglite_uint32 buflen;
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

enum
{
    PNGLITE__SCAN_load = 0,
    PNGLITE__SCAN_type,
    PNGLITE__SCAN_header
};

enum {
    PNGLITE_FILTER_none = 0,
    PNGLITE_FILTER_sub = 1,
    PNGLITE_FILTER_up = 2,
    PNGLITE_FILTER_avg = 3,
    PNGLITE_FILTER_paeth = 4,
    PNGLITE_FILTER_avg_first,
    PNGLITE_FILTER_paeth_first
};

static pnglite_uc pnglite__first_row_filter[5] = {
    PNGLITE_FILTER_none,
    PNGLITE_FILTER_sub,
    PNGLITE_FILTER_none,
    PNGLITE_FILTER_avg_first,
    PNGLITE_FILTER_paeth_first
};

typedef struct pnglite__zhuffman {
    pnglite_us fast[1 << PNGLITE__ZFAST_BITS];
    pnglite_us firstcode[16];
    int maxcode[17];
    pnglite_us firstsymbol[16];
    pnglite_uc size[288];
    pnglite_us value[288];
} pnglite__zhuffman_t;

typedef struct pnglite__zbuf {
    pnglite_uc *zbuffer, *zbuffer_end;
    pnglite_uint32 num_bits;
    pnglite_uint32 code_buffer;

    char *zout;
    char *zout_start;
    char *zout_end;
    int z_expandable;

    pnglite__zhuffman_t z_length, z_distance;
} pnglite__zbuf_t;

static const char *pnglite__failure_reason;

static void pnglite__ctx_init(pnglite_ctx_t *ctx, pnglite_uc *buffer, pnglite_uint32 len);
static void pnglite__refill_buffer(pnglite_ctx_t *ctx);
static void pnglite__skip(pnglite_ctx_t *ctx, pnglite_uint32 n);
static pnglite__chunk_t pnglite__get_chunk_header(pnglite_ctx_t *ctx);
static int pnglite__check_png_header(pnglite_ctx_t *ctx);
static int pnglite__err(const char *reason);
static void* pnglite__load_main(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components, pnglite__result_info_t *result_info);
static void *pnglite__load_png(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components, pnglite__result_info_t *result_info);
static int pnglite__parse_png(pnglite__png_t *png, pnglite_uint32 scan, pnglite_uint32 requested_components);
static void pnglite__fill_bits(pnglite__zbuf_t *z);
static int pnglite__parse_zlib_header(pnglite__zbuf_t *a);
static int pnglite__zexpand(pnglite__zbuf_t *z, char *zout, pnglite_uint32 n);
static int pnglite__parse_uncompressed_block(pnglite__zbuf_t *z);
static int pnglite__zbuild_huffman(pnglite__zhuffman_t *z, pnglite_uc *sizelist, pnglite_uint32 num);
static int pnglite__zhuffman_decode_slowpath(pnglite__zbuf_t *a, pnglite__zhuffman_t *z);
static int pnglite__zhuffman_decode(pnglite__zbuf_t *a, pnglite__zhuffman_t *z);
static int pnglite__compute_huffman_codes(pnglite__zbuf_t *a);
static int pnglite__parse_huffman_block(pnglite__zbuf_t *a);
static int pnglite__parse_zlib(pnglite__zbuf_t *a, int parse_header);
static int pnglite__do_zlib(pnglite__zbuf_t *a, char *obuf, int olen, int exp, int parse_header);
static int pnglite__create_png_image_raw(pnglite__png_t *png, pnglite_uc *raw, pnglite_uint32 raw_len, int out_n, pnglite_uint32 x, pnglite_uint32 y, int depth, int color);
static int pnglite__create_png_image(pnglite__png_t *png, pnglite_uc *image_data, pnglite_uint32 image_data_len, int out_n, int depth, int color);
static void *pnglite__do_png(pnglite__png_t *png, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *n, pnglite_uint32 requested_components, pnglite__result_info_t *result_info);
static pnglite_uc *pnglite__convert_format(pnglite_uc *data, pnglite_uint32 img_n, pnglite_uint32 requested_components, pnglite_uint32 x, pnglite_uint32 y);
static int pnglite__getn(pnglite_ctx_t *ctx, pnglite_uc *buffer, pnglite_uint32 n);
static pnglite_uc pnglite__compute_y(int r, int g, int b);
static int pnglite__paeth(int a, int b, int c);
static pnglite_us *pnglite__load_16bit(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components);
static pnglite_us *pnglite__convert_8_to_16(pnglite_uc *original, pnglite_uint32 width, pnglite_uint32 height, pnglite_uint32 channels);
static int pnglite__compute_transparency(pnglite__png_t *png, pnglite_uc tc[3], pnglite_uint32 out_n);
static int pnglite__compute_transparency16(pnglite__png_t *png, pnglite_us tc[3], pnglite_uint32 out_n);
static int pnglite__expand_palette(pnglite__png_t *png, pnglite_uc *palette, pnglite_uint32 len, pnglite_uint32 pal_image_n);
static pnglite_us *pnglite__convert_format16(pnglite_us *data, pnglite_uint32 img_n, pnglite_uint32 requested_components, pnglite_uint32 x, pnglite_uint32 y);
static pnglite_us pnglite__compute_y16(int r, int g, int b);

pnglite_inline static pnglite_uc *pnglite__load_8bit(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components);
pnglite_inline static pnglite_uc pnglite__get8(pnglite_ctx_t *ctx);
pnglite_inline static pnglite_uc pnglite__zget8(pnglite__zbuf_t *z);
pnglite_inline static pnglite_uint32 pnglite__zreceive(pnglite__zbuf_t *z, int n);
pnglite_inline static int pnglite__bitreverse16(int n);
pnglite_inline static int pnglite__bit_reverse(int v, int bits);
static pnglite_us pnglite__get16be(pnglite_ctx_t *ctx);
static pnglite_uint32 pnglite__get32be(pnglite_ctx_t *ctx);

PNGLITEDEF pnglite_uc* pnglite_load_from_memory(pnglite_uc* buffer, pnglite_uint32 len, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components);
PNGLITEDEF pnglite_uc *pnglite_load(const char *filename, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components);
PNGLITEDEF pnglite_uc *pnglite_load_from_file(FILE *f, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components);
PNGLITEDEF char *pnglite_zlib_decode_malloc_guessize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
PNGLITEDEF pnglite_us *pnglite_load_16_from_memory(pnglite_uc *buffer, pnglite_uint32 len, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components);

#ifdef __cplusplus
}
#endif

#endif
