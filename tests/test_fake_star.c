#include <app/extension_loader.h>
#include <canvas/canvas_view.h>
#include <document/document.h>
#include <document/object.h>
#include <document/persistence.h>

#include <stdio.h>
#include <string.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                      \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",                \
                    __FILE__, __LINE__, #expr);                               \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define EXPECT_STR_EQ(actual, expected)                                       \
    do {                                                                      \
        if (strcmp((actual), (expected)) != 0) {                              \
            fprintf(stderr, "EXPECT_STR_EQ failed: %s:%d: \"%s\" != \"%s\"\n",\
                    __FILE__, __LINE__, (actual), (expected));                \
            return 1;                                                         \
        }                                                                     \
    } while (0)

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

    return snprintf(buffer, buffer_size, "%s%s", base_name, suffix) < (int)buffer_size;
}

int main(void)
{
    Document document;
    Document loaded;
    CanvasView canvas;
    GraphicObject* created = NULL;
    GraphicObject* picked = NULL;
    RectF bounds = {-50.0f, -50.0f, 100.0f, 100.0f};
    char path[L_tmpnam + 16];

    EXPECT_TRUE(extension_loader_register_fake_star());

    document_init(&document);
    document_init(&loaded);
    canvas_view_init(&canvas, &document, (RectF){0.0f, 0.0f, 800.0f, 600.0f});

    created = object_create("fake_star", &bounds, object_default_style());
    EXPECT_TRUE(created != NULL);
    EXPECT_TRUE(document_add_object(&document, created));
    EXPECT_STR_EQ(object_type_id(document.objects[0]), "fake_star");
    EXPECT_TRUE(object_get_path_point_count(document.objects[0]) == 11);

    picked = canvas_view_pick_object(&canvas, (Vec2){400.0f, 300.0f}, 4.0f);
    EXPECT_TRUE(picked != NULL);
    EXPECT_STR_EQ(object_type_id(picked), "fake_star");

    EXPECT_TRUE(make_temp_path(path, sizeof(path), ".json"));
    EXPECT_TRUE(document_save_json(&document, path));
    EXPECT_TRUE(document_load_json(&loaded, path));
    EXPECT_TRUE(loaded.count == 1);
    EXPECT_STR_EQ(object_type_id(loaded.objects[0]), "fake_star");

    remove(path);
    document_shutdown(&loaded);
    document_shutdown(&document);
    printf("[PASS] fake star extension loader registration\n");
    return 0;
}
