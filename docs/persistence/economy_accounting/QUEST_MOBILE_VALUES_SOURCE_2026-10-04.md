# Quest mobile native values and complete stock capture — source checkpoint

## Integrated NPC values and stock capture component — 2026-10-04

The exact reviewed quest_mobile_native C871b2473/H2a149708 is now source-integrated
and registered in the maintained Makefile. It supplies canonical values and pure
complete ordered NPC EQ/INV capture, including the corrected shared size estimate.
This changes no birth, ID allocator, native custody, quest/lifecycle route, activation
or inactive behavior. Current candidate source pins are refreshed; no coverage
completion is inferred. Builds/tests remain deferred to major-plan readiness.
The previous e018 SQL build/restore evidence remains valid for that historical
tree and does not qualify this newly extended native candidate. Actual native SQL/
flat participant, owner/source authority, rebind, producer and guarded ACK remain
required. See [the detailed source checkpoint](QUEST_MOBILE_VALUES_SOURCE_2026-10-04.md).

The following records the private preparation and source review preceding this
component integration. Native build and runtime qualification remain pending.
Primary published base before this checkpoint is5e7a0a768. The independent
Plan5 SQL build/restore evidence applies to published native treee0185879 only.

## Implemented and reviewed

Private files under `tmp/plan3-quest-mobile-native-values-proposal-20261004/`
implement canonical native-mobile reference/image value codecs and read-only
complete NPC stock capture. Reference framing is148 bytes; image overhead is216
bytes plus the existing item codec, with the original4MiB/4096-object/depth32
bounds. Existing48-byte economic_source_event encoding is reused exactly;
checksums establish integrity, not authenticated birth/source authority.

Equipment roots retain ascending one-based native slots, followed by actual
carried-root order and contiguous depth-first subtrees. Full literal strings,
NORENT stock, dynamic affects and ordered descriptors use the existing literal
tree capture. Cross-root UID/pointer aliases, cycles and reciprocal physical
location errors refuse; output remains unchanged on failure. Explicit LIVE
capture cannot revive RETIRED values, whose encoded stock must be empty.
Zero reset zones and signed birthplace values are preserved from actual native
source semantics rather than receiving invented positive-only rules.

Independent source review established one composition defect: existing tree
capture includes sizeof(player_snapshot) in every estimate. Summing whole
estimates overcharged a multi-root forest. The correction initializes one shared
header and adds only each tree's estimate minus that header with safe bounds,
following the already accepted complete-keeper capture policy. Encoded image
size is still independently bounded. Original worker inputs/preimage remain
preserved, including the initially overstrict candidate.

## Frozen evidence

- Candidate C:`871b247307723cbdae9e5aa29a21ed8f0f48bfb90ff5c1202b8f5b7bcfe8601d`.
- Candidate H:`2a1497080811cc6e85ba58c89846a85fce6cd275d8812101dcb2a5e358b91dd4`.
- Initial C before review:`b5916bf230ad8ac2e8f3a294be148a6dff10ba46ad59c1767f7e551bd39b70b6`.
- Source receipt:`tmp/plan3-quest-mobile-native-values-proposal-20261004/source-pins.json`.

Nine selected preserved inputs are comparison evidence, not a complete compiler
closure. Independent final source review, changed-line clang18 fixed point and
whitespace checks pass. No compiler, tests, SQL, services, migrations, gameplay
or native recovery ran. Qualification stays at the agreed major-plan batch.

## Required integration remains

Actual native birth/ID assignment and source proof, SQL/flat participants and
shared custody contracts, sequential addressed-NPC quest producer/reward binding,
publication/guarded ACK, pre-reset roster rebind, versioned world/copyover data,
death/extraction/disappearance stock transitions and original backend/player
journeys remain required. Caller-supplied IDs/operations/revisions/state are
values only. No new mutable UID catalog, allocation counter, queue or release
gate is introduced. This slice does not complete Plan3, writer coverage or R1–R8.
Current inactive behavior and the declined spell-path boundary are unchanged.
