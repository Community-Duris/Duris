# Plan 5 independent original-plan SQL operator check

The new SELECT-only `scripts/economic_sql_canonical_audit.py` authenticates
retained original EAI1/EAP1 capsules and their SQL detail rows across every book
in one consistent read view. Six schema-valid corrupted SQL cuts remain clean
in the existing version-1 projection reader; this command refuses all six.
This is a separate operator qualification slice. Saved projections still omit
original capsules, and full command/receipt authentication remains open.

## Delivery and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch: `codex/accounting-plan5-canonical-plans`, published separately on
  `Community-Duris/Duris`; never pushed to `experimental-accounting` here.
- Refreshed primary/base: `92784e323186f62cf1067b6f8a27f3fc70b702d5`.
- Native tree: `cddb88bb4db8c826eb2519f6d85381a7a10fd7b6`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`.
- Both private databases applied the authoritative migration manifest through
  `56 / 0056_spell_ward_durability`. Earlier 0055 evidence is not substituted.
- Result commit is recorded in the post-commit delivery receipt
  `tmp/plan5/canonical-plans-delivery.json` and the delivery message.

Owned files are the new command, its new focused test
`tests/async/test_economic_sql_canonical_audit.py`, the usage addition in
`AUDIT_OPERATIONS.md`, and this report. Native code, migrations, coordinator,
accounting contracts, producer integration, registry/matrix and activation
owner are untouched. Existing inactive behavior, wallet-root item exclusions
and the declined inactive spell-path change are preserved.

No storage/schema/native interface change is requested. Shared registration
requests appear below. The primary maintains its notebook locally; this report
is the evidence/curator handoff, and notebook availability is not a blocker.

## Established defect and complete scope of this slice

The ordinary SQL exporter omits original EAI1/EAP1 capsules and several original
root facts. Its projection reader can check child derivation and balanced
detail relationships without proving that those relationships were compiled
in the original immutable plan. The native fixture supplies a bank transfer,
two nested children and one player-to-player UID event. On both engines:

1. Rewriting the first child's domain/derived ID and the descendant's derived
   ID coherently passes the old reader while leaving the original plan intact.
2. Changing a posting's child index to another valid existing child passes the
   old reader while disagreeing with the original posting.
3. Changing an item reference's child index likewise passes the old reader.

All six cuts retain canonical 0056 constraints and actual native-encoded root
bytes. The comparison's opening balances and UID origin are explicitly modeled;
they do not prove native source capture or a producer journey.

The new command consumes the existing independent Python restore decoder. It
does not import native mutation codecs or call a writer. It validates original
metadata, intent/domain/plan digests, six native counts, canonical account
effects, posting line/event indices, original child facts, item references and
joined legacy custody facts. Its standalone boundary also refuses orphan
ordinary details and details attached to rejected roots. Baseline interpretation
uses the existing independent EAB1 checks; this fixture contains no baseline
root, so a new nonempty baseline qualification is not claimed here.

The database-wide scope includes inactive and unselected epochs/lineages. A
read-only repeatable-read transaction owns every query and always rolls back;
the command requires all fourteen source tables to be InnoDB. It refuses above
100,000 retained roots or 32 MiB of original intent/plan/witness bytes before
decoding. Each SQL result is limited server-side to 100,001 rows and streamed
client-side under the same row/byte limits. Missing or oversized evidence is
never truncated into a successful report. Larger histories need a separately
reviewed partition/sweep method.

Success emits only small scope/count/coverage JSON. Failure exits 2 with no
stdout and a diagnostic code on stderr. Neither path prints original capsules,
command bodies, aliases or passwords. Successful JSON explicitly sets complete
command/receipt authentication, source capture and release qualification false.
This command and the partial exporter are separate read views; their outputs
do not establish one combined cut without the release quiescence procedure.

## Exact source and runtime

The initial freeze records 6,142 regular tracked inputs, including all 1,506
native/migration inputs, and the inherited 62,092 artifact hashes. Final
preservation and owned-file hashes are sealed in
`tmp/plan5/canonical-plans-evidence.json`.

Tested implementation SHA-256:

- Command: `765b0d81cfcfba5ae0e5a41e88a672625f178bfeeb1749396e41946ec29dfd2a`.
- Focused test: `c967b4553c0a72bb7aee06c4b847f33d0b2c06b3165633eca49f954a5348f6cf`.
- Unchanged independent decoder:
  `dcf870dbf4abf5c2dc762cdab0ac73468945e590d0ba51e3f8c6d3315a60f5cb`.

Every final unit/native launch records source hashes in `*-inputs.json` and
checks them again on completion. The execution environment is the immutable
Docker image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Ubuntu 24.04.4, GCC 13.3, Python 3.12.3 and OpenSSL 3.0.13.
Containers use `--rm --network none`, a read-only repository mount and a writable
`bin` mount. Private daemons have fresh datadirs, task-owned Unix sockets and
TCP disabled. No project `.env` or production data is used.

## Commands, results and evidence

Evidence root: `bin/tests/plan5-canonical-plans-2026-10-05/`.
Exact launch commands, environment and durations are in `*-command.json`;
complete test output is retained in matching logs.

```sh
python3 -u -B -m unittest -v \
  test_economic_sql_canonical_audit.CanonicalAuditTests

DURIS_PLAN5_CANONICAL_NATIVE=1 \
DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/plan5-canonical-plans-2026-10-05/native-5 \
DURIS_PLAN5_CANONICAL_PROBE_REUSE=/workspace/bin/tests/plan5-canonical-plans-2026-10-05/native-4 \
python3 -u -B -m unittest -v \
  test_economic_sql_canonical_audit.NativeCanonicalAuditTests

