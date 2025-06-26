#include <pnglite.h>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(){
    const char *filename = "../tests/wallpaper_plte.png";
    const char *filename16 = "../tests/wallpaper16.png";

    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "failed to open file!");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    pnglite_uc *buffer = (pnglite_uc*)malloc(filesize);
    if (!buffer) {
        fprintf(stderr, "failed to allocate memory!");
        fclose(f);
        return 1;
    }

    size_t read_bytes = fread(buffer, 1, filesize, f);
    fclose(f);

    FILE *f16 = fopen(filename16, "rb");
    if(!f){
        fprintf(stderr, "failed to open file!");
        return 1;
    }

    fseek(f16, 0, SEEK_END);
    long filesize16 = ftell(f16);
    fseek(f16, 0, SEEK_SET);

    pnglite_uc *buffer16 = (pnglite_uc*)malloc(filesize16);
    if(!buffer16){
        fprintf(stderr, "failed to allocate memory!");
        fclose(f16);
        return 1;
    }

    size_t read_bytes16 = fread(buffer16, 1, filesize16, f16);
    fclose(f16);

    pnglite_uint32 x, y, components;
    pnglite_uint32 x16, y16, components16;
    int x16stb, y16stb, components16stb;
    int xstb, ystb, componentstb;
    pnglite_uc *buf = pnglite_load_from_memory(buffer, read_bytes, &x, &y, &components, 0);
    stbi_uc *bufstb = stbi_load_from_memory(buffer, read_bytes, &xstb, &ystb, &componentstb, 0);

    pnglite_us *buf16 = pnglite_load_16_from_memory(buffer16, read_bytes16, &x16, &y16, &components16, 0);

    return 0;
}
