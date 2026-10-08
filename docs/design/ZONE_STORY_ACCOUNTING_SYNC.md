# Zone story research branch accounting integration

Date: October 7, 2026. Branch: `codex/discovered-zone-dailies`.
Accounting source: `experimental-accounting` at
`6d2bd242df08fbd74c97f9ae5a2dd1617ad8c1c9`.
The source build anchor is `99b2a13a4141e8d36ac0f695e9e31d556139b5d6`;
the subsequent upstream commits change only accounting documentation.
Previous research head: `8e4b6d9222259f187c62b64cf794df80ceed910d`.

This integration updates the research branch against the newer accounting
implementation so future zone investigation can use its current native quest,
ownership, recovery, and source contracts. It does not qualify the feature for
activation or replace the zone-specific journeys in the
[integration plan](ZONE_STORY_INTEGRATION_PLAN.md) and
[execution register](ZONE_STORY_ROADMAP_EXECUTION.md).

## Preserved world and research content

All 2,789 tracked area files from the previous research head remain byte-identical.
This includes the 232 authored journals and native quest data. This integration
contains no new world or quest repairs. Existing repairs retain their separate
commits and release-note records.

Zone dossiers remain useful source maps. Their older controller line references,
source pins, and descriptions of accounting limitations are historical evidence;
recheck those details against the integrated implementation when a zone is next
qualified. A research dossier does not establish that a live progression journey
or its reward recovery has passed.

## Reward and arrival compatibility

Both branches already used quest reward continuation version 6 for different
bounded shapes. Preserve both existing formats with their original root-count
distinction:

| Existing format | Discriminator | Preserved terms and behavior |
| --- | --- | --- |
| Zone-story daily offering | One or more consumed item roots | Original item reward source identity, frozen season/catalog/policy and daily recipients, per-recipient XP, ordinary obligation ACK and recovery |
| Accounting fee-only reward | Zero consumed item roots | Original `QRF6` action binding, retained acceptance result, native fee owner validation, fee reward source identity and ACK |

The shared decoder selects the shape after validating the bounded header. Each
decoder still requires its own complete tail and rejects malformed or mixed
records. Native item/cost continuation checks accept rooted daily records while
retaining their exact root ordering and native context checks. Fee-only paths
continue to require their own native ownership proof.

Fee-only records do not acquire a frozen daily-recipient extension in this
integration. The existing catalog still limits potential dailies to supported
item offerings; qualify expanded accounting quest types explicitly before
extending that policy or their durable format.

The catalog regression now recognizes accounting's existing native-owned NPC
reset exception: G/E/P item commands require regular SQL accounting and the
retained native birth owner. Ordinary room/object reset commands remain blocked.
This updates a historical test expectation; no reset or zone behavior is added
by the research integration.

Discovery and encountered-NPC hooks are retained alongside accounting's newer
native mobile restore and arrival behavior. Player-facing zone tracking still
requires accounting to be active and READY. Daily activation remains disabled by
default; this merge changes no runtime settings.

## Immutable migration histories

All three supported migration manifests retain the newer accounting branch's
first 64 entries exactly. The existing `0056_discovered_zone_daily_state` migration
is appended at sequence 65 with its original ID, SQL, verifier, and checksums.
Migration IDs are immutable identifiers; the numeric name is not its current
manifest sequence. Compiled and JSON runtime compatibility contracts now agree
on all three 65-entry histories and the resulting 230-table schema.

A research database that already recorded the older daily migration at sequence
56 has a different immutable history. The runner deliberately refuses it before
applying changes. Such a database needs a separately designed and verified
transition or a disposable research rebuild; do not edit migration receipts or
run a generic update over that history. An accounting database with the matching
64-entry prefix can append the daily migration through the normal guarded runner.
No database migrations were executed as part of this branch integration.

## Verification scope

The integration is checked with the maintained server build, runtime contract
validation, the immutable migration runner, actual reward codec/provider tests,
durable item offering callbacks, native arrival and accounting gates, the full
production catalog, and compiled journal coverage. Results and limitations are
recorded in the PR integration checkpoint.

Build outputs and retained evidence are under task-specific directories on D:.
Tests requiring Unix owner-only backup permissions use disposable Linux scratch
inside Docker's existing D:-resident disk. This keeps the permission requirement
intact instead of weakening it for a Windows filesystem.

This checkpoint does not rerun live player journeys, database crash/restart
journeys, accounting activation, or a full server burn-in. Earlier qualification
records describe their own historical commits and do not extend automatically
to this integrated head.
