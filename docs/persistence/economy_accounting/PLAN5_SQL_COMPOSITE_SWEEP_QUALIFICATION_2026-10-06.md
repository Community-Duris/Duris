# Independent composite-key SQL audit sweep qualification

Local and remote branch `codex/accounting-plan5`; separate worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `9328c5c3f6974a92f02c9ec23ab6f53fe92dafd9`. The result commit and exact remote tip are bound after
publication by `composite-sweep-delivery-01-20261006/delivery.json`.
The refreshed primary is `eefb96d1b7f8a7184a765b72dcd15b2b00e11781`, native tree
`ebe71d02025fbad7864e74113ef7d2741da3597b`. Its published auction source-claim
qualification and latest requirement/checkpoint updates were read. That newer
shared producer source is not imported or qualified by this independent slice;
the primary owns its combined integration. The older Plan5 fixes and seven
previous branch tips remain ancestors on the same publication branch.

## Established gap and owned fix

The seven-source operation-ID sweep could not schedule a standalone baseline
control, or a reservation whose witness/root/source claim no longer survived.
Controls and reservations use composite primary keys rather than an
operation-leading enumeration index. The frozen base with five new regression
methods executes five methods, zero skips and six missing-API errors. The
final native SQL fixture separately demonstrates that an unchanged root-only
page passes while malformed standalone controls and imported orphan reservations
exist. It then observes the specific findings in the new namespace pages.

The existing independent operator gains explicit `--all-namespaces`, requiring
`--progress-path`. One invocation checks one bounded page; the fixed order is
roots, controls, reservations, then roots again. It delegates roots to the
existing original-byte interpreter and uses bounded expanded lexicographic
PRIMARY seeks for the other two namespaces. Each namespace has its own pinned
ceiling and starts again from its lowest key after finishing that range.
Delayed lower commits are eligible on the following pass, and large root
histories cannot starve the other namespaces. No new migration/index is needed.

Control checks retain valid empty revision0 staged books. They check opening
authority, creator/lifecycle identity and bounded first/terminal/overrun witness
references. Reservation checks require valid identity and attached witness,
control, root and lifecycle evidence. Specific findings are
`restore_economic_baseline_book_mismatch` and
`restore_economic_baseline_reservation_mismatch`. Original capsule membership
and interior book continuity stay in the per-root interpreter; these pages do
not decode every complete capsule again for each reserved identity.

The private v2 checkpoint contains `format`, `source_digest`, `next_namespace`
and three `namespaces` leaves. The root leaf preserves v1 progress exactly;
controls use64 hex digits for lineage/epoch and reservations use82 for those
identities plus unsigned kind/id in fixed-width big-endian hex. SQL ordering
and private scheduling ordering agree through UINT64_MAX. Zero scheduling
keys remain enumerable. Incompatible signed/noninteger reservation-key storage
refuses before capturing a range, avoiding HEX(-1)/UINT64_MAX aliases.

The default root-only mode and v1 format remain compatible. Explicit v1-to-v2
upgrade preserves all root counters, cursor, ceiling and sticky findings;
root-only mode then refuses the v2 file. Protected locking, JSON bounds,
fsync/atomic replace and directory fsync remain. Refused budgets leave the
checkpoint and next namespace unchanged. Findings retain at most32 observations
per namespace and sticky truncation. Routine reports exclude private keys and
exit1 after any retained finding, including on a subsequent clean root page.

All reads occur in read-only repeatable-read transactions and end with rollback
and cursor close. All complete/native/receipt/allocation/orphan/release coverage
flags remain false. The reported completed-sweep count is the minimum of the
three independent historical pass counts; it is not one consistent authority
cut or a commit watermark. Backlog is an inexact lower bound for the selected
page. No audit finding is auto-corrected.

## Exact source and executed checks

