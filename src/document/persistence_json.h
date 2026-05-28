#ifndef GLDRAW_DOCUMENT_PERSISTENCE_JSON_H
#define GLDRAW_DOCUMENT_PERSISTENCE_JSON_H

#include <stddef.h>

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

void json_parser_next(JsonParser* parser);
int json_token_is_string(const JsonParser* parser, const char* literal);
int json_expect(JsonParser* parser, JsonTokenType type);
int json_parse_u32(JsonParser* parser, unsigned int* out_value);
int json_parse_float(JsonParser* parser, float* out_value);
int json_parse_bool(JsonParser* parser, int* out_value);
int json_parse_string_value(JsonParser* parser, char* buffer, size_t buffer_size);
int json_skip_value(JsonParser* parser);
int json_skip_object(JsonParser* parser);
int json_skip_array(JsonParser* parser);

#endif /* GLDRAW_DOCUMENT_PERSISTENCE_JSON_H */
