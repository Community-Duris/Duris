#ifndef RUNTIME_COMPATIBILITY_CONTRACT_H
#define RUNTIME_COMPATIBILITY_CONTRACT_H

#include <cstddef>

constexpr unsigned RUNTIME_COMPATIBILITY_MANIFEST_VERSION = 1;
constexpr const char *RUNTIME_BASELINE_ID = "duris-schema-2026-08-27-session11";
constexpr const char *RUNTIME_BASELINE_FINGERPRINT =
	"db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd";
constexpr unsigned RUNTIME_BASELINE_TABLE_COUNT = 170;
constexpr unsigned RUNTIME_CURRENT_TABLE_COUNT = 267;
constexpr const char *RUNTIME_TABLE_SQL_LIST =
	"'account_banks','account_bound_reward_pwipe_state','account_bound_reward_summons',"
	"'account_bound_rewards','account_characters','account_erasure_evidence',"
	"'account_erasure_requests','account_erasure_stores','account_erasure_tombstones',"
	"'account_ips','account_locker_access','account_locker_item_affects',"
	"'account_locker_item_extra_descr','account_locker_items','account_lockers','accounts',"
	"'alliances','artifact_bind','artifact_delta_ledger','artifact_domain_baseline',"
	"'artifact_domain_state','artifact_guild_outcome','artifact_guild_outcome_delta',"
	"'artifact_mana','artifacts','artifacts_mortal','associations','auction_bid_history',"
	"'auction_item_custody','auction_item_pickups','auction_ledger','auction_money_pickups',"
	"'auction_reconciliation_quarantine','auctions','boon_reward_outcome',"
	"'boon_reward_outcome_entry','boons','boons_progress','boons_shop','categories',"
	"'changes','classes','collector_catalog_state','collector_deaths','collector_ledger',"
	"'collector_listings','collector_reconciliation_quarantine','combat_frag_baseline',"
	"'combat_frag_ledger','combat_outcome','combat_outcome_participant',"
	"'corpse_catalog_state','corpse_item_affects','corpse_item_extra_descr','corpse_items',"
	"'corpses','critical_operation_inbox','critical_outbox',"
	"'critical_outbox_delivery_dedupe','critical_test_state','ctf_data',"
	"'currency_bank_baseline','currency_ledger','currency_wallet_baseline',"
	"'economic_account_mapping','economic_accounting_account_effect',"
	"'economic_accounting_child','economic_accounting_coin_posting',"
	"'economic_accounting_item_reference','economic_accounting_operation',"
	"'economic_accounting_source_claim','economic_baseline_control',"
	"'economic_baseline_reservation','economic_baseline_witness','economic_epoch',"
	"'economic_lineage_state','economic_pending_claim_source',"
	"'economic_sql_activation_receipt','economic_sql_global_activation',"
	"'economic_sql_lifecycle_installation','epic_balance_baseline','epic_bonus','epic_gain',"
	"'epic_ledger','epic_stone_claim','eq_drop','frag_leaderboard','guild_members',"
	"'guild_outcome_ledger','guild_ranks','guild_transactions','guildhall_rooms',"
	"'guildhalls','guilds','ip_info','item_current_owner','item_owner_revision',"
	"'item_ownership_baseline','item_ownership_ledger','item_ownership_quarantine',"
	"'item_uid_allocator','items','kingdom_garrison','kingdom_land','kingdom_realms',"
	"'level_cap','lifecycle_archive_batches','lifecycle_archive_evidence',"
	"'lifecycle_archive_jobs','lifecycle_archive_rows','locker_access','locker_activity_log',"
	"'locker_chests','locker_item_affects','locker_item_extra_descr','locker_items',"
	"'locker_kickouts','locker_session_state','lockers','log_entries','lookup_dataset_state',"
	"'mud_info','mud_schema_baselines','mud_schema_history','mud_schema_migration_state',"
	"'mud_schema_migrations','multiplay_whitelist','nexus_stones','offline_message_receipts',"
	"'offline_messages','outposts','pages','persistence_item_events',"
	"'persistence_scalar_events','personal_data_export_audit',"
	"'personal_data_export_requests','personal_data_export_sections','ping','pkill_event',"
	"'pkill_info','player_affects','player_craft_progression','player_data',"
	"'player_death_conflict_evidence','player_death_custody','player_death_disposition',"
	"'player_death_restitution_delivery','player_death_restitution_item',"
	"'player_death_restitution_receipt','player_death_restitution_runtime',"
	"'player_forged_items','player_granted_cmds','player_intros','player_item_affects',"
	"'player_item_extra_descr','player_item_runtime_state','player_items','player_languages',"
	"'player_pet_item_affects','player_pet_item_extra_descr','player_pet_items',"
	"'player_pets','player_recipes','player_shapechanges','player_skills',"
	"'player_spell_effect_receipt','player_spellbooks','player_timers','player_undead_slots',"
	"'player_witnesses','poll_options','poll_votes','polls','prepstatement_duris_sql',"
	"'private_chest_log','private_chests','progress','quest_reward_obligation',"
	"'quest_reward_xp_entitlement','quest_trophy','races','racewar_stat_mods',"
	"'saved_item_affects','saved_item_extra_descr','saved_item_recovery_handoff',"
	"'saved_items','season_reset_state','server_reboots','session_audit_outcome',"
	"'ship_armor','ship_cargo_market_mods','ship_cargo_prices','ship_crew','ship_slots',"
	"'ships','shop_trophy','shopkeeper_affects','shopkeeper_item_affects',"
	"'shopkeeper_item_extra_descr','shopkeeper_items','shopkeepers','siege_item_affects',"
	"'siege_item_extra_descr','siege_items','sql_room_item_payload','statistics',"
	"'telemetry_account_lifetime','telemetry_account_token','telemetry_battle_input',"
	"'telemetry_battle_input_v6','telemetry_battle_input_v7','telemetry_battle_source','telemetry_battle_source_v6','telemetry_battle_source_v7',"
	"'telemetry_cohort_day','telemetry_cohort_member','telemetry_config',"
	"'telemetry_generation_identity','telemetry_identity_association',"
	"'telemetry_identity_input','telemetry_identity_registry','telemetry_identity_reviewer',"
	"'telemetry_incident','telemetry_incident_registry','telemetry_incident_registry_v2',"
	"'telemetry_incident_registry_v3','telemetry_incident_registry_v4',"
	"'telemetry_incident_registry_v5','telemetry_incident_registry_v6',"
	"'telemetry_incident_v2','telemetry_incident_v3','telemetry_incident_v4',"
	"'telemetry_incident_v5','telemetry_incident_v6','telemetry_interval',"
	"'telemetry_player_day','telemetry_quarantine','telemetry_reward_projection',"
	"'telemetry_reward_projection_state','telemetry_rollup_battle_coverage',"
	"'telemetry_rollup_battle_coverage_v6','telemetry_rollup_battle_coverage_v7','telemetry_rollup_battle_row',"
	"'telemetry_rollup_battle_row_v6','telemetry_rollup_battle_row_v7','telemetry_rollup_combat_actor',"
	"'telemetry_rollup_encounter','telemetry_rollup_encounter_participant',"
	"'telemetry_rollup_identity_coverage','telemetry_rollup_identity_effort',"
	"'telemetry_rollup_incident','telemetry_rollup_incident_coverage',"
	"'telemetry_rollup_level_event','telemetry_rollup_portfolio_xp',"
	"'telemetry_rollup_progression_day','telemetry_rollup_session','telemetry_rollup_state',"
	"'telemetry_session','timers','towns','world_quest_accomplished',"
	"'zone_story_quest_state','zone_touch_outcome','zone_touch_outcome_participant',"
	"'zone_touches','zone_trophy','zones'";
