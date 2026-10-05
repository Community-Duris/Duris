# Plan 5 published 0056 native and managed recovery qualification

This qualification consumes the primary's published candidate with the new
shop recovery-binding module and canonical migration 0056. Earlier 0055 or
older native-tree results are not relabeled as this candidate. Full accounting
and original R1–R8 release acceptance remain incomplete.

## Candidate and ownership

- Branch: `codex/accounting-plan5-published-0056`, independently remote-visible.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Exact base and refreshed primary:
  `6fc75e410a40ff879228919273d21940d8d27ecb`.
- Native tree: `5ca91c7f3369df5ce655af15bf3ffb6afba7ef20`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, through
  `0056_spell_ward_durability`.
- Result and verified remote commit SHAs are recorded after commit in
  `tmp/plan5/published-0056-delivery.json` and the slice delivery.

This report is the sole owned tracked change. The candidate is read-only during
qualification; no shared native source, migration, accounting contract,
coordinator, registry, matrix or activation owner is edited. No shared interface
or schema change is requested by this qualification.

The previous child identity fix remains preserved at remote
`codex/accounting-plan5`, commit
`50495fdd2fdf43ef5c513e6eb95f543a886b2711`. The frozen primary candidate lacks
that fix and its central registration. Branch selection preserves both histories
without a merge, reset or force push. The final refresh observed
`0366c30a9c4c90f12494830ff5136df5edccf05f`, which imports the child identity fix
and its central adapter/registrations. Its native and migration trees exactly
equal this freeze. No execution of that newer combined audit/coordinator source
is claimed here; the earlier integration request is now resolved in source.

## Source freeze and maintained builds

The freeze validates 6,131 original regular tracked inputs against exact Git
blobs, including all 1,500 native/migration inputs. Relative to the preceding
qualified `ab5e68c90f00268b62c4b811c36b46fcfc4e4db7` native tree, exactly three
native inputs differ: `src/Makefile`,
`src/economy/shop_trade_recovery_manifest.c`, and its header. Removing the one
new Makefile object line reproduces the old Makefile byte-for-byte; flags, rules,
existing object order and every existing translation-unit dependency are exact.

Both builds use immutable tools image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
with GCC 13.3, Ubuntu 24.04.4, Python 3.12.3 and OpenSSL 3.0.13. The source bind
is read-only, `bin` is writable, and containers use `--network none`.

The builds copy the preceding qualified objects to fresh directories only after
checking every object and dependency-file SHA-256, all original dependency
inputs, exact immutable image identity and the bounded Makefile difference.
Dependency targets are rebased in the copies; original evidence is untouched.
The previous full 720-unit clean builds and subsequent custody rebuilds retain
their original source scopes. This is verified incremental qualification of
complete newly linked servers, not a claim of new clean builds.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-published-0056-2026-10-05/objects-sql \
  DMS_BINARY=/workspace/bin/tests/plan5-published-0056-2026-10-05/server-sql
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-published-0056-2026-10-05/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-published-0056-2026-10-05/server-flatfile
```

| Backend | Exact observed build |
| --- | --- |
| SQL | Exit 0; 720 verified reused objects, one new module compiled, 721 linked objects; zero warnings/errors; 140.655044 seconds verification/copy, 80.288293 seconds make |
| Flatfile | Exit 0; 720 verified reused objects, one new module compiled, 721 linked objects; zero warnings/errors; 140.379939 seconds verification/copy, 78.594023 seconds make |

The new translation unit is compiled with the maintained C++20 warning-as-error
flags in each configuration. The SQL server is 186,848,568 bytes, SHA-256
`6d4b3c24dc4b595e2b82f21251aae0101958c87ea081cbf6dbeee7d35b203ac0`.
The flatfile server is 168,903,696 bytes, SHA-256
`770e94e5f734b256796e34ed1cab066bca6542fb5bd3db527ee264681e395a9e`.
Exact compile/link commands, dependency and reuse proofs are retained. Linking
and ordinary boot do not establish the new pure binding codec's runtime
semantics or actual SHOP publication/recovery consumption.

## Managed backup/restore and cold service checks

The fresh evidence namespace is
`bin/tests/plan5-published-0056-2026-10-05`. The source freeze selects 3,151
regular inputs under `src`, `scripts`, `tests`, `migrations`, `areas_mini`, and
`lib` for a private SQL runtime checkout. Every copied input and both copied
server hashes are verified before execution; source copies are checked again
after the selected tests. No checkout `.env`, existing player/account data or
production credentials are consumed.

The existing test owners are selected without modifying their assertions:

```sh
python3 -u -B tmp/plan5/run-published-0056-sql-restore-qualified.py
python3 -u -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

The SQL helper selects exactly
`PersistenceRecoveryIntegration.test_mariadb_full_dump_schema_history_values_and_isolated_service_boot`
and
`PersistenceRecoveryIntegration.test_mysql_full_dump_schema_history_values_and_isolated_service_boot`.
It enables both explicit backup-integration opt-ins before import and refuses
skipped invocation. The flatfile class enables its existing opt-in and supplies
the exact newly linked flat server plus fresh native/artifact/cache paths.
The retained command JSON includes the complete environment and arguments.

