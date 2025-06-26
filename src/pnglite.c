#include <pnglite.h>

static void pnglite__ctx_init(pnglite_ctx_t *ctx, pnglite_uc *buffer, pnglite_uint32 len){
    ctx->image_data = ctx->image_data_original = buffer;
    ctx->image_data_end = ctx->image_data_original_end = (pnglite_uc*)buffer + len;
}

static pnglite__chunk_t pnglite__get_chunk_header(pnglite_ctx_t *ctx){
    pnglite__chunk_t chunk;

    chunk.length = pnglite__get32be(ctx);
    chunk.type = pnglite__get32be(ctx);

    return chunk;
}

static int pnglite__check_png_header(pnglite_ctx_t* ctx){
    static const pnglite_uc png_signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    for(pnglite_uint32 i = 0; i < 8; i++){
        if (pnglite__get8(ctx) != png_signature[i]) return pnglite__err("incorrect png signature!");
    }

    return 1;
}

static int pnglite__err(const char* reason){
    pnglite__failure_reason = reason;

    #ifndef PNGLITE_NO_STDIO
    fprintf(stderr, "%s", reason);
    #endif

    return 0;
}

static void* pnglite__load_main(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components, pnglite__result_info_t *result_info){
    memset(result_info, 0, sizeof(*result_info));
    result_info->bits_per_channel = 8;
    result_info->channel_order = PNGLITE_ORDER_RGB;
    result_info->num_channels = 0;

    return pnglite__load_png(ctx, x, y, components, requested_components, result_info);
}

static void *pnglite__load_png(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components, pnglite__result_info_t *result_info){
    pnglite__png_t png;
    png.ctx = ctx;
    return pnglite__do_png(&png, x, y, components, requested_components, result_info);
}

static pnglite_uc pnglite__get8(pnglite_ctx_t *ctx) {
    if(ctx->image_data >= ctx->image_data_end) {
        return 0;
    }
    return *ctx->image_data++;
}

static void pnglite__refill_buffer(pnglite_ctx_t *ctx){
    ctx->image_data = ctx->image_data_start;
    ctx->image_data_end = ctx->image_data_start + ctx->buflen;
}

static pnglite_us pnglite__get16be(pnglite_ctx_t *ctx){
    pnglite_uc uc = pnglite__get8(ctx);
    return ((pnglite_us)uc << 8) + pnglite__get8(ctx);
}

static pnglite_uint32 pnglite__get32be(pnglite_ctx_t *ctx){
    pnglite_us us = pnglite__get16be(ctx);
    return ((pnglite_uint32)us << 16) + pnglite__get16be(ctx);
}

static pnglite_us pnglite__get16le(pnglite_ctx_t *ctx){
    pnglite_uc uc = pnglite__get8(ctx);
    return (pnglite_us)uc + (pnglite_us)(pnglite__get8(ctx) << 8);
}

static pnglite_uint32 pnglite__get32le(pnglite_ctx_t *ctx){
    pnglite_us us = pnglite__get16le(ctx);

    return (pnglite_uint32)us + (pnglite_uint32)(pnglite__get16le(ctx) << 16);
}

pnglite_inline static pnglite_uc *pnglite__load_8bit(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    pnglite__result_info_t result_info;
    void *result = pnglite__load_main(ctx, x, y, components, requested_components, &result_info);

    if(result == NULL) return NULL;

    return (pnglite_uc*)result;
}

static void pnglite__skip(pnglite_ctx_t *ctx, pnglite_uint32 n){
    if(n < 0){
        ctx->image_data = ctx->image_data_end;
        return;
    }
    pnglite_uint32 blen = ctx->image_data_end - ctx->image_data;
    if(blen < n){
        ctx->image_data = ctx->image_data_end;
        return;
    }

    ctx->image_data += n;
}

static int pnglite__compute_transparency16(pnglite__png_t *png, pnglite_us tc[3], pnglite_uint32 out_n){
    pnglite_ctx_t *ctx = png->ctx;
    pnglite_uint32 i, pixel_count = ctx->image_x * ctx->image_y;
    pnglite_us *p = (pnglite_us*)png->out;

    if(out_n == 2){
        for(i = 0; i < pixel_count; ++i){
            p[1] = (p[0] == tc[0] ? 0 : 65535);
            p += 2;
        }
    } else {
        for(i = 0; i < pixel_count; ++i){
            if(p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2]){
                p[3] = 0;
            }
            p += 4;
        }
    }

    return 1;
}

static int pnglite__compute_transparency(pnglite__png_t *png, pnglite_uc tc[3], pnglite_uint32 out_n){
    pnglite_ctx_t *ctx = png->ctx;
    pnglite_uint32 i, pixel_count = ctx->image_x * ctx->image_y;
    pnglite_uc *p = png->out;

    if(out_n == 2){
        for(i = 0; i < pixel_count; ++i){
            if(p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2]){
                p[3] = 0;
            }
            p += 4;
        }
    }

    return 1;
}

static int pnglite__expand_palette(pnglite__png_t *png, pnglite_uc *palette, pnglite_uint32 len, pnglite_uint32 pal_image_n){
    pnglite_uint32 i, pixel_count = png->ctx->image_x * png->ctx->image_y;
    pnglite_uc *p, *temp_out, *original = png->out;

    p = (pnglite_uc*)PNGLITE_MALLOC(pixel_count * pal_image_n + 0);
    if(p == NULL) return pnglite__err("out of memory!");

    temp_out = p;

    if(pal_image_n == 3){
        for(i = 0; i < pixel_count; ++i){
            pnglite_uint32 n = original[i] * 4;
            p[0] = palette[n];
            p[1] = palette[n + 1];
            p[2] = palette[n + 2];
            p += 3;
        }
    } else {
        for(i = 0; i < pixel_count; ++i){
            pnglite_uint32 n = original[i] * 4;
            p[0] = palette[n];
            p[1] = palette[n + 1];
            p[2] = palette[n + 2];
            p[3] = palette[n + 3];
            p += 4;
        }
    }

    PNGLITE_FREE(png->out); png->out = temp_out;

    return 1;
}

