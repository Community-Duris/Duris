# Legacy starter-kit preparation

Issue: [Community-Duris/Duris #160](https://github.com/Community-Duris/Duris/issues/160)

The legacy kit table is initialized once and selects an ordered, owned plan from
copied race, class, alignment, override, and special-item inputs. Repeated VNUMs
are intentional. The game-thread capture adapter snapshots spell IDs and weapon
admission using the existing skill and item-use predicates; pure preparation
filters unavailable or inadmissible items before any live object is allocated.

During boot, after the object index, pool and special-procedure setup, `boot_db`
parses every VNUM enumerated by the same kit table and its special additions.
`object_template` owns strings, extra descriptions and scalar prototype fields.
Cache construction creates no object identity, live object, list entry or event.
Missing VNUMs are logged at boot and omitted, matching the legacy missing-item
behavior. Runtime cache misses cannot fall back to file parsing.

`instantiate_object_template` runs on the game thread. It registers the live
object, preserves nullable/shared indexed text and independent extra descriptions,
and invokes runtime procedures, events and object conversion. Ordinary
`read_object` callers use the same parser/publication implementation and retain
cold loading and the shared-index-string fast path. Starter publication applies
legacy cost/transience, spellbook counts and keywords, then uses `obj_to_char`
and the existing creation-grant coordinator.

The durable character baseline still precedes kit submission. A busy coordinator
prevents a repeated request while earlier items are awaiting publication, even
when carried inventory is empty. Grant validation and publication resolve retained
live characters by PID, so losing a descriptor does not abandon a committed item
or the remaining queue. Existing ownership revisions, operation IDs, retries,
replay receipts, command/prompt gates, and snapshot save ordering remain authority.
No new worker, persistence format, database migration or recovery queue is added.

## Verification

Focused tests are automatically discovered by the normal regression runner:

- `test_newbie_kit_plan.py`: frozen legacy-table digest, all 471 table cells,
  36,360 selection combinations, template enumeration, pure preparation and
  integration boundaries. Includes the inclusive `LAST_RACE` boundary safely.
- `test_newbie_object_template.py`: actual parser/cache/publication functions,
  closed-file instantiation, allocator ownership, nullable strings, B5/A/T data,
  normalization and deferred runtime hooks, with ASan/UBSan.
- `test_newbie_grant_lifecycle.py`: held completion, unrelated actor dispatch,
  linkdead and replacement-character publication, duplicate completion,
  rejection cleanup, refused admission/retry and pre-entry grants, with ASan/UBSan.

The lifecycle test fails against the old descriptor-based publication code.
Snapshot pipeline, critical-command journal/retry and disposable flat-file
ownership-replay regressions also pass. `make -C src` passes with the repository's
strict warning profile. These are synthetic and local checks, not a live latency
benchmark or production database test. Replay guarantees remain per accepted
ownership operation; this change does not introduce a durable kit-level receipt
or redesign existing ambiguous-commit recovery.
