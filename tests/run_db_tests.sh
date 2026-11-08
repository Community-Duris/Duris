#!/usr/bin/env bash
# make test-db: the isolated database legs and the MariaDB journeys, side by side.
# One test per line below: a name, then its command. Every test brings its own
# container, so each line stands alone. Logs are kept under bin/tests/db/.
set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

export LOGS=bin/tests/db
rm -rf "$LOGS"
mkdir -p "$LOGS"

run_one() {
    local name=$1 started=$SECONDS
    shift
    if "$@" >"$LOGS/$name.log" 2>&1; then
        echo "PASS $name ($((SECONDS - started))s)"
    else
        echo "FAIL $name ($((SECONDS - started))s)"
        touch "$LOGS/$name.failed"
        return 1
    fi
}
export -f run_one

DB=tests/async/with_disposable_mariadb.sh
SERVER=${DURIS_SERVER_BINARY:-bin/server/dms_new}
started=$SECONDS

# Longest first.
xargs -P "${TEST_DB_JOBS:-12}" -L 1 bash -c 'run_one "$@"' _ <<EOF
artifact_guild_reward_journey $DB python3 tests/async/test_mysql_artifact_guild_reward_journey.py --server $SERVER
epic_training_journey $DB python3 tests/async/test_mysql_epic_training_journey.py --server $SERVER
epic_task_recovery_journey $DB python3 tests/async/test_epic_task_recovery_journey.py --server $SERVER
epic_task_catalog $DB python3 tests/async/test_epic_task_catalog_mysql.py
game_loop_budget $DB python3 tests/async/test_mysql_game_loop_budget_journey.py --server $SERVER
world_recovery $DB python3 tests/async/run_world_recovery_journey.py $SERVER
world_restart_crash $DB python3 tests/async/run_world_restart_journey.py $SERVER crash
world_capture $DB python3 tests/async/test_mysql_world_capture_journey.py --server $SERVER
legacy_migration tests/async/run_legacy_migration_mysql.sh
telemetry_repository $DB python3 tests/async/test_telemetry_repository.py --sql-fixture
telemetry_schema_boot $DB python3 tests/async/run_telemetry_schema_boot_journey.py --server $SERVER
saved_item_allocator $DB python3 tests/async/run_saved_item_allocator_journey.py $SERVER
mysql_combat $DB python3 tests/async/test_mysql_combat_journey.py --server $SERVER
flight_network $DB python3 tests/async/test_flight_network_journey.py --backend mariadb --server $SERVER
world_restart_copyover $DB python3 tests/async/run_world_restart_journey.py $SERVER copyover
world_restart_midcapture $DB python3 tests/async/run_world_restart_journey.py $SERVER midcapture
world_restart_clean $DB python3 tests/async/run_world_restart_journey.py $SERVER restart
world_restart_taken $DB python3 tests/async/run_world_restart_journey.py $SERVER taken
world_restart_handover $DB python3 tests/async/run_world_restart_journey.py $SERVER handover
world_restart_slowread $DB python3 tests/async/run_world_restart_journey.py $SERVER slowread
saved_item_recovery $DB python3 tests/async/run_saved_item_recovery_journey.py $SERVER
playtime $DB python3 tests/async/test_mysql_playtime_journey.py --server $SERVER
runtime_compatibility tests/async/run_runtime_compatibility_mysql.sh
runtime_compatibility_mariadb_11_8 env RUNTIME_DB_IMAGE=mariadb:11.8 tests/async/run_runtime_compatibility_mysql.sh
migration_runner_engines python3 tests/async/run_migration_runner_engines.py
corpse_haul $DB python3 tests/async/run_corpse_haul_journey.py $SERVER
corpse_haul_count_cap $DB python3 tests/async/run_corpse_haul_count_cap_journey.py $SERVER mariadb
corpse_trap_restart $DB python3 tests/async/test_corpse_trap_restart_journey.py $SERVER mariadb
information_cache $DB python3 tests/async/test_information_cache_journey.py --backend mariadb --server $SERVER
deletion $DB python3 tests/async/run_mysql_deletion_journey.py --server $SERVER
stalled_writer tests/async/run_mysql_stalled_writer_journey.sh
game_loop_queries $DB python3 tests/async/test_mysql_game_loop_queries_journey.py --server $SERVER
bank_restart $DB python3 tests/async/run_mysql_bank_restart_journey.py $SERVER
idle_timeout $DB python3 tests/async/run_mysql_idle_timeout_journey.py $SERVER
address_retention $DB python3 tests/async/run_address_retention_journey.py $SERVER
level_cap $DB python3 tests/async/run_level_cap_mysql.py
launcher_stop $DB python3 tests/async/run_launcher_stop_journey.py
chaos_raise $DB python3 tests/async/run_chaos_raise_transient_journey.py $SERVER
collector_intake $DB python3 tests/async/run_mysql_collector_intake_journey.py $SERVER
world_writer_retry $DB python3 tests/async/run_world_writer_retry_journey.py $SERVER
collector_repository tests/async/run_collector_repository_schema_mysql.sh
account_bound_reward tests/async/run_account_bound_reward_schema_mysql.sh
pet_repository tests/async/run_pet_repository_mysql.sh
player_save_claim tests/async/run_player_save_claim_mysql.sh
sql_pool_interrupt tests/async/run_sql_pool_interrupt_mysql.sh
experience_trophy tests/async/run_experience_trophy_mysql.sh
item_transfer tests/async/run_item_transfer_schema_mysql.sh
collector_catalog tests/async/run_collector_catalog_schema_mysql.sh
epic_zone_seed_mysql python3 tests/async/run_epic_zone_seed_mysql.py --image mysql:8.0
epic_zone_seed_mariadb python3 tests/async/run_epic_zone_seed_mysql.py --image mariadb:11.4
lookup_dataset tests/async/run_lookup_dataset_mysql.sh
persistence_contract tests/async/run_persistence_contract_mysql.sh
account_erasure tests/async/run_account_erasure_schema_mysql.sh
immutable_migration_ledger tests/async/run_immutable_migration_ledger_mysql.sh
corpse_persistence tests/async/run_corpse_persistence_schema_mysql.sh
collector_item_owner tests/async/run_collector_item_owner_schema_mysql.sh
output_preferences tests/async/run_output_preferences_mysql.sh
personal_data_export tests/async/run_personal_data_export_schema_mysql.sh
lifecycle_archive tests/async/run_lifecycle_archive_schema_mysql.sh
telemetry_reports_mariadb python3 tests/async/run_telemetry_reports_mysql.py --image mariadb:10.11
telemetry_reports_mysql python3 tests/async/run_telemetry_reports_mysql.py --image mysql:8.0
critical_command_schema tests/async/run_critical_command_schema_mysql.sh
combat_baseline_repair tests/async/run_combat_baseline_repair_mysql.sh
collector_notification tests/async/run_collector_notification_mysql.sh
boon_reward_zone $DB tests/async/run_boon_reward_zone_schema_mysql.sh
combat_outcome $DB tests/async/run_combat_outcome_schema_mysql.sh
account_bank_delta tests/async/run_account_bank_delta_mysql.sh
account_locker_conversion_check tests/async/run_account_locker_conversion_check_mysql.sh
legacy_personal_locker_access_repair tests/async/run_legacy_personal_locker_access_repair_mysql.sh
player_replacement_state tests/async/run_player_replacement_state_mysql.sh
epic_transaction $DB tests/async/run_epic_transaction_schema_mysql.sh
auction_transaction $DB tests/async/run_auction_transaction_schema_mysql.sh
artifact_guild $DB tests/async/run_artifact_guild_schema_mysql.sh
statistics $DB bash tests/async/run_statistics_mysql.sh
session_audit $DB tests/async/run_session_audit_schema_mysql.sh
artifact_mana tests/async/run_artifact_mana_mysql.sh
player_load_repository $DB tests/async/run_player_load_repository_mysql.sh
character_rename_references $DB tests/async/run_character_rename_references_mysql.sh
account_character_identity $DB tests/async/run_account_character_identity_mysql.sh
account_character_projection $DB tests/async/run_account_character_projection_mysql.sh
EOF
status=$?

for failed in "$LOGS"/*.failed; do
    [ -e "$failed" ] || continue
    echo
    echo "--- ${failed%.failed}.log"
    tail -n 40 "${failed%.failed}.log"
done
echo
echo "make test-db: $(ls "$LOGS"/*.log | wc -l) tests in $((SECONDS - started))s"
[ "$status" -eq 0 ]
