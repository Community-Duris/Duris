# Deferred legacy starter kits

Issue #181 covers first-entry legacy equipment and the intentional repeated grant
for an existing character with `toggle newbie` enabled and an empty carried
inventory. This includes existing Chaos characters; their legacy regrant is
separate from the standard Chaos new-character kit.

## Lifecycle

`load_obj_to_newbies()` reserves a creation queue by PID before making any PC
objects. A duplicate request sees the existing busy fence. The queue owns a
value-only preparation callback, rather than retaining a character or descriptor
pointer. Each pulse resolves the current live character by PID, including a
linkdead character. A fully absent player waits for a replacement incarnation.

The first preparation step captures the fixed legacy selection, weapon
eligibility, and first-circle spell decisions. Later steps instantiate one
detached root each from the boot-loaded object templates. No cold prototype load
or ownership submission happens in `enter_game()`. Existing item selection,
newbie keywords, cost, transience, and spellbook rules are retained. The NPC
equipment helper keeps its synchronous publication behavior.

Preparation runs once per game pulse, with at most eight steps per player and
32 steps globally. A round-robin queue prevents a busy group of logins from
starving later requests. Completion drains and player-ready notifications cannot
spend additional preparation steps. The admission queue and root count have
explicit bounds.

After preparation, the entire kit enters the existing multi-root creation
transaction. All roots stay detached until durable success. There is one
creation operation, rather than one journal/admission operation per root.
Transient admission refusal retains the complete staged kit for retry. A
preparation error or terminal transaction failure discards the whole staged kit.
Successful completion uses the existing ownership-revision checks and committed
graph reconstruction before publishing every root exactly once.

Independent gameplay creations arriving during preparation or the batch commit
wait in a separate trailing queue. They cannot enter the kit's payload and are
not discarded when the kit fails. They resume after the kit settles.

The recipient's normal prompt and command dequeue remain held while the kit is
pending, then resume after complete publication or failure. Other players remain
playable. Orderly maintenance sees the pending batch fence; a pre-entry
disconnect cancels only unsubmitted work and preserves an already journaled
batch. This does not make `toggle newbie` one-time or exempt Chaos characters.

## Verification

Run the focused queue and policy regressions with:

```sh
python3 tests/async/test_newbie_grant_lifecycle.py
python3 tests/async/test_newbie_kit_plan.py
python3 tests/async/test_newbie_kit_readiness_contract.py
python3 tests/async/test_creation_grant_batch_submission.py
python3 tests/async/test_creation_grant_reconciliation.py
python3 tests/async/test_item_transfer_creation_batch.py
```

The lifecycle harness links the production queue, ownership registry, capture,
and codecs under ASan/UBSan. It injects deferred preparation, global fairness,
duplicate requests, cancellation/re-reservation, offline replacement, delayed
admission, terminal failure, replay, and independent rewards queued behind both
successful and rejected kits.

The independent legacy-policy oracle checks 36,360 selections. The socket
journey uses real legacy prototypes, private flat-file state, two synthetic
accounts on distinct loopback addresses, and the production snapshot decoder:

```sh
python3 tests/async/test_flatfile_newbie_regrant_journey.py
```

It covers normal and Chaos Warrior/Sorcerer characters, two repeated grants per
case, new ownership identities, unchanged kit composition, saved transient
roots, native spellbook spell IDs/page counts, and no extra grant on a nonempty
relogin. A second player continuously runs `score`; the test measures actual
command-to-prompt responses. The regression runner schedules this journey in its
serial resource-intensive partition.

The same 28-root non-Chaos regrant took 14.016 seconds from preparation notice to
usable prompt on the original `115fe3bad` implementation, failing the journey's
eight-second guard. The deferred implementation took approximately 1.5 seconds
for 28 roots and 1.0 second for 23-root Sorcerer kits in the isolated local run.
These are local measurements, not a production latency guarantee. Login/account
authentication and save acknowledgements are separate from preparation timing.

The MariaDB item-transfer harness additionally exercises real multi-root
creation, revision refusal, replay, nested topology, and failure behavior:

```sh
# Only with an explicitly configured disposable local test database:
tests/async/run_item_transfer_schema_mysql.sh
```

No schema change or data repair is needed for this change.
