# Batched corpse creation

A player death submits registered inventory roots and their nested contents in
one `corpse_create` ownership command. The committed batch validates the live
containment graph before publishing custody and moving any root. The final
corpse is written once, and completion advances the existing guarded death
finalizer to the next pulse. It does not extract the character inside the
coordinator callback.

Legacy roots without runtime ownership entries retain the adoption path before
the remaining registered roots are batched. Currency and item fences still
delay finalization while relevant work is pending. An admission refusal or
repository conflict preserves disputed custody for the durable death
disposition. A committed result with changed live containment retains its
publication and busy state until it can safely publish.

## Regression evidence

`python3 tests/async/test_corpse_creation_batch.py` compiles the actual admission,
codec, runtime registry, publication and completion paths with ASan/UBSan. It
covers 1, 15 and 100 roots with nested contents, one movement command and final
corpse write, duplicate completions, pending coin work, rejected commands,
stale parent/list topology, and event/fallback wake-up without inline extraction.

`python3 tests/async/test_mysql_combat_journey.py` creates a unique schema on a
disposable loopback MariaDB server and boots the normal MariaDB server binary.
Set `TEST_DB_HOST=127.0.0.1`, `TEST_DB_USER` and `TEST_DB_PASSWORD`; the runner
never reads the checkout's `.env` and drops only its own schema. It builds the
server by default; `--server bin/server/dms_new` reuses an explicitly selected
fresh build.

The MariaDB journey creates an account and character through the game protocol,
fights and loots an NPC, recovers the starter inventory, and dies with multiple
roots. It checks that all captured UIDs have corpse custody, their ledger rows
share one corpse-create operation, and the corresponding corpse item rows
exist. It then retrieves loot and coins through ordinary commands. A synthetic
durable child missing from the live item tree provokes the real repository's
`EMSGSIZE` refusal. At account-menu release the test requires the durable death
record, captured custody evidence, no remaining active player custody, and
exactly one increment to the death count. After restart, another login/save/quit
must preserve wallet/revision, death count, experience/level and evidence hash.
Variants cover default coins, reset-created coins and enabled boons.

`python3 tests/async/test_flatfile_combat_journey.py` exercises the corresponding
flatfile authority through the real server for the same three variants,
including refusal disposition and restart/re-entry conservation. Focused
death-custody, in-flight handoff, equipment/corpse-latency and corpse-persistence
contracts retain the existing finalization guards.

## Scope of the measurements

These are isolated synthetic worlds and freshly created test characters, not
production accounts. Reported attack-to-menu durations include combat and are
individual samples, not a production latency bound. The fixed-cost improvement
is one ownership transaction for a registered inventory batch; database delay,
pending rewards/currency and the final corpse snapshot still contribute.

Minimal-world boot intentionally skips MariaDB corpse restoration. The MariaDB
journey therefore checks durable corpse rows and live corpse recovery before
restart; its restart assertion covers player authority and disputed-death
evidence. It does not claim full-world SQL corpse restoration coverage. The
existing MariaDB login path uses normal login text, unlike the flatfile death
return message, so the test checks the persisted consequences directly.
