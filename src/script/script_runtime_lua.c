#include <script/script_runtime.h>

#include <document/object.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <string.h>

struct ScriptRuntime {
    lua_State* state;
    Document* document;
    SelectionSet* selection;
    CommandExecutor* executor;
};

static ScriptRuntime* script_runtime_from_lua(lua_State* state)
{
    return (ScriptRuntime*)lua_touserdata(state, lua_upvalueindex(1));
}

static int script_runtime_execute_command(ScriptRuntime* runtime,
                                          Command* command,
                                          const char** out_error)
{
    CommandExecuteCheck check = COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;

    if (!runtime || !runtime->document || !runtime->executor || !command) {
        if (out_error) {
            *out_error = command_execute_check_message(COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT);
        }
        return 0;
    }

    check = command_check_execute(command, runtime->document);
    if (check != COMMAND_EXECUTE_CHECK_OK) {
        if (out_error) {
            *out_error = command_execute_check_message(check);
        }
        if (command->vtable && command->vtable->destroy) {
            command->vtable->destroy(command);
        }
        return 0;
    }

    if (!command_executor_execute(runtime->executor, command, runtime->document)) {
        if (out_error) {
            *out_error = "Command execution failed.";
        }
        return 0;
    }

    if (out_error) {
        *out_error = NULL;
    }
    return 1;
}

static int script_runtime_push_style(lua_State* state, GraphicStyle* style, int index)
{
    if (!lua_istable(state, index)) {
        return 1;
    }

    lua_getfield(state, index, "stroke_width");
    if (lua_isnumber(state, -1)) {
        style->stroke_width = (float)lua_tonumber(state, -1);
    }
    lua_pop(state, 1);
    return 1;
}

static int script_runtime_build_bag_from_table(lua_State* state,
                                               int index,
                                               GraphicPropertyBag* bag)
{
    if (!lua_istable(state, index)) {
        return 0;
    }

    graphic_property_bag_init(bag);
    lua_pushnil(state);
    while (lua_next(state, index) != 0) {
        if (lua_type(state, -2) == LUA_TSTRING && lua_isnumber(state, -1)) {
            const char* key = lua_tostring(state, -2);
            float value = (float)lua_tonumber(state, -1);
            if (!graphic_property_bag_set(bag, key, value)) {
                lua_pop(state, 1);
                return 0;
            }
        }
        lua_pop(state, 1);
    }

    return 1;
}

static int script_api_document_add_object(lua_State* state)
{
    ScriptRuntime* runtime = script_runtime_from_lua(state);
    const char* type_id = luaL_checkstring(state, 1);
    const char* error_message = NULL;
    GraphicStyle style = object_default_style();
    GraphicPropertyBag bag;
    const GraphicObjectDescriptor* descriptor = NULL;
    GraphicObject* object = NULL;
    Command* command = NULL;

    if (!runtime || !runtime->document || !runtime->executor || !type_id) {
        lua_pushnil(state);
        return 1;
    }

    descriptor = object_registry_lookup(type_id);
    if (!descriptor || !descriptor->deserialize) {
        lua_pushnil(state);
        return 1;
    }

    graphic_property_bag_init(&bag);
    if (lua_gettop(state) >= 2) {
        if (!script_runtime_build_bag_from_table(state, 2, &bag)) {
            lua_pushnil(state);
            return 1;
        }
        script_runtime_push_style(state, &style, 2);
    }

    object = descriptor->deserialize(&bag, style);
    if (!object) {
        lua_pushnil(state);
        return 1;
    }

    command = command_create_create_object(object);
    if (!command || !script_runtime_execute_command(runtime, command, &error_message)) {
        lua_pushnil(state);
        lua_pushstring(state, error_message ? error_message : "Command creation failed.");
        return 2;
    }

    lua_pushinteger(state, (lua_Integer)object->id);
    return 1;
}

static int script_api_command_execute(lua_State* state)
{
    ScriptRuntime* runtime = script_runtime_from_lua(state);
    const char* text = luaL_checkstring(state, 1);

    if (!runtime || !runtime->document || !runtime->executor || !text) {
        lua_pushboolean(state, 0);
        return 1;
    }

    if (strstr(text, "undo") != NULL) {
        lua_pushboolean(state, command_executor_undo(runtime->executor, runtime->document));
        return 1;
    }
    if (strstr(text, "redo") != NULL) {
        lua_pushboolean(state, command_executor_redo(runtime->executor, runtime->document));
        return 1;
    }

    lua_pushboolean(state, 0);
    return 1;
}

