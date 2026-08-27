# Code Review

## Findings Repaired

1. **High - same-owner container movement did not update authoritative topology.** The
   v2 payload now distinguishes source root, selected subtree, destination root, and
   destination parent. The repository proves the exact selected descendants and
   atomically updates root/parent state under the owner revision.
2. **High - corpse `save_id` alone can collide across players dying in one second.**
   Corpse custody now uses the collision-free `(player_pid << 32) | save_id` identity;
   baseline, guarded normalization, live death, and restore use the same mapping.
3. **High - newly materialized items had no authoritative row.** Movement performs an
   explicit system-to-current-custody creation command, waits for ACK, then resubmits
   the requested transfer using the committed revisions.
4. **High - death published the whole player inventory before durable custody.** Player
   corpses are created empty and durable subtrees move one at a time after ACK; a
   rejected submission leaves the prior character custody visible.
5. **Medium - Redis pickup/drop hints and relative event timestamps could veto or admit
   restore.** SQL, Redis-floor, and immutable-world restore now require an active exact
   `item_current_owner` match; the old records remain observability only.
6. **Medium - disconnect could strand an applied completion.** Pending entries contain
   only scalar data and retain completed results until login or reconnect calls the
   game-thread publication hook.
7. **Medium - currency objects entered the durable item chain during death.** Money
   keeps its existing currency/corpse path and is excluded from item ownership
   transfer, avoiding a durable coin owner row that loot would not consume.
8. **Low - isolated world recovery tests inherited a full SQL header dependency.** The
   narrow authoritative-owner declaration preserves the standalone harness boundary.

No unresolved blocking finding remains. Durable bulk get/drop/put intentionally fails
closed with one-at-a-time guidance because sibling operations would otherwise capture
the same owner revision; no legacy unacknowledged durable movement is used as fallback.
