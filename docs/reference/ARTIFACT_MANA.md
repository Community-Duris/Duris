# Physical artifact mana

This is the resource foundation for #292, used by the shared item-action adapters
from #291. It installs no artifact bindings or balance values by itself. Both
`itemActions.enabled` and `itemActions.mana.enabled` default to zero. A migrated
paid power suppresses its selected action when payment is unavailable; it must
never interpret failed payment as permission to run the legacy power.

## Amounts, profiles and admission

`artifact_mana_publish(vnum, profile)` binds a template to a named resource profile.
All paid active, passive and reactive powers on one physical UID use this pool.
Different forms may bind to the same profile ID. Changing a physical UID's profile
ID is rejected; an intentional pool migration needs its own explicit conversion.
Definitions use an ID, increasing revision, capacity, regeneration rate and passive
conservation floor. Costs belong to individual ability definitions and are passed
to the shared debit operation, not stored in generic object counters.

Amounts are integer thousandths of one point. Regeneration is thousandths per
whole elapsed second; a rate of 1 therefore generates 0.001 points per second.
Capacity is at most 10^12 units, rate at most 10^9 units per second, and costs must
be positive and at most 10^12. This bounds the arithmetic without floating-point
rounding or overflow. Exact-cost actions are affordable. An automatic power also
requires that its post-payment reserve meet the profile's conservation floor.
The initial floor is zero; this release does not add an owner-editable spending
preference. Active powers may consume the protected reserve.

An item without an existing record enrolls **empty**, whether newly generated,
converted from legacy data or administratively cloned with a new UID. Enrollment
must reach durable storage before it can spend. The first attempted use or owner
inspection may therefore report that the record is not ready. No rollout toggle,
login, equip operation or missing storage row grants a full pool.

Adapters call `artifact_mana_debit` from their `commit` callback, after the event
scheduler accepts the action and before the telegraph. The action identity's
single-use, globally increasing token prevents repeated payment. Completion does
not debit again. The adapter must return `committed` only on success. A failed
payment has changed no resource and suppresses that attempt.

All accepted costs are retained on ordinary interruption, source extraction,
technical cancellation, scheduling rearm failure, operator rollback and partial
resolution. There is deliberately no refund operation. This conservative policy
is uniform and idempotent; it avoids a second resource transaction during cleanup.
The scheduler's initial rejection incurs no charge because commit has not run.

## Regeneration and definition changes

UTC seconds settle regeneration lazily. The calculation is independent of holder,
online status, equipment slot and storage location. A single settlement credits
at most 86,400 elapsed seconds and clamps at capacity. Inspecting is a projection;
it does not write an owner snapshot or continuously flush idle regeneration.
Payment stores the resulting reserve and timestamp. Time going backwards grants
no regeneration and does not move the settled timestamp backwards.

A newer revision first settles elapsed time using the stored old rate, then
clamps to the new capacity and adopts the new rate. Increasing capacity does not
fill it. A lower revision, different profile identity, or changed capacity/rate
under the same revision fails closed. An invalid publication retains the previous
valid profile. Reloading item actions and switching flags do not clear mana state.
Real elapsed regeneration continues while powers are disabled; re-enabling is
neither a refill nor a revival of a cancelled action.

## Persistence and the crash window

The authoritative record is `(item_uid, profile_id, profile_revision, version,
capacity, regeneration, reserve, settled_at)`. It is separate from player, pet,
locker, corpse, auction and world item snapshots. Those snapshots already retain
the physical UID. They never contain or overwrite the authoritative mana reserve.
A stale owner save therefore cannot replace a newer debit.

MariaDB/MySQL uses the additive `0014_artifact_mana` migration and an InnoDB table
keyed by `item_uid`. Flat-file primary uses a versioned, SHA-256-checked record at
`FLATFILE_STATE_DIR/domains/artifact-mana-<uid>`. Writes use the existing atomic
write/fsync/rename helper under `.artifact-mana.lock`. Database modes never fail
over to a second flat-file mana authority on SQL errors.

One worker performs storage reads and compare-and-swap writes. Game-thread
admission does no SQL, fsync or disk read. Requests and completions together have
a fixed 4,096-entry limit; the cache has a 65,536-UID limit. Only one write per UID
is in flight. Further spends coalesce behind it. Lost acknowledgements retry the
exact outstanding record before advancing to a coalesced newer version. Equal
retries succeed; mismatching versions cannot overwrite each other.

