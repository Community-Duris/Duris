# ACC-01 frozen combined executable checkpoint

Both fresh production binaries compile and link on frozen source
`6022c0ad7feab34c13e27738d4295b65b8d010ac`. The maintained inactive flatfile
preflight passes health, missing/stale UID authority refusal, exact restoration,
game-loop boot and clean shutdown. Runtime source contracts pass 10/10.
Accounting remains inactive, admission CLOSED and release qualification false.
Mutable SQL startup was deliberately sequenced with Plan 5's isolated schema65
and restore work; no SQL gameplay or two-engine acceptance is claimed here.

## Frozen input and observed toolchain

- Owner chat: `01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0`.
- Job: `ACC-01-6022c0ad7`, disposable foreground `duris-build-a`, attempt 2.
- Native tree: `df568142d3c57d74683bed76da2a0eec67d4a326`.
- Migration tree: `95aa09d334004bc048dec65affa2cdd86a702cd6`.
- Archive SHA256: `74774296e26462008de621c727752c4631a75337b3510620f3db29600caea8d0`.
- Pinned image: `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
- Observed compiler: GCC 13.3.0, Ubuntu package toolchain; exact compiler/linker
  search directories and available libraries retained in `toolchain.log`.
- Container ID: `cee234bd9277fda9fd5874c760509140cd86c754e6b69990ba4dbc105a2dffb7`.
- Resource limits: CPU2, memory3GiB, swap4GiB; network none; restart no.
- All maintained make flags and dependency recipes retained; `BIN_ROOT`,
  `OBJDIR` and `DMS_BINARY` place generated artifacts under the owned D: mount.

No preexisting object cache was accepted. Each backend compiled 789 objects from
the read-only export. Before/after source inventory hashes agree. Attempt 1
failed before compiler because the nested read-only source lacked the empty
`bin` mountpoint; that failure and released receipt remain preserved. Creating
the mountpoint changed no source bytes or compiler flags.

## Terminal results

| Step | Result | Elapsed seconds | Binary bytes / objects |
| --- | --- | ---: | --- |
| SQL production compile/link | PASS, exit 0 | 804.105 | 281222776 / 789 |
| SQL linked library inspection | PASS, exit 0 | 0.036 | Actual linked binary |
| Flatfile production compile/link | PASS, exit 0 | 773.948 | 240495464 / 789 |
| Flatfile linked library inspection | PASS, exit 0 | 0.033 | Actual linked binary |
| Maintained inactive flatfile preflight | PASS, exit 0 | 3.323 | Supplied freshly built binary |
| Runtime compatibility source contracts | PASS, 10 tests, exit 0 | 39.509 | Original maintained test module |

SQL binary SHA256:
`7a51cc92d0eca598c00e0040a5f7958b122909c7c221ae65ad6b65529a53d859`.
Flatfile binary SHA256:
`b566691caf34c2a04d70a735c976bb6ad03549b2136afded02e57b31c4df06d7`.

Commands:

```sh
make -C /source/src -k -j2 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production \
  BIN_ROOT=/source/bin OBJDIR=/source/bin/objects/sql/production \
  DMS_BINARY=/source/bin/server/dms_new
make -C /source/src -k -j2 PERSISTENCE_BACKEND=flatfile BUILD_PROFILE=production \
  BIN_ROOT=/source/bin OBJDIR=/source/bin/objects/flatfile/production \
  DMS_BINARY=/source/bin/server/dms_restore_flatfile
python3 tests/async/test_flatfile_boot_preflight.py \
  --server /source/bin/server/dms_restore_flatfile
python3 tests/async/test_runtime_boot_compatibility.py -v
```

## Evidence and resource release

Evidence directory: `D:/Dev/Tests/Duris/accounting-accel-integration/ACC-01`.
Build directory: `D:/Dev/Builds/Duris/accounting-accel-integration/ACC-01/bin`.
Cache directory: `D:/Dev/Cache/Duris/accounting-accel-integration/ACC-01`.
Receipt directory: `D:/Dev/Temp/accounting-acceleration-bootstrap-20261011`.
Source, bin, evidence and cache were direct D: bind mounts. Private 128MiB `/t`
tmpfs supplied Linux owner-mode scratch; persistent evidence remains on D:.

| Evidence | SHA256 |
| --- | --- |
| `RESULTS.json` | `5c930d7aeed6f51fda0952f78fb355dc90c6b22bc416765a89832a2abbb61ef4` |
| `toolchain.log` | `fcfc736c253cf31c0e72d313ba9341b74277c519453db1243d69b74cc0e893b0` |
| `sql-production.log` | `035605a60880c78526303585c161f6bd45f4382261bbdab7fc675f86be0fea2a` |
| `flat-production.log` | `9d599f1e1fa38b84e1f66162c879bb415e51b2481bb25b4977044ee0167b0e9b` |
| `inactive-flat-boot.log` | `802f06ddfb0cd06301d1f0a50dfef20a5459389470fc51ca6cb643c01fd65ae4` |
| `runtime-source-contract.log` | `e7c319fe9162a24a85edcbe53289dfa404075401d77e4b346b8d59fd32df8452` |
| `ACC-01-lease.json` | `f462e24338da2b7ae6e1e0746838678bbbe325ac8ba34dd0f8b8d82b63a2589a` |

Host finally completed at `2026-10-11T07:18:19.4691544Z` with exit 0. The
exact owned container was absent on subsequent Docker inventory, the receipt
was RELEASED and the matching exclusive heavy lock was removed. Seven unrelated
services remained untouched; no images, volumes, recovery containers or evidence
were pruned. The first checkpoint target was met within an hour of launch.

This receipt qualifies only its frozen bytes and listed predicates. Later
primary changes (`757c3f6fe`, standalone recovery forest providers) require
follow-up validation; ACC-01 is not relabeled to include them. Plan 5's ACC-02
private candidate `a60f5dcce0c6496e687a0806ee7d0c559579e4fb` has the same
native/migration trees and separately reviewed audit reader/output changes.
Schema65 measurements remain pending and shared constants stay primary-owned.
