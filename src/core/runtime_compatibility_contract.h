#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 177;
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
	"b5d52cca2efb9fc8f1192249c25d4affdf1b6cc005fb9ab982f0309f859c6287";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"6068b13bb215df55c01fd42521ca2a6ff73cdab281a7f214b247eeb841be9eab";
/* Includes the death disposition and custody tables introduced by migration 0011.
 * Metadata fingerprints are measured on MySQL 8 and MariaDB 10.11. */
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0011_player_death_disposition";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 11;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"209a3156bc163e756dbf9f5edd025445ab3ee180fa75fc63d0d4685e5e1d8057";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"cfeba2cc7cd8f07b1b24e490d2d05167231e76cbdcb96ced21b98671612f12e3";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"c308bec958dac85b44ad4b0fd26c9cef8a174b8d34094de502a5a7881d9abce6";
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
