# Complete Item fixture passes; registered outcome remains rejected

On published primary `7f1d766c068501427c099360bb4d1985240395a4`, the original
complete bounded Item entry passes its SQL-header and flat fixtures. Each mode
reports 15 successful groups, 2,653 refusal points and 2,653 reservations, including
the lockpick zero-destination-root, malformed-binding and NQF1 markers. The
unchanged original integration outcome checker rejects the actual receipt with
`missing required cases: main`. Both required cases remain in the registration.

An independent source hash check also finds a stale integration-manifest pin in
both writer artifacts. The portable two-field repair corrects that hash without
changing any registered case, marker, timeout, source body or policy. The original
default validator passes despite the mismatch; the candidate validator also
passes. This is a hash-accuracy repair, with no validator red-to-green claim.

## Exact source and original entry

Coordinator job `ACC-08-ITEM-BOUNDED-01`, owner chat
`01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`:

- Primary commit `7f1d766c068501427c099360bb4d1985240395a4`, tree
  `2ec2ee21a1be222719f4969ae5909f4677690b2f`.
- Previous primary `97ded8571375f3fdc6168487a13120f65caedbec`; the three new
  commits publish the wire correction, complete Item providers and NPC source join.
- Frozen inputs: 3,983 regular Git blobs, including migrations needed for the
  declared source-pin audit. All remain unchanged after execution.
- Complete changed 20-file freeze SHA256
  `592ddbc98d317a1171a94019fe50ef791176eab39dc5745985e73d44c3ecc797`.
- Source inventory SHA256
  `81a2120903ba4e34f789a7428b5ac347c8546ec746d41e4ff5080b5c6cb87bb3`;
  source archive SHA256
  `9451faf74c294577fec3c168a0f5d3dbed69c882d413238bf3d99030ad5355ae`.
- Image `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`:
  GNU 13.3.0, Python 3.12.3, OpenSSL 3.0.13. Original production Make object
  recipes compile 15 translation units per mode; the original harness links
  with its strict flags. No source, assertion, library substitution or entry
  argument was changed. Only the registered ordinary lane was executed here.

Actual command:

```text
python3 -B /opt/source/tests/run_test_entry.py /opt/source/tests/async/test_item_transfer_complete_bounded.py script /evidence/entry-cases.json 1
```

Start `2026-10-11T15:49:11.802638Z`; original entry exit **0** after
**129.014212634 seconds**, empty stderr, zero skips. Both original ordinary mode
markers and their structured reports were captured. The temporary-directory
manager removed the executables; there is no retained-ELF or binary-hash claim.

The bounded WORLD review is published at
`5e679b5fe0720f0506fa15a1eedd06cbf6e23fab`, sole standalone report
`WORLD_ITEM_BOUNDED_GATE_REVIEW_2026-10-11.md`, imported byte-for-byte:
blob `fe52d54e242f2dbf72aeeb0595ccc8bae6823aff`, SHA256
`6c12f883d7bc0fa23044cb57ea1207e610e51d7f93d159a2d87a280951dc590a`.
Its independent review receipt SHA256 is
`95d632998dbd7223a546096863a90c793b137afa360f7bcc05e8800faa7ac829`.
It verified all 4,022 sealed members, 3,983 selected Git blobs/modes/bytes/archive
and the exact two-field candidate, using reads/hashes/Git only. It took no fresh
resource inventory, reran no executable/checker/validator and returned idle.

## Original registered outcome refusal

The registered row `item_transfer_complete_bounded` retains `mode=script`,
`required_cases=[script,main]`, the 5,400-second timeout and all three markers.
The genuine original receipt contains one passed `script` case,
`called=[main]`, `collected=[]`, and `complete=true`.

The unchanged `tests/run_integration_matrix.py::outcome_contract`, using the
unchanged `tests/run_regression_tests.py::TestResult` and the actual original
receipt, returns exactly `missing required cases: main`. The outer qualification
process exits **1** for this acceptance refusal, with no transport/collector error.
The full integration matrix was not invoked. This reproduces the same case
observation boundary as ACC-07 on a different, newly registered native fixture;
ACC-07 remains proof on its original 97ded857 source and was not rerun.

Primary must publish genuine `main` invocation/outcome evidence under the
existing receipt contract, keeping both required cases, assertions, markers,
modes, deadlines and genuine rejection controls. The withdrawn field-removal
proposal remains diagnostic history only. Called-main observation alone does
not establish a passed required `main` case.

## Narrow manifest-pin repair for primary adoption

All ten refreshed/new pins match the published Git bytes; 486 previous pin
values remain unchanged, for 496 declared pins. All 931 writer policy values
outside lexical `sites` are preserved, and the 2,911 census occurrences remain.
Existing integration rows are unchanged; the new Item row is additive.

One preserved pin became stale when the new integration row was added:

| Input | Declared in both artifacts | Actual published bytes |
| --- | --- | --- |
| `tests/integration_manifest.json` | `a1c78d916a646baf5e668365a877dc5bd83f5f9bec205237c4dfd9a63cc68741` | `268f2921125a4b9b47607c28ed12dcf42ad8c4e29e08d6a29fbf1640946b4dc2` |

The old declared hash matches the previous 97ded857 manifest. The patch
`patches/ITEM_BOUNDED_MANIFEST_PIN_REFRESH_2026-10-11.patch` changes only
`candidate_worktree_evidence.source_pins.tests/integration_manifest.json` in
`writers.json` and `writer_coverage_matrix.json`, on exact base 7f1d766c0.
Patch SHA256 `7e5cb59e22224765c572d484ffe12b8993d9ccd797c5703a439f4e392df2eb3c`.

