# Plan 5 coherent 0056 build, recovery and failure handoff

The current composed SHOP/mobile candidate does not qualify for release. Its
maintained SQL build fails on a nonexistent frozen-intent member, two accounting
contracts have obsolete source anchors, and the retained-publication fixture
fails before executing cases. The exact flatfile candidate clean-builds all
726 translation units and passes the managed recovery/retention journey with
two actual cold boots. These outcomes belong to the same frozen source below.

## Exact candidate and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Remote-visible branch: `codex/accounting-plan5-coherent-0056` on
  `Community-Duris/Duris`.
- Refreshed primary/base: `cfd9c8ab2405920691b9ec0613e0c6217fc93ae7`.
- Native tree: `85b990d3130bb0ac1f718bcedd288f4c57594a53`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`.
- Canonical schema source remains 0056. No migration is executed in this slice;
  previous 0055 or earlier combined native/SQL results are not substituted.
- Result commit and remote-tip verification are recorded in the delivery
  receipt `tmp/plan5/coherent-0056-delivery.json` and the delivery message.

The primary has composed 32 SHOP production inputs and installed the two-file
detached mobile stage. Compared with the previous frozen `92784e323` source,
33 native/Makefile inputs differ: 28 modified and five added. This freeze
records 6,149 regular tracked inputs, all 1,511 native/migration inputs, and
3,164 possible isolated recovery-copy inputs. Source bytes match Git blobs.

This report is the only tracked Plan 5 change. No shared contracts, coordinator,
producer, registry/matrix, migration or activation-owner files are edited here.
The earlier canonical SQL operator command remains separately published on
`codex/accounting-plan5-canonical-plans` at
`36355900e9cdf28e83413b5171cce2d43a3b9864`; it is not part of this primary base.
Existing work and evidence remain preserved. The primary's local notebook is
not a blocker; this report supplies its required curator handoff.

## Maintained build commands and results

Both builds start with new empty object directories and no cached objects.
The repository's full C++20 warning/hardening profile is unchanged, including
`-Werror`, default development profile and `-D__NO_TESTS__`. All artifacts are
under `bin`. The repository is mounted read-only into the immutable tool image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
with `--network none` and a writable `bin` mount. Tool versions are GCC 13.3,
Python 3.12.3 and OpenSSL 3.0.13 on Ubuntu 24.04.4. Native/migration input hashes
are verified before and after each build.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/objects-sql \
  DMS_BINARY=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/server-sql

make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/server-flatfile
```

- SQL: make exits **2** after 379 compile commands and 378 successful objects,
  260.226634919 s. Zero warnings, one compiler error, no linked server. Fresh
  partial objects and complete compiler argv/diagnostics are retained.
- Flatfile: make exits **0**, **726 compiled units / 726 objects**, zero reused
  objects, zero warnings/errors, 559.031951940 s. Server is 170,199,968 bytes,
  SHA-256 `6f5cbbc64f48cf45a2ded3f42bcfa4d896f2509954b6c0d8c8557a203cb80e0d`.

Exact compile/link output and source lists are retained in
`build-{sql,flatfile}.log` and `build-{sql,flatfile}-results.json` under
`bin/tests/plan5-coherent-0056-2026-10-05`.

## Shared defect 1: SQL frozen epoch access

The real compiler reports:

```text
economy/shop_trade_transaction.c:2146:28:
error: 'struct economic_frozen_intent' has no member named 'epoch'
```

The consumer is
`shop_trade_preparation_owner::build_accounted_command`, immediately after
`shop_trade_accounting_decode(projection, &intent, ...)`. It currently compares
`intent.epoch.bytes` with `prepared.mapping.epoch.bytes`.

The existing contract in `economic_accounting_intent.h` stores
`economic_frozen_intent.admission.metadata`, whose
`economic_operation_metadata.epoch` is the original 16-byte epoch. The narrow
requested source repair is this existing field path:

```cpp
intent.admission.metadata.epoch.bytes != prepared.mapping.epoch.bytes
```

Invariant: the decoded frozen command epoch must equal the originally retained
prepared mapping epoch. Preserve the wallet/bank-key comparisons, decoded
accounting schema check, original token ownership and one-time accepted timestamp
retention. This request changes no schema, wire layout, public API or field.
Consumers remain the original SHOP preparation/accounting decoder and their
native/SQL command owners. Required proof after the primary repair is the
maintained SQL/flatfile builds and the original accounted preparation checks,
including refusal on an epoch mismatch. This report does not apply the repair.

## Shared defects 2–3: source-contract anchors

The exact command

```sh
python3 -u -B -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
```

runs **71 methods: 69 pass, one fails, one errors, zero skips**, 13.659 s
unittest / 14.109322243 s subprocess. `contracts.log` retains the complete
results. The independent invariant suite passes; both failures are in
`SplitEconomyActivationContract`:

- `test_checked_item_placement_refactor_keeps_all_sites_classified` searches
  `static bool shop_trade_publish_physical(` for
  `obj_to_char_checked(object, buying ? ch : keeper)`. The current wrapper
  delegates to `shop_trade_publish_physical_impl`; the actual unique checked
  placement is in that implementation at `src/economy/shop.c:356`. The source
  probe should follow the implementation while preserving the complete
  checked-placement set and the existing `shop.buy_produced` owner assertion.
- `test_direct_sql_economy_sites_have_named_routes` indexes old lines
  887/907/940 and 1074. The current registry and source bind
  `shop.sql_native_item_events` to 1044/1064/1097 and
  `shop.sql_native_balances` to 1250 in
  `src/persistence/economic_sql_shop_trade_transaction.c`. The first lookup
  raises a KeyError at 887. Refresh these probes against the actual owning
  functions/unique SQL expressions, preserving exact route ownership, all
  detected-write coverage and unqualified backend/release status.

