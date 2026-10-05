# Plan 5: independent qualification of the source-pin repair

Branch `codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen base `86a22cda00a629ce95dfc2f4ebbe3fbf5646a21b` is an ordinary clean
merge of freshly fetched primary `b67c1fb0defb07cc0a088a47723ec647d85c6db0`
into the completed census result `d2f8fd4ca456fb75fb25c5c45c49420b004b90d1`.
The base was pushed only to the Plan 5 branch and verified remotely before
execution. Delivery supplies the separate result commit and final remote SHA.
The only owned tracked file in this slice is this report. Shared changes are
imported from the primary; none are independently edited.

Native tree remains `ab5e68c90f00268b62c4b811c36b46fcfc4e4db7`, and migrations
remain `1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
All 1,498 native/migration file hashes and both owned census regression inputs
exactly match the [sealed census qualification](PLAN5_ACTIVE_CUSTODY_CENSUS_QUALIFICATION_2026-10-05.md).
The current metadata import does not change those already-tested source bytes.

## Established defect, primary fix and independent verification

The strict provenance test failed on the original combined candidate and again
on frozen 3e1c9c865: five recorded hashes did not match the checkout's raw source
bytes. The primary's [checkout policy repair](SOURCE_PIN_CHECKOUT_POLICY_REPAIR_2026-10-05.md)
establishes Windows CRLF versus canonical Git/Linux LF as the cause. Commit
66560837c changes only those five values in the registry and generated matrix,
and gives all 58 existing pinned paths explicit `text eol=lf` attributes.

Independent checks compare the complete before/after registry and matrix JSON
objects after substituting only the five expected pin values. Both complete
objects then compare equal: source/base history, status, scope, ownership,
routes, evidence, activation refusals and all other metadata are preserved.
All 58 current raw-byte SHA-256 values match their pins and contain no CRLF.
The native/migration trees remain unchanged, as do the owned native probes.
The strict test implementation is unchanged.

The five changed pin paths are economic_command_admission.h, both
flatfile_accounting_coin_transaction files, and both inert_item_stage files.
Exact previous registry/matrix bytes, changed-path list and current input hashes
are retained with this evidence. `git check-attr text eol -- <58 pinned paths>`
returns all 116 expected attributes, with `text: set` and `eol: lf` for every
path. No source content or hash assertion is weakened to make the test pass.

## Executed commands and results

The same pinned tool image is used:
`duris-plan5-origin-sql-tools:local`, immutable ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The checkout is read-only at `/workspace`, with fresh bin evidence output and
the ignored manifest/control directory writable; network is disabled. No
production environment, credentials, database, migration or server is used.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
python3 -u -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
python3 scripts/validate_economy_accounting.py --release
```

| Check | Actual result | Outer seconds |
| --- | --- | ---: |
| Strict writer/audit contracts | 71 tests pass, 0 failures/errors/skips; unittest 10.283 seconds | 10.506702 |
| Normal accounting validation | Exit 0; 14 fixtures, 887 routes, 2,843 sites; release_ready=False | 11.556967 |
| Generated matrix check | Exit 0; 887 rows, 879 function anchors, 2,784 unique sites, 0 unmapped | 12.321994 |
| Release validation | Expected exit 1: writer has no executable evidence | 0.123427 |

The original failing provenance method now passes on the actual combined
checkout, as part of all 71 checks. The earlier frozen failure remains intact
as evidence of the defect. Registry status is still
`source_integrated_unqualified`; coverage is false and release remains BLOCKED.
Expected release refusal is not reported as release success.

## Evidence and remaining gates

Fresh evidence root:
`bin/tests/plan5-source-pin-repair-qualified-2026-10-05`.
It contains the exact driver, state, before registry/matrix, Git changed-file
inventory, effective attributes, source-pin proof, input hashes, commands,
timings and complete logs. The sealed 18-artifact manifest is
`tmp/plan5/source-pin-repair-qualified-evidence.json`, SHA-256
`76ffde863dc8a36a24cc846a1af6cbea982585e8ef2977e7c33d6bb0372e5ec0`.
Its 1,507 recorded inputs remain unchanged through execution. It references
the prior census manifest SHA-256
`6052e243bdd94e8b37ad38d65eadc2197b5e33f51858ee7f56c7a82e0be84a71`.
Both artifact roots are preserved without rewriting their sealed contents.

The source-pin defect is resolved on this combined candidate; no further pin
repair request remains. There is no new shared schema or interface request.
Original command timestamp authentication, coherent 0057/0058/0059 publication,
private shop v7/NPC v11 consumers and codecs, native producer/capture authority,
guarded publication/recovery/ACK, both-backend gameplay, full writer execution,
release-host budgets, SQL retention/replica evidence and R1–R8 remain open.

No unchanged native build or disposable-database recovery suite is repeated in
this metadata-only slice. The census report supplies the exact current native
component tests and both maintained incremental builds; earlier managed SQL
and flatfile restore evidence retains its documented frozen scope. No new
durable producer or full-world qualification is inferred from these contracts.
Accounting stays inactive, wallet-root exclusions and the declined inactive
spell change remain, and no production mutation, autocorrection, deployment,
activation or PR merge occurs. Only the separate Plan 5 branch is pushed.

This is the curator evidence handoff for the primary's locally maintained
shared notebook. Notebook upkeep is not an engineering blocker and no remote
notebook update is claimed. This completed defect qualification leaves the
overall Plan 5/release goal active.
