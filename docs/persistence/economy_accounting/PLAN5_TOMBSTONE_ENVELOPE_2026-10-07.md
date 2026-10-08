# Plan 5 strict restore tombstone envelope — 2026-10-07

The restore preflight now requires a JSON object with integer version 1. The
previous equality check accepted `true` and `1.0` as version 1. A null, scalar,
or an array containing the four field names could raise an uncontrolled Python
type error. All of these now produce `invalid_tombstone_evidence` before a
restore candidate, service, or private database is created.

This fixes the existing envelope contract. Freshness, independent placement,
policy identity, duplicate-key rejection, nonempty-ledger refusal and the final
unchanged-evidence check retain their existing behavior. The actual erasure
custodian must still provide current authoritative evidence; the tests use
disposable synthetic ledgers and do not establish source-wide erasure propagation.

## Branch, ownership and exact source

Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Local and sole remote branch: `codex/accounting-plan5`.
Base commit: `d4fd388d1e1a76fd5a4b39e8914a9e8830fea4ba`.
The containing commit is the result; the post-commit delivery receipt records
its full SHA, matching remote tip, clean worktree and all seven earlier branch
tips as ancestors. No branch switch or edit to another checkout occurred.

Owned files are `scripts/persistence_restore.py`, the existing
`tests/async/test_persistence_backup.py`, this report and the additive remote
follow-up. The existing regression module gains one test method; all original
methods remain intact. There is no shared API/schema request or mutation-logic,
coordinator, producer, writer registry/matrix, activation or shared runner edit.
The primary can integrate this commit through the expected remote branch.

| Executed archive | Exact tree | Source overlay |
| --- | --- | --- |
| Red, based on Plan 5 base | `152f5a33d31d40bc6b927801df6a92daabc47a8b` | New regression only; unchanged old gate |
| Green, based on Plan 5 base | `812b0a939d96272c20225f0c4a7cd68a175c999a` | Exactly the two owned Python files |
| Green, based on published primary `5dc5b181978d01f5f21cf70463e984dcd4c80144` | `39f9f6500afdca882e0722bcd16a170033cffb79` | Exactly the same two Python blobs |

The two tested green blobs are:

- Restore: `23b17a3d158ac455105de1cd1303561dcc028ad8`, SHA256
  `e59d968e71fa8bf4fe24deea3fa2c05dddc25506a452621793f1900f964ff47e`.
- Regression module: `914653ba80cc7e234f878c9d7608f005ea3c56eb`, SHA256
  `c4b0180c8619bdcb6075892aca3b55867752a573658d67a8b456abb803948703`.

