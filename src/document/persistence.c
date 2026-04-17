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

#include <base/log.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef enum {
    JSON_TOKEN_EOF = 0,
    JSON_TOKEN_LBRACE,
    JSON_TOKEN_RBRACE,
    JSON_TOKEN_LBRACKET,
    JSON_TOKEN_RBRACKET,
    JSON_TOKEN_COLON,
    JSON_TOKEN_COMMA,
    JSON_TOKEN_STRING,
    JSON_TOKEN_NUMBER,
    JSON_TOKEN_TRUE,
    JSON_TOKEN_FALSE,
    JSON_TOKEN_NULL,
    JSON_TOKEN_INVALID
} JsonTokenType;

typedef struct {
    const char* text;
    size_t pos;
    size_t length;
    JsonTokenType type;
    const char* token_start;
    size_t token_length;
    double number_value;
} JsonParser;

/**
 * @brief Intermediate staging struct for JSON deserialization.
 *
 * Holds all parsed fields for one object with companion `has_*` flags
 * indicating which fields were actually present in the JSON.
 */
typedef struct {
    ObjectId id;
    int has_id;
    GraphicObjectType type;
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
} LoadedObjectData;

/** Read entire file into heap buffer (NUL-terminated). Caller owns returned memory. */
static char* read_text_file(const char* path)
{
    FILE* file = NULL;
    char* buffer = NULL;
    long size = 0;
    size_t read_size = 0;

    file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    buffer = (char*)malloc((size_t)size + 1u);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    read_size = fread(buffer, 1u, (size_t)size, file);
    if (read_size != (size_t)size && ferror(file)) {
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[read_size] = '\0';
    fclose(file);
    return buffer;
}

/** Allocate `path + suffix` string. Caller owns returned memory. */
static char* duplicate_path_with_suffix(const char* path, const char* suffix)
{
    size_t path_length = 0;
    size_t suffix_length = 0;
    char* result = NULL;

    if (!path || !suffix) {
        return NULL;
    }

    path_length = strlen(path);
    suffix_length = strlen(suffix);
    result = (char*)malloc(path_length + suffix_length + 1u);
    if (!result) {
        return NULL;
    }

    memcpy(result, path, path_length);
    memcpy(result + path_length, suffix, suffix_length);
    result[path_length + suffix_length] = '\0';
    return result;
}

/** Cheap file-exists check via open attempt. */
static int file_exists_at_path(const char* path)
{
    FILE* file = NULL;

    if (!path) {
        return 0;
    }

    file = fopen(path, "rb");
    if (!file) {
        return 0;
    }

    fclose(file);
    return 1;
}

/** Map object type enum to persisted JSON tag. */
static const char* object_type_tag(GraphicObjectType type)
{
    switch (type) {
    case GRAPHIC_OBJECT_LINE:
        return "line";
    case GRAPHIC_OBJECT_RECT:
        return "rect";
    case GRAPHIC_OBJECT_ELLIPSE:
        return "ellipse";
    default:
        return "unknown";
    }
}

/**
 * @brief Atomically replace target file with temp file.
 * Why:
 * - Write-then-replace avoids corrupting existing file on partial write failure.
 */
static int replace_file_with_temp(const char* temp_path, const char* target_path)
{
    if (!temp_path || !target_path) {
        return 0;
    }

#ifdef _WIN32
    if (MoveFileExA(temp_path,
                    target_path,
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return 1;
    }

    LOG_ERROR("Failed to replace document file: %s", target_path);
    remove(temp_path);
    return 0;
#else
    if (rename(temp_path, target_path) == 0) {
        return 1;
    }

    LOG_ERROR("Failed to replace document file: %s", target_path);
    remove(temp_path);
    return 0;
#endif
}

/**
 * Serialize document as JSON into already-open file handle.
 * Risk note: individual `fprintf` returns are not checked line-by-line; final
 * `ferror(file)` is used as aggregate write-failure detection.
 */
static int write_document_json(FILE* file, const Document* document)
{
    int i = 0;

    if (!file || !document) {
        return 0;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"format\": \"gldraw-document\",\n");
    fprintf(file, "  \"version\": 1,\n");
    fprintf(file, "  \"next_id\": %u,\n", document->next_id);
    fprintf(file, "  \"selection\": [");
    for (i = 0; i < document->selection.count; ++i) {
        fprintf(file, "%s%u", (i == 0) ? "" : ", ", document->selection.ids[i]);
    }
    fprintf(file, "],\n");
    fprintf(file, "  \"objects\": [\n");

    for (i = 0; i < document->count; ++i) {
        const GraphicObject* object = document->objects[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"id\": %u,\n", object->id);
        fprintf(file, "      \"type\": \"%s\",\n", object_type_tag(object->type));
        fprintf(file,
                "      \"stroke\": { \"r\": %.9g, \"g\": %.9g, \"b\": %.9g, \"a\": %.9g, \"width\": %.9g },\n",
                object->style.stroke_color.r,
                object->style.stroke_color.g,
                object->style.stroke_color.b,
                object->style.stroke_color.a,
                object->style.stroke_width);

        if (object->type == GRAPHIC_OBJECT_LINE) {
            float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
            object_get_scalar(object, "x1", &x1);
            object_get_scalar(object, "y1", &y1);
            object_get_scalar(object, "x2", &x2);
            object_get_scalar(object, "y2", &y2);
            fprintf(file,
                    "      \"geometry\": { \"x1\": %.9g, \"y1\": %.9g, \"x2\": %.9g, \"y2\": %.9g }\n",
                    x1, y1, x2, y2);
        } else {
            float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
            object_get_scalar(object, "x", &x);
            object_get_scalar(object, "y", &y);
            object_get_scalar(object, "width", &width);
            object_get_scalar(object, "height", &height);
            fprintf(file,
                    "      \"geometry\": { \"x\": %.9g, \"y\": %.9g, \"width\": %.9g, \"height\": %.9g }\n",
                    x, y, width, height);
        }

        fprintf(file, "    }%s\n", (i + 1 < document->count) ? "," : "");
    }

    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    return ferror(file) == 0;
}

/** Check whether parser position matches keyword and token boundary. */
static int json_match_keyword(const JsonParser* parser, const char* keyword)
{
    size_t length = 0;
    char next = '\0';

    if (!parser || !keyword) {
        return 0;
    }

    length = strlen(keyword);
    if (parser->pos + length > parser->length) {
        return 0;
    }

    if (strncmp(parser->text + parser->pos, keyword, length) != 0) {
        return 0;
    }

    if (parser->pos + length >= parser->length) {
        return 1;
    }

    next = parser->text[parser->pos + length];
    return !(isalnum((unsigned char)next) || next == '_');
}

/** Skip JSON whitespace from current parser position. */
static void json_parser_skip_ws(JsonParser* parser)
{
    while (parser->pos < parser->length &&
           isspace((unsigned char)parser->text[parser->pos])) {
        parser->pos++;
    }
}

/**
 * @brief Advance parser to next token.
 * Risk note:
 * - Keeps implementation intentionally strict; malformed escape/number forms
 *   become `JSON_TOKEN_INVALID` to fail-fast during load.
 */
static void json_parser_next(JsonParser* parser)
{
    const char* start = NULL;
    const char* number_end = NULL;
    size_t i = 0;

    json_parser_skip_ws(parser);
    parser->token_start = NULL;
    parser->token_length = 0;
    parser->number_value = 0.0;

    if (parser->pos >= parser->length) {
        parser->type = JSON_TOKEN_EOF;
        return;
    }

    switch (parser->text[parser->pos]) {
    case '{':
        parser->type = JSON_TOKEN_LBRACE;
        parser->pos++;
        return;
    case '}':
        parser->type = JSON_TOKEN_RBRACE;
        parser->pos++;
        return;
    case '[':
        parser->type = JSON_TOKEN_LBRACKET;
        parser->pos++;
        return;
    case ']':
        parser->type = JSON_TOKEN_RBRACKET;
        parser->pos++;
        return;
    case ':':
        parser->type = JSON_TOKEN_COLON;
        parser->pos++;
        return;
    case ',':
        parser->type = JSON_TOKEN_COMMA;
        parser->pos++;
        return;
    case '"':
        parser->pos++;
        start = parser->text + parser->pos;
        while (parser->pos < parser->length) {
            if (parser->text[parser->pos] == '\\') {
                if (parser->pos + 1 >= parser->length) {
                    parser->type = JSON_TOKEN_INVALID;
                    return;
                }
                parser->pos += 2;
                continue;
            }
            if (parser->text[parser->pos] == '"') {
                break;
            }
            parser->pos++;
        }
        if (parser->pos >= parser->length) {
            parser->type = JSON_TOKEN_INVALID;
            return;
        }
        parser->type = JSON_TOKEN_STRING;
        parser->token_start = start;
        parser->token_length = (size_t)((parser->text + parser->pos) - start);
        parser->pos++;
        return;
    default:
        break;
    }

    if (json_match_keyword(parser, "true")) {
        parser->type = JSON_TOKEN_TRUE;
        parser->pos += 4;
        return;
    }
    if (json_match_keyword(parser, "false")) {
        parser->type = JSON_TOKEN_FALSE;
        parser->pos += 5;
        return;
    }
    if (json_match_keyword(parser, "null")) {
        parser->type = JSON_TOKEN_NULL;
        parser->pos += 4;
        return;
    }

    if (parser->text[parser->pos] == '-' || isdigit((unsigned char)parser->text[parser->pos])) {
        start = parser->text + parser->pos;
        parser->number_value = strtod(start, (char**)&number_end);
        if (number_end == start) {
            parser->type = JSON_TOKEN_INVALID;
            return;
        }
        i = (size_t)(number_end - parser->text);
        parser->type = JSON_TOKEN_NUMBER;
        parser->token_start = start;
        parser->token_length = (size_t)(number_end - start);
        parser->pos = i;
        return;
    }

    parser->type = JSON_TOKEN_INVALID;
}

/** Compare current string token with literal. */
static int json_token_is_string(const JsonParser* parser, const char* literal)
{
    size_t literal_length = 0;

    if (!parser || parser->type != JSON_TOKEN_STRING || !literal) {
        return 0;
    }

    literal_length = strlen(literal);
    return parser->token_length == literal_length &&
           strncmp(parser->token_start, literal, literal_length) == 0;
}

/** Check expected token type. */
static int json_expect(JsonParser* parser, JsonTokenType type)
{
    return parser && parser->type == type;
}

/** Parse unsigned integer token with integer range validation. */
static int json_parse_u32(JsonParser* parser, unsigned int* out_value)
{
    double value = 0.0;

    if (!parser || !out_value || parser->type != JSON_TOKEN_NUMBER) {
        return 0;
    }

    value = parser->number_value;
    if (value < 0.0 || value > (double)UINT_MAX ||
        (double)((unsigned int)value) != value) {
        return 0;
    }

    *out_value = (unsigned int)value;
    json_parser_next(parser);
    return 1;
}

/** Parse float token and advance parser. */
static int json_parse_float(JsonParser* parser, float* out_value)
{
    if (!parser || !out_value || parser->type != JSON_TOKEN_NUMBER) {
        return 0;
    }

    *out_value = (float)parser->number_value;
    json_parser_next(parser);
    return 1;
}

static int json_skip_value(JsonParser* parser);

/** Skip unknown JSON object recursively. */
static int json_skip_object(JsonParser* parser)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            return 0;
        }
        json_parser_next(parser);
        if (!json_expect(parser, JSON_TOKEN_COLON)) {
            return 0;
        }
        json_parser_next(parser);
        if (!json_skip_value(parser)) {
            return 0;
        }
        if (parser->type == JSON_TOKEN_COMMA) {
            json_parser_next(parser);
            continue;
        }
        if (parser->type == JSON_TOKEN_RBRACE) {
            json_parser_next(parser);
            return 1;
        }
        return 0;
    }
}

