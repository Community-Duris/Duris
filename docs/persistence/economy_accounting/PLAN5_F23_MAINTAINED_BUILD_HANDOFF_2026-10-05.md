# Plan 5 maintained builds on the combined f23 candidate

Both maintained server policies compile all 726 units with zero warnings and
link successfully after relocating only the final output to a drive with free
space. The previous SHOP shadowing defect is closed in this source. This is
build evidence; release qualification remains open.

## Source and ownership

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `d18f8de63d2f7f7605bd37567d64435b6cf86650`, preserving merge of primary
  `f23aaa3c725c720f70ab2182a0942da511da90dd` into Plan5
  `fddc14b37c5eff2312e4d556de103a8acca9bab0`.
- Native tree: `139556ad49b39d006588572015dc5006e8186a59`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, public
  canonical0056. Earlier0055 results and private successors do not qualify it.
- Sole owned file: this report. Result commit and verified remote are recorded
  in `tmp/plan5/f23-build-delivery.json`. Pending independent baseline-reader
  edits are outside this slice and do not change its native/migration inputs.

The preserving merge also retains the compiler erratum in the7cd9 report:
the tested compiler is GCC13.3.0. No shared implementation, accounting contract,
producer, migration, registry/matrix or activation change was independently
authored. All earlier branch histories remain ancestors of the requested branch.
No interface or schema request is needed for the observed build results.

## Maintained build evidence

Image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
GCC13.3.0, Python3.12.3, Make4.3; network disabled and checkout read-only.
Each policy starts with separate empty objects and isolated `BIN_ROOT`.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  BIN_ROOT=/workspace/bin/tests/p5-f23-build-20261005/bin-sql \
  OBJDIR=/workspace/bin/tests/p5-f23-build-20261005/objects-sql \
  DMS_BINARY=/workspace/bin/tests/p5-f23-build-20261005/server-sql
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=/workspace/bin/tests/p5-f23-build-20261005/bin-flatfile \
  OBJDIR=/workspace/bin/tests/p5-f23-build-20261005/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/p5-f23-build-20261005/server-flatfile
```

| Policy | Fresh compile units/objects | Warnings | Initial exit/seconds | Final-link retry exit/seconds |
| --- | ---: | ---: | --- | --- |
| SQL | 726/726 | 0 | 2 /497.091 | 0 /93.093 |
| Flatfile | 726/726 | 0 | 2 /510.376 | 0 /85.249 |

Both initial link attempts report `/usr/bin/ld: final link failed: Input/output
error`. The Windows `C:` bind mount has approximately 74 MB free; Docker's filesystem
also reports 100% usage with 4.0 GiB available. The failed attempts, input maps,
commands, logs and all fresh objects remain preserved under
`bin/tests/p5-f23-build-20261005/`. No evidence or unrelated container is deleted.

For each retry, execute the identical command above with only
`DMS_BINARY=/plan5-output/server-sql` or `server-flatfile`, respectively.
`/plan5-output` is a writable bind mount of
`D:/CodexEvidence/accounting-plan5/20261005-f23`; that drive has ample free space.
Make emits zero compile commands on either retry. The links consume only the
fresh726 objects from their corresponding retained initial attempt. These are
final-link retries, not separate fresh builds or cached native probes.
The full maintained sources, hardening, warning profile and libraries remain.
Native/migration hashes are unchanged before and after all four commands.

| Output | Bytes | SHA256 |
| --- | ---: | --- |
| `server-sql` | 192803584 | `9a2015be06661256160a90ea44c5d693c20d0f0e09efcf5bc6f50b02145387ea` |
| `server-flatfile` | 170838952 | `234463e996bbd344cab816070162aefe13e9b443b5c2cc233a87cac2bda117c8` |

After both links, the complete new output directory is moved intact to
`D:/CodexEvidence/accounting-plan5/bin/f23-maintained-20261005`, keeping all
compiled artifacts under `bin/`. Retry logs, receipts and binaries remain there.
`tmp/plan5/f23-build-evidence.json` seals their original locations;
`tmp/plan5/f23-build-retention-evidence.json` records the verified relocation and
current report hash. Original objects remain available in place. All bytes and
SHA256 values are preserved; no data or unrelated work is removed.

## Contract checks and release gates

```sh
PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 -u -B scripts/validate_economy_accounting.py
python3 -u -B scripts/generate_economy_writer_coverage.py --check
python3 -u -B scripts/validate_economy_accounting.py --release
```

All71 original methods pass with zero skips:12.285 seconds in unittest,
12.558 including process startup. Ordinary validator exits0 in12.559 seconds;
matrix check exits0 in14.301 seconds; release exits1 in0.200 seconds with
`writer has no executable evidence`. The input map and unchanged-source receipt
were sealed before the pending baseline-reader changes. Inventory and source
contracts do not establish original producer coverage or release completion.

No database, service boot, gameplay, backup/restore, production data or activation
is involved in this slice. The primary's expanded cold SHOP component recipes
still need their declared executable repair and qualification. NativeEAB2 and
schema integration, cold flatfile parity and remaining primary publication
gates, original writer capture, fresh/populated database upgrade qualification,
real backup/restore/retention, gameplay/workload/fault journeys and R1–R8 remain
open. The earlier link failure is resolved by output relocation; no selected
test skips or notebook blocker remains. Preserve inactive behavior, wallet-root
item exclusions and the declined inactive spell-path change.

This report is the curator handoff for the primary's locally maintained notebook.
Import it from remote `codex/accounting-plan5`, retain the failed attempts and
successful links as one measured build sequence, and register evidence only for
the exact recorded source and scope. Central registration and publication of
the tested combined candidate remain primary owned.
