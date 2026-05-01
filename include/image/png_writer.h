/**
 * @file png_writer.h
 * @brief Minimal PNG writer for RGBA framebuffer exports.
 */
#ifndef GLDRAW_IMAGE_PNG_WRITER_H
#define GLDRAW_IMAGE_PNG_WRITER_H

/**
 * @brief Write RGBA pixels to a PNG file.
 *
 * The input buffer is expected in OpenGL readback order: bottom row first,
 * four bytes per pixel, RGBA channel order.
 *
 * @param path Destination file path.
 * @param rgba_bottom_up Pixel buffer in bottom-up RGBA order.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @return Non-zero on success, zero on failure.
 */
int png_writer_write_rgba_file(const char* path,
                               const unsigned char* rgba_bottom_up,
                               int width,
                               int height);

#endif /* GLDRAW_IMAGE_PNG_WRITER_H */
