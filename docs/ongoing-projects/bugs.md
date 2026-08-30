# phase 1 live test

Note some of these may be false positives or may have been already repaired.  They must be addressed carefully to not break working existing systems.

## (potential) BUGS & ISSUES

 -  VERIFIED FIXED (2026-08-30): password-hash temporaries now use matching `malloc`/`free`; the focused hash runtime test and an actual-character login/`score` under Memcheck completed with 0 errors.
 -  VERIFIED FIXED (2026-08-30): restored the framed, colored good/evil/neutral race-selection screen while preserving availability filtering and the restricted-race testing block; verified through live account creation after an actual-character login.
 `docs/ongoing-projects/1767242209173_x3drwi.webp`
 -  VERIFIED FIXED (2026-08-30): character-creation confirmations now allow 10 minutes instead of 60 seconds; after an actual-character login, a live creation session remained connected past the old cutoff and continued successfully.
 -  VERIFIED FIXED (2026-08-30): character creation now renders the authoritative rules file before agreement; focused catalog tests and a live actual-character check passed.
 -  VERIFIED FIXED (2026-08-30): chaos-mode actual-character login completed without ownership-authority grant rejections; the durable starter baseline and serialized grant regressions also pass.
 -  VERIFIED FIXED (2026-08-30): no delayed ownership-authority rejection appeared during a 150-second post-login chaos-mode watch, and the actual character remained responsive afterward.
 -  VERIFIED FIXED (2026-08-30): commune-capable non-dragoon characters now silently resume spell-slot recovery on login; a temporary actual druid communed without issuing the command.
 -  VERIFIED FIXED (2026-08-30): account login now displays the tracked news before the MOTD; verified through an actual-character login and focused account/flat-file tests.
 -  VERIFIED FIXED (2026-08-30): positive carried-coin rewards now serialize and rebase behind in-flight wallet work instead of falling back to auction pickup; two immediate live 10k-platinum grants to an actual character produced no fallback message and durably recorded 20k platinum across two ledger entries.
 -  VERIFIED FIXED (2026-08-30): `auction pickup` accepted a live staged 1-platinum claim without the busy response, credited the actual character, cleared the staged balance, and recorded the durable auction-claim ledger entry; no further code change was required.
 -  VERIFIED FALSE (2026-08-30): "no NPCs loaded with items." A full-world game-loop audit found 13,433 NPCs with inventory or equipment (9,663 equipped); no code change was required.
 -  VERIFIED FIXED (2026-08-30): unpaid locker entry now captures the bank exit before the temporary locker is released and leaves cleanup to the normal locker hook; an actual zero-balance character received the payment rejection and was returned to the named Tharnadian Bank instead of an anonymous locker room.
 -  VERIFIED FIXED (2026-08-30): linkdead battle deaths now publish corpse-item completions against the live in-world character instead of waiting on a descriptor; all 28 actual-character items reached one corpse, death recovery completed, and the character logged back in alive without the save warning.
 -  VERIFIED FIXED (2026-08-30): corpse creation now defers extraction while its item handoff completes, after which the death save retries; the live linkdead battle test ended with one durable 28-item corpse and one saved death rather than an unsaved character/corpse split.
 -  VERIFIED FIXED (2026-08-30): `suicide` now rejects an already-dead character before confirmation or `die()` can run; the actual-character retest returned "You are already dead" and advanced durable state by exactly one battle death and one corpse.
 -  VERIFIED FIXED (2026-08-30): salvage eligibility now excludes corpses; live examination of the actual character's corpse reports that it cannot yield usable salvage materials.
 -  VERIFIED FIXED (2026-08-30): an actual-character disconnect/relogin during deferred death remained dead until the terminal retry completed, then returned to the account menu; it did not revive before the death was saved.
 -  VERIFIED FALSE (2026-08-30): an actual character crossed adjacent ASCII world-map road tiles and the prior tile remained `+`; road sectors explicitly do not create tracks, and no terrain mutation occurred.
 -  VERIFIED FIXED (2026-08-30): track objects now retain their creator and the world map ignores the viewer's own tracks; with improved tracking enabled, an actual character crossed adjacent forest tiles and the vacated tile remained `*` instead of becoming `.`.
 -  VERIFIED FALSE (2026-08-30): an actual level-56 druid recovered all 84 spell slots through passive communion in 181.91 seconds without a stall (longest gap: 5.25 seconds); the brief cadence changes match the intentional random 33% recovery acceleration.
 -  VERIFIED FIXED (2026-08-30): bare `list` at a shipyard now defaults to the purchasable hull catalog; an actual character at Quietus Quay received the ship list beginning with Sloop.
 -  VERIFIED FALSE (2026-08-30): at a valid inn, an actual character carrying `0/11` items and wearing nothing rented successfully and returned to the account menu; the reported save failure did not occur.
 -  VERIFIED FIXED (2026-08-30): manual saves now distinguish queued and already-queued requests, then report database acknowledgment or failure; an actual character received `queued`, `already queued`, and `complete` in sequence.
 -  VERIFIED FALSE (2026-08-30): the live Human class menu displayed lettered choices, and selecting the shown `w)` key chose Warrior and advanced to alignment without requiring the class name.
 -  VERIFIED FALSE (2026-08-30): the live Human class menu explicitly listed `d) Druid`, and selecting `d` accepted Druid and advanced to hometown selection.
 -  VERIFIED FIXED (2026-08-30): declining the final keep-character prompt now frees the uncommitted character and returns to the existing account menu; the complete live creation/discard flow no longer falls into legacy name login.
 -  VERIFIED FIXED (2026-08-30): room-level track output now excludes tracks owned by the viewer; with improved tracking enabled, an actual character returned along its route without seeing its own track message.
 -  VERIFIED FALSE (2026-08-30): a newly created actual character received 21 starter items, all 21 persisted through a clean shutdown/restart, and the same character as a level-30 Wizard materialized its innate spellbook; the full-world audit also found 13,433 item-bearing NPCs.
 -  VERIFIED FALSE (2026-08-30): an actual level-30 Sorcerer at the correct teacher was offered Wizard, specialized successfully, and received the complete response within 250.3 ms; the specialization also persisted through a clean shutdown.
 -  VERIFIED FIXED (2026-08-30): `logit()` now creates a missing parent-directory chain after `ENOENT` and retries the same append once; the focused runtime test wrote the original marker through the real logger into a previously absent nested path.
-  VERIFIED FIXED (2026-08-30): starter-kit creation now withholds the gameplay prompt and queued commands until every serialized ownership grant finishes; an actual new Thri-kreen Warrior received readiness at 16.991 seconds and its queued first inventory then showed the complete 33-item kit instead of an intermediate 5- or 21-item kit.
-  VERIFIED FALSE (2026-08-30): actual new neutral and good Human Illusionists selecting Tharnadia both entered `Inside a Twisted Twilight Tower of Illusion`; each durably stored room `132501` as hometown, birthplace, and original birthplace rather than falling back to Cage of Smoke.
