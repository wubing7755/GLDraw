# Security Policy

## Supported Versions

Security fixes target the current `main` branch and the latest published
release when a backport is practical. Older releases may receive fixes when the
impact is high and the patch can be applied safely.

## Reporting a Vulnerability

Do not publish exploit details in a public issue.

Preferred reporting path:

1. Use GitHub private vulnerability reporting if it is enabled for the
   repository.
2. If private reporting is not available, open a minimal public issue asking for
   a private maintainer contact and omit sensitive details.

Please include:

* Affected version, commit, or release asset.
* Operating system and toolchain.
* Reproduction steps or a proof of concept.
* Impact assessment.
* Whether the issue affects source builds, packaged releases, or both.

## Dependency Security

GLDraw uses CMake, GLFW, GLAD, Nuklear, OpenGL platform libraries, and packaging
tools. Dependency updates should go through normal pull request review and CI.
Security-driven dependency updates may be handled outside the regular release
cadence when the risk justifies it.

## Disclosure

Maintainers should acknowledge reports as soon as practical, coordinate a fix in
a private branch or security advisory when available, and publish release notes
once users have a patched version or clear mitigation.
