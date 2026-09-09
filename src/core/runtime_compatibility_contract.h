#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 178;
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
	"'epic_ledger','epic_stone_claim','eq_drop','frag_leaderboard','guild_members',"
	"'guild_outcome_ledger','guild_ranks','guild_transactions','guildhall_rooms',"
	"'guildhalls','guilds','ip_info','item_current_owner',"
	"'item_owner_revision','item_ownership_baseline','item_ownership_ledger','item_ownership_quarantine',"
	"'item_uid_allocator','items','kingdom_garrison','kingdom_land',"
	"'kingdom_realms','level_cap','lifecycle_archive_batches','lifecycle_archive_evidence',"
	"'lifecycle_archive_jobs','lifecycle_archive_rows','locker_access','locker_activity_log',"
	"'locker_chests','locker_item_affects','locker_item_extra_descr','locker_items',"
	"'locker_kickouts','locker_session_state','lockers','log_entries',"
	"'lookup_dataset_state','mud_info','mud_schema_baselines','mud_schema_history',"
	"'mud_schema_migration_state','mud_schema_migrations','multiplay_whitelist','nexus_stones',"
	"'offline_messages','outposts','pages','persistence_item_events',"
	"'persistence_scalar_events','personal_data_export_audit','personal_data_export_requests','personal_data_export_sections',"
	"'ping','pkill_event','pkill_info','player_affects',"
	"'player_data','player_death_custody','player_death_disposition','player_forged_items',"
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
	"bcd4e7b97cb03b81af1a9136bb7ba70391c03344babd0836311a3193843b0c0a";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"092ee0e322c5ffcb3a5d40594314b2bc749db0709d074ad8671edc133cf2c510";
/* Includes the durable epic stone claims introduced by migration 0012.
 * Metadata fingerprints are measured on MySQL 8 and MariaDB 10.11. */
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0012_epic_stone_claim";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 12;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"8a1ea4c09d6989ffa90a6b174e2fc88f52d0642cb0940461e46a457039c107a9";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"8e159b95bf80b1484c60bc4a9f983a4aa9c0fb0523e85b4c48db7d798c556e16";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"ff1588566c193a669a35b4dc633bf3aa210ac71d441c2285648d2cc4e02fc8bd";
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
