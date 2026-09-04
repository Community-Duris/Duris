#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 174;
constexpr const char *RUNTIME_TABLE_SQL_LIST =
	"'account_banks','account_bound_reward_pwipe_state','account_bound_reward_summons','account_bound_rewards',"
	"'account_characters','account_erasure_evidence','account_erasure_requests','account_erasure_stores',"
	"'account_erasure_tombstones','account_ips','account_locker_access','account_locker_item_affects',"
	"'account_locker_item_extra_descr','account_locker_items','account_lockers','accounts',"
	"'alliances','artifact_bind','artifact_delta_ledger','artifact_domain_baseline',"
	"'artifact_domain_state','artifact_guild_outcome','artifact_guild_outcome_delta','artifacts',"
	"'artifacts_mortal','associations','auction_bid_history','auction_item_custody',"
	"'auction_item_pickups','auction_ledger','auction_money_pickups','auction_reconciliation_quarantine',"
	"'auctions','boon_reward_outcome','boon_reward_outcome_entry','boons',"
	"'boons_progress','boons_shop','categories','changes',"
	"'classes','combat_frag_baseline','combat_frag_ledger','combat_outcome',"
	"'combat_outcome_participant','corpse_item_affects','corpse_item_extra_descr','corpse_items',"
	"'corpses','critical_operation_inbox','critical_outbox','critical_outbox_delivery_dedupe',"
	"'critical_test_state','ctf_data','currency_bank_baseline','currency_ledger',"
	"'currency_wallet_baseline','epic_balance_baseline','epic_bonus','epic_gain',"
	"'epic_ledger','eq_drop','frag_leaderboard','guild_members',"
	"'guild_outcome_ledger','guild_ranks','guild_transactions','guildhall_rooms',"
	"'guildhalls','guilds','ip_info','item_current_owner',"
	"'item_owner_revision','item_ownership_baseline','item_ownership_ledger','item_ownership_quarantine',"
	"'item_uid_allocator','items','kingdom_land','kingdom_realms','level_cap',"
	"'lifecycle_archive_batches','lifecycle_archive_evidence','lifecycle_archive_jobs','lifecycle_archive_rows',"
	"'locker_access','locker_activity_log','locker_chests','locker_item_affects',"
	"'locker_item_extra_descr','locker_items','locker_kickouts','locker_session_state',"
	"'lockers','log_entries','lookup_dataset_state','mud_info',"
	"'mud_schema_baselines','mud_schema_history','mud_schema_migration_state','mud_schema_migrations',"
	"'multiplay_whitelist','nexus_stones','offline_messages','outposts',"
	"'pages','persistence_item_events','persistence_scalar_events','personal_data_export_audit',"
	"'personal_data_export_requests','personal_data_export_sections','ping','pkill_event',"
	"'pkill_info','player_affects','player_data','player_forged_items',"
	"'player_granted_cmds','player_intros','player_item_affects','player_item_extra_descr',"
	"'player_items','player_languages','player_pet_item_affects','player_pet_item_extra_descr',"
	"'player_pet_items','player_pets','player_recipes','player_shapechanges',"
	"'player_skills','player_spellbooks','player_timers','player_undead_slots',"
	"'player_witnesses','poll_options','poll_votes','polls',"
	"'prepstatement_duris_sql','private_chest_log','private_chests','progress',"
	"'quest_trophy','races','racewar_stat_mods','saved_item_affects',"
	"'saved_item_extra_descr','saved_items','season_reset_state','server_reboots',"
	"'session_audit_outcome','ship_armor','ship_cargo_market_mods','ship_cargo_prices',"
	"'ship_crew','ship_slots','ships','shop_trophy',"
	"'shopkeeper_affects','shopkeeper_item_affects','shopkeeper_item_extra_descr','shopkeeper_items',"
	"'shopkeepers','siege_item_affects','siege_item_extra_descr','siege_items',"
	"'statistics','timers','towns','world_quest_accomplished',"
	"'zone_touch_outcome','zone_touch_outcome_participant','zone_touches','zone_trophy',"
	"'zones'";
constexpr const char *RUNTIME_MYSQL8_METADATA_FINGERPRINT =
	"0ec0366920b78703c006c6d4f159026c659d071c38aad5e142c2573754aaa8f3";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"92dd682008ba94f8aecc63595dc46f9d6f1f865adecd174e58e2a2ce14220f2c";
/* Head 0009 adds kingdom_garrison, the roster of purchased guards. The three
 * checksums below are SHA-256 over the migration FILES and over the rolling
 * ledger, all computable offline; the two normalised metadata fingerprints
 * above are NOT, because they hash information_schema on a live server. That is
 * why kingdom_garrison is deliberately absent from RUNTIME_TABLE_SQL_LIST: the
 * fingerprint, the table count and the engine/collation checks all scope
 * themselves to that list, so a table outside it needs no live regeneration --
 * and RUNTIME_CURRENT_TABLE_COUNT above is therefore unchanged at 174.
 * Adding it to the list is a follow-up for whoever next has both a MySQL 8 and
 * a MariaDB 10.11 to hand; until then the module degrades exactly the way
 * kingdom_realms already does when its own table cannot be read. */
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0009_kingdom_garrison";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 9;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"e295446be0ae22bb48989db87166641b8f8f599fa711133a3fd3f8370db9d8b3";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"79a099d088ef091026ce63114c560c52d158377971c4dfab80b0785bd62f46c9";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"37fabde7d3c00518b92b35519442106b74dcb92fd18eebc6251c3470da56f561";
constexpr const char *LOOKUP_DATASET_NAME = "race_class";
constexpr unsigned LOOKUP_DATASET_VERSION = 1;
constexpr const char *RUNTIME_DB_CHARACTER_SET = "utf8mb4";
constexpr const char *RUNTIME_DB_TIME_ZONE = "+00:00";
constexpr const char *RUNTIME_DB_ISOLATION = "READ-COMMITTED";
constexpr const char *RUNTIME_DB_SQL_MODE =
	"STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION";
constexpr unsigned RUNTIME_DB_TIMEOUT_SECONDS = 10;
constexpr bool RUNTIME_DB_REMOTE_TLS_REQUIRED = true;
constexpr size_t RUNTIME_METADATA_MAX_BYTES = 4 * 1024 * 1024;

#endif
