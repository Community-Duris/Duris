# Plan 5: independent native custody census qualification

Branch `codex/accounting-plan5`; separate worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen, remotely verified base is
`3e1c9c86558f587a2dd059c26f0b86e9bda8a685`, importing primary
`2a3c05f3f5927bb5a49980f966c6435a55b9b4bf` after a fresh fetch.
Native tree is `ab5e68c90f00268b62c4b811c36b46fcfc4e4db7`; migrations tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
Delivery supplies the result commit and verified remote SHA. This branch alone
is pushed; the primary owns publication of the tested combined candidate.

Owned files are this report and the two new focused native regression files:
`tests/async/plan5_active_custody_census_fixture.cpp` and
`tests/async/test_plan5_active_custody_census.py`. No shared implementation,
contract, producer, registry, matrix, migration, activation or central test
registration is independently edited. Audit/operator production code remains
independent of mutation helpers. Native hydration/reset occurs only inside the
separate fixture process; it never touches a running server or database.

## Established problem and qualified observation

The primary's [source checkpoint](ACTIVE_CUSTODY_CENSUS_SOURCE_2026-10-05.md)
establishes why active custody and historical evidence need separate budgets:
retained destroyed/quarantined descendants can exhaust an all-state census
before a consumer filters it. The shared fix filters active state before both
counting and capture, while preserving the legacy all-state root API.
This slice independently executes those exact published functions.

The fixture demonstrates the distinction in a full 262,144-row native cache:
one active row and 262,142 historical rows belong to the selected root, with
one unrelated active row. The new active census succeeds with limit 1; the
legacy all-state census refuses that same limit without allocating. The legacy
census still returns all 262,143 selected rows with its sufficient exact limit.

Each backend configuration executes 29 cases. Small, deliberately unordered
rows cover exact limits, overflow refusal, maximum size_t, invalid/zero inputs,
null output, unknown roots, a history-only root whose root UID is absent,
conflicting owners, dangling/self parent claims, and stable UID ordering.
The active API observes every active claim rather than hiding conflicting
ownership/topology. Historical-only and unknown roots return an empty successful
observation; that does not prove root existence or world authority.

Four deterministic operator-new allocation failures per configuration verify
empty-output refusal without repairing cache state. Unknown/overbudget inputs
also run under allocation refusal and must avoid allocation entirely. Every
case compares every retained row field and owner revisions against the original
fixture; the full-cache cases compare all 262,144 rows. A newer owner revision
than retained rows and a legitimate zero owner revision remain unchanged.
Missing UID and owner lookups remain absent through the read-only peek API.

Exact shared source SHA-256 values:

- `src/item/item_ownership_runtime.c`:
  `a0a58d74b75fdbc78d4987a83804bd23ffbcf7bea96fefb06b78ea8b13ad5a25`.
- `src/item/item_ownership_runtime.h`:
  `19ba1140abdb70406ebd4f351f230715c6b6b15405c44688f315e76e13d2cafb`.

## Native commands and results

The pinned Ubuntu tool image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Runs use `--network none`, the checkout read-only at `/workspace`, and only
`/workspace/bin` writable. No production environment or database credentials
are used. GCC 13.3 compiles C++20 with warnings as errors, ASan/UBSan,
`-O1 -g -fno-omit-frame-pointer -fno-pie -no-pie -D__NO_TESTS__ -Isrc`,
`-Wl,--wrap=_Znwm -lcrypto`; flatfile additionally defines `__NO_MYSQL__`.
The fixture links the real ownership runtime, transfer command, craft pouch
mutation, chaos pouch ledger, player snapshot codec and critical command units.
Exact source lists, flags, commands, binary hashes and sanitizer environment
are retained in `native/*-results.json`.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
DURIS_PLAN5_CUSTODY_CENSUS_ARTIFACTS=/workspace/bin/tests/plan5-active-custody-census-2026-10-05/native \
python3 -u -m unittest -v test_plan5_active_custody_census

PATH=/workspace/bin/tests/plan5-active-custody-census-2026-10-05/formatter18/clang_format/data/bin:$PATH \
bash ./scripts/format.sh --check --file tests/async/plan5_active_custody_census_fixture.cpp
```

Both native tests pass: 2 tests, 58 executed cases, 0 failures/errors/skips,
82.495 seconds unittest / 82.695539 seconds outer driver. Compilation and
execution exit 0; both sanitizer stderr files are empty. Both probe binaries
have SHA-256 `47289ab8a323bc1f02236a81886d403634e6e3e83f0de0f32afa1c3a6a738b00`.
These pure-cache units do not vary with the backend macro; this is not durable
SQL/flatfile parity evidence. No database is opened by these two tests.

The single full-cache active observation plus its complete immutability check
takes 26,742 usec in flatfile configuration and 25,628 usec in SQL configuration.
Peak process RSS is respectively 84,448 and 84,196 KiB. Those samples include
fixture verification and are not release-host latency or storage qualification.

Python compilation passes with its output retained under bin. The repository's
scoped format check passes using clang-format 18.1.8, whose advertised wheel
SHA-256 was verified before extracting the tool under this fresh artifact root.
No global tool installation or source-independent package update was performed.

## Maintained builds and preservation

Both maintained configurations pass using verified incremental builds into
fresh artifact directories. These are not fresh clean builds. The original
720-unit clean builds are sealed in the previous combined-candidate report.
Only `item_ownership_runtime.c` and `.h` differ in the 1,498 native/migration
inputs. Before reuse, each previous object and dependency file matches that
sealed manifest; every repository dependency has unchanged exact bytes.
Absolute external library headers are verified as files under `/usr/include`
in the same immutable image, and their current hashes are recorded. No mount
or operation modifies those image headers. Copies preserve object mtimes and
hashes; dependency targets are rebased into new directories. Every object whose
dependency closure intersects either changed input is rebuilt, followed by the
maintained whole-server link. Original artifacts are never rewritten.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-active-custody-census-2026-10-05/objects-sql-qualified \
  DMS_BINARY=/workspace/bin/tests/plan5-active-custody-census-2026-10-05/server-sql
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-active-custody-census-2026-10-05/objects-flatfile-qualified \
  DMS_BINARY=/workspace/bin/tests/plan5-active-custody-census-2026-10-05/server-flatfile
```

