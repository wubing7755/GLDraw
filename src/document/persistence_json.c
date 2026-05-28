#include "persistence_json.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

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

static void json_parser_skip_ws(JsonParser* parser)
{
    while (parser->pos < parser->length &&
           isspace((unsigned char)parser->text[parser->pos])) {
        parser->pos++;
    }
}

void json_parser_next(JsonParser* parser)
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

int json_token_is_string(const JsonParser* parser, const char* literal)
{
    size_t literal_length = 0;

    if (!parser || parser->type != JSON_TOKEN_STRING || !literal) {
        return 0;
    }

    literal_length = strlen(literal);
    return parser->token_length == literal_length &&
           strncmp(parser->token_start, literal, literal_length) == 0;
}

int json_expect(JsonParser* parser, JsonTokenType type)
{
    return parser && parser->type == type;
}

int json_parse_u32(JsonParser* parser, unsigned int* out_value)
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

int json_parse_float(JsonParser* parser, float* out_value)
{
    if (!parser || !out_value || parser->type != JSON_TOKEN_NUMBER) {
        return 0;
    }

    *out_value = (float)parser->number_value;
    json_parser_next(parser);
    return 1;
}

int json_parse_bool(JsonParser* parser, int* out_value)
{
    if (!parser || !out_value) {
        return 0;
    }
    if (parser->type == JSON_TOKEN_TRUE) {
        *out_value = 1;
        json_parser_next(parser);
        return 1;
    }
    if (parser->type == JSON_TOKEN_FALSE) {
        *out_value = 0;
        json_parser_next(parser);
        return 1;
    }
    return 0;
}

int json_parse_string_value(JsonParser* parser, char* buffer, size_t buffer_size)
{
    size_t i = 0u;
    size_t out = 0u;

    if (!parser || !buffer || buffer_size == 0u ||
        !json_expect(parser, JSON_TOKEN_STRING)) {
        return 0;
    }

    for (i = 0u; i < parser->token_length; ++i) {
        char ch = parser->token_start[i];

        if (out + 1u >= buffer_size) {
            return 0;
        }

        if (ch == '\\') {
            if (i + 1u >= parser->token_length) {
                return 0;
            }
            i++;
            switch (parser->token_start[i]) {
            case '"':
                ch = '"';
                break;
            case '\\':
                ch = '\\';
                break;
            case '/':
                ch = '/';
                break;
            case 'b':
                ch = '\b';
                break;
            case 'f':
                ch = '\f';
                break;
            case 'n':
                ch = '\n';
                break;
            case 'r':
                ch = '\r';
                break;
            case 't':
                ch = '\t';
                break;
            default:
                return 0;
            }
        }

        buffer[out++] = ch;
    }

    buffer[out] = '\0';
    json_parser_next(parser);
    return 1;
}

int json_skip_value(JsonParser* parser)
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

int json_skip_object(JsonParser* parser)
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

int json_skip_array(JsonParser* parser)
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
