# Plan 5 independent dual-version baseline readers

The independent Python and standalone restore consumers now accept exact EAB1
and EAB2 layouts and reconstruct original equipment-bearing EAP1 snapshots.
Historical EAB1 equipment loss still refuses an original root mismatch. No
history is inferred, relabeled, resealed or corrected. Native EAB2 and schema
integration remain primary owned and are not qualified by these reference cases.

## Source, ownership and delivery

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `310ad8391f37dab602afbebd109272431e9199e0`, the published compiler-version
  erratum to build report `1ad3f71ec0c70ed842f333d2101cfb35baccff09`.
- Consumed primary: `7cd9b42ee09b080c0b438eec83741c2828610447`, preserving merge
  `5284d155bdbc92010ddc923ce6d6ced5b4b33600`. The native and migration trees did
  not change during the owned reader work or any final checks.
- Native tree: `ddfebad0ee6e2bdc6a0854fbe98f0e5cf9860e56`.
- Migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, through public0056.
- Result SHA, exact owned Git blobs, verified remote and source state:
  `tmp/plan5/eab2-reader-delivery.json`.
- Exact evidence manifest: `tmp/plan5/eab2-reader-evidence.json`; retained logs,
  executable/source preimages, capsules, database generations and recipes:
  `bin/tests/p5-eab2-independent-20261005/`.

Owned files are this report, `scripts/economic_sql_audit_origins.py`,
`scripts/economic_restore_evidence.py`,
`scripts/qualify_flatfile_economic_baseline.h`,
`scripts/qualify_flatfile_economic_lifecycle.h`,
`tests/async/test_economic_sql_audit_origins.py` and
`tests/async/test_economic_restore_mobile_grammar.py`. No shared contract,
producer, coordinator, writer registry/matrix, migration or activation file was
independently changed. All prior branch work remains in this branch's ancestry.

## Established defects and complete owned repair