constexpr const char *RUNTIME_MYSQL8_METADATA_FINGERPRINT =
	"e06a960be47c0eceea443f808d4908ff5a1bab2d9f96651338aa5073847a6506";
constexpr const char *RUNTIME_MARIADB10_11_METADATA_FINGERPRINT =
	"e3b55899684dd05ba01bbdf0dd8b6af7a886c632c793cdc209f7025cd5d3f025";
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
constexpr const char *RUNTIME_MIGRATION_HEAD_ID = "0068_telemetry_control_publication";
constexpr unsigned RUNTIME_MIGRATION_HEAD_SEQUENCE = 71;
constexpr const char *RUNTIME_MIGRATION_APPLY_CHECKSUM =
	"f12dc17675d1139e8fcfc54d9cfbd2d8fa2acc488965b891bb8f968980f64c28";
constexpr const char *RUNTIME_MIGRATION_VERIFY_CHECKSUM =
	"ca003a4e0e99f838fa479730381a16a3ad34aed151561fa656c0284d379e60bc";
constexpr const char *RUNTIME_MIGRATION_HISTORY_CHECKSUM =
	"bb9f55bf17b7017ebc946d7f57c214f1573f959784c084f0f214e6b7f7f5cb75";
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_HEAD_ID = "0068_telemetry_control_publication";
constexpr unsigned RUNTIME_STAGING_0045_MIGRATION_HEAD_SEQUENCE = 71;
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_APPLY_CHECKSUM =
	"f12dc17675d1139e8fcfc54d9cfbd2d8fa2acc488965b891bb8f968980f64c28";
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_VERIFY_CHECKSUM =
	"ca003a4e0e99f838fa479730381a16a3ad34aed151561fa656c0284d379e60bc";
