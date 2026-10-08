# Plan 5: current combined builds, managed recovery and provenance handoff

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
The previous density repair was committed and remotely verified at
`9b0816d6ff19c3d48b95b07aecf8ebcf5eeaf570`. After a fresh primary fetch,
`c2b98c3e4935279ecf051c1be4e14dcdc82db759` merged cleanly into the separate
Plan 5 branch. Frozen base `55a314496555dcd9a768e94965ae125185f4d509` was
pushed only to that branch and verified remotely before qualification.
Native tree is `a5d8b2580bd794a0efa9a97032de4d1d4ff38ddd`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
Result commit and verified remote SHA are supplied in delivery.

Delivery refresh finds primary `2a3c05f3f5927bb5a49980f966c6435a55b9b4bf`,
native tree `ab5e68c90f00268b62c4b811c36b46fcfc4e4db7`, migrations unchanged.
It imports the already-tested density repair in `62487c668` and adds a bounded
active-root custody census preserving historical records. That new native
census is not mixed into this frozen run or qualified by these binaries.
The five stale metadata pins below remain unchanged in that primary advance.

This consumes the primary's actual compiler include, writer-contract and
native keeper-order changes. Earlier e018/d6 native results are historical;
they are not substituted for execution of this combined source. The audit
density reader remains byte-for-byte SHA-256
`5ab01420abdb9e7e6cd766cec7ae58373bf360cbae5bf08705e1e76b849b4d8d`.
Owned tracked file for this slice is this report only. Shared native source,
contracts, registry, matrix, migration and activation files are unchanged
from the imported primary. The metadata proposal below is tested on copies.

## Fresh maintained builds

Both full maintained server builds pass from new, separate object directories:

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/objects-sql \
  DMS_BINARY=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/server-sql

