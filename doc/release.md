Release Process
===============

This document defines the release workflow used by maintainers. Build commands
and package layouts are documented in build.md.


Versioning
----------

Use `vX.Y.Z` Git tags for published releases. The release workflow accepts either
a pushed tag such as `v0.0.3` or a manual workflow dispatch version such as
`0.0.3`.

Before tagging:

* Update CHANGELOG.md with user-visible changes.
* Confirm README and doc/build.md still match the packaged layout.
* Confirm release notes use the short format documented in build.md.
* Run the release smoke workflow for packaging changes.


Preflight
---------

For normal code releases:

```sh
./scripts/check.sh --preset release
```

For Linux sanitizer coverage when available:

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan --output-on-failure
```

For package-related changes, validate the release smoke workflow before
publishing.


Publishing
----------

Preferred path:

1. Ensure `main` contains the release commit.
2. Create an annotated tag:

   ```sh
   git tag -a vX.Y.Z -m "chore(release): vX.Y.Z"
   ```

3. Push the tag:

   ```sh
   git push origin vX.Y.Z
   ```

4. Wait for the release workflow to build Windows and Linux assets.
5. Confirm the GitHub Release contains the expected assets:

   ```text
   GLDraw-vX.Y.Z-windows-x64-setup.exe
   GLDraw-vX.Y.Z-windows-x64.zip
   GLDraw-vX.Y.Z-linux-x64.AppImage
   GLDraw-vX.Y.Z-linux-x64.tar.gz
   ```

Manual workflow dispatch may be used when maintainers need to rebuild assets
for an existing release version.


Smoke Validation
----------------

The release smoke workflow validates package structure without publishing a real
release. It should run for changes to:

* Release workflows.
* Packaging scripts.
* Installer or AppImage tooling.
* Resource layout.
* CMake install or bundle rules.


Hotfixes
--------

Use a `fix/<topic>` or `release/<version>` branch for hotfixes. Keep the patch
minimal, add a regression test when practical, update CHANGELOG.md, and publish a
patch version tag.


Legacy Releases
---------------

For older tags that predate current packaging scripts, use the `Legacy Release`
workflow. It builds from the target ref while using current packaging tools to
produce the expected release assets.
