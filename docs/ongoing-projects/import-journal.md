# Legacy `prod.sql` import journal

## Scope and safety contract

- Requested: import `/home/duris/prod.sql` into the game's database without
  destroying existing data; record anything that does not import cleanly.
- Journal started: 2026-09-03 UTC.
- Canonical journal: this file. It incorporates the DurisWeb-side production
  acceptance record and the later DurisMUD persistence/materialization audit;
  the former DurisWeb-local copy is no longer the authoritative record.
- This file contains no credentials or row-level private data.
- Target resolved from `duris/.env`: production, loopback
  `127.0.0.1:3307/duris`, explicitly allow-listed.
- Initial service state: `duris-mud-production.service` and
  `durisweb-production.service` were both active.
- Source file mode is `0600`; size is 345,366,975 bytes.
- Verified source SHA-256:
  `8f882d16be743b42467890f7714052d64b1cf8eca75be89c77efaeb3d7e5cc44`.
  It is byte-for-byte the same dump covered by
  `duris/docs/persistence/LEGACY_DUMP_IMPORT.md`.
- Initial read-only target inventory: MariaDB 10.11.14, 249 base tables,
  immutable migration head 0008, 82 recorded DurisWeb migrations, 7 accounts,
  12 account-character rows, and 12 player rows. Eleven other target
  connections were active.
- The target identifies its MUD baseline as `fresh_bootstrap` and contains no
  `legacy_import_*` preservation archives. Therefore the documented prior
  reference import into a development database is not evidence that this live
  target already contains the dump.

## Critical finding

`duris/scripts/import_legacy_dump.py` is a guarded **replacement** importer,
not a merge tool. It backs up the target, drops all target tables/views/routines/
events, restores the dump, and migrates it. It deliberately rejects production.
Using or weakening that path here would violate the requirement to preserve
existing data, so it has not been run against the game database.

## Planned non-destructive path

1. Fingerprint and inspect the source offline.
2. Inventory the live target read-only and determine whether this exact dump was
   already imported by the documented 2026-08-31 process.
3. Restore and migrate the dump only in an isolated staging database.
4. Take and validate a fresh owner-only production backup before any merge.
5. Quiesce all writers, compare every table/key/row class, and insert only rows
   with a proven non-conflicting mapping. Never use blanket `REPLACE`,
   `INSERT ... ON DUPLICATE KEY UPDATE`, truncate, drop, or disable integrity
   safeguards on production.
6. Leave ambiguous/conflicting rows unchanged and record them below.
7. Verify both the MUD runtime contract and DurisWeb behavior before restarting.

## Import results and exceptions

### Staging attempt 1

- Result: safely failed during legacy migration step 143, before immutable
  migration application.
- Cause: the copied production environment retained `REDIS=TRUE`; the migration
  correctly refused its Redis cleanup because the isolated SQL target used
  `ENVIRONMENT=development`, not `local`.
- Recovery: the guarded importer automatically wiped the partial staging schema
  and restored its pre-attempt empty backup. Verified afterward: zero staging
  tables and zero staging connections.
- Impact: no production SQL or Redis mutation. Retry will set `REDIS=FALSE` in
  the staging-only environment; Redis is irrelevant to an offline SQL import.

### Staging attempt 2

- Result: all 143 legacy migration steps completed with Redis disabled, and the
  immutable runner advanced the isolated copy, but the final runtime verifier
  rejected its normalized metadata fingerprint.
- Expected MariaDB fingerprint:
  `92dd682008ba94f8aecc63595dc46f9d6f1f865adecd174e58e2a2ce14220f2c`.
- Observed staging fingerprint:
  `65932399e1d39ce1d11bcdb2be005d1da2312472670dd2239b24020db91d0aa5`.
- Recovery: automatic rollback again restored the staging schema to its empty
  pre-attempt state.
- Production comparison: the current live database passes the same read-only
  runtime verifier. The mismatch is isolated to legacy-dump convergence and
  will be diagnosed in staging; the verifier will not be weakened.

### Staging qualification

- Root cause of the fingerprint mismatch: the legacy migration sequence left
  one obsolete foreign key on `user_profile_stats.account_name`. No migration
  was bypassed or altered.
- The isolated legacy database was then advanced through the eight current
  DurisWeb migrations that post-date the dump. Those migrations removed the
  obsolete metadata and brought the isolated copy to all 82 recorded web
  migrations.
- Result: the current MUD runtime compatibility verifier passes unchanged on
  the staged legacy data. The qualified stage contains 257 base tables and
  1,773,796 rows (aggregate counts only).
- Eight tables exist only in the qualified legacy stage:
  `builder_notifications`, `forum_notifications`,
  `legacy_import_player_item_extra_descr`,
  `legacy_import_player_pet_item_extra_descr`,
  `legacy_import_server_reboots`, `players_core`,
  `prepstatment_duris_sql`, and `siege_objects`. Final disposition: the three
  `legacy_import_*` tables were copied as inert preservation archives;
  `forum_notifications` (2 obsolete rows) was explicitly skipped; the other
  four tables are empty and were not added to production.

### Candidate merge preparation

- Created an owner-only, transaction-consistent snapshot of the live target at
  `duris/tmp/legacy-import-work-20260903/production-analysis-snapshot.sql`
  (mode `0600`) without stopping or mutating production.
- Restored that snapshot into isolated database
  `duris_import_candidate_20260903`. It is the candidate on which the merge
  will be exercised first; production is still untouched.
- Source/target schemas share 249 tables. Four need special treatment:
  `account_login_history` and `forum_categories` differ only in column order,
  while `admin_action_log` and `wipe_history` have incompatible column types or
  shape. Inserts will always name columns explicitly; the two truly
  incompatible tables are excluded.
