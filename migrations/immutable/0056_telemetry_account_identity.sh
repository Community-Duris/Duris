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
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('int','bigint','smallint','tinyint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable,':',COALESCE(column_default,'NULL'),':',extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_account_lifetime' ORDER BY ordinal_position;" | paste -sd'|' -)
[[ "$shape" == 'lifetime_id:bigint unsigned:NO:NULL:|account_name:varchar(50):YES:NULL:' ]] || { echo 'telemetry_account_lifetime shape differ' >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',index_type,':',GROUP_CONCAT(CONCAT(column_name,':',COALESCE(sub_part,0)) ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_account_lifetime' GROUP BY index_name,non_unique,index_type ORDER BY BINARY index_name;" | paste -sd'|' -)
[[ "$indexes" == 'PRIMARY:0:BTREE:lifetime_id:0|uq_telemetry_lifetime_account:0:BTREE:account_name:0' ]] || { echo 'telemetry_account_lifetime indexes differ' >&2; exit 1; }
constraints=$("${MYSQL[@]}" -e "SELECT constraint_name FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_account_lifetime' ORDER BY BINARY constraint_name;" | paste -sd'|' -)
[[ "$constraints" == 'PRIMARY|chk_telemetry_lifetime_nonzero|fk_telemetry_lifetime_account|uq_telemetry_lifetime_account' ]] || { echo 'telemetry_account_lifetime constraints differ' >&2; exit 1; }
checks=$("${MYSQL[@]}" -e "SELECT CONCAT(t.constraint_name,':',LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,' ',''),CHAR(96),''),'(',''),')',''),CHAR(10),''))) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='telemetry_account_lifetime' AND t.constraint_type='CHECK' ORDER BY BINARY t.constraint_name;" | paste -sd'|' -)
[[ "$checks" == 'chk_telemetry_lifetime_nonzero:lifetime_id<>0' ]] || { echo 'telemetry_account_lifetime checks differ' >&2; exit 1; }
foreign=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.ordinal_position,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule,':',BINARY k.referenced_table_schema=BINARY DATABASE()) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='telemetry_account_lifetime' AND k.referenced_table_name IS NOT NULL ORDER BY BINARY k.constraint_name,k.ordinal_position;" | paste -sd'|' -)
[[ "$foreign" == 'fk_telemetry_lifetime_account:1:account_name:accounts:account_name:CASCADE:SET NULL:1' ]] || { echo 'telemetry_account_lifetime foreign differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_account_lifetime';")
[[ "$engine" == InnoDB:utf8mb4_unicode_ci ]] || { echo 'telemetry_account_lifetime engine or collation differs' >&2; exit 1; }
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('int','bigint','smallint','tinyint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable,':',COALESCE(column_default,'NULL'),':',extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_account_token' ORDER BY ordinal_position;" | paste -sd'|' -)
[[ "$shape" == 'environment_id:bigint unsigned:NO:NULL:|season_id:bigint unsigned:NO:NULL:|account_token:bigint unsigned:NO:NULL:|lifetime_id:bigint unsigned:NO:NULL:' ]] || { echo 'telemetry_account_token shape differ' >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',index_type,':',GROUP_CONCAT(CONCAT(column_name,':',COALESCE(sub_part,0)) ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_account_token' GROUP BY index_name,non_unique,index_type ORDER BY BINARY index_name;" | paste -sd'|' -)
[[ "$indexes" == 'PRIMARY:0:BTREE:environment_id:0,season_id:0,account_token:0|idx_telemetry_token_lifetime:1:BTREE:lifetime_id:0|uq_telemetry_token_lifetime:0:BTREE:environment_id:0,season_id:0,lifetime_id:0' ]] || { echo 'telemetry_account_token indexes differ' >&2; exit 1; }
constraints=$("${MYSQL[@]}" -e "SELECT constraint_name FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_account_token' ORDER BY BINARY constraint_name;" | paste -sd'|' -)
[[ "$constraints" == 'PRIMARY|chk_telemetry_token_nonzero|fk_telemetry_token_lifetime|uq_telemetry_token_lifetime' ]] || { echo 'telemetry_account_token constraints differ' >&2; exit 1; }
checks=$("${MYSQL[@]}" -e "SELECT CONCAT(t.constraint_name,':',LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,' ',''),CHAR(96),''),'(',''),')',''),CHAR(10),''))) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='telemetry_account_token' AND t.constraint_type='CHECK' ORDER BY BINARY t.constraint_name;" | paste -sd'|' -)
[[ "$checks" == 'chk_telemetry_token_nonzero:environment_id<>0andseason_id<>0andaccount_token<>0andlifetime_id<>0' ]] || { echo 'telemetry_account_token checks differ' >&2; exit 1; }
foreign=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.ordinal_position,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule,':',BINARY k.referenced_table_schema=BINARY DATABASE()) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='telemetry_account_token' AND k.referenced_table_name IS NOT NULL ORDER BY BINARY k.constraint_name,k.ordinal_position;" | paste -sd'|' -)
[[ "$foreign" == fk_telemetry_token_lifetime:1:lifetime_id:telemetry_account_lifetime:lifetime_id:RESTRICT:RESTRICT:1 ]] || { echo 'telemetry_account_token foreign differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_account_token';")
[[ "$engine" == InnoDB:utf8mb4_unicode_ci ]] || { echo 'telemetry_account_token engine or collation differs' >&2; exit 1; }
binding=$("${MYSQL[@]}" -e "SELECT CONCAT(character_set_name,':',collation_name) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_account_lifetime' AND column_name='account_name';")
[[ "$binding" == utf8mb4:utf8mb4_unicode_ci ]] || { echo 'account binding collation differs' >&2; exit 1; }
if [[ $("${MYSQL[@]}" -e 'SELECT VERSION();') != *MariaDB* ]]; then
    unenforced=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND constraint_type='CHECK' AND table_name IN ('telemetry_account_lifetime','telemetry_account_token') AND enforced <> 'YES';")
    [[ "$unenforced" == 0 ]] || { echo 'telemetry account identity checks are not enforced' >&2; exit 1; }
fi
printf 'telemetry account identity schema verified\n'
