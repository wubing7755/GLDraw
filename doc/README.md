GLDraw Documentation
====================

This directory contains only the small documentation that should live with the
repository: build instructions, run commands, basic controls, testing policy,
release policy, AI agent policy, and short architecture decision records.

Project architecture, source navigation, and module-level explanations are
maintained through Zread:

* Zread project page: https://zread.ai/wubing7755/GLDraw
* Repository README: ../README.md
* Chinese README: ../README.zh-CN.md


Quick Start
-----------

* Build and run GLDraw: See build.md
* Learn basic controls: See controls.md
* Understand testing expectations: See testing.md
* Prepare a release: See release.md
* Use AI agents safely: See ai-agents.md and agent-playbooks.md
* Review stable architecture decisions: See adr/README.md
* Read historical notes when needed: See archive/README.md
* Read project architecture: https://zread.ai/wubing7755/GLDraw
* Check the license: See ../LICENSE.txt


Essential Documentation
-----------------------

All users and contributors should be familiar with:

* Build requirements: build.md
* Runtime controls: controls.md
* Testing policy: testing.md
* Release policy: release.md
* AI agent policy: ai-agents.md
* AI agent playbooks: agent-playbooks.md
* Contributor guide: ../CONTRIBUTING.md
* Project overview: ../README.md
* Source and architecture guide: https://zread.ai/wubing7755/GLDraw


Documentation Policy
--------------------

Keep this directory short.

* Build, run, dependency, control, testing, release, and AI agent notes belong
  here.
* Short ADRs may live in adr/ when they record stable contributor-facing
  decisions.
* Historical notes may live in archive/ when they are retained for reference but
  no longer define the active workflow.
* Long architecture pages should stay out of this directory.
* Source maps, subsystem descriptions, and refactor narratives should be read
  through Zread or generated from the current source tree.
* Update build.md when build scripts, supported toolchains, or output paths
  change.
* Update controls.md when shortcuts or pointer behavior change.
