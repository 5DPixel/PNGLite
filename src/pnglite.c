#include <pnglite.h>

static void pnglite__ctx_init(pnglite_ctx_t *ctx, pnglite_uc *buffer, pnglite_uint32 len){
    ctx->io_ctx.read = NULL;
    ctx->image_data = buffer;
    ctx->image_data_end = (pnglite_uc*)buffer + len;
}

static pnglite__chunk_t pnglite__get_chunk_header(pnglite_ctx_t *ctx){
    pnglite__chunk_t chunk;
    chunk.length = pnglite__get8(ctx);
    chunk.type = pnglite__get8(ctx);

    return chunk;
}

static int pnglite__check_png_header(pnglite_ctx_t* ctx){
    static const pnglite_uc png_signature[8] = { 137,80,78,71,13,10,26,10 };
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

static void pnglite__load_main(pnglite_ctx_t *ctx, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components, pnglite__result_info_t *result_info){
    memset(result_info, 0, sizeof(*result_info));
    result_info->bits_per_channel = 8;
    result_info->channel_order = PNGLITE_ORDER_RGB;
    result_info->num_channels = 0;
}

pnglite_inline static pnglite_uc pnglite__get8(pnglite_ctx_t *ctx){
    if(ctx->image_data < ctx->image_data_end){
        return *ctx->image_data++;
    }

    return 0;
}

PNGLITEDEF pnglite_uc* pnglite_load_from_memory(pnglite_uc* buffer, pnglite_uint32 len, pnglite_uint32 *x, pnglite_uint32 *y, pnglite_uint32 *components){
    pnglite_ctx_t ctx;
    pnglite__ctx_init(&ctx, buffer, len);
}
