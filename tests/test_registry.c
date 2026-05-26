#include <app/command_catalog.h>
#include <app/command_types.h>
#include <app/workspace_internal.h>
#include <commands/command.h>
#include <input/keymap.h>
#include <tools/tool.h>
#include <ui/editor_viewmodel.h>

#include "../src/ui/ui_menu_def.h"

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

static const ToolDescriptor G_FAKE_TOOL = {
    "fake-tool",
    "Fake Tool",
    "tool.fake",
    "Synthetic registry tool",
    "Fake",
    "F",
    1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static int register_fake_tool_once(void)
{
    if (tool_registry_lookup(G_FAKE_TOOL.id)) {
        return 1;
    }
    return register_tool(&G_FAKE_TOOL);
}

static int find_fake_tool_index(void)
{
    int i = 0;

    for (i = 0; i < tool_registry_count(); ++i) {
        const ToolDescriptor* descriptor = tool_registry_at(i);
        if (descriptor && strcmp(descriptor->id, G_FAKE_TOOL.id) == 0) {
            return i;
        }
    }

    return -1;
}

static int test_dynamic_tool_command_and_menu_registration(void)
{
    const CommandDescriptor* command_descriptor = NULL;
    const CommandDescriptor* menu_descriptor = NULL;
    const MenuItemDef* menu_items = NULL;
    int fake_tool_index = 0;
    int menu_count = 0;
    int i = 0;
    int found_menu_item = 0;

    EXPECT_TRUE(register_fake_tool_once());
    fake_tool_index = find_fake_tool_index();
    EXPECT_TRUE(fake_tool_index >= 0);

    command_descriptor = command_catalog_find_by_id("tool.fake");
    EXPECT_TRUE(command_descriptor != NULL);
    EXPECT_TRUE(command_descriptor->command >= EDITOR_COMMAND_DYNAMIC_TOOL_BASE);
    EXPECT_STR_EQ(command_descriptor->tool_id, "fake-tool");

    menu_descriptor = command_catalog_find_by_menu_id(MENU_ID_TOOL_DYNAMIC_BASE + fake_tool_index);
    EXPECT_TRUE(menu_descriptor != NULL);
    EXPECT_TRUE(menu_descriptor->command == command_descriptor->command);

    menu_items = ui_menu_def_items();
    menu_count = ui_menu_def_count();
    for (i = 0; i < menu_count; ++i) {
        if (menu_items[i].id == MENU_ID_TOOL_DYNAMIC_BASE + fake_tool_index &&
            menu_items[i].parent_id == MENU_ID_EDIT) {
            found_menu_item = 1;
            EXPECT_STR_EQ(menu_items[i].label, "Fake Tool");
            break;
        }
    }
    EXPECT_TRUE(found_menu_item);

    return 0;
}

static int test_dynamic_tool_shortcut_and_availability(void)
{
    Workspace workspace;
    EditorViewModel view_model = {0};
    LayerId locked_layer = 0u;
    EditorKeymap keymap;
    char shortcut[64];
    int i = 0;
    int found_tool = 0;

    memset(&workspace, 0, sizeof(workspace));
    document_init(&workspace.core.document);
    EXPECT_TRUE(command_executor_init(&workspace.core.commands));
    EXPECT_TRUE(register_fake_tool_once());
    keymap_init(&keymap, "registry.test.keymap.json");

    shortcut[0] = '\0';
    keymap_format_command_shortcut(&keymap, "tool.fake", KEY_SCOPE_GLOBAL, shortcut, sizeof(shortcut));
    EXPECT_STR_EQ(shortcut, "F");

    workspace.session.keymap = keymap;
    tool_controller_init(&workspace.core.tools);

    locked_layer = document_create_layer(&workspace.core.document, "Locked");
    EXPECT_TRUE(locked_layer != 0u);
    EXPECT_TRUE(document_set_active_layer(&workspace.core.document, locked_layer));
    EXPECT_TRUE(document_set_layer_locked(&workspace.core.document, locked_layer, 1));

    EXPECT_TRUE(editor_viewmodel_build(&view_model, &workspace));
    for (i = 0; i < view_model.tool_count; ++i) {
        if (strcmp(view_model.tools[i].id, "fake-tool") == 0) {
            found_tool = 1;
            EXPECT_TRUE(view_model.tools[i].available == 0);
            EXPECT_STR_EQ(view_model.tools[i].shortcut, "F");
            EXPECT_STR_EQ(view_model.tools[i].unavailable_reason, "Active layer is locked.");
            break;
        }
    }
    EXPECT_TRUE(found_tool);

    editor_viewmodel_shutdown(&view_model);
    tool_controller_shutdown(&workspace.core.tools);
    keymap_shutdown(&workspace.session.keymap);
    command_executor_shutdown(&workspace.core.commands);
    document_shutdown(&workspace.core.document);
    return 0;
}

int main(void)
{
    if (test_dynamic_tool_command_and_menu_registration()) return 1;
    if (test_dynamic_tool_shortcut_and_availability()) return 1;

    printf("[PASS] dynamic tool registry integration\n");
    return 0;
}
