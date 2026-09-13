# Locker identification

Paid locker `stat <item>` captures the selected item's description and charges the configured copper value through the critical currency pipeline. It does not add a fixed recovery timer or clear existing combat/skill waits. Ordinary lore and legend-lore recovery remain unchanged. Fighting, ownership and level-based identification restrictions are checked before accepting the request.

The captured description belongs to the selection at request time. Renaming, moving or extracting that item while payment is pending does not change the purchased description. The deferred service retains no object or character pointers.

## Durable payment and replay

The service first writes a prepared receipt containing the captured text and the complete immutable currency command: operation ID, timestamp, account, player ID, racewar, deltas and original expected revisions. Only after that atomic write and directory synchronization succeed may payment be submitted. After a committed callback, a paid marker is durably written before sending the description, and a delivered marker is written after sending it. All receipt reads and writes run in workers; the game pulse only polls ready futures. A storage failure prevents charging or delays delivery, depending on which boundary failed.

Login, reconnect and copyover recovery resume a prepared receipt and show a paid receipt that was never delivered. They print nothing for delivered or failed receipts. Players can also use `stat receipt` at the lockers to recover or reread the latest receipt, including when automatic recovery was busy or ordinary network output was lost. A prepared receipt is resubmitted with the identical operation and bytes: the existing MariaDB or flatfile ledger returns an already committed result without another debit. Definite insufficient-funds or stale-revision failures are recorded as failed; unknown results retain prepared evidence for reconciliation. Account and racewar must still match before recovery submits payment or displays text.

The latest receipt is retained after delivery, so `stat receipt` can repeat its text without charging. Automatic recovery does not repeat it. If the delivered marker is not saved, for example after a crash right after the text is sent, the next login, reconnect or copyover shows the text once more. A later identification replaces the previous paid, delivered or definitively failed receipt. An unresolved prepared receipt is recovered before another selection can be accepted. This is one recoverable latest purchase per player, not a historical archive or a guarantee of exactly-once network output. Older builds reject the delivered state as an invalid receipt, which blocks paid identification for that player; do not roll back to a binary that cannot read delivered receipts.

## Storage and operational bounds

An explicit `stat receipt` joins a pending recovery for that player instead of
being dropped. A reread requested while the delivered marker is being written is
shown after that worker finishes. These requests reuse the existing admission
slot and never submit another payment for a finished receipt. When all slots are
occupied by other players, `stat receipt` reports that the clerk is busy.

Receipts live under `CRITICAL_COMMAND_JOURNAL_DIR/locker-identification/<pid>.receipt` on both backends. The directory is private (0700), files are private (0600), and an exclusive service lock prevents two processes from operating the same receipt store. Files contain a bounded binary command, at most 64 KiB of captured text, and a SHA-256 checksum. Reads reject wrong player IDs, corrupt/truncated data, oversized data, public permissions, symlinks and nonregular files. Initialization failure disables the paid service.

At most 16 requests, each with at most one worker operation, are active at once. Busy callers can retry; automatic login replay has the same capacity limit. Failed writes retry at one-second intervals while the owner is online. Disk latency does not block the game pulse; orderly shutdown joins outstanding I/O and therefore can wait for storage. A process crash leaves the last durable prepared or paid state recoverable.

Keep the receipt subdirectory in recursive backups of the critical journal root, alongside the corresponding currency authority/ledger and its recovery history. Do not selectively delete prepared receipts or payment deduplication history. The lifecycle manifest classifies these files under the protected critical-command journal. New code cannot reconstruct descriptions lost by the older process-local implementation before this upgrade.

## Validation

`python3 tests/async/test_locker_receipt_recovery.py` runs the production receipt service and codec with controlled scheduling/networking and the real flatfile payment repository. It exits separate processes before payment, after commit, and after the paid marker, then recovers wallet and bank purchases and verifies one debit and the original text. A further process verifies that automatic recovery of the delivered receipt prints nothing and submits nothing, while `stat receipt` still shows the original text. It also checks duplicate callbacks, disconnect recovery, uncertain outcomes, ownership, admission, insufficient funds, storage failures, malformed receipts, and `stat receipt` during pending recovery, a pending delivered-marker write and full admission. A blocked storage worker leaves 10,000 game pulses responsive without submitting payment.

With `TEST_DB_HOST`, `TEST_DB_USER` and `TEST_DB_PASSWORD` set to a disposable test server, the same runner creates a unique temporary schema, applies the existing schema and runs the identical crash matrix using the real MariaDB repository. It drops only that schema and never reads `.env`.

`test_locker_identify.py` checks the actual lore renderer for weapons, armor, totems, potions and wands. `test_currency_input_queue.py` checks prepared-command encoding and existing currency gates under ASan/UBSan on both builds; `test_currency_transaction_contract.py` retains schema and source contracts. These controlled tests do not constitute a live multiplayer latency benchmark.

## Backup and restore qualification

The backup journal inventory preserves `locker-identification/<pid>.receipt` and
its empty `.service-lock` under the critical journal root. Receipt filenames must
contain a positive signed 32-bit player ID and files must fit the native codec's
size bound. Existing ownership, permissions, symlink, hardlink and capture
consistency checks still apply. Transient or unexpected files fail capture.

The restore verifier recognizes this directory before and after service boot.
It accepts only an empty service lock and receipts that pass the production
bounded decoder, including checksum, payment validity and matching player ID.
It does not treat the presence of a receipt as proof that the critical WAL has
drained: both player and critical journal drain checks still run. Receipts remain
available for the player to claim after recovery on either persistence backend.
