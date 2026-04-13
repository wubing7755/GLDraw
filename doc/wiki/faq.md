# FAQ

## What changed in the refactor?

The old direct-coupled runtime was removed. The project now separates:

- document data
- canvas view state
- tool routing
- rendering
- UI

## Why keep old wiki filenames like `core-systems-shape.md`?

To preserve links. The content now explains the new `GraphicObject` model.

## Where should new editor logic go?

- object data and selection: `document/`
- zoom, pan, transforms: `canvas/`
- interaction behavior: `tools/`
- drawing code: `render/`
- editor chrome: `ui/`

## Why are objects stored in world coordinates?

Because the canvas should be a view over the document, not the owner of geometry. That keeps resize, zoom, and pan separate from document data.

## What is the next best subsystem to add?

Layers or persistence are the strongest next steps now that basic undo/redo exists.

## Why does the renderer work in line strips only?

Because the first refactor stage prioritized clear boundaries over a full retained rendering backend. Filled geometry and batching can be added later.