- A preliminary unique-key collision audit found 328 structurally new accounts
  and initially counted 773 structurally new player rows. The latter was an
  intermediate result and was superseded by the final guarded selection and
  verified production count of 783 recorded below. Existing target rows win
  every collision; source rows are never used to update or replace them.
- Legacy item identifiers require quarantine rather than a blanket copy. Across
  the three largest item payload tables, 38 rows have a missing/zero UID and
  23,366 rows are excess occurrences of duplicated UIDs. No nonzero legacy UID
  overlaps a current production UID. Only globally unique, nonzero UIDs with a
  valid and unambiguous ownership/container chain will be eligible; ambiguous
  rows and their descendants will be skipped and counted. The live UID
  allocator will only be advanced monotonically if eligible legacy items are
  ultimately inserted.
- Legacy `pages` contains 15 rows whose title is absent from the snapshot, but
  only 14 distinct such titles. Because title is not a database unique key,
  this source-side duplicate remains pending semantic review rather than being
  silently deduplicated.

### Candidate merge qualification

- A second frozen copy of the production snapshot was restored as
  `duris_import_reference_20260903`. It is never a merge target and provides a
  row-for-row preservation oracle for the candidate.
- Candidate attempt 1 stopped before canonical commit because
  `notifications.data` uses the current web schema's binary JSON collation,
  while the migrated source uses the database collation. The source values are
  valid `utf8mb4` JSON; the candidate now uses one explicit, lossless collation
  conversion for that column. Raw preservation archives created before the
  failed transaction were discarded with the disposable candidate rebuild.
- Candidate attempt 2 stopped before canonical commit because the fail-closed
  inventory found five non-empty tables without an explicit decision. The four
  stale corpse tables were classified as skipped; the append-only
  `zone_touches` history was classified for surrogate-ID remapping. The
  disposable candidate was rebuilt again.
- Candidate attempt 3 stopped before canonical commit because a validation
  query detected 12 missing combat-frag baselines. Those 12 were proven to be a
  pre-existing condition in the frozen production snapshot. The merge creates
  and validates baselines for every imported character but does not rewrite
  current characters merely to repair an unrelated prior condition.
- The final candidate run completed. The run is transactionally marked by dump
  SHA-256 so a repeat cannot duplicate remapped append-only history.

#### Candidate rows accepted

- Identity and character state: 328 accounts, 783 players, 783 exact
  account-character mappings, 119 account banks, and 673 account/IP rows.
  Opening currency, epic, and combat-frag baselines were created for every
  imported player; bank baselines were created for all 119 imported banks.
- Player components include 20,307 affects, 17,239 languages, 20,542 skills,
  1,259 timers, 673 undead-slot rows, 192 spellbooks, 57 recipes, 183 epic-bonus
  selections, 54,422 epic-gain rows, 764 leaderboard rows, 779 IP-state rows,
  14 offline messages, 363 progress rows, 5,160 world-quest rows, and 19,663
  shop-trophy rows.
- Items: 36,993 player items and 31,253 locker items passed the global UID,
  ownership, artifact, and container-ancestry checks. All 38 pet items were
  retained by allocating fresh UIDs above every legacy/current UID. Matching
  canonical item metadata was copied, and 68,284 baseline/current custody rows
  plus 715 owner-revision rows were created. The allocator moved monotonically
  from 25,000,001 to 53,582,089 in the candidate. The largest imported player
  inventory is 1,159 items, below the runtime limit of 4,096.
- Lockers and ships: 115 lockers, 136 chests, 71 access grants, 6 chest-log
  rows, 216 ships, 864 ship-armor rows, 216 crew rows, and 3,456 slot rows.
- Community/content: all 107 forum categories, 9 threads, 11 posts, 5 images,
  5 reactions, and 9 subscriptions; 1,674 notifications; 3 user profiles; 4
  website changelog rows and 99 reads; 5 missing `mud_info` entries; and 14
  source-only help pages (one row chosen for each distinct missing title).
- Append-only history: 158,139 log rows, 256,292 statistics rows, 195,092 page
  views, 2,180 visitor sessions, 54,037 health metrics, 648 normalized reboot
  rows, 505 zone-touch rows, 187 PK events, 802 PK participant rows, and 2 PvP
  comments whose referenced battle survived. Surrogate IDs are newly allocated
  only after proving the source history ends before the current history begins.
- Preservation archives copied verbatim: 289,015 raw player-item-description
  rows, 441 raw pet-item-description rows, and 648 raw reboot rows.

#### Candidate conflicts, normalization, and quarantine

- Current target rows always win: one account, one bank, one account/IP row,
  two player rows, three `mud_info` entries, and all 1,738 overlapping help-page
  titles were left unchanged. Twenty-nine considered player-skill rows also
  collided with current target keys and were left unchanged. Ninety-two
  account-character rows were not exact mappings to a newly accepted player;
  the one duplicate missing help-page title was not inserted twice.
