# Original SHOP no-execution SQL BEFORE reader — 2026-10-05

## Missing original recovery interface

Cold recovery needs to cancel an authenticated original never-admitted SHOP
operation only after proving its original physical/economic BEFORE cut. Existing
publication readers require an actual sealed execution receipt. Reusing rejection
would manufacture execution, and the precommit reader may INSERT a missing owner
revision. Neither interface supplies the required no-execution, read-only cut.

## Implemented borrowed reader

economic_sql_shop_trade_lock_never_admitted_before(MYSQL*, const critical_command&,
economic_sql_shop_trade_publication*) noexcept accepts no completion or result.
Its explicit never_admitted output flag is distinct from rejected=false. It
requires original schema2/v8 SHOP metadata, a complete exact frozen manifest and
a reconnect-disabled active transaction on the same original connection session.
It SELECT-locks original inbox absence first, then retains existing authority,
player/status/bank, keeper, sorted owner, custody and physical lock order.

Exact command-bound player level/save, wallet/bank, shop/cash/VNUM/roam, selected
item/stock/target-parent revisions and complete player/keeper BEFORE forests
are checked. Values and aggregate owner revisions absent from the original
command are observed under the locks; historical monetary vectors are not
invented. A produced UID must be absent across custody and all eight physical
stores. Missing owner revisions are observed zero and never inserted.

Bounded indexed absence reads reject any original root/account effects/postings,
item references, child links, ownership ledger, source claims or outbox. Distinct
legacy_operation_id and child_operation_id links and the original encoded
lineage/source_event claim are checked as well as operation_id. No execution
receipt, plan, admission row, journal transition or rejection is manufactured.
A final session check precedes output assignment; failure preserves the caller's
output. The no-MySQL policy remains ENOTSUP.

Existing execution overloads pass their actual sealed completion into the
internal reader. Their canonical result validation, execution/rejection phase
selection and later-revision semantics remain. The new never-admitted path alone
requires exact original BEFORE revisions.

## Evidence and integration obligations

Private source receipt 9cbed8102cf57f975abf414dda0ef01ce274352b3c75aca236c8f73ec8ad1eb5;
patch e37999d20e282b811cc414be074d0317eb3628b8687d70a56905ac4b93c12759.
Database specialist source review accepted both files and all recorded schema/
source dependencies: lock/session/order, indexed absence, strict phase guards,
legacy behavior and strong output preservation. Exact Git preimages, forward/
inverse and changed-line clang18 fixed point pass. Source pins/census and static
validator/matrix checks are recorded in the milestone receipt. No local build,
native test, gameplay, SQL, service, persistence or recovery execution ran.

This reader grants no cleanup or ACK authority. The cold caller must retain the
authenticated original refusal and guarded publication reservation, own this
transaction cut through genuine native cleanup, and confirm rollback/idle or
verified lease retirement before guarded cancellation succeeds. That caller
successor is being prepared against this interface; it is not installed by this
slice. Sealed native binding, flat parity and original major-plan recovery/ACK
qualification remain unfinished. Existing inactive behavior and safety gates
stay. R1–R8, coverage, release and production activation remain BLOCKED.