Apply-check on the untouched freeze and reverse-check on the candidate both
exit 0. The derived candidate preserves all other 3,981 files and all other JSON
values; every one of its 496 declared hashes matches its exact input bytes.
Original Linux default validator: exit 0 / 13.802406640 seconds.
Candidate Windows default validator: exit 0 / 6.625 seconds.
Both report 14 fixtures, 931 writer routes, 2,911 candidate sites and
`release_ready=False`. No release audit or native fixture was repeated for the
metadata candidate. Shared runtime, interface, migration, registry and final
integration ownership remain with primary; this patch is unadopted.

## Evidence, failures and resource closure

Native evidence:
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-08-ITEM-BOUNDED-01`.

| Receipt | SHA256 |
| --- | --- |
| `native/registered-item-entry.result.json` | `1c8309645886871cd062280e11c25ae20753f7b05fc42236a2375a2bd0faf78c` |
| `native/entry-cases.json` | `5840204b3c34125d22fa80433be99a463ea5d5417be4fc8d1f07358bf8f2bd75` |
| `native/original-outcome-contract.json` | `7c74a632b885f8f707ae9624dbfeef3e1f12e66eabb97172a00e4b70faf12c30` |
| `native/source-contracts.result.json` | `2f667cfdd85b0180db287f131c7778ec46c361f1fddb92bb1f4485642be0e3bf` |
| `evidence-seal.json` | `eac76e0f3e36236942e608a08dd1d85a1f79e759eeb68d344dd96a8ae5103028` |
| `resource-release.json` | `b0416b5ad274818e9e26b37691df01d9ee22b312d67831938ab24c2315deebde` |

The native seal contains 4,022 members / 147,390,631 bytes. Two preparation
failures remain preserved: a read-only inspection printer sliced a mapping,
then the exact all-pin assertion detected the stale manifest hash. Neither had
started native work or obtained a resource grant. Completion verified the
existing source archive against Git without rewriting it; the native job ran once.

Separate metadata review:
`D:/Dev/Tests/Duris/accounting-accel-integration/ACC-08-ITEM-BOUNDED-01-metadata-review`.
Result SHA256 `bcc3ea353c1248cd341d9b1d12e9728e5c886b7dc4ba5d8a1f060b424023c49a`;
seal SHA256 `3049ef8a1c26945fdd92a9dcdcf9c3f4c3705c2794c750d5603b87b01e6a598f`
contains 3,995 members / 71,600,731 bytes.

The sole QA used CPU1 / 2GiB, restart=no, no network, exact pinned image,
read-only original source and helper inputs, and six direct task-specific D:
binds. Its 64MiB fixture stack ceiling implements the original runner's stated
requirement and establishes no 32MiB whole-host accounting bound. Exact QA
`6f74aca168ca02ec39684c153e6ba4f1b2ad9a2c0e8113b834c80964e8d4f194` was removed;
the matching heavy lease was released. Seven unrelated service IDs and four
passive diagnostic volumes were preserved, with no passive-volume consumers.

Fresh coordinator inventory at `2026-10-11T16:02:15.394884Z` confirms exact QA
absence, seven preserved service IDs, zero owned jobs and no heavy lock; receipt
`delivery/fresh-owned-absence.json`, SHA256
`19f0fc23b9d2beca310450a1485c309f15b85660087dc323d038d81c8ef80f31`.
Its earlier case-sensitive Docker-error printer assertion is preserved separately
in `delivery/fresh-inventory-failure-01.json`; only the receipt printer changed,
with no qualification rerun or resource operation.

This qualifies the bounded Item value/refusal component on the exact published
source and the proposed hash correction. Primary's reported combined full builds
and private Source9 results are not independently promoted here. NPC gameplay,
shared command/publication protocol, runtime ABI correspondence, real SQL/journal
and both-engine durability, whole-host 32MiB peak, cold restore/shutdown,
gameplay/recovery/workload, Plans 1-5/R1-R8 and release remain open. Accounting
stays inactive, admission CLOSED, release BLOCKED. No implementation-speedup
percentage or shorter completion estimate is inferred.

## Later primary publication observed without a qualification repeat

Primary advanced to `4dba71a03bdbd54e366b19fc1dec5c9925f3ebf0`, tree
`bc62157fd8c1d58cd2ce13663987ebeab322d160`, during this packet's publication.
Its parent `9d2bdd74cbee176a1087fe3f15298c65f8e04ca5` adopts the exact previously
reviewed three-provider retention recipe: the complete module matches the helper
candidate bytes, SHA256
`3dc38861cdb14f6aae61f586a0943c38962fa9a17c93b40615480b106a042878`.
Primary adds its source pin, preserving the previous 496 values, for 497 pins.
Primary's new reported retention execution on 7f1d766c0 remains a published
report here; its raw private packet was not independently reviewed. ACC-06
retains its original 055+3 source limits, and full retention remains unqualified.

The complete `src/` subtree, Item fixture/runner, Makefile, case adapter/checker
and integration manifest remain byte-identical to this ACC-08 freeze. Neither
native gate nor validator was repeated for the later documentation/retention
batch. The manifest hash remains stale in both current metadata files; the
two-field primary-adoption handoff remains relevant.

The later charter also claims renewed human authorization for WORLD3/SHOP5
runtime leases. This chat's latest human heartbeat explicitly retains the reduced
arrangement and withdraws those allocations. A published claim is not a direct
human instruction to this coordinator. Clarification is requested; both helpers
remain parked and no runtime lease or unqualified WORLD draft is activated or
imported. Shared runtime/final integration ownership stays with RGitGoin under
the currently controlling instruction.

Exact source-only observation:
`delivery/primary-observation-4dba71a03.json`, SHA256
`39cc1e7e82c76bc153a28bcbea65e1c02c837e8c6d3885bbbeadfd0086b22174`.
