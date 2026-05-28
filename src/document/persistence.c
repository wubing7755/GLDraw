/**
 * @file persistence.c
 * @brief Public document persistence orchestration.
 *
 * Role in project:
 * - Owns `document_save_json()` and `document_load_json()` public entry points.
 * - Coordinates staged load so current documents are only replaced after parse success.
 *
 * Module relationships:
 * - Delegates JSON tokenization, layer parsing, object parsing, and writing to focused modules.
 * - Called by application save/load commands.
 */
#include <document/persistence.h>

#include <base/file_utils.h>
#include <base/log.h>

#include "persistence_layers.h"
#include "persistence_json.h"
#include "persistence_objects.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int write_document_json(FILE* file, const Document* document);

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

    if (!document || !path || path[0] == '\0') {
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

    if (!document || !path || path[0] == '\0') {
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
