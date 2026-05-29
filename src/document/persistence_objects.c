#include "persistence_objects.h"

#include <base/log.h>

#include "document_internal.h"

#include <stdio.h>
#include <string.h>

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

int parse_objects_array(JsonParser* parser, Document* document)
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
