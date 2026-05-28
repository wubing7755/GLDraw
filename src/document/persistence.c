/**
 * @file persistence.c
 * @brief JSON serialization/deserialization for document files.
 *
 * Role in project:
 * - Writes stable document JSON format (`version=1`) for save.
 * - Parses JSON into validated runtime objects on load.
 *
 * Module relationships:
 * - Uses document/object APIs for model construction and ID handling.
 * - Called by application save/load commands.
 */
#include <document/persistence.h>

#include <base/file_utils.h>
#include <base/log.h>

#include "persistence_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Intermediate staging struct for JSON deserialization.
 *
 * Holds all parsed fields for one object with companion `has_*` flags
 * indicating which fields were actually present in the JSON.
 */
typedef struct {
    ObjectId id;
    int has_id;
    LayerId layer_id;
    int has_layer_id;
    const GraphicObjectDescriptor* descriptor;
    int has_type;
    GraphicStyle style;
    int has_stroke_r;
    int has_stroke_g;
    int has_stroke_b;
    int has_stroke_a;
    int has_stroke_width;
    float x1;
    float y1;
    float x2;
    float y2;
    float x;
    float y;
    float width;
    float height;
    int has_x1;
    int has_y1;
    int has_x2;
    int has_y2;
    int has_x;
    int has_y;
    int has_width;
    int has_height;
    GraphicPropertyBag properties;
} LoadedObjectData;

static const char* layer_blend_mode_name(DocumentLayerBlendMode blend_mode)
{
    switch (blend_mode) {
    case DOCUMENT_LAYER_BLEND_MULTIPLY:
        return "multiply";
    case DOCUMENT_LAYER_BLEND_SCREEN:
        return "screen";
    case DOCUMENT_LAYER_BLEND_NORMAL:
    default:
        return "normal";
    }
}

static DocumentLayerBlendMode layer_blend_mode_from_name(const char* name)
{
    if (name && strcmp(name, "multiply") == 0) {
        return DOCUMENT_LAYER_BLEND_MULTIPLY;
    }
    if (name && strcmp(name, "screen") == 0) {
        return DOCUMENT_LAYER_BLEND_SCREEN;
    }
    return DOCUMENT_LAYER_BLEND_NORMAL;
}

/**
 * @brief Serializes document to JSON and writes to open file.
 * @param file [in,out] Target file handle.
 * @param document [in] Document to serialize.
 * @return 1 on success, 0 on failure.
 *
 * @note Uses ferror(file) for overall write failure detection.
 */
static int write_document_json(FILE* file, const Document* document)
{
    int i = 0;

    if (!file || !document) {
        return 0;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"format\": \"gldraw-document\",\n");
    fprintf(file, "  \"version\": 2,\n");
    fprintf(file, "  \"next_id\": %u,\n", document->next_id);
    fprintf(file, "  \"active_layer_id\": %u,\n", document->active_layer_id);
    fprintf(file, "  \"layers\": [\n");

    for (i = 0; i < document->layer_count; ++i) {
        const DocumentLayer* layer = &document->layers[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"id\": %u,\n", layer->id);
        fprintf(file, "      \"name\": \"%s\",\n", layer->name);
        fprintf(file, "      \"visible\": %s,\n", layer->visible ? "true" : "false");
        fprintf(file, "      \"locked\": %s,\n", layer->locked ? "true" : "false");
        fprintf(file, "      \"blend_mode\": \"%s\"\n",
                layer_blend_mode_name(layer->blend_mode));
        fprintf(file, "    }%s\n", (i + 1 < document->layer_count) ? "," : "");
    }

    fprintf(file, "  ],\n");
    fprintf(file, "  \"objects\": [\n");

    for (i = 0; i < document->count; ++i) {
        const GraphicObject* object = document->objects[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"id\": %u,\n", object->id);
        fprintf(file, "      \"layer_id\": %u,\n", object->layer_id);
        fprintf(file, "      \"type\": \"%s\",\n", object_type_id(object));
        fprintf(file,
                "      \"stroke\": { \"r\": %.9g, \"g\": %.9g, \"b\": %.9g, \"a\": %.9g, \"width\": %.9g },\n",
                object->style.stroke_color.r,
                object->style.stroke_color.g,
                object->style.stroke_color.b,
                object->style.stroke_color.a,
                object->style.stroke_width);
        {
            GraphicPropertyBag properties;
            int property_index = 0;

            graphic_property_bag_init(&properties);
            if (!object->descriptor || !object->descriptor->serialize ||
                !object->descriptor->serialize(object, &properties)) {
                return 0;
            }

            fprintf(file, "      \"properties\": {");
            for (property_index = 0; property_index < properties.count; ++property_index) {
                fprintf(file,
                        "%s \"%s\": %.9g",
                        property_index == 0 ? "" : ",",
                        properties.values[property_index].name,
                        properties.values[property_index].value);
            }
            fprintf(file, " }\n");
        }

        fprintf(file, "    }%s\n", (i + 1 < document->count) ? "," : "");
    }

    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    return ferror(file) == 0;
}

