# Crafting conservation revision (#551 / PR573)

## Supported writers

Assassin poison mixing, epic Encrust and the Harvester soul-shard exchange use one durable craft receipt with exact input UIDs/revisions and complete output snapshots. Poison mixing plans at most 64 products in one operation, preserving the first eligible recipe's bulk behavior. Ingredients are untouched until commit. A successful operation retires its inputs once; deliberate Encrust failure records zero outputs. Replays return the recorded outcome.

Unsupported player potion mixing has been removed from the interpreter and its executable function/declaration retired. NPC potion replacement and exclusively unused helper removal are tracked separately in #661. Persisted skill/class IDs and shared potion content remain intact.

## Persistence and recovery

On this branch craft is reason 34; existing soulbind 27, slip 28, wear 29, remove 30, fumble 31, disarm 32 and quest turn-in 33 remain stable. Decoder compatibility recognizes version 7 same-player craft shapes at draft reason 27 or master reason 29. Version 9 wear 29 and normal cross-player soulbind retain their meanings.

SQL commits custody, ledger events, physical output rows and runtime state together, and deletes consumed physical input rows. Migration 0051 adds `player_item_runtime_state`, an item-row child with cascading deletion. Canonical columns remain authoritative for placement and existing properties; the snapshot preserves craftsmanship, generated keys, anti/extra2 flags, all timers and dynamic affects. Both asynchronous and legacy synchronous save/load adapters maintain this state. Runtime and lifecycle manifests include the table. Metadata fingerprints were measured on fresh MySQL 8.0.46 and MariaDB 10.11.14 schemas after applying and reapplying the migration.

Flat-file custody and materialization commit the same exact craft outcome. Committed results stay in retained publication until registry/live publication succeeds and coordinator acknowledgement completes. Missing live outputs are reconstructed from the exact committed snapshot/UID. Reconnect and acknowledgement retries do not reroll outputs or repeat progression/messages. Uncertain commits keep their fence and durable retry record.

## Explicit restrictions

- Virtual Chaos-pouch Encrust reuses the delivered native pouch mutation: its UID remains active and its usage counter delta joins the craft receipt, including failed attempts.
- Trapped-item Encrust refuses because trap fields are absent from the existing snapshot codec. Both inputs remain intact.
- These three writers select carried roots. The craft admission boundary refuses a selected nested input rather than admit different SQL/flat-file subtree semantics.
- Restitution-delivery inputs refuse before mutation; a craft output blob cannot safely stand in for an input restitution snapshot.

## Qualification

Focused executable coverage includes poison batches/refusals at 1, 3 and 65 available recipes (64 product bound), duplicate admission/replay, stale inputs, multi-output and zero-output conservation, ownership reload/idempotency, persisted reason compatibility, retained publication with an absent player and missing output, ordinary rich-state item materialization, and legacy save of multiple roots/SQL refusal.

The native SQL harness ran on isolated disposable MySQL and MariaDB instances. It performs craft, normal checkpoint save, normal player repository reload, receipt replay, stale refusal and a subsequent zero-output retirement, checking custody/revisions, runtime fields and orphan cleanup. The normal save/load fixture runs the real repository implementations. No configured game database or production credentials were used.

Local commands:

- `python3 tests/async/test_issue_551_crafting_conservation.py`
- `python3 tests/async/test_issue_551_flatfile_craft.py`
- `python3 tests/async/test_issue_551_runtime_state_save.py`
- `python3 tests/async/test_item_transfer_version_compatibility.py`
- `python3 tests/async/test_item_ownership_runtime.py`
- `python3 tests/async/test_player_load_items.py`
- `python3 tests/async/test_publication_retention_runtime.py`
- `python3 tests/async/test_data_lifecycle_manifest.py`
- `python3 tests/async/test_runtime_boot_compatibility.py`
- `make -C src PERSISTENCE_BACKEND=mariadb` and `PERSISTENCE_BACKEND=flatfile` in the maintained build image, GCC13.3.
- `./scripts/format.sh --rev origin/master --check` and `git diff --check`.

The combined #573/#663 real-server journey passed with flat-file authority and
with isolated MySQL plus Redis world recovery. It exercised three copyovers,
trusted staff theft of an automatic VNUM 102 vial, actual Assassin poison mixing,
NPC death and corpse loot, epic Encrust, Harvester exchange, acknowledged saves,
account-menu exit and cold reconnect. Exact consumed UIDs disappear once and
all output/vial UIDs survive reload. SQL runtime-state bytes are identical after
normal save/load and Redis cold recovery. Ordinary player stealing is currently
disabled; the theft proof uses the supported trusted staff route. Combat was
observed through two controlled NPCs; cadence is measured by the native fixture.

Gameplay qualification exposed and repaired staff membership saving that
replaced the gameplay racewar with the account menu's immortal category,
copyover using a character name instead of its authoritative account name, and
exec retaining the old Redis writer lease. Account restoration verifies PID/name
membership. Redis writers drain before the lease is released, after the durable
copyover file is published and before transports change; failed exec resumes
recovery. Focused native regressions cover refusals and recovery.

Apply migration 0051 through the normal migration workflow before booting the
revised SQL server. Disposable SQL gameplay databases used normal adoption,
application and replay. No configured game database or production migration
was changed. The experimental-accounting delivery has its own IDs, migration
history and activation requirements; these master results do not certify active
accounting crafting or automatic vial issuance.

The unsupported player mixing retirement also removes its exclusive ingredient
lookup/selection/extraction and bottle-selection helpers. This keeps the later
NPC stock cleanup compatible without retaining unreachable player code.

The replaced poison extraction helper is also retired: exact ingredient retirement now belongs solely to the durable craft receipt.

Current accounting identities and native route requirements are detailed in
[alchemist-accounting-port-551-661.md](alchemist-accounting-port-551-661.md).
The #551 continuation extension freezes poison notching and retains alchemy
publication across player saves and restart; see the dated
[active recovery evidence](persistence/economy_accounting/ALCHEMY_ACTIVE_RECOVERY_2026-10-03.md).
