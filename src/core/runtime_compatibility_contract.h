#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 273;
constexpr const char *RUNTIME_TABLE_SQL_LIST =
	"'account_banks','account_bound_reward_pwipe_state','account_bound_reward_summons','ac"
	"count_bound_rewards','account_characters','account_erasure_evidence','account_erasure"
	"_requests','account_erasure_stores','account_erasure_tombstones','account_ips','accou"
	"nt_locker_access','account_locker_item_affects','account_locker_item_extra_descr','ac"
	"count_locker_items','account_lockers','accounts','alliances','artifact_bind','artifac"
	"t_delta_ledger','artifact_domain_baseline','artifact_domain_state','artifact_guild_ou"
	"tcome','artifact_guild_outcome_delta','artifact_mana','artifacts','artifacts_mortal',"
	"'associations','auction_bid_history','auction_item_custody','auction_item_pickups','a"
	"uction_ledger','auction_money_pickups','auction_reconciliation_quarantine','auctions'"
	",'boon_reward_outcome','boon_reward_outcome_entry','boons','boons_progress','boons_sh"
	"op','categories','changes','classes','collector_catalog_state','collector_deaths','co"
	"llector_ledger','collector_listings','collector_reconciliation_quarantine','combat_fr"
	"ag_baseline','combat_frag_ledger','combat_outcome','combat_outcome_participant','corp"
	"se_catalog_state','corpse_item_affects','corpse_item_extra_descr','corpse_items','cor"
	"pses','critical_operation_inbox','critical_outbox','critical_outbox_delivery_dedupe',"
	"'critical_test_state','ctf_data','currency_bank_baseline','currency_ledger','currency"
	"_wallet_baseline','economic_account_mapping','economic_accounting_account_effect','ec"
	"onomic_accounting_child','economic_accounting_coin_posting','economic_accounting_item"
	"_reference','economic_accounting_operation','economic_accounting_source_claim','econo"
	"mic_baseline_control','economic_baseline_reservation','economic_baseline_witness','ec"
	"onomic_epoch','economic_lineage_state','economic_pending_claim_source','economic_sql_"
	"activation_receipt','economic_sql_global_activation','economic_sql_lifecycle_installa"
	"tion','epic_balance_baseline','epic_bonus','epic_gain','epic_ledger','epic_stone_clai"
	"m','eq_drop','frag_leaderboard','guild_members','guild_outcome_ledger','guild_ranks',"
	"'guild_transactions','guildhall_rooms','guildhalls','guilds','ip_info','item_current_"
	"owner','item_owner_revision','item_ownership_baseline','item_ownership_ledger','item_"
	"ownership_quarantine','item_uid_allocator','items','kingdom_garrison','kingdom_land',"
	"'kingdom_realms','level_cap','lifecycle_archive_batches','lifecycle_archive_evidence'"
	",'lifecycle_archive_jobs','lifecycle_archive_rows','locker_access','locker_activity_l"
	"og','locker_chests','locker_item_affects','locker_item_extra_descr','locker_items','l"
	"ocker_kickouts','locker_session_state','lockers','log_entries','lookup_dataset_state'"
	",'mud_info','mud_schema_baselines','mud_schema_history','mud_schema_migration_state',"
	"'mud_schema_migrations','multiplay_whitelist','nexus_stones','offline_message_receipt"
	"s','offline_messages','outposts','pages','persistence_item_events','persistence_scala"
	"r_events','personal_data_export_audit','personal_data_export_requests','personal_data"
	"_export_sections','ping','pkill_event','pkill_info','player_affects','player_craft_pr"
	"ogression','player_data','player_death_conflict_evidence','player_death_custody','pla"
	"yer_death_disposition','player_death_restitution_delivery','player_death_restitution_"
	"item','player_death_restitution_receipt','player_death_restitution_runtime','player_f"
	"orged_items','player_granted_cmds','player_intros','player_item_affects','player_item"
	"_extra_descr','player_item_runtime_state','player_items','player_languages','player_p"
	"et_item_affects','player_pet_item_extra_descr','player_pet_items','player_pets','play"
	"er_recipes','player_shapechanges','player_skills','player_spell_effect_receipt','play"
	"er_spellbooks','player_timers','player_undead_slots','player_witnesses','poll_options"
	"','poll_votes','polls','prepstatement_duris_sql','private_chest_log','private_chests'"
	",'progress','quest_reward_obligation','quest_reward_xp_entitlement','quest_trophy','r"
	"aces','racewar_stat_mods','saved_item_affects','saved_item_extra_descr','saved_item_r"
	"ecovery_handoff','saved_items','season_reset_state','server_reboots','session_audit_o"
	"utcome','ship_armor','ship_cargo_market_mods','ship_cargo_prices','ship_crew','ship_s"
	"lots','ships','shop_trophy','shopkeeper_affects','shopkeeper_item_affects','shopkeepe"
	"r_item_extra_descr','shopkeeper_items','shopkeepers','siege_item_affects','siege_item"
	"_extra_descr','siege_items','sql_room_item_payload','statistics','telemetry_account_l"
	"ifetime','telemetry_account_token','telemetry_battle_input','telemetry_battle_input_v"
	"6','telemetry_battle_input_v7','telemetry_battle_input_v8','telemetry_battle_source',"
	"'telemetry_battle_source_v6','telemetry_battle_source_v7','telemetry_battle_source_v8"
	"','telemetry_cohort_day','telemetry_cohort_member','telemetry_config','telemetry_gene"
	"ration_identity','telemetry_identity_association','telemetry_identity_input','telemet"
	"ry_identity_registry','telemetry_identity_reviewer','telemetry_incident','telemetry_i"
	"ncident_registry','telemetry_incident_registry_v2','telemetry_incident_registry_v3','"
	"telemetry_incident_registry_v4','telemetry_incident_registry_v5','telemetry_incident_"
	"registry_v6','telemetry_incident_registry_v7','telemetry_incident_v2','telemetry_inci"
	"dent_v3','telemetry_incident_v4','telemetry_incident_v5','telemetry_incident_v6','tel"
	"emetry_incident_v7','telemetry_interval','telemetry_player_day','telemetry_quarantine"
	"','telemetry_reward_projection','telemetry_reward_projection_state','telemetry_rollup"
	"_battle_coverage','telemetry_rollup_battle_coverage_v6','telemetry_rollup_battle_cove"
	"rage_v7','telemetry_rollup_battle_coverage_v8','telemetry_rollup_battle_row','telemet"
	"ry_rollup_battle_row_v6','telemetry_rollup_battle_row_v7','telemetry_rollup_battle_ro"
	"w_v8','telemetry_rollup_combat_actor','telemetry_rollup_encounter','telemetry_rollup_"
	"encounter_participant','telemetry_rollup_identity_coverage','telemetry_rollup_identit"
	"y_effort','telemetry_rollup_incident','telemetry_rollup_incident_coverage','telemetry"
	"_rollup_level_event','telemetry_rollup_portfolio_xp','telemetry_rollup_progression_da"
	"y','telemetry_rollup_session','telemetry_rollup_state','telemetry_session','timers','"
	"towns','world_quest_accomplished','zone_story_quest_state','zone_touch_outcome','zone"
	"_touch_outcome_participant','zone_touches','zone_trophy','zones'";
