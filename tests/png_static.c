#include <pnglite.h>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(){
    const char *filename = "../tests/wallpapergrey.png";
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Failed to open file");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *buffer = malloc(filesize);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(f);
        return 1;
    }

    size_t read_bytes = fread(buffer, 1, filesize, f);
    fclose(f);

    pnglite_uint32 x, y, components;
    int x2, y2, components2;
    stbi_uc *buf2 = stbi_load_from_memory(buffer, read_bytes, &x2, &y2, &components2, 0);
    pnglite_uc *buf = pnglite_load_from_memory(buffer, read_bytes, &x, &y, &components, 0);
    for(uint32_t i = 0; i < 100; i++){
        printf("buf1, buf2: %u, %u\n", buf[i], buf2[i]);
    }
    //printf("%u", buf[0]);
    //pnglite_load("../tests/wallpapergrey.png", &x, &y, &components, 3);

    return 0;
}