constexpr const char *RUNTIME_STAGING_0045_MIGRATION_HISTORY_CHECKSUM =
	"0d70a29ffc542c71339ddbcf611f4134855e39c0699e462b28321bd3448ee4c9";
/* Master recorded runtime state at 0031; its accounting upgrade retains that prefix. */
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_HEAD_ID = "0068_telemetry_control_publication";
constexpr unsigned RUNTIME_MASTER_0031_MIGRATION_HEAD_SEQUENCE = 71;
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_APPLY_CHECKSUM =
	"f12dc17675d1139e8fcfc54d9cfbd2d8fa2acc488965b891bb8f968980f64c28";
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_VERIFY_CHECKSUM =
	"ca003a4e0e99f838fa479730381a16a3ad34aed151561fa656c0284d379e60bc";
constexpr const char *RUNTIME_MASTER_0031_MIGRATION_HISTORY_CHECKSUM =
	"c31111eef4456962073ec775bbc5bf11e69c0f3f384e1a812c37ae848b99cbff";
/* Recorded telemetry prefixes converge by appending the three independent base migrations. */
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_HEAD_ID =
	"0068_telemetry_control_publication";
constexpr unsigned RUNTIME_TELEMETRY_0067_MIGRATION_HEAD_SEQUENCE = 71;
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_APPLY_CHECKSUM =
	"f12dc17675d1139e8fcfc54d9cfbd2d8fa2acc488965b891bb8f968980f64c28";
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_VERIFY_CHECKSUM =
	"ca003a4e0e99f838fa479730381a16a3ad34aed151561fa656c0284d379e60bc";
constexpr const char *RUNTIME_TELEMETRY_0067_MIGRATION_HISTORY_CHECKSUM =
	"7459a7f0e2d99083ecfeb2d8b6ff961641ac34e1673529af4a5318c4d1334e9e";
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_HEAD_ID =
	"0068_telemetry_control_publication";
constexpr unsigned RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_HEAD_SEQUENCE = 71;
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_APPLY_CHECKSUM =
	"f12dc17675d1139e8fcfc54d9cfbd2d8fa2acc488965b891bb8f968980f64c28";
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_VERIFY_CHECKSUM =
	"ca003a4e0e99f838fa479730381a16a3ad34aed151561fa656c0284d379e60bc";
constexpr const char *RUNTIME_TELEMETRY_0067_STAGING_0045_MIGRATION_HISTORY_CHECKSUM =
	"660685bd846baf1272dc0c15d6a9e63d07ebcaa5b330475af6985b1b0adff701";
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_HEAD_ID =
	"0068_telemetry_control_publication";
constexpr unsigned RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_HEAD_SEQUENCE = 71;
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_APPLY_CHECKSUM =
	"f12dc17675d1139e8fcfc54d9cfbd2d8fa2acc488965b891bb8f968980f64c28";
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_VERIFY_CHECKSUM =
	"ca003a4e0e99f838fa479730381a16a3ad34aed151561fa656c0284d379e60bc";
constexpr const char *RUNTIME_TELEMETRY_0067_MASTER_0031_MIGRATION_HISTORY_CHECKSUM =
	"149f936e7cbe87124ace61bff2a380547063511cc6f7c50f1147149879252977";
constexpr const char *RUNTIME_MIGRATION_HISTORY_SQL =
	"SELECT HEX(CONCAT(UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(migration_id USING utf8mb4))),16,'0')),CONVERT(migration_id USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(CAST(sequence_number AS CHAR) USING utf8mb4))),16,'0')),CONVERT(CAST(sequence_number AS CHAR) USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(description USING utf8mb4))),16,'0')),CONVERT(description USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(LOWER(HEX(apply_checksum)) USING utf8mb4))),16,'0')),CONVERT(LOWER(HEX(apply_checksum)) USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(LOWER(HEX(verify_checksum)) USING utf8mb4))),16,'0')),CONVERT(LOWER(HEX(verify_checksum)) USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(compatibility USING utf8mb4))),16,'0')),CONVERT(compatibility USING utf8mb4),UNHEX(LPAD(HEX(OCTET_LENGTH(CONVERT(CAST(runner_version AS CHAR) USING utf8mb4))),16,'0')),CONVERT(CAST(runner_version AS CHAR) USING utf8mb4))) FROM mud_schema_history ORDER BY sequence_number LIMIT 72";
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
