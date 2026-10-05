# Plan 5 retained baseline equipment format handoff

The current native EAB1 witness loses original equipment position on decode.
The native command rehydrator accepts the decoded witness and generates a
different EAP1 plan. Independent readers correctly refuse the equipped original
because its retained EAP1 disagrees with EAB1. This is an established shared
format/replay gap, not a completed native fix or release qualification.

## Delivery and exact source

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Previously delivered tip: `673b9f0f2970bd0bf7423822e2db1811ede515f0`.
- Refreshed primary consumed: `9a02a0ee5d2e0490a8e09a710aac6af09f358b7d`.
- Preserving integration/base: `7bfd5347e7e47e4ce5206839c353bd620d882ffc`.
- Native tree: `08727e151d8086c57c82449f2c456cf7dec45782`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical
  history through0056. The original EAB1 SQL shape is in immutable0032.
- Result SHA and verified remote tip: `tmp/plan5/baseline-equipment-delivery.json`.

This report is the sole owned tracked file. No native producer, codec,
coordinator, migration, contract, matrix/registry, activation or audit algorithm
is independently changed. All earlier Plan5 work, including the canonical,
published0056, coherent0056, audit/release, qualification and recovery branch
tips, remains an ancestor of the requested delivery branch. Their reports retain
their historical source facts. The new native tree is not qualified by earlier
0055,255d,d9fc or other source results.

## Established defect and limits

The probe uses the production `economic_baseline_prepare`, EAB1 encode/decode,
`economic_baseline_command_build`, CCM1 encoder,
`economic_baseline_command_plan` and EAP1 encoder. It retains their exact output
bytes. Each native configuration covers player owner1 slots0,1,43,65535 and
native-mobile owner12 slots0,1,43,44. Native admits seven cases and refuses
mobile44. Player65535 covers the current historical native grammar; this does
not assert that a gameplay equipment slot of that number exists.

For the five admitted equipped cases, EAB1 decode succeeds but returns slot0.
Rehydrating the original command against that decoded preparation also succeeds
and returns a carried-item EAP1 different from the original equipped EAP1.
For a given owner, the controlled carried/equipped batches have identical
identity, source fingerprint, boundary and coverage facts. They yield identical
EAB1, CCM1, EAI1, prepared domain digest and prepared intent digest, despite
different original EAP1 plans. The production source fingerprints are opaque
here: this establishes the native API's missing field and lossy roundtrip, not
a claim that a real producer would hash two changed physical snapshots equally.

EAB1 currently stores a192-byte header,112-byte holdings and88-byte item records.
An item record contains UID, owner, state, root, parent, revision and a32-byte
source digest, but no `economic_item_position::equipment_slot`. Baseline
preparation's domain encoding also omits that field. EAP1 retains it.

The independent origin verifier compares original EAP1 snapshots against EAB1;
it accepts the two carried originals and refuses all five equipped originals
with `EAB1 committed root mismatch`. This refusal must remain. It must not be
weakened to ignore equipment differences, treat absence as authenticated zero,
or reconstruct a historical slot from the current projection.

Native outer replay code compares regenerated EAP1 to the retained original:
flatfile baseline loading explicitly checks `encoded == retained.plan`, and
the SQL evidence comparison checks the regenerated root fields and plan. Those
guards should preserve refusal when the original cannot be reproduced. These
outer repository/replay paths are inspected, not executed by this probe. The
finding is a reproducibility/qualification gap; no silent repository rewrite is
claimed. Neither the probe nor either audit reader corrects the disagreement.

All commands use `accepted_at_usec=1` to isolate this field from the separately
open SQL admission-time retention gap. No admission time is inferred or added
to a retained history, and this fixture does not qualify ordinary production
admission-time authentication.

## Native and disposable database checks

Native command:

```text
python3 -u -B tmp/plan5/run-baseline-equipment-probe.py
```

Both fresh configurations use strict C++20 `-Wall -Wextra -Wpedantic -Werror`,
ASan/UBSan, `-D__NO_TESTS__`, `-lcrypto`; flatfile also uses `-D__NO_MYSQL__`.
The exact14 production/generated translation units and full flags are retained
in `native-builds.json`. No binary is reused. Both runs exit0 with no sanitizer
stderr, reproduce the same eight decisions/bytes and produce binary SHA-256
`e7f597a228783ebf9b7b4fe88b78118b7422dcdd65823664c309b004d1aa1089`.
Both independent decisions and the five lossy roundtrips are checked explicitly.

