# Historical quarantine reconciliation for #664

Reviewed on 2026-10-03 against accounting commit
`9d0ea2a1dd49cdb3fa132e5d38035c931a080b14`.

**Result: 25 observed staging cases remain held; none is established as eligible
for the delivered grant/save recovery transition.** The protected register now
records every observed case, its original frame identities, capture boundary,
native evidence references and specific refusal reasons. This is case
reconciliation evidence for [#664](https://github.com/Community-Duris/Duris/issues/664),
not historical release or accounting implementation completion.

## Evidence boundaries

Read-only SSH inspection indexed seven production and 49 staging backup
manifests. One complete generation from each environment was copied to an
owner-only, Git-ignored evidence directory. Every copied file matched its
manifest size and SHA-256, and each source manifest was unchanged before and
after capture. The production generation was captured on October 1 UTC and the
staging generation on September 30 UTC. Exact identities, source paths,
generation IDs, payloads, UIDs, credentials and per-case references remain
private.

Both SQL dumps were restored under distinct database names into a disposable,
network-isolated MariaDB 10.11.14 instance. No migration or recovery operation
was run on either live environment. These restores supplied native historical
reads; they are not current-schema, full-world or rollout qualification.

The selected staging generation has 22 archived PIDs and 3,862 frames. Its
active player and critical-command journals are empty, but its archive and PID
policy preserve unresolved evidence. The selected production generation has
active journal evidence and no quarantine archive or PID policy. That finding
is limited to the reviewed generation and runtime directory; it does not
establish that production has no other ownership or recovery cases.

A later read-only staging observation found 25 archived PIDs and 3,865 frames.
The live archive contains every original captured frame hash plus three new
runtime-terminal frames for three additional PIDs. Its archive, player journal
and policy copies matched stable source hashes before and after transfer.
Separate native SQL observations of those three cases used one read-only
consistent transaction. **These file and SQL observations are not a coherent,
stopped restore generation.** They are registered as additional holds and are
never combined with the older SQL restore to prepare a replacement.

## Recorded outcomes

The register assigns a stable private case ID from the first retained frame
identity, preserves every archive entry reference, and records an unresolved
`hold` for all 25 cases. Categories overlap.

| Evidence or refusal | Cases / frames | Consequence |
| --- | ---: | --- |
| Captured explicit PID policy and explicit-policy archive reason | 22 cases / 3,862 frames | The current owner refuses inspection for recovery; these are outside its runtime-terminal-only transition. |
| Native player absent from the selected SQL capture | 4 cases | No complete native player projection is proved. Do not recreate a player from archived snapshots. |
| Retained death obligation | 2 cases / 5 frames | The first recovery slice excludes death obligations. |
| Repeated archived revision | 1 case | A tied generation cannot be silently selected. |
| Archived revision at or below later native save revision | 1 case | A higher native revision does not retire retained component obligations. |
| Additional runtime-terminal cases after the selected capture | 3 cases / 3 frames | Native inspect can decode them, but a matching coherent restore and required original command proof are not established. |
| No original creation command for scoped native creation history in reviewed journals | 24 cases | Native hashes/results and mutable current rows cannot reconstruct the missing original envelope and frozen payload. |
| Established replacement eligibility | 0 cases | No replacement was prepared, applied or resolved; no historical release occurred. |

The absence-of-command finding is scoped to the reviewed retained journals. It
is not proof that no independent original bytes exist elsewhere, and it does
not identify a particular creation operation as the cause of an archived
failure. Explicit-policy archive reasons do not themselves establish the
original grant/save ordering diagnosis.

Thirteen staging generations contain critical-command bytes, with two distinct
journal payloads; two production generations contain bytes, also with two
distinct payloads. All distinct retained journals were copied, checksum-verified
and decoded with the native command codec. Older staging command/capture source
manifests were rechecked unchanged. The original 4,810-frame split capture was
also recovered from a manifest-verified retained journal and decoded natively.
The original private alias index and reconciliation report were not inferred or
manufactured, so the historical four-input copied-capture test was not claimed
as rerun.

One retained creation command for a captured policy case has a successful native
receipt and 61 exact retained UIDs. The native
`critical_command_repository_verify_creation_in_transaction` accepted its
original command, receipt, source and UID history on the isolated clone. The
native `player_quarantine_recovery_prepare_sql` still refused the case with
`missing or unsupported archive/command evidence`, because its independent
policy/archive boundary is unsupported. This positive grant proof supplies no
permission to remove that boundary or discard its other archived components.

Native historical reads retain the 420 scoped creation operations, their inbox
references, and the later item history. The complete relevant UID set has 3,169
identities, 21,526 retained ledger rows and 3,169 current custody rows. Later
read-only observations record 22 scoped creation operations for the three newer
cases, without claiming original command retention or restore coherence.
The complete SQL dumps remain protected inputs; missing native state and
unsupported receipt obligations are not replaced with inferred state.

## Native verification

Private listener-free harnesses were compiled against the existing production
codecs and recovery owners, reusing the maintained SQL harness source recipe.
They introduce no repository API, format, schema or recovery override.

- Every captured and observed frame passed native snapshot decoding and journal
  header/CRC validation. Every archive entry matched its retained SHA-256.
- Two native initializations of the captured archive refused all 22 policy cases
  with `replay_blocked`; every PID remained fenced.
- Two native initializations of the later archive gave the same policy refusals
  and successful **inspection only** for the three runtime-terminal cases.
  Every PID remained fenced, including those three.
- The native original-grant verifier passed, while preparation remained refused.
  Copied archive, journal and policy bytes were unchanged. The exact scoped SQL
  readback was also unchanged after verification and preparation refusal.
- The maintained regressions below passed on the reviewed source. The journal
  quarantine test used its argument-free synthetic capture, not the historical
  four-input fixture.

```sh
python3 tests/async/test_player_quarantine_recovery.py
python3 tests/async/test_player_quarantine_restore.py
python3 tests/async/test_player_save_journal_quarantine.py
```

The private evidence packet retains `case-register.private.json`, input/source
manifests, raw copied bytes, native decoded metadata, SQL reads, harness source,
commands and logs. The aggregate register was checked for unique case IDs,
complete observed PID/frame coverage and an explicit hold on every case. No
private identity, original command or recovery payload is published here.

## Remaining #664 acceptance

Keep #664 open. The 22 captured cases need reviewed dispositions for their
explicit policy/archive boundaries, missing native players, unsupported death
obligations and incomplete original command history. An independently verified
grant cannot make those case-level obligations disappear.

The three newer cases additionally need a coherent isolated capture and exact
original command evidence before a prepare/resume rehearsal can be admitted.
Inspection success is not replacement eligibility. If original evidence cannot
be supplied, preserve the hold; any restitution requires its separate reviewed
operator action.

For any subsequently eligible case, use the delivered stopped recovery sequence
and verify native replacement commit/readback, durable resolution, later ordinary
saves and cold restart against that case's own generation. Production release
still requires its backup, repair/opening procedure and owner approval. None of
those operations or outcomes is established by this report.
