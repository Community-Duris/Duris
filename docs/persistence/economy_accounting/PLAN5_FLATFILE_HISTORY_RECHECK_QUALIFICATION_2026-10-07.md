# Plan 5 flatfile history recheck qualification — 2026-10-07

A completed flatfile baseline-history checkpoint previously kept reporting
healthy known-book closure after a witness, reservation shard or common segment
was lost. The independent whole reader refused the same damaged native evidence.
The fixed operator starts another bounded physical traversal after closure,
clears current traversal progress and preserves sticky findings. Closure is
reported when that fresh traversal finishes, rather than cached by later
metadata-only invocations.

This is one independently qualified operator defect. Full Plan 5, R7, R8 and
the combined release remain open.

## Branch, ownership and source

Work remains on local and remote `codex/accounting-plan5`, in
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
The base is `04750f3f2f9744c9a74b4d4c212b2022bd7f87ae`. The result commit and verified
remote SHA are recorded after publication in
`D:/CodexEvidence/accounting-plan5/bin/flatfile-baseline-history-recheck-delivery-01-20261007/delivery.json`.
All seven earlier branch tips remain ancestors; no branch history is rewritten.

Owned files are `scripts/flatfile_baseline_history_audit.py`,
`tests/async/test_flatfile_restore_economic_authority.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report and
`docs/persistence/economy_accounting/PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
The lifecycle suite imports the owned authority test helper and needs no
independent source edit. No native decoder, producer, mutation path, schema,
shared coordinator, registry, activation owner, manifest or shared runner is
changed. No shared storage or producer interface request is required.