Disposable database command:

```text
python3 -u -B tmp/plan5/run-baseline-equipment-sql.py
```

Final SQL checks pass in423.119 seconds on MariaDB10.11.14 and MySQL8.0.46,
with four fresh source schemas and four additional cold restore daemons. Both
owner1 and owner12 carried originals pass in each engine. Both equipped originals
are refused at source and again after cold import by both independent readers.
There are24 API checks (eight accepted,16 refused) and12 canonical CLI checks
(four exit0, eight exit2). All selected checks execute; no unittest case is
skipped. The unselected origin CLI limitation is disclosed below. The two
readers report `EAB1 committed root mismatch` and
`restore_economic_baseline_witness_mismatch` respectively for the equipped cuts.
Original witnesses, intents, plans, projections, reservations, claims and receipts
retain identical SQL values across each full dump/cold import. All reader calls
roll back/close, all SELECT-role UPDATE attempts fail with1142, and inventories
stay unchanged by auditing or dumping. This is native-authored capsule retention
and reader refusal evidence, not native SQL transaction execution or service
restart/gameplay qualification.

The harness starts fresh private Unix-socket MariaDB/MySQL daemons with TCP
disabled, installs authoritative bootstrap and all immutable migrations through
0056, and uses the native carried/equipped owner1/owner12 bytes. Writes are
performed only by the disposable fixture owner. Reader accounts have SELECT
only. The source witness, command hash and frozen intent are identical between
the controlled carried/equipped cases; the original plan and its digest change.
Each equipped SQL dump is imported into another newly initialized same-engine
daemon. The entire selected retained namespace is compared byte-for-byte at the
SQL-value level before independent origin/canonical audit. Audit/dump operations
must leave the source inventory unchanged; every reader call must roll back and
close, and reader UPDATE attempts must fail with1142.

Both commands run in immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with `--network none`, read-only `/workspace` and writable `/workspace/bin`.
Private database execution also uses `--cap-add SYS_ADMIN` and unconfined
seccomp/AppArmor, as required by the existing isolated database helper. The
native probes do not require those privileges. No live `.env`, existing SQL
database or game server is used.

| Executed independent reader | SHA-256 |
| --- | --- |
| `scripts/economic_restore_evidence.py` | `8dc9aa2e4c1f7a08178854b3410f054a31c696a94765a9b251526d0b13ef4613` |
| `scripts/economic_sql_audit_origins.py` | `9d1a5f03950c61b8794eb066c1456fd0ce1d434b1d05cbc4929f9baf0844d620` |
| `scripts/economic_sql_canonical_audit.py` | `765b0d81cfcfba5ae0e5a41e88a672625f178bfeeb1749396e41946ec29dfd2a` |

The initial generated probe's mixed signed/unsigned initializer failed strict
compilation; its exact source/runner and compiler output are retained. The first
SQL fixture set the baseline control's terminal operation before inserting its
inbox row and correctly hit the schema foreign key; the corrected fixture sets
the terminal after seeding the original root. A second fixture invocation tried
the origin CLI without its required output/file interface and failed argument
parsing. Final origin coverage uses its API: its current CLI has no Unix-socket
option for these TCP-disabled daemons. Canonical CLI socket coverage is retained.
No schema checks, native warnings, reader refusals or audit requirements are
disabled to repair these diagnostic recipes.

## Narrow request to the primary

The primary owns the versioned native/schema repair. The smallest candidate is
to retain the complete original equipment position in the baseline witness and
bind it before admission. One concrete wire proposal for review is:

