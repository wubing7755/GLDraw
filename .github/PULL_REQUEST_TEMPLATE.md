<!-- PR title format: type(scope): summary, for example docs(ai): add agent policy -->

Closes #xx

## Summary
- ...
- ...

## Change Type
- [ ] Bug fix
- [ ] Feature
- [ ] Refactor
- [ ] Infrastructure / CI / packaging
- [ ] Documentation
- [ ] Release preparation

## Areas Touched
- [ ] Document model / persistence
- [ ] Commands / undo / redo
- [ ] Tools / input handling
- [ ] Workspace / session / services
- [ ] UI / menus / panels / dialogs
- [ ] Rendering / shaders / resources
- [ ] Build / tests / CI / release

## Architecture Checklist
- [ ] Durable document mutations go through `CommandExecutor`.
- [ ] UI code dispatches actions and does not own editing rules.
- [ ] Tool input flows through editor controller / tool runtime boundaries.
- [ ] Rendering changes consume scene state rather than defining document behavior.
- [ ] No new `workspace_internal.h` include was added outside workspace implementation files.
- [ ] Not applicable.

## Testing
- [ ] Local build completed.
- [ ] Relevant CTest tests completed.
- [ ] Format or static-analysis checks completed when applicable.
- [ ] UI/rendering screenshots or recordings attached when applicable.
- [ ] Not run; reason:

## AI Assistance
- [ ] AI assistance was used.
- Agent/tool:
- What the agent did:
- Required docs read:
- Human verification performed:
- [ ] Not applicable.

## Compatibility and Release Notes
- File format impact:
- Packaging or runtime dependency impact:
- User-visible change for changelog:

## Related
- Refs #yy

## Notes
- Optional: migration details, review context, or follow-up work.