static int script_runtime_execute_command_text(ScriptRuntime* runtime, const char* text)
{
    if (!runtime || !runtime->document || !runtime->executor || !text) {
        return 0;
    }
    if (strstr(text, "undo") != NULL) {
        return command_executor_undo(runtime->executor, runtime->document);
    }
    if (strstr(text, "redo") != NULL) {
        return command_executor_redo(runtime->executor, runtime->document);
    }
    return 0;
}

static int script_api_selection_get_ids(lua_State* state)
{
    ScriptRuntime* runtime = script_runtime_from_lua(state);
    int i = 0;

    lua_newtable(state);
    if (!runtime || !runtime->selection) {
        return 1;
    }

    for (i = 0; i < runtime->selection->count; ++i) {
        lua_pushinteger(state, i + 1);
        lua_pushinteger(state, (lua_Integer)runtime->selection->ids[i]);
        lua_settable(state, -3);
    }

    return 1;
}

static int script_runtime_register_api(ScriptRuntime* runtime)
{
    if (!runtime || !runtime->state) {
        return 0;
    }

    lua_pushlightuserdata(runtime->state, runtime);
    lua_pushcclosure(runtime->state, script_api_document_add_object, 1);
    lua_setglobal(runtime->state, "gldraw_document_add_object");

    lua_pushlightuserdata(runtime->state, runtime);
    lua_pushcclosure(runtime->state, script_api_command_execute, 1);
    lua_setglobal(runtime->state, "gldraw_command_execute");

    lua_pushlightuserdata(runtime->state, runtime);
    lua_pushcclosure(runtime->state, script_api_selection_get_ids, 1);
    lua_setglobal(runtime->state, "gldraw_selection_get_ids");
    return 1;
}

int script_runtime_init(ScriptRuntime* runtime)
{
    if (!runtime) {
        return 0;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->state = luaL_newstate();
    if (!runtime->state) {
        return 0;
    }
    luaL_openlibs(runtime->state);
    return script_runtime_register_api(runtime);
}

void script_runtime_shutdown(ScriptRuntime* runtime)
{
    if (!runtime) {
        return;
    }

    if (runtime->state) {
        lua_close(runtime->state);
    }
    memset(runtime, 0, sizeof(*runtime));
}

void script_runtime_set_context(ScriptRuntime* runtime,
                                Document* document,
                                SelectionSet* selection,
                                CommandExecutor* executor)
{
    if (!runtime) {
        return;
    }

    runtime->document = document;
    runtime->selection = selection;
    runtime->executor = executor;
}

int script_runtime_execute_file_event(ScriptRuntime* runtime,
                                      const char* script_path,
                                      const char* event_name,
                                      const ToolEvent* event)
{
    int call_result = 0;

    if (!runtime || !runtime->state || !script_path || !event_name || !event) {
        return 0;
    }

    if (luaL_dofile(runtime->state, script_path) != LUA_OK) {
        lua_pop(runtime->state, 1);
        return 0;
    }

    lua_getglobal(runtime->state, "on_event");
    if (!lua_isfunction(runtime->state, -1)) {
        lua_pop(runtime->state, 1);
        return 0;
    }

    lua_newtable(runtime->state);
    lua_pushstring(runtime->state, event_name);
    lua_setfield(runtime->state, -2, "name");
    lua_pushnumber(runtime->state, event->screen_pos.x);
    lua_setfield(runtime->state, -2, "screen_x");
    lua_pushnumber(runtime->state, event->screen_pos.y);
    lua_setfield(runtime->state, -2, "screen_y");
    lua_pushnumber(runtime->state, event->world_pos.x);
    lua_setfield(runtime->state, -2, "x");
    lua_pushnumber(runtime->state, event->world_pos.y);
    lua_setfield(runtime->state, -2, "y");
    lua_pushinteger(runtime->state, event->button);
    lua_setfield(runtime->state, -2, "button");
    lua_pushinteger(runtime->state, event->mods);
    lua_setfield(runtime->state, -2, "mods");
    lua_pushnumber(runtime->state, event->wheel_y);
    lua_setfield(runtime->state, -2, "wheel_y");

    call_result = lua_pcall(runtime->state, 1, 1, 0);
    if (call_result != LUA_OK) {
        lua_pop(runtime->state, 1);
        return 0;
    }

    if (lua_isstring(runtime->state, -1)) {
        const char* command_text = lua_tostring(runtime->state, -1);
        if (command_text) {
            lua_pop(runtime->state, 1);
            return script_runtime_execute_command_text(runtime, command_text);
        }
    }

    lua_pop(runtime->state, 1);
    return 1;
}