static const pnglite_uc pnglite__depth_scale_table[9] = { 0, 0xFF, 0x55, 0, 0x11, 0, 0, 0, 0x01 };

static int pnglite__parse_png(pnglite__png_t *png, pnglite_uint32 scan, pnglite_uint32 requested_components){
    pnglite_ctx_t *ctx = png->ctx;

    pnglite_uc color = 0;
    pnglite_uc filter = 0;
    pnglite_uc interlace = 0;
    pnglite_uc palette[1024], pal_image_n = 0;
    pnglite_uc has_tRNS = 0, tc[3];
    pnglite_us tc16[3];

    pnglite_uint32 ioff = 0;
    pnglite_uint32 idata_limit = 0;
    pnglite_uint32 palette_len = 0;

    png->expanded = NULL;
    png->idata = NULL;
    png->out = NULL;

    if(!pnglite__check_png_header(ctx)) return 0;

    if(scan == PNGLITE__SCAN_type) return 1;

    for(;;){
        pnglite__chunk_t chunk = pnglite__get_chunk_header(ctx);
        switch(chunk.type){
            case PNGLITE__CHUNK_TYPE('C', 'g', 'B', 'I'):
                pnglite__err("iphone png type not supported!");
                break;

            case PNGLITE__CHUNK_TYPE('I', 'H', 'D', 'R'): {
                pnglite_uint32 compression;
                if(chunk.length != 13) return pnglite__err("bad IHDR length!");
                ctx->image_x = pnglite__get32be(ctx);
                ctx->image_y = pnglite__get32be(ctx);
                png->depth = pnglite__get8(ctx);
                color = pnglite__get8(ctx);
                if(color == 3) pal_image_n = 3; else if(color & 1) return pnglite__err("bad ctype!");
                filter = pnglite__get8(ctx);
                compression = pnglite__get8(ctx);
                interlace = pnglite__get8(ctx);
                if(!pal_image_n){
                    ctx->image_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
                    if(scan == PNGLITE__SCAN_header) return 1;
                } else {
                    ctx->image_n = 1;
                }
                break;
             }

            case PNGLITE__CHUNK_TYPE('I', 'D', 'A', 'T'): {
                if(pal_image_n && !palette_len) return pnglite__err("no PLTE!");
                if(scan == PNGLITE__SCAN_header) { ctx->image_n = pal_image_n; return 1; }
                if (((int)ioff + chunk.length) < (int)ioff) return 0;
                if(ioff + chunk.length > idata_limit){
                    pnglite_uint32 idata_limit_old = idata_limit;
                    pnglite_uc *p;
                    if(idata_limit == 0) idata_limit = (chunk.length > 4096) ? chunk.length : 4096;
                    while(ioff + chunk.length > idata_limit)
                        idata_limit *= 2;

                    p = (pnglite_uc*)PNGLITE_REALLOC(png->idata, idata_limit);
                    png->idata = p;
                }
                if(!pnglite__getn(ctx, png->idata + ioff, chunk.length)) return pnglite__err("corrupt PNG!");
                ioff += chunk.length;
                break;
            }

            case PNGLITE__CHUNK_TYPE('I', 'E', 'N', 'D'): {
                pnglite_uint32 raw_len, bpl;
                if(scan != PNGLITE__SCAN_load) return 1;

                bpl = (ctx->image_x * png->depth + 7) / 8;
                raw_len = bpl * ctx->image_y * ctx->image_n + ctx->image_y;

                png->expanded = (pnglite_uc*)pnglite_zlib_decode_malloc_guessize_headerflag((char*)png->idata, ioff, raw_len, (int*)&raw_len, 1);
                if(png->expanded == NULL) return 0;
                PNGLITE_FREE(png->idata); png->idata = NULL;

                ctx->image_out_n = ctx->image_n;

                if((requested_components == ctx->image_n + 1 && requested_components != 3 && !pal_image_n) || has_tRNS){
                    ctx->image_out_n = ctx->image_n + 1;
                } else {
                    ctx->image_out_n = ctx->image_n;
                }

                if(!pnglite__create_png_image(png, png->expanded, raw_len, ctx->image_out_n, png->depth, color)) return 0;
                if(has_tRNS){
                    if(png->depth == 16){
                        if(!pnglite__compute_transparency16(png, tc16, ctx->image_out_n)) return 0;
                    } else {
                        if(!pnglite__compute_transparency(png, tc, ctx->image_out_n)) return 0;
                    }
                }

                if(pal_image_n){
                    ctx->image_n = pal_image_n;
                    ctx->image_out_n = pal_image_n;

                    if(requested_components >= 3) ctx->image_out_n = requested_components;
                    if(!pnglite__expand_palette(png, palette, palette_len, ctx->image_out_n)) return 0;
                } else if(has_tRNS){
                    ++ctx->image_n;
                }

                PNGLITE_FREE(png->expanded); png->expanded = NULL;

                return 1;
            }

            case PNGLITE__CHUNK_TYPE('P', 'L', 'T', 'E'): {
                if(chunk.length > 256 * 3) return pnglite__err("invalid PLTE chunk!");
                palette_len = chunk.length / 3;
                if(palette_len * 3 != chunk.length) return pnglite__err("invalid PLTE chunk!");
                for(pnglite_uint32 i = 0; i < palette_len; ++i){
                    palette[i * 4 + 0] = pnglite__get8(ctx);
                    palette[i * 4 + 1] = pnglite__get8(ctx);
                    palette[i * 4 + 2] = pnglite__get8(ctx);
                    palette[i * 4 + 3] = 255;
                }
                break;
            }

            case PNGLITE__CHUNK_TYPE('t', 'R', 'N', 'S'): {
                if(png->idata) return pnglite__err("tRNS after IDAT!");
                if(pal_image_n){
                    if(scan == PNGLITE__SCAN_header) { ctx->image_n = 4; return 1; }
                    if(palette_len == 0) return pnglite__err("tRNS before PLTE!");
                    if(chunk.length > palette_len) return pnglite__err("bad tRNS length!");
                    pal_image_n = 4;
                    for(pnglite_uint32 i = 0; i < chunk.length; ++i){
                        palette[i * 4 + 3] = pnglite__get8(ctx);
                    }
                } else {
                    if(chunk.length != (pnglite_uint32)ctx->image_n * 2) return pnglite__err("bad tRNS length!");
                    has_tRNS = 1;
                    if(png->depth == 16){
                        for(pnglite_uint32 k = 0; k < ctx->image_n; ++k) tc16[k] = (pnglite_us)pnglite__get16be(ctx);
                    } else {
                        for(pnglite_uint32 k = 0; k < ctx->image_n; ++k) tc[k] = (pnglite_uc)(pnglite__get16be(ctx) & 255) * pnglite__depth_scale_table[png->depth];
                    }
                }
            }

            default: {
                pnglite__skip(ctx, chunk.length);
            }
        }

        pnglite__get32be(ctx);
    }
}