/**
 * @brief Parses a stroke object.
 * @param parser JSON parser.
 * @param data Loaded object data.
 * @return 1 on success, 0 on failure.
 */
static int parse_stroke_object(JsonParser* parser, LoadedObjectData* data)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        LOG_ERROR("%s", "[persistence][parse][stroke] expected object");
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            LOG_ERROR("%s", "[persistence][parse][stroke] expected key");
            return 0;
        }

        if (json_token_is_string(parser, "r")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] expected ':' after r");
                return 0;
            }
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.r)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] invalid r value");
                return 0;
            }
            data->has_stroke_r = 1;
        } else if (json_token_is_string(parser, "g")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] expected ':' after g");
                return 0;
            }
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.g)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] invalid g value");
                return 0;
            }
            data->has_stroke_g = 1;
        } else if (json_token_is_string(parser, "b")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] expected ':' after b");
                return 0;
            }
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.b)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] invalid b value");
                return 0;
            }
            data->has_stroke_b = 1;
        } else if (json_token_is_string(parser, "a")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] expected ':' after a");
                return 0;
            }
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.a)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] invalid a value");
                return 0;
            }
            data->has_stroke_a = 1;
        } else if (json_token_is_string(parser, "width")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] expected ':' after width");
                return 0;
            }
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_width)) {
                LOG_ERROR("%s", "[persistence][parse][stroke] invalid width value");
                return 0;
            }
            data->has_stroke_width = 1;
        } else {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_skip_value(parser)) return 0;
        }

        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACE) {
            json_parser_next(parser);
            return 1;
        }
        LOG_ERROR("%s", "[persistence][parse][stroke] malformed object");
        return 0;
    }
}

/**
 * @brief Parses a geometry object.
 * @param parser JSON parser.
 * @param data Loaded object data.
 * @return 1 on success, 0 on failure.
 */
static int parse_properties_object(JsonParser* parser, LoadedObjectData* data)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        LOG_ERROR("%s", "[persistence][parse][properties] expected object");
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        char property_name[32];
        size_t token_length = 0u;
        float value = 0.0f;

        if (parser->type != JSON_TOKEN_STRING) {
            LOG_ERROR("%s", "[persistence][parse][properties] expected key");
            return 0;
        }

        token_length = parser->token_length;
        if (token_length >= sizeof(property_name)) {
            return 0;
        }
        memcpy(property_name, parser->token_start, token_length);
        property_name[token_length] = '\0';

        json_parser_next(parser);
        if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
        json_parser_next(parser);
        if (!json_parse_float(parser, &value)) return 0;
        if (!graphic_property_bag_set(&data->properties, property_name, value)) {
            return 0;
        }

        if (strcmp(property_name, "x1") == 0) { data->x1 = value; data->has_x1 = 1; }
        if (strcmp(property_name, "y1") == 0) { data->y1 = value; data->has_y1 = 1; }
        if (strcmp(property_name, "x2") == 0) { data->x2 = value; data->has_x2 = 1; }
        if (strcmp(property_name, "y2") == 0) { data->y2 = value; data->has_y2 = 1; }
        if (strcmp(property_name, "x") == 0) { data->x = value; data->has_x = 1; }
        if (strcmp(property_name, "y") == 0) { data->y = value; data->has_y = 1; }
        if (strcmp(property_name, "width") == 0) { data->width = value; data->has_width = 1; }
        if (strcmp(property_name, "height") == 0) { data->height = value; data->has_height = 1; }

        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACE) {
            json_parser_next(parser);
            return 1;
        }
        LOG_ERROR("%s", "[persistence][parse][properties] malformed object");
        return 0;
    }
}

