# FAQ

## Where should new editor logic go?

- object data and selection: `document/`
- zoom, pan, transforms: `canvas/`
- interaction behavior: `tools/`
- drawing code: `render/`
- editor chrome: `ui/`

## Why are objects stored in world coordinates?

Because the canvas should be a view over the document, not the owner of geometry. That keeps resize, zoom, and pan separate from document data.

## What object types are supported?

Currently: Line, Rectangle, Ellipse. Each is implemented via `GraphicObjectVTable` polymorphism. See [Extending](Extending) to add new types.

## Why does the renderer work in line strips only?

The first refactor stage prioritized clear boundaries over a full retained rendering backend. Filled geometry can be added later.

## What is the next best subsystem to add?

Layers or grouping would allow organizing objects beyond a flat list.
