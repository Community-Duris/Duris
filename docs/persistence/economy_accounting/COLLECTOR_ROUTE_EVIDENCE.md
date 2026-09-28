# Collector accounting route evidence

The schema 2 collector component is inactive at gameplay admission. SQL and
flatfile support a priced purchase, expiry into destruction, and held
cancellation into destruction or quarantine. Other collector actions keep
their current schema 1 route and are not admitted by the flatfile component.

`collector_purchase_accounting_intent` freezes the complete listing record,
its death operation, the selected item UID and revision, and the wallet and
bank lifetime mapping IDs. The purchase plan debits the player's wallet by the
listing price and credits collector sink 24 by the same value. The bank has a
revision-only effect, matching the native purchase. One item event moves the
same UID from collector custody to the player. Expiry and held cancellation
have no money postings and one item event to destruction or quarantine. Every
plan carries the original death operation ID.

The SQL critical-command repository now routes a schema 2 collector envelope
through one inbox transaction. It calls `economic_sql_collector_lock`, then
`economic_sql_collector_execute_and_record`. The adapter renews the active
epoch and lifetime mapping locks, runs the native collector writer, and inserts
the accounting root, exact money rows, and an item reference to the legacy
ownership event. The repository writes the receipt and outbox, checks the
retained root, and commits once; any error rolls back. Replay and reconcile
check the frozen intent, canonical plan, exact effect/posting rows, item
reference, and outbox. A context retained from an earlier transaction cannot
bypass a changed epoch. The schema 1 entry point stays closed to schema 2.

The flatfile collector owner renews the active epoch and, for a purchase, the
wallet/bank lifetime mappings before native preparation. Under one authority
lock, it snapshots the listing and item custody, plus purchase balances;
compares the prepared native result with the pure plan; and commits the catalog,
custody, canonical item reference, and EAP1 record in one journal. A purchase
also commits its wallet and materialization. Expiry and held cancellation have
one item event and no money postings. A forced interruption recovers the
complete bundle. The same operation replays its recorded result; stale
purchases retain an empty-plan rejection. The typed flatfile owner can be
called directly; common admission still refuses collector schema 2.
Unsupported actions fail before native mutation.

The disposable `run_collector_sql_accounting_mysql.py` journey verifies a
purchase through the SQL critical-command root, zero-sum postings, source link,
native listing and item state, receipt, outbox, retained intent and result
bytes after reconnect, replay and reconcile, rejection of a changed posting,
a terminal stale purchase with no effect rows, a separate expiry root,
quarantine cancellation with the original UID, and rollback when an accounting
reference write fails after native writes. Purchase, expiry, cancellation, and
the forced failure all use the critical-command root. The journey passed on
MySQL 8.0 and MariaDB 10.11.
The focused pure plan
and flatfile collector tests cover typed price/custody checks, active-epoch
purchase and rejection, terminal expiry and quarantine cancellation, journal
recovery, replay, and item-reference recovery. The SQL and flatfile server
builds pass with GCC 12 and isolated outputs under `bin/`.

Run the disposable journey with
`bash tests/async/run_collector_sql_accounting_schema_mysql.sh` for MariaDB or
`COLLECTOR_ACCOUNTING_DB_IMAGE=mysql:8.4 bash tests/async/run_collector_sql_accounting_schema_mysql.sh`
for MySQL. The wrapper creates and removes its own container and database.

Plan 1 still needs to admit the schema 2 collector at the coordinator. The
gameplay producer still submits schema 1. This evidence does not activate the
route. Collector collection, activation, pause/resume, and hint
actions require their own accounting policy before an active epoch can admit
them. The player command surface has list, inspect, and carried-coin buyback;
gem barter remains unavailable.
