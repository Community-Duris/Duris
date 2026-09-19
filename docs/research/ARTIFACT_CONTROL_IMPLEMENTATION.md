# Artifact control implementation report

This report accompanies the artifact control pilot. The source audit starts at
`440248b17eecc3517229a48a4946cf6c0a33ffa5` and covers the current source tree,
not live production custody or balance state.

## What the audit found

Artifacts are assembled from several independent authorities. Object prototypes
in `areas/obj/*.obj` define vnums, keywords, flags, values, affects, and text.
Zone resets in `areas/zon/*.zon` use `M/E/G/O/P` commands, limits, chance,
conditional flags, and the last loaded mob or room. Native callbacks are bound
by vnum in `src/specs/specs.assign.c` and implemented across the `src/specs`
tree. Object values, extra descriptions, `_proclib_` hooks, Studio triggers,
properties, and the item-action scheduler can all add behavior.

The existing immortal surface is therefore investigative and state-oriented:
`artifact major|unique|ioun` lists records, `artifact timer` changes an existing
expiry deadline, `artifact poof` removes a tracked instance, `artifact reset`
repairs soul binding, `artifact swap` changes a tracked template, and `zreset`
reruns a zone. None of those is a single editor for prototype, reset placement,
legacy/new behavior, holder policy, or power tuning. Properties can tune some
new adapters, but they are float-keyed, wildcarded, and independent from
artifact identity and custody.

The source inventory contains 169 flagged templates, 78 literal native bindings,
67 callbacks, 91 templates without a literal callback, and 29 placeholder
tokens. The pilot deliberately does not invent modern implementations for the
remaining legacy-only templates. The source map records their callback and
reset evidence so later migrations can be reviewed individually.

## What this PR adds

* `lib/artifacts/catalog.json` is a validated, hashed control catalog. Each
  definition records classification, uniqueness family, legacy binding, source
  provenance, load room/mob/slot/chance/limit, lifetime, enabled state, holder
  variants, compiled adapter IDs, and named powers. The pilot definitions are
  Mayhem, Symmetry, mirrored ioun, Avernus, Tsunami, wand of wonder, and Living
  Necroplasm.
* `src/artifact/artifact_control.*` provides strict typed parsing, canonical
  FNV hashing, immutable active/draft snapshots, validation, atomic publication,
  variant and power policy resolution, status, inspect, list, preview, discard,
  and reload. The catalog loads during property initialization and never performs
  file I/O in an item action.
* `artifact control ...` is an additive immortal command namespace. Forger rank
  can stage holder mode or enablement changes, preview them, publish a monotonic
  revision, discard a draft, and reload a catalog. Existing artifact list/timer/
  poof/swap/reset commands retain their old semantics.
* `scripts/artifactctl.py` gives developers a no-login validator, inspector,
  staged editor, and atomic publisher. Repeated staged edits preserve the draft;
  the active file changes only on publish.
* `migrations/immutable/0029_artifact_control.sql` adds revision/draft/head,
  holder and power projections, publish audit, idempotent request/result inbox,
  and a writer lease. Runtime and lifecycle manifests are resealed for 211
  tables. The SQL verifier is additive and has a rolled-back request/result
  smoke test.
* The native bridge routes the pilot through holder-aware legacy/telegraphic
  policy. It covers Tsunami, mirrored ioun, Living Necroplasm, Mayhem/Symmetry,
  Avernus, and wand of wonder. Disabling a catalog definition suppresses both
  modes. A `legacy` holder returns to the existing callback; a `telegraphic`
  holder reaches the compiled adapter. A failed modern admission does not spend
  another random roll by falling through to legacy. The shipped revision keeps
  every player, wild-NPC, and controlled-NPC policy on `legacy`; telegraphic is
  an explicit per-artifact/per-holder canary choice.
* Named power enablement is enforced at the adapter boundary. `powerLevel`
  currently feeds the necroplasm transformation, wand-of-wonder selection,
  Avernus drain cap, and sword combat selection. Chance, cooldown, windup, and
  mana fields are published metadata until their legacy timer/resource owner is
  migrated; this is exposed in the operations guide instead of being implied.
* C++ and Python harnesses cover active-vs-draft publication, duplicate and hash
  rejection, holder routing, and the existing item-action/native/mana behavior.

## Safe rollout and next steps

1. Run the CLI validator against the exact catalog intended for a staging boot.
   In-game, inspect the same vnums and holder policies before publishing.
2. Confirm the properties required by the modern pilot are valid while every
   holder remains on the shipped legacy baseline. Select one artifact and one
   holder class for an explicit telegraphic canary, then exercise that path
   alongside the legacy path, including transfer, cancellation, and power
   suppression.
3. Publish one small revision at a time. Record the revision/hash and retain the
   previous catalog for a new-revision rollback. A variant edit does not recall
   an existing UID, reset its expiry, refill mana, or move its custody.
4. Apply migration 0029 only through the existing migration runner and run its
   verifier on disposable MySQL 8/MariaDB 10.11 databases. The checked-in SQL
   was syntax-reviewed and manifest-validated here, but no live database was
   available in this workspace.
5. The next implementation wave should move fresh-load reservation and instance
   policy behind the same service. That is when catalog load points can become
   authoritative for reset/recovery instead of remaining reviewed metadata. The
   following wave should migrate timer, cooldown, mana, feed, binding, and war
   calculations, then add the full immortal workbench and restricted SQL writer.

## Validation evidence

Passing checks in this workspace:

```text
python tests/async/test_artifact_control.py
wsl bash tests/async/run_artifact_control_harness.sh
wsl python3 scripts/artifact_source_inventory.py --check
python scripts/validate_runtime_compatibility.py
python scripts/validate_data_lifecycle.py
wsl python3 tests/async/test_item_actions_runtime.py
wsl python3 tests/async/test_weapon_actions_runtime.py
wsl python3 tests/async/test_native_artifact_runtime.py
wsl python3 tests/async/test_sword_actions_runtime.py
wsl python3 tests/async/test_wonder_actions_runtime.py
wsl python3 tests/async/test_artifact_mana_runtime.py
wsl python3 tests/async/test_artifact_mana_game.py
wsl python3 tests/async/test_artifact_mana_restore.py
wsl python3 tests/async/test_data_lifecycle_manifest.py
wsl python3 tests/async/test_collector_catalog_schema.py
wsl python3 tests/async/test_corpse_lifecycle_repository.py
wsl python3 tests/async/test_immutable_migration_runner.py
```

The new and changed C++ translation units compile with the repository warning
profile in focused WSL builds. A complete `make -C src` was attempted, but the
installed GCC 11 rejects pre-existing `std::atomic<std::shared_ptr<...>>` code
in `net/output_profiles.h`; the failure occurs in unrelated baseline files.
Windows launches of the C++ test scripts also lack `g++`, so those tests were
rerun successfully under WSL. No production server or database was modified.
