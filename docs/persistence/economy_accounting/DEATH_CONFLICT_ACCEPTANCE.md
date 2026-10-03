# Death, custody conflicts, and connection release

## Release-blocking invariant

An accounting build is not gameplay-qualified merely because it compiles or a
happy-path corpse journey reports PASS. A dead player must not remain stranded
in the death/retry state because ownership and payload authorities disagree.
Qualification requires the original item identities and money to remain
recoverable and visible through the intended in-game recovery route, with
foreign ownership and account/soul bindings preserved.

The accounting feature remains unactivated. These requirements do not authorize
production migration, data repair, forced disconnection, or clearing custody
fences.

## Existing coverage limit

`tests/async/test_mysql_combat_journey.py` has a deliberate dispute fixture:

1. Complete a normal real-PC death, account-menu transition, corpse loot, save,
   and reconnect.
2. Insert a durable child custody record whose payload is absent, then reconnect
   and confirm the valid live object remains visible under the degraded loader.
3. Insert an additional `player_items` payload row after capture, outside the
   live corpse graph, and kill the character again.
4. The original fixture waited for `custody_payload_mismatch_rejected`, manually
   deleted the additional row, and then waited for the account menu and a
   committed death disposition.

That default case proves the integrity fence and recovery **after explicit
fixture repair**. It does not establish automatic reconciliation. Its output now
reports `manual_fixture_repair=True` instead of leaving that qualification
implicit.

## No-manual-repair probe

After providing the environment for a uniquely named, task-created, disposable
loopback database, run:

```sh
python3 tests/async/test_mysql_combat_journey.py \
  --server bin/server/dms_new --one \
  --require-unassisted-recovery \
  --evidence-dir /absolute/private/task-evidence
```

`TEST_DB_DISPOSABLE=1` is mandatory, in addition to the existing `TEST_DB_*`
connection settings. Pass generated fixture credentials through the process
environment, not logs, committed configuration, or command arguments. Do not
source a checkout `.env` or use a shared database.

For the guarded retained-conflict owner, the selected executable must be built
with `BUILD_PROFILE=development` (`TEST_MUD`). The probe's unassisted option sets
the existing disposable selector, but a production executable cannot select that
owner merely because the environment requests it. Keep the production and
development binary hashes and evidence separate. A production refusal/hold does
not qualify unassisted release and is not permission to remove the selector gate.

This probe leaves the injected payload untouched. It requires the account-menu
transition within the existing 45-second test budget and records whether that
transition occurred. It must not be described as passing when it times out,
when an operator/test edits the conflict away, or when extraction simply drops
unrepresented payloads. The current conservative retention assertion may only
be replaced by exact original-UID/payload read-back from a reviewed durable
reconciliation route; it is not permission to silently discard the row.

The fixture preserves full `server.out`, the complete runtime `logs` tree, the
immutable player/critical journals, and `custody-evidence.json` on both runtime
success and runtime failure when an evidence directory is supplied. The JSON
records source/binary/test identity, original UIDs, timestamped fixture phases,
SQL payload/custody/corpse observations, death receipts, wallet/death counters,
and whether manual repair was used. These are diagnostic observations, not a
claim that multiple separate SQL reads form one atomic snapshot. A blocked-state
observation is taken before closing the test client, including a log-byte
boundary, so cleanup cannot be mistaken for natural recovery. The database
itself remains disposable and is removed on exit.

## Required evidence review

- Bound the disputed-death log window by its recorded byte offset. An earlier
  character's recovery or an earlier healthy death must not close this failure.
- Read both the canonical persistence/WIZLOG file and player-save traces in
  server output. The diagnostic helper's bounded log tail is not the full
  evidence set.
- Correlate each custody mismatch or refused death with the same player's
  revision, original UID/root/parent set, and final authoritative receipt.
  `recapture_scheduled=1` and queue admission are not commit acknowledgements.
- Expected `corpse_items_in_flight`/retry messages are acceptable only when the
  corresponding operation reaches durable completion and the player reaches
  the terminal transition. Repeated rejection with no completion is a failure.
- Prove original items and money across save, reconnect and server restart.
  A retained quarantine/disposition record alone is not proof of player-visible
  restitution, and should be reported as incomplete coverage.
- Include foreign-owner conflict, same-owner revision/topology drift, missing
  payload and unrepresented payload cases in final qualification. This one
  probe does not claim coverage for every conflict class, pet raise, item
  creation, or item-flag spells.

## Repair boundary

Keep the pre-destructive payload and authority checks in place. A safe repair
must establish the winning authority or durably retain unresolved evidence
before releasing the terminal player state. Do not make this probe green by
removing guards, deleting surplus rows, inventing UIDs/owners, weakening binding
rules, or force-extracting a character whose durable outcome is unknown.
