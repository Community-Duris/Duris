# Watched-UID capture handoff — 2026-10-08

Implemented and component-qualified an explicit watched-item selection in the
existing read-only quest capture reader. Previously observed foreign-VNUM roots
and descendants can now remain in current-row/history reads after custody moves
or destruction. Missing requested current rows are reported, never reconstructed.
This is reader/query unit evidence, **not genuine SQL or native quest acceptance**.
The continuing native Goal remains BLOCKED; this finite delivery does not resume
or complete it.

## Exact bundle and preimages

| Pin | Value |
| --- | --- |
| Preserved prep parent / completed blueprint | `4e78ff44b7216c8336c063d61b97ff3f370d255c` |
| Executable code/test result | `cf0bb0a977e6995946c235b70feaaa604dc3f689` |
| Latest fetched primary / assignment review | `6f5d208de6dbf91d57e40b17e03b21f328337518` |
| Primary source / migration trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; unchanged from blueprint source pin |
| Reader preimage blob | `ecf52e1ec9dbdd0a72e0eb01890f4e099aa5f757` |
| Reader result blob | `8a079422a452028e4ff8c5e94f8031fa9b350358` |
| New focused test blob | `5d4aa65a80cb2d779a9b45ab0daeb3e39b47e462` |
| Existing `case_data.py` dependency blob | `44c08a70031bde27ebb9a0996c2706e93a5cf8d0` |
| Existing adjacent `test_quest_cut_checks.py` blob, unchanged | `d6c149c4632ecc939fd388c403883149ac5ce2f2` |
| Maintained imported mobile decoder `scripts/economic_restore_evidence.py`, unchanged | `06a2ebeaddd4bf604990b0f6cacf0c842555ccf3` |

The code commit changes only:

- `tests/async/quest_accounting_prep/capture_quest_cut.py`
- NEW `tests/async/quest_accounting_prep/test_capture_quest_cut.py`

The following documentation commit adds only this file. Its exact result SHA is
the containing commit (`git log -1 --format=%H -- <this-path>`) and is reported
with final remote delivery. Canonical `HANDOFF.md`, blueprint, previous evidence,
production source, existing assertion helpers, shared drivers, migrations,
manifests and registries are unchanged.

## Interface and behavior

CLI adds repeatable `--item-uid <uid> [<uid> ...]`; Python adds the optional
keyword-only `capture(..., item_uids=())`. Validation occurs before cursor/query
construction, and before connecting in the CLI:

- Each UID must be a strict Python integer with `1 <= uid <= 2**64-2`; bool,
  float, string, zero, negative, reserved `UINT64_MAX` and larger values refuse.
  The CLI uses its existing integer parser convention. Native allocator ranges
  exclude their end and the native runtime reserves zero/`UINT64_MAX`; this is
  selection validation, not proof that a requested lifetime exists.
- At most2,048 distinct UIDs, using the existing `MAX_ROWS` ceiling. Duplicate
  selections refuse, including duplicates across repeated CLI options. An
  iterable stops at its first excess value rather than materializing an
  unbounded input. Canonical watched metadata/parameters are sorted.
- Existing player, case-VNUM and recipient-stock predicates remain. Watched
  IDs add one parameterized `OR item_uid IN (...)` predicate. No recursive
  root/descendant traversal or new ownership interpretation occurs.
- Ownership history uses the union of current result UIDs and exact requested
  UIDs, including requested UIDs whose current row is absent. Existing history
  operation discovery then reads available economic evidence as before.
- Only nonempty selection adds `meta.watched_item_uids` and
  `meta.missing_item_uids`. The latter lists watched UIDs absent from the current
  result; history may still exist for them. No placeholder item/receipt is added.
- Omitting the option, or passing an empty selection, retains original queries,
  bindings and output shape. Existing positional callers and legacy gating remain.

Example future usage, **not a command executed against SQL in this delivery**:

```text
python -B tests/async/quest_accounting_prep/capture_quest_cut.py <existing-verified-disposable-capture-options> --item-uid <observed-root-UID> <observed-child-UID>
```

Carry the actual observed UID selection between cuts. Neither supplied IDs nor
empty `missing_item_uids` establish complete forest coverage, valid origins,
native authority or permission to recover. A selected root does not automatically
select its children. Native acquisition/census must supply the complete forest.

All original engine/epoch/legacy/disposable/credential/binary safeguards,
read-only RR transaction, BLOB preflights,2,048-row per-query/16MiB byte budgets,
rollback, transaction-close verification, cursor cleanup and CLI connection
close remain. The existing current-row union can still exceed its row limit;
adding watched UIDs grants no larger capture. History parameters remain bounded
by the selected current rows plus the explicit watched set, and returned history
retains its original row/byte limit.

## Executed focused checks