No registry/matrix or shared contract-test change is made independently.
Normal validator, generated matrix check and full inventory listing all pass
on these exact source bytes. Their success does not waive the two contracts.
The primary should rerun all 71 methods after updating the strict anchors.

## Shared defect 4: retained-publication recipe

The original `test_shop_trade_publication_retention.py` is invoked unchanged
through an evidence-only wrapper which retains exact subprocess argv and
diagnostics. It exits **1** in 22.684252139 s at the first SQL-header/owner
compilation:

```text
src/player/player_sql_transaction_cleanup.h:4 -> src/sql/sql_pool.h:16:
fatal error: mysql.h: No such file or directory
```

The SQL-header recipe supplies `-Isrc` but not the maintained SQL include path
`-I/usr/include/mysql`, now needed by the incoming owner source. The immutable
tool image has that header: the maintained SQL build progressed to the separate
epoch error. The primary must complete the native recipe/interface adaptation
and rerun the original owner/physical scenarios in both policies. Preserve
ASan/UBSan, `-Werror`, 300 s compilation limits, 30 s case limits, all 21 owner
cases and 16 physical cases per policy. **Zero publication cases executed**;
no result after the first compile failure is inferred. Any further defect must
be established by that rerun. `focused-publication.log` and
`focused-publication/commands.json` preserve this first failure.

## Passing native codec and managed flatfile recovery

The original `tests/async/test_shop_trade_command.py` passes unchanged,
13.708217795 s wrapper subprocess. Its exact generated C++ harness, compile/run
argv and binary are retained in `focused-command/`. Compilation uses its
original strict warning flags; no assertions or codec expectations are
changed. Binary SHA-256:
`0a6260c0ba770e447650a0de99bb4ae5dd3a26207f1a58a56caa5c05ccd34349`.
This preserves an existing command-codec regression; it does not qualify
accounted SHOP gameplay, cold physical publication or ACK.

The managed journey runs the original method:

```sh
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_SERVER=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/server-flatfile \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/flat-managed/native \
DURIS_REGRESSION_BUILD_CACHE=/workspace/bin/tests/plan5-coherent-0056-2026-10-05/flat-managed/native-cache \
python3 -u -B -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

The outer Docker launch also has `--cap-add SYS_ADMIN`,
`--security-opt seccomp=unconfined` and `--security-opt apparmor=unconfined`
for private tmpfs/user/network/PID isolation. No existing server is stopped.

**One method passes, zero skips**, 343.181 s unittest / 343.956315160 s
subprocess. Both actual cold boots use the current flatfile binary and reach
`Entering game loop.` (line 53 in both retained service logs). The journey:

- Replays the pending native transaction and drains native journals.
- Preserves the old inactive receipt, original source and captured generations.
- Retains two generations and prunes the unretained old generation.
- Refuses missing/corrupt manifest evidence before service boot in two cuts.
- Refuses checksum-valid corrupt receipt and receipt loss before capture,
  independently of transport checksums, before boot and without repair.
- Advances the allocator and sealed witness exactly once on the second boot:
  `(1000203, 2)` to `(2000203, 3)`, with only those two metadata files changed.

`flat-managed/native/evidence.json`, both service logs and
`cold-restart-state-delta.json` retain proof and binary/source hashes. The
fixture's origins are modeled, inactive native-codec history. Its source-capture,
lifecycle-install, activation and full-R8 flags remain **false**. This is an
actual maintained-service recovery component pass, not full writer/gameplay
or accounted SHOP/mobile qualification.

## Validators, preservation, curator handoff and remaining gates

```sh
python3 scripts/validate_economy_accounting.py                 # exit 0
python3 scripts/generate_economy_writer_coverage.py --check    # exit 0
python3 tests/run_integration_matrix.py --list                # exit 0
python3 scripts/validate_economy_accounting.py --release       # exit 1
```

Release refusal remains `writer has no executable evidence`. Inventory listing
is not execution of the full matrix. `git diff --check` passes. No source,
warning flag, assertion, route qualification or activation policy is weakened
to turn a failure into a pass.

The inherited 65,882 artifact hashes are retained and verified by the final
seal, along with every frozen regular/native/migration input. Exact final
counts, owned report hash and manifest checksum are in
`tmp/plan5/coherent-0056-evidence.json`; final result/remote commit verification
is in `tmp/plan5/coherent-0056-delivery.json`. All earlier branches/commits and
artifacts remain available. No log, player/account data, archive, credential,
environment file or compiled artifact is committed.

Curator: record the clean current flatfile build and two-boot managed recovery
with their component limits; separately record the failed SQL build, two failed
source contracts and failed publication recipe. Keep the combined candidate
unqualified until the primary repairs and republishes those shared inputs.
This handoff does not assert that the primary's local notebook was updated.

The two managed SQL full-dump/history/value/service-boot methods are **not
started**, because this candidate has no linked SQL server. An older binary
is not substituted. Current SQL/MySQL/MariaDB recovery and original accounted
SHOP preparation/publication/ACK must be tested after the primary fixes.
Saved-projection/original-command/child-receipt authentication, coherent future
migrations, actual producer installation, original mobile issuance and durable
reference restore, gameplay/fault journeys, major-plan execution and full
R1–R8 remain open. No accounting activation, direct experimental-branch push,
PR merge, deployment, production mutation or audit auto-correction occurs.