constexpr const char *RUNTIME_MYSQL8_METADATA_FINGERPRINT =
	"7240e35acdbfb9ba4de6bbd5b2dd0635c9f3f9963fc98f0ea3f31de28a24ce27";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"84a48b5c8767c39c81f67389f9e8ae87bc667979512f9a4d6a14b852a5626408";
/* Includes the six telemetry stores introduced by migration 0014, the two
 * rollup stores introduced by migration 0017, the five Collector authority
 * stores introduced by migration 0018, corpse catalog authority introduced by
 * migration 0019, restitution receipt/runtime stores introduced by migration
 * 0020, identity-stable Collector notification receipt/outbox state introduced
 * by migration 0021, additive progression fields introduced by migration
 * 0022, committed reward projection/reconciliation state introduced by
 * migration 0023, bounded encounter lifecycle facts introduced by migration
 * migration 0024, the durable zone-story quest state introduced by migration 0026, and
 * the saved-item recovery receipt introduced by migration 0027, and the durable
 * telemetry quarantine introduced by migration 0030. Migration 0029 adds the
 * additive critical-operation failure stage; it changes no runtime table count.
 * Migration 0031 adds retained accounting evidence and identity metadata.
 * Migration 0032 adds baseline witness retention and opening reservations.
 * Migration 0033 adds staged SQL lifecycle receipts, not gameplay activation.
 * Migration 0034 adds retained death-conflict evidence, not capture activation.
 * Migration 0036 adds scoped activation receipts; migration 0040 adds the
 * separate global SQL cutover decision. Migration 0041 records shopkeeper cash
 * and a stable shop revision; migration 0042 records nullable keeper roaming
 * configuration; migration 0043 records nullable keeper item condition, and
 * migration 0044 preserves dynamic state in shopkeeper item checkpoints.
 * Migration 0045 retains quest reward obligations with offering custody and
 * adds one runtime table. Migration 0046 adds realized copper prices to
 * economic operation roots without adding a runtime table; migration 0047 adds
 * a durable quest XP receipt mask and migration 0048 adds per-recipient XP
 * entitlements. Migration 0049 adds player spell-effect receipts, 0051 adds
 * player item runtime state, and 0052 indexes quest item witness reads. */
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0070_telemetry_result_publication";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 73;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"cb53c4cbad9a8bad2af7260fcdc1c6d8c080579cea41e964523af86c35849a0c";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"60010330c9219535e74f1bd899580a749eefb2cba9e05b20a2f085bb538f5f2b";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"714f51a02c89fc16dd51deea7dadf861e2b668c60123e64672a53d82e968b145";
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_HEAD_ID = "0070_telemetry_result_publication";
constexpr unsigned RUNTIME_STAGING_0045_MIGRATION_HEAD_SEQUENCE = 73;
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_APPLY_CHECKSUM =
	"cb53c4cbad9a8bad2af7260fcdc1c6d8c080579cea41e964523af86c35849a0c";
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_VERIFY_CHECKSUM =
	"60010330c9219535e74f1bd899580a749eefb2cba9e05b20a2f085bb538f5f2b";
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_HISTORY_CHECKSUM =
	"687f947bf8b6e504df2d1c8774ed05b18be180c62341c10641200cc71cb658ec";
