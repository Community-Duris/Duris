# Plan 5 MariaDB staging data-storage comparison — 2026-10-11

The fresh Windows-bound control reproduced the immutable0060 staging rename
failure. The fresh native Linux-volume arm passed the identical original
staging45 append, rerun, receipt/data preservation and exact65 history/state
checks. Changing the datadir storage profile avoids this reproduced failure
under the tested pipeline. MariaDB automatically chooses case mode2 on the
Windows-bound filesystem and0 on native Linux storage; the comparison does not
independently separate that engine-derived setting from the filesystem or
establish a kernel/immutable-SQL defect. It qualifies the storage profile for
this staging slice, not the full matrix,65 startup, restore or release.

Owned branch/remote `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`. Slice base
`d7571a7e11459695f0e5e68cdef0bbc3388e2c35`; result and verified remote are bound
by post-publication `delivery/result.json`. Owned maintained files are this
report and additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. No maintained code,
schema, interface, registry, matrix or activation changes; no shared interface
request is required to retain this result.

## Exact native source, commands and predicates

Frozen source `a60f5dcce0c6496e687a0806ee7d0c559579e4fb`, tree
`3f8d87b957866857ecd3833c71fdace8b671bfed`; src tree
`df568142d3c57d74683bed76da2a0eec67d4a326`, migrations tree
`95aa09d334004bc048dec65affa2cdd86a702cd6`. All6901 exported files, including
both private qualification contracts, remained byte-exact after both arms.
Archive SHA256 `cafdd1c7f774b896f208215cd3569ab41ecf70b596e5f0d3605a45ba5339f980`.

Windows CPython3.12.10 invoked:

```text
C:/Users/alexa/AppData/Local/Programs/Python/Python312/python.exe -B
  D:/Dev/Temp/accounting-plan5-ACC02-20261011/run_mariadb_storage_pair.py
  --preparation D:/Dev/Tests/Duris/accounting-plan5/ACC-02-MARIADB-FS-01/preparation-receipt.json
  --grant D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-02-MARIADB-FS-01-resource-grant.json
```

Helper SHA256 `aa427c4bdfa2b5f9bb690de0bb88b00e759f7f435dd2a3d4713c1858563a1e24`;
preparation `e815dd82bcd0e2a2f39d75a849f395a66e404ff4195e5a79a5bba26ae1e77c3a`;
grant `8c936928645c1c9a7a1ecafdb5fb73824f0440050891753f57bfdd4f5e06c146`.
Native session57476 finished terminalexit1 at08:34:57Z after173.009seconds.
Exit1 records the deliberately retained failed control; the Linux arm passed,
and no cleanup/source failure occurred. Observation timeouts did not restart it.

The helper imported unchanged maintained helpers and executed the original
`tests/async/test_staging_migration_fork_mysql.py` staging block, lines454-480,
dedented without code changes. Block SHA256
`77b34fc74273f2315f8b60028872c5a9a5ea931eafa5eb7796480d96ed33003e`;
full maintained runner `908ab9a227cf732b86bd3503aa0d3ccec07e0d2f35998568ea41d56543ba723d`.
The real prefix SQL/verifiers, seed history, canonical-selector refusal,
protected receipts/descriptions, exact append/rerun/count and original65
history/state predicate were preserved. Original MysqlExecutor ran the pending
migrations in serial pinned QA containers. No verifier, fixture, SQL default,
enforcement switch, native assertion or immutable0060 payload was changed.

## Actual paired results and storage boundary

Both `SELECT VERSION()` results were10.11.19-MariaDB-ubu2204 from pinned image
`sha256:ce66c7be32a03aabe7241d0a10993a2db827ef652a35d25727d92a832ac8ef73`.
QA image `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
At most one512MiB/CPU1 DB and one serial2GiB/CPU2 QA were active. Both DBs used
networknone/restartno, identical default entrypoint/options and Linux overlay
`/tmp`. No C++ compilation, native gameplay or other backend ran in this slice.

| Arm | Datadir | Engine-derived case mode | Original staging result |
| --- | --- | --- | --- |
| Control | Direct D: bind, `v9fs` | `lower_case_table_names=2` |0060 failed with the same InnoDB missing-source rename |
| Linux | Fresh local Docker volume, `ext2/ext3` | `lower_case_table_names=0` | Append/rerun/preservation/count and exact65 history/state passed |

SQL mode was identical: STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,
NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION. `check_constraint_checks=1` on both.
`paired-command-comparison.json` mechanically verifies all common DB startup
arguments after normalizing fresh CID-file/container identities; only the
datadir mount kind/source differs. Case mode was selected by the engine from
the filesystem, with no manual configuration change.

Control CID `e9a160c88cdfcca0b0b5f97e226d0efc861b4d37e8ce5b0f17a74a12429f1f33`.
At08:33:24Z, connection261 reported missing `item_owner_revision.ibd` while
renaming to `#sql-backup-1-105.ibd`. This is the comparable retained failure
observed in RETRY-02, not an invented expected rejection. Linux CID
`808d72eb1e81377457ca4c4bac58bfd67f9e4223bd02d48dbcf7936b58134cdd`.
Its native65 staged history and state matched the registered checksum
`9e81f924525c9e089a92559d1429e83f7e1c05cead3ca652a89eb68ec5534f97`.
Its daemon log contains no matching rename error. The control finished cleanup
before Linux started; there was no concurrent DB pair.

## Evidence, cleanup and next accepting gate

Evidence `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-MARIADB-FS-01`: source archive,
6901-file inventory/export, exact staging block, helpers and preparation/grant/
lock snapshots, paired command comparison, client/daemon logs, live/terminal
states, CIDs, per-arm receipts, native output and independent final inventories.
Manifest SHA256 `0dc00da9c1240b365ff62ec8e4594b2da3511ac2380c321cf0e2f203fb198961`
seals6946members/550808899bytes. All members were rehashed before lease release.

Each DB was inspected for exact CID/image/owner/job/source, then stopped safely
with daemonexit0/OOMKilledfalse and removed with exit0. Both per-arm inventories
and an independent final exact-owned inventory succeeded and were empty. Seven
unrelated services preserved their initial IDs and remained running. Only after
sealing was the matching FS-01 global lock released; its delivery release receipt
binds the manifest. No owned active container or heavy lease remains.

Passive volume `plan5-acc02-maria-fs01-linux-data` remains on Docker's registered
D: storage, local driver/optionsnull, labeled with exact owner/job/source and
retention purpose. Independent volume-consumer inventory succeeded/empty. Its
gracefully stopped datadir copy is retained under `linux/backends`; control data
under `control/backends` is also preserved. These private data/keys are excluded
from immutable result members and Git. The copy is diagnostic scratch, not
qualified backup/restore or retention evidence.

Next accepting gate is the original full two-engine matrix on fresh frozen
source/build/evidence, with a reviewed external transport restoring native Linux
MariaDB data storage and retaining guarded logs/cleanup. It requires its own
orchestrator grant. No full retry has started; no schema65 fingerprints/flags
have been written from this partial result. MySQL's earlier one-engine measured
fingerprint remains unqualified. Primary owns any measured manifest/header
integration and the subsequent combined-candidate build/publication.

Current65 full histories/measurement and accepting shell/compiled boot/tamper,
native restore/room/wallet authority, backup/retention/erasure, recovery/load and
R1-R8 release gates remain OPEN. Later primary source is not qualified by this
frozen slice. Full goalACTIVE, accounting inactive/admissionCLOSED. Report and
follow-up are additive curator input; notebook import and release acknowledgment
are unclaimed.