/** Skip unknown JSON array recursively. */
static int json_skip_array(JsonParser* parser)
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
        if (!json_skip_value(parser)) {
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

/** Skip arbitrary JSON value recursively. */
static int json_skip_value(JsonParser* parser)
{
    switch (parser->type) {
    case JSON_TOKEN_STRING:
    case JSON_TOKEN_NUMBER:
    case JSON_TOKEN_TRUE:
    case JSON_TOKEN_FALSE:
    case JSON_TOKEN_NULL:
        json_parser_next(parser);
        return 1;
    case JSON_TOKEN_LBRACE:
        return json_skip_object(parser);
    case JSON_TOKEN_LBRACKET:
        return json_skip_array(parser);
    default:
        return 0;
    }
}

/** Parse `"stroke"` sub-object into loaded object staging struct. */
static int parse_stroke_object(JsonParser* parser, LoadedObjectData* data)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            return 0;
        }

        if (json_token_is_string(parser, "r")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.r)) return 0;
            data->has_stroke_r = 1;
        } else if (json_token_is_string(parser, "g")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.g)) return 0;
            data->has_stroke_g = 1;
        } else if (json_token_is_string(parser, "b")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.b)) return 0;
            data->has_stroke_b = 1;
        } else if (json_token_is_string(parser, "a")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_color.a)) return 0;
            data->has_stroke_a = 1;
        } else if (json_token_is_string(parser, "width")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->style.stroke_width)) return 0;
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
        return 0;
    }
}

