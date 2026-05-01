/**
 * @file png_writer.c
 * @brief Minimal PNG encoder used by canvas export.
 */
#include <image/png_writer.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int png_crc32_update(unsigned int crc,
                                     const unsigned char* data,
                                     size_t size)
{
    size_t i = 0u;
    int bit = 0;

    crc = ~crc;
    for (i = 0u; i < size; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }

    return ~crc;
}

static unsigned int png_adler32(const unsigned char* data, size_t size)
{
    const unsigned int mod_adler = 65521u;
    unsigned int a = 1u;
    unsigned int b = 0u;
    size_t i = 0u;

    for (i = 0u; i < size; ++i) {
        a = (a + data[i]) % mod_adler;
        b = (b + a) % mod_adler;
    }

    return (b << 16) | a;
}

static void png_store_u32_be(unsigned char bytes[4], unsigned int value)
{
    bytes[0] = (unsigned char)((value >> 24) & 0xFFu);
    bytes[1] = (unsigned char)((value >> 16) & 0xFFu);
    bytes[2] = (unsigned char)((value >> 8) & 0xFFu);
    bytes[3] = (unsigned char)(value & 0xFFu);
}

static int png_write_u32_be(FILE* file, unsigned int value)
{
    unsigned char bytes[4];

    if (!file) {
        return 0;
    }

    png_store_u32_be(bytes, value);
    return fwrite(bytes, 1u, sizeof(bytes), file) == sizeof(bytes);
}

static int png_write_chunk(FILE* file,
                           const char type[4],
                           const unsigned char* data,
                           size_t size)
{
    unsigned int crc = 0u;

    if (!file || !type || size > 0xFFFFFFFFu) {
        return 0;
    }

    if (!png_write_u32_be(file, (unsigned int)size)) {
        return 0;
    }
    if (fwrite(type, 1u, 4u, file) != 4u) {
        return 0;
    }
    if (size > 0u && fwrite(data, 1u, size, file) != size) {
        return 0;
    }

    crc = png_crc32_update(0u, (const unsigned char*)type, 4u);
    if (size > 0u) {
        crc = png_crc32_update(crc, data, size);
    }
    return png_write_u32_be(file, crc);
}

static int png_build_uncompressed_zlib(const unsigned char* data,
                                       size_t data_size,
                                       unsigned char** out_data,
                                       size_t* out_size)
{
    size_t block_count = 0u;
    size_t capacity = 0u;
    size_t source_offset = 0u;
    size_t write_offset = 0u;
    unsigned char* output = NULL;
    unsigned int adler = 0u;

    if (!data || !out_data || !out_size) {
        return 0;
    }

    block_count = (data_size + 65534u) / 65535u;
    capacity = 2u + data_size + block_count * 5u + 4u;
    output = (unsigned char*)malloc(capacity);
    if (!output) {
        return 0;
    }

    output[write_offset++] = 0x78u;
    output[write_offset++] = 0x01u;

    while (source_offset < data_size || (data_size == 0u && source_offset == 0u)) {
        size_t remaining = data_size - source_offset;
        unsigned int block_size = (remaining > 65535u) ? 65535u : (unsigned int)remaining;
        unsigned int final_block = (source_offset + block_size >= data_size) ? 1u : 0u;

        output[write_offset++] = (unsigned char)final_block;
        output[write_offset++] = (unsigned char)(block_size & 0xFFu);
        output[write_offset++] = (unsigned char)((block_size >> 8) & 0xFFu);
        output[write_offset++] = (unsigned char)((~block_size) & 0xFFu);
        output[write_offset++] = (unsigned char)(((~block_size) >> 8) & 0xFFu);

        if (block_size > 0u) {
            memcpy(output + write_offset, data + source_offset, block_size);
            write_offset += block_size;
            source_offset += block_size;
        } else {
            break;
        }
    }

    adler = png_adler32(data, data_size);
    output[write_offset++] = (unsigned char)((adler >> 24) & 0xFFu);
    output[write_offset++] = (unsigned char)((adler >> 16) & 0xFFu);
    output[write_offset++] = (unsigned char)((adler >> 8) & 0xFFu);
    output[write_offset++] = (unsigned char)(adler & 0xFFu);

    *out_data = output;
    *out_size = write_offset;
    return 1;
}

int png_writer_write_rgba_file(const char* path,
                               const unsigned char* rgba_bottom_up,
                               int width,
                               int height)
{
    static const unsigned char signature[8] = {137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u};
    FILE* file = NULL;
    unsigned char ihdr[13];
    unsigned char* filtered = NULL;
    unsigned char* zlib_data = NULL;
    size_t filtered_stride = 0u;
    size_t filtered_size = 0u;
    size_t zlib_size = 0u;
    int row = 0;
    int ok = 0;

    if (!path || !rgba_bottom_up || width <= 0 || height <= 0) {
        return 0;
    }

    filtered_stride = (size_t)width * 4u + 1u;
    filtered_size = filtered_stride * (size_t)height;
    filtered = (unsigned char*)malloc(filtered_size);
    if (!filtered) {
        return 0;
    }

    for (row = 0; row < height; ++row) {
        const unsigned char* source =
            rgba_bottom_up + (size_t)(height - 1 - row) * (size_t)width * 4u;
        unsigned char* target = filtered + (size_t)row * filtered_stride;

        target[0] = 0u;
        memcpy(target + 1u, source, (size_t)width * 4u);
    }

    if (!png_build_uncompressed_zlib(filtered, filtered_size, &zlib_data, &zlib_size)) {
        free(filtered);
        return 0;
    }

    file = fopen(path, "wb");
    if (!file) {
        free(zlib_data);
        free(filtered);
        return 0;
    }

    memset(ihdr, 0, sizeof(ihdr));
    png_store_u32_be(&ihdr[0], (unsigned int)width);
    png_store_u32_be(&ihdr[4], (unsigned int)height);
    ihdr[8] = 8u;
    ihdr[9] = 6u;

    ok = fwrite(signature, 1u, sizeof(signature), file) == sizeof(signature) &&
         png_write_chunk(file, "IHDR", ihdr, sizeof(ihdr)) &&
         png_write_chunk(file, "IDAT", zlib_data, zlib_size) &&
         png_write_chunk(file, "IEND", NULL, 0u);

    if (fclose(file) != 0) {
        ok = 0;
    }

    free(zlib_data);
    free(filtered);
    return ok;
}
