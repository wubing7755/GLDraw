#ifndef GLDRAW_RENDER_RENDER_DEVICE_H
#define GLDRAW_RENDER_RENDER_DEVICE_H

#include <base/types.h>

typedef enum RenderPrimitiveType {
    RENDER_PRIMITIVE_LINES = 0,
    RENDER_PRIMITIVE_LINE_STRIP
} RenderPrimitiveType;

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

typedef struct RenderMaterial {
    Color color;
    float line_width;
} RenderMaterial;

typedef struct RenderGeometry {
    const Vec2* points;
    int point_count;
    RenderPrimitiveType primitive;
} RenderGeometry;

typedef struct RenderPass {
    RectF clip_rect;
    RenderTransform transform;
} RenderPass;

typedef struct RenderDevice RenderDevice;

typedef struct RenderDeviceVTable {
    int (*resize)(RenderDevice* device,
                  int logical_width,
                  int logical_height,
                  int framebuffer_width,
                  int framebuffer_height);
    int (*begin_frame)(RenderDevice* device, const RenderFrameDesc* frame_desc);
    int (*begin_pass)(RenderDevice* device, const RenderPass* pass);
    int (*draw_geometry)(RenderDevice* device,
                         const RenderGeometry* geometry,
                         const RenderMaterial* material);
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
int render_device_begin_pass(RenderDevice* device, const RenderPass* pass);
int render_device_draw_geometry(RenderDevice* device,
                                const RenderGeometry* geometry,
                                const RenderMaterial* material);
int render_device_read_pixels(RenderDevice* device,
                              RectF rect,
                              unsigned char* out_rgba,
                              int stride_bytes);
void render_device_end_frame(RenderDevice* device);
void render_device_destroy(RenderDevice* device);

#endif /* GLDRAW_RENDER_RENDER_DEVICE_H */