/** Parse `"geometry"` sub-object into loaded object staging struct. */
static int parse_geometry_object(JsonParser* parser, LoadedObjectData* data)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            return 0;
        }

        if (json_token_is_string(parser, "x1")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->x1)) return 0;
            data->has_x1 = 1;
        } else if (json_token_is_string(parser, "y1")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->y1)) return 0;
            data->has_y1 = 1;
        } else if (json_token_is_string(parser, "x2")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->x2)) return 0;
            data->has_x2 = 1;
        } else if (json_token_is_string(parser, "y2")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->y2)) return 0;
            data->has_y2 = 1;
        } else if (json_token_is_string(parser, "x")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->x)) return 0;
            data->has_x = 1;
        } else if (json_token_is_string(parser, "y")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->y)) return 0;
            data->has_y = 1;
        } else if (json_token_is_string(parser, "width")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->width)) return 0;
            data->has_width = 1;
        } else if (json_token_is_string(parser, "height")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_float(parser, &data->height)) return 0;
            data->has_height = 1;
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
        return 0;
    }
}

/** Parse and validate object `"type"` token. */
static int parse_object_type(JsonParser* parser, LoadedObjectData* data)
{
    if (!json_expect(parser, JSON_TOKEN_STRING)) {
        return 0;
    }

    if (json_token_is_string(parser, "line")) {
        data->type = GRAPHIC_OBJECT_LINE;
        data->has_type = 1;
    } else if (json_token_is_string(parser, "rect")) {
        data->type = GRAPHIC_OBJECT_RECT;
        data->has_type = 1;
    } else if (json_token_is_string(parser, "ellipse")) {
        data->type = GRAPHIC_OBJECT_ELLIPSE;
        data->has_type = 1;
    } else {
        return 0;
    }

    json_parser_next(parser);
    return 1;
}

