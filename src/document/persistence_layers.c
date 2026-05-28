#include "persistence_layers.h"

#include <stdio.h>
#include <string.h>

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

int parse_layers_array(JsonParser* parser, Document* document)
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
