#include <model/selection.h>

#include <inttypes.h>
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
#define EXPECT_UINT64_EQ(actual, expected)                                     \
    do {                                                                       \
        uint64_t actual_value = (actual);                                      \
        uint64_t expected_value = (expected);                                  \
        if (actual_value != expected_value) {                                  \
            fprintf(stderr,                                                     \
                    "EXPECT_UINT64_EQ failed: %s:%d: %s=%" PRIu64              \
                    " expected %" PRIu64 "\n",                                 \
                    __FILE__, __LINE__, #actual, actual_value, expected_value); \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void)
{
    SelectionSet selection = {0};
    SelectionSet copy = {0};

    EXPECT_UINT64_EQ(selection.revision, 0u);
    EXPECT_TRUE(selection_set_add(&selection, 7u));
    EXPECT_UINT64_EQ(selection.revision, 1u);
    EXPECT_TRUE(selection_set_contains(&selection, 7u));
    EXPECT_INT_EQ(selection.count, 1);

    EXPECT_TRUE(selection_set_add(&selection, 7u));
    EXPECT_UINT64_EQ(selection.revision, 1u);

    selection_set_toggle(&selection, 7u);
    EXPECT_UINT64_EQ(selection.revision, 2u);
    EXPECT_FALSE(selection_set_contains(&selection, 7u));
    EXPECT_INT_EQ(selection.count, 0);

    EXPECT_TRUE(selection_set_add(&selection, 9u));
    EXPECT_UINT64_EQ(selection.revision, 3u);
    EXPECT_TRUE(selection_set_copy(&copy, &selection));
    EXPECT_UINT64_EQ(copy.revision, 1u);
    EXPECT_TRUE(selection_set_copy(&copy, &selection));
    EXPECT_UINT64_EQ(copy.revision, 1u);

    selection_set_clear(&selection);
    EXPECT_UINT64_EQ(selection.revision, 4u);

    selection_set_shutdown(&copy);
    selection_set_shutdown(&selection);
    printf("[PASS] selection helpers track content revisions\n");
    return 0;
}
