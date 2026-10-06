# Plan 5 nonempty pending-claim native retention qualification

The original inactive SQL retention driver seeded zero-effect accounting roots
but no pending-claim sources or partial allocations. This slice extends that
same driver to retain a nonempty book across actual native account/character
erasure, cold restart and safe name reuse. The SELECT-only auditor must refuse
a lost partial allocation after the native erasure boundary. This closes a test
coverage omission; it does not establish a defect in native deletion behavior.

## Source, ownership and remote branch

Work stays on local/remote `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base is `f99fb470cec4dc346ed661ceac3bf3d5836eb20b`; separate solved-issue result is
`0a76ce875f4b5b67a16c963f262b553fda0db0d9`. Primary refresh observes
`9817f58b57a4a4179c4db5506fce3ccfe166c12b`. Base already includes its exact published source and
canonical 0062, all 62 migration receipts. Native tree is
`cf8dc0761057de7087a347b8e8c21285968ef4f9`; migration tree is
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.

The only owned implementation change is
`tests/async/run_plan5_retention_journeys.py`. No shared producer, coordinator,
contract, registry/matrix, migration or activation file is independently
changed. No new shared interface request is needed. The existing
[original opening-selector/PID handoff](PLAN5_CLAIM_ORIGIN_SELECTOR_HANDOFF_2026-10-06.md)
remains under primary ownership. This report, the owned remote follow-up and
sealed publication receipt form the curator packet for the primary's locally
maintained shared notebook, which the user declared nonblocking.

## Established gap and exact extended check

The red retaining observer executes the exact base driver and original native
menu creation. It requires two pending-claim sources and one allocation at the
first independent read. The actual original book contains 0/0; the cleanup hook
produces a second 0/0 observation. Red02 exits 0 only after verifying this precise
expected coverage failure and both counts, taking 85.370687s. The early abort
does not prove native erasure; its cleanup marker is not an erasure result.

The extended native fixture retains its original PID-only interface and emits
the original two capsules byte for byte. Its second real caller uses `PID claims`
to encode three additional real EAI1/EAP1 roots with the original codec. Both
ASan/UBSan modes agree. Twelve comparisons against the original red02 binaries
cover both modes and all six real native PID creations. Original PID 1/PID 2
capsules remain 1040 bytes, SHA256 respectively
`1b6238c4976e2211e3dee2ef51caafc4f84511c48032751a78cf3b851862e1c6` and
`904fab85143a5be433cb6fd16be5cfcdf4b6dd7202c66b85e1c9146434bcb214`.

Each actual native PID gets committed credit roots of 5 and 3 copper and a spend
of 2 copper, two source rows and one partial allocation, leaving a modeled claim
of 6 copper at revision 3. Native capsule actor and source-event subject must both
equal that PID. Capsule SHA256 for PID 1/PID 2 is respectively
`e52d11421f2dbd6bfec5206c5b0008a079ca51b244a249657d82ee391eb8b3d5` and
`685d4c9d687412f374a5879e8a0ac54d3856d4d87d2ac98ea4b168b1591e70b0`.
Allocation/mapping/native pickup rows are disposable modeled projections; the
wallet is not an authenticated live opening or a real producer. Each PID has
five roots/four source-event claims in total, including the original committed
and rejected zero-effect roots.

The original native menu scripts, cases, reconnects, failure controls, server
shutdown checks and deadlines stay unchanged. The character journey first
performs whole-account erasure, then safely reuses names with PID 2 before real
character deletion. Both original aliases/player state and accounting rows are
checked at their actual boundaries. Ten retained collections now include roots,
source-event claims, inboxes, effects/postings, mapping and source/allocation
book; exact earlier rows must survive, including when new identity rows append.

The current inactive paths have distinct native pickup outcomes, which are
asserted exactly: whole-account cleanup removes the old PID's pickup; character
deletion retains `pid=2`, `money=6`, `claim_revision=3`. `sql_delete_account` explicitly deletes
the pickup; `sql_delete_player` deletes `player_data`. The existing lifecycle
manifest classifies pickups as non-subject-scoped world/gameplay state with
terminal retention. This test preserves those paths; it does not change their
policy or qualify active erasure semantics.

The independent reader uses a dedicated SELECT-only role, whose UPDATE must
fail with 1142. Each cut uses repeatable read and a read-only consistent snapshot,
authenticates canonical evidence/allocations and rolls back/closes once. The
reconciler and bounded operation views retain the global incomplete/unfenced
exceptions and two unknown openings per PID. No complete-capture waiver is made.

Only after native player/account outcomes and cold restarts are asserted does
the private fixture owner introduce the original actor fault and the new missing
partial allocation. The reader must refuse with
`restore_economic_metadata_mismatch` and
`restore_economic_pending_claim_consumption_mismatch`, respectively, without
changing damaged rows or increasing successful capture counts. The private owner
restores its own injected fixture fault before the final equality cut; no audit
finding or real balance is corrected.

## Frozen commands and terminal results

Pinned image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
GCC 13.3.0/Python 3.12.3, MariaDB 10.11.14 and MySQL 8.0.46. Containers have two CPUs/
4GiB, no external network or host ports, a read-only root, and separate 2GiB
workspace/tmp RAM filesystems. SQL engines and native game ports are restricted
to the private container loopback; original Unix admin sockets/private datadirs
are retained. Only committed public source and protected fixture evidence are
mounted; no environment file or production database is used. Native cache is off.

Green03 freezes 6,351 regular committed inputs/four public symlinks, overlaying
only the owned driver. Archive SHA256:
`6b30eaedda918d129d68bd8191eac3da7fe5d419bdb2af23502a238cdf743ac4`. Owned driver SHA256:
`3f4c40f72f2e1b686d48c0c1f34b545ceabec514354caf353403331b2875b73b`.
Every committed result byte equals that freeze; source bytes are verified before
and after execution.

The maintained SQL server is the previously qualified build with 738 fresh objects,
SHA256 `51f3eef4cd9db98f723bd1a090071936e78185d8f07b13a70974b1ee24fad016`.
Its exact native/migration/provider/public-runtime inputs equal this candidate.
The original production build command was `make -C src -j2 BUILD_PROFILE=production
PERSISTENCE_BACKEND=mariadb BIN_ROOT=/evidence/build/bin
OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server`, with 0 reused
objects/warnings/errors and original 600s deadline. Its prior
[managed qualification](PLAN5_NONEMPTY_CLAIM_MANAGED_RESTORE_QUALIFICATION_2026-10-06.md)
and protected seal remain the build attribution; this test-only change does not
claim a new maintained build. The native fixture is rebuilt here in both modes
with the original eleven providers, C++20/Werror/O1/debug/ASan/UBSan/fno-pie/no-pie/
libcrypto recipe; both binaries have SHA256
`d11d09b093cf29c9bf7a415865d85380e90f198d77aabf8a10e1d519b64f94b7`.
Exact command arrays are retained in `native-commands.json`.

| Invocation | Terminal result and scope |
| --- | --- |
| `python -B tmp/plan5/launch-claim-retention-red-2.py`; frozen `python3 -u -B /evidence/observer.py red` | PASS expected coverage omission, both original cuts 0/0; no erasure claim |
| `python -B tmp/plan5/launch-claim-retention-green-3.py`; frozen `python3 -u -B /evidence/observer.py green` | PASS original four account/character SQL journeys, both engines; 426.192516s process; 7200s outer deadline retained |
| Selected driver CLI in that observer: `run_plan5_retention_journeys.py --server /workspace/bin/server/dms_new --backend sql`, with `DURIS_RUN_PLAN5_RETENTION_INTEGRATION=1`, `DURIS_REGRESSION_BUILD_CACHE=off`, `DURIS_TEST_SANITIZERS=1` | 22 successful read-only captures; 10 cold restarts; 6 actual PID creations; 4 actor and 4 lost-allocation refusals after asserted native erasure |
| Host syntax parse of the owned driver and `git diff --check` | PASS |

Each engine ends with 5 roots/4 source-event claims/2 pending sources/1 allocation
in the account journey; the character/name-reuse journey ends with 10/8/4/2.
Account captures are 4 and character captures 7 per engine. All original native
refusal/reconnect/retry and cold-account-login controls pass. Selected checks
have 0 skips. This run exercises SQL retention in both engines and both codec
modes; it does not exercise a nonempty flatfile retention journey.

## Protected evidence, failures and remaining gates

Protected root is `D:/CodexEvidence/accounting-plan5/bin/`. The final seal
`claim-retention-final-seal-03-20261006/evidence.json`, SHA256
`251b8ba009dfe513016c09fd26c9d8ab51acda53d7d3dda38b5f83a7c09b5d91`, binds base/result, all 6,351 result inputs, mode/link identities,
build equivalence, original capsule comparisons, terminal commands/results,
all failed attempts and 3757 regular artifacts/4,557,433,394 bytes
plus 18 original Linux link targets recorded without traversal.
Green03 process log SHA256 is `4b617e82c27f5105d75a10ec1bf35c7982c703c6fcdb6d4c999f0f49af9b9b9e`. Its folder retains
native probes/raw capsules, executed preparation/load/observer helpers, process
receipt, original default comparisons and private runtime/datadir copies.
Publication receipt separately binds the remote documentation tip, clean state,
unchanged qualified code and seven preserved earlier branch tips.

Red01 and green01/02 remain failed and unqualified. Red01 incorrectly required
only one original empty observation; red02 verifies both cleanup/read counts.
Green01 used an incorrect fixture posting column, then corrected canonical
`line_index`/`copper_value`. Green02 passed MariaDB account deletion and completed
the original character journey but failed the added, incorrect expectation that
inactive character deletion removes native pickups. Source/manifest inspection
corrected only that test assertion; green03 requires the exact retained row and
repeats the whole original four-journey scope. Partial markers do not qualify
their aborted processes. Seal01's Windows collector also fails when following
a retained Linux runtime link. Its failed helper/source archive are preserved;
the original seal02 collector inventories those regular artifacts and link targets
through a separate read-only Linux mount. Windows observation times out at 60s;
the same container completes 0 at 68.927774s. Its original logs/identity/read-only
mounts are verified and adopted without restarting into seal03. An initial
adoption assertion looked for desktop CLI labels in Config.Labels; corrected
validation uses authoritative Mounts. Both failed helpers are preserved.
No qualified test input or behavior changes.

There is no independent blocker for this completed scope and no new shared
interface request. Authenticated original opening selector/PID reference,
complete R6 native/live-world capture and borrowed-connection activation
verification, actual financial producer/ACK/lost-reply/cold recovery, typed active
erasure, full managed flatfile/nonempty retention and release-host mixed-workload
latency/storage/checkpoint/reconciliation budgets remain open. `AI_CONTEXT.md`
is absent in the inspected checkouts; that unavailable documentation read is not
a test skip or a slice blocker. Primary integrates this same remote branch and
owns publication/qualification of its final combined candidate.

Accounting remains inactive; wallet-root item exclusions and the declined
inactive spell-path change remain. There is no production mutation, accounting
activation, deployment, PR merge or automatic correction. This is component
retention evidence, not full Plan 5/R8 or release completion.
