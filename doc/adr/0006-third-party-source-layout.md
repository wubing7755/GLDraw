# ADR 0006: Third-Party Source Layout

## Status

Accepted.

## Context

GLDraw currently commits GLAD and Nuklear directly under the normal source and
include roots:

```text
include/glad/
include/KHR/
include/nuklear/
src/glad.c
```

This layout is simple for the current CMake targets, but it mixes third-party
headers with project-owned headers. Moving these files to `third_party/` would
make ownership clearer, but it would also touch include paths, static-analysis
exclusions, formatting exclusions, packaging assumptions, and review history.

## Decision

Keep the current third-party file locations for now. Document the dependency
ownership and revisit a `third_party/` migration only when there is a concrete
maintenance need, such as adding another vendored dependency or updating GLAD or
Nuklear.

## Consequences

* No source files move in this ADR.
* CMake include paths, package layouts, static-analysis suppressions, and local
  check scripts remain stable.
* `doc/dependencies.md` records current dependency locations and the checklist
  for future moves.
* A future `third_party/` migration should happen in its own PR and must update
  CMake modules, check scripts, CI exclusions, documentation, and license notes
  together.
