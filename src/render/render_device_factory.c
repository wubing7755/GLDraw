#include <render/render_device_factory.h>

#include "render_device_gl.h"

RenderDevice* render_device_factory_create_gl(PlatformWindow* window)
{
    return gl_render_device_create(window);
}
