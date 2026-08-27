# Implementation Notes

- 2026-08-27: Session opened from `65ae3d71` after Session 09 passed and published.
- Inventory found `Guild::add_points_from_epics` saving through `add_prestige` before
  adding threshold construction, while `epic_feed_artifacts` performs a per-equipped-item
  bind read, artifact read/update, and Redis invalidation after every epic completion.
- Ordinary item custody remains owned by the Session 05 item transaction; this session
  owns only artifact tracking/feed/bind metadata and guild award totals.
- A bounded maximum-15-artifact command carries the parent identity, actor player key,
  optional guild fence, and exact artifact timer/bind revisions. The deterministic
  child is submitted immediately after its epic/combat parent so equipment and group
  state cannot change before capture, while the shared player key orders it after the
  parent commit.
- The repository locks each artifact and optional guild row, validates the hydrated
  snapshot against both domain and compatibility state, and commits current rows,
  immutable artifact/guild ledgers, inbox result, and cache outbox in one transaction.
- Guarded two-map hydration swaps only after all artifact and guild revision rows parse.
  Generic guild saves preserve transaction-owned prestige/construction columns.
- The additive schema, bootstrap parity, migration runner, guarded baseline, verification,
  and read-only reconciliation tools were exercised only against the `.env`-identified
  local development database; reconciliation reported zero mismatches.
- Standalone domain harness link lines include all generic repository dispatch targets
  and OpenSSL, and their fixtures clean isolated operation/outbox rows.
