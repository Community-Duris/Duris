# Plan 5: bind SQL opening witnesses to committed canonical roots

This slice starts at `17b824618cb1ba6c20f8a2feebde8d65d0e559eb` on
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Its result
commit and exact committed inputs are bound by
`tmp/plan5/sql-baseline-root-binding-evidence.json`. Publication uses this lane's
remote branch; the primary owner integrates completed slices.

The refreshed integration remote remains
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b` and all 1,232 native server inputs
remain unchanged, as do all 236 migration inputs. Native canonical base is
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Every SQL integration uses a fresh
private schema through `0056_spell_ward_durability`; 0055 evidence does not
qualify this source or the primary owner's combined candidate.

## Demonstrated defect and independent fix

The SQL opening-origin reader verified an EAB1 witness's own digest and record
grammar without binding the complete witness bytes to the committed root's
canonical intent and plan. A disposable owner changed an opening denomination
and recomputed the witness digest. The actual native owner's retained canonical
root was unchanged, but the independent audit admitted the changed opening.
Both engines reproduced this RED: one test, two engine failures, zero skips,
154.558 seconds. Original source, binaries and logs are preserved.

`economic_sql_audit_origins.py` now reuses the existing independent Python
EAI1/EAP1 readers. It imports no native mutation, producer or coordinator code.
For every opening witness in the same consistent SELECT-only read, it:

- Derives the native operation ID from preparation, baseline domain and batch;
  derives the kind-10 source from preparation, epoch, batch and slot zero.
- Requires the existing SQL root fields and canonical intent/plan metadata to
  agree with that identity, native baseline writer 4, operator actor, version-1
  accounting/policy/compiler, reason 38 and no original operation.
- Reconstructs the 48-byte EBC1 payload from the complete witness's size and
  digest. It verifies the tagged native domain digest against the intent, plan
  and SQL root. This binds all witness bytes, including boundary/coverage,
  native revisions and per-origin source digests.
- Verifies the stored tagged intent digest and raw plan digest, full canonical
  decoding, declared counts and the exact expected baseline plan body: ordinary
  opening effects, balanced opening-equity postings and unchanged item positions.
- Requires the baseline receipt's native command type/schema/payload versions,
  durable book revision and empty result payload, in addition to the existing
  committed status, success result, failure stage and commit-time checks.

Selected-book disagreement refuses capture with `EAB1 committed root mismatch`.
Retained-book disagreement leaves the existing `baseline_source_claim` finding
visible through the full exporter. Stronger selected-book refusal replaces the
previous post-capture diagnosis for a selected root with a mismatched source.
No successful audit is manufactured by changing retained evidence.

Both the per-book and whole-lineage preflight now count EAB1, EAI1 and EAP1
bytes together before fetching capsules. Existing limits remain 100,000 rows
and 32 MiB. These are structural input bounds, not measured workload budgets
or a fair live-history watermark.

Existing audit formats and native schemas remain unchanged. There is no new
shared interface request. All required source fields already exist in the
canonical schema. The missing complete native capture and authority-bound
initialization interfaces remain primary-owner dependencies.

## Qualification and exact source

Environment: `duris-plan5-origin-sql-tools:local`, image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
Ubuntu 24.04, Python 3.12.3, GCC 13.3, PyMySQL 1.0.2-2ubuntu1.1.
MariaDB version is `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL version is
`8.0.46-0ubuntu0.22.04.4`.

The checkout is mounted read-only and `bin/` is a separate writable mount.
Private disposable daemons use Unix sockets with TCP disabled. A clean fixture
environment supplies only private test constants; checkout `.env` is not read.
Audit users have SELECT only and UPDATE is denied with 1142. All 15 captured
authority/evidence tables remain unchanged during each native audit. Fixture
restoration is performed only by the private owner, then native exact replay
verifies the original state.

The existing native fixture's 13 production/test compile inputs and bytes are
unchanged. SQL and client-free modes retain strict C++20 warnings and
ASan/UBSan. SQL binary SHA-256 is
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
client-free binary SHA-256 is
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
Both native books apply, replay and reconcile successfully. Client-free
initialize/apply/reconcile continue to refuse with `ENOTSUP`.

Each engine executes 61 damage cuts: 42 new selected/retained witness, root,
receipt and rehashed capsule checks, plus the previous 19 source/book cuts.
Twenty-four cuts refuse capture; 37 produce exact expected diagnostics.
Canonical source/root foreign-key and operation-claim uniqueness refusals remain
1452 and 1062. The five existing damaged-import cases use only the private
owner's foreign-key switch; foreign keys are enabled for every audit and native
replay, and CHECK/UNIQUE constraints remain enforced.

