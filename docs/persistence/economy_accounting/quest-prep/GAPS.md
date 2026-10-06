# Quest acceptance gaps — 2026-10-06

Accounting pin: `17c033d69316b21da8598791fc95cae79baa8dc2` (latest fetched
`origin/experimental-accounting` at prep start). PR #678 research head:
`55905eac1906cf59405764407f9d22497cccfff3`, branch `codex/discovered-zone-dailies`.
The finish plan's older reviewed research reference is
`beeb031030106a0c72e9321bbd1e2baaaea24259`; this pack uses the newer head.

## Existing coverage to reuse

| Existing executable | What it already establishes at its own evidence scope | Missing selected acceptance |
| --- | --- | --- |
| `test_static_quest_reward_journey.py` | Synthetic Lapney, three different item inputs, item/cash output, save and cold reconnect; flatfile | Real recipe quantities/order, authentic reset birth, active SQL accounting links |
| `run_quest_reward_ack_crash.py` | Synthetic offering-publication and XP-ACK crash; item UID, cash, XP, tombstones, closed obligation and second restart; guarded disposable SQL/flatfile | Real Kord terms, parent/child terminal retirement, later reward transfer/destruction, corrupted acknowledged receipt |
| `test_durable_quest_offering.py` | Extracted live helpers, publication/partial-root holds, missing recipient, solo/group/linkdead frozen XP, exact save receipts, paid-slot recovery | Production Q/G order, identical-looking kinds, repeated input roots, real birth binding and disappearing stock |
| `test_static_quest_reward_source.py` | Stable separate item-slot source identities | Actual creation/retirement and source-event witnesses in a native SQL journey |
| `test_world_quest_item_completion.py` | Extracted reward callback, actor/start/target and UID/custody checks, rejected grant effects | Genuine lost reply/cold publication and lifecycle evidence |
| `test_world_quest_dynamic_policy.py`, target/reward/XP tests | Cached target policy, bounded retries, feedback/math/reward ordering | Paid creation failure/refund and stale map/abandon attempt settlement |
| `run_world_quest_dual_backend.py` | Full-world paid task/queued abandon/new task and persisted task fields on MariaDB/flatfile | Active-epoch fee root/refund, frozen attempt, lost completion and restart; this existing journey is not an activation proof |

## Selected missing coverage

1. **QP01:** Tikitzopl's same-named sapphire kinds: exact four roots including consumed Orb; wrong-kind duplicates must preserve every root.
2. **QP02:** Gagga'Jobo three-hide backpack versus earlier unsupported paid recipes; actual reverse loader order, exact roots and fee refusal. Concrete source blocker on the pin.
3. **QP03:** Auriam's D completion: two fresh output UIDs, original birth/cash/stock, remaining lance retirement, absent versus reset replacement NPC and paired cleanup.
4. **QP04:** Dynamic bartender charged creation with no eligible target: exact debit and restitution or visible held obligation through lost reply/restart. Current active admission refuses; legacy refund helper is not durable restitution.
5. **QP05:** Darlene repeated skins, plus cap-one huge-skin fresh-world availability. Preserve three/two distinct roots and supplied-goods eligibility; do not weaken quantities or silently raise caps.
6. **QP06:** Kord's real mixed item/3,000-copper/XP rewards; lost ACK after later reward custody change must clean the original pair without repaying.
7. **QP07:** Dynamic bartender map/abandon settlement after task replacement. Same active flag cannot authenticate an attempt; map and abandonment must not affect the new task.

These seven cases are prep, not new product scope. QP01–QP04 are the first
priority specifications. Private birth/native12 source described by the latest
handoffs is not installed in the published pin. The pack must not invent a birth
binding, activation baseline, receipt or successful native journey to work around
that gap. Primary owns all producer/contract/schema/registry changes. Plan 5 owns
independent audit/restore. Integrated SQL-first qualification remains at the agreed
major-batch boundary; no full server or database batch ran during this survey.