make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/server-flatfile
```

| Backend | Exit | Seconds | Unique compiled units / objects | Warnings / errors | Binary bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| SQL | 0 | 583.0690843390767 | 720 / 720 | 0 / 0 | 186,618,416 |
| Flatfile | 0 | 603.3577774998266 | 720 / 720 | 0 / 0 | 168,669,392 |

SQL binary SHA-256:
`b0a92dae8ede97ba7f774b40112a93ec8a44dd04104e71f14b518c3eddc0c104`.
Flatfile binary SHA-256:
`5dcec5754af9d257b2f224a47012aac7f9ca037f18a635520b88c1a49b58f974`.
Both are the maintained default development profile, with C++20, `TEST_MUD`,
`__NO_TESTS__`, persistent transport, original hardening and the complete
`-Werror` warning profile. Flatfile additionally uses `__NO_MYSQL__`.
No source overlay, warning suppression, old object or previous binary is used.
The previously failing NPC capture unit now compiles in both complete builds.
Its repaired source SHA-256 is
`c6062851c70d8d54e4677c3763ba6452c6af140aaa37eab6ba7f2b2c003400e5`.

Docker image `duris-plan5-origin-sql-tools:local` is pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It supplies Ubuntu 24.04.4, GCC 13.3, Python 3.12.3, OpenSSL 3.0.13,
MariaDB 10.11.14 and MySQL 8.0.46. Source is read-only at `/workspace`,
only private `bin` artifacts are writable, and Docker networking is disabled.
Recovery containers add `SYS_ADMIN` and `seccomp=unconfined` for disposable
tmpfs mounts and private user/network/PID namespaces. Recorders alone also
mount ignored `tmp/plan5` writable. No project `.env`, production credential,
database or live server is consumed.

## Current-binary managed recovery

The retained driver
`tmp/plan5/run-current-candidate-qualified-sql-restore.py` invokes these original
`test_persistence_backup_integration.PersistenceRecoveryIntegration` methods:
`test_mariadb_full_dump_schema_history_values_and_isolated_service_boot` and
`test_mysql_full_dump_schema_history_values_and_isolated_service_boot`.
It sets `DURIS_RUN_BACKUP_INTEGRATION=1` and
`DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1` before importing their decorators,
asserts that neither is skipped, and executes:

```sh
python3 -u -B tmp/plan5/run-current-candidate-qualified-sql-restore.py
```

The driver creates a private checkout from 3,146 verified tracked inputs in
`areas_mini`, `lib`, `migrations`, `scripts`, `src` and `tests`, retaining Git
executable modes. It copies and verifies both new binaries into private aliases
required by the integration class. SQL service boots consume the new SQL binary;
the new flat binary is checked as a setup prerequisite and is not executed by
these SQL methods. Native tools are freshly built inside the private checkout,
with a separate private cache. Observing wrappers call the real backup,
restore, independent database qualifier and service-load implementations;
they retain artifacts rather than substituting successful results.

Both tests pass, zero failures/errors/skips, exit 0: 655.787 unittest seconds,
681.4379618801177 driver execution/verification seconds and
766.5524153001606 outer seconds including checkout preparation. Source and
cold-restore versions match exactly per engine:
`10.11.14-MariaDB-0ubuntu0.24.04.1` and `8.0.46-0ubuntu0.22.04.4`.
Each uses fresh datadirs and distinct Unix sockets with TCP disabled. Both
fixtures migrate from the 170-table bootstrap to 226 runtime tables at
`0056_spell_ward_durability`, sequence 56. The captured apply checksum is
`e53e06ca7a5b482b6efee950a4db69fdb7d796b7ff71ed495e67bd3194133bb1`;
history checksum is
`fd82b219e1bdf95804fa6ee8dfe3d3c0fd29f59e910da83bf82c7e816f06cc60`.

There are 12 accepted qualifier checks and eight expected damage refusals per
engine. Refusals cover account association, wallet revision, bank revision,
missing cancelling currency rows, epic revision, missing cancelling epic rows,
wallet baseline and migration-history checksum damage. Damage is restricted
to disposable fixtures. Each full dump cold-restores exact modeled native
values `SyntheticRestore:42:11:15:23`, and preserves the original 355-byte locker
receipt, SHA-256
`cdaad1fb39c9264c866920ae2de2925cf57c5fed01746ebaff1317ad8707a9fa`.
The actual isolated boots enter the game loop in 2.17725824797526 seconds
(MariaDB) and 3.5619018028955907 seconds (MySQL). Captured generation inventories
and all 3,146 copied inputs stay unchanged. No replica is configured.
SQL retention pruning and full-world player/producer journeys are not covered.

The flatfile command is:

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_SERVER=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/server-flatfile \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/flat-managed/native \
DURIS_REGRESSION_BUILD_CACHE=/workspace/bin/tests/plan5-current-candidate-qualified-2026-10-05/flat-managed/native-cache \
  python3 -u -m unittest -v test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

It passes one test, zero skips, exit 0: 321.704 unittest seconds,
322.38774963095784 outer seconds. The native lifecycle codec fixture builds
with zero reused objects in a new cache. Native qualifier SHA-256 is
`c97d54e58efdcc1d62e5bc04fde083ce9eab2be18fc886d324463b4b926a15a5`;
state fixture is
`840f395eddd501e575f53b1ab4a1e2bdd6cdc7abd35b08aa543a0e91fac8692a`;
lifecycle fixture is
`c4a3ef1b34e174fd630ce3cdf7afa400f6d4f8296a44f1b13319da457e3a3f4a`.
These are fresh compilations even where deterministic bytes match history.
The returned cache executable and its metadata are retained.

Two actual isolated flatfile service boots pass. Pending native transactions
replay, player/critical journals drain, and original inactive lifecycle
receipts survive capture, restore and restart unchanged. Restart advances
only the sealed UID allocator/witness from next UID 1,000,203/revision 2 to
2,000,203/revision 3, the expected single boot reservation. The unretained
old generation is pruned; two valid retained generations keep exact evidence.
Missing/corrupt manifest-bound receipts, checksum-valid semantic corruption,
and a required receipt missing before capture all refuse before boot. Source
and captured generation inventories stay unchanged through the applicable
capture, recovery and refusal checks. Fixture-owner damage remains confined
to deliberately corrupted disposable cases.

Flat evidence explicitly records modeled known-native origins, inactive
accounting, no native source capture or lifecycle install, and
`full_R8_qualified=false`. This is current-binary recovery/retention evidence
for that modeled state, not new NPC/shop publication or real player journeys.

## One remaining shared contract finding and concrete proposal

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
  python3 -u -m unittest -v test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
python3 scripts/validate_economy_accounting.py --release
```

The 71-test contract invocation runs 15.954 unittest seconds / 16.549984348006546
outer seconds: 70 pass and one fails, zero skips/errors, exit 1. The repaired
checked-placement, terminal unloading and integrated-status interpretation
checks pass. The remaining failure is
`test_source_provenance_distinguishes_candidate_from_published_source` at
line 126, first identifying `src/economy/economic_command_admission.h`.
An independent comparison of all 56 component pins finds exactly five stale
values in `candidate_worktree_evidence.source_pins`:

| Source path | Current SHA-256 required by the strict contract |
| --- | --- |
| src/economy/economic_command_admission.h | 5ed371f12a0f6fa6764039c788c416ccacbc96a1435edad46ca5ec446e43e68d |
| src/flatfile/flatfile_accounting_coin_transaction.c | bb059c451b0d5cae84e30078393d6749c8137ce578ed86115c15bed45e03f894 |
| src/flatfile/flatfile_accounting_coin_transaction.h | 92949cacbe51eaa9b6a65536b7f94b32d4a7b80ac264ae0a259085126feaf1f4 |
| src/player/inert_item_stage.c | 629ea84a78d3f2787b9122abfc3317e06a2236274cf53a09fb18a7fdfc6af26e |
| src/player/inert_item_stage.h | 091f2cb9f7ffe81a0888b4f102a1c6874b2ce2f43549ca1203e7b7cbb4e9d05b |

