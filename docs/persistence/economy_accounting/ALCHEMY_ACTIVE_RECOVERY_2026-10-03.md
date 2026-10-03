# Native alchemy accounting and recovery — #551

The isolated change starts from `experimental-accounting`, refreshed to
`a209af8277426a5fcd9cae475b6aec7a4bcbc3b4` for transport-integrated gameplay,
then refreshed to `2ec15b79e` to incorporate the delivered native COMMIT reply-loss
regressions. That second refresh changes tests and evidence only. It reuses the
delivered native craft root, exact custody
references, retained publication, pouch mutation, and Craft/Forge progression
receipt owner. It does not implement a second crafting system. #568 character
initialization, #661 NPC vial sources, and #490 global activation remain separate.

## Durable operation and publication

Assassin poison, Encrust success/failure and Harvester use native item craft
reason **34**, accounting `crafting_cost` **17**, and source kind `crafting` **8**.
The source lifetime is selected from consumed input UIDs rather than a provisional
output or process-local actor. The existing schema2 owner binds actor, recipe,
input identities/revisions, complete output snapshots, source claim, accounting
root, and every input/output custody reference in one SQL transaction or flatfile
authority commit. A zero-output failure still retires its selected inputs and
retains the publication obligation. Flatfile materialization needs no fabricated
output after-image for that case.

Alchemy extends the existing `craft_recipe` continuation with its own version2
layout: disciplines3–6, zero XP, exact output root count, and the frozen poison
notch result. Version1 Craft/Forge bytes and existing reason IDs remain unchanged.
The output limit remains64. Partial poison planning admits only completed
candidates; unsuccessful admission frees those provisional candidates and
preserves all authoritative ingredients.

Poison uses the production skill probability, eligibility, learning, cap and
timer rules to prepare a result before admission. Publication applies that
result without RNG. The existing progression owner persists skills, affects,
status and the operation receipt in the same player save, waits for that exact
save, and only then permits publication acknowledgement. Failed/coalesced saves,
offline owners, reconnect, terminal death receipt encoding, and cold receipt
recovery retain the operation identity and suppress repeated skill application.
Messages and Encrust audit output are selected from the committed continuation.

Migration **0054_alchemy_publication** additively widens the existing closed
receipt discipline CHECK and enforces zero XP for alchemy. It adds no table,
column or index and leaves existing migration files unchanged. Canonical,
staging0045 and master0031 histories advance to54 with their existing prefixes
preserved. The compiled runtime contract and normal migration verifier use the
same sealed head. The CHECK replacement is guarded and re-runnable on both SQL
engines; normal adoption/application/replay is required for each disposable SQL
gameplay database.

## Qualification boundary

All gameplay state, accounts, world fixtures and SQL schemas are synthetic and
isolated. The two SQL engines are dedicated MySQL8.0.46 and MariaDB10.11.19
containers. No configured game database, `.env` credentials, production service,
or player data is used.

`issue551_active_projection_fixture.cpp` is linked only into the qualification
binary. It installs a scoped admission projection over a real disposable active
epoch. Native craft storage, journals, source claims, root/reference writes,
player saves and recovery execute normally. SQL boot temporarily deselects this
synthetic epoch while the separate global activation loader runs, then selects
it before command recovery/gameplay and installs the projection. This is route
qualification under an active epoch; it does not claim #490's global census,
baseline witnesses, or authorization to activate accounting throughout the game.
There is no production activation bypass or operator configuration switch.

The same test-only link fixture refuses publication ACK to expose committed
recovery windows and freezes a selected Encrust failure roll before admission.
Poison notching and other random draws use the production generator. Hard kills
target only the fixture's own server process group, including the transport and
world processes. Copyover targets its persistent parent PID.

## Executable evidence

All scoped acceptance rows below pass locally. The three gameplay runs use the
persistent transport parent and its real world child. Exact SQL reconciliation
checks each observed operation independently of the command callback. Native
SQL and flatfile tests additionally qualify the version2 alchemy continuation
containing the retained pouch mutation on both success and zero-output failure.
Both maintained production server builds passed (`make -C src` with flatfile
and MariaDB persistence). The focused accounting and progression executables
passed with ASan/UBSan, all 52 writer coverage contract tests passed, and the
changed-line/touched-file formatting and whitespace checks passed.

| Passing qualification | Observed evidence |
| --- | --- |
| Flatfile, MySQL, MariaDB gameplay | Exact three-unit poison batch, retained ACK/crash recovery, same saved skill, retry without more outputs; exact Harvester inputs/orb after retained disconnect/reconnect; Encrust success and forced zero-output failure; copyover and two cold restarts |
| Independent SQL reconciliation | Each alchemy receipt joins one accounting root and source claim; exact reference UID sets equal consumed plus admitted UIDs; no unapplied receipt remains |
| Native SQL conservation | Both engines: exact source/root/references, stale/duplicate refusal, rollback, fresh-connection replay, COMMIT reply loss, zero-output failure, retained pouch identity/counters |
| Native flatfile conservation | Exact input/output/pouch references, replay, precommit refusal, separate-process interrupted journal/after-image recovery, zero-output failure |
| Production-function regressions | Poison 1/3/65 recipe plans, partial/zero allocation failures, refusal preservation; frozen notch probability/learning/caps/idempotency; active 1/2/64-output accounting plans |
| Durable player owner and restore | Failed/slow/coalesced save, exact ACK, reconnect fence, cold receipts, terminal death envelopes, corrupt/missing obligation/root/revision/checksum rejection |

Reproduce gameplay with `bash tests/async/run_issue_551_active_accounting.sh file`
or `... sql`. SQL requires explicit disposable loopback `TEST_DB_*` settings and
`TEST_DB_DISPOSABLE=1`; the runner creates/drops only its generated schema. Builds
use the maintained strict compiler flags. Qualification helpers and binaries
remain under ignored `bin/` or temporary directories.

Focused commands include:

```sh
python3 tests/async/test_issue_551_crafting_conservation.py
python3 tests/async/test_item_transfer_accounting.py
python3 tests/async/test_issue_551_flatfile_craft.py
python3 tests/async/test_craft_progression.py
python3 tests/async/test_craft_progression_restore.py
python3 tests/async/test_recipe_craft_transaction.py
python3 tests/async/test_publication_retention_runtime.py
python3 tests/async/test_item_transfer_version_compatibility.py
python3 tests/async/test_player_load_items.py
python3 tests/async/test_issue_551_runtime_state_save.py
python3 scripts/validate_runtime_compatibility.py
python3 tests/async/test_economy_writer_coverage_contract.py
python3 scripts/generate_economy_writer_coverage.py --check
python3 scripts/validate_economy_accounting.py
```

Writer coverage retains the unsupported player-potion writer retirement and the
separate NPC-vial refusal. The affected source anchors are refreshed against the
final task base. Custody-only craft evidence is not described as coin double
entry or market valuation. The wider accounting release matrix remains blocked
until its other writer and activation requirements are independently qualified.
