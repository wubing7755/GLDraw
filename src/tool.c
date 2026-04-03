#include <core/tool.h>

const char* tool_name(const Tool* t)
{
    if (!t || !t->vtable || !t->vtable->name) {
        return "unknown";
    }
    return t->vtable->name(t);
}
