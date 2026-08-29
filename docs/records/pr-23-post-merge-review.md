# PR 23 Post-Merge Code Review

**Reviewed:** 2026-08-30
**Merge commit:** `d97304ac12b97370c193d7b6439b3e9b6b03387f`
**Review base:** `5359723fbde4c521b64ec33225a62aeef8817ad1`
**Branch:** `master`
Result: RESOLVED

## Scope

The review covered every changed file and hunk in the merge range. The range contains 37 files, 1,193 insertions, and 6,803 deletions. The repair diff was then reviewed independently.

Evidence:

- `git diff --stat 5359723fbde4c521b64ec33225a62aeef8817ad1..d97304ac12b97370c193d7b6439b3e9b6b03387f`
- `git diff --name-status 5359723fbde4c521b64ec33225a62aeef8817ad1..d97304ac12b97370c193d7b6439b3e9b6b03387f`
- `git diff --check`

## Resolved Findings

### High: container creation grants committed the wrong parent

`start_creation_grant()` submitted a null target container, so the ownership ledger committed a root inventory item. The completion then attempted an interactive `put()`, making live placement disagree with the durable parent and allowing a post-commit placement failure. The transaction now validates and submits the target container, and completion publishes directly with `obj_to_obj()` after the durable commit.

### High: death recovery could be canceled or dereference a removed corpse

The recovery event was attached to the corpse. Corpse extraction disarmed the event, and automatic raising could extract the corpse before event registration. The event is now character-owned with no corpse pointer. It also abandons extraction when the character is no longer dead.

### High: automatic raising could race the player-to-corpse handoff

Automatic undead raising could remove a player corpse while item ownership transfers were still pending. Player corpses are no longer raised automatically until their ownership handoff has drained.

### High: repeated divine claims could duplicate an in-flight reward

`summon_one()` called the low-level player publication path and immediately tested for a carried object. The new asynchronous ownership fence made that test fail before the grant completed, leaving an unrecorded queued reward that could be claimed repeatedly. The cooldown row is now reserved before explicit creation-grant submission, and immediate submission failure marks the reward recoverable and discards the staged object.

### Medium: item state was mutated after asynchronous snapshot capture

New-character keywords, restored soulbind flags, and holiday ownership metadata were assigned after `obj_to_char()`. With asynchronous creation grants, the durable snapshot omitted those mutations. These fields are now set before publication is submitted.

### Medium: shop completion could diverge from its durable parent

Committed shop purchases reran `put()` after the ownership commit. A closed or full container could reject the live placement even though the ledger already named that container. Completion now publishes directly into the committed destination. The legacy purchase path also validates container capacity before submission and delivers an already-paid item to inventory when the amount is invalid instead of destroying it.

### Medium: NPC description generation could free shared prototype memory

`generate_desc()` tested the `only.pc` union member directly. For NPCs, that aliases the NPC member and can be non-null, causing a shared prototype description to be freed. The replacement description is freed only for `IS_PC(ch)`.

### Medium: the admin nowhere-item list trusted truncated `snprintf()` lengths

The list offset was advanced by the required output length even when the write was truncated, allowing the next append to address beyond the buffer. The path now uses the capacity-aware `APPENDF()` helper and trims only a separator that was actually stored.

### Low: the ownership audit exposed the database password in process arguments

The read-only audit passed `DB_PASSWD` with the MySQL `-p` command-line option. It now supplies the password through `MYSQL_PWD` for the child process. The skip-cap heading was also corrected to match the query's strict greater-than condition.

## Validation

- PASS - formatting: `./scripts/format.sh --check`
- PASS - server build: `make -C src`
- PASS - full local gate: `make test-all` (336 Python tests passed, 0 failed; native signal-handler test passed)
- PASS - append scanner: `python3 scripts/scan-append-bounds.py` (no offenders)
- PASS - security source scan: `python3 scripts/security_source_check.py`
- PASS - dependency/security contracts: `python3 tests/async/test_security_dependency_baseline.py`
- PASS - patch hygiene: `git diff --check`
- PASS - focused ownership, death, reward, shop, and append contracts were run directly before the full suite and all passed.
- N/A - database/schema alignment: the repair changes runtime ordering, validation, publication, and tests only; it introduces no schema or migration change.
- N/A - GDPR: the repair adds no personal-data collection, logging, sharing, or retention behavior.
- N/A - UI surface review: no graphical or web UI was changed.

## Behavioral and Security Review

The highest-risk mutation paths were rechecked for duplicate submission, stale post-commit validation, failure handling, and contract alignment. Creation queues remain serialized per actor, death retries use bounded backoff, divine claims reserve their duplicate-prevention row before submission, and all immediate failure paths notify the player or emit an operational alert. No hardcoded credentials, new dependency, new external data transfer, or unescaped user input was introduced.