/** Build runtime object from parsed staged fields; returns `NULL` on missing required data. */
static GraphicObject* build_loaded_object(const LoadedObjectData* data)
{
    if (!data || !data->has_id || !data->has_type ||
        !data->has_stroke_r || !data->has_stroke_g || !data->has_stroke_b ||
        !data->has_stroke_a || !data->has_stroke_width) {
        return NULL;
    }

    switch (data->type) {
    case GRAPHIC_OBJECT_LINE:
        if (!data->has_x1 || !data->has_y1 || !data->has_x2 || !data->has_y2) {
            return NULL;
        }
        return object_create_line((Vec2){data->x1, data->y1},
                                  (Vec2){data->x2, data->y2},
                                  data->style);
    case GRAPHIC_OBJECT_RECT:
        if (!data->has_x || !data->has_y || !data->has_width || !data->has_height) {
            return NULL;
        }
        return object_create_rect((RectF){data->x, data->y, data->width, data->height},
                                  data->style);
    case GRAPHIC_OBJECT_ELLIPSE:
        if (!data->has_x || !data->has_y || !data->has_width || !data->has_height) {
            return NULL;
        }
        return object_create_ellipse((RectF){data->x, data->y, data->width, data->height},
                                     data->style);
    default:
        return NULL;
    }
}

