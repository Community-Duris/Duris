# Deliberate accounting port of #591, #522 and #511

## Scope and source

Base: `experimental-accounting` at
`903af10c97e717a5a6b05c4599fcc5796faed63c`.

The user authorized the selected changes to merge into master and then to be
brought deliberately into accounting. The source master merges are:

| PR | Master merge |
| --- | --- |
| #591 telemetry SQL gate | `6e82a0d3ad4f7f2ca00fe2f0b19125dfc2544758` |
| #522 rested toggle | `8497a4f78e03add5334bd5280bfade6e63620dfb` |
| #511 opt-in world activity | `1862a5fbdd03824363212df3a3c6ff00271a3f83` |

Port the selected net changes; preserve accounting's other implementation and
its sealed schema, operation identities, receipt formats and recovery fences.

## Required branch adaptations

- Accounting has 51 immutable migrations, ending at
  `0051_player_item_runtime_state`; master has 31. Canonical and staging-history
  tests assert accounting's current head and receipt count, and SQL fixtures
  discover the complete current manifest.
- The sealed accounting schema verifier accepts MySQL 8.0 and MariaDB 10.11.
  Use pinned `mysql:8.0.46` and `mariadb:10.11.14` for the accounting telemetry
  matrix. The initial newer-engine runs correctly failed at migration 0031;
  the verifier and sealed migrations were preserved.
- `newb_spellup` lives in `cmd/staff_newbie_aid.c`; apply the staff rested helper
  there. Community `newbsa` also uses the shared helper.
- `witch_doctor` lives in `specs/specs.heavens.c`; retain its active-accounting
  refusal before ownership or currency effects and add the rested gate there.
  The runtime harness asserts that the accounting refusal charges nothing.
- Keep account menu option 9 for retained death-recovery records while gating
  option 8. Keep accounting's track/damage headers and moved functions.
- Combat engagement lives in `combat/fight_state.c`; promote both participants
  after successful engagement there. Preserve the extracted combat/weapon
  functions rather than recreating their former definitions in `fight.c`.
- Preserve `item_transfer_reason::craft == 34` and the #551/#661 guards. The
  port does not modify the transfer enum, migrations, persistence authority,
  source claims, creation grant/save pipeline or quarantine release rules.
- The inherited combat journey's synthetic disputed-custody injector still
  expected format 4. Adapt that test fixture to current format 8 and its
  two-byte item equipment-slot suffix, preserving the actual header checksum,
  owner/item revisions and every operation/receipt byte. Production formats
  and codecs remain unchanged.
- World activity remains opt-in: code and shipped properties default to zero,
  including missing keys. See `ISSUE_299_WORLD_ACTIVITY.md` for the measured
  master capture and the broader rollout requirements still owned by #299.

## Qualification

All qualification uses disposable local state or databases.

- Both maintained full server builds passed with GCC 13.3; final engagement
  adaptation was rebuilt and linked for both MariaDB and flatfile backends.
- Production activity plus the actual scheduler passed under ASan/UBSan,
  including repeated dispatched successors, one-event rescheduling, bounded
  regions, corpse halo/counts, malformed graphs, cancellation and recovery.
- Scheduler budget/lifetime/cancellation, alchemist cadence/vial, equipment,
  world recovery, zone purge, publication retention, rested/UI/telemetry,
  community spell-up and all 23 immutable-migration tests passed.
- `test_telemetry_combat_hooks.py` executes both accepted combat-start activity
  promotions while preserving telemetry context boundaries.
- `test_item_transfer_version_compatibility.py` checks accounting's existing
  version/operation identities, including craft reason 34.
- Actual telemetry SQL repository runs passed on MySQL 8.0.46 and MariaDB
  10.11.14, all 51 migrations, record kinds 1–8, ten golden fixtures, replay,
  failure/isolation cases and disposable-target safety checks.
- The rested server journey passed with automatic bonuses disabled: actual
  fixed-input melee XP was 3/4/6 for ordinary/staff rested/staff well-rested.
  Staff flag 16 survived cold reload and copyover; live toggles and menu
  suppression passed. Final combined-port journeys are recorded in the PR.
- Authoritative whole-file clang-format and `git diff --check` passed. The WSL
  changed-line wrapper's NTFS temporary-checkout mode artifact has no textual
  formatting difference.

### Existing qualification limitation

`test_attack_continuation.py` fails before compiling its harness at line 619
with `ValueError: substring not found`. The same command against the untouched
accounting base reproduces the same failure. This port changes neither the
test nor the inspected `pv_common` implementation. Record this as existing
accounting test-contract drift; the focused accepted-start and real combat
journeys provide the applicable port evidence without claiming the complete
attack suite passed.

## Remaining integration

The current-head/count fixes required by #591 absorb the immutable-migration
expectation portion of #667. Refresh #667 to retain its two remaining collector
and corpse compatibility expectation updates. #668 remains a separate creation
grant/save prevention repair. Neither is silently included in this port.

#664 and #490 still own historical quarantine evidence and fault/recovery
qualification. Native accounting crafting (#551) and durable NPC vial issuance
(#661) retain their guards pending their own implementations. This port does
not establish production telemetry activation or broad world-activity rollout.