Frozen executable source archive SHA256 is
`9b4d82dd8719e06865834c3dc7aa3f216b6f3380c80c51c3d007bca841a50101`. Original red archive is
`f8e169f832fcfc84e2c1e1a4bece0d3c95661b54a3f44701294a1d53c9ba83b4`. The only executable payload changes
between them are the two owned Python files; all other payloads, modes and links
are identical. Both existing Python files retain canonical archive mode `0664`.
Native tree is `4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations tree is
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical sequence62. Publication documentation
is compared separately with the frozen executable source in the delivery receipt.

## Established defect and complete fix

The red probe uses the real `baseline-history` native fixture: three empty
baseline batches followed by a nonempty batch, plus an initialized revision-zero
book. Its active epoch remains zero. Actual native command/plan/witness bytes
produce a clean closed private checkpoint before each fault.

Deleting an earlier witness, one reservation shard or one common segment leaves
heads and indexes unchanged. For each cut, two actual CLI calls return exit0,
`consistent_page=true` and `known_initialized_baseline_books_closed=true`.
The sanitized independent whole audit returns exit1 for the same state.
All reader calls preserve file bytes, modes, link counts, inodes, sizes and
modification timestamps. These are native evidence losses, not a fabricated
successful page response. Red binaries are reused from the preceding qualified
history slice only after comparing their complete native/C++ source inputs.

The fix first checks a closed checkpoint against its original metadata cut.
On success it starts the controls phase, or the roots phase when no books are
required. It resets control flags, reservation totals, bitmaps, root/member
counts, terminal flags, bucket cursors and exhaustion, rotation and current
traversal totals. It retains source/cut identity, original start time, findings,
their cumulative count and truncation state. A context refusal leaves existing
traversal progress intact and records a sticky finding.

Healthy repeated traversals reach closure without duplicated root counts or
bitmap findings. Missing/corrupt witnesses and segments produce semantic record
findings; missing/corrupt control shards refuse the control page. Later restored
fixture bytes cannot erase a previously retained finding. Persisted CLI progress
is resumed through those transitions.

`historical_range_complete` and `known_initialized_baseline_books_closed` describe
the freshly finished traversal. A subsequent successful closed-context invocation
starts physical rechecking and returns both false. The progress and report
format names remain version1. Their exact source binding already requires a
fresh private checkpoint when operator bytes change; no silent upgrade or loss
of the older checkpoint is performed.

This reporting change is the narrow handoff to primary: consumers of the
baseline-history CLI must capture the page with `next_phase=closed` and
`known_initialized_baseline_books_closed=true` to
retain a successful traversal result. Later `phase=closed` calls start another
traversal. The exact revision-density, root-count, membership, terminal and
sticky-finding invariants remain required. Both original native audit suites
exercise these consumer transitions. There are no additional fields or schema
requests.

## Native and disposable-database qualification

The frozen image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The original commands run in an isolated `/workspace`, with no network,
2 CPUs, 4 GiB memory, 3 GiB workspace tmpfs and 2 GiB temporary tmpfs. No
maintained checkout/runtime or production database is mounted. Native fixture
cache is explicitly disabled. Fixture and independent-reader builds retain
strict C++20 warning flags and ASan/UBSan; the native qualifier retains its
original strict listener-free build flags. Exact commands, environment,
binary identities, logs and original source transport are retained in the seal.

| Exact original command | Result |
| --- | --- |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 20 accepted / 367 refused stores, 28 root pages, 29 authority pages, 1,058 metadata and 574 command-envelope comparisons; 30 control cases and 77 history checks; zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py --artifacts /workspace/bin/tests/baseline-history-recheck-receipts` | 131 cases: 9 accepted / 122 refused; 46 lifecycle pages, 35 control cases and 69 history checks; zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py --native-source /workspace --artifacts /workspace/bin/tests/baseline-history-recheck-markers` | 69 cases: 56 structural refusals / 7 readable unqualified / 6 provenance qualified; zero skips |
| `python3 -u -B tests/async/test_economic_sql_canonical_audit.py` | 67 methods, native/source/mobile checks explicitly enabled; zero skips |

The SQL command sets `DURIS_PLAN5_CANONICAL_NATIVE=1`,
`DURIS_PLAN5_CANONICAL_SOURCE=1`, `DURIS_PLAN5_CANONICAL_MOBILE=1` and
`DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/baseline-history-recheck-canonical`.
Fresh private engines are mariadb 10.11.14-MariaDB-0ubuntu0.24.04.1 / mysql 8.0.46-0ubuntu0.22.04.4. Both apply the
canonical migrations through62 and exercise actual SELECT-only connections,
expected denied writes with error1142, imported reservation faults and complete
unchanged database inventories around reader calls. These supplemental SQL
checks use their original modeled capsules and are not genuine producer or
gameplay qualification.

The focused supplement independently runs the same 77/69 history checks using
previously qualified native binaries whose C++ inputs are unchanged. The full
original commands then compile their own fixtures/readers afresh; their actual
ELF hashes are recorded separately. No fresh production server build is claimed:
this fix edits Python and documentation, with all C/C++ inputs unchanged.
The preceding slice's two clean 740-object production builds retain their
original source attribution and do not qualify primary's producer composition.

Post-closure fault detection requires at most
249 additional
pages in the supplied small native fixtures. The seal records each exact count.
This is a component bound, not a release-host or growing-history latency claim.
Native budgets remain 16,384 file reads, 128 MiB bytes, 8,192 decoder entries
and a 30-second cooperative deadline, with a 45-second Python subprocess
timeout and a 2 MiB protected private checkpoint. Existing budget refusal,
cursor preservation, fair rotation, owner lock, atomic replacement and malformed
progress/page checks all pass in the expanded native sets.

Local `git diff --check`, `python -B scripts/validate_economy_accounting.py`,
the writer-coverage contract suite's 55 methods and regression discovery of920
entries pass. `python -B scripts/validate_economy_accounting.py --release`
returns the expected refusal for writers lacking executable evidence. Inventory
coverage and this expected refusal do not establish release completion.

## Evidence, remaining gates and curator packet

All paths below are beneath
`D:/CodexEvidence/accounting-plan5/bin/`:

- `flatfile-baseline-history-recheck-red-01-20261007`: verified original source,
  reused native tools, clean closed checkpoint and all three native defects.
- `flatfile-baseline-history-recheck-green-01-20261007`: retained failed initial
  regression assertion. It incorrectly required refusal for a semantic record
  finding; the corrected assertion requires an inconsistent page. No fault or
  failure evidence is overwritten.
- `flatfile-baseline-history-recheck-green-02-20261007`: exact final executable
  source and passing 77/69 checks with verified reused tools.
- `flatfile-baseline-history-recheck-full-01-20261007`: fresh original native
  builds, all four passing commands, retained fixtures/probes/checkpoints and
  both private database inventories/query plans.
- `flatfile-baseline-history-recheck-gates-01-20261007`: five local gate results.
- `flatfile-baseline-history-recheck-seal-01-20261007/evidence.json`: SHA256
  `77d19c7d4a566f73f8aca2959b33a41c4ae185a90c908787130a3d0616cb8e9d`, binding 24,215 artifacts and
  3,576,289,272 bytes, terminal process/container statuses,
  complete source attribution and all findings.
- `flatfile-baseline-history-recheck-delivery-01-20261007/delivery.json`:
  final commit, canonical publication comparison, old-tip ancestry and remote
  verification after publication.

Latest refreshed primary is `e29b222191475382043001326b39fd877049364b`, native
tree `01291db15446d94f36e066032aa3a1eea28ef354`. Primary has integrated the
baseline-control component. Its private producer cold fault recovery, auction
end-to-end journey and native-v2 current-cut proof remain unqualified. That
source is distinct from this independently tested native tree.

The metadata cut guards normal head/index revisions; it is not a simultaneous
hash of all physical files. Repeated traversals detect subsequent physical loss
when that control or root is visited. A protected quiescent restored image is
needed for a stable snapshot; paged observations do not certify atomic current
holdings. Orphan filenames, unknown-initialization books, full native holdings,
genuine writers/typed receipts/recovery/gameplay, combined backup/restore and
retention, and release-host latency/storage remain open. Full R7/R8 and release
qualification remain false. There are zero selected skips and no blocker to
this solved operator slice.

Primary maintains the shared notebook locally, as the user confirmed. This
owned report, the remote follow-up and sealed delivery receipt are its curator
packet. Applying or acknowledging that packet is not claimed, and no cross-chat
message is sent. Accounting remains inactive; wallet-root exclusions and the
declined inactive spell-path change are preserved. No production data changes,
automatic correction, activation, deployment or PR merge occur.
