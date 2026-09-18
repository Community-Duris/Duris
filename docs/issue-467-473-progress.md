# September 17, 2026 bug batch progress

This ledger tracks issues #467–#473 against master. Runtime evidence uses disposable local game state. Production recovery and item reimbursement are outside this batch.

| Issue | Root cause / disposition | Branch and PR | Validation | Merge commit / remaining work |
| --- | --- | --- | --- | --- |
| #467 | corpse_bulk_get reached strict GET_PID for an NPC after a container claim. PC guard added before PID access. | codex/issue-467-npc-pickup; PR #491 | Baseline strict-NPC abort; fixed flatfile NPC claim with one committed revision, UID/custody checks, MariaDB player-haul and stale/rejected journeys, ASan/UBSan harnesses, both backend builds, and all PR checks passed. | Merged 04fee929b0d10c8b72ca10cdd3f5df1eabe57c75; issue closed. |
| #468 | CREATE/str_dup allocations in sql_restore_saved_items were released with libc free. Matched FREE/str_free now release them. | codex/issue-468-restored-list-release; PR #493 | Baseline full-world MariaDB abort; fixed zero/one/two roots and malformed-row boots; allocator sanitizer boot checks and focused contracts passed. | PR CI/review/merge pending; nested case depends on #469's saved_items key migration. |
| #469 | Restore publishes SQL rows then deletes every source before durable handoff. | codex/issue-469-durable-saved-items; PR pending | Baseline MariaDB two-boot journey: root visible first boot, zero source rows, item absent after restart. | Handoff/replay fix and failure injection in progress. |
| #470 | Boot lease contention used permanent quiesce state. Retryable lease recovery implemented separately from deliberate shutdown. | codex/issue-470-lease-retry; PR #494 | Baseline expired lease never resumed; fixed real MariaDB/Redis/game journeys covered active writer, expiry, outage, auth correction, cancellation, repeated boot and generation/floor acknowledgment; focused sanitizer and live Redis checks passed. | PR CI/review/merge pending. |
| #471 | Pending follower custody investigation. | Pending | Pending gameplay, persistence and backend journeys. | Pending. |
| #472 | Pending chaos-mode dracolich reproduction. | Pending | Pending gameplay journey. | Pending. |
| #473 | Pending backpack give/return reproduction. | Pending | Pending gameplay and UID graph journey. | Pending. |