Windows commands ran from the existing prep worktree. Before tests:

```powershell
$env:TEMP='D:\Dev\Temp'
$env:TMP='D:\Dev\Temp'
$env:TMPDIR='D:\Dev\Temp'
python -B tests/async/quest_accounting_prep/test_capture_quest_cut.py -v
```

PASS:19 methods on Python3.12.10. The first development run had17 methods;
two CLI controls were then added and the complete19-method suite passed.
The fake cursor executes the real reader and records actual SQL/parameter
arguments. Covered behavior includes observed moved/destroyed foreign-VNUM
root/descendant cuts, retained history with missing current rows, exact UID
parameterization, nonrecursive selection, union deduplication, maximum selection,
invalid/duplicate/oversized inputs, unchanged filters/default output, original
engine/epoch/legacy/player gates, row/byte/BLOB/aggregate refusals, injected read
and rollback failures, transaction-close refusal and CLI connection/output safety.

An adjacent Windows control run:

```text
python -B tests/async/quest_accounting_prep/test_quest_cut_checks.py -v
```

retained16 passes/1 environment error: the existing legacy-XP helper imports
Linux `fcntl` through `native_build_artifacts`. No decoder build occurred; its
tested missing-obligation rejection precedes `decode`. No unrelated portability
edit or substitute codec was made. The same unchanged suite passed on WSL.

Exact WSL commands:

```powershell
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' -- env TMPDIR=/mnt/d/Dev/Temp TMP=/mnt/d/Dev/Temp TEMP=/mnt/d/Dev/Temp python3 -B tests/async/quest_accounting_prep/test_capture_quest_cut.py -v
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' -- env TMPDIR=/mnt/d/Dev/Temp TMP=/mnt/d/Dev/Temp TEMP=/mnt/d/Dev/Temp python3 -B tests/async/quest_accounting_prep/test_quest_cut_checks.py -v
```

PASS:19 reader methods and17 unchanged assertion methods, Python3.10.12. These
are unit/oracle controls with fake rows; the native-shaped mode of a fake capture
is not native execution. No SQL connection, migration, server or native build ran.

Additional executed checks:

- `python -B D:/Dev/Temp/quest-watched-uid-20261008/check_default_compatibility.py`:
  PASS,14 fake-cut scenarios (QP01–QP07, each native-shaped and legacy). Loads
  the actual reader source from prep parent `4e78ff44b...` using `git show` and
  compares with the changed reader: byte-identical serialized no-option output,
  identical SQL/bindings and cleanup. Temporary comparison script remains on D:,
  outside the repository; it is not another maintained reader implementation.
- `python -B tests/async/quest_accounting_prep/capture_quest_cut.py --help`:
  PASS, new option exposed alongside original flags.
- `git diff --check` and staged code diff check: PASS. Inspected diff preserves
  the original queries/transaction/cleanup/gates; only selection additions and
  CLI threading changed. Source/manifest/driver/assertion-helper diffs are empty.

Test output was observed directly; no logs, credentials, generated worlds,
player/account data or binaries are committed. CLI unit scratch uses temporary
directories on D: and removes them on exit.

## Import, remaining dependencies and next slice

At primary `6f5d208de...`, `tests/async/quest_accounting_prep/capture_quest_cut.py`
is absent. This is an incremental owned-pack change, not a claim that its patch
applies alone to the primary tree. First select the compatible previously reviewed
owned reader/`case_data.py` pack preimages above and their maintained script/
production-area dependencies; then review/import code commit
`cf0bb0a977e6995946c235b70feaaa604dc3f689` and this handoff. Do not import the
prep branch's older production tree or unrelated historical fixes wholesale.
Run the new reader suite on the actual integrated owned-pack selection. No
primary adoption or final qualification is inferred from remote availability.

The completed [QP02/QP03 blueprint](QP02_QP03_NATIVE_JOURNEY_BLUEPRINT_2026-10-08.md)
still governs authentic initialization/acquisition, full same-cut world/cash/
mapping observations, D ownership, held publication, ACK and paired cleanup.
Its conditional QP02 funding example is not currently executable: active quest
dispatch refuses numeric offerings, as the coordinator review records. Watched
UIDs remove one instrumentation gap, not that route or any native prerequisite.

Next concrete candidate slice, for a separate reservation: a bounded QP02
native-NPC-cost assertion over the original instance's maintained before/after
cash images, explicit original wallet-account identity, fee operation and exact
economic effects/postings, with player funding checked in its separate interval.
Reuse existing mobile/account decoders and cut/book patterns. Caller-supplied
mapping identity must not become authentication; the genuine same-cut mapping
and physical publication inputs remain unavailable. Therefore only its pure
reader/oracle portion could currently be component-qualified; native qualification
must wait for the existing owners. No such assertion edit is included here.
