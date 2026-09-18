# September 17, 2026 bug batch progress

This ledger tracks issues #467–#473 against master. Runtime evidence uses disposable local game state. Production recovery and item reimbursement are outside this batch.

| Issue | Root cause / disposition | Branch and PR | Validation | Merge commit / remaining work |
| --- | --- | --- | --- | --- |
| #467 | corpse_bulk_get reached strict GET_PID for an NPC after a container claim. PC guard added before PID access. | codex/issue-467-npc-pickup; PR pending | Real flatfile NPC claim: baseline SIGABRT, fixed one committed room-owner revision, preserved UIDs, NPC custody and empty container; focused ASan/UBSan harnesses passed. | Pending CI, review and merge. |
| #468 | Pending reproduction and narrow allocator fix. | Pending | Pending real MariaDB recovery boot. | Pending. |
| #469 | Pending durable restore-handoff design and replay validation. | Pending | Pending MariaDB failure injection and restart journeys. | Pending. |
| #470 | Pending boot lease retry investigation. | Pending | Pending disposable Redis writer/floor journeys. | Pending. |
| #471 | Pending follower custody investigation. | Pending | Pending gameplay, persistence and backend journeys. | Pending. |
| #472 | Pending chaos-mode dracolich reproduction. | Pending | Pending gameplay journey. | Pending. |
| #473 | Pending backpack give/return reproduction. | Pending | Pending gameplay and UID graph journey. | Pending. |
