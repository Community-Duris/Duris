# Luna Max implementation handoff

Use the requested Luna model with Max reasoning for the implementation task. Model/reasoning selection is separate from the goal text; this handoff does not change settings or create an implementation task. All documents in this package describe proposed work, except the baseline source research and inventory.

The official [Follow a goal](https://learn.chatgpt.com/use-cases/follow-goals) guide describes `/goal <objective>` for sustained work with a verifiable stopping condition. The prompt below ties completion to explicit requirement evidence rather than a prototype or one passing test.

## Before starting

The plan currently lives in this detached research worktree:

```text
C:\Users\alexa\OneDrive\Documents\ChatGPT\NewDuris Max\artifact-control-research
```

The research files are local work products, not presumed to exist on origin/master. If implementing from another checkout, first copy the complete `docs/research/ARTIFACT_CONTROL_*.md` package and `artifact-control-source-map.json` into it, preserving the repository-relative links. Preserve unrelated files. Do not start by deleting/recreating this worktree. The implementation agent should create an isolated `codex/artifact-control` branch/worktree from current master and carry the package forward.

Read order:

1. `AGENTS.md` and `README.md`.
2. `docs/research/ARTIFACT_CONTROL_SPEC.md` — authoritative required behavior and architecture.
3. `docs/research/ARTIFACT_CONTROL_OPERATIONS.md` — required UI, CLI, SQL and operator journeys.
4. `docs/research/ARTIFACT_CONTROL_WORK_PACKAGES.md` — W01–W10, T01–T16, completion evidence.
5. `docs/research/ARTIFACT_CONTROL_PLAN.md` and `artifact-control-source-map.json` — researched baseline, limitations, code locations.

## Paste-ready goal

```text
/goal Implement the complete artifact control system specified by docs/research/ARTIFACT_CONTROL_SPEC.md, docs/research/ARTIFACT_CONTROL_OPERATIONS.md, and docs/research/ARTIFACT_CONTROL_WORK_PACKAGES.md in this Duris repository. Use the source research in docs/research/ARTIFACT_CONTROL_PLAN.md and artifact-control-source-map.json as evidence, rechecking current master and applicable AGENTS.md before modifying code. Preserve the complete specification scope: all R01-R10 requirements, W01-W10 work packages, U01-U10 operator journeys, and T01-T16 verification obligations.

Deliver a usable in-game immortal workbench with searchable/paginated lists, typed balance and lifecycle editors, guided load-point/equipment selection, player/NPC previews, saved/resumable drafts, conflict resolution, review/publish/status/history/rollback and role enforcement. Also deliver no-game-login config import/export and an artifactctl CLI, plus a restricted SQL draft-edit/submit/status workflow that uses the same validation and publication service. A raw JSON editor, property dump, parser-only prototype, or SQL schema without working runtime/UI integration is not completion.

Centralize artifact identity, uniqueness, placement/reset policy, lifetime/binding/feed rules, existing script/adapter ownership and supported balance controls. Support per-template, per-placement and per-UID variant selection, legacy/reworked coexistence, holder-following and pinned instances, and explicit player/wild-NPC/player-controlled-NPC policy. Complete every existing new-mode family named in the specification. Preserve declared legacy-only behavior for other artifacts while providing their common controls and truthful capabilities; do not invent unreviewed powers or claim unsupported variants work.

Reuse existing item action, mana, custody and artifact/guild persistence services. Preserve UID, scarcity, expiration, binding, paid reserve, cooldown obligations and exactly-once selected-power ownership across transfer, death/loot, mode changes, reload, restart/copyover and rollback. Never fall back to free legacy effects on new-mode failure. File, in-game and SQL changes must become immutable validated publication requests with expected revisions, atomic head change, explicit applied acknowledgement, audit, bounded work, crash recovery and no gameplay-thread blocking I/O. Implement both supported SQL engines and flat-file primary with additive guarded migrations, compatibility/lifecycle/backup/restore updates, and no storage-failure fallback authority.

Work through W01-W10 in dependency order on an isolated codex/artifact-control implementation branch/worktree, preserving these local plan files and unrelated work. Make routine implementation decisions consistent with the spec and continue without requesting approval at every phase. Keep docs/implementation/ARTIFACT_CONTROL_PROGRESS.md with the current stage, changed files, requirement status, exact validation commands/results and remaining work so continuation can resume from evidence. Reconcile changed source locations with current master without weakening requirements. Do not mark planned tests as passed or claim fixture-only evidence is a real server journey.

Build and format touched code, run meaningful focused tests for each package, exercise the real immortal editor through Telnet, and exercise config/CLI/restricted SQL workflows against disposable running and stopped server fixtures. Complete concurrency, invalid-input, permissions, identity/transfer, resource/cooldown continuity, crash/restart, both SQL engine and flat-file backup/restore tests. Use the final requirement-to-evidence audit and inspect actual outputs before declaring completion. Finish only when all specified controls and required journeys work and the implementation documentation matches tested commands; report the final build revision and evidence, and retain the active goal if a required outcome remains unverified.

Keep shipped live defaults compatible and modern balance changes confined to explicit fixtures/candidates. Do not access or mutate production data, run production migrations, deploy, or merge as part of this goal. Private DurisStudio UI implementation and redesigning the remaining legacy-only powers are outside scope; the public Studio contract and existing typed abilities are inside scope. Those boundaries must not be used to omit the in-game workbench, offline tooling, SQL authoring, lifecycle controls or completed existing adapters.
```

## How to judge the result

The implementation should let an authorized immortal find Tsunami, change its supported wild-NPC mode while retaining the player mode, tune its cooldown, change its future load placement, preview the consequences and publish one reviewed candidate. An operator outside the game must be able to make the same valid change using a file/CLI or restricted SQL draft, and see when it is applied. An invalid or stale candidate must leave the game on its prior valid configuration. Looting, switching versions, or restarting must not duplicate the artifact or replenish its timer/resource/cooldown state.

The plan-writing goal ends when this implementation package is complete and audited. The separate goal above ends only when the specified software is implemented and verified. Do not confuse those two completion conditions.
