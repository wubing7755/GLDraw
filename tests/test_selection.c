#include <model/selection.h>

#include <stdio.h>

#define EXPECT_TRUE(expr)                                                     \
    do {                                                                     \
        if (!(expr)) {                                                        \
            fprintf(stderr, "EXPECT_TRUE failed: %s:%d: %s\n",               \
                    __FILE__, __LINE__, #expr);                              \
            return 1;                                                        \
        }                                                                    \
    } while (0)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))
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
    SelectionSet selection = {0};

    EXPECT_TRUE(selection_set_add(&selection, 7u));
    EXPECT_TRUE(selection_set_contains(&selection, 7u));
    EXPECT_INT_EQ(selection.count, 1);

    selection_set_toggle(&selection, 7u);
    EXPECT_FALSE(selection_set_contains(&selection, 7u));
    EXPECT_INT_EQ(selection.count, 0);

    selection_set_shutdown(&selection);
    printf("[PASS] selection helpers add and toggle IDs\n");
    return 0;
}