- Current world association/guild definitions do not match the old database.
  Rather than attach players to the wrong guild, 176 imported nonzero
  association IDs and all 190 nonzero guild-status bitsets were reset to zero.
  Semantic reconciliation is tracked in
  [DurisWeb #16](https://github.com/LuminariMUD/DurisWeb/issues/16) and the
  MUD-owned tooling issue
  [DurisMUD #127](https://github.com/LuminariMUD/DurisMUD/issues/127).
  Seven forum-thread and nine forum-post character IDs that no longer identify
  an imported character were cleared, while their account-attributed content
  was preserved. Two imported PvP comments had unusable participant pointers;
  those pointers were cleared.
- Item evidence covered 107,172 rows in all UID-bearing payload tables. It
  contains 7,293 duplicated-UID groups (23,332 excess occurrences), affecting
  30,482 otherwise owner-eligible player/locker rows; 477 eligible rows use an
  artifact vnum; and 6,610 further rows have ancestry that reaches excluded or
  ambiguous evidence. Consequently 13,246 player-item payload rows and 25,011
  locker-item payload rows were not activated. Their associated canonical
  metadata was also left out. Raw descriptions remain in the preservation
  archive and the original dump/stage remains intact. Recoverability and final
  disposition are tracked in
  [DurisWeb #15](https://github.com/LuminariMUD/DurisWeb/issues/15) and the
  MUD-owned tooling issue
  [DurisMUD #126](https://github.com/LuminariMUD/DurisMUD/issues/126).
- Three lockers and three chests were excluded for target identity conflict or
  obsolete association ownership; two locker-access grants then lacked an
  accepted visitor. Two ships belonged to conflicted players. The source also
  contains 1,236 ship-armor rows, 309 crew rows, and 32 slot rows without an
  accepted ship parent; foreign-key checks remained enabled and rejected no
  accepted row.
- Thirty-seven PvP comments reference PK events absent from the source dump and
  were left out rather than attached to a same-numbered unrelated event. Their
  recovery/archive decision is tracked in
  [DurisWeb #17](https://github.com/LuminariMUD/DurisWeb/issues/17).

#### Whole-table policy skips from the non-empty source

- Current authority/control: `account_bound_reward_pwipe_state` (1),
  `item_uid_allocator` (1; handled by monotonic advance), `level_cap` (1),
  `season_reset_state` (1), `mud_process_state` (1), `mud_schema_baselines` (1),
  `mud_schema_history` (8), `mud_schema_migration_state` (1),
  `mud_schema_migrations` (1), `knex_migrations` (82), and
  `knex_migrations_lock` (1).
- Security/admin/session: `admin_account_roles` (5), `admin_action_log` (142;
  also schema-incompatible), `admin_permission_audit` (5),
  `admin_permissions` (28), `admin_role_permissions` (29), `admin_roles` (4),
  `multiplay_whitelist` (2), `player_granted_cmds` (1),
  `suspicious_accounts` (14), `terminal_logs` (21,742),
  `terminal_sessions` (68), and `web_sessions` (195).
- Live transactional/temporary state: artifacts/binds (96/56/85 rows across
  `artifacts`, `artifacts_mortal`, `artifact_bind`); auctions and their old
  pickup/bid rows (249/248/30/67); boons and progress (46/1,835); corpses and
  their item payload/metadata (24/114/27/61); shopkeepers and their active
  state (70/517/161/842); and `server_incidents` (71, potentially unresolved or
  public).
- Current world/static/generated state: `associations` (3), `guilds` (3),
  `guild_members` (163), `guild_ranks` (24), `classes` (31), `races` (101),
  `zones` (355), `timers` (4), ship cargo prices/modifiers (200 each),
  `builder_flags` (590), all older wiki cache tables (409,254 rows in total),
  `wiki_settings` (2), and `forum_settings` (10). Supported publication of the
  currently empty object/mob generations remains tracked in
  [DurisWeb #8](https://github.com/LuminariMUD/DurisWeb/issues/8) and
  [DurisWeb #9](https://github.com/LuminariMUD/DurisWeb/issues/9).
- Operational/workflow data not portable to this installation:
  `deployment_log` (31), `gemini_analysis_log` (5), `mud_backups` (19),
  `help_file_suggestions` (48, superseded page IDs), and the obsolete
  stage-only `forum_notifications` (2).

#### Candidate verification

- Current runtime metadata contract: pass (174 required tables, immutable head
  0008).
- DurisWeb migrations: 82 completed, zero pending.
- Frozen-row proof: all 233,656 pre-existing rows across all 249 original
  tables remain byte-for-byte present. The only expected existing-row change is
  the monotonic `item_uid_allocator` advance. Three unkeyed static tables were
  also proven unchanged by count and database checksum.
- Foreign-key audit: all 123 declared relationships have zero orphan rows.
- Currency, epic, combat, item, artifact/guild, boon/zone, and auction
  reconciliation queries all report zero mismatches.
- Item topology: zero imported mismatch and zero pet mismatch. The checker sees
  14 mismatches, all 14 proven byte-for-byte pre-existing in the frozen target;
  none was introduced by this merge.

### Final production preflight

- Both production writers were stopped cleanly. Verified immediately before
  backup: MUD inactive, web inactive, and zero remaining connections to
  `duris`.
- Fresh owner-only backup:
  `duris/tmp/legacy-import-work-20260903/production-premerge-20260903T185345Z.sql`
  (mode `0600`, 54,129,150 bytes), SHA-256
  `227269e7ea6d3d61c01060d00bb645f6d4ea8aecb94ecccd05fa2e1f7d2638e1`.
- MariaDB's logical dump serialization rounded 110 single-precision
  `zones.frequency_mod` values on a test restore. The maximum observed delta
  was approximately 0.0000023842; no live value changed. An owner-only recovery
  companion was therefore created at
  `duris/tmp/legacy-import-work-20260903/production-premerge-20260903T185345Z-zones-float.sql`
  (mode `0600`, 23,760 bytes), SHA-256
  `84fbede59e52c6f6992cd82b61c968c712ed2ceaeeae853295e4879bbedd64cb`.
- The main backup plus companion were restored into
  `duris_import_final_reference_20260903`. All 235,313 live pre-merge rows in
  all 249 tables then matched the recovery copy exactly. This final reference
  will also be used to prove the merge did not change or remove an existing
  row.
- Live pre-merge runtime contract passes, DurisWeb has 82 completed and zero
  pending migrations, and the production database still has zero connections.

### Production merge and verification

- Result: **completed successfully**. The canonical production transaction
  committed at `2026-09-03T18:56:36.618227Z`. Its durable
  `legacy_import_runs` marker contains the exact source SHA-256, source size,
  import version `legacy-target-wins-v1`, and valid aggregate-only JSON report.
  The committed production counts match the qualified candidate counts above.
- The merge used insert-only, target-wins DML. It performed no delete,
  truncate, replacement, or blanket upsert. The sole existing-row operation
  was the guarded monotonic `item_uid_allocator` advance from 25,000,001 to
  53,582,089; its automatic `updated_at` timestamp advanced with it.
- Immediately after commit and before either writer restarted, production had
  253 base tables and 1,619,901 rows. The only four tables added to the original
  249-table schema are `legacy_import_runs` and the three inert preservation
  archives. Archive counts exactly match their source counts: 289,015 raw
  player-item-description rows, 441 raw pet-item-description rows, and 648 raw
  reboot rows.
- Frozen-row proof against `duris_import_final_reference_20260903`: all 235,313
  pre-merge rows across all 249 original tables remain present with identical
  values. The two nonempty unkeyed static tables also match by exact count and
  extended database checksum. There were zero preservation failures after
  accounting for the explicitly allowed allocator value/timestamp advance;
  an empty pre-merge `epic_bonus` table now legitimately holds 183 imports.
- Current MUD runtime compatibility passes unchanged (174 required tables,
  immutable head 0008). DurisWeb still has all 82 migrations complete and zero
  pending. `mysqlcheck --check --silent duris` passes.
- All 123 declared foreign-key relationships were audited dynamically: zero
  orphan relationships and zero orphan rows. Currency, epic, combat, item,
  artifact/guild, boon/zone, and auction reconciliation checks have zero
  unexpected mismatches. The known 12 current characters without a combat-frag
  baseline are all proven pre-existing; imported characters introduced zero
  missing baselines.
- Official item-nesting evidence has zero invalid roots. Its 14 drift rows are
  all proven pre-existing in the frozen reference; imported rows account for
  zero. All 38 imported pet items match their payload parent/root, current
  owner, ownership baseline, owner identity, vnum, state, and revision.
- Post-restart verification: `duris-mud-production.service` and
  `durisweb-production.service` are both active/running with zero restarts. The
  MUD health endpoint reports `healthy` with persistence `ready`; the web health
  endpoint reports `ok` with database and cache both `ok`. A second probe passed
  for each service. Production still exposes one valid marker for this dump,
  783 imported players, and 68,284 imported item-ownership baselines.
- Recovery and forensic material was deliberately retained rather than
  cleaned up: the original dump, fresh main backup and exact-float companion,
  qualified stage, disposable candidate/reference databases, final frozen
  reference, and merge script remain available. The production recovery pair
  is:
  `duris/tmp/legacy-import-work-20260903/production-premerge-20260903T185345Z.sql`
  plus
  `duris/tmp/legacy-import-work-20260903/production-premerge-20260903T185345Z-zones-float.sql`,
  with the SHA-256 values recorded in the preflight section above.

Anything not imported smoothly is recorded in the staging attempts, conflict/
normalization/quarantine section, and whole-table policy-skip inventory above.
No ambiguous legacy row was used to overwrite current production data.

## Post-import DurisWeb application and public-runtime audit

Audit window: 2026-09-03 19:03-19:55 UTC. This follow-up was performed after
the production transaction and writer restart. All database inspection in this
section was read-only; no import was replayed, no migration was run, and no MUD
database row was changed.

### Outcome

- The committed import remained relationally consistent. Its marker, aggregate
  report, accepted identities, ownership baselines, preservation archives, and
  current foreign-key graph passed the checks below. The later cross-layer MUD
  audit recorded in the next major section found semantic and materialization
  defects that these relational checks could not detect.
- The public website outage was not a database or application failure and did
  not require a host, MUD, or web-application reboot. The origin on loopback
  port 7770 was healthy, but `durisweb-cloudflared.service` had been stopped for
  import maintenance at 18:53:38 UTC and was omitted from the 19:02 writer
  restart. Cloudflare consequently returned HTTP 530/error 1033 while local
  `/health`, `/`, and `/api/site-config` continued to work.
- Starting only `durisweb-cloudflared.service` at 19:24:02 UTC restored public
  HTTP 200 responses. The tunnel registered four connections and loaded the
  expected `duris.sbs`/`www.duris.sbs` ingress routes. Neither the MUD nor the
  web application was restarted to resolve that outage.
- The audit also exposed a separate schema-drift defect in the forum profile
  projection: every existing profile route, including both imported and
  pre-existing profiles, returned HTTP 500 because `getUserProfile` read the
  removed `frag_leaderboard.money` and `frag_leaderboard.balance` columns. This
  was a code defect, not malformed imported data. The query now reads canonical
  wallet and bank denominations from `player_data`, using the same copper
  conversion already used elsewhere in the application. A regression contract
  was added. The remaining review and durable-test work is tracked in
  [DurisWeb #7](https://github.com/LuminariMUD/DurisWeb/issues/7).
- The profile repair was compiled, passed the production preflights, and was
  released with a deliberate DurisWeb-only restart at 19:48 UTC. The MUD PID
  remained unchanged. All seven current profile routes then returned valid
  HTTP 200 responses both locally and through Cloudflare.

### Durable import evidence

- `legacy_import_runs` still contains exactly one marker. Its SHA-256, source
  size (345,366,975 bytes), import version (`legacy-target-wins-v1`), completion
  time (`2026-09-03T18:56:36.618227Z`), and JSON report all validate.
- The marker report classifies 70 imported tables. For every one, the qualified
  stage count still equals `source_rows`; there are zero source-count
  mismatches. Every current production table count is at least the report's
  committed `inserted` count; there are zero below-import-count tables. Normal
  live append and lifecycle activity was not mistaken for import drift.
- A fresh dynamic audit of all 123 declared foreign-key relationships found
  zero violating relationships and zero orphan rows.
- The three preservation archives retain the expected counts and continue to
  match the stage/candidate checksums. Current checksums are 4,097,517,792 for
  raw player-item descriptions, 1,358,063,138 for raw pet-item descriptions,
  and 1,842,988,780 for raw reboot rows.
- All 329 source account names are present. The 328 inserted account passwords
  match their source bytes; the single name collision retained the target
  password. All 335 current account passwords have a recognized bcrypt prefix.
  No password or account identifier was printed during this audit. Retaining
  the target password did not, however, prove that the colliding source and
  target accounts had the same human owner; the later identity-boundary audit
  below addresses the nine imported characters attached to that account.
- All 783 accepted imported players retain the exact account-character mapping
  present in the qualified candidate: 687 mappings are selectable and 96 retain
  a non-null deletion marker. Wallet, epic-balance, and combat-frag opening
  baselines exist for all 783; bank baselines cover the 119 accepted imported
  banks. Accepted component counts match the marker report, including the
  expected 29 target-wins skill collisions.
- Twelve active, account-mapped target characters still lack a combat-frag
  baseline. The frozen reference proves that all 12 predate the import, while
  wallet and epic baselines are complete. Targeted repair is tracked in
  [DurisWeb #13](https://github.com/LuminariMUD/DurisWeb/issues/13) and the
  MUD-owned implementation issue
  [DurisMUD #124](https://github.com/LuminariMUD/DurisMUD/issues/124).
- The 96 non-selectable imported mappings are preserved tombstones: the
  qualified candidate contains the same deletion markers. They are intentional
  retained candidate state, not post-commit loss or missing mapping rows.

### Item, container, and ownership follow-up

- The live payload at this audit point contained 37,401 player items, 31,253
  locker items, and 38 pet items. Across those 68,692 rows there were zero
  null/zero UIDs and zero duplicate UID groups or excess occurrences.
- All 68,284 import ownership baselines still had a current owner whose vnum
  matched and whose revision was not behind the opening revision. Current
  totals were 68,810 owner rows and 753 owner-revision rows. The allocator was
  54,582,089, safely above the maximum persisted payload UID of 53,636,178.
- At this live snapshot, 68,283 of the 68,284 imported baseline UIDs were
  present in one of the three SQL payload tables. The absent payload retained
  an active current-owner row, a valid active player mapping, matching vnum,
  and a non-regressed revision. It was initially recorded as possibly
  consistent with an in-memory item lifecycle. The later DurisMUD audit
  disproved that tentative interpretation: a coin-combine save had replaced
  the payload without transferring custody, leaving the old owner row and a
  new orphan payload. The confirmed defect is documented below and remains
  tracked in [DurisWeb #12](https://github.com/LuminariMUD/DurisWeb/issues/12)
  and [DurisMUD #118](https://github.com/LuminariMUD/DurisMUD/issues/118).
- The official topology checker separately retains 14 mismatches proven
  byte-for-byte pre-existing in the frozen target; imported items account for
  zero of them. Classification and repair are tracked in
  [DurisWeb #14](https://github.com/LuminariMUD/DurisWeb/issues/14) and
  [DurisMUD #125](https://github.com/LuminariMUD/DurisMUD/issues/125).
- Imported player, locker, and pet container ancestry had zero cross-owner
  mismatches. All 216 retained ships resolved to a current player and had the
  expected four armor rows, one crew row, and 16 slot rows. All 71 existing
  locker-access rows resolved by the schema's locker-owner name key. That last
  check did not detect an imported personal locker with no access row at all;
  the later audit below records it.

### Application projections and public acceptance

- Forum storage retained 107 categories, 9 threads, 11 posts, 5 images,
  5 reactions, and 9 subscriptions. Anonymous `/api/forum/categories` exposed
  four categories because the other active categories are authenticated,
  role-based, or guild-scoped; this is ACL filtering, not missing import data.
  Latest and popular activity routes both returned HTTP 200 on their canonical
  `/api/forum/activity/*` paths. The separate empty-target provisioning and
  readiness gap remains tracked in
  [DurisWeb #10](https://github.com/LuminariMUD/DurisWeb/issues/10).
- All 1,674 imported notifications had valid JSON where data was present, and
  there were no missing recipient or triggering-account references. The
  authenticated UI was not exercised during this audit. The later semantic
  link audit found that all 861 auction notifications refer to auctions that
  were intentionally not imported; 551 of those notifications remain unread.
- The imported content projections were usable: all 14 source-only help-page
  details returned nonempty HTTP 200 responses; the retained PvP event/comment
  sample returned both imported comments; 187 events and 802 participant rows
  remained represented; changelog, news, guide, statistics, reboot, status,
  auction, wiki, and frag/PvP aggregates returned structured responses.
- An empty ordinary `/api/server/reboot/history` response was expected because
  that route represents host-monitor history. The 648 normalized imported MUD
  reboots are exposed by `/api/server/reboot/mud-history`, which returned data.
- A canonical 37-route anonymous sweep through `https://duris.sbs` passed all
  expected statuses after correcting two exploratory non-route paths to the
  documented forum activity paths. The protected `/api/zones` route returned
  its expected HTTP 401; the other 36 checks returned HTTP 200. No route
  returned a 5xx response.
- Public `/health` reported `ok` with database and cache both `ok`. Public and
  local site configuration responses matched, the root document was served,
  and the deployed `ForumView`, `GuideView`, `PvPListView`, `StatusView`, and
  `UserProfileView` chunks matched the local frontend artifacts byte-for-byte.
- The initial startup warning that `logs/log/comm` was unavailable occurred
  while the MUD was still creating that file. The regular file then existed
  with owner-only access, and the later web startup produced no corresponding
  unavailable warning. No permission was weakened.

### Profile repair release and rollback evidence

- DurisWeb source changes were limited to
  `/home/duris/durisweb/backend/src/services/forumService.ts` and the existing
  regression-contract suite at
  `/home/duris/durisweb/backend/src/services/__tests__/productionReviewRegression.test.ts`.
- A mode-0700, secret-free release/rollback set is retained at
  `/home/duris/.local/state/durisweb-profile-repair-20260903-T3rxTa`. Its
  reconstructed HEAD rollback tree SHA-256 is
  `2104e6cf8b6793a171282c3dad98f7f6e67222a704c96c662deae23b9ea6cc6b`;
  the deployed candidate tree SHA-256 is
  `5709e3c8278ae37fab2b9e7cc550cde5b96adacd81f2eb8d79d3590e5170d1fa`.
  The deployed `backend/dist` tree matched that candidate digest.
- After cutover, DurisWeb, its Cloudflare tunnel, and the MUD were all
  active/running with zero failure restarts. The MUD process was not restarted.
  DurisWeb reauthenticated to the MUD WebSocket, applied hook state, and logged
  no application errors during acceptance.

### Verification record and limitations

- From `/home/duris/durisweb`, the following passed:
  `pnpm --dir backend format:check`, `pnpm --dir backend lint`,
  `pnpm --dir backend type-check`, `pnpm --dir backend build`, and
  `pnpm --dir backend verify:mud-writes` (53 classified writes).
- The following passed twice around release:
  `node dist/scripts/productionPreflight.js --configuration` and
  `node dist/scripts/productionPreflight.js --dependencies`. They found all
  82 migrations, 12 required tables, and configured Redis dependencies healthy.
  `pnpm migrate:status` also reported 82 complete and zero pending migrations.
- The focused command
  `pnpm --dir backend test --runInBand src/services/__tests__/productionReviewRegression.test.ts`
  passed 12/12 tests, including the profile-query regression. A compiled
  direct-call check then evaluated all seven profiles against production
  successfully, followed by 7/7 local and 7/7 public HTTP profile checks after
  release.
- The full `pnpm --dir backend test --runInBand` run was not green: 75 suites
  and 614 tests passed, while six database-backed suites (69 test cases) could
  not connect to the explicitly isolated test MySQL endpoint on port 7779; the
  isolated Redis endpoint on port 7780 was also absent. The failures were
  dependency connection refusals rather than assertion failures. The retrying
  Redis handle kept Jest open after its final summary and was interrupted. No
  test was redirected to production and no test dependency was provisioned as
  part of this production audit.
- No frontend source changed, so frontend format/lint/type/unit/build gates were
  not rerun. The already deployed frontend was instead checked at its public
  document, configuration, route, and asset boundaries.
- Interactive browser screenshot, console, responsive-layout, and click-flow QA
  remained unverified because no Browser connector was available and the
  DurisWeb checkout had no installed Playwright command. Authenticated login,
  notification, profile-edit, and administrator flows were not attempted
  without credentials.

### Operational follow-up

`durisweb-cloudflared.service` is enabled and has `BindsTo=`/`PartOf=` links to
the web application. The later deliberate application restart proved that this
relationship correctly stopped and restarted an already-active tunnel. The
import maintenance sequence was different: it stopped the tunnel as an
independent unit, then started only the MUD and web writers. An enabled unit is
not automatically re-added to that partial start transaction. Maintenance that
stops units individually must therefore restart the complete enabled service
group and require both unit state and a public health probe before declaring
recovery. Omitting that acceptance step—not the import data or the unit's
normal restart propagation—caused this incident. Executable complete-group
recovery and acceptance tracking is in
[DurisWeb #11](https://github.com/LuminariMUD/DurisWeb/issues/11).

## Independent post-import DurisMUD persistence and materialization audit

Audit window: 2026-09-03 19:19-20:11 UTC. This second follow-up checked the
frozen candidate, live production state, current generated world data, and the
actual MUD repository/materialization code together. It used aggregate-only
read-only SQL, source inspection, repository harnesses, focused regressions,
service health probes, and log review. It did not replay the import, run a
migration, alter a database row, or restart a service.

### Overall conclusion

- The import transaction itself preserved its target-wins and relational
  guarantees. The durable marker, accepted row counts, frozen histories,
  preservation archives, foreign keys, and imported custody baselines remain
  intact.
- Cross-layer checking found three high-risk operational defects: an unresolved
  account-identity collision, 17 selectable characters whose saved state cannot
  pass the runtime materializer, and a generic money-custody bug already
  observed on one imported active character. It also found stale auction
  notification links and three lower-risk storage/portability gaps.
- None of these findings was repaired during the audit. The required production
  changes need an exact candidate-tested repair set and, for the account
  collision, an owner identity decision.

### Account identity boundary

- The source contains 329 account names and the import inserted 328 accounts.
  The remaining source name matched a pre-existing target account, but its
  password bytes, email, and creation timestamp all differ from the target.
  One historical IP overlaps, so the records may represent the same person
  recreating an account, but that is not proof of common ownership.
- The target-wins policy correctly left the target account row and credentials
  unchanged. However, `merge_legacy_stage.py` selected new player rows whenever
  their source account existed and their PID/name did not collide, then copied
  them whenever an account of that name existed in the target. It did not
  require the parent account to be newly imported or prove authentication
  equivalence.
- Consequently nine structurally new, selectable legacy characters were
  attached to the existing target credentials. Their accepted dependent state
  includes one bank row, one IP-history row, 354 items, two ships, eleven
  notifications, and four changelog reads. The account remains under the
  16-character runtime cap, and none of these characters is immortal.
- If the two account records represent different people, this is an
  authorization-boundary violation. The nine mappings and their dependent
  state must therefore be kept, remapped, or quarantined only after owner-level
  adjudication; differing hashes alone cannot answer that question. MUD-owned
  tracking: [DurisMUD #120](https://github.com/LuminariMUD/DurisMUD/issues/120).

### Character materialization failures

- A direct read-only harness linked against the actual
  `src/player/player_load_repository.c` implementation and applied all 783
  imported PIDs in rollback-only transactions. Repository extraction succeeded
  for all 783: 687 have selectable mappings and 96 are retained deleted
  mappings. The largest extraction used 2,381 rows and 223,438 bytes; no stale,
  missing, promoted, or repaired rows were reported at that frozen point.
- Repository success did not prove runtime materialization. A separate
  materializer-equivalent audit used the current generated object prototypes
  and the checks in `src/player/player_load_items.c`,
  `src/player/player_load_pets.c`, and `src/world/handler.c`.
- Sixteen selectable characters each have one imported parent item whose saved
  `item_type` override is `ITEM_ARMOR` even though its current canonical
  prototype is `ITEM_CONTAINER`. Those 16 parent instances span 10 vnums and
  retain 911 child relationships. The importer proved ancestry and ownership,
  but not the effective post-override parent type. At load time the override is
  applied before `obj_can_nest`, so each graph is rejected as an invalid
  snapshot.
- Three imported pets belonging to two selectable owners fail pet
  materialization. All three saved rooms differ from their owner's saved room,
  and one pet also has `hit` greater than `max_hit`. The fourth imported pet is
  valid. All 38 imported pet items belong to the three failing pets and are
  otherwise internally consistent.
- One affected player appears in both groups, so the combined impact is exactly
  17 selectable characters. A refused materialization prevents the character
  from entering the game. No corresponding refusal appeared in the MUD log by
  the end of the audit, which shows only that none had triggered the logged path
  during the observed window.
- The narrow candidate repair is to clear the 16 stale `item_type` overrides so
  canonical container types win, set the three pet rooms to their owners'
  saved rooms, and clamp the one invalid hit value. The real materializer must
  pass for all 17 in an isolated candidate before any production repair.
  MUD-owned tracking:
  [DurisMUD #119](https://github.com/LuminariMUD/DurisMUD/issues/119).

### Confirmed money-custody defect

- The frozen candidate had zero imported player payloads without custody and
  zero active imported ownership rows without any player/pet payload. In live
  production, exactly one imported character now has one new money payload
  without `item_current_owner`, while the superseded money UID still has an
  active owner row but no payload. Exactly one imported player had a save
  revision/timestamp change after commit, and it is this character; it is not
  one of the 17 materialization failures above.
- `uses_generic_item_ownership` in `src/cmd/actobj.c` excludes `ITEM_MONEY`.
  `publish_coin_put` combines a pile by creating a new money object, extracting
  the old object, putting the new object into the container, and marking the
  player dirty. The subsequent snapshot save rewrites `player_items`, but the
  generic custody ledger is not transferred from the old UID to the new UID.
- `src/player/player_load_repository.c` treats a payload without a custody row
  as stale and skips it; `src/player/player_snapshot_repository.c` deletes and
  rebuilds the relevant player-item component on a later full save. Therefore
  the next load omits the new coin pile and a subsequent save removes its
  remaining payload row. This is a general runtime persistence defect rather
  than a merge-script defect, but imported live data has already exercised it.
- Repair requires both a code correction that gives replacement money valid
  custody and an exact reconciliation of the affected old-owner/new-payload
  pair. The existing coin transaction contract covers durable wallet debit
  before publication but does not cover item-custody transfer for a combined
  physical pile. MUD-owned tracking:
  [DurisMUD #118](https://github.com/LuminariMUD/DurisMUD/issues/118).

### Notification semantics

- The import deliberately skipped `auctions`, bid history, and pickup state as
  current transactional authority, but copied `notifications` for every
  account present in the target without validating notification links.
- All 861 imported auction notifications link to auction IDs absent from the
  target; 551 are unread. Of these, 761 already referenced absent auctions in
  the source and 100 referenced source auctions that the import intentionally
  skipped. DurisWeb returns these rows and routes their stored links directly,
  so they create dead navigation and unread-count pollution despite having
  valid JSON and account references.
- A disposition should preserve or discard history deliberately: null the dead
  links, archive/delete the exact imported set, or mark them read according to
  product policy. This should not be treated as a JSON or foreign-key repair.

### Locker, ship, and forum-image follow-up

- The 115 imported lockers comprise 114 canonical account/side lockers and one
  legacy personal locker. Locker/chest structure, passwords, container graphs,
  31,253 item payloads, and custody all validate. The personal locker has no
  `locker_access` row and contains 21 items. At audit time, blank locker entry
  resolved an account locker and explicit named entry validated solely through
  `locker_access`, so a normal non-staff owner could not reach this legacy
  locker. Explicit entry now has a stable owner-identity path; a rehearsed
  visitor grant remains available as a rollout fallback. MUD-owned
  tracking: [DurisMUD #122](https://github.com/LuminariMUD/DurisMUD/issues/122).
  The anonymized rehearsal and guarded backup-first grant procedure are in
  `docs/operations/legacy-personal-locker-access-repair.md`; they do not apply
  any production change by themselves.
- Two of the 71 imported access grants are unusable because their visitors are
  deleted or on the wrong racewar side. Four account/side lockers containing 37
  items currently have no selectable character on that side; those can become
  accessible if an appropriate side character is created and are not by
  themselves corruption.
- Production uses the MariaDB locker repository with mixed fallback disabled.
  The flatfile locker validator rejects canonical `account.*` names, so the 114
  account lockers are a portability caveat if persistence mode ever changes;
  they do not affect the current SQL runtime. MUD-owned tracking:
  [DurisMUD #123](https://github.com/LuminariMUD/DurisMUD/issues/123).
- All 216 imported ships resolve to selectable owners and have valid room,
  class, race, time, armor, crew, and slot cardinality. Three `SLOT_EMPTY` rows
  on three ships retain stale cargo item indices and value fields. The SQL
  loader ignores those fields for empty slots, but the flatfile validator
  requires `item_index=-1`; those ships cannot round-trip through the flatfile
  catalog until the empty-slot sentinel fields are normalized. MUD-owned
  tracking: [DurisMUD #121](https://github.com/LuminariMUD/DurisMUD/issues/121).
- All five imported `forum_post_images` rows have null `post_id` and
  `thread_id` while `is_orphan=0`. No current forum content references them.
  DurisWeb displays images only through post/thread linkage and cleans only rows
  explicitly marked orphan, leaving these records stranded. Their external
  object-store objects were not inspected; cleanup must coordinate database and
  object storage rather than deleting metadata alone.

### Broad integrity checks that passed

- The sole `legacy_import_runs` marker still exactly matches the documented
  SHA-256, 345,366,975-byte source size, import version, completion time, and
  valid JSON aggregate report.
- Current MUD runtime verification passes all 174 required tables at immutable
  migration head 0008. DurisWeb retains 82 completed migrations with zero
  pending or unknown migrations. `mariadb-check --check --silent duris` passes.
- A generated audit of all 123 declared foreign keys found zero orphans. All
  68,284 import baselines still have current custody rows; the confirmed money
  defect is the one missing baseline payload plus its replacement orphan
  payload, not a missing baseline or owner row.
- The current generated world contains 19,133 unique object prototypes, 17,678
  mob prototypes, 253,260 rooms, and 351 zones. Imported player location,
  hometown, birthplace, race/racewar, class, quest, recipe, spellbook, pet, and
  active item references all resolve to supported current definitions. All
  current account passwords use recognized bcrypt forms.
- The imported locker graph has no unreachable items, cross-owner parents,
  non-container parents, missing prototypes, payload/custody mismatches, or
  unsupported password hashes. Ship structures have the expected 216 crew,
  864 armor, and 3,456 slot rows apart from the three flatfile sentinels above.
- Exact frozen multiset hashes match for imported statistics, log entries, page
  views, visitor sessions, health metrics, reboot history, and zone touches.
  All three preservation archive counts and checksums match the qualified
  source/candidate evidence.
- Post-restart differences from the frozen candidate were otherwise normal
  runtime activity: player saves, allocator/timer/artifact maintenance,
  shopkeeper rewrites that were logically identical, and 110 hourly
  `zones.frequency_mod` increments from the epic maintenance tick. No separate
  import loss was found.
- At the final probe, `duris-mud-production.service` had been active since
  19:02:45 UTC and `durisweb-production.service` since its deliberate 19:48:39
  UTC profile-repair restart; both reported zero failure restarts. The MUD
  health script passed, and `http://127.0.0.1:7770/health` reported web database
  and cache status `ok`. Logs contained no materialization refusal, custody
  alert, fatal MUD error, or DurisWeb application error in the reviewed windows.

### DurisMUD focused verification record

The following focused regression commands passed:

- `python3 tests/async/test_player_load_items.py`
- `python3 tests/async/test_player_load_pets.py`
- `python3 tests/async/test_player_load_topology.py`
- `python3 tests/async/test_coin_command_transaction_contract.py`
- `python3 tests/async/test_orphan_item_session_regressions.py`
- `python3 tests/async/test_flatfile_locker_repository.py`
- `python3 tests/async/test_flatfile_ship_repository.py`
- `python3 tests/async/test_locker_ownership_cutover.py`
- `python3 tests/async/test_ship_save_guards.py`
- `python3 tests/async/test_legacy_dump_import.py` (12 tests)
- `python3 tests/async/test_epic_bonus_hot_path.py`
- `python3 tests/async/test_epic_bonus_state.py`
- `python3 tests/async/test_flatfile_recipe_repository.py`
- `python3 tests/async/test_flatfile_recipe_runtime_contract.py`
- `python3 tests/async/test_flatfile_spellbook_repository.py`
- `python3 tests/async/test_flatfile_spellbook_runtime_contract.py`
- `python3 tests/async/test_runtime_boot_compatibility.py`

No C/C++ source changed during this audit, so a new full server build was not
required. The purpose-built repository validation binary was placed under the
ignored `bin/tests/` artifact tree and must not be committed.

### Required disposition order

1. Adjudicate whether the colliding source and target accounts have the same
   owner; quarantine or remap the nine characters and dependent state if that
   cannot be established
   ([DurisMUD #120](https://github.com/LuminariMUD/DurisMUD/issues/120)).
2. Apply the exact item/pet normalization in an isolated candidate and prove
   full materialization for all 17 affected selectable characters before a
   guarded production repair
   ([DurisMUD #119](https://github.com/LuminariMUD/DurisMUD/issues/119)).
3. Fix replacement-money custody, add a focused ownership regression, and
   reconcile the one live old-owner/new-payload pair without duplicating or
   destroying currency
   ([DurisMUD #118](https://github.com/LuminariMUD/DurisMUD/issues/118)).
4. Repair the 12 missing combat-frag baselines
   ([DurisMUD #124](https://github.com/LuminariMUD/DurisMUD/issues/124)) and
   classify/repair the 14 pre-existing topology mismatches
   ([DurisMUD #125](https://github.com/LuminariMUD/DurisMUD/issues/125)).
5. Decide recoverability of the quarantined legacy items
   ([DurisMUD #126](https://github.com/LuminariMUD/DurisMUD/issues/126)) and
   normalized association/guild state
   ([DurisMUD #127](https://github.com/LuminariMUD/DurisMUD/issues/127)).
6. Choose and apply a historical policy for the 861 dead auction notifications.
7. Resolve the inaccessible personal locker
   ([DurisMUD #122](https://github.com/LuminariMUD/DurisMUD/issues/122)), make
   account lockers representable in flatfile authority
   ([DurisMUD #123](https://github.com/LuminariMUD/DurisMUD/issues/123)), then
   normalize the three ship sentinels
   ([DurisMUD #121](https://github.com/LuminariMUD/DurisMUD/issues/121)) and five
   stranded image records if their retained content has been reviewed.