static int pnglite__getn(pnglite_ctx_t *ctx, pnglite_uc *buffer, pnglite_uint32 n){
    pnglite_uint32 blen = ctx->image_data_end - ctx->image_data;
    if(blen < n){
        pnglite_uint32 res, count;

        memcpy(buffer, ctx->image_data, blen);
        ctx->image_data += blen;

        return 0;
    }

    memcpy(buffer, ctx->image_data, n);
    ctx->image_data += n;

    return 1;
}

pnglite_inline static pnglite_uc pnglite__zget8(pnglite__zbuf_t *z){
    if(z->zbuffer >= z->zbuffer_end) return 0;
    return *z->zbuffer++;
}

static void pnglite__fill_bits(pnglite__zbuf_t *z){
    do {
        z->code_buffer |= (pnglite_uint32)pnglite__zget8(z) << z->num_bits;
        z->num_bits += 8;
    } while(z->num_bits <= 24);
}

pnglite_inline static pnglite_uint32 pnglite__zreceive(pnglite__zbuf_t *z, int n){
    pnglite_uint32 k;
    if(z->num_bits < n) pnglite__fill_bits(z);
    k = z->code_buffer & ((1 << n) - 1);
    z->code_buffer >>= n;
    z->num_bits -= n;
    return k;
}

static int pnglite__parse_zlib_header(pnglite__zbuf_t *a){
    int cmf = pnglite__zget8(a);
    int cm = cmf & 15;
    int flags = pnglite__zget8(a);

    if((cmf * 256 + flags) % 31 != 0) return pnglite__err("bad zlib header!");
    if(flags & 32) return pnglite__err("no preset dictionary!");
    if(cm != 8) return pnglite__err("bad compression!");

    return 1;
}

static int pnglite__zexpand(pnglite__zbuf_t *z, char *zout, pnglite_uint32 n){
    char *q;
    pnglite_uint32 cur, limit, old_limit;
    z->zout = zout;
    if(!z->z_expandable) return pnglite__err("output buffer limit!");
    cur = z->zout - z->zout_start;
    limit = old_limit = z->zout_end - z->zout_start;
    while(cur + n > limit)
        limit *= 2;

    q = (char*)PNGLITE_REALLOC(z->zout_start, limit);

    z->zout_start = q;
    z->zout = q + cur;
    z->zout_end = q + limit;

    return 1;
}

static int pnglite__parse_uncompressed_block(pnglite__zbuf_t *a){
    pnglite_uc header[4];
    pnglite_uint32 len, nlen, k;
    if(a->num_bits & 7)
        pnglite__zreceive(a, a->num_bits & 7);

    k = 0;
    while(a->num_bits > 0){
        header[k++] = (pnglite_uc)(a->code_buffer & 255);
        a->code_buffer >>= 8;
        a->num_bits -= 8;
    }

    while(k < 16){
        header[k++] = pnglite__zget8(a);
    }

    len = header[1] * 256 + header[0];
    nlen = header[3] * 256 + header[2];
    if(nlen != (len ^ 0xFFFF)) return pnglite__err("zlib corrupt!");
    if(a->zbuffer + len > a->zbuffer_end) return pnglite__err("read past buffer!");
    if(a->zout + len > a->zout_end)
        if(!pnglite__zexpand(a, a->zout, len)) return 0;

    memcpy(a->zout, a->zbuffer, len);
    a->zbuffer += len;
    a->zout += len;

    return 1;
}

static int pnglite__zbuild_huffman(pnglite__zhuffman_t *z, pnglite_uc *sizelist, pnglite_uint32 num){
    pnglite_uint32 i, k = 0;
    int code, next_code[16], sizes[17];

    memset(sizes, 0, sizeof(sizes));
    memset(z->fast, 0, sizeof(z->fast));

    for(i = 0; i < num; ++i){
        ++sizes[sizelist[i]];
    }
    sizes[0] = 0;
    for(i = 1; i < 16; ++i){
        if(sizes[i] > (1 << i)){
            return pnglite__err("bad sizes!");
        }
    }

    code = 0;
    for(i = 1; i < 16; ++i){
        next_code[i] = code;
        z->firstcode[i] = (pnglite_us)code;
        z->firstsymbol[i] = (pnglite_us)k;
        code = (code + sizes[i]);
        if(sizes[i]){
            if(code - 1 >= (1 << i)) return pnglite__err("bad codelengths!");
        }

        z->maxcode[i] = code << (16 - i);
        code <<= 1;
        k += sizes[i];
    }

    z->maxcode[16] = 0x10000;
    for(i = 0; i < num; ++i){
        pnglite_uint32 s = sizelist[i];
        if(s){
            pnglite_uint32 c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
            pnglite_us fastv = (pnglite_us)((s << 9) | i);
            z->size[c] = (pnglite_uc)s;
            z->value[c] = (pnglite_us)i;
            if(s <= PNGLITE__ZFAST_BITS){
                pnglite_uint32 j = pnglite__bit_reverse(next_code[s], s);
                while (j < (1 << PNGLITE__ZFAST_BITS)) {
                   z->fast[j] = fastv;
                   j += (1 << s);
                }
            }

            ++next_code[s];
        }
    }

    return 1;
}

