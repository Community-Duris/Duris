# Shared initial checkpoint code milestone - 2026-10-08

The original birth command retained cash and stock but omitted the initial
keeper's save time, roaming and saved affects. Recovery needs the original
complete record, rather than a newly sampled replacement after restart.

## Maintained implementation

`src/flatfile/flatfile_shopkeeper_repository.h/.c` now provide passive initial
checkpoint encode/decode using the existing DURSHOPv2 catalog codec. The wrapper
contains exactly one revision1 record. Its catalog revision1 is deterministic
framing only, not an observed whole-file clock or write authorization. It keeps
the original scalar cash, room/time/roaming, every saved affect and complete
serialized item forest. Affect sorting follows the existing canonical order.

Preflight checks original byte/count/extents before decoder allocation. Decode
requires exact canonical v2 bytes and rejects trailing, alternate and multiple
records. Outputs change only after success. Existing catalog reads, writes and
trades remain exact; no production route, activation, schema or storage action
was added. Only the accepted codec insertions were ported to maintained source;
unrelated private catalog/trade/checkpoint-owner helpers were excluded.

Independent source review accepted port manifest
`175f9d5cd3d62a8ee4ae7394618889e5a247587b938db686ddac584ced233546`,
46 actual maintained provider pins and four insertion-only inverses. Both real
maintained preimages authenticate. New additions match accepted private codec
`8158f49f82e7220d04eefc6b9884092b0c17e914aea8271d61a37317ffb647aa`
exactly after newline normalization. Original catalog/item helpers agree.
Changed-line formatting and token checks passed; source application preserved
both unrelated modified harnesses and the untracked Plan5 qualification note.

## Integrated private implementation

Combined source candidate
`e59b317e13073554c3ff650f986a013ad98849778cd540e0d1c6f259ae6b1bb7`
joins the genuinely detached original keeper capture, canonical checkpoint,
immutable original recovery carrier, actual coordinator worker handoff and
shared retained SQL receipt proof with the earlier SQL/Smith/room work.
Capture/carrier join `afc8e8c23ea12530c7fa70b8f0e585f405ac9948f28bf08e289939772767471e`
authenticated 695 provider records and30 stacked inverse spans. Worker/receipt
join authenticated590 records and31 spans. All209 selected bodies authenticate;
these inventories establish composition, not accounting completion.

The shared recovery family keeps original checkpoint bytes through successors.
Its initial receipt enforces absent SHOP-before0/present-after1, exact empty-stock
owner preservation or checked nonempty increment. The actual borrowed worker
view pins original state/thread/attempt/generation/revision without copying the
native attachment. Review corrected worker-registry authentication and a shutdown
race: lifetime/stop checks now precede scanning thread objects that shutdown joins.
Shared callback exceptions retain ambiguous-commit status; refused preparation
keeps its queued identity and the existing pulse wakes retries.

Root implemented shared historical MBR4 replay proof in the original SQL
transaction provider. It verifies complete original inbox/result, initial policy,
canonical plan/source/item evidence, no newly created wallet and exact outbox;
it does not rerun missing-keeper creation or reject legitimate later owner clocks.
It performs SELECTs including the original inbox FOR UPDATE. The root retains
lock-order and transaction cleanup responsibility. Original save time/affects/
roaming authenticity and current physical proof remain separate obligations.
Each coupled slice passed independent source review. The larger candidate is
unpromoted; genuine producer invocation, complete transition memory budget,
root SQL dispatch/reconciliation, publication/ACK/origin and flat counterpart
remain implementation work. Existing-row warm/cold remains closed.

## Qualification and remaining scope

No compiler, preprocessor, unit/native, gameplay, SQL, persistence, recovery or
performance execution ran. Major-plan testing remains deferred by the user;
none of those acceptance gates is waived. Maintained source-only codec readiness
does not complete the shared lifecycle, Plans2-4, combined Plan5, R1-R8 or release.
Original Plan1 acceptance retains its recorded scope. Inactive behavior,
the declined inactive spell path, production data and activation gates are intact.

Source evidence: `bin/tests/shared-original-checkpoint-recovery-primary-20261008/`,
`bin/tests/shared-worker-retained-sql-primary-20261008/`, and
`bin/tests/shared-initial-codec-maintained-port-primary-20261008/`.
The ongoing goal remains active.