Restores need the existing tests' private tmpfs and user/network/PID namespace
isolation. Their Docker launch adds `--cap-add SYS_ADMIN`,
`--security-opt seccomp=unconfined`, and
`--security-opt apparmor=unconfined`; networking remains disabled, source remains
read-only, and only workspace `bin` is writable. The exact outer profiles are
retained in `outer-launch-profiles.json`. Private SQL datadirs use separate Unix
sockets with database TCP disabled. Service processes are separately isolated.

The first SQL and flat launches omitted those permissions. SQL stopped in setup
with two mount-permission errors before database work; flat stopped with the same
setup error after its native fixture compilation. Both failed transcripts and
namespaces are retained. Fresh corrected namespaces keep the tests' isolation,
source guards, filesystem requirements and skip refusal intact.

The SQL checks model synthetic wallet/bank/epic/frag baselines and a native
locker receipt, run full authoritative migration adoption, managed dumps and
restoration, compare original/restored values and receipt bytes, and boot the
actual maintained SQL binary. Post-capture private damage checks include account
ownership disagreement, unwitnessed native revisions, loss of cancelling money
or epic events, bad opening value and altered schema history. These are actual
backup/restore and database qualifier executions on modeled data, not original
gameplay or complete nonempty accounting-root journeys.

The flatfile class uses modeled, inactive native-codec lifecycle history. It
exercises the real managers, two actual cold boots, journal replay/drain, native
authority reconstruction, required-file discovery, loss/corruption refusal and
generation retention. Its second boot checks the exact native UID reservation
and sealed allocator witness advancement. Its explicit evidence flags do not
claim native source capture, lifecycle installation or accounting activation.

The final flatfile method passes with zero failures/errors/skips, 322.492 seconds
in unittest and 322.887852 seconds outer. Its two actual service boots use the
exact flatfile server hash above. Required receipt loss before capture is
refused before boot; two manifest loss/corruption cases and a checksum-valid
corrupt receipt also refuse before boot. The original source and generations
remain unchanged, old inactive receipts remain retained, two retained generations
survive and the unretained generation is pruned. Cold restart advances the sealed
UID pair exactly from `(1000203, 2)` to `(2000203, 3)`.

Both SQL methods pass with zero failures/errors/skips: 683.705 seconds in
unittest, 693.249173 seconds for the retained helper scope, and 799.499366 seconds
for the outer command including private-checkout preparation. Source and restored
engine versions match exactly: MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and
MySQL `8.0.46-0ubuntu0.22.04.4`. The actual cold service boots take 2.366897 and
2.855243 seconds respectively and use the exact SQL server hash above. The helper
retains two generation inventories and two qualified restore receipts, 24 accepted
and 16 deliberately refused database qualifications, and verification that all
3,151 copied source inputs remain unchanged. The flat server copied for fixture
setup is explicitly not executed by this SQL selection.

Across the selected contracts and recovery owners, 74 distinct unittest methods
pass with zero failures/errors/skips. The three errors from the two initial
launcher failures remain separately retained. Four actual maintained service
boots occurred in the final recovery selections; none is a staff/player journey.

The sealed manifest `tmp/plan5/published-0056-evidence.json` has SHA-256
`b3c520dcc7ce0400b0ff166bcdfffb838845670dafeba4031d204b413287daf9`.
It records 9,348 new artifacts, successful preservation of all 52,744 prior
artifacts, and unchanged raw hashes for every original 6,131 regular tracked
input, including all 1,500 native/migration inputs. The recorder verifies source
again after the prior-artifact pass. No earlier sealed namespace is rewritten.
The copied recorders, launch profiles, exact commands, build/dependency proofs,
generation inventories, restore receipts, service logs, failed attempts and final
results are bound by that manifest. The result and remote verification are in
`tmp/plan5/published-0056-delivery.json`.

## Contracts, inventory and remaining release gates

```sh
python3 -u -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
python3 tests/run_integration_matrix.py --list
python3 scripts/validate_economy_accounting.py --release
```

The 71 contract/invariant methods pass with zero failures/errors/skips, 11.541
seconds in unittest and 11.742945 seconds outer. The ordinary validator passes
with 14 fixtures, 887 routes and 2,843 candidate sites, explicitly
`release_ready=False`. The matrix check passes with incomplete coverage and
release BLOCKED. Central inventory/workload validation passes before list
selection; listing does not execute its 92 integration rows. The release
validator returns expected exit 1, `writer has no executable evidence`.

The primary's private cold SHOP source, producer/command installation, guarded
publication/recovery ACK, pending coherent 0057–0060 migration chain and native
NPC ownership remain outside this published candidate. Actual supported writers
on both backends, player journeys, native source/capture authenticity, current
combined independent reconciliation including the child fix, populated upgrade
and full R1–R8 fault/replay/restart, storage-growth, checkpoint, latency, retention
and replica acceptance remain required. Compilation, fixture passes and inventory
registration do not complete those gates.

The primary's local notebook remains authoritative. Its curator handoff should
record this candidate's exact source/build/recovery/manifest/delivery hashes and
the retained launcher failures, while preserving every modeled-data and runtime
scope limit above. `AI_CONTEXT.md` is absent from this independent checkout;
the user confirmed the primary maintains its notebook locally and directed
independent work to continue. Its local location does not block Plan 5 work.
Inactive behavior, wallet-root item exclusions and the declined inactive spell
path remain preserved. No activation, production mutation, audit correction,
deployment, merge or push to `experimental-accounting` occurs.
