# Selection System

## Overview

Selection is now owned by `Document`, not by a separate selection manager module.

The document stores a `SelectionSet` of `ObjectId` values.

## Behavior

- single click selects one object
- `Shift+Click` toggles an object in the set
- click on empty space clears selection
- dragging a selected object moves every selected object

## Why ObjectId

The previous implementation selected by raw pointer. The new model stores ids so selection remains aligned with document ownership.

That is a better base for:

- undo/redo
- serialization
- multi-view editing
- layer reordering
