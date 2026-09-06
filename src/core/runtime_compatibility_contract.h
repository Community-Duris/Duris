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
	"4f662dcbddd047e5843d4c7d4a3f4a0b63695058e22695db599ce74a5cd6a5f5";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"afeb08122578efb0a62b045e87d7f57f06412db85a65163f64d34e9804cee598";
/* Coin payloads are part of the fingerprinted item_current_owner table, so the
 * fingerprints above were regenerated against MySQL 8 and MariaDB 10.11 after
 * 0010. Head 0011 adds player_death_disposition and player_death_custody, the
 * durable record of a death whose corpse handoff was refused. Like
 * kingdom_garrison they are created only by their migration, never by the
 * bootstrap schema, and are deliberately absent from RUNTIME_TABLE_SQL_LIST: the
 * fingerprint, the table count and the engine/collation checks all scope
 * themselves to that list, so a table outside it needs no live regeneration --
 * and RUNTIME_CURRENT_TABLE_COUNT above is therefore unchanged at 174. The three
 * checksums below are SHA-256 over the migration FILES and over the rolling
 * ledger, all computable offline. Adding the new tables to the list is a
 * follow-up for whoever next has both a MySQL 8 and a MariaDB 10.11 to hand. */
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0011_player_death_disposition";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 11;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"209a3156bc163e756dbf9f5edd025445ab3ee180fa75fc63d0d4685e5e1d8057";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"f9fcb0d7dcfec03b87d6f3caa1d64f1301c93c55362320b7bfe78dace90c98e8";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"daf69aa4a6b640a506c03274d427c3d414d24d5a3f8ec369a4594e101ebcf177";
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