/* Master recorded runtime state at 0031; its accounting upgrade retains that prefix. */
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_HEAD_ID = "0070_telemetry_result_publication";
constexpr unsigned RUNTIME_MASTER_0031_MIGRATION_HEAD_SEQUENCE = 73;
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_APPLY_CHECKSUM =
	"cb53c4cbad9a8bad2af7260fcdc1c6d8c080579cea41e964523af86c35849a0c";
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_VERIFY_CHECKSUM =
	"60010330c9219535e74f1bd899580a749eefb2cba9e05b20a2f085bb538f5f2b";
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_HISTORY_CHECKSUM =
	"aa7aa2ada3a4a9fead2c859758f9bc6cefc575accb3125b66eaa4d53a84279a3";
/* Recorded telemetry prefixes converge by appending the three independent base migrations. */
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_HEAD_ID =
	"0070_telemetry_result_publication";
constexpr unsigned RUNTIME_TELEMETRY_0067_MIGRATION_HEAD_SEQUENCE = 73;
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_APPLY_CHECKSUM =
	"cb53c4cbad9a8bad2af7260fcdc1c6d8c080579cea41e964523af86c35849a0c";
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_VERIFY_CHECKSUM =
	"60010330c9219535e74f1bd899580a749eefb2cba9e05b20a2f085bb538f5f2b";
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_HISTORY_CHECKSUM =
	"cb76887ef7d22b78b75a626670fdbca5644770b5f2a228244295ba87ac99845f";
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_HEAD_ID =
	"0070_telemetry_result_publication";
