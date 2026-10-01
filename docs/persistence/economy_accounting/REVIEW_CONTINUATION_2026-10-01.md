# Accounting review continuation, 2026-10-01

Local implementation remains on `codex/accounting-review-fixes`. Accounting
activation and full-feature release remain blocked. No production access,
deployment, push, PR publication, or GitHub merge was performed.

## Established fixes and evidence

- Quest recovery now has a nonunique `(reason_type, reason_id)` witness index.
  Disposable MySQL 8.0.46 and MariaDB 10.11.14 tests include 100,000 unrelated
  historical rows and prove the bounded three-query, 4,160-row, 466,984-byte
  maximum recovery read. Wrong index shape and duplicate witnesses still fail.
- A real MySQL crash journey exposed a save/publication race: a queued quest
  XP save could omit an already-committed item. Save admission now waits for
  creation publication, and the quest owner retries the save after item and
  cash callbacks. Pending effect receipts and dirty components remain retained.
  Focused creation, player-save, and mixed item/cash/XP tests pass.
- SQL offering and XP-ack crash journeys pass on both engines through two cold
  restarts. The tested pre-format SQL binary SHA-256 is
  `1e09339bcc1ebecbdaa880111530c6dc941ca79d83b53cfb778dd4afac1a0a9d`.
  Corresponding flatfile journeys and plain/MCCP copyover pass with binary
  `d4daf8f399e4567b6bce9522c8d3108725e865e6796dfa05ef53da7661bd3d21`.
  These hashes do not certify the later integration candidate.
- Both native engines pass exact spell/quest receipt transactions, item save
  reconciliation, reconnect/restart, foreign-owner refusal, complete canonical
  and staging-fork migration checks, and metadata/index tamper probes.
- The guarded native lifecycle runner executes the maintained sanitizer
  harnesses on caller-owned disposable loopback servers. Both engines pass
  exact-session ownership, staged baseline recovery, held publications,
  uncertain commits, lease transfer, and post-terminal cleanup failures.
- Regression fixtures now select the available compiler, link actual moved
  modules, use current callback and ownership format contracts, and preserve
  real disconnect state. The pfile tool refuses SQL deletion approval.
- Formatting changed 93 tracked C/C++ files. Every change preserved significant
  code characters and string literals. Writer sites were reanchored by ordered
  occurrences without changing evidence or qualification status. The ordinary
  accounting validator passes; the release validator remains blocked.

## Integration and remaining gates

The initial reviewed public branch was `8ef7e9d243d3ccb14477a88d142c17743fba78a0`.
A current GitHub refresh found `bd7fe95c6dcb47c04c68ba9689429f12e112909e`, including
the alchemist accounting port and immutable migration
`0051_player_item_runtime_state`. The local integration preserves that immutable
migration at sequence 51 and appends the unpublished quest index at sequence 52.
Canonical and staging histories now have 52 receipts. Fresh disposable native
schemas, migration replay, bounded quest reads, item save reconciliation, exact
spell/quest receipts, and shell compatibility pass on both engines. The staging
fork also passes on both engines with its original 45 receipts preserved,
seven appended steps, compiled boot verification, metadata tamper refusal,
advisory-lock ownership, connection-loss rollback, and cancellation faults.

The measured complete runtime metadata fingerprints are
`13daaa95b721f9492328e33cbf8a657a800c383ef799f80d88902b0f504d63bd`
for MySQL and
`59c33f6d8b4de0ca6919c7e609d48df6ea7a98031292c030efc9bf2668214cf4`
for MariaDB. The compiled header and runtime manifest validate together.
The writer matrix now contains 864 routes and 2,816 lexical occurrences;
all 2,758 unique sites are mapped. Release qualification remains blocked.

The checkout's `origin` points to `Community-Duris/DurisMUD`; refresh
the requested `Community-Duris/Duris` repository explicitly.

