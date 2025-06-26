#include <pnglite.h>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(){
    const char *filename = "../tests/wallpaper_plte.png";
    const char *filename16 = "../tests/wallpaper16.png";

    pnglite_uint32 x, y, components;
    pnglite_uint32 x16, y16, components16;
    pnglite_uc *buf = pnglite_load(filename, &x, &y, &components, 0);

    pnglite_us *buf16 = pnglite_load_16(filename16, &x16, &y16, &components16, 0);

    return 0;
}
