# Plan 5 flatfile retained-source loss after native erasure

The original flatfile retention driver never removes a retained source-event
claim after actual native erasure and cold restart. This slice adds that check
to its three original journeys. The independent reader refuses the missing
claim and leaves the damaged evidence and native authority unchanged. This
closes a test coverage omission; no native deletion defect is established.

## Exact source and ownership

Work remains on local/remote `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base is `445b4caa9f755ae7283cc6a0de1aec5ac67ed22a`; separate solved-issue result is
`c00bf4dd1c215025799556575b243c2ce0fcd0a8`. Its only implementation change is
`tests/async/run_plan5_retention_journeys.py`, 65 insertions/four deletions in
the flatfile runner. The SQL runner and native codec prefix remain byte-identical.
Shared producers, coordinator, contracts, migrations, registry/matrix and
activation owner are not independently edited.

The tested base includes primary `9817f58b57a4a4179c4db5506fce3ccfe166c12b`.
Native tree is `cf8dc0761057de7087a347b8e8c21285968ef4f9`; canonical 0062
migration tree is `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, all 62 receipts.
The publication refresh observes newer primary
`de3296fd028261f650e86165f632690e14f98b4a`, native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b`. Its auction/opening changes are
outside this exact tested source. This result does not qualify that successor.

The [shared player-inspector provider handoff](PLAN5_SHARED_PLAYER_INSPECTOR_PROVIDER_HANDOFF_2026-10-06.md)
requests primary review of two missing real provider entries in its existing
`SOURCES` list. There is no schema, contract, API or wire-format request for the
retention extension. The handoff is a distinct unresolved shared build finding,
not a completed repair. This report, the owned remote follow-up, seal and
publication receipt are the curator packet for the primary's locally maintained
shared notebook, which the user declared nonblocking.

## Established omission and added check

Red02 runs the exact original deletion preparation and complete original
character/durable/uncertain retention scope. All three native journeys finish,
with 13 successful independent captures and seven cold restarts, but zero
missing-source refusal checks. Its observer exits 0 only after verifying that
precise expected coverage omission against the required three checks. It does
not claim the original reader accepts a damaged store.

The existing native fixture and providers remain unchanged. Each journey has
four retained roots, two committed source-backed nonzero modeled wallet-reward
credits across two epochs and two claimless rejected roots. Its two common
`source-claim-*.bin` files are native source-event deduplication evidence. They
are not the SQL pending-claim allocation format; no new flatfile format is
invented. Each intact inventory has 269 economic evidence files. These encoded
plans do not establish a real financial producer or an authenticated opening.

Only after the actual deleted PID 1 snapshot is absent, the latest native
server exits 0 and the original cold-restart count is exact does the new fault
check run. No pending native authority transaction may remain. The disposable
fixture owner upgrades the original authority lease to exclusive/nonblocking,
removes one original source claim, then downgrades to shared/nonblocking for
the same standalone independent reader. The reader must exit 1 with no stdout
and the exact existing generic diagnostic `native_restore_qualification_failed`.
No specialized diagnostic or mutation/recovery reader is substituted.

Hashes/modes/link counts for all remaining economic files and all regular
native authority files outside that directory must remain exact after refusal.
The owner restores only its injected fixture bytes with exclusive create and
no-follow under the exclusive lease, then returns to the shared lease. The
same reader must accept that restored valid control and leave both complete
inventories unchanged. This is private fixture cleanup, not audit correction.
All three fault markers bind operation `01000000000000000000000000000001`.

The original native menu cases, refusal/retry identities, aliases, reconnects,
journal boundaries, deadlines and server shutdown assertions remain. Positive
counts stay three/five/five captures and one/three/three cold restarts. Two
original capture boundaries genuinely contain a pending native journal.

## Commands, environment and terminal results

Pinned image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3.0/Python 3.12.3. Native fixtures use their original cache-disabled
providers, C++20/Werror/ASan/UBSan/non-PIE flags. The standalone auditor includes
only the independent `qualify_flatfile_economic_records.h` operator reader.
Exact compiler command arrays, flags and observed deadlines are retained in
`native-commands.json`. Existing recipes without an individual subprocess
timeout retain that behavior; the observer's original outer limit is 7200s.

The runner has two CPUs/4GiB, a read-only root, no external network or host
ports, and separate 2GiB workspace/tmp RAM filesystems. Native game ports use
private container loopback. Only public committed source and private fixture
evidence are mounted; no environment file or production data is used.

Green01 freezes 6,352 regular committed inputs/four public symlinks, overlaying
only the owned driver. Archive SHA256 is
`60c16f5066cf064184bfbc052be740a25dbdbcccbbf26da90134ae34cd8d3bec`;
driver SHA256 is
`29b7775691ac6da22b00130203e4289ae88ac34c7d8ebe13f18cba94e7d3be0b`.
Every result input/mode/link equals that freeze, verified before and after run.

The maintained flatfile server is reused read-only from the prior
[managed qualification](PLAN5_NONEMPTY_CLAIM_MANAGED_RESTORE_QUALIFICATION_2026-10-06.md).
Its exact native/migration/provider/public-runtime inputs match this candidate.
Original command is `make -C src -j2 BUILD_PROFILE=production
PERSISTENCE_BACKEND=flatfile BIN_ROOT=/evidence/build/bin
OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server`: 738 fresh
objects, zero reuse/warnings/errors, 449.423221s under the original 600s limit.
Server SHA256 is `e1aa4901d7f656b5292a4c626bc07ef047da4b6699c628855493fffde1de61db`.
No fresh maintained build is claimed for this Python-only extension.

| Invocation | Terminal result and exact scope |
| --- | --- |
| `python -B tmp/plan5/launch-flat-source-loss-retention-red-2.py`; frozen `python3 -u -B /evidence/observer.py red` | PASS expected omission after all three original native journeys; 422.303338s; zero source-loss checks |
| `python -B tmp/plan5/launch-flat-source-loss-retention-green-1.py`; frozen `python3 -u -B /evidence/observer.py green` | PASS complete extended three-journey scope, 430.367305s; terminal exit 0; source unchanged |
| Original `test_flatfile_character_delete.py` preparation inside both observers | PASS all eighteen native journal boundaries and fresh `seed-empty-deletion` fixture; original deletion inspector copied |
| Selected CLI `run_plan5_retention_journeys.py --server /workspace/bin/server/dms_new --backend flatfile --inspector /workspace/bin/tests/flatfile-character-delete-inspector` | 13 successful captures, seven cold restarts, three missing-source refusals and three restored valid controls; original character/durable/uncertain cases |
| Host syntax parse and `git diff --check` | PASS |

Environment explicitly enables `DURIS_RUN_PLAN5_RETENTION_INTEGRATION=1`,
`DURIS_REGRESSION_BUILD_CACHE=off`, `DURIS_TEST_SANITIZERS=1`. Red/green freshly
compiled native binaries are identical: inspector
`d9b13b46c2431f5b3e81e53f07ac66163017ae683285df962405edc22eb3945c`,
fixture `78a9f8b006f09e2eccb45c23f59b421d4fcb4b1bbc27003ba1a8cef59c629af0`,
auditor `b5a074f13318a2e1672a39d2c4b6d41ec3fb25f8f993c84759b05486c6334307`.
No SQL database is involved in this flatfile-only check. The unchanged SQL
retention result remains separately attributed to its preceding exact-source
report; it is not relabelled as execution on the newer primary source.

## Evidence, failed attempt and remaining gates

Protected root is `D:/CodexEvidence/accounting-plan5/bin/`. Final seal is
`flat-source-loss-retention-final-seal-01-20261006/evidence.json`, SHA256
`df3b089aa52ad7a2a76d3f66ad672891f9d918274fe899ba4d80bf064630cd33`.
It binds base/result, all 6,352 result inputs, original public modes/link targets,
terminal commands, exact original/extended markers and 3,623 regular evidence
artifacts/2,075,389,938 bytes plus twelve original Linux link targets without
traversal. The read-only named Linux collector completes 0 in 49.828s under
its 300s inventory limit, with no restart. Its deadline is evidence collection,
not a changed native qualification limit.

Green01's log SHA256 is
`c6391aaabb376c77746abecc44af6c51dcd9712ca16c50de48e2f3a7e654e512`;
red02's is `b1339722624b07ab5d1ac07f28a89ccb45690408fc1f924c040a0905ba2ca193`.
Folders retain original public transports, executed helpers, native binaries,
command arrays, process receipts and fixture-owned runtime copies. Publication
receipt separately verifies the remote documentation tip, clean worktree,
unchanged qualified code and ancestry of all seven earlier branch tips.

Red01 remains failed/unqualified. Its observer incorrectly selects the common
player inspector instead of the original deletion preparation. Before any
journey, the unchanged shared recipe also fails to link `economic_baseline_decode`.
The separate narrow handoff records that genuine provider finding. The correct
red02/green01 preparation runs the original deletion inspector, including all
eighteen journal controls, without changing the shared helper. Red01 process
and failed command/log are preserved; no partial marker qualifies that attempt.

Selected completed checks have zero skips and no independent slice blocker.
`AI_CONTEXT.md` remains absent in the inspected checkouts. The shared inspector
repair, original opening selector/PID reader integration, complete R6 native/
live-world capture and borrowed-session verification, genuine financial
producer/ACK/lost-reply/cold journeys, typed active erasure, complete managed
flatfile/pending-claim parity and release-host mixed native workload budgets
remain open. The primary's newer published opening component needs independent
inspection and candidate qualification before its evidence can close a gate.

Accounting stays inactive; wallet-root item exclusions and the declined inactive
spell path remain. No production mutation, activation, deployment, PR merge or
automatic audit correction occurs. These are seeded-history component checks;
full Plan 5/R8 and release are incomplete. Primary owns regular integration and
publication of the tested combined candidate.
