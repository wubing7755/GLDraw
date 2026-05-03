#ifndef GLDRAW_RENDER_RENDER_DEVICE_H
#define GLDRAW_RENDER_RENDER_DEVICE_H

#include <base/types.h>

typedef enum RenderPathMode {
    RENDER_PATH_LINES = 0,
    RENDER_PATH_LINE_STRIP
} RenderPathMode;

typedef struct RenderFrameDesc {
    int logical_width;
    int logical_height;
    int framebuffer_width;
    int framebuffer_height;
    Color clear_color;
} RenderFrameDesc;

typedef struct RenderTransform {
    float scale_x;
    float scale_y;
} RenderTransform;

typedef struct RenderDevice RenderDevice;

typedef struct RenderDeviceVTable {
    int (*resize)(RenderDevice* device,
                  int logical_width,
                  int logical_height,
                  int framebuffer_width,
                  int framebuffer_height);
    int (*begin_frame)(RenderDevice* device, const RenderFrameDesc* frame_desc);
    void (*set_transform)(RenderDevice* device, const RenderTransform* transform);
    void (*set_clip_rect)(RenderDevice* device, RectF clip_rect);
    void (*set_color)(RenderDevice* device, Color color);
    void (*draw_line)(RenderDevice* device, Vec2 from, Vec2 to, float line_width);
    void (*draw_rect)(RenderDevice* device, RectF rect, float line_width);
    void (*draw_path)(RenderDevice* device,
                      const Vec2* points,
                      int point_count,
                      RenderPathMode mode,
                      float line_width);
    int (*read_pixels)(RenderDevice* device,
                       RectF rect,
                       unsigned char* out_rgba,
                       int stride_bytes);
    void (*end_frame)(RenderDevice* device);
    void (*destroy)(RenderDevice* device);
} RenderDeviceVTable;

struct RenderDevice {
    const RenderDeviceVTable* vtable;
};

int render_device_resize(RenderDevice* device,
                         int logical_width,
                         int logical_height,
                         int framebuffer_width,
                         int framebuffer_height);
int render_device_begin_frame(RenderDevice* device, const RenderFrameDesc* frame_desc);
void render_device_set_transform(RenderDevice* device, const RenderTransform* transform);
void render_device_set_clip_rect(RenderDevice* device, RectF clip_rect);
void render_device_set_color(RenderDevice* device, Color color);
void render_device_draw_line(RenderDevice* device, Vec2 from, Vec2 to, float line_width);
void render_device_draw_rect(RenderDevice* device, RectF rect, float line_width);
void render_device_draw_path(RenderDevice* device,
                             const Vec2* points,
                             int point_count,
                             RenderPathMode mode,
                             float line_width);
int render_device_read_pixels(RenderDevice* device,
                              RectF rect,
                              unsigned char* out_rgba,
                              int stride_bytes);
void render_device_end_frame(RenderDevice* device);
void render_device_destroy(RenderDevice* device);

#endif /* GLDRAW_RENDER_RENDER_DEVICE_H */
