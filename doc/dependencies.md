Dependencies
============

GLDraw keeps runtime dependencies small and explicit. This document records
where dependencies come from, how they are laid out, and what must be updated
when they move.


Build-Time Requirements
-----------------------

* CMake 3.15 or newer.
* CMake 3.21 or newer for presets.
* A C11 compiler.
* OpenGL runtime and development files.
* Git and network access for the first CMake configure when GLFW is not already
  cached.


Bundled or Fetched Dependencies
-------------------------------

* GLFW 3.3.9 is fetched by CMake through `cmake/Dependencies.cmake`.
* GLAD is committed in the repository:
  * `include/glad/`
  * `include/KHR/`
  * `src/glad.c`
* Nuklear is header-only and committed in:
  * `include/nuklear/`


Current Layout Policy
---------------------

The current third-party layout is intentionally unchanged:

```text
include/glad/
include/KHR/
include/nuklear/
src/glad.c
```

This avoids a noisy include-path migration while the source tree is still being
organized. See `doc/adr/0006-third-party-source-layout.md` for the decision.


When Updating Dependencies
--------------------------

When dependency versions, paths, or ownership change, update:

* `cmake/Dependencies.cmake`
* `cmake/Sources.cmake`
* `.clang-format`
* `.clang-tidy`
* `scripts/check.sh`
* `scripts/check.ps1`
* `.github/workflows/static-analysis.yml`
* `doc/build.md`
* This file
* Release packaging scripts when installed files or runtime assets change


License Notes
-------------

Keep third-party license requirements visible in release review. If a dependency
is added, moved, or replaced, include its license and redistribution notes in the
same pull request.
