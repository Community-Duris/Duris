# Plan 5 claim posting batch qualification — 2026-10-06

The SQL snapshot collector replaced its source-root posting dictionary after
every 64-pair query. A cut containing67 root/account pairs therefore lost the
first64 posting summaries and falsely marked those sources invalid. Separate
owned fix `09b7ceac7925fd9e144302219c0709cafbc7aa07` accumulates all batches. The
original full restore method passes on both fresh canonical0062 engines with
the standalone two-file fix. The boundary probe is explicitly modeled SQL
metadata; its copied capsules do not authenticate the newly assigned root IDs.

All work stays in `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`,
on remote `codex/accounting-plan5`. Base is
`1b4ab18728e10b302d72ecff38650451a4fac53c`; result is
`09b7ceac7925fd9e144302219c0709cafbc7aa07`, verified as the remote tip after an
ordinary push. The worktree's separate partial-consumption changes were
preserved byte-for-byte while the exact qualified two-file overlay was staged.
All seven earlier preserved branch tips remain ancestors. No history is
rewritten and no push is made to `experimental-accounting`.

Owned files are `scripts/economic_sql_audit_snapshot.py` and
`tests/async/run_restore_accounting_evidence_mysql.py`. The fix initializes
the posting dictionary before the loop and updates it within each batch.
The regression adds65 modeled metadata roots to the two native fixture source
roots, traverses the64-pair boundary through the SELECT-only native reader,
then deletes exactly those modeled rows and verifies that captured table data
is restored. Original canonical controls run on the restored native fixtures.
No shared interface, schema, native implementation, producer, registry/matrix
or activation change is requested.

Both red and green use the original command, assertions, native compiler and
sanitizer flags, child deadlines and1,800-second outer deadline:

```text
python3 -u -B tests/async/test_restore_economic_coin_effects.py
```

| Tested source | Result | Observer seconds | Log SHA256 |
| --- | --- | --- | --- |
| Base plus metadata regression only; original reader | Exit1, two engine subtest failures, zero skips. Each reports67 source rows and64 invalid source roots. | 361.573029 | `449d87cdfc88801a6a0704dbdd772d66069d929f975abef37bc34c76d6a65819` |
| Base plus exactly the two committed files | Exit0, zero skips. Both engines retain all67 metadata summaries and pass the original full restore method. | 384.056009 | `3de2b31c4aade16dd1093997c3da3698421f4a661aa7fb2632ab8bb1ddcc6dba` |

Connected backends are MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL8.0.46-0ubuntu0.22.04.4. Setup verifies all62 immutable migration receipts
and head `0062_economic_pending_claim_consumption`. The passing method includes
the original32 native coin controls,3,026 native/independent decoder decisions,
both SQL and flatfile native fixture compilations with zero reused objects,
105 canonical cuts/89 refusals per engine,55 full-entry cuts,259-root pagination,
and21 claim allocation cuts/four valid controls per engine. Those existing
allocation cuts qualify the original retained readers, not the separately
pending snapshot partial-consumption fix.

The pinned offline image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with two CPUs/4 GiB and private workspace/tmp filesystems. Network is disabled;
the checkout `.env` is excluded. Exact environment is retained in each
`execution-helper.py` and `docker-command.json`:

```text
PYTHONPATH=/workspace/tests/async
PYTHONDONTWRITEBYTECODE=1
DURIS_REGRESSION_BUILD_CACHE=off
DURIS_RUN_RESTORE_COIN_INTEGRATION=1
DURIS_PLAN5_CANONICAL_EVIDENCE=1
DURIS_RUN_NATIVE_BASELINE_AUDIT=1
DURIS_RUN_STAKE_SQL_INTEGRATION=1
```

The last two gates do not run additional methods in this isolated batch slice.
Only the original restore command above is dispatched. Both observer containers
finish normally without OOM; red's expected command failure is distinguished
from the observer's exit0. Final source-unchanged receipts pass. The SELECT-only
reader runs in a read-only consistent snapshot and rolls back; only the owner
alters private fixture data. Captured table data returns exactly to its original
state. This does not prove unchanged AUTO_INCREMENT metadata or full-world
authority.

Protected evidence under `D:/CodexEvidence/accounting-plan5/bin/`:

- `claim-metadata-batch-red-01-20261006`, source archive SHA256
  `c2157d96648d73657519e188b4c8e27f489002814381daff73e3565115bb480d`;
- `claim-metadata-batch-green-01-20261006`, source archive SHA256
  `c71c617db0e39ec9dce9fc8e6ff4448e88af2df65cda52b4cab0920aa9b36baa`;
- `claim-metadata-batch-final-seal-01-20261006/evidence.json`, SHA256
  `b075c95ea4e32d1ce3d6581262c16870eebedfb35527e6746a9cc5c83fd0279c`.

The seal binds3,125 committed code inputs to the green raw source,49 retained
artifacts/319,426,437 bytes, original commands/log hashes, native outputs,
terminal states, source modes and staging preservation. Git's executable mode
and the archive's group-write mode are recorded separately. Retained evidence
size is not a release storage-growth measurement. This Python-only issue
changes no maintained C++ input. Prior production builds apply only to their
unchanged native tree `f0ae5c63273e94035552a75a1b70596d5021e54d`; migration tree
is `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.

A subsequent refresh observes primary
`26d7b66b86e1a38d09430386be257a065fceacd5`, with a separate shared baseline
NULL-price repair. That newer source is outside this slice's tested native
tree. The primary owns composing and qualifying its combined candidate.

This owned report, seal and expected remote branch provide the primary's local
notebook curator packet. Independent saved-snapshot partial-consumption coverage
is a separate slice. Authenticated opening-policy/original-PID reference,
complete R6 capture and activation verifier, real producer/replay/lost-reply/
cold-recovery journeys, typed erasure, full managed backup/restore/retention,
and release-host operational budgets remain gates. Accounting remains inactive;
wallet-root item exclusions and the declined inactive spell change are preserved.
No release completion, production operation or audit correction is claimed.

Subsequent separate [snapshot qualification](PLAN5_PARTIAL_CLAIM_SNAPSHOT_QUALIFICATION_2026-10-06.md)
closes that pending snapshot omission at
`1047e8c48cb9214d1c3fd76984472201ca80a4fd`, preserving this independently tested
batch commit and its exact source/evidence scope.
