#ifndef GLDRAW_RENDER_RENDER_DEVICE_FACTORY_H
#define GLDRAW_RENDER_RENDER_DEVICE_FACTORY_H

#include <platform/window.h>
#include <render/render_device.h>

RenderDevice* render_device_factory_create_gl(PlatformWindow* window);

#endif /* GLDRAW_RENDER_RENDER_DEVICE_FACTORY_H */
