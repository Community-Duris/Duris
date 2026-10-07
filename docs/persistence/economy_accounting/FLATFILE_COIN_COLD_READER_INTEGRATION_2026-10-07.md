# Flatfile ordinary room-pile proof reader - 2026-10-07

After an ordinary coin command's journal ACK, flatfile recovery lacked a typed
reader that could authenticate the current room pile directly from its retained
head and original native proof. A UID or structurally valid saved literal cannot
grant recovery authority.

`flatfile_accounting_coin_transaction::read_room_pile_locked` now borrows the
caller's original authority lock. It recovers the existing journal and checks
the indexed original root, frozen intent, source claim, complete accounting plan,
native command digest/result and exact item references. It checks the current
UID-keyed head, full literal, room custody and owner revision against the live
lineage/epoch. Historical wallet retirement or re-enrollment does not require the
old wallet mapping to remain current.

The narrow indexed lookup stays private to the existing COIN friend owner.
Original full-command lookup equality and native formats are preserved, including
zero separate flatfile child receipts and original normalized embedded endpoint
IDs. Output remains unchanged on every refusal. The reader applies no native
effect, publishes no object, repairs no wallet, activates no epoch and ACKs no
command.

## Qualification

The same combined candidate as the [source-claim fix](FLATFILE_COIN_SOURCE_CLAIM_FIX_2026-10-07.md)
passes the complete original sanitizer suite and both original 754-provider
production links. All original assertions remain and the final original split
journey runs after the new controls.

Thirty named reader groups include twenty missing/corrupt/conflicting-proof
controls, genuine partial pickup, consumed-pile refusal, exact later room-counter
advancement, historical wallet retirement/re-enrollment and inactive/different
epoch refusals. Every negative preserves output and native durable bytes. The
fixture derives original normalized endpoint IDs and reference shards from the
actual canonical command rather than assuming caller-supplied endpoint IDs.

Primary independently checks exact raw source/artifact bytes and modes, complete
original compiler closures, generated fixture prefix, sanitizer flags/budgets,
seven token/literal formatting bridges and both complete production caches.
The eight-file formatted seal is
`a8b1eec2a4cd981390a676b8ec83fa51548f4dbf93f6925feb96bad49b268722`.
SQL production ELF:
`76ec9d0388c306462fc62e42a05300c59955acdb5144e4d0e8e4d2972eeee4f1`.
Flatfile production ELF:
`febc439749d98de4db556407cf91192bf082657b8658a796b16892ccee06f74b`.
The ten original shared contracts and final accounting/writer/route/site checks
pass; failed environment and fixture attempts remain separately documented.

## Remaining acceptance

This is a qualified data-reader prerequisite. Shared flatfile boot ordering,
recognizable typed-history and consumed-UID legacy fences, inert physical
enrollment, real player/coordinator/ACK/cold journeys and activation composition
still require implementation and qualification. All 926 writer policies and
backend evidence states remain unchanged. No full route, Plan 2 or R1-R8/release
gate is promoted; `coverage_complete=false` and `release=BLOCKED` remain.
