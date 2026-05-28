#include <document/persistence.h>

#include <base/file_utils.h>

#include <stdio.h>
#include <stdlib.h>

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

static int write_json_string(FILE* file, const char* text)
{
    const unsigned char* cursor = (const unsigned char*)(text ? text : "");

    if (!file) {
        return 0;
    }

    fputc('"', file);
    while (*cursor) {
        switch (*cursor) {
        case '"':
            fputs("\\\"", file);
            break;
        case '\\':
            fputs("\\\\", file);
            break;
        case '\b':
            fputs("\\b", file);
            break;
        case '\f':
            fputs("\\f", file);
            break;
        case '\n':
            fputs("\\n", file);
            break;
        case '\r':
            fputs("\\r", file);
            break;
        case '\t':
            fputs("\\t", file);
            break;
        default:
            if (*cursor < 0x20u) {
                fprintf(file, "\\u%04x", (unsigned int)*cursor);
            } else {
                fputc((int)*cursor, file);
            }
            break;
        }
        cursor++;
    }
    fputc('"', file);
    return ferror(file) == 0;
}

int write_document_json(FILE* file, const Document* document)
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
        fprintf(file, "      \"name\": ");
        if (!write_json_string(file, layer->name)) return 0;
        fprintf(file, ",\n");
        fprintf(file, "      \"visible\": %s,\n", layer->visible ? "true" : "false");
        fprintf(file, "      \"locked\": %s,\n", layer->locked ? "true" : "false");
        fprintf(file, "      \"blend_mode\": ");
        if (!write_json_string(file, layer_blend_mode_name(layer->blend_mode))) return 0;
        fprintf(file, "\n");
        fprintf(file, "    }%s\n", (i + 1 < document->layer_count) ? "," : "");
    }

    fprintf(file, "  ],\n");
    fprintf(file, "  \"objects\": [\n");

    for (i = 0; i < document->count; ++i) {
        const GraphicObject* object = document->objects[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"id\": %u,\n", object->id);
        fprintf(file, "      \"layer_id\": %u,\n", object->layer_id);
        fprintf(file, "      \"type\": ");
        if (!write_json_string(file, object_type_id(object))) return 0;
        fprintf(file, ",\n");
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
                fprintf(file, "%s ", property_index == 0 ? "" : ",");
                if (!write_json_string(file, properties.values[property_index].name)) {
                    return 0;
                }
                fprintf(file, ": %.9g", properties.values[property_index].value);
            }
            fprintf(file, " }\n");
        }

        fprintf(file, "    }%s\n", (i + 1 < document->count) ? "," : "");
    }

    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    return ferror(file) == 0;
}