| Maintained configuration | Verified reused objects | Recompiled units | Final objects | Build seconds | Exit/warnings/errors |
| --- | ---: | ---: | ---: | ---: | --- |
| SQL/MariaDB | 569 | 151 | 720 | 252.638983 | 0/0/0 |
| Flatfile | 572 | 148 | 720 | 254.239836 | 0/0/0 |

Verified copy preparation takes 184.352387 seconds SQL / 183.028492 flatfile.
SQL server: 186,618,688 bytes, SHA-256
`ed3cee2cbaed86f9dd4c5ad49ee3e9de82be9d0f1785c767c8e44e9115ac2d66`.
Flatfile server: 168,669,720 bytes, SHA-256
`ab9cb50ab5454fbe3fcdae3e722bbea1af529204eabd53c4bf17ff8b8fbe3eaf`.
Make retains its development profile, `TEST_MUD`, `__NO_TESTS__` and hardening
warnings-as-errors flags; no production-profile or in-game journey is claimed.

Two initial reuse-driver preflights stopped before make because the driver
treated external MariaDB/libxml headers as repository-relative inputs. The
driver was corrected to bind their provenance to the verified identical tool
image. Those partial copies, original driver and failure explanations remain
under this new root; retry copies use separate `*-qualified` directories.
The initial missing formatter and TLS issuer refusal are likewise retained;
trusted Windows TLS downloaded the advertised wheel, whose SHA-256 was checked.
No TLS verification was disabled. These are harness/tool setup corrections,
not native source fixes or successful first-attempt claims.

Evidence root is `bin/tests/plan5-active-custody-census-2026-10-05`.
`build-*-results.json`, `build-*.log` and `reuse-*.json` retain exact command,
invalidated/reused closure, dependency/header hashes, timings and binary results.
The manifest path is `tmp/plan5/active-custody-census-evidence.json`, SHA-256
`6052e243bdd94e8b37ad38d65eadc2197b5e33f51858ee7f56c7a82e0be84a71`.
It seals 3,054 new artifacts. Source preservation passes and
checks all 6,119 regular Git blobs against the frozen commit, all 1,498
native/migration inputs, both owned probe files, and all 46,531 prior artifacts.
The seal also records four symlink Git identities rather than executing them.
Both owned inputs and every prior artifact remain byte-identical; the sealed
artifact root is not modified afterward.

## Frozen metadata checks and delivery refresh

On frozen 3e1c9c865, normal validation passes: 14 fixtures, 887 writer routes,
2,843 candidate sites, `release_ready=False`. Matrix `--check` passes with
887 rows, 879 function anchors, 2,784 unique sites and zero unmapped sites;
`coverage_complete=False`, release `BLOCKED`. Release validation exits 1 as
expected with `writer has no executable evidence`.

The strict existing provenance method executes and fails once, with no errors
or skips, on the first stale pin. An independent inventory checks all 58 pins
and records the same five mismatches already handed to the primary. The
expected-failure driver exit is not reported as a green provenance contract.
Exact commands/logs are `*-command.json`, `normal.log`, `matrix.log`,
`release.log`, `provenance.log`, and `provenance-pin-mismatches.json`.

A delivery fetch finds primary `b67c1fb0defb07cc0a088a47723ec647d85c6db0`.
Its native and migration trees exactly match this frozen run. It publishes the
five-pin and canonical-LF checkout policy repair in 66560837c; private shop/NPC
handoffs remain source-only. The frozen failure above is historical evidence,
not an assertion that the repaired primary still fails. Import and execution
of the repaired metadata are a separate qualification slice.

## Remaining gates and curator handoff

No new shared schema/interface request is needed for the cache observation.
Existing legacy consumers are unchanged; the new active API has no published
in-tree consumer beyond this probe. Its private shop SQL consumer, native
destination weight/retry continuation, producer wiring, cold recovery and
guarded ACK must land coherently and receive their own native/SQL/gameplay tests.
The private explicit shop v7 weight facts and NPC v11/native-reference recipe
handoffs are not consumed or qualified from unpublished proposal files.

Disposable MariaDB/MySQL recovery tests and flatfile managed lifecycle recovery
were executed in the [previous combined slice](PLAN5_COMBINED_BUILD_RESTORE_AND_PIN_HANDOFF_2026-10-05.md)
on its exact frozen candidate. They are not rerun or promoted as new producer,
native capture or full-world evidence here. The new pure cache API has no SQL
consumer to exercise against a disposable database in this slice.

Original command-admission timestamp authentication, coherent 0057/0058/0059,
actual native source/capture authenticity, producer journeys on both durable
backends, complete writer evidence, release-host operation/resource budgets,
SQL retention/replica qualification and R1–R8 release acceptance remain open.
Inventory coverage, synthetic fixtures and isolated native success do not close
these gates. Accounting stays inactive, wallet-root exclusions remain, and the
declined inactive spell-path change is preserved. No production data, audit
autocorrection, activation, deployment or merge of a PR occurs.

This report is the exact evidence handoff to the primary's locally maintained
shared notebook/curator workflow. Notebook maintenance is not an engineering
blocker and no remote notebook update is claimed. The overall goal remains
active; this completed cache qualification does not establish release completion.