| Field | Proposed exact bytes/invariant |
| --- | --- |
| Header version/magic | Version2/EAB2; retain a192-byte header and existing112-byte holdings if no other field needs change |
| Item UID | Existing uint64 little-endian at item offset0 |
| Native position | Full56-byte EAP1 position at item offset8: owner/state+6 reserved zero bytes, five uint64 fields, uint16 equipment slot,6 reserved zero bytes |
| Source fingerprint | Existing32-byte source digest at item offset64 |
| Item record/total |96 bytes; `192 + 112*holdings + 96*items`, maximum920144 with existing3071/6000 bounds |
| Baseline domain/intent | Versioned digest grammar that binds the retained equipment field and format; equal command/operation identity cannot admit changed equipment effects |
| EBC/CCM/EAI/EAP binding | Hash the complete original versioned witness; decoded/replayed position and regenerated original EAP1 must be exact |

This is a proposal, not an implemented or approved shared schema. If the primary
chooses another compatible representation, it still must retain the exact
original uint16 slot, validate reserved bytes/bounds/topology and distinguish a
changed request under the same original identity. Retain historical EAB1 bytes,
digests, receipts and their current refusal decisions. Never reseal or backfill
v1 evidence to manufacture a slot. Carried v1 compatibility is separate from
proof that an absent field represents original physical equipment. Coordinate
this version choice with the primary's admission-time/command retention work;
no speculative0057/0058 layout or migration sequence is consumed here.

Consumers and required checks are specific:

- Native baseline adapter/header/codec and command builder/rehydrator; SQL
  baseline transaction, flatfile baseline/lifecycle transaction and staging
  witness consumers. Original source snapshots, equipment, identities and
  admission metadata must survive encode/decode/command replay without inference.
- A new additive, guarded immutable migration, runtime schema/compatibility
  manifests and backup/retention/restore evidence bounds. Keep immutable0032 and
  existing canonical histories unchanged; its current CHECK admits only v1 and
  the88-byte item formula. Fresh and supported upgrade histories must agree.
- Plan5 independent EAB/EAP decoder, origin exporter, database restore verifier,
  flatfile restore readers and operator tooling. Supply the exact versioned
  field grammar and digest/command invariants in the handoff; independent
  readers will implement it without importing native mutation code.
- Native SQL/flatfile equipment slots0,1,43 for player/mobile, mobile44 refusal,
  historical v1/player grammar, reserved bytes and equipped topology; two
  preparations differing only in slot must have authenticated different
  witness/binding and same-ID changed requests must refuse. Decode and original
  command rehydration must reproduce original EAP1 exactly.
- Both private SQL engines must preserve complete original witness/intent/plan/
  receipt through full dumps and cold imports; flatfile journal, baseline and
  lifecycle replay/restore must do the same. Missing, mismatching or fabricated
  equipment evidence must refuse without mutation. Preserve original UID/native
  owner identity and personal-alias exclusion. Include failed/lost-reply replay,
  active-custody boundary and full source-capture requirements in combined
  qualification; this isolated probe does not supply them.

## Evidence, curator handoff and remaining gates

Evidence namespaces are `bin/tests/p5-eab-slot-20261005` (initial compiler
attempt) and `bin/tests/p5-eab-slot-20261005-final` (native bytes, builds,
findings and every SQL attempt). `tmp/plan5/baseline-equipment-evidence.json`
seals all new artifacts and exact executed inputs, including failed recipe
preimages; it inherits the previous sealed index without claiming another full
old-archive rehash. `baseline-equipment-delivery.json` records the result SHA,
committed report blob and verified requested remote branch. Protected evidence
is retained locally and not committed as logs, dumps, generated sources or data.

This report is the curator handoff for the primary's locally maintained shared
notebook; notebook locality is not a blocker. It records the established native
gap, conservative independent refusal, narrow interface request and exact proof
limits. There is no producer/writer coverage promotion or matrix change.

Complete equipped baseline/recovery remains a genuine shared-format dependency.
Plan5 independent work can continue while it is repaired. Complete admission/
command authentication, native census and authoritative source capture, real
writer/producer journeys, both maintained server builds, gameplay/fault/restart/
lost-reply, protected restore/retention/service journeys and release-host growth/
operation budgets remain. Earlier shared SHOP/build/contract findings are not
newly executed or waived here. No full server build or native repository mutation
journey is claimed by these component probes and SQL fixture writes. Accounting
stays inactive; wallet-root exclusions and the declined inactive spell change
remain. No production mutation, auto-correction, activation, deployment, PR merge
or independent experimental-accounting push occurs.
