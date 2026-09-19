# Artifact control planning and pilot implementation

This package is the research baseline and implementation contract for centralized control of artifacts, uniques, iouns, classic/reworked powers, load/reset/lifetime policy, and usable immortal plus no-login configuration/database workflows. This PR adds the revisioned pilot catalog, runtime/admin controls, CLI, adapter routing bridge, and additive SQL authority described in [the operations guide](../operations/ARTIFACT_CONTROL.md). The larger work-package specification remains the roadmap for migrating every legacy load and instance-state path. Research baseline: `440248b17eecc3517229a48a4946cf6c0a33ffa5`, fetched 2026-09-18.

| Document | Purpose |
| --- | --- |
| [Source research and recommendation](ARTIFACT_CONTROL_PLAN.md) | Existing commands, source paths, defaults, limitations, timers and pilot behavior. |
| [Normative implementation specification](ARTIFACT_CONTROL_SPEC.md) | R01–R10, modules, configuration schema, database/file authority, publication/recovery, holder/instance policy and migration. |
| [Operator experience](ARTIFACT_CONTROL_OPERATIONS.md) | Concrete Telnet screens/navigation/editing flows, permissions, CLI commands, SQL procedures/examples, failures and U01–U10 journeys. |
| [Implementation backlog and tests](ARTIFACT_CONTROL_WORK_PACKAGES.md) | W01–W10 dependencies/deliverables, T01–T16 acceptance tests, existing regression commands, requirement evidence. |
| [Luna Max /goal handoff](ARTIFACT_CONTROL_GOAL.md) | Read order, local-worktree transfer instructions, complete paste-ready implementation objective. |
| [Source map](artifact-control-source-map.json) | 169 source templates, native definitions, and bounded direct-load evidence from active areas. |
| [Planning completion audit](ARTIFACT_CONTROL_PLANNING_AUDIT.md) | Verification of this planning deliverable, separate from future implementation evidence. |
| [Implementation report](ARTIFACT_CONTROL_IMPLEMENTATION.md) | Pilot behavior, rollout steps, scope limits, and validation evidence for this change. |

The system will have three first-class authoring interfaces: a guided in-game workbench; file import/export plus `artifactctl`; and restricted SQL draft editing/submission. All submit through one validation/publication service. No interface writes raw changes into live ownership or mana records.

The selected architecture keeps existing execution and persistence modules, adds typed configuration/policy/services, and preserves exact artifact identity across variants. Classic/reworked selection is independent of scarcity. Player, wild-NPC and player-controlled-NPC policies can differ, with supported per-placement and per-UID overrides. Invalid edits keep the prior valid runtime configuration; accepted publication has durable and applied states that operators can distinguish.

The implementation contract covers all common control surfaces across the full catalog and all already-delivered modern adapters. It does not invent redesigned powers for every legacy-only template. Such templates retain their declared native behavior, common controls and truthful capability diagnostics; a later power adapter follows the same registry/parameterization/test pattern.

All new command/table/test names in the specification are proposed deliverables. Existing functionality is identified in the source research. Do not run proposed SQL or CLI examples against the current server and expect them to exist.
