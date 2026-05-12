#include <script/script_runtime.h>

#include <document/object.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdio.h>
#include <string.h>

#define GLDRAW_SCRIPT_MAX_PATH 260

struct ScriptRuntime {
    lua_State* state;
    Document* document;
    SelectionSet* selection;
    ToolPorts ports;
    int environment_ref;
    int script_loaded;
    char loaded_script_path[GLDRAW_SCRIPT_MAX_PATH];
};

static ScriptRuntime* script_runtime_from_lua(lua_State* state)
{
    return (ScriptRuntime*)lua_touserdata(state, lua_upvalueindex(1));
}

static void script_runtime_env_copy_global(lua_State* state, int env_index, const char* name)
{
    env_index = lua_absindex(state, env_index);
    lua_getglobal(state, name);
    lua_setfield(state, env_index, name);
}

static int script_runtime_execute_command(ScriptRuntime* runtime,
                                          Command* command,
                                          const char** out_error)
{
    CommandExecuteCheck check = COMMAND_EXECUTE_CHECK_INVALID_ARGUMENT;

    if (!runtime || !runtime->document || !runtime->ports.execute_command || !command) {
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

    if (!runtime->ports.execute_command(runtime->ports.user, command, runtime->document)) {
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

    if (!runtime || !runtime->document || !runtime->ports.execute_command || !type_id) {
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

static int script_api_command_undo(lua_State* state)
{
    ScriptRuntime* runtime = script_runtime_from_lua(state);

    if (!runtime || !runtime->document || !runtime->ports.undo) {
        lua_pushboolean(state, 0);
        return 1;
    }

    lua_pushboolean(state, runtime->ports.undo(runtime->ports.user, runtime->document));
    return 1;
}

static int script_api_command_redo(lua_State* state)
{
    ScriptRuntime* runtime = script_runtime_from_lua(state);

    if (!runtime || !runtime->document || !runtime->ports.redo) {
        lua_pushboolean(state, 0);
        return 1;
    }

    lua_pushboolean(state, runtime->ports.redo(runtime->ports.user, runtime->document));
    return 1;
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
    lua_State* state = NULL;

    if (!runtime || !runtime->state) {
        return 0;
    }

    state = runtime->state;
    lua_newtable(state);
    lua_pushvalue(state, -1);
    lua_setfield(state, -2, "_G");

    script_runtime_env_copy_global(state, -1, "assert");
    script_runtime_env_copy_global(state, -1, "error");
    script_runtime_env_copy_global(state, -1, "ipairs");
    script_runtime_env_copy_global(state, -1, "next");
    script_runtime_env_copy_global(state, -1, "pairs");
    script_runtime_env_copy_global(state, -1, "pcall");
    script_runtime_env_copy_global(state, -1, "select");
    script_runtime_env_copy_global(state, -1, "tonumber");
    script_runtime_env_copy_global(state, -1, "tostring");
    script_runtime_env_copy_global(state, -1, "type");
    script_runtime_env_copy_global(state, -1, "xpcall");
    script_runtime_env_copy_global(state, -1, "math");
    script_runtime_env_copy_global(state, -1, "string");
    script_runtime_env_copy_global(state, -1, "table");

    lua_newtable(state);

    lua_pushlightuserdata(state, runtime);
    lua_pushcclosure(state, script_api_document_add_object, 1);
    lua_setfield(state, -2, "document_add_object");

    lua_pushlightuserdata(state, runtime);
    lua_pushcclosure(state, script_api_command_undo, 1);
    lua_setfield(state, -2, "undo");

    lua_pushlightuserdata(state, runtime);
    lua_pushcclosure(state, script_api_command_redo, 1);
    lua_setfield(state, -2, "redo");

    lua_pushlightuserdata(state, runtime);
    lua_pushcclosure(state, script_api_selection_get_ids, 1);
    lua_setfield(state, -2, "selection_get_ids");

    lua_setfield(state, -2, "gldraw");
    runtime->environment_ref = luaL_ref(state, LUA_REGISTRYINDEX);
    return 1;
}

static int script_runtime_reset_state(ScriptRuntime* runtime)
{
    if (!runtime) {
        return 0;
    }

    if (runtime->state) {
        lua_close(runtime->state);
        runtime->state = NULL;
    }
    runtime->environment_ref = LUA_NOREF;
    runtime->script_loaded = 0;
    runtime->loaded_script_path[0] = '\0';

    runtime->state = luaL_newstate();
    if (!runtime->state) {
        return 0;
    }
    luaL_requiref(runtime->state, "_G", luaopen_base, 1);
    lua_pop(runtime->state, 1);
    luaL_requiref(runtime->state, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(runtime->state, 1);
    luaL_requiref(runtime->state, LUA_STRLIBNAME, luaopen_string, 1);
    lua_pop(runtime->state, 1);
    luaL_requiref(runtime->state, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(runtime->state, 1);

    return script_runtime_register_api(runtime);
}

int script_runtime_init(ScriptRuntime* runtime)
{
    if (!runtime) {
        return 0;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->environment_ref = LUA_NOREF;
    return script_runtime_reset_state(runtime);
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
                                const ToolPorts* ports)
{
    if (!runtime) {
        return;
    }

    runtime->document = document;
    runtime->selection = selection;
    runtime->ports = ports ? *ports : (ToolPorts){0};
}

static int script_runtime_ensure_script_loaded(ScriptRuntime* runtime, const char* script_path)
{
    if (!runtime || !runtime->state || !script_path || script_path[0] == '\0') {
        return 0;
    }
    if (runtime->script_loaded &&
        strcmp(runtime->loaded_script_path, script_path) == 0) {
        return 1;
    }
    if (!script_runtime_reset_state(runtime)) {
        return 0;
    }
    if (luaL_loadfile(runtime->state, script_path) != LUA_OK) {
        lua_pop(runtime->state, 1);
        return 0;
    }

    lua_rawgeti(runtime->state, LUA_REGISTRYINDEX, runtime->environment_ref);
    if (!lua_setupvalue(runtime->state, -2, 1)) {
        lua_pop(runtime->state, 1);
        return 0;
    }
    if (lua_pcall(runtime->state, 0, 0, 0) != LUA_OK) {
        lua_pop(runtime->state, 1);
        return 0;
    }

    snprintf(runtime->loaded_script_path,
             sizeof(runtime->loaded_script_path),
             "%s",
             script_path);
    runtime->script_loaded = 1;
    return 1;
}

int script_runtime_execute_file_event(ScriptRuntime* runtime,
                                      const char* script_path,
                                      const char* event_name,
                                      const ToolEvent* event)
{
    int call_result = 0;
    int success = 1;

    if (!runtime || !runtime->state || !script_path || !event_name || !event) {
        return 0;
    }
    if (!script_runtime_ensure_script_loaded(runtime, script_path)) {
        return 0;
    }

    lua_rawgeti(runtime->state, LUA_REGISTRYINDEX, runtime->environment_ref);
    lua_getfield(runtime->state, -1, "on_event");
    if (!lua_isfunction(runtime->state, -1)) {
        lua_pop(runtime->state, 2);
        return 0;
    }
    lua_remove(runtime->state, -2);

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

    if (lua_isboolean(runtime->state, -1)) {
        success = lua_toboolean(runtime->state, -1) != 0;
    }

    lua_pop(runtime->state, 1);
    return success;
}
