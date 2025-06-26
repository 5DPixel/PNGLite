# PNGLite
PNGLite is a simple, lightweight C library for decoding PNG images with no dependencies. It's designed for simplicity and ease of integration.

# Features
- Loads 8-bit paletted PNGs (indexed colour)
- Loads all other standard PNG types (greyscale, truecolour, alpha variants)
- No dependencies
- Easy to integrate and build

# Building from source
## Requirements
- A C compiler supporting the C99 standard or later (for example, [gcc](https://gcc.gnu.org/install/download.html) or [clang](https://releases.llvm.org/download.html))
- CMake (get it at [CMake](https://cmake.org))

Compile with:
```bash
cmake ..
make
```

Building as a shared library:
```bash
cmake -DBUILD_SHARED_LIBS=ON ..
make
```