/**
 * @brief Parses an object type.
 * @param parser JSON parser.
 * @param data Loaded object data.
 * @return 1 on success, 0 on failure.
 */
static int parse_object_type(JsonParser* parser, LoadedObjectData* data)
{
    char type_name[32];
    size_t token_length = 0u;

    if (!json_expect(parser, JSON_TOKEN_STRING)) {
        return 0;
    }

    token_length = parser->token_length;
    if (token_length >= sizeof(type_name)) {
        return 0;
    }
    memcpy(type_name, parser->token_start, token_length);
    type_name[token_length] = '\0';

    data->descriptor = object_registry_lookup(type_name);
    if (!data->descriptor) {
        return 0;
    }
    data->has_type = 1;

    json_parser_next(parser);
    return 1;
}

static int parse_layer_entry(JsonParser* parser, Document* document)
{
    unsigned int id = 0u;
    int visible = 1;
    int locked = 0;
    DocumentLayer layer = {0};
    char name[32] = {0};
    char blend_mode[16] = {0};
    int has_id = 0;
    int has_name = 0;

    if (!parser || !document || !json_expect(parser, JSON_TOKEN_LBRACE)) {
        return 0;
    }

    json_parser_next(parser);
    while (parser->type != JSON_TOKEN_RBRACE) {
        if (parser->type != JSON_TOKEN_STRING) {
            return 0;
        }

        if (json_token_is_string(parser, "id")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &id)) return 0;
            has_id = 1;
        } else if (json_token_is_string(parser, "name")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_string_value(parser, name, sizeof(name))) return 0;
            has_name = 1;
        } else if (json_token_is_string(parser, "visible")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_bool(parser, &visible)) return 0;
        } else if (json_token_is_string(parser, "locked")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_bool(parser, &locked)) return 0;
        } else if (json_token_is_string(parser, "blend_mode")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_string_value(parser, blend_mode, sizeof(blend_mode))) return 0;
        } else {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_skip_value(parser)) return 0;
        }

        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
        } else if (parser->type != JSON_TOKEN_RBRACE) {
            return 0;
        }
    }

    json_parser_next(parser);
    if (!has_id || !has_name || id == 0u) {
        return 0;
    }

    layer.id = id;
    snprintf(layer.name, sizeof(layer.name), "%s", name);
    layer.visible = visible ? 1 : 0;
    layer.locked = locked ? 1 : 0;
    layer.blend_mode = layer_blend_mode_from_name(blend_mode);
    return document_insert_layer_at(document, &layer, document->layer_count);
}

static int parse_layers_array(JsonParser* parser, Document* document)
{
    if (!parser || !document || !json_expect(parser, JSON_TOKEN_LBRACKET)) {
        return 0;
    }

    document->layer_count = 0;
    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACKET) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        if (!parse_layer_entry(parser, document)) {
            return 0;
        }
        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACKET) {
            json_parser_next(parser);
            return 1;
        }
        return 0;
    }
}

/**
 * @brief Builds a graphic object from loaded data.
 * @param data Loaded object data.
 * @return New object or NULL.
 */
static GraphicObject* build_loaded_object(const LoadedObjectData* data)
{
    if (!data || !data->has_id || !data->has_type ||
        !data->has_stroke_r || !data->has_stroke_g || !data->has_stroke_b ||
        !data->has_stroke_a || !data->has_stroke_width) {
        LOG_ERROR("%s", "[persistence][parse][object entry] missing required fields");
        return NULL;
    }

    if (data->id == 0u) {
        LOG_ERROR("%s", "[persistence][parse][object entry] invalid object id=0");
        return NULL;
    }
    if (data->style.stroke_width <= 0.0f) {
        LOG_ERROR("[persistence][parse][stroke] invalid stroke width=%.3f",
                  data->style.stroke_width);
        return NULL;
    }

    if (!data->descriptor || !data->descriptor->deserialize) {
        return NULL;
    }

    return data->descriptor->deserialize(&data->properties, data->style);
}

/**
 * @brief Parse one object entry and append it to document.
 * @param parser [in,out] JSON parser.
 * @param document [in,out] Target document.
 * @return 1 on success, 0 on failure.
 *
 * Algorithm steps:
 * 1. Reads object fields (id/type/stroke/geometry).
 * 2. Skips unrecognized fields.
 * 3. Validates and constructs runtime object.
 * 4. Appends to document with specified ID.
 *
 * Risk note:
 * - On append failure, allocated object is destroyed immediately to prevent leaks.
 */
