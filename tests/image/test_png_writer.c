#include <image/png_writer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int make_temp_path(char* buffer, size_t buffer_size, const char* suffix)
{
    char base_name[L_tmpnam];

    if (!buffer || buffer_size == 0u || !suffix) {
        return 0;
    }

#ifdef _WIN32
    if (tmpnam_s(base_name, sizeof(base_name)) != 0) {
        return 0;
    }
#else
    if (!tmpnam(base_name)) {
        return 0;
    }
#endif

    if (snprintf(buffer, buffer_size, "%s%s", base_name, suffix) >= (int)buffer_size) {
        return 0;
    }

    return 1;
}

static unsigned int read_u32_be(const unsigned char bytes[4])
{
    return ((unsigned int)bytes[0] << 24) |
           ((unsigned int)bytes[1] << 16) |
           ((unsigned int)bytes[2] << 8) |
           (unsigned int)bytes[3];
}

int main(void)
{
    static const unsigned char expected_signature[8] = {
        137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u
    };
    const unsigned char pixels[16] = {
        255u, 0u, 0u, 255u,
        0u, 255u, 0u, 255u,
        0u, 0u, 255u, 255u,
        255u, 255u, 255u, 255u
    };
    unsigned char header[33];
    char path[L_tmpnam + 16];
    FILE* file = NULL;
    int failed = 0;

    if (!make_temp_path(path, sizeof(path), ".png")) {
        fprintf(stderr, "failed to create temporary path\n");
        return 1;
    }

    if (!png_writer_write_rgba_file(path, pixels, 2, 2)) {
        fprintf(stderr, "failed to write png\n");
        return 1;
    }

    file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "failed to reopen png\n");
        remove(path);
        return 1;
    }

    if (fread(header, 1u, sizeof(header), file) != sizeof(header)) {
        fprintf(stderr, "failed to read png header\n");
        failed = 1;
    }
    fclose(file);

    if (!failed && memcmp(header, expected_signature, sizeof(expected_signature)) != 0) {
        fprintf(stderr, "invalid png signature\n");
        failed = 1;
    }
    if (!failed && read_u32_be(&header[8]) != 13u) {
        fprintf(stderr, "invalid IHDR chunk length\n");
        failed = 1;
    }
    if (!failed && memcmp(&header[12], "IHDR", 4u) != 0) {
        fprintf(stderr, "missing IHDR chunk\n");
        failed = 1;
    }
    if (!failed && read_u32_be(&header[16]) != 2u) {
        fprintf(stderr, "invalid IHDR width\n");
        failed = 1;
    }
    if (!failed && read_u32_be(&header[20]) != 2u) {
        fprintf(stderr, "invalid IHDR height\n");
        failed = 1;
    }
    if (!failed && (header[24] != 8u || header[25] != 6u)) {
        fprintf(stderr, "invalid IHDR color format\n");
        failed = 1;
    }

    remove(path);
    if (failed) {
        return 1;
    }

    printf("[PASS] png writer creates rgba png header\n");
    return 0;
}
