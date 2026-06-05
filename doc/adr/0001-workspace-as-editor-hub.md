# ADR 0001: Workspace as the Editor Hub

## Status

Accepted.

## Context

GLDraw coordinates document state, editing commands, tools, UI state, save/load
services, and rendering state. These areas need one integration point, but they
should not freely reach into each other's internals.

## Decision

`Workspace` remains the editor hub. It owns the editor core, editor session, and
editor services integration, while public workspace and controller APIs are the
preferred access path for other modules.

## Consequences

* New top-level editor behavior should normally enter through workspace,
  controller, service, or command APIs.
* Direct access to workspace internals should stay inside workspace
  implementation files.
* Tests may include internal headers only when they are explicitly verifying
  internal lifecycle behavior.
* UI, tools, rendering, and platform code should not become alternate editor
  hubs.