static int parse_object_entry(JsonParser* parser, Document* document)
{
    LoadedObjectData data;
    GraphicObject* object = NULL;
    unsigned int id_value = 0;

    memset(&data, 0, sizeof(data));
    data.style = object_default_style();
    graphic_property_bag_init(&data.properties);

    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        LOG_ERROR("%s", "[persistence][parse][object entry] expected object");
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        LOG_ERROR("%s", "[persistence][parse][object entry] empty object entry");
        return 0;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            LOG_ERROR("%s", "[persistence][parse][object entry] expected field name");
            return 0;
        }

        if (json_token_is_string(parser, "id")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &id_value)) return 0;
            data.id = id_value;
            data.has_id = 1;
        } else if (json_token_is_string(parser, "layer_id")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &id_value)) return 0;
            data.layer_id = id_value;
            data.has_layer_id = 1;
        } else if (json_token_is_string(parser, "type")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_object_type(parser, &data)) return 0;
        } else if (json_token_is_string(parser, "stroke")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_stroke_object(parser, &data)) {
                LOG_ERROR("%s", "[persistence][parse][object entry] failed parsing stroke");
                return 0;
            }
        } else if (json_token_is_string(parser, "geometry") ||
                   json_token_is_string(parser, "properties")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_properties_object(parser, &data)) {
                LOG_ERROR("%s", "[persistence][parse][object entry] failed parsing properties");
                return 0;
            }
        } else {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_skip_value(parser)) return 0;
        }

        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACE) {
            json_parser_next(parser);
            break;
        }
        LOG_ERROR("%s", "[persistence][parse][object entry] malformed fields");
        return 0;
    }

    object = build_loaded_object(&data);
    if (!object) {
        LOG_ERROR("%s", "[persistence][parse][object entry] failed object build");
        return 0;
    }

    object->layer_id = data.has_layer_id ? data.layer_id : document_active_layer_id(document);
    if (!document_append_object_with_id_to_layer(document,
                                                 object,
                                                 data.id,
                                                 object->layer_id)) {
        object_destroy(object);
        return 0;
    }

    return 1;
}

/**
 * @brief Parses an objects array.
 * @param parser JSON parser.
 * @param document Target document.
 * @return 1 on success, 0 on failure.
 */
static int parse_objects_array(JsonParser* parser, Document* document)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACKET)) {
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACKET) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        if (!parse_object_entry(parser, document)) {
            LOG_ERROR("[persistence][parse][object entry] parse failed at index=%d", document->count);
            return 0;
        }
        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACKET) {
            json_parser_next(parser);
            return 1;
        }
        return 0;
    }
}

/**
 * @brief Parse top-level document JSON object.
 * @param parser [in,out] JSON parser.
 * @param document [in,out] Target document.
 * @return 1 on valid schema/content, 0 on parse/schema mismatch.
 *
 * @note This function only accepts supported schema and strictly validates format/version.
 */
static int parse_document_root(JsonParser* parser, Document* document)
{
    int got_format = 0;
    int got_version = 0;
    int got_objects = 0;
    int got_next_id = 0;
    int got_layers = 0;
    int got_active_layer_id = 0;
    unsigned int version = 0;
    unsigned int next_id = 0;
    unsigned int active_layer_id = 0;

    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        LOG_ERROR("%s", "[persistence][parse][top-level] expected root object");
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        LOG_ERROR("%s", "[persistence][parse][top-level] empty root object");
        return 0;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            LOG_ERROR("%s", "[persistence][parse][top-level] expected field name");
            return 0;
        }

        if (json_token_is_string(parser, "format")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_STRING) ||
                !json_token_is_string(parser, "gldraw-document")) {
                return 0;
            }
            got_format = 1;
            json_parser_next(parser);
        } else if (json_token_is_string(parser, "version")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &version)) return 0;
            got_version = 1;
        } else if (json_token_is_string(parser, "next_id")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &next_id)) return 0;
            got_next_id = 1;
        } else if (json_token_is_string(parser, "active_layer_id")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &active_layer_id)) return 0;
            got_active_layer_id = 1;
        } else if (json_token_is_string(parser, "layers")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_layers_array(parser, document)) return 0;
            got_layers = 1;
        } else if (json_token_is_string(parser, "objects")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_objects_array(parser, document)) return 0;
            got_objects = 1;
        } else {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_skip_value(parser)) return 0;
        }

        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACE) {
            json_parser_next(parser);
            break;
        }
        return 0;
    }

    if (!got_format || !got_version || (version != 1 && version != 2) || !got_objects) {
        LOG_ERROR("%s", "[persistence][parse][top-level] missing or invalid required keys (format/version/objects)");
        return 0;
    }

    if (version == 1 || !got_layers || document->layer_count <= 0) {
        document->layer_count = 1;
        document->layers[0].id = 1u;
        snprintf(document->layers[0].name, sizeof(document->layers[0].name), "%s", "Layer 1");
        document->layers[0].visible = 1;
        document->layers[0].locked = 0;
        document->layers[0].blend_mode = DOCUMENT_LAYER_BLEND_NORMAL;
        document->next_layer_id = 2u;
    }
    if (got_active_layer_id && document_layer_find_const(document, active_layer_id)) {
        document->active_layer_id = active_layer_id;
    } else if (document->layer_count > 0) {
        document->active_layer_id = document->layers[0].id;
    }

    /* Selection is editor-session state and intentionally not serialized. */
    if (got_next_id && next_id > document_max_id(document)) {
        document->next_id = next_id;
    } else {
        document->next_id = document_max_id(document) + 1;
    }
    document->revision = 1;
    return 1;
}

