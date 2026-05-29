#include <canvas/canvas_view.h>
#include <document/document_internal.h>
#include <render/canvas_drawlist.h>
#include <render/canvas_renderer.h>
#include <render/render_system.h>

#include <app/extension_loader.h>

#include <stdio.h>
#include <string.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                     \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",               \
                    __FILE__, __LINE__, #expr);                              \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define EXPECT_INT_EQ(actual, expected)                                       \
    do {                                                                      \
        int actual_value = (actual);                                          \
        int expected_value = (expected);                                      \
        if (actual_value != expected_value) {                                 \
            fprintf(stderr, "EXPECT_INT_EQ failed: %s:%d: %d != %d\n",        \
                    __FILE__, __LINE__, actual_value, expected_value);        \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_FLOAT_EQ(actual, expected)                                     \
    do {                                                                      \
        float actual_value = (actual);                                        \
        float expected_value = (expected);                                    \
        if ((actual_value - expected_value > 1e-4f) ||                        \
            (expected_value - actual_value > 1e-4f)) {                        \
            fprintf(stderr, "EXPECT_FLOAT_EQ failed: %s:%d: %.4f != %.4f\n",  \
                    __FILE__, __LINE__, actual_value, expected_value);        \
            return 1;                                                         \
        }                                                                     \
    } while (0)

typedef struct MockDrawCall {
    int point_count;
    RenderPrimitiveType primitive;
    float line_width;
    Vec2 first_point;
    Color color;
} MockDrawCall;

typedef struct MockRenderDevice {
    RenderDevice base;
    char calls[16][16];
    int call_count;
    RenderPass pass;
    RenderFrameDesc frame_desc;
    MockDrawCall draw_calls[8];
    int draw_call_count;
    int destroy_count;
} MockRenderDevice;

static int mock_resize(RenderDevice* device,
                       int logical_width,
                       int logical_height,
                       int framebuffer_width,
                       int framebuffer_height)
{
    (void)device;
    (void)logical_width;
    (void)logical_height;
    (void)framebuffer_width;
    (void)framebuffer_height;
    return 1;
}

static int mock_begin_frame(RenderDevice* device, const RenderFrameDesc* frame_desc)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "begin");
    mock->frame_desc = *frame_desc;
    return 1;
}

static int mock_begin_pass(RenderDevice* device, const RenderPass* pass)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "pass");
    mock->pass = *pass;
    return 1;
}

static int mock_draw_geometry(RenderDevice* device,
                              const RenderGeometry* geometry,
                              const RenderMaterial* material)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "draw");
    mock->draw_calls[mock->draw_call_count].point_count = geometry->point_count;
    mock->draw_calls[mock->draw_call_count].primitive = geometry->primitive;
    mock->draw_calls[mock->draw_call_count].line_width = material->line_width;
    mock->draw_calls[mock->draw_call_count].color = material->color;
    mock->draw_calls[mock->draw_call_count].first_point =
        geometry->point_count > 0 ? geometry->points[0] : (Vec2){0.0f, 0.0f};
    mock->draw_call_count++;
    return 1;
}

static int mock_read_pixels(RenderDevice* device,
                            RectF rect,
                            unsigned char* out_rgba,
                            int stride_bytes)
{
    (void)device;
    (void)rect;
    (void)out_rgba;
    (void)stride_bytes;
    return 1;
}

static void mock_end_frame(RenderDevice* device)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "end");
}

static void mock_destroy(RenderDevice* device)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    mock->destroy_count++;
}

static const RenderDeviceVTable MOCK_VTABLE = {
    mock_resize,
    mock_begin_frame,
    mock_begin_pass,
    mock_draw_geometry,
    mock_read_pixels,
    mock_end_frame,
    mock_destroy
};

static GraphicObject* make_rect(float x, float y, float w, float h)
{
    return object_create_rect((RectF){x, y, w, h}, object_default_style());
}

static void mock_reset_frame_capture(MockRenderDevice* mock)
{
    if (!mock) {
        return;
    }

    memset(mock->calls, 0, sizeof(mock->calls));
    memset(mock->draw_calls, 0, sizeof(mock->draw_calls));
    mock->call_count = 0;
    mock->draw_call_count = 0;
}

static const MockDrawCall* mock_find_widest_draw_call(const MockRenderDevice* mock)
{
    const MockDrawCall* widest = NULL;
    int i = 0;

    if (!mock || mock->draw_call_count <= 0) {
        return NULL;
    }

    widest = &mock->draw_calls[0];
    for (i = 1; i < mock->draw_call_count; ++i) {
        if (mock->draw_calls[i].line_width > widest->line_width) {
            widest = &mock->draw_calls[i];
        }
    }

    return widest;
}