python3 -u -B -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 scripts/validate_economy_accounting.py
python3 scripts/validate_economy_accounting.py --release
python3 tests/run_integration_matrix.py --list
```

- Final unit namespace `unit-5`: **10 methods pass, zero skips**, 0.019 s
  unittest / 0.293386864 s recorded subprocess. Covers read-only SQL admission,
  source engines, root/capsule/result budgets, malformed scalars, rejected/orphan
  detail admission, rollback/close on refusal and success scope disclosures.
- Final native namespace `native-5`: **one method passes, zero skips**,
  132.182 s unittest / 132.470719682 s subprocess. Both engines execute inside
  this method and are mandatory:
  MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1`,
  MySQL `8.0.46-0ubuntu0.22.04.4`.
- Two native configurations execute the same native-generated EAI1/EAP1 plan
  and fourteen child-contract assertions total under ASan/UBSan. Compile/run
  exits are zero, compiler and sanitizer logs empty. `native-4` freshly compiled
  SQL and flatfile probes in 34.505571487 / 30.616483977 s. `native-5` verifies
  original probe bytes, every native link input, all compiler flags and the
  original binary hash before reusing and executing each probe. This is
  verified component reuse, not a fresh maintained-server build.
- Both probe binary hashes:
  `715824ba633ebc56a361ecf861f691421de02bad75fdaf89c088fe3b074cdf43`.
  Complete compile argv is in `native-5/native-builds.json`; nine repository
  translation units include the newly extracted `economic_source_event.c`.
- **38 read-only API captures and 38 CLI executions**, each interface with
  20 valid cuts accepted and 18 corrupted cuts refused. All captured tables
  are unchanged by the reader; each API capture rolls back and closes once.
  Original schema constraints remain installed. `native-5/results.json`
  records every query/CLI command, engine version, exit and preservation check.
- Per engine the nine refusals are coherent child rewrites, posting child
  change, item child change, custody before-owner change, root actor change,
  plan-digest damage, valid-length intent-header damage, rejected-root details
  and orphan account detail. Every controlled fault is repaired only by the
  disposable fixture owner, followed by another valid audit. Orphan creation
  disables foreign-key checks only in that owner session for the insertion,
  then restores them immediately. The reader has SELECT only and rejects an
  attempted UPDATE with SQL error 1142 on both engines.
- **Six false-clean old projection cuts** and their zero-finding old reports
  are retained as `native-5/{engine}-{fault}-old-{clean,report}.json`.
- **Six rejected schema corruptions**: both engines enforce posting event/line
  equality, item event/line equality and minimum intent length. They return
  MariaDB 4025 / MySQL 3819 without changing rows. These are enforced schema
  protections, not audit defects or bypassed tests.
- `contracts-final`: **71 methods pass, zero skips**, 11.336 s in the earlier
  contract run; final subprocess 11.768771040 s. Normal validator exits 0.
  **82 distinct selected methods pass in this slice**, zero skips.
- Release validator exits **1**, `writer has no executable evidence`.
- Central inventory listing exits **1**, solely because the new test file is
  unclassified. This is a primary-owned registration handoff, not waived.
- `git diff --check` passes. No C/C++ source is edited by this slice; no new
  maintained `make -C src` pass is claimed.

Four earlier failed native namespaces are preserved. `native-1`/`native-2`
failed the modeled old-reader comparison because its opening UID origin had
been replaced by the empty exported-origin array. `native-3` encountered the
enforced posting-index CHECK; `native-4` encountered the enforced intent-length
CHECK. The final fixture preserves its explicit origin and uses schema-valid
disagreements. No constraint or production semantics were weakened. The first
attempt also preceded the final preflight checks and is excluded from coherent
qualification. Every failure log, datadir and probe artifact is retained.

## Narrow primary/curator handoff and remaining gates

Register these exact class selectors through the shared curator/coordinator
workflow; do not change accounting contracts or schemas for this slice:

- `test_economic_sql_canonical_audit.CanonicalAuditTests`: ten pure methods,
  required zero skips.
- `test_economic_sql_canonical_audit.NativeCanonicalAuditTests`: one native
  method, both MariaDB and MySQL mandatory, explicit
  `DURIS_PLAN5_CANONICAL_NATIVE=1`, a fresh artifact directory below `bin`,
  no default skip accepted. Omit probe reuse for a fresh run, or supply only a
  fully verified source/flags/binary provenance. Allow 900 s outer timeout.

The shared inventory at the frozen base has 915 owners/95 rows. This slice
adds eleven methods and requires the corresponding class-specific matrix
registration by its owner. Source/registry pins for the new operator command
must be maintained there if it is added to the qualification closure. Preserve
all existing rows and engine policy.

The curator should record this as a SQL operator component qualification with
original-plan checks, six demonstrated projection limitations, explicit saved
projection/command/source/release gaps and preserved failure attempts. The
primary remains responsible for integrating and publishing a tested combined
candidate. This report is not evidence that the notebook has been updated on
the primary's separate system.

Saved JSON canonical authentication, complete original CCM1 command/child
receipt authentication, the baseline original accepted-timestamp interface,
resumable historical sweeps, current combined service builds/recovery,
producer installation, actual writer/player journeys, major-plan execution
and full R1–R8 acceptance remain open. Existing 0056 native/recovery evidence
for the earlier `5ca91c7f...` tree does not qualify the current `cddb88bb...`
combined tree. This SQL command supplies no flatfile operator qualification.
No accounting activation, PR merge, deployment, production mutation or audit
auto-correction was performed.