static const int pnglite__zlength_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static const int pnglite__zlength_extra[31]=
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static const int pnglite__zdist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static const int pnglite__zdist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int pnglite__parse_huffman_block(pnglite__zbuf_t *a){
    char *zout = a->zout;
    for(;;){
        int z = pnglite__zhuffman_decode(a, &a->z_length);
        if(z < 256){
            if(z < 0) return pnglite__err("bad huffman code!");
            if(zout >= a->zout_end){
                if(!pnglite__zexpand(a, zout, 1)) return 0;
                zout = a->zout;
            }
            *zout++ = (char)z;
        } else {
            pnglite_uc *p;
            int len, dist;
            if(z == 256){
                a->zout = zout;
                return 1;
            }
            z -= 257;

            len = pnglite__zlength_base[z];
            if(pnglite__zlength_extra[z]) len += pnglite__zreceive(a, pnglite__zlength_extra[z]);
            z = pnglite__zhuffman_decode(a, &a->z_distance);
            if(z < 0) return pnglite__err("bad huffman code!");
            dist = pnglite__zdist_base[z];
            if (pnglite__zdist_extra[z]) dist += pnglite__zreceive(a, pnglite__zdist_extra[z]);
            if(zout - a->zout_start < dist) return pnglite__err("bad dist!");
            if(zout + len > a->zout_end){
                if(!pnglite__zexpand(a, zout, len)) return 0;
                zout = a->zout;
            }

            p = (pnglite_uc*)(zout - dist);
            if(dist == 1){
                pnglite_uc v = *p;
                if (len) { do *zout++ = v; while (--len); }
            } else {
                if (len) { do *zout++ = *p++; while (--len); }
            }
        }
    }
}

static int pnglite__zhuffman_decode_slowpath(pnglite__zbuf_t *a, pnglite__zhuffman_t *z){
    int b, s, k;

    k = pnglite__bit_reverse(a->code_buffer, 16);
    for(s = PNGLITE__ZFAST_BITS + 1; ; ++s){
        if (k < z->maxcode[s])
           break;
    }

    if(s == 16) return -1;

    b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
    a->code_buffer >>= s;
    a->num_bits -= s;
    return z->value[b];
}

static int pnglite__zhuffman_decode(pnglite__zbuf_t *a, pnglite__zhuffman_t *z){
    int b, s;
    if(a->num_bits < 16) pnglite__fill_bits(a);
    b = z->fast[a->code_buffer & PNGLITE__ZFAST_MASK];
    if (b) {
       s = b >> 9;
       a->code_buffer >>= s;
       a->num_bits -= s;
       return b & 511;
    }

    return pnglite__zhuffman_decode_slowpath(a, z);
}