/**
 * @brief Parse one object entry and append it to document.
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

    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        return 0;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
            return 0;
        }

        if (json_token_is_string(parser, "id")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!json_parse_u32(parser, &id_value)) return 0;
            data.id = id_value;
            data.has_id = 1;
        } else if (json_token_is_string(parser, "type")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_object_type(parser, &data)) return 0;
        } else if (json_token_is_string(parser, "stroke")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_stroke_object(parser, &data)) return 0;
        } else if (json_token_is_string(parser, "geometry")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_geometry_object(parser, &data)) return 0;
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

    object = build_loaded_object(&data);
    if (!object) {
        return 0;
    }

    if (!document_append_object_with_id(document, object, data.id)) {
        object_destroy(object);
        return 0;
    }

    return 1;
}

/** Parse selection ID array (extra IDs beyond max are safely ignored). */
static int parse_selection_array(JsonParser* parser, ObjectId* selection_ids, int* selection_count)
{
    if (!json_expect(parser, JSON_TOKEN_LBRACKET)) {
        return 0;
    }

    json_parser_next(parser);
    *selection_count = 0;

    if (parser->type == JSON_TOKEN_RBRACKET) {
        json_parser_next(parser);
        return 1;
    }

    while (1) {
        unsigned int value = 0;

        if (!json_parse_u32(parser, &value)) {
            return 0;
        }

        if (*selection_count < DOCUMENT_MAX_SELECTION) {
            selection_ids[(*selection_count)++] = value;
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

/** Parse objects array and append all parsed entries to document. */
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
 * @return `1` on valid schema/content, `0` on parse/schema mismatch.
 */
static int parse_document_root(JsonParser* parser, Document* document)
{
    int got_format = 0;
    int got_version = 0;
    int got_objects = 0;
    int got_next_id = 0;
    unsigned int version = 0;
    unsigned int next_id = 0;
    ObjectId selection_ids[DOCUMENT_MAX_SELECTION];
    int selection_count = 0;
    int i = 0;

    memset(selection_ids, 0, sizeof(selection_ids));

    if (!json_expect(parser, JSON_TOKEN_LBRACE)) {
        return 0;
    }

    json_parser_next(parser);
    if (parser->type == JSON_TOKEN_RBRACE) {
        return 0;
    }

    while (1) {
        if (parser->type != JSON_TOKEN_STRING) {
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
        } else if (json_token_is_string(parser, "selection")) {
            json_parser_next(parser);
            if (!json_expect(parser, JSON_TOKEN_COLON)) return 0;
            json_parser_next(parser);
            if (!parse_selection_array(parser, selection_ids, &selection_count)) return 0;
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

    if (!got_format || !got_version || version != 1 || !got_objects) {
        return 0;
    }

    document_clear_selection(document);
    for (i = 0; i < selection_count; ++i) {
        if (document_find_object(document, selection_ids[i])) {
            document_selection_add(document, selection_ids[i]);
        }
    }

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
int document_save_json(const Document* document, const char* path)
{
    FILE* file = NULL;
    int close_result = 0;
    int target_exists = 0;
    char* temp_path = NULL;

    if (!document || !path) {
        return 0;
    }

    temp_path = duplicate_path_with_suffix(path, ".tmp");
    if (!temp_path) {
        LOG_ERROR("Failed to allocate temp path for document: %s", path);
        return 0;
    }

    target_exists = file_exists_at_path(path);
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

    if (!replace_file_with_temp(temp_path, path)) {
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
 * @brief Load document from JSON file into runtime model.
 * @return `1` on success, `0` on I/O/parse/validation failure.
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

    text = read_text_file(path);
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
        LOG_ERROR("Failed to parse document JSON: %s", path);
        return 0;
    }

    document_shutdown(document);
    *document = loaded_document;
    free(text);
    return 1;
}
