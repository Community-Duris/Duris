# Session 04: Set-Based PvP and Epic Task Reads

**Session ID**: `phase03-session04-set-based-pvp-and-epic-task-reads`
**Status**: Not Started
**Work Window**: The remaining gameplay read fan-out boundary covering recent PvP death
calculation, eligible epic-task selection, related history reads, in-memory hydration,
set-based repositories, and semantic equivalence tests.

---

## Objective

Remove the confirmed recent-death N+1 sequence and random-zone full scan/sort from
gameplay callbacks while preserving heaven-time and epic-task selection rules exactly.

---

## Scope

### In Scope (MVP)

- Re-audit post-Phase-02 recent PvP death, heaven-time, epic task, completed-zone,
  trophy/history, zone eligibility, and refresh call sites and record stable query IDs.
- Replace the latest-20-ID loop plus one `pkill_event` query per ID with one bounded
  join/aggregate or equivalent set-based read against the implemented PvP schema.
- Keep the heaven-time base, one-hour boundary, latest-20 scope, doubling rule, and
  extreme-duration rule behaviorally identical, including zero-row and failure cases.
- Hydrate the bounded task-zone catalog in game memory and retrieve a player's completed
  task identities set-wise from the Phase 02 epic ledger or its read model.
- Select uniformly from the in-memory eligible set without `ORDER BY RAND()`, `NOT IN`
  null ambiguity, a database query in the callback, or repeated full-history scans.
- Make zone catalog and completion refresh identity, staleness, failure, and invalidation
  explicit and preserve the prior safe value when refresh fails.
- Rewrite directly related non-sargable or N+1 history helpers found by the scoped audit
  when they share these result sets and verification boundary.

### Out of Scope

- PvP write batching, epic transactions, or reward semantics completed in Phase 02.
- Broad report-cache redesign or unrelated gameplay query modernization.
- Composite index application before Session 05 measurements.

---

## Prerequisites

- [ ] Phase 02 PvP and epic domain schemas, ledgers, and read identities are validated.
- [ ] Sessions 01 through 03 expose reusable bounded read/result patterns.
- [ ] Phase 00 heaven-time and artifact/frag correctness regressions remain passing.

---

## Deliverables

1. Set-based recent-death repository and unchanged heaven-time calculation integration
   in `src/fight.c` and focused database modules.
2. In-memory task-zone catalog, player completion set, bounded refresh, and uniform
   eligible-zone selection replacing callback SQL in `src/epic.c`.
3. Stable query-site metrics and a scoped inventory proving the eliminated N+1 and
   random-sort routes.
4. Focused boundary-time, latest-20, zero/many-death, no-zone, all-complete,
   distribution, refresh-failure, query-count, and semantic-equivalence regressions.

---

## Success Criteria

- [ ] One recent-death read returns the count needed for heaven time without a query per
      event and preserves every documented calculation boundary.
- [ ] Epic task selection performs no database or Redis operation in the gameplay
      callback and never uses `ORDER BY RAND()`.
- [ ] Completed-zone membership is loaded set-wise and eligible selection is uniform
      within tested tolerance without returning a completed or ineligible zone.
- [ ] Refresh failure retains or rejects under a documented safe rule and never turns
      missing data into a duplicate award opportunity.
- [ ] Scoped source and query-count tests find no remaining N+1 route in these paths.
- [ ] Focused regressions, isolated query tests, formatting checks, and `make -C src`
      pass.