static int pnglite__compute_huffman_codes(pnglite__zbuf_t *a){
    static const pnglite_uc length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
    pnglite__zhuffman_t z_codelength;
    pnglite_uc lencodes[286 + 32 + 137];
    pnglite_uc codelength_sizes[19];
    int i, n;

    int hlit = pnglite__zreceive(a, 5) + 257;
    int hdist = pnglite__zreceive(a, 5) + 1;
    int hclen = pnglite__zreceive(a, 4) + 4;
    int ntot = hlit + hdist;

    memset(codelength_sizes, 0, sizeof(codelength_sizes));
    for (i=0; i < hclen; ++i) {
       int s = pnglite__zreceive(a,3);
       codelength_sizes[length_dezigzag[i]] = (pnglite_uc)s;
    }

    if(!pnglite__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

    n = 0;
    while(n < ntot){
        int c = pnglite__zhuffman_decode(a, &z_codelength);
        if(c < 0 || c >= 19) return pnglite__err("bad codelengths!");
        if(c < 16)
            lencodes[n++] = (pnglite_uc)c;
        else {
            pnglite_uc fill = 0;
            if(c == 16){
                c = pnglite__zreceive(a, 2) + 3;
                if(n == 0) return pnglite__err("bad codelengths!");
                fill = lencodes[n - 1];
            } else if (c == 17){
                c = pnglite__zreceive(a, 3) + 3;
            } else {
                c = pnglite__zreceive(a, 7)+ 11;
            }

            if(ntot - n < c) return pnglite__err("bad codelengths!");
            memset(lencodes + n, fill, c);
            n += c;
        }
    }

    if(n != ntot) return pnglite__err("bad codelengths!");
    if(!pnglite__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
    if(!pnglite__zbuild_huffman(&a->z_distance, lencodes + hlit, hdist)) return 0;
    return 1;
}

static pnglite_uc pnglite__zdefault_length[288] =
{
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
   8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8
};
static pnglite_uc pnglite__zdefault_distance[32] =
{
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

static int pnglite__parse_zlib(pnglite__zbuf_t *a, int parse_header){
    int final, type;
    if(parse_header){
        if(!pnglite__parse_zlib_header(a)) return 0;
    }

    a->num_bits = 0;
    a->code_buffer = 0;
    do {
        final = pnglite__zreceive(a, 1);
        type = pnglite__zreceive(a, 2);
        if(type == 0){
            if(!pnglite__parse_uncompressed_block(a)) return 0;
        } else if(type == 3){
            return 0;
        } else {
            if(type == 1){
                if(!pnglite__zbuild_huffman(&a->z_length, pnglite__zdefault_length, 288)) return 0;
                if(!pnglite__zbuild_huffman(&a->z_distance, pnglite__zdefault_distance, 32)) return 0;
            } else {
                if(!pnglite__compute_huffman_codes(a)) return 0;
            }
            if(!pnglite__parse_huffman_block(a)) return 0;
        }
    } while(!final);

    return 1;
}

static int pnglite__do_zlib(pnglite__zbuf_t *a, char *obuf, int olen, int exp, int parse_header){
    a->zout_start = obuf;
    a->zout = obuf;
    a->zout_end = obuf + olen;
    a->z_expandable = exp;

    return pnglite__parse_zlib(a, parse_header);
}

PNGLITEDEF char *pnglite_zlib_decode_malloc_guessize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header){
    pnglite__zbuf_t a;
    char *p = (char*)PNGLITE_MALLOC(initial_size);

    if (p == NULL) return NULL;
    a.zbuffer = (pnglite_uc*)buffer;
    a.zbuffer_end = (pnglite_uc*)buffer + len;
    if(pnglite__do_zlib(&a, p, initial_size, 1, 1)){
        if (outlen) *outlen = (int)(a.zout - a.zout_start);
        return a.zout_start;
    } else {
        PNGLITE_FREE(a.zout_start);
        return NULL;
    }
}

static int pnglite__create_png_image_raw(pnglite__png_t* png, pnglite_uc *raw, pnglite_uint32 raw_len, int out_n, pnglite_uint32 x, pnglite_uint32 y, int depth, int color){
    int bytes = (depth == 16 ? 2 : 1);
    pnglite_ctx_t *s = png->ctx;
    pnglite_uint32 i, j, stride = x * out_n * bytes;
    pnglite_uint32 img_len, img_width_bytes;
    int k;
    pnglite_uint32 img_n = s->image_n;

    int output_bytes = out_n * bytes;
    int filter_bytes = img_n * bytes;
    int width = x;

    png->out = (pnglite_uc*)PNGLITE_MALLOC(x * y * output_bytes + 0);
    if(!png->out) return pnglite__err("out of memory!");

    img_width_bytes = (((img_n * x * depth) + 7) >> 3);
    img_len = (img_width_bytes + 1) * y;

    if(raw_len < img_len) return pnglite__err("not enough pixels!");

    for(j = 0; j < y; ++j){
        pnglite_uc *cur = png->out + stride * j;
        pnglite_uc *prior;
        int filter = *raw++;

        if(filter > 4)
            return pnglite__err("invalid filter!");

        if(depth < 8){
            cur += x * out_n - img_width_bytes;
            filter_bytes = 1;
            width = img_width_bytes;
        }

        prior = cur - stride;

        if(j == 0) filter = pnglite__first_row_filter[filter];

        for(k = 0; k < filter_bytes; ++k){
            switch(filter){
                case PNGLITE_FILTER_none: cur[k] = raw[k]; break;
                case PNGLITE_FILTER_sub: cur[k] = raw[k]; break;
                case PNGLITE_FILTER_up: cur[k] = PNGLITE__BYTECAST(raw[k] + prior[k]); break;
                case PNGLITE_FILTER_avg: cur[k] = PNGLITE__BYTECAST(raw[k] + (prior[k] >> 1)); break;
                case PNGLITE_FILTER_paeth: cur[k] = PNGLITE__BYTECAST(raw[k] + pnglite__paeth(0, prior[k], 0)); break;
                case PNGLITE_FILTER_avg_first: cur[k] = raw[k]; break;
                case PNGLITE_FILTER_paeth_first: cur[k] = raw[k]; break;
                default: return pnglite__err("filter type not supported yet!");
            }
        }

        if(depth == 8){
            if(img_n != out_n){
                cur[img_n] = 255;
            }
            raw += img_n;
            cur += out_n;
            prior += out_n;
        } else if(depth == 16){
            if (img_n != out_n) {
               cur[filter_bytes] = 255;
               cur[filter_bytes + 1] = 255;
            }
            raw += filter_bytes;
            cur += output_bytes;
            prior += output_bytes;
        } else {
            raw += 1;
            cur += 1;
            prior += 1;
        }

        if(depth < 8 || img_n == out_n){
            int nk = (width - 1)*filter_bytes;

            #define PNGLITE__CASE(f) \
                case f:     \
                    for(k = 0; k < nk; ++k)

            switch(filter){
                case PNGLITE_FILTER_none: memcpy(cur, raw, nk); break;
                PNGLITE__CASE(PNGLITE_FILTER_sub) { cur[k] = PNGLITE__BYTECAST(raw[k] + cur[k - filter_bytes]); } break;
                PNGLITE__CASE(PNGLITE_FILTER_up) { cur[k] = PNGLITE__BYTECAST(raw[k] + prior[k]); } break;
                PNGLITE__CASE(PNGLITE_FILTER_avg) { cur[k] = PNGLITE__BYTECAST(raw[k] + ((prior[k] + cur[k - filter_bytes]) >> 1)); } break;
                PNGLITE__CASE(PNGLITE_FILTER_paeth) { cur[k] = PNGLITE__BYTECAST(raw[k] + pnglite__paeth(cur[k - filter_bytes], prior[k], prior[k - filter_bytes])); } break;
                PNGLITE__CASE(PNGLITE_FILTER_avg_first) { cur[k] = PNGLITE__BYTECAST(raw[k] + (cur[k - filter_bytes] >> 1)); } break;
                PNGLITE__CASE(PNGLITE_FILTER_paeth_first) { cur[k] = PNGLITE__BYTECAST(raw[k] + pnglite__paeth(cur[k - filter_bytes], 0, 0)); } break;
            }

            #undef PNGLITE__CASE

            raw += nk;
        } else {
            #define PNGLITE__CASE(f) \
                case f:
                    for(i = x - 1; i >= 1; --i, cur[filter_bytes] = 255, raw += filter_bytes, cur += output_bytes, prior += output_bytes) \
                        for(k = 0; k < filter_bytes; ++k)
            switch(filter){
                PNGLITE__CASE(PNGLITE_FILTER_none) { cur[k] = raw[k]; } break;
                PNGLITE__CASE(PNGLITE_FILTER_sub) { cur[k] = PNGLITE__BYTECAST(raw[k] + cur[k - output_bytes]); } break;
                PNGLITE__CASE(PNGLITE_FILTER_up) { cur[k] = PNGLITE__BYTECAST(raw[k] + prior[k]); } break;
                PNGLITE__CASE(PNGLITE_FILTER_avg) { cur[k] = PNGLITE__BYTECAST(raw[k] + ((prior[k] + cur[k - output_bytes]) >> 1)); } break;
                PNGLITE__CASE(PNGLITE_FILTER_paeth) { cur[k] = PNGLITE__BYTECAST(raw[k] + pnglite__paeth(cur[k - output_bytes], prior[k], prior[k - output_bytes])); } break;
                PNGLITE__CASE(PNGLITE_FILTER_avg_first) { cur[k] = PNGLITE__BYTECAST(raw[k] + (cur[k - output_bytes] >> 1)); } break;
                PNGLITE__CASE(PNGLITE_FILTER_paeth_first) { cur[k] = PNGLITE__BYTECAST(raw[k] + pnglite__paeth(cur[k - output_bytes], 0, 0)); } break;
            }

            if(depth == 16){
                cur = png->out + stride * j;
                for (i=0; i < x; ++i,cur += output_bytes) {
                   cur[filter_bytes + 1] = 255;
                }
            }
        }
    }

    if(depth < 8){
        for(j = 0; j < y; ++j){
            pnglite_uc *cur = png->out + stride * j;
            pnglite_uc *in = png->out + stride * j + x * out_n - img_width_bytes;
            pnglite_uc scale = (color == 0) ? pnglite__depth_scale_table[depth] : 1;

            if(depth == 4){
                for (k=x*img_n; k >= 2; k-=2, ++in) {
                   *cur++ = scale * ((*in >> 4));
                   *cur++ = scale * ((*in) & 0x0f);
                }
                if(k > 0) *cur++ = scale * ((*in >> 4));
            } else if(depth == 2){
                for (k=x*img_n; k >= 4; k-=4, ++in) {
                   *cur++ = scale * ((*in >> 6)       );
                   *cur++ = scale * ((*in >> 4) & 0x03);
                   *cur++ = scale * ((*in >> 2) & 0x03);
                   *cur++ = scale * ((*in     ) & 0x03);
                }
                if (k > 0) *cur++ = scale * ((*in >> 6));
                if (k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
                if (k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
            } else if (depth == 1) {
                for (k=x*img_n; k >= 8; k-=8, ++in) {
                    *cur++ = scale * ((*in >> 7)       );
                    *cur++ = scale * ((*in >> 6) & 0x01);
                    *cur++ = scale * ((*in >> 5) & 0x01);
                    *cur++ = scale * ((*in >> 4) & 0x01);
                    *cur++ = scale * ((*in >> 3) & 0x01);
                    *cur++ = scale * ((*in >> 2) & 0x01);
                    *cur++ = scale * ((*in >> 1) & 0x01);
                    *cur++ = scale * ((*in     ) & 0x01);
                }
                if (k > 0) *cur++ = scale * ((*in >> 7)       );
                if (k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
                if (k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
                if (k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
                if (k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
                if (k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
                if (k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
            }

            if(img_n != out_n){
                int q;

                cur = png->out + stride * j;
                if(img_n == 1){
                    for(q = x - 1; q >= 0; --q){
                        cur[q * 2 + 1] = 255;
                        cur[q * 2 + 0] = cur[q];
                    }
                } else {
                    for (q = x - 1; q >= 0; --q) {
                       cur[q * 4 + 3] = 255;
                       cur[q * 4 + 2] = cur[q * 3 + 2];
                       cur[q *4 + 1] = cur[q * 3 + 1];
                       cur[q * 4 + 0] = cur[q * 3 + 0];
                    }
                }
            }
        }
    } else if(depth == 16){
        pnglite_uc *cur = png->out;
        pnglite_us *cur16 = (pnglite_us*)cur;

        for(i = 0; i < x * y * out_n; ++i, cur16++, cur += 2){
            *cur16 = (cur[0] << 8) | cur[1];
        }
    }

    return 1;
}

static int pnglite__create_png_image(pnglite__png_t *png, pnglite_uc *image_data, pnglite_uint32 image_data_len, int out_n, int depth, int color){
    int bytes = (depth == 16 ? 2 : 1);
    int out_bytes = out_n * bytes;
    pnglite_uc *final;
    int p;
    return pnglite__create_png_image_raw(png, image_data, image_data_len, out_n, png->ctx->image_x, png->ctx->image_y, depth, color);
}

pnglite_inline static int pnglite__bitreverse16(int n){
    n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
    n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
    n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);

    return n;
}

pnglite_inline static int pnglite__bit_reverse(int v, int bits){
    return pnglite__bitreverse16(v) >> (16 - bits);
}

static void *pnglite__do_png(pnglite__png_t *png, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *n, pnglite_uint32 requested_components, pnglite__result_info_t *result_info){
    void *result = NULL;
    if(pnglite__parse_png(png, PNGLITE__SCAN_load, requested_components)){
        if(png->depth < 8)
            result_info->bits_per_channel = 8;
        else
            result_info->bits_per_channel = png->depth;

        result = png->out;
        png->out = NULL;
        if(requested_components && requested_components != png->ctx->image_n){
            if(result_info->bits_per_channel == 8){
                result = pnglite__convert_format((pnglite_uc*)result, png->ctx->image_out_n, requested_components, png->ctx->image_x, png->ctx->image_y);
            } else {
                result = pnglite__convert_format16((pnglite_us*)result, png->ctx->image_out_n, requested_components, png->ctx->image_x, png->ctx->image_y);
            }

            png->ctx->image_out_n = requested_components;
            if (result == NULL) return result;
        }

        *x = png->ctx->image_x;
        *y = png->ctx->image_y;
        if(n) *n = png->ctx->image_n;
    }

    PNGLITE_FREE(png->out); png->out = NULL;
    PNGLITE_FREE(png->expanded); png->expanded = NULL;
    PNGLITE_FREE(png->idata); png->idata = NULL;

    return result;
}

static pnglite_uc pnglite__compute_y(int r, int g, int b){
    return (pnglite_uc)(((r * 77) + (g * 150) + (b * 29)) >> 8);
}

static pnglite_us pnglite__compute_y16(int r, int g, int b){
    return (pnglite_us)(((r * 77) + (g * 150) + (b * 29)) >> 8);
}

static int pnglite__paeth(int a, int b, int c){
    int p = a + b - c;
    int pa = abs(p - a);
    int pb = abs(p - b);
    int pc = abs(p - c);

    if(pa <= pb && pa <= pc) return a;
    if(pb <= pc) return b;

    return c;
}

static pnglite_us *pnglite__load_16bit(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    pnglite__result_info_t result_info;
    void *result = pnglite__load_main(ctx, x, y, components, requested_components, &result_info);

    if(result == NULL) return NULL;

    if(result_info.bits_per_channel != 16){
        result = pnglite__convert_8_to_16((pnglite_uc*)result, *x, *y, requested_components == 0 ? *components : requested_components);
        result_info.bits_per_channel = 16;
    }

    return (pnglite_us*)result;
}

static pnglite_us *pnglite__convert_8_to_16(pnglite_uc *original, pnglite_uint32 width, pnglite_uint32 height, pnglite_uint32 channels){
    pnglite_uint32 i;
    pnglite_uint32 image_len = width * height * channels;

    pnglite_us *enlarged;

    enlarged = (pnglite_us*)PNGLITE_MALLOC(image_len * 2);

    for(i = 0; i < image_len; ++i){
        enlarged[i] = (pnglite_us)((original[i] << 8) + original[i]);
    }

    PNGLITE_FREE(original);
    return enlarged;
}

static pnglite_uc *pnglite__convert_format(pnglite_uc *data, pnglite_uint32 img_n, pnglite_uint32 requested_components, pnglite_uint32 x, pnglite_uint32 y){
    pnglite_uint32 i, j;
    pnglite_uc *good;

    if (requested_components == img_n) return data;

    good = (pnglite_uc*)PNGLITE_MALLOC(requested_components * x * y + 0);

    pnglite_uint32 combo = img_n * 8 + requested_components;

    for(j = 0; j < y; ++j){
        pnglite_uc *src = data + j * x * img_n;
        pnglite_uc *dest = good + j * x * requested_components;

        switch(combo){
            switch(combo){
                case 1*8 + 2:
                    for (i = x; i > 0; --i, src += 1, dest += 2) {
                        dest[0] = src[0];
                        dest[1] = 255;
                    }
                    break;

                case 1*8 + 3:
                    for (i = x; i > 0; --i, src += 1, dest += 3) {
                        dest[0] = dest[1] = dest[2] = src[0];
                    }
                    break;

                case 1*8 + 4:
                    for (i = x; i > 0; --i, src += 1, dest += 4) {
                        dest[0] = dest[1] = dest[2] = src[0];
                        dest[3] = 255;
                    }
                    break;

                case 2*8 + 1:
                    for (i = x; i > 0; --i, src += 2, dest += 1) {
                        dest[0] = src[0];
                    }
                    break;

                case 2*8 + 3:
                    for (i = x; i > 0; --i, src += 2, dest += 3) {
                        dest[0] = dest[1] = dest[2] = src[0];
                    }
                    break;

                case 2*8 + 4:
                    for (i = x; i > 0; --i, src += 2, dest += 4) {
                        dest[0] = dest[1] = dest[2] = src[0];
                        dest[3] = src[1];
                    }
                    break;

                case 3*8 + 4:
                    for (i = x; i > 0; --i, src += 3, dest += 4) {
                        dest[0] = src[0];
                        dest[1] = src[1];
                        dest[2] = src[2];
                        dest[3] = 255;
                    }
                    break;

                case 3*8 + 1:
                    for (i = x; i > 0; --i, src += 3, dest += 1) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                    }
                    break;

                case 3*8 + 2:
                    for (i = x; i > 0; --i, src += 3, dest += 2) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                        dest[1] = 255;
                    }
                    break;

                case 4*8 + 1:
                    for (i = x; i > 0; --i, src += 4, dest += 1) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                    }
                    break;

                case 4*8 + 2:
                    for (i = x; i > 0; --i, src += 4, dest += 2) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                        dest[1] = src[3];
                    }
                    break;

                case 4*8 + 3:
                    for (i = x; i > 0; --i, src += 4, dest += 3) {
                        dest[0] = src[0];
                        dest[1] = src[1];
                        dest[2] = src[2];
                    }
                    break;
            }
        }
    }

    PNGLITE_FREE(data);
    return good;
}

static pnglite_us *pnglite__convert_format16(pnglite_us *data, pnglite_uint32 img_n, pnglite_uint32 requested_components, pnglite_uint32 x, pnglite_uint32 y){
    pnglite_uint32 i, j;
    pnglite_us *good;

    if (requested_components == img_n) return data;

    good = (pnglite_us*)PNGLITE_MALLOC(requested_components * x * y + 0);

    pnglite_uint32 combo = img_n * 8 + requested_components;

    for(j = 0; j < y; ++j){
        pnglite_us *src = data + j * x * img_n;
        pnglite_us *dest = good + j * x * requested_components;

        switch(combo){
            switch(combo){
                case 1*8 + 2:
                    for (i = x; i > 0; --i, src += 1, dest += 2) {
                        dest[0] = src[0];
                        dest[1] = 0xFFFF;
                    }
                    break;

                case 1*8 + 3:
                    for (i = x; i > 0; --i, src += 1, dest += 3) {
                        dest[0] = dest[1] = dest[2] = src[0];
                    }
                    break;

                case 1*8 + 4:
                    for (i = x; i > 0; --i, src += 1, dest += 4) {
                        dest[0] = dest[1] = dest[2] = src[0];
                        dest[3] = 0xFFFF;
                    }
                    break;

                case 2*8 + 1:
                    for (i = x; i > 0; --i, src += 2, dest += 1) {
                        dest[0] = src[0];
                    }
                    break;

                case 2*8 + 3:
                    for (i = x; i > 0; --i, src += 2, dest += 3) {
                        dest[0] = dest[1] = dest[2] = src[0];
                    }
                    break;

                case 2*8 + 4:
                    for (i = x; i > 0; --i, src += 2, dest += 4) {
                        dest[0] = dest[1] = dest[2] = src[0];
                        dest[3] = src[1];
                    }
                    break;

                case 3*8 + 4:
                    for (i = x; i > 0; --i, src += 3, dest += 4) {
                        dest[0] = src[0];
                        dest[1] = src[1];
                        dest[2] = src[2];
                        dest[3] = 0xFFFF;
                    }
                    break;

                case 3*8 + 1:
                    for (i = x; i > 0; --i, src += 3, dest += 1) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                    }
                    break;

                case 3*8 + 2:
                    for (i = x; i > 0; --i, src += 3, dest += 2) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                        dest[1] = 255;
                    }
                    break;

                case 4*8 + 1:
                    for (i = x; i > 0; --i, src += 4, dest += 1) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                    }
                    break;

                case 4*8 + 2:
                    for (i = x; i > 0; --i, src += 4, dest += 2) {
                        dest[0] = pnglite__compute_y16(src[0], src[1], src[2]);
                        dest[1] = src[3];
                    }
                    break;

                case 4*8 + 3:
                    for (i = x; i > 0; --i, src += 4, dest += 3) {
                        dest[0] = src[0];
                        dest[1] = src[1];
                        dest[2] = src[2];
                    }
                    break;
            }
        }
    }

    PNGLITE_FREE(data);
    return good;
}

PNGLITEDEF pnglite_uc *pnglite_load_from_memory(pnglite_uc *buffer, pnglite_uint32 len, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    pnglite_ctx_t ctx;
    pnglite__ctx_init(&ctx, buffer, len);
    return pnglite__load_8bit(&ctx, x, y, components, requested_components);
}

PNGLITEDEF pnglite_us *pnglite_load_16_from_memory(pnglite_uc *buffer, pnglite_uint32 len, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    pnglite_ctx_t ctx;
    pnglite__ctx_init(&ctx, buffer, len);
    return pnglite__load_16bit(&ctx, x, y, components, requested_components);
}

#ifndef PNGLITE_NO_STDIO
PNGLITEDEF pnglite_uc *pnglite_load(const char *filename, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    FILE *f = fopen(filename, "rb");
    if(!f){
        pnglite__err("failed to open file!");
    }

    pnglite_uc *result;
    result = pnglite_load_from_file(f, x, y, components, requested_components);
    fclose(f);
    return result;
}

PNGLITEDEF pnglite_uc *pnglite_load_from_file(FILE *f, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    pnglite_uc *result;
    pnglite_ctx_t ctx;

    if (!f) {
        pnglite__err("file pointer is NULL!");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    pnglite_uc *buffer = (pnglite_uc*)PNGLITE_MALLOC(file_size);
    size_t read_size = fread(buffer, 1, file_size, f);
    if (read_size != file_size) {
        pnglite__err("failed to read full file!");
        free(buffer);
        return NULL;
    }

    result = pnglite_load_from_memory(buffer, read_size, x, y, components, requested_components);
    if (!result) {
        pnglite__err("failed to load PNG from memory!");
        free(buffer);
        return NULL;
    }

    return result;
}
PNGLITEDEF pnglite_us *pnglite_load_16_from_file(FILE *f, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    pnglite_us *result;
    pnglite_ctx_t ctx;

    if (!f) {
        pnglite__err("file pointer is NULL!");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    pnglite_uc *buffer = (pnglite_uc*)PNGLITE_MALLOC(file_size);
    size_t read_size = fread(buffer, 1, file_size, f);
    if (read_size != file_size) {
        pnglite__err("failed to read full file!");
        free(buffer);
        return NULL;
    }

    result = pnglite_load_16_from_memory(buffer, read_size, x, y, components, requested_components);
    if (!result) {
        pnglite__err("failed to load PNG from memory!");
        free(buffer);
        return NULL;
    }

    return result;
}
PNGLITEDEF pnglite_us *pnglite_load_16(const char *filename, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite_uint32 requested_components){
    FILE *f = fopen(filename, "rb");
    if(!f){
        pnglite__err("failed to open file!");
    }

    pnglite_us *result;
    result = pnglite_load_16_from_file(f, x, y, components, requested_components);
    fclose(f);
    return result;
}
#endif
