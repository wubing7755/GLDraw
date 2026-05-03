#include <canvas/canvas_view.h>
#include <document/document.h>
#include <render/canvas_drawlist.h>
#include <render/canvas_renderer.h>

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
    RenderPathMode mode;
    float line_width;
} MockDrawCall;

typedef struct MockRenderDevice {
    RenderDevice base;
    char calls[16][16];
    int call_count;
    RectF clip_rect;
    RenderTransform transform;
    Color last_color;
    RenderFrameDesc frame_desc;
    MockDrawCall draw_calls[8];
    int draw_call_count;
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

static void mock_set_transform(RenderDevice* device, const RenderTransform* transform)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "transform");
    mock->transform = *transform;
}

static void mock_set_clip_rect(RenderDevice* device, RectF clip_rect)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "clip");
    mock->clip_rect = clip_rect;
}

static void mock_set_color(RenderDevice* device, Color color)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "color");
    mock->last_color = color;
}

static void mock_draw_line(RenderDevice* device, Vec2 from, Vec2 to, float line_width)
{
    (void)device;
    (void)from;
    (void)to;
    (void)line_width;
}

static void mock_draw_rect(RenderDevice* device, RectF rect, float line_width)
{
    (void)device;
    (void)rect;
    (void)line_width;
}

static void mock_draw_path(RenderDevice* device,
                           const Vec2* points,
                           int point_count,
                           RenderPathMode mode,
                           float line_width)
{
    MockRenderDevice* mock = (MockRenderDevice*)device;
    (void)points;
    snprintf(mock->calls[mock->call_count++], sizeof(mock->calls[0]), "path");
    mock->draw_calls[mock->draw_call_count].point_count = point_count;
    mock->draw_calls[mock->draw_call_count].mode = mode;
    mock->draw_calls[mock->draw_call_count].line_width = line_width;
    mock->draw_call_count++;
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
    (void)device;
}

static const RenderDeviceVTable MOCK_VTABLE = {
    mock_resize,
    mock_begin_frame,
    mock_set_transform,
    mock_set_clip_rect,
    mock_set_color,
    mock_draw_line,
    mock_draw_rect,
    mock_draw_path,
    mock_read_pixels,
    mock_end_frame,
    mock_destroy
};

static GraphicObject* make_rect(float x, float y, float w, float h)
{
    return object_create_rect((RectF){x, y, w, h}, object_default_style());
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

    canvas_drawlist_init(&draw_list);
    EXPECT_TRUE(canvas_drawlist_build(&draw_list, &document, &selection, &canvas, NULL));
    EXPECT_INT_EQ((int)draw_list.stroke_count, 1);

    mock.base.vtable = &MOCK_VTABLE;
    frame_desc.logical_width = 800;
    frame_desc.logical_height = 600;
    frame_desc.framebuffer_width = 800;
    frame_desc.framebuffer_height = 600;
    frame_desc.clear_color = draw_list.clear_color;

    EXPECT_TRUE(canvas_renderer_submit(&mock.base, &frame_desc, &transform, &draw_list));
    EXPECT_INT_EQ(mock.call_count, 6);
    EXPECT_TRUE(strcmp(mock.calls[0], "begin") == 0);
    EXPECT_TRUE(strcmp(mock.calls[1], "transform") == 0);
    EXPECT_TRUE(strcmp(mock.calls[2], "clip") == 0);
    EXPECT_TRUE(strcmp(mock.calls[3], "color") == 0);
    EXPECT_TRUE(strcmp(mock.calls[4], "path") == 0);
    EXPECT_TRUE(strcmp(mock.calls[5], "end") == 0);
    EXPECT_FLOAT_EQ(mock.clip_rect.w, 800.0f);
    EXPECT_FLOAT_EQ(mock.clip_rect.h, 600.0f);
    EXPECT_INT_EQ(mock.draw_calls[0].point_count, 5);
    EXPECT_INT_EQ((int)mock.draw_calls[0].mode, (int)RENDER_PATH_LINE_STRIP);

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
    EXPECT_TRUE(canvas_drawlist_build(&draw_list, &document, &selection, &canvas, NULL));
    EXPECT_INT_EQ((int)draw_list.stroke_count, 2);
    EXPECT_TRUE(draw_list.strokes[0].line_width > draw_list.strokes[1].line_width);

    canvas_drawlist_shutdown(&draw_list);
    document_shutdown(&document);
    return 0;
}

int main(void)
{
    if (test_canvas_renderer_submit_sequence()) return 1;
    if (test_selected_object_emits_highlight_first()) return 1;

    printf("[PASS] canvas draw list and renderer submission\n");
    return 0;
}
