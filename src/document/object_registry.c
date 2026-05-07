#include <document/object.h>

#include <stdlib.h>
#include <string.h>

#define OBJECT_REGISTRY_INITIAL_CAPACITY 8

typedef struct {
    GraphicObjectDescriptor* descriptors;
    int count;
    int capacity;
    unsigned int next_dynamic_type;
    int initialized;
} ObjectRegistryState;

static ObjectRegistryState g_registry = {0};

static GraphicObjectDescriptor* object_registry_alloc_slot(void)
{
    if (g_registry.count >= g_registry.capacity) {
        int new_capacity = g_registry.capacity > 0 ? g_registry.capacity * 2
                                                   : OBJECT_REGISTRY_INITIAL_CAPACITY;
        GraphicObjectDescriptor* new_descriptors =
            (GraphicObjectDescriptor*)realloc(g_registry.descriptors,
                                              (size_t)new_capacity *
                                                  sizeof(g_registry.descriptors[0]));
        if (!new_descriptors) {
            return NULL;
        }
        g_registry.descriptors = new_descriptors;
        g_registry.capacity = new_capacity;
    }

    return &g_registry.descriptors[g_registry.count++];
}

static const GraphicObjectDescriptor* object_registry_find_by_type_id(const char* type_id)
{
    int i = 0;

    if (!type_id || type_id[0] == '\0') {
        return NULL;
    }

    for (i = 0; i < g_registry.count; ++i) {
        if (g_registry.descriptors[i].type_id &&
            strcmp(g_registry.descriptors[i].type_id, type_id) == 0) {
            return &g_registry.descriptors[i];
        }
    }

    return NULL;
}

static const GraphicObjectDescriptor* object_registry_find_by_type(GraphicObjectType type)
{
    int i = 0;

    for (i = 0; i < g_registry.count; ++i) {
        if (g_registry.descriptors[i].type == type) {
            return &g_registry.descriptors[i];
        }
    }

    return NULL;
}

void object_registry_init(void)
{
    if (g_registry.initialized) {
        return;
    }

    memset(&g_registry, 0, sizeof(g_registry));
    g_registry.initialized = 1;
    g_registry.next_dynamic_type = 100u;
}

int register_object_type(const GraphicObjectDescriptor* descriptor)
{
    GraphicObjectDescriptor* slot = NULL;
    GraphicObjectDescriptor copy;

    if (!descriptor || !descriptor->type_id || descriptor->type_id[0] == '\0' ||
        !descriptor->name || descriptor->name[0] == '\0' ||
        !descriptor->create || !descriptor->clone || !descriptor->destroy ||
        !descriptor->get_bounds || !descriptor->hit_test ||
        !descriptor->get_path_point_count || !descriptor->write_path_points ||
        !descriptor->get_scalar || !descriptor->set_scalar ||
        !descriptor->serialize || !descriptor->deserialize) {
        return 0;
    }

    object_registry_init();
    if (object_registry_find_by_type_id(descriptor->type_id) ||
        object_registry_find_by_type(descriptor->type)) {
        return 0;
    }

    copy = *descriptor;
    if (copy.type == GRAPHIC_OBJECT_INVALID) {
        copy.type = g_registry.next_dynamic_type++;
    } else if (copy.type >= g_registry.next_dynamic_type) {
        g_registry.next_dynamic_type = copy.type + 1u;
    }

    slot = object_registry_alloc_slot();
    if (!slot) {
        return 0;
    }

    *slot = copy;
    return 1;
}

const GraphicObjectDescriptor* object_registry_lookup(const char* type_id)
{
    return object_registry_find_by_type_id(type_id);
}

const GraphicObjectDescriptor* object_registry_lookup_by_type(GraphicObjectType type)
{
    return object_registry_find_by_type(type);
}

int object_registry_count(void)
{
    return g_registry.count;
}

const GraphicObjectDescriptor* object_registry_at(int index)
{
    if (index < 0 || index >= g_registry.count) {
        return NULL;
    }

    return &g_registry.descriptors[index];
}

const char* object_type_name(GraphicObjectType type)
{
    const GraphicObjectDescriptor* descriptor = object_registry_lookup_by_type(type);
    return descriptor ? descriptor->name : "Unknown";
}

GraphicObject* object_create(const char* type_id, const void* init_data, GraphicStyle style)
{
    const GraphicObjectDescriptor* descriptor = object_registry_lookup(type_id);

    if (!descriptor || !descriptor->create) {
        return NULL;
    }

    return descriptor->create(init_data, style);
}
