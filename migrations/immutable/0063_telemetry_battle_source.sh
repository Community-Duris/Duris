#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if [[ -n "${DB_SOCKET:-}" ]]; then
    [[ "$DB_SOCKET" == /* ]] || { echo 'database socket must be absolute' >&2; exit 1; }
    CONNECTION=(--protocol=socket --socket="$DB_SOCKET")
else
    CONNECTION=(--protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}")
    help=$(mysql --no-defaults --help)
    if [[ "$DB_HOST" == 127.0.0.1 || "$DB_HOST" == localhost || "$DB_HOST" == ::1 ]]; then
        if [[ "$help" == *--ssl-mode* ]]; then CONNECTION+=(--ssl-mode=PREFERRED); else CONNECTION+=(--skip-ssl); fi
    else
        [[ "${DB_TLS:-}" == TRUE && -f "${DB_SSL_CA:-}" ]] || { echo 'remote verification requires TLS and a CA file' >&2; exit 1; }
        if [[ "$help" == *--ssl-mode* ]]; then
            CONNECTION+=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
        elif [[ "$help" == *--ssl-verify-server-cert* ]]; then
            CONNECTION+=(--ssl-verify-server-cert --ssl-ca="$DB_SSL_CA")
        else
            echo 'database client cannot verify remote identity' >&2; exit 1
        fi
    fi
fi
MYSQL=(timeout 30 mysql --no-defaults --connect-timeout=10 "${CONNECTION[@]}" -u "$DB_USER" -N -B --raw "$DB_NAME")
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('int','bigint','smallint','tinyint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable,':',COALESCE(column_default,'NULL'),':',extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_battle_source' ORDER BY ordinal_position;" | paste -sd'|' -)
[[ "$shape" == 'definition_version:int unsigned:NO:NULL:|generation:bigint unsigned:NO:NULL:|environment_id:bigint unsigned:NO:NULL:|season_id:bigint unsigned:NO:NULL:|input_origin:bigint unsigned:NO:NULL:|input_watermark:bigint unsigned:NO:NULL:|source_fact_count:int unsigned:NO:NULL:|source_digest:binary(32):NO:NULL:|ownership_count:int unsigned:NO:NULL:|association_count:int unsigned:NO:NULL:|contribution_count:int unsigned:NO:NULL:|quality_flags:bigint unsigned:NO:NULL:|publication_complete:tinyint unsigned:NO:NULL:' ]] || { echo 'telemetry_battle_source shape differ' >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',index_type,':',GROUP_CONCAT(CONCAT(column_name,':',COALESCE(sub_part,0)) ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_battle_source' GROUP BY index_name,non_unique,index_type ORDER BY BINARY index_name;" | paste -sd'|' -)
[[ "$indexes" == PRIMARY:0:BTREE:definition_version:0,generation:0,environment_id:0,season_id:0 ]] || { echo 'telemetry_battle_source indexes differ' >&2; exit 1; }
constraints=$("${MYSQL[@]}" -e "SELECT constraint_name FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_battle_source' ORDER BY BINARY constraint_name;" | paste -sd'|' -)
[[ "$constraints" == 'PRIMARY|chk_battle_source_complete|chk_battle_source_counts|chk_battle_source_quality|chk_battle_source_scope|fk_battle_source_identity|fk_battle_source_state' ]] || { echo 'telemetry_battle_source constraints differ' >&2; exit 1; }
checks=$("${MYSQL[@]}" -e "SELECT CONCAT(t.constraint_name,':',REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,' ',''),CHAR(96),''),'(',''),')',''),CHAR(10),'')),'octet_length','length')) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='telemetry_battle_source' AND t.constraint_type='CHECK' ORDER BY BINARY t.constraint_name;" | paste -sd'|' -)
[[ "$checks" == 'chk_battle_source_complete:publication_completebetween0and1|chk_battle_source_counts:input_origin<=input_watermarkandsource_fact_count<=16384andsource_fact_count<=castinput_watermarkasdecimal21,0-castinput_originasdecimal21,0andsource_fact_count=ownership_count+association_count+contribution_count|chk_battle_source_quality:quality_flags&18446744073172745216=0|chk_battle_source_scope:definition_version=5andgeneration>0andenvironment_id>0andseason_id>0' ]] || { echo 'telemetry_battle_source checks differ' >&2; exit 1; }
foreign=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.ordinal_position,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule,':',BINARY k.referenced_table_schema=BINARY DATABASE()) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='telemetry_battle_source' AND k.referenced_table_name IS NOT NULL ORDER BY BINARY k.constraint_name,k.ordinal_position;" | paste -sd'|' -)
[[ "$foreign" == 'fk_battle_source_identity:1:definition_version:telemetry_generation_identity:definition_version:RESTRICT:RESTRICT:1|fk_battle_source_identity:2:generation:telemetry_generation_identity:generation:RESTRICT:RESTRICT:1|fk_battle_source_identity:3:environment_id:telemetry_generation_identity:environment_id:RESTRICT:RESTRICT:1|fk_battle_source_identity:4:season_id:telemetry_generation_identity:season_id:RESTRICT:RESTRICT:1|fk_battle_source_state:1:definition_version:telemetry_rollup_state:definition_version:RESTRICT:RESTRICT:1|fk_battle_source_state:2:generation:telemetry_rollup_state:generation:RESTRICT:RESTRICT:1|fk_battle_source_state:3:environment_id:telemetry_rollup_state:environment_id:RESTRICT:RESTRICT:1|fk_battle_source_state:4:season_id:telemetry_rollup_state:season_id:RESTRICT:RESTRICT:1' ]] || { echo 'telemetry_battle_source foreign keys differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_battle_source';" | paste -sd'|' -)
[[ "$engine" == InnoDB:utf8mb4_unicode_ci ]] || { echo 'telemetry_battle_source engine/collation differ' >&2; exit 1; }
if [[ $("${MYSQL[@]}" -e 'SELECT VERSION();') != *MariaDB* ]]; then
    enforced=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_battle_source' AND constraint_type='CHECK' AND enforced='YES';")
    [[ "$enforced" == 4 ]] || { echo 'telemetry_battle_source checks are not enforced' >&2; exit 1; }
fi
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('int','bigint','smallint','tinyint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable,':',COALESCE(column_default,'NULL'),':',extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_battle_input' ORDER BY ordinal_position;" | paste -sd'|' -)
[[ "$shape" == 'definition_version:int unsigned:NO:NULL:|generation:bigint unsigned:NO:NULL:|environment_id:bigint unsigned:NO:NULL:|season_id:bigint unsigned:NO:NULL:|ingest_id:bigint unsigned:NO:NULL:|boot_id:bigint unsigned:NO:NULL:|process_id:bigint unsigned:NO:NULL:|record_seq:bigint unsigned:NO:NULL:|record_kind:tinyint unsigned:NO:NULL:|payload:varbinary(8192):NO:NULL:|payload_digest:binary(32):NO:NULL:' ]] || { echo 'telemetry_battle_input shape differ' >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',index_type,':',GROUP_CONCAT(CONCAT(column_name,':',COALESCE(sub_part,0)) ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_battle_input' GROUP BY index_name,non_unique,index_type ORDER BY BINARY index_name;" | paste -sd'|' -)
[[ "$indexes" == 'PRIMARY:0:BTREE:definition_version:0,generation:0,environment_id:0,season_id:0,ingest_id:0|uq_battle_input_replay:0:BTREE:definition_version:0,generation:0,environment_id:0,season_id:0,boot_id:0,process_id:0,record_seq:0' ]] || { echo 'telemetry_battle_input indexes differ' >&2; exit 1; }
constraints=$("${MYSQL[@]}" -e "SELECT constraint_name FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_battle_input' ORDER BY BINARY constraint_name;" | paste -sd'|' -)
[[ "$constraints" == 'PRIMARY|chk_battle_input_kind|chk_battle_input_payload|chk_battle_input_receipt|chk_battle_input_scope|fk_battle_input_source|uq_battle_input_replay' ]] || { echo 'telemetry_battle_input constraints differ' >&2; exit 1; }
checks=$("${MYSQL[@]}" -e "SELECT CONCAT(t.constraint_name,':',REPLACE(LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,' ',''),CHAR(96),''),'(',''),')',''),CHAR(10),'')),'octet_length','length')) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='telemetry_battle_input' AND t.constraint_type='CHECK' ORDER BY BINARY t.constraint_name;" | paste -sd'|' -)
[[ "$checks" == 'chk_battle_input_kind:record_kindin9,10,11|chk_battle_input_payload:lengthpayloadbetween1and8192|chk_battle_input_receipt:ingest_id>0andboot_id>0andprocess_id>0andrecord_seq>0|chk_battle_input_scope:definition_version=5andgeneration>0andenvironment_id>0andseason_id>0' ]] || { echo 'telemetry_battle_input checks differ' >&2; exit 1; }
foreign=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.ordinal_position,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule,':',BINARY k.referenced_table_schema=BINARY DATABASE()) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='telemetry_battle_input' AND k.referenced_table_name IS NOT NULL ORDER BY BINARY k.constraint_name,k.ordinal_position;" | paste -sd'|' -)
[[ "$foreign" == 'fk_battle_input_source:1:definition_version:telemetry_battle_source:definition_version:RESTRICT:RESTRICT:1|fk_battle_input_source:2:generation:telemetry_battle_source:generation:RESTRICT:RESTRICT:1|fk_battle_input_source:3:environment_id:telemetry_battle_source:environment_id:RESTRICT:RESTRICT:1|fk_battle_input_source:4:season_id:telemetry_battle_source:season_id:RESTRICT:RESTRICT:1' ]] || { echo 'telemetry_battle_input foreign keys differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_battle_input';" | paste -sd'|' -)
[[ "$engine" == InnoDB:utf8mb4_unicode_ci ]] || { echo 'telemetry_battle_input engine/collation differ' >&2; exit 1; }
if [[ $("${MYSQL[@]}" -e 'SELECT VERSION();') != *MariaDB* ]]; then
    enforced=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_battle_input' AND constraint_type='CHECK' AND enforced='YES';")
    [[ "$enforced" == 4 ]] || { echo 'telemetry_battle_input checks are not enforced' >&2; exit 1; }
fi
printf 'retained battle source checkpoint verified\n'
