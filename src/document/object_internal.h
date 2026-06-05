/**
 * @file object_internal.h
 * @brief Shared helpers for builtin object type implementations.
 *
 * These functions live in the internal object modules and are exposed here so
 * that separate translation units (object_line.c, object_rect.c,
 * object_ellipse.c, object_fake_star.c) can reuse them without duplicating
 * logic.
 *
 * This header is NOT part of the public API — it is internal to
 * src/document/ and src/app/extension_loader.c.
 */
#ifndef GLDRAW_DOCUMENT_OBJECT_INTERNAL_H
#define GLDRAW_DOCUMENT_OBJECT_INTERNAL_H

#include <document/object.h>

/**
 * @brief Read a style-related scalar property from a GraphicObject.
 *
 * Handles "stroke_r", "stroke_g", "stroke_b", "stroke_a", "stroke_width".
 * Type-specific properties must be handled by the type's own get_scalar.
 */
int style_get_scalar(const GraphicObject* object, const char* key, float* out_value);

/**
 * @brief Write a style-related scalar property to a GraphicObject.
 *
 * Handles the same keys as style_get_scalar.
 */
int style_set_scalar(GraphicObject* object, const char* key, float value);

/**
 * @brief Clone an object by serializing it to a property bag and
 * deserializing from the bag. Works for any object whose descriptor
 * provides valid serialize and deserialize functions.
 */
GraphicObject* default_clone_object(const GraphicObject* object);

/**
 * @brief Serialize an object by walking its descriptor's property_schema
 * and reading every property via get_scalar / style_get_scalar.
 */
int default_serialize_from_schema(const GraphicObject* object,
                                  GraphicPropertyBag* out_properties);

/**
 * @brief Apply serialized style properties to an existing object.
 *
 * Reads optional "stroke_r", "stroke_g", "stroke_b", "stroke_a", and
 * positive "stroke_width" values from @p properties.
 */
void default_apply_style_properties(const GraphicPropertyBag* properties,
                                    GraphicObject* object);

/**
 * @brief Allocate and initialize a GraphicObject with the given fields.
 *
 * Takes ownership of @p impl on success; caller must NOT free it.
 * On failure, frees @p impl and returns NULL.
 */
GraphicObject* object_alloc(GraphicObjectType type,
                            const GraphicObjectDescriptor* descriptor,
                            void* impl,
                            GraphicStyle style);

/**
 * @brief Destroy a GraphicObject allocated with object_alloc().
 */
void default_destroy_object(GraphicObject* object);

#endif /* GLDRAW_DOCUMENT_OBJECT_INTERNAL_H */
