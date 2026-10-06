# Plan 5 nonempty pending-claim managed restore qualification

The managed SQL backup/restore tests previously qualified empty pending-claim
tables. This slice adds a nonempty retained claim book to both original full
managed methods and checks exact preservation before and after actual isolated
server boot. Missing or altered allocations in newly imported disposable
candidates must refuse before boot. This repairs a qualification coverage gap;
it does not assert that the backup manager previously lost claim data.

## Source, ownership and remote branch

Work stays on local/remote `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base is `4900b21effb754d6c50f5f64b0a63964626d4496`; solved-issue result is
`9683bb0d28744167357b7f179269bdb31d981251`.

The base normally merges primary
`9817f58b57a4a4179c4db5506fce3ccfe166c12b` with Plan 5
`e6aadf6ec58aef431f9d2b04bfc6b3c0589215ed`. Incoming primary source and its nine
changed paths were imported exactly. Two conflicts preserved the newer owned
audit guide and restore fixture driver from the Plan 5 parent; primary's driver
blob equals its earlier preimage, so no incoming implementation was dropped.
Import evidence is `tmp/plan5/claim-managed-primary-import.json`.

Native tree is `cf8dc0761057de7087a347b8e8c21285968ef4f9`; migration tree is
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical0062 with all62 receipts.
The primary's absent-price predicate and runner-provider repair are included.
Its original fault-suite outcomes remain attributed to its published
[qualification](BASELINE_NULL_PRICE_PRIMARY_QUALIFICATION_2026-10-06.md), whose
protected artifacts were not independently inspected here.

Owned implementation files are
`tests/async/test_persistence_backup_integration.py` and
`tests/async/test_restore_economic_coin_effects.py`. The latter exposes the
existing native fixture build recipe for its second real caller; providers,
flags, sanitizer settings and executable limits stay exact. This report and
the owned remote follow-up form the curator packet. No shared interface or
schema change is requested. The existing
[opening-selector/PID handoff](PLAN5_CLAIM_ORIGIN_SELECTOR_HANDOFF_2026-10-06.md)
remains open under primary ownership.

## Established gap and complete scoped check

The retaining observer runs each original base `sql_full_dump_restore` method
unchanged, extracted from its exact Git source, and requires two source rows
and one partial allocation at the original first qualifier. Both actual native
qualifiers accepted their fixtures, whose source/allocation counts were0/0.
Both observer assertions failed:2 selected methods,2 expected failures,0 errors
or skips,136.313540s. The red observer exits0 only after verifying those exact
failures, both zero counts and no boot/archive. This is an established test
coverage omission, not a backup corruption finding.

The extended original methods use actual EAI1/EAP1 capsules emitted by the
existing ASan/UBSan coin fixture in both SQL and flatfile modes. Their identical
five-root claim history SHA256 is
`2f0c5dc917aef019c60befbd592c9d0e0a1bf6b0c6bbef828996b9ad077bd635`.
The managed book selects three roots, sources of5 and3 copper and a partial
spend of2 copper, leaving6 copper at native pickup PID42/revision3. Allocation
rows are modeled fixture projections over those native capsules. The historical
lineage has a NULL active epoch; original opening/producer proof is not claimed.

The independent canonical reader authenticates retained root bytes/details and
allocations. The native snapshot reader and reconciler independently check the
source/debit census and residual. A dedicated SELECT-only account must refuse
UPDATE with error1142. Exact rows from eleven retained/native collections are
compared at the source, restored import, after real boot and after each failed
corruption cut. Original wallet/bank/epic/schema/receipt and isolation assertions
remain present.

Three real imports per engine are independently corrupted only in their new
private candidates: delete partial allocations, change the allocation amount,
or delete an original credit source. The independent reader must diagnose
`pending_claim`; the actual managed qualifier must then refuse. Service boot is
not called, `QUALIFIED.json` is absent and `FAILED.json` is checked. After source
and generation preservation checks, the stopped failed candidate is removed
with the original guarded cleanup routine, keeping the existing512MiB limit.
The fully verified successful candidate is also removed before the independent
cuts; each corruption must explicitly reach real import and audit refusal.
No balances or audit findings are repaired.

## Frozen inputs, commands and results

All runs use pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
actual GCC13.3.0/Python3.12.3, MariaDB10.11.14 and MySQL8.0.46. Each container has
two CPUs/4GiB, no network or host ports and fresh private RAM filesystems. Managed
namespace cases keep their original container-only capabilities/security
options. Databases use new datadirs and Unix sockets with TCP disabled. No live
checkout or local environment file is mounted. Native fixture cache is off.

The maintained build snapshot is full committed base source:6350 regular files
and four verified public symlinks. Archive SHA256 is
`fa276388a9f8f05b516e1a125a3bb66e5d1aadad2b111433e47ac154ea154a2c`.
Fresh commands are `make -C src -j2 BUILD_PROFILE=production
PERSISTENCE_BACKEND=mariadb BIN_ROOT=/evidence/build/bin
OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server` and the same
command with `PERSISTENCE_BACKEND=flatfile`. Each original600-second build passes
with738 fresh objects/dependency records,0 reused objects, warnings or errors.
SQL takes478.835453s, server SHA256
`51f3eef4cd9db98f723bd1a090071936e78185d8f07b13a70974b1ee24fad016`;
flatfile449.423221s, server
`e1aa4901d7f656b5292a4c626bc07ef047da4b6699c628855493fffde1de61db`.

The final managed snapshot changes only the two owned test files over that
base, archive SHA256
`2412cee47354e478ee67e6940313b35aa304ae679b777c1094843aa34954b1ef`.
The original native full-restore repeat used archive
`6fd708cb6ac6855a59afd68786a69004fb9ac335b3eb9c61dcdcdb3478a9a550`.
Its source differs from the final managed snapshot only in the managed backup
test's candidate cleanup/count assertions. The coin fixture helper/full native
method, all production/migration/provider inputs and original native controls
are byte-identical. Each6350-file frozen snapshot is verified before and after
execution; final committed files are compared individually to the final freeze.

| Selected command/workload | Result and actual scope |
| --- | --- |
| `python -B tmp/plan5/launch-claim-managed-red-1.py`; frozen `python3 -u -B /evidence/observer.py red` | Exact two expected nonempty-coverage failures on original methods; no skips |
| `python -B tmp/plan5/launch-claim-managed-green-4.py`; frozen `python3 -u -B /evidence/observer.py green`, selecting both original `PersistenceRecoveryIntegration` full-dump/schema-history/value/isolated-boot methods | PASS;2 full methods;0 failures/errors/skips;242.268s unittest,242.413978s process;2 real restored-server boots,2 full dumps,2 qualified receipts,6 corrupt-import refusals,14 equal source/restored claim cuts |
| `python -B tmp/plan5/launch-claim-managed-native-full-2.py`; `python3 -u -B tests/async/test_restore_economic_coin_effects.py` | PASS;1 full native method;0 failures/errors/skips;393.195s unittest,393.320694s process; both canonical0062 engines |
| Host `python -B -m py_compile` for both owned test files; `git diff --check` | PASS |

The original full native method preserves32 native coin cases,3026 decoder
cases/1054 accepted, both codec modes and both real databases. Each engine
passes109 canonical cuts/90 refusals/58 full entries,25 claim cuts/7 valid
controls and source/consumption pagination of258/257 rows. Original authority
is unchanged. These are retained/snapshot component checks, not genuine new
opening or financial producer journeys.

## Retained evidence and failed attempts

Protected evidence root is `D:/CodexEvidence/accounting-plan5/bin/`.
`claim-managed-final-seal-01-20261006/evidence.json` binds the base/result,
all6350 committed input bytes, archive/mode/link identities, both build outputs,
all selected run results and retained artifact hashes. Seal SHA256:
`cbe094e981501099671f7f8ec7dffcb0d62e624cb9c184b0e286399129392d61`.
The seal verifies3203 retained artifacts/4,558,252,570 bytes, including failed
attempts. Managed native qualifier observations total46:12 accepted/11 refused
per engine, preserving the original20 observations and adding three genuine
corrupt-import refusals each. Six independent audit diagnoses are retained.

Exact Docker commands, executed preparation/load/observer helpers, logs and
process receipts are under `claim-managed-current-*-build-01-20261006`,
`claim-managed-red-01-20261006`, `claim-managed-green-04-20261006` and
`claim-managed-native-full-02-20261006`. The successful managed run retains two
full dump generations, inventories, qualified restore receipts, real game-loop
logs and independent claim cuts. Artifacts/archives/binaries remain uncommitted.

Failed attempts are preserved and unqualified. Native-full01 passed native
decoder cases but stopped before SQL because the added read-only container root
refused the original `/` temporary directory; the corrected native-full02 keeps
the original test unchanged. Managed-green01 stopped in the evidence collector's
path-security call on the Windows bind mount; green02 checks collector-copy
hashes/sizes while all original production path checks remain. Green02 passes
MariaDB but two later MySQL corrupt candidates exhaust the original512MiB mount.
Green03 cleans stopped failed candidates and its two tests return OK, but the
observer rejects that outcome: all six corruption imports fail before reaching
the independent reader/native qualifier, with the successful candidate still
using restore capacity. Green04 frees that already verified candidate and adds
explicit progress assertions through real import and audit refusal. It preserves
the capacity, cases, controls and deadlines. Earlier OK/partial markers from
failed wrappers do not qualify the selected corruption methods.

## Remaining gates and curator handoff

All seven earlier branch tips remain ancestors on this same remote branch;
follow-ups continue there without history rewriting. The refresh observed primary
`9817f58b57a4a4179c4db5506fce3ccfe166c12b`. The publication receipt binds the final
remote tip separately from this issue result. Primary maintains the shared
notebook locally and the user declared it nonblocking; this owned report,
remote follow-up and sealed receipt are its curator packet.
`AI_CONTEXT.md` is absent in both inspected checkouts; that documentation read
is unavailable and does not count as a test skip or block this slice.

SELECT-only retained/native claim restore scope is covered by this slice.
Authenticated original opening policy/PID, complete R6 capture, real producer/
ACK/lost-reply/cold journeys, active erasure and full managed flatfile/retention
and mixed-workload release-host budgets remain open. Earlier canonical61 managed
or synthetic qualification is not promoted to this combined0062 candidate.
There is no independent slice blocker after its selected checks pass; primary's
shared selector/PID publication remains a genuine dependency for broader claim
qualification. Accounting stays inactive, wallet-root exclusions and the
declined inactive spell path remain, and no production action or correction is
performed. This is not Plan completion or release authorization.