The intact fixture still reports one evidence-loss, two missing-native-holding
and two missing-native-item findings. It supplies no full world/native capture,
so these limits remain visible and accounting stays inactive.

| Exact command in the image | Result and evidence |
| --- | --- |
| `DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v` | Pass: one guarded test, both engines, zero skips, 201.038 seconds; 61 cuts and two constraints each, native replay and full sibling exporter/CLI matrix. `tmp/plan5/root-binding-green2.log`, `bin/tests/plan5-baseline-root-binding/{mariadb,mysql}.log`. |
| `python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v` from `tests/async` | 118 collected: 113 passes and five explicit skips, 16.800 seconds. `tmp/plan5/root-binding-components.log`. |
| `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 python3 -m unittest test_economic_sql_audit_origins.NativeSQLOriginTests -v` | Two native-origin passes, zero skips, 227.271 seconds. Both canonical engines exercise four native EAB1 batches across two epochs, three captures, three refusals and six rollbacks each. `tmp/plan5/root-binding-origins.log`. |
| `python tests/async/test_economic_sql_audit_origins.py -v` on Windows | 18 passes and two explicit native-origin skips; 0.008 seconds. `tmp/plan5/root-binding-host.log`. The native pair above executes those skipped cases. |
| Six changed Python files: `python -m py_compile`; `git diff --check` and staged checks | Pass, recorded in `tmp/plan5/root-binding-final-checks.log`. |

The exact syntax-check inputs are:

```sh
python -m py_compile scripts/economic_sql_audit_origins.py scripts/economic_sql_audit_snapshot.py tests/async/test_economic_sql_audit_origins.py tests/async/run_economic_sql_audit_snapshot_mysql.py tests/async/test_native_sql_baseline_audit.py tests/async/run_native_sql_baseline_audit.py
```

The first fixed run passed all 61 cuts and both constraints on both engines,
then the broader synthetic unsigned-revision test failed: it rewrote its EAB1
without rewriting its canonical root. That failed run took 273.435 seconds and
is preserved with frozen inputs. Positive synthetic revision fixtures now
publish a consistent synthetic root through their existing private test owner;
all corruption assertions remain intact. Explicit old-column insertion keeps
ordinary sibling fixtures unchanged when adding canonical fields to the
synthetic schema. No audit implementation repairs a fixture or authority.

Unit opt-in skips are the two native-origin tests, two near-limit budget cases
and one native-stake SQL case. The native-origin pair is explicitly qualified
above; budget and stake cases are not counted as passes or rerun here. Core
inputs of the unit/native-origin runs remain unchanged after the sibling-only
fixture correction. The evidence manifest verifies that every preceding slice
artifact and unchanged helper remains preserved.

## Ownership and remaining gates

Seven owned files: `scripts/economic_sql_audit_origins.py`,
`scripts/economic_sql_audit_snapshot.py`,
`tests/async/test_economic_sql_audit_origins.py`,
`tests/async/run_economic_sql_audit_snapshot_mysql.py`,
`tests/async/run_native_sql_baseline_audit.py`,
`tests/async/test_native_sql_baseline_audit.py`, and this report. Existing
independent canonical decoders, the reconciler, native fixture, native owners,
accounting contracts, migrations, registry/matrix and activation owner are
unchanged. New artifact directories preserve earlier source-specific evidence.

The existing primary-owner release-registration request remains: explicitly run
`NativeBaselineAuditTests.test_native_baseline_claims_both_canonical_engines`
with `DURIS_RUN_NATIVE_BASELINE_AUDIT=1`, both engines and zero skips, and register
the native-origin pair with its explicit opt-in. Shared fields are `path`,
`arguments`, `environment`, `required_cases`, `provider`, `engines` and timeout;
consumers are the integration runner and combined release report. Central
registration remains outside this lane's edits.

Complete native capture, namespace authority, all executable writer/player
journeys and fault/restart/lost-reply matrices, populated upgrades, retention/
erasure/export policy, live sweeps and measured mixed-workload budgets remain
required. Independent normalized SQL baseline detail and full restore/clone
qualification remain separate evidence requirements; this slice binds opening
witnesses to their canonical roots and does not certify the complete candidate.
Prior backup/restore evidence remains pinned to its source and is not rerun or
transferred to a combined candidate by this audit slice.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable; the earlier request is unanswered. Bounded accessible Pages searches
returned no target and do not establish absence. This report and protected
manifest are a curator-ready handoff, not a claimed notebook update. Wallet-root
item exclusions, declined inactive spell behavior and active blackjack refusal
are preserved. No activation, production access, merge, deployment or audit
auto-correction occurred.
