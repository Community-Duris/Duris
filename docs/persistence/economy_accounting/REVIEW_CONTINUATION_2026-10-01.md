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

The fresh 803-test regression batch and auction/collector/shop native fixtures
are still running against their frozen pre-integration copies. Completed
component checks cannot qualify an unexecuted player route. The remaining
R1-R8 requirements, full gameplay and fault matrix, independent audit,
activation/refusal coverage, lifecycle proof, and flatfile parity still govern
completion. Keep unsupported checks and failures visible; do not infer a green
release from source contracts or an incomplete writer matrix.