Plan 5 base has native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`,
migration tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, and canonical
0062/229-table manifest. The primary overlay has native tree
`833d3085815b396861ad18a77635412212381e4b`, migration tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, and canonical 0064/230-table
manifest. These are Python/filesystem qualifications; neither archive's native
tree is compiled or its schema applied by this slice.

The terminal refresh is `e9e4da5a14e106ddc4e8d1b78749f7d9042c5653`. It adds
three coordinator/review documents and retains the tested Python, native and
migration bodies. Raw refreshed documents are preserved in the evidence packet.
No private combined candidate or original lifecycle V2 encoder/installer is
imported or qualified.

## Commands, native results and preserved evidence

Evidence root:
`D:/Dev/Tests/Duris/accounting-plan5/tombstone-envelope-20261007/`.
`source.json`, `source.tar`, `docker-command.json`, native logs, terminal receipts,
host launchers, source guards, raw refresh, seal and delivery are retained.
The pinned existing image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Actual interpreter: Python 3.12.3; executable `/usr/bin/python3`, SHA256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.

All phases use an isolated, read-only, network-disabled container with direct D:
evidence mount and RAM-backed `/workspace` and `/tmp`. Native POSIX mode/owner
checks require that scratch filesystem: the existing D: WSL mount lacks permission
metadata. New host temporary helpers use `D:/Dev/Temp/`; no build output, package,
image, Docker volume or existing worktree is relocated. Docker became available
through external owner operations; this worker performed no start/stop/restart.

The native observer executes these exact commands in `/workspace`, with `-B`,
`PYTHONDONTWRITEBYTECODE=1`, `PYTHONPATH=/workspace/tests/async` and `TMPDIR=/tmp`:

```sh
/usr/bin/python3 -B -m unittest -v test_persistence_backup.RestoreTests.test_restore_requires_a_version_one_tombstone_object
/usr/bin/python3 -B tests/async/test_persistence_backup.py
/usr/bin/python3 -B tests/async/test_backup_review_remediations.py
```

The red phase executes only the first command: exit 1, four false-acceptance
failures and ten top-level-type errors. Both green phases execute all three.
The focused regression covers 18 malformed values in each of `flatfile-primary`
and `mariadb-primary`: 36 cases, each directly refused and refused again through
the complete restore entry point. It asserts unchanged ledger bytes, generation,
live files and journals, no candidate, and no invocation of the service,
database manager or native command runner.

| Green source | Focused regression | Full backup module | Full remediation module |
| --- | --- | --- | --- |
| Plan 5 | 1 test passes | All 41 tests pass | All 18 tests pass |
| Published primary plus two-file overlay | 1 test passes | All 41 tests pass | All 17 tests pass |

All commands exit 0; zero skipped tests. The differing remediation totals are
the complete unchanged modules at their respective bases, not filtered suites.
Existing valid-ledger, stale/future evidence, policy mismatch, nonempty tombstone,
changed-during-restore, retention and filesystem refusal assertions remain active.
These modules explicitly model database dump bytes. No MariaDB or MySQL daemon,
migration, SQL import, game service or native C++ executable runs in this slice.
Database startup is prohibited by this preflight result, so the entry-point test
asserts it is never called. Actual both-engine restore evidence remains the
separate published canonical-64 packet and does not qualify this candidate.

Archive SHA256 values are:

- Red: `3ef2ec95f4ca494a24b18e8b74378191f4c6f67fd2d31d6bca67d7041f67e55a`.
- Plan 5 green: `64fddebdd4e1c911ec88bf96c3d8558e2a7fc62ddad7732d87a283b4aadcd844`.
- Primary green: `77c483fab4bb8ac8ba523963d4319549bc493b1e0e70004943c5b35f09f6d15c`.

The first two archives authenticate all 6,436 regular Git bodies and four links;
the primary archive authenticates 6,461 regular bodies and four links. Terminal
native guards retain body hashes, canonical tar modes (Plan 5: 6,079 at 0664 and
357 at 0775; primary: 6,102 at 0664 and 359 at 0775), and original link targets.
All three Docker terminal inspections show exited, no OOM and expected exit
codes 1/0/0. No observation timeout or execution restart occurs.

## Disposition and continuing gates

This solved issue is complete and independently reproducible. It does not close
Plan 5, R7/R8 or release qualification. Remaining gates include the primary's
actual application of the restore/accounting-store provider-list handoffs and
owned pending-source fixture fix, all required tests on one published combined
candidate, native lifecycle V2 source/installer/recapture, complete independent
physical/currency/UID census, player/pet/treasury/history reconciliation,
original gameplay/fault/load journeys, applicable upgrade/rerun checks on both
database engines, authentic retention/erasure continuity and full R1–R8.

There is no runtime availability blocker at this slice's terminal inspection.
The private combined source and full native interfaces remain unavailable to
this worker. No accounting activation, audit correction, deployment, merge,
production change or inactive-path change occurs. The wallet-root money item
exclusion and declined inactive spell-path change remain preserved.

This report, follow-up, seal and delivery form the curator-ready notebook
handoff. The primary-maintained local notebook remains nonblocking; no notebook
application, acknowledgement or cross-chat notification is claimed.