static int test_canvas_renderer_submit_sequence(void)
{
    Document document;
    CanvasView canvas;
    SelectionSet selection = {0};
    CanvasDrawList draw_list;
    MockRenderDevice mock = {0};
    RenderFrameDesc frame_desc;
    RenderTransform transform = {1.0f, 1.0f};

    document_init(&document);
    canvas_view_init(&canvas, &document, (RectF){0.0f, 0.0f, 800.0f, 600.0f});
    canvas.show_grid = 0;
    EXPECT_TRUE(document_add_object(&document, make_rect(10.0f, 20.0f, 40.0f, 50.0f)));
    EXPECT_TRUE(document_add_object(&document, make_rect(80.0f, 20.0f, 40.0f, 50.0f)));

    canvas_drawlist_init(&draw_list);
    EXPECT_TRUE(canvas_drawlist_build(&draw_list,
                                      &document,
                                      &selection,
                                      &canvas,
                                      0,
                                      (Vec2){0.0f, 0.0f},
                                      NULL));
    EXPECT_INT_EQ((int)draw_list.stroke_count, 2);

    mock.base.vtable = &MOCK_VTABLE;
    frame_desc.logical_width = 800;
    frame_desc.logical_height = 600;
    frame_desc.framebuffer_width = 800;
    frame_desc.framebuffer_height = 600;
    frame_desc.clear_color = draw_list.clear_color;

    EXPECT_TRUE(canvas_renderer_submit(&mock.base, &frame_desc, &transform, &draw_list));
    EXPECT_INT_EQ(mock.call_count, 4);
    EXPECT_TRUE(strcmp(mock.calls[0], "begin") == 0);
    EXPECT_TRUE(strcmp(mock.calls[1], "pass") == 0);
    EXPECT_TRUE(strcmp(mock.calls[2], "draw") == 0);
    EXPECT_TRUE(strcmp(mock.calls[3], "end") == 0);
    EXPECT_INT_EQ(mock.draw_call_count, 1);
    EXPECT_FLOAT_EQ(mock.pass.clip_rect.w, 800.0f);
    EXPECT_FLOAT_EQ(mock.pass.clip_rect.h, 600.0f);
    EXPECT_FLOAT_EQ(mock.pass.transform.scale_x, 1.0f);
    EXPECT_FLOAT_EQ(mock.pass.transform.scale_y, 1.0f);
    EXPECT_INT_EQ(mock.draw_calls[0].point_count, 16);
    EXPECT_INT_EQ((int)mock.draw_calls[0].primitive, (int)RENDER_PRIMITIVE_LINES);

    canvas_drawlist_shutdown(&draw_list);
    document_shutdown(&document);
    return 0;
}

static int test_selected_object_emits_highlight_first(void)
{
    Document document;
    CanvasView canvas;
    SelectionSet selection = {0};
    CanvasDrawList draw_list;

    document_init(&document);
    canvas_view_init(&canvas, &document, (RectF){0.0f, 0.0f, 640.0f, 480.0f});
    canvas.show_grid = 0;
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 20.0f)));
    EXPECT_TRUE(selection_set_add(&selection, 1u));

    canvas_drawlist_init(&draw_list);
    EXPECT_TRUE(canvas_drawlist_build(&draw_list,
                                      &document,
                                      &selection,
                                      &canvas,
                                      0,
                                      (Vec2){0.0f, 0.0f},
                                      NULL));
    EXPECT_INT_EQ((int)draw_list.stroke_count, 2);
    EXPECT_TRUE(draw_list.strokes[0].line_width > draw_list.strokes[1].line_width);

    canvas_drawlist_shutdown(&draw_list);
    selection_set_shutdown(&selection);
    document_shutdown(&document);
    return 0;
}

