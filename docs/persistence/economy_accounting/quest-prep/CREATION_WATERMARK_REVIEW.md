# Creation-request watermark review — 2026-10-07

The actual creation callback can dispatch creation after an **injected** later
attempt advances quest_started and is reset. That component observation is not
proof of a reachable native stale-payment defect. The default fresh-world
configuration disables sharing, and the real currency input gate defers ASK.
A configured cross-actor share/reset sequence is a precise remaining review
candidate, not an implemented production correction or a durable refund claim.

## Current source and observed behavior

Source pins: actual build6db65f624836f150ed3dfe33508e1f1719145fdb, accounting
f04317d9d72aa5594448809baad6041936b09801, refreshed accounting
d91f59af06239a5736d10498b89091699c5e05c6 (qualification inputs unchanged).
Existing QP07 fix fd997ee4147ba58d835bf4bd61783b51307bc68c covers map/abandon.
Research PR67855905eac1906cf59405764407f9d22497cccfff3 remains unchanged.

The request initializer at `src/specs/specs.world_quest.c:431` captures
quest_started even for action quest. Its callback at163 checks active,
accomplished and remaining quota; it does not compare the captured watermark.
The actual resetQuest body at `src/world/world_quest.c:127` clears active/completed
state while retaining quest_started. Successful real creation and sharing advance
the watermark strictly through world_quest_next_started, including same-second
or clock rollback. No attempt identity/schema substitution is needed to describe
the question.

Owned diagnose_creation_watermark.py extracts the actual callback, request
initializer, reset and ADD_MONEY. The creation producer/history/SQL/debit
boundaries remain explicit stubs. It passes original-request, reset-only and
new-player watermark0 controls; observes producer dispatch after advanced101/reset
with original context100; preserves active/completed/quota guards, rejected debit,
invalid context/length/fee/giver/action and negative-watermark refusal controls.
It does not simulate an actual debit or install a persisted native task.

## Real reachability evidence and limits

1. `src/cmd/interp.c:1500` classifies CMD_ASK as currency-dependent. Current
   get_playing_cmd_from_q at `src/net/comm.c:1462` chooses the filtered queue when
   currency_transaction_player_busy is true. A second same-player bartender ASK
   is deferred until that boundary clears. CMD_QUEST is absent from this gate.
   The component gate proof below executes actual search/classification/dequeue
   bodies, with busy-state boundaries and the maintained reduced command table.
   It proves queue behavior given busy=true; it does not prove the entire debit
   lifecycle or the absence of every asynchronous interleaving.
2. Current source callers of createQuestForGiverVnum are the settlement callback
   and createQuest's wrapper. No independent gameplay caller of that wrapper
   was found in current src. Reset alone preserves the existing watermark and
   does not create B. These facts do not establish the proposed same-player
   second-ASK sequence while A remains busy.
3. `lib/duris.properties:2360` sets world.quest.share.max=0.000; the helper's
   default is0 and its clamp is0..4. do_quest rejects sharing immediately at
   `src/world/world_quest.c:741` under this default. The default fresh-world
   share/reset route is therefore unavailable.
4. If sharing is explicitly configured positive, an actual donor can share to
   an inactive recipient when receiver/shares/consent/level/quota/history checks
   pass. This path at747–873 advances the recipient watermark and copies task
   state. It does not check the recipient's currency busy/fence in that block.
   An already-consenting recipient could have creation A pending; the donor's
   command is a different actor. A trusted actor can then use actual
   `quest reset <recipient>` at663–684, clearing B while retaining its watermark.
   Neither other actor's input is gated by the recipient's ASK queue. If A's
   committed debit callback then arrives, with quota still available, the
   observed callback would dispatch creation again. This is a **source-supported
   conditional reachability candidate**, not a completed native journey.

No configured donor/recipient/trusted-reset/debit-held native journey was run.
No staff permissions, successful debit, active epoch or task receipt was
manufactured. Do not classify a fresh-world production defect from the injected
observation, or claim broad safety merely because the default shares property is0.

## Commands actually executed and results

In owned Docker quest-prep-runtime-20261007, `/current`, same pinned QA image as
FLAT_DROP_DIAGNOSIS.md:

```bash
python3 -B tests/async/quest_accounting_prep/diagnose_creation_watermark.py
python3 -B tests/async/quest_accounting_prep/test_creation_input_gate.py
python3 -B tests/async/test_currency_input_queue.py
```

First command **PASS**, current callback/reset component only. Second **PASS**:
actual dequeue defers ASK, allows QUEST, leaves ASK queued and later releases it
when busy clears. Both use C++20, Wall/Wextra/Werror; the queue harness retains
the maintained no-MySQL include pattern. No server/database journey follows.
Private outputs: followup-creation-component-current.out and
followup-creation-input-gate-current.out under the existing ignored evidence root.

Third command **FAIL**, unmodified shared runner: current extracted coin helpers
lack bulk_gets, corpse_bulk_get forward declaration, coins_to_string, writeCorpse,
finish_bulk_get_after_commit/finish_bulk_get and current accounted physical
publication/notification declarations. This is an obsolete/incomplete shared
test seam, not a failing queue-behavior assertion. No shared test was edited.
The owned queue subset avoids those unrelated coin fixtures; it does not claim
the full original suite passed. Earlier owned extraction/table/type-reader
setup errors are retained privately; final commands above are the passing inputs.

Hash bindings are printed by both owned tools. Relevant production hashes:
specs4d102b8ec12497bc0ce9c0c8bf170932c99495911f6a12d8871514f88b2563f4;
world questcef7f6e43527a34cefd750056ba7308702a526aec82ceb956ac7191b0dfe3517;
interp4f7dcf15adf0217193c9398c70c99d90ac59090f85f043b1b34e6d9be0df3dba;
comm2c884cfa5ec1644a33f40252af816cd6eccd10dff8dccbd32715eaedc4814303.
The unchanged shared queue runner hashes to
b61beae075c6bf0fba226f3b23eb59f124f7425418a3887d44a0fefef34192f6.

## Bounded owner handoff and disposition

The queued review is discharged at component/source scope with its limitation
explicit. Before selecting a production patch, the primary currency/lifecycle
owner should establish a supported held-debit fixture and determine whether
positive-sharing/trusted-reset is an enabled requirement. If applicable, execute
that real interleaving with authentic actors/history and preserve the creation
fee220 at level11/default cost, original request identity and exact task mutation.
For the previously executed Woodseer16553 level56 fixture the observed creation
quote was1120 copper; these are different fixtures, not interchangeable fees.

A proposed narrow continuation, **unimplemented**, is to compare the captured
creation watermark before producer/history effects while keeping genuine original
and first-player0 requests valid. A stale paid request would still require the
shared durable refund/held-obligation owner; generic ADD_MONEY refusal remains
the separate QP04 dependency. No automatic production correction, source proof
replacement or numeric-payment takeover follows from this review. The original
map/abandon QP07 acceptance remains qualified as previously published.