The frozen pre-integration regression batch completed with 790 passing and
13 failing tests out of 803. Its generic SQL combat entry skipped without
database settings; it is not native SQL gameplay evidence. The fresh integrated
812-test batch completed with 759 passing and 53 failing checks. Its failures
remain visible; focused reruns after fixture and dependency repairs pass 23/24,
then the remaining copyover check, followed by 6/25, 15/19, and the last four
checks. Full-world and CHAOS kit gameplay require separate current-binary
qualification below. A separate
current-fixture follow-up passed 23 of 24 checks; the remaining copyover harness
then passed with the current world-worker hooks. Death Field runtime and all
52 writer-coverage contract tests also pass.

A consolidated rerun of the 50 component failures finished with 49 passing and
one writer-census failure in its frozen pre-reanchor snapshot. After retaining
each route's evidence and updating the SQL line references, all 52 current-tree
writer-coverage tests pass. Together with the three direct gameplay checks
below, the previously failing checks have passing focused replacements. This
does not claim a new all-green 812-test batch or release qualification.

At integration commit `a7902d2dd95b8d0d93c30ad6e099d85da897ec8b`, strict SQL and
flatfile production builds passed with SHA-256 values
`7e2489cadb04f5d3fe864ef87a30d27202d005ad0de9a176a7635c73418b6b13` and
`fa244bbd47949ffab54dbfb35b3c8fcfe65beb77ca378094b7f48bb74e2381b8`.
The SQL offering and XP-ack crash journeys pass on both engines, and the same
flatfile journeys and plain/MCCP copyover pass on these frozen binaries.
Integrated native auction listing, bids, claims, settlement, shop, and collector
fixtures pass on both engines. The integrated flatfile combat/death/loot/restart
and first-session currency journeys also pass.

Both engines pass the guarded, unassisted SQL retained-death-conflict journey:
durable acknowledgement precedes the account menu, self-scoped list/detail and
cold-entry refusal work, and restart retains the exact original items without
manual fixture repair. This uses a separate strict `TEST_MUD` development binary,
SHA-256 `5b65e7de33849c43f6b5a7005b7946a9dd7b1094a81febb28beeaad842cab093`.
The test-only selector remains constrained to an explicitly disposable loopback
database; this does not enable production conflict application.

Native SELECT-only audit-origin and partial-snapshot fixtures pass on both
engines. Real SQL exposed `SUM(BIGINT)` returning `Decimal`, which broke JSON
export of pending-claim consumers. Export now converts only exact integral
values, preserves values above JavaScript's exact integer range, and refuses
fractions. Nine focused tests pass. Fixtures now retain inbox receipts and
locator kinds and explicitly require the current fail-closed findings for
unmapped wallets, unauthorized mapping creation, and unattributed UID history.
The exporter still declares incomplete coverage; these checks do not qualify a
complete independent audit.

A direct latest-binary static quest diagnostic passes item save and cold
reconnect with the same reward UID. Its fixture now waits for the recovery
message and verifies the actual inventory instead of the old live-grant text.
The integrated CHAOS kit journey reproduced a real equipment regression:
historical creation custody with slot zero overrides later ordinary worn slots
on load. Native repository tests now distinguish initial creation references
from later custody movements: creation preserves a newer saved worn slot, while
movement references and explicit native positions override stale snapshots.
The reset manifest also now deletes `player_item_runtime_state` before its
parent item rows; its schema and deletion-order regression passes. Strict
production builds after these repairs have SHA-256 values
`dcf11a07b85076455b32dba76d05cda04696e4bca49e4695ebed9d6d5dd3a3e8`
(SQL) and
`1da2726364dfa36e3ff7aabc39193588e2e99fd3e8d4d7f0424f58114060d8cd`
(flatfile). On that flatfile binary, the static quest reward and all five CHAOS
class kit journeys pass: Warrior, Monk, Thief, Sorcerer, and Dragoon. The
full-world player/floor-item process-restart journey passes with the original
mace UID, retained native custody, and private accounting directories. These
are explicitly selected production artifacts, rather than claims that the
regression suite's cache rebuilt them. The kit checks do not exercise virtual
pouch encrust; that feature's explicit refusal remains unfinished.

Completed component checks cannot qualify an unexecuted player route. The remaining
R1-R8 requirements, full gameplay and fault matrix, independent audit,
activation/refusal coverage, lifecycle proof, and flatfile parity still govern
completion. Keep unsupported checks and failures visible; do not infer a green
release from source contracts or an incomplete writer matrix.