static int test_render_system_invalidates_on_selection_revision(void)
{
    Document document;
    CanvasView canvas;
    SelectionSet selection = {0};
    RenderSceneDesc scene;
    MockRenderDevice mock = {0};
    PlatformWindow window = {0};
    RenderSystem* renderer = NULL;
    const MockDrawCall* first_highlight = NULL;
    const MockDrawCall* second_highlight = NULL;
    float first_highlight_x = 0.0f;
    float second_highlight_x = 0.0f;

    document_init(&document);
    canvas_view_init(&canvas, &document, (RectF){0.0f, 0.0f, 640.0f, 480.0f});
    canvas.show_grid = 0;
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 10.0f, 20.0f)));
    EXPECT_TRUE(document_add_object(&document, make_rect(100.0f, 0.0f, 10.0f, 20.0f)));
    EXPECT_TRUE(selection_set_add(&selection, 1u));

    mock.base.vtable = &MOCK_VTABLE;
    window.width = 640;
    window.height = 480;
    window.framebuffer_width = 640;
    window.framebuffer_height = 480;
    renderer = render_system_create(&mock.base, &window);
    EXPECT_TRUE(renderer != NULL);

    scene.document = &document;
    scene.selection = &selection;
    scene.canvas = &canvas;
    scene.selection_preview_active = 0;
    scene.selection_preview_delta = (Vec2){0.0f, 0.0f};
    scene.overlay_object = NULL;
    render_system_draw(renderer, &scene);
    first_highlight = mock_find_widest_draw_call(&mock);
    EXPECT_TRUE(first_highlight != NULL);
    first_highlight_x = first_highlight->first_point.x;

    mock_reset_frame_capture(&mock);
    selection_set_clear(&selection);
    EXPECT_TRUE(selection_set_add(&selection, 2u));
    render_system_draw(renderer, &scene);
    second_highlight = mock_find_widest_draw_call(&mock);
    EXPECT_TRUE(second_highlight != NULL);
    second_highlight_x = second_highlight->first_point.x;
    EXPECT_TRUE(first_highlight_x != second_highlight_x);

    render_system_destroy(renderer);
    EXPECT_INT_EQ(mock.destroy_count, 1);
    selection_set_shutdown(&selection);
    document_shutdown(&document);
    return 0;
}

static int test_render_system_desc_can_borrow_device(void)
{
    MockRenderDevice mock = {0};
    RenderSystemDesc desc;
    RenderSystem* renderer = NULL;

    mock.base.vtable = &MOCK_VTABLE;
    desc.logical_width = 320;
    desc.logical_height = 240;
    desc.framebuffer_width = 640;
    desc.framebuffer_height = 480;
    desc.owns_device = 0;

    renderer = render_system_create_with_desc(&mock.base, &desc);
    EXPECT_TRUE(renderer != NULL);
    render_system_destroy(renderer);
    EXPECT_INT_EQ(mock.destroy_count, 0);
    return 0;
}

static int test_canvas_drawlist_reuses_scratch_arena_between_builds(void)
{
    Document document;
    CanvasView canvas;
    SelectionSet selection = {0};
    CanvasDrawList draw_list;
    unsigned char* first_memory = NULL;
    size_t first_capacity = 0u;

    document_init(&document);
    canvas_view_init(&canvas, &document, (RectF){0.0f, 0.0f, 640.0f, 480.0f});
    canvas.show_grid = 1;
    EXPECT_TRUE(document_add_object(&document, make_rect(0.0f, 0.0f, 120.0f, 80.0f)));
    EXPECT_TRUE(document_add_object(&document, make_rect(240.0f, 120.0f, 100.0f, 60.0f)));

    canvas_drawlist_init(&draw_list);
    EXPECT_TRUE(canvas_drawlist_build(&draw_list,
                                      &document,
                                      &selection,
                                      &canvas,
                                      0,
                                      (Vec2){0.0f, 0.0f},
                                      NULL));
    EXPECT_TRUE(draw_list.scratch_arena.memory != NULL);
    EXPECT_TRUE(draw_list.scratch_arena.capacity_bytes > 0u);
    first_memory = draw_list.scratch_arena.memory;
    first_capacity = draw_list.scratch_arena.capacity_bytes;

    EXPECT_TRUE(canvas_drawlist_build(&draw_list,
                                      &document,
                                      &selection,
                                      &canvas,
                                      0,
                                      (Vec2){0.0f, 0.0f},
                                      NULL));
    EXPECT_TRUE(draw_list.scratch_arena.memory == first_memory);
    EXPECT_TRUE(draw_list.scratch_arena.capacity_bytes == first_capacity);

    canvas_drawlist_shutdown(&draw_list);
    document_shutdown(&document);
    return 0;
}

int main(void)
{
    extension_loader_register_all();

    if (test_canvas_renderer_submit_sequence()) return 1;
    if (test_selected_object_emits_highlight_first()) return 1;
    if (test_render_system_invalidates_on_selection_revision()) return 1;
    if (test_render_system_desc_can_borrow_device()) return 1;
    if (test_canvas_drawlist_reuses_scratch_arena_between_builds()) return 1;

    printf("[PASS] canvas draw list and renderer submission\n");
    return 0;
}
