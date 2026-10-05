# Native quest borrowed item persistence integrated — 2026-10-05

The explicit native-v11 contracts could identify an original NPC and final giver,
but neither backend could yet mutate its ordered stock together with UID custody.
Six source/header files now provide that bounded persistence slice beside the
existing generic APIs. They are deliberately dormant until the original root,
source, lifecycle and publication owners are implemented.

## Result and reviewed failures

The pure transform preserves equipment prefix, literal DFS and native carried
grouping. Acceptance inserts one complete player root; consumption removes exact
complete native roots. Each transition advances original mobile/stock revisions
once, retaining original identity, source facts and known cash-v2 values/revision.
Historical image/reference bytes and all previous source bodies are unchanged.

SQL locks the existing native lifetime first under the original reconnect-disabled
transaction, then existing owner rows and ordered custody. It verifies the held
whole player literal preimage before selected-root removal. Native image, custody,
legacy events, player projection removal and owner revisions share that borrowed
transaction, with complete native AFTER readback. The attempt marker precedes DML;
the parent must roll back or retire the original transaction after any later error.
This API does not commit, issue IDs, create native owners or insert rewards.

Flat preparation reads existing native/catalog authority, reconciles the original
player preimage, and returns native image, existing ownership catalog and player
outbound materialization together. Original operation/result and frozen final-giver
consumption continuation remain in that same catalog. The parent must commit these
with source, accounting evidence, inbox/receipt/outbox and cleanup in one authority
transaction. Preparing a vector is not durable application or acknowledgement.

Independent review corrected real counterexamples before installation:

- Complete forests are bounded separately from selected events: up to 4096 per
  body, 8192 in the combined SQL proof cut and 4096 in native readback, with SQL
  LIMIT bound+1 before buffering. The generic 3000 transfer limit is unchanged.
- Accounting witnesses include only selected complete roots, within the existing
  budgets of 3000 events and 6000 witnesses. Full player/native proof remains independent.
  A selected parent with an omitted child now refuses before cascade deletion.
- Flat snapshot/materialization I/O, missing/corrupt data and codec allocation
  failures remain errors; only obtained valid mismatches become stale refusals.
  SQL reader diagnostics survive before and after DML; consumption codec errors
  propagate. Unclaimed inline coins retain the existing ordinary-item separation.
- SQL-only helper declarations stay excluded under __NO_MYSQL__; its explicit
  unsupported entrypoint cannot open generic flat or legacy execution.

The existing image-writer registry now describes its dormant caller. Two new
definition rows describe explicit SQL/flat item participants, with every backend
unverified. Source census mapping remains an inventory, never route qualification.

## Link closure and evidence

Twenty-seven recipe/helper files add only required source arguments; fifteen
existing value/SQL jobs use a scoped unavailable-world fixture whose physical
capture, template/string/memory entrypoints abort unexpected calls. It cannot
fabricate world proof or qualify native quest gameplay. Two no-MySQL SQL-context
recipes remain byte-identical, relying on the explicit unsupported guard. Existing
cases, assertions, strict warnings, sanitizer flags, timeouts, budgets and opt-ins
are preserved. The production Makefile already contains all required units.

Worker receipt af5f943f2f59cefad3a8392de995817b21cd1c89cc6c49d95255f3498377db4a;
accepted root successor 8e7a4ba0c55efcaa85dd4ee73dcb9c5ecfd2a52e38f3bbcae632fc59e46e7e86.
SQL C b4966c9da23d0362ac2879f7c38e61a510522c109f0c63d9ea5e305b52cd34dc.
Exact current preimages/candidate hashes, old-source inverses, changed-line clang18,
Python AST inverses, shell syntax and complete direct/inherited consumer inventory
are retained under tmp/plan3-native-quest-participant-installation-20261005.
No compilation, test, SQL, service, gameplay, persistence or recovery run occurred;
original major-plan testing remains deferred to readiness, per user instruction.

## Original work still required

Native birth/reset/spawn/source and lifetime/restore/retirement ownership,
qualified0059/0060 and both-engine metadata, original atomic root classification/
admission/source/evidence/receipt, save/checkpoint/literal and body holds,
final-giver reward insertion, zero-item monetary quest completion, sequential
producer and retained physical publication/recovery/guarded ACK remain open.
Plan 5's independent readers also need native-owner grammar parity. SHOP cold
replay/publication and flat parity continue separately. No full NPC-state ledger
or new release gate is introduced. Accounting remains inactive; the declined
spell path is unchanged. Plans3/4, R1–R8, release and activation remain BLOCKED.