/**
 * @brief Save document to JSON path using temp-file replacement.
 * @return `1` on success, `0` on allocation/I/O/serialization failure.
 */

/**
 * @brief Saves document to JSON file.
 * @param document Document to save.
 * @param path Output file path.
 * @return 1 on success, 0 on failure.
 */
int document_save_json(const Document* document, const char* path)
{
    FILE* file = NULL;
    int close_result = 0;
    int target_exists = 0;
    char* temp_path = NULL;

    if (!document || !path) {
        return 0;
    }

    temp_path = file_utils_duplicate_path_with_suffix(path, ".tmp");
    if (!temp_path) {
        LOG_ERROR("Failed to allocate temp path for document: %s", path);
        return 0;
    }

    target_exists = file_utils_path_exists(path);
    file = fopen(temp_path, "wb");
    if (!file) {
        LOG_ERROR("Failed to open temp document for writing: %s", temp_path);
        free(temp_path);
        return 0;
    }

    if (!write_document_json(file, document)) {
        fclose(file);
        remove(temp_path);
        LOG_ERROR("Failed to write document JSON temp file: %s", temp_path);
        free(temp_path);
        return 0;
    }

    close_result = fclose(file);
    if (close_result != 0) {
        remove(temp_path);
        LOG_ERROR("Failed to finalize document JSON temp file: %s", temp_path);
        free(temp_path);
        return 0;
    }

    if (!file_utils_replace_file_with_temp(temp_path, path)) {
        LOG_ERROR("Failed to replace document file: %s", path);
        if (target_exists) {
            LOG_ERROR("Preserved existing document after failed replace: %s", path);
        }
        free(temp_path);
        return 0;
    }

    free(temp_path);
    return 1;
}

/**
 * @brief Loads document from JSON file into runtime model.
 * @param document [in,out] Target document.
 * @param path [in] JSON file path.
 * @return 1 on success, 0 on I/O/parse/validation failure.
 *
 * Why staged load:
 * - Parses into a temporary `loaded_document` first; current document is only
 *   replaced after full success, preserving previous state on failure.
 */
int document_load_json(Document* document, const char* path)
{
    char* text = NULL;
    JsonParser parser;
    Document loaded_document;

    if (!document || !path) {
        return 0;
    }

    text = file_utils_read_text_file(path);
    if (!text) {
        LOG_ERROR("Failed to open document for reading: %s", path);
        return 0;
    }

    document_init(&loaded_document);
    parser.text = text;
    parser.pos = 0;
    parser.length = strlen(text);
    parser.type = JSON_TOKEN_INVALID;
    parser.token_start = NULL;
    parser.token_length = 0;
    parser.number_value = 0.0;

    json_parser_next(&parser);
    if (!parse_document_root(&parser, &loaded_document) || parser.type != JSON_TOKEN_EOF) {
        document_shutdown(&loaded_document);
        free(text);
        LOG_ERROR("[persistence][parse][top-level] Failed to parse document JSON: %s", path);
        return 0;
    }

    document_shutdown(document);
    *document = loaded_document;
    free(text);
    return 1;
}
