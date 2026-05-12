#include <app/extension_loader.h>
#include <commands/command.h>
#include <document/document.h>
#include <script/script_runtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            fprintf(stderr, "EXPECT_INT_EQ failed: %s:%d: %d != %d\n",       \
                    __FILE__, __LINE__, actual_value, expected_value);       \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct {
    CommandExecutor* executor;
} ScriptRuntimeTestPorts;

static int test_ports_execute_command(void* user, Command* command, Document* document)
{
    ScriptRuntimeTestPorts* ports = (ScriptRuntimeTestPorts*)user;

    if (!ports || !ports->executor || !document || !command) {
        return 0;
    }

    return command_executor_execute(ports->executor, command, document);
}

static int test_ports_undo(void* user, Document* document)
{
    ScriptRuntimeTestPorts* ports = (ScriptRuntimeTestPorts*)user;

    if (!ports || !ports->executor || !document) {
        return 0;
    }

    return command_executor_undo(ports->executor, document);
}

static int test_ports_redo(void* user, Document* document)
{
    ScriptRuntimeTestPorts* ports = (ScriptRuntimeTestPorts*)user;

    if (!ports || !ports->executor || !document) {
        return 0;
    }

    return command_executor_redo(ports->executor, document);
}

static int make_temp_path(char* buffer, size_t buffer_size, const char* suffix)
{
    char base_name[L_tmpnam] = {0};

#ifdef _WIN32
    if (tmpnam_s(base_name, sizeof(base_name)) != 0) {
        return 0;
    }
#else
    if (!tmpnam(base_name)) {
        return 0;
    }
#endif

    if (snprintf(buffer, buffer_size, "%s%s", base_name, suffix) >= (int)buffer_size) {
        return 0;
    }

    return 1;
}

static int write_script_file(const char* path, const char* source)
{
    FILE* file = NULL;

    if (!path || !source) {
        return 0;
    }

    file = fopen(path, "wb");
    if (!file) {
        return 0;
    }
    if (fputs(source, file) < 0) {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

static int test_script_runtime_uses_safe_env_and_caches_script(void)
{
    static const char* script_source =
        "local gld = gldraw\n"
        "gld.load_count = (gld.load_count or 0) + 1\n"
        "if os ~= nil or io ~= nil or package ~= nil or debug ~= nil or dofile ~= nil or loadfile ~= nil or require ~= nil then\n"
        "    error('unsafe lua libraries are exposed')\n"
        "end\n"
        "function on_event(event)\n"
        "    if event.name ~= 'pointer_up' then\n"
        "        return true\n"
        "    end\n"
        "    if gld.load_count ~= 1 then\n"
        "        error('script should be loaded once')\n"
        "    end\n"
        "    return gld.document_add_object('rect', {\n"
        "        x = event.x,\n"
        "        y = event.y,\n"
        "        width = 10.0,\n"
        "        height = 5.0,\n"
        "        stroke_width = 1.0\n"
        "    }) ~= nil\n"
        "end\n";
    Document document;
    CommandExecutor executor;
    SelectionSet selection = {0};
    ScriptRuntime runtime;
    ScriptRuntimeTestPorts ports;
    ToolPorts tool_ports;
    ToolEvent event = {0};
    char script_path[512] = {0};

    extension_loader_register_all();
    document_init(&document);
    EXPECT_TRUE(command_executor_init(&executor));
    EXPECT_TRUE(script_runtime_init(&runtime));
    ports.executor = &executor;
    memset(&tool_ports, 0, sizeof(tool_ports));
    tool_ports.execute_command = test_ports_execute_command;
    tool_ports.undo = test_ports_undo;
    tool_ports.redo = test_ports_redo;
    tool_ports.user = &ports;

    EXPECT_TRUE(make_temp_path(script_path, sizeof(script_path), ".lua"));
    EXPECT_TRUE(write_script_file(script_path, script_source));

    script_runtime_set_context(&runtime, &document, &selection, &tool_ports);
    event.world_pos.x = 12.0f;
    event.world_pos.y = 34.0f;

    EXPECT_TRUE(script_runtime_execute_file_event(&runtime, script_path, "pointer_up", &event));
    EXPECT_TRUE(script_runtime_execute_file_event(&runtime, script_path, "pointer_up", &event));
    EXPECT_INT_EQ(document.count, 2);

    remove(script_path);
    script_runtime_shutdown(&runtime);
    command_executor_shutdown(&executor);
    selection_set_shutdown(&selection);
    document_shutdown(&document);
    return 0;
}

int main(void)
{
    if (test_script_runtime_uses_safe_env_and_caches_script()) return 1;

    printf("[PASS] script runtime uses safe env and caches scripts\n");
    return 0;
}