constexpr unsigned RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_HEAD_SEQUENCE = 73;
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_APPLY_CHECKSUM =
	"cb53c4cbad9a8bad2af7260fcdc1c6d8c080579cea41e964523af86c35849a0c";
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_VERIFY_CHECKSUM =
	"60010330c9219535e74f1bd899580a749eefb2cba9e05b20a2f085bb538f5f2b";
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_HISTORY_CHECKSUM =
	"855b5cef43ae76380e386561aa4dc5f3d99c8fe4b5c4f4ec8504005ef5a0b55f";
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_HEAD_ID =
	"0070_telemetry_result_publication";
constexpr unsigned RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_HEAD_SEQUENCE = 73;
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_APPLY_CHECKSUM =
	"cb53c4cbad9a8bad2af7260fcdc1c6d8c080579cea41e964523af86c35849a0c";
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_VERIFY_CHECKSUM =
	"60010330c9219535e74f1bd899580a749eefb2cba9e05b20a2f085bb538f5f2b";
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_HISTORY_CHECKSUM =
	"e97465d3fee2a6d733038d0e33857f88331e7ac6aa0f920323d8f19e537d47bb";
constexpr const char *RUNTIME_MIGRATION_HISTORY_SQL =
	"SELECT HEX(CONCAT(UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(migration_id USING utf8mb4))),1"
	"6,'0')),CONVERT(migration_id USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(CAST("
	"sequence_number AS CHAR) USING utf8mb4))),16,'0')),CONVERT(CAST(sequence_number AS CH"
	"AR) USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(description USING utf8mb4))),1"
	"6,'0')),CONVERT(description USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(LOWER("
	"HEX(apply_checksum)) USING utf8mb4))),16,'0')),CONVERT(LOWER(HEX(apply_checksum)) USI"
	"NG utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(LOWER(HEX(verify_checksum)) USING utf"
	"8mb4))),16,'0')),CONVERT(LOWER(HEX(verify_checksum)) USING utf8mb4),UNHEX(LPAD(HEX(OC"
	"TET_LENGTH(CONVERT(compatibility USING utf8mb4))),16,'0')),CONVERT(compatibility USIN"
	"G utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(CAST(runner_version AS CHAR) USING utf"
	"8mb4))),16,'0')),CONVERT(CAST(runner_version AS CHAR) USING utf8mb4))) FROM mud_schem"
	"a_history ORDER BY sequence_number LIMIT 74";
constexpr const char *RUNTIME_EXTRA_DESCRIPTION_GENERATION_SQL =
	"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN "
	"('player_item_extra_descr','player_pet_item_extra_descr') AND column_name='description_sha256' AND "
	"data_type='binary' AND character_maximum_length=32 AND LOWER(extra) LIKE '%stored generated%' AND LO"
	"WER(REPLACE(REPLACE(REPLACE(REPLACE(generation_expression,CONCAT(CHAR(92),CHAR(39),CHAR(92),CHAR(39)"
	"),CONCAT(CHAR(39),CHAR(39))),CHAR(96),''),' "
	"',''),'_utf8mb4',''))='unhex(sha2(coalesce(description,''''),256))'";
constexpr const char *LOOKUP_DATASET_NAME = "race_class";
constexpr unsigned LOOKUP_DATASET_VERSION = 1;
constexpr const char *RUNTIME_DB_CHARACTER_SET = "utf8mb4";
constexpr const char *RUNTIME_DB_COLLATION = "utf8mb4_unicode_ci";
constexpr const char *RUNTIME_DB_TIME_ZONE = "+00:00";
constexpr const char *RUNTIME_DB_ISOLATION = "READ-COMMITTED";
constexpr const char *RUNTIME_DB_SQL_MODE =
	"STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION";
constexpr unsigned RUNTIME_DB_TIMEOUT_SECONDS = 10;
constexpr bool RUNTIME_DB_REMOTE_TLS_REQUIRED = true;
constexpr size_t RUNTIME_METADATA_MAX_BYTES = 4 * 1024 * 1024;

#endif
