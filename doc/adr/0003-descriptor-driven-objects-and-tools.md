# ADR 0003: Descriptor-Driven Objects and Tools

## Status

Accepted.

## Context

GLDraw supports built-in and future extensible object and tool behavior. The
editor needs stable registration points without hard-coding every behavior into
the workspace or UI.

## Decision

Graphic objects are registered through `GraphicObjectDescriptor`, and tools are
registered through `ToolDescriptor`. Built-in registration is centralized in the
application manifests.

## Consequences

* New object behavior should be expressed through descriptor callbacks for
  lifecycle, geometry, hit-testing, properties, and persistence.
* New tools should use `register_tool()` or `register_shape_tool()` depending on
  whether the standard shape-drag lifecycle is sufficient.
* Built-in object registration belongs in `src/app/extension_manifest.c`.
* Built-in tool registration belongs in `src/app/tool_manifest.c`.
* The UI should discover and invoke registered behavior instead of duplicating
  object or tool rules.