The chosen guarantee is **a bounded crash-refund window, not synchronous durable
payment before every effect**. Once a UID has an unacknowledged debit, it can
accept additional spending for less than 2,000 monotonic milliseconds from the
oldest unacknowledged change. At that boundary it suppresses further paid powers
until all its accepted changes are acknowledged. A storage outage cannot extend
that admission window. A crash can restore the last acknowledged reserve and
refund debits accepted in that window; elapsed offline regeneration then follows
the ordinary rule. The amount at risk is the actual spending admitted in that
two-second interval, bounded by available reserve, costs and action concurrency.
It is not an unlimited number of seconds of storage-outage combat.

Shutdown pumps the worker for up to five seconds, then stops it before the SQL
pool closes. A current I/O call can take its configured timeout to join. Outstanding
dirty records produce an aggregate warning and retain the same crash-window
guarantee. Copyover or forced process termination also uses that crash policy.
No pending gameplay action or token is restored after process restart.

## Ownership and lifecycle

| Operation | Mana result |
| --- | --- |
| Equip/unequip; give/drop/get; put/get from container | Same UID, same reserve; action cancellation does not refund. |
| Permitted trade, auction, account/private locker | Owner snapshot retains UID; the independent ledger remains authoritative. Existing artifact transfer prohibitions still apply. |
| Death, corpse storage, loot, resurrection, corpse destruction | UID retains reserve across custody changes. Destroyed-item ledger rows remain as replay fences. |
| Disconnect, rent, relog, restart, Redis world recovery | Restored UID reads the current ledger, never mana from an older snapshot. Cold reads suppress paid powers until ready. |
| Expiration or extraction | Does not erase or refill the ledger. A newly allocated replacement UID enrolls empty. |
| Unique/major/ioun replacement or equipment forms | Same UID and profile preserve the pool; a new UID enrolls empty. A different profile ID is rejected. |
| Admin clone | A new UID starts empty. Simultaneous live objects with the same UID are denied inspection and spending. |
| Restring or other cosmetic edit | No change to the UID or pool. |
| Ability reload, capacity reduction or feature toggle | Cancel according to the action engine, retain paid costs, settle/clamp without filling. |

The bridge checks live UID uniqueness before admission, including objects inside
containers. It currently scans the live object list for each resource request;
the artifact pilots must include that cost in their latency measurements before
expanding the rollout roster. Item ownership rules remain enforced by the existing
custody system; this module grants no new transfer permissions.

`itemmana <carried-or-equipped-item>` reports exact reserve, capacity, rate,
conservation floor and storage/enablement readiness only to the holder. Public
telegraphs must not include these values. Depletion only affects opted-in paid
powers; ordinary damage, equipment stats and unconverted passive bonuses retain
their original behavior.

## Backup and rollout

The lifecycle manifest retains both authorities as protected reconciliation state.
The flat-file backup manager takes the mana writer lock before copying a generation.
Native restore qualification decodes every retained mana record, including orphan
UIDs, and rejects corrupt contents, invalid values and noncanonical or mismatched
filename identities before service boot. Database backups include the mana table
in the sealed 179-table runtime inventory.

Runtime rollback is to disable an ability, category or master flag. It retains the
mana ledger. Binary/schema rollback is a separate operation: an old binary's sealed
schema contract can refuse the new migration head. Do not drop the table or remove
flat-file records as a runtime rollback, because doing so discards the version
fences. Production migration, initial balance values and live enablement remain
separate rollout decisions under #297.

## Verification

The focused runtime test executes the production model, worker and flat-file
repository under ASan/UBSan. It covers fractional/exact/insufficient spending,
backwards and long offline time, invalid limits, conservation, revision/clamp
behavior, repeated action tokens, slow/failed writes, coalescing, acknowledgement
loss, recovery, stale writes, clone initialization and corrupt storage.

`tests/async/run_artifact_mana_mysql.sh` creates a disposable database and tests the
production SQL repository. Run with `ARTIFACT_MANA_DB_IMAGE=mariadb:10.11` and
`mysql:8.0`. It never reads the project's `.env`. Both engines also need the normal
runtime compatibility migration/replay checks when this migration changes.

`test_artifact_mana_restore.py` exercises native restore qualification without a
game listener. The backup manifest test checks mana capture and writer exclusion;
the opt-in full recovery integration includes a corrupt mana catalog case.
The shared item-action tests continue to cover scheduler acceptance and exactly-once
commit/finish. Gameplay payment integration and sustained PvE-to-PvP evidence are
added with the actual opted-in artifact pilots in #293/#296/#297.
