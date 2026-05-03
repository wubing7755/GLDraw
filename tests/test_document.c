#include <document/document.h>
#include <document/object.h>

#include <stdio.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                     \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",               \
                    __FILE__, __LINE__, #expr);                              \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define EXPECT_INT_EQ(actual, expected)                                      \
    do {                                                                     \
        int actual_value = (actual);                                         \
        int expected_value = (expected);                                     \
        if (actual_value != expected_value) {                                \
            fprintf(stderr, "EXPECT_INT_EQ failed: %s:%d: %s=%d expected %d\n", \
                    __FILE__, __LINE__, #actual, actual_value, expected_value); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void)
{
    Document document;
    GraphicObject* object = NULL;

    document_init(&document);
    object = object_create_rect((RectF){0.0f, 0.0f, 10.0f, 12.0f},
                                object_default_style());

    EXPECT_TRUE(object != NULL);
    EXPECT_TRUE(document_add_object(&document, object));
    EXPECT_INT_EQ(document.count, 1);
    EXPECT_TRUE(document_find_object(&document, 1u) == object);

    document_shutdown(&document);
    printf("[PASS] document initializes and owns added object\n");
    return 0;
}