- `python3 -B tests/async/test_economic_sql_canonical_audit.py`: exit0, 64 methods, zero skips, 178.190380s; environment `{"DURIS_PLAN5_CANONICAL_ARTIFACTS": "/workspace/bin/tests/resumable-baseline-canonical", "DURIS_PLAN5_CANONICAL_MOBILE": "1", "DURIS_PLAN5_CANONICAL_NATIVE": "1", "DURIS_PLAN5_CANONICAL_SOURCE": "1"}`; log SHA256 `77a898da671aafbf2f6f7b9dfec3ebc3d7df7721fb6c6ca1e340671d1e2c1835`.
- `python3 -B tests/async/test_economic_sql_audit_origins.py`: exit0, 52 methods, zero skips, 93.147828s; environment `{"DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION": "1"}`; log SHA256 `6770bf7fa53646a71389b63f9a1dfdf54d5f4ed3ce297aff832be0b6d2118600`.
- `python3 -B tests/async/test_restore_economic_coin_effects.py`: exit0, 1 methods, zero skips, 286.648680s; environment `{"DURIS_PLAN5_CANONICAL_EVIDENCE": "1", "DURIS_RUN_RESTORE_COIN_INTEGRATION": "1"}`; log SHA256 `74b7f96f5df07141514354a4b516cab2101f263e3b7c2b819bafdc568e215778`.

All **117 Linux methods pass, zero skips**:64 canonical,52 origin and one
complete original coin/restore method. Seven new pure methods protect fairness,
durable resume and upgrade, delayed lower keys, sticky findings, interrupted
checkpoint replacement, mode/identity/type refusals, bounds and unsigned primary
key order. The one new native method runs on both fresh disposable engines.
All56 original canonical method ASTs and all five red-method ASTs are preserved.
Original origin/captured-source and complete coin/restore checks remain unchanged.

Final source archive `8c8627ec8b4a3e5dd557237c936c7c15bdeaf8cc9480d3162a73dfea0fbeb1ed` contains
6371 regular source files, 4 original repository links,
and three owned overlays using original Git modes. Every regular source file
is hashed before and after execution. Tested overlay hashes:

- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`: SHA256 `97d0db8e2edcd6ae2c9a005004dcfd021f4a91ab64d5252977854875dcb741ef`.
- `scripts/economic_sql_canonical_audit.py`: SHA256 `0583b42e6a233735ad296f5ce728c13c6977d7006f457ebc52e1121c8344bc41`.
- `tests/async/test_economic_sql_canonical_audit.py`: SHA256 `8a246165c57ccf9e9930b4fd342e325915340ab76ec16ff222017a03e12f9d0e`.

Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`, migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`; fresh canonical migration0062.
Pinned image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/Python3.12, network disabled, no ports or production-data mounts,
two CPUs,4 GiB memory,3 GiB source/fixture tmpfs and2 GiB temporary tmpfs.
Original cache-disabled native SQL and flatfile probes compile afresh with
C++20, Werror, O1/debug, ASan/UBSan and non-PIE flags; no probe reuse. Original
full restore/native timeout and query/page budgets are unchanged.

- mariadb `10.11.14-MariaDB-0ubuntu0.24.04.1`: 35 durable namespace pages, four actual CLI pages, one legacy root-only comparison, two deliberate budget refusals, 28 actual EXPLAIN plans; final per-namespace completed passes `{"controls": 2, "reservations": 3, "roots": 12}`; max page queries 40, max returned text 4850 bytes, max page seconds 0.017469.
- mysql `8.0.46-0ubuntu0.22.04.4`: 35 durable namespace pages, four actual CLI pages, one legacy root-only comparison, two deliberate budget refusals, 28 actual EXPLAIN plans; final per-namespace completed passes `{"controls": 2, "reservations": 3, "roots": 12}`; max page queries 40, max returned text 4850 bytes, max page seconds 0.021553.

Both engines grant the reader SELECT only; UPDATE refuses with1142. The
normal reservation foreign key refuses the orphan insertion with1452; the
orphan fixture explicitly disables its private session FK checks to model
imported/lost-parent corruption. Capsules and controls are explicit reference
models, not genuine producer openings or gameplay. The native SQL assertions
exercise original maintained SELECT predicates, key ordering and transaction
behavior. Every app-table inventory remains identical before/after each reader,
CLI invocation and budget refusal. Actual PRIMARY ceiling/range plans have no
filesort. The original native codec/restore evidence has separate scope.

The new case retains 70 namespace pages, eight actual CLI pages,
four deliberate budget refusals, two legacy comparison pages and
56 actual EXPLAIN plans across both engines. A delayed control and
reservation commit behind their persisted cursors are found on later passes;
higher control tails do not prevent completion. Valid empty controls remain
clean. A clean root page retains earlier namespace findings and status1.

Windows selected60 canonical plus52 origin methods pass with four skips:
one POSIX-lock method and three opt-in native origin methods. The other108
selected methods pass. These Windows skips are all executed in the final Linux
run. Normal accounting validator and whitespace checks exit0; release validator
exits1 with `writer has no executable evidence`.

## Preserved attempts, evidence and handoff

The initial local draft had one Python continuation syntax error, corrected
before native freezing. The first native green freeze executes63 methods with
one error and zero skips: the added both-engine case passes, but the subsequent
original resumable fixture's MySQL bootstrap exhausts the2 GiB fixture tmpfs.
Its daemon log explicitly records error28, `No space left on device`; a read-only
diagnostic observes2.0G used,13M available and100% use. The observer stops before
the origin and full restore commands. Those are unexecuted in that failed
attempt, not passing or skipped results. The complete final117-method run uses
3 GiB fixture storage while retaining4 GiB memory, all original timeouts and
read budgets. It also includes the unsigned-key storage guard and its regression.

Protected evidence root `D:/CodexEvidence/accounting-plan5/bin/`:

- `composite-sweep-red-01-20261006/`: five-method missing-capability proof.
- `composite-sweep-green-01-20261006/`: preserved storage failure and initial native observations.
- `composite-sweep-green-02-20261006/`: final archive, transport, commands, logs and retained native artifacts.
- `composite-sweep-component-gates-02-20261006/`: exact-source Windows/validator/whitespace checks.
- `composite-sweep-seal-01-20261006/evidence.json`: SHA256 `a3badb088eac9feff52330b6ceb62987ea44343eff8d00d0da6f4621a58b3f25`;
  9053 regular artifacts, 5389662817 bytes, zero links.
- `composite-sweep-delivery-01-20261006/delivery.json`: result, remote tip,
  source/mode comparison, owned scope and seven earlier-tip ancestry, bound after publication.

Owned files are `scripts/economic_sql_canonical_audit.py`,
`tests/async/test_economic_sql_canonical_audit.py`, the operator guide, this
qualification report and the remote follow-up. No shared producer, coordinator,
accounting contract, migration, registry/matrix or central manifest is edited.
No shared accounting/schema interface change is requested. Primary integration
must preserve the predecessor reader APIs and storage/book/orphan checks already
present at the base: `CanonicalReader`, `verify_canonical_root`,
`verify_canonical_baseline`, and the original root progress/candidate walker.
Operators explicitly selecting the new mode consume v2 `namespace`,
`next_namespace`, `namespaces` counters/ages, sticky aggregate status and the
existing false authority flags. Default v1 consumers are unaffected.

The required curator packet is this report, remote follow-up, source seal and
delivery receipt. The user says primary maintains the shared notebook locally
and that it is nonblocking. No curator application/acknowledgement or private
primary artifact inspection is claimed. AI_CONTEXT.md remains unavailable and
nonblocking. No cross-chat message is sent.

## Remaining gates

This closes bounded fair scheduling for these three SQL namespaces. Full R7
still needs the other retained/native namespaces, complete current holdings and
receipt/allocation reconstruction, flatfile resumable reconciliation, quiescent
whole-store comparison and release-host large-history measurements. Genuine R6
capture authority, complete item selection/wallet exclusions, actual command/
native/save/publication/recovery journeys, managed backup/restore/retention,
typed active erasure,1000 mixed roots and latency/storage/checkpoint budgets
remain separate requirements. The refreshed primary's auction source-claim
component is reported qualified on its own source; it is not independently
requalified by this slice. The full restore fixture still privately synthesizes
activation and cannot establish genuine capture/activation authority.

No isolated/model/inventory result completes Plan5 or R1–R8. Release validator
continues to refuse. No independent blocker or notebook blocker is asserted;
the primary owns the future tested combined candidate. Accounting remains
inactive, wallet-root item exclusions and the declined inactive spell-path
change remain. No production mutation, audit autocorrection, deployment,
accounting activation or PR merge occurs.