The imported primary and tested checkout have identical bytes for all these
shared files, the registry, matrix, generator and contract test. The mismatch
does not come from the independently added density reader. Exact before/after
hashes are retained in `provenance-pin-mismatches.json`.

Narrow primary request: refresh those five existing metadata values in
`writers.json`, then regenerate `writer_coverage_matrix.json` with the existing
generator. Consumers are `generate_economy_writer_coverage.build`, the strict
provenance contract and release-evidence readers. There is no runtime field,
schema or native-code change. Keep integrated/unqualified status, historical
base/source fields and scope, route ownership, actual evidence, activation
refusals, `coverage_complete=false` and release `BLOCKED`. Do not weaken the
hash assertion or fabricate dirty-worktree fields. Preserve historical source
qualification at its actual older pins.

The concrete `provenance-metadata-proposal.patch` changes only those five
hashes in the registry and corresponding generated matrix values. A copied
registry is fed to the real generator against the complete frozen source.
Every generated field except these pins is proven identical to the original
matrix. The existing provenance method passes on this copied proposal;
reintroducing a stale pin or changing copied component bytes each produces
one expected assertion failure with zero errors/skips. Probe exit is 0,
19.099346577888355 seconds. Shared original registry/matrix bytes and all 56
actual component files remain unchanged. This narrow proposal proof does not
turn the original failing 71-test run into a passing repaired checkout.
Primary validation should rerun the 71 contracts, normal validator and generated
matrix check after adopting the metadata repair; release must continue refusing
missing executable writer evidence.

Normal validator and generated matrix check pass, exits 0, with 14 fixtures,
887 routes and 2,843 lexical occurrences; release readiness/coverage remain
false. Release validator returns expected exit 1 with `writer has no executable
evidence`. These inventory checks are not route or release qualification.

## Evidence and remaining qualification

Sealed artifact root is
`bin/tests/plan5-current-candidate-qualified-2026-10-05/`. The manifest
`tmp/plan5/current-candidate-qualified-evidence.json` has SHA-256
`8f3ccd9c2ebee0c9b90ca907d1687a398bac34bc5ac6c97b188e7c14337cf0db`.
It seals 6,193 fresh artifacts, all 1,498 native/migration inputs, all 6,116
regular tracked inputs and all 3,146 copied recovery inputs. Preservation of
all 40,338 prior artifacts and every frozen regular input is verified by bytes.
Four Git symlinks are recorded as metadata and are not copied into the private
recovery checkout. The source tree, exact commands/logs, binaries/objects,
private recovery inputs, native fixture/cache artifacts, database generations,
service logs, receipts, damage outcomes, copied proposal/negative checks and
delivery refresh are retained. The report alone is tracked/published; generated
evidence remains ignored local material.

Selected recovery-test source SHA-256 is
`14754afdc8a667a1a4926c62f519c55a540b1aca4ce4f8be9d1b529303f64b61`.
Database qualifier source is
`7eb4c2426a0250b694ecde7ee888ad3cb0c6bb4a1f384f223674350f14baed1c`;
backup manager is
`c5883f9289d62bd161834a7ccfd395745f0ace73b7776c20e5cecc2a85d9b52b`;
restore manager is
`44db4015239cb49f1356dc9978c16062cbff5d9d95d9fac7f195d040372b9f47`.
The manifest retains remaining individual input hashes and compiler commands.
All three managed recovery tests have zero skips; the original contract run
has one established failure and no skips. No broad passing suite or full release
claim is substituted for these exact results.

The primary's locally maintained shared notebook is not a blocker. This report
is its curator/integration handoff, with no claim of a remote notebook update.
The historical compiler finding is now resolved by executed maintained builds;
the five-pin metadata finding remains with the primary owner. Remaining release
gates include coherent 0057/0058/0059 source/migrations, original CCM1 admission
time, native capture/mapping authenticity, actual SQL/flat producer and gameplay
journeys, recovery/publication/guarded ACK of incoming components, complete
writer coverage, release-host operation/storage/retention/replica measurements,
and full R1-R8 acceptance. SQL retention pruning, replica recovery, production
build profile and a full-world player journey are not executed by this slice.

Inactive behavior, wallet-root item exclusions and the declined inactive spell
change are preserved. There is no activation, experimental-branch push, PR
merge, deployment, production-data change or audit autocorrection.