The [primary's reviewed EAB2 interface](BASELINE_EQUIPMENT_V2_SOURCE_HANDOFF_2026-10-05.md)
fixes original EAB1 equipment loss. The old independent Python reader rejects
valid v2 records/maxima and reconstructs every baseline snapshot with zero
equipment. Its SQL witness-version equality also accepts `True` and `1.0` as1.
The retained red run records eight selected methods, two failures and12 errors;
two count-corruption errors were in the synthetic connection builder itself.
That builder now tolerates malformed wire counts so the actual reader receives
the malformed evidence and proves refusal/rollback. These harness errors are
not counted as production-reader defects.

Compiling the original standalone headers against the same reference harness
rejects valid EAB1 native-mobile custody, valid EAB2 custody and a valid EAB2
empty lifecycle. Historical empty EAB1 lifecycle still passes. The former forest
reader capped owners at11, and both consumers required EAB1/88-byte items.
Original headers, unchanged authority header, harness, executable and exact
three-refusal/one-accept receipts are retained under `old-components/`.

The complete owned repair uses explicit version selection by matching magic
and numeric version, checks count bounds and exact length before decoding rows,
retains the 872144-byte v1 maximum and accepts the 920144-byte v2 maximum.
The v2 position validates both reserved regions and the existing owner/state,
equipment and full-forest rules. Python exports `equipment_slot` for v2 item
origins; historical v1 dictionaries retain their original shape and implicit
zero-slot interpretation. SQL witness_version requires an actual integer and
agreement with the decoded original version.

Original EBC1 reference, EAI1/EAP1 identity, source identity, command binding and
whole-witness digest rules stay. Snapshot reconstruction copies the full64-byte
UID/position portion for v2 and the original56 bytes plus zero equipment for v1.
Both SQL and standalone reservation reconstruction use the selected88/96-byte
stride. Standalone native-mobile custody follows original owner12 lifetime,
zero-context, active/quarantined and slot0–43 rules; player uint16 slots retain
existing accounting grammar, without a physical MAX_WEAR claim.

The SQL restore consumer delegates to the independent dual-version parser and
uses its bounded maximum. The lifecycle reader accepts exact1/2 and continues
to require an empty item book and its original wallet/bank scope. No mutation,
native codec, recovery or activation API was added to either audit reader.

## Exact checks and evidence boundaries

All Linux checks used immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, read-only checkout and writable ignored `bin/`. Runtime is
GCC13.3.0, Python3.12.3 and GNU Make4.3. Source maps and executed preimages are
retained for each run; final tracked source inputs remain unchanged. C++ checks
retain `-Wall -Wextra -Wpedantic -Werror`, with ASan/UBSan where selected.

| Check | Result | Process seconds | Scope |
| --- | --- | ---: | --- |
| Reference and historical reader methods | 41 pass, zero skips | 23.001 | Includes8 new Python methods and2 explicitly selected C++ methods |
| Independent C++ reference cuts | 18 accepts,46 refusals | Included above | 64 immutable cuts: grammar, slots, reserved bytes, version/length/count mismatches, rehashed root disagreement, resealed reservation mismatch, both maxima and empty lifecycle |
| Native grammar | 1 method passes, zero skips | 83.299 | Fresh SQL/flat policy probes;82 cases each,32 native accepts each, exact independent agreement |
| Native canonical SQL audit | 1 method passes, zero skips | 239.402 | Fresh canonical0056 on MariaDB10.11.14 and MySQL8.0.46;19 API/CLI cuts per engine,10 accepts/9 refusals each |
| Native opening-origin SQL audit | 2 methods pass, zero skips | 212.263 | Four native EAB1 witnesses across two epochs;3 captures/3 refusals and6 rollbacks per engine, source/schema0056, inactive |
| Full existing flatfile authority/restore script | Exit0 | 351.539 | 18 positive stores and317 corruptions refused; the suite explicitly distinguishes legacy readability from qualification |
| Historical reference comparison | Pass | Separate receipt | Three old/new EAB1 fixture dictionaries byte-identical, including witnesses, original intents/plans and every digest |
| Formatting | Exit0 | Separate receipt | Changed-line clang18.1.8 formatting, then repository whole-file checks for both touched headers |

The default canonical SQL test additionally retains24 damaged saved projections,
72 full reconciliation reports and216 API/CLI operator views each. Limits0/1/100
do not hide global exceptions; evidence and aliases remain protected. These are
native-capsule and modeled-authority component checks, not gameplay capture.
Both private SQL engines enforce canonical schema checks. Native rich baseline
bytes are generated by the production baseline/store fixture and compared with
committed SQL projections; no v2 schema constraint was weakened for a test.

The flatfile script exercises its independent sanitized reader, operator audit,
preflight and post-replay qualifier on every cut, asserting unchanged accounting
bytes/metadata. Complete executables and final isolated state are retained;
a preservation-only wrapper copies temporary directories before normal cleanup.
Native fixture artifacts are identified by the exact printed/tested binary hash.
These fixture/recovery checks do not replace real-player journeys or qualified
backup generations. The larger v2 maxima are reference capsules, not measured
production storage, latency or retention workloads.

Exact recipes, with artifact gates set by `tmp/plan5/run-eab2-checks.py`:

```sh
python3 -u -B tmp/plan5/run-eab2-checks.py green-final
python3 -u -B tmp/plan5/run-eab2-checks.py grammar-native-v1
python3 -u -B tmp/plan5/run-eab2-checks.py sql-native-v1
python3 -u -B tmp/plan5/run-eab2-checks.py origins-native-v1
python3 -u -B tmp/plan5/run-eab2-checks.py flat-native-v1
python3 -u -B tmp/plan5/prove-old-eab2-components.py
python -B tmp/plan5/check-eab1-reference-bytes.py
```

The final selected unittest command covers `BaselineVersionTests`,
`ItemRevisionTests`, `OriginTests`, `MobilePositionTests` and
`IndependentBaselineVersionTests`. Native selectors are
`DURIS_PLAN5_BASELINE_VERSION_NATIVE=1`, `DURIS_PLAN5_MOBILE_GRAMMAR_NATIVE=1`,
`DURIS_PLAN5_CANONICAL_NATIVE=1` and
`DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1`, with per-run ignored artifact paths.
The origin preservation wrapper relocates the requested fixture destination
and retains the native builder's content-verified artifact; it does not change
fixture or audit logic. Full generated compiler commands, native build receipts,
SQL commands, engine versions, captured source inputs and results are retained.

## Narrow primary interface/registration handoff and remaining gates

The actual declaration of the accepted native explicit-v1 preparation selector
is still private. Publish its exact identifier/type/default, supported1/2 values
and refusal behavior with the accepted header. Consumer
`native_source()` in `test_economic_restore_mobile_grammar.py` currently makes
the original280-byte EAB1 fixture. Once newly prepared native input defaults2,
that historical case must select1 through the actual published API; it must not
relabel new bytes as old history. Add separate native v2 roundtrip/equipment and
original-root fixtures against the integrated source. No selector identifier
was guessed or independently added here.

Primary must complete the additive guarded successor to immutable0032, exact
runtime/history metadata and measured migration fingerprints, native codec/
transaction/activation version agreement, and both-engine fresh/populated
upgrade/backup closure. Public0056 still emits/stores v1; private0057–0060 and
synthetic v2 records do not qualify that combination. Original equipped EAB1
refusal stays until authentic compatible evidence exists.

Primary owns central registration of the8 new pure methods and2 mandatory C++
reference methods with their exact gate/recipe, plus subsequent native-v2 SQL/
flat and recovery recipes. Existing native-v1 checks and historical expected
bytes remain. The [current maintained-build handoff](PLAN5_7CD9_MAINTAINED_BUILD_HANDOFF_2026-10-05.md)
records a successful flatfile server build and the reproduced primary-owned SQL
SHOP shadowing error; this reader change does not repair that producer.

There are no selected skips or notebook blockers. Full writer execution,
combined server/gameplay/fault journeys, both-backend migration and genuine
backup/restore/retention workloads, and full R1–R8/release remain open. Accounting
stays inactive. No activation, production mutation, audit correction, merge or
deployment occurred. Wallet-root item exclusions and the declined inactive
spell-path change remain.

This report is a curator handoff for the primary's locally maintained notebook.
Import this completed slice from `codex/accounting-plan5`, preserve every exact
source/evidence boundary, and publish qualification only for the tested combined
candidate. Notebook locality does not block the remaining owned work.
