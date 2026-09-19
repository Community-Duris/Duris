#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
"${MYSQL[@]}" -e '
SELECT IF(
 (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN
   ("artifact_config_revision","artifact_config_head","artifact_config_draft",
    "artifact_config_publish_audit","artifact_instance_policy","artifact_power_policy",
    "artifact_control_request","artifact_control_result","artifact_control_lease"))=9
 AND (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()
   AND TABLE_NAME="artifact_config_revision" AND COLUMN_NAME="document" AND DATA_TYPE="longtext")=1
 AND (SELECT COUNT(*) FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE()
   AND TABLE_NAME="artifact_control_request" AND COLUMN_NAME="idempotency_key")=1
 AND (SELECT COUNT(*) FROM information_schema.VIEWS WHERE TABLE_SCHEMA=DATABASE()
   AND TABLE_NAME="artifact_control_active")=1
 AND (SELECT COUNT(*) FROM information_schema.ROUTINES WHERE ROUTINE_SCHEMA=DATABASE()
   AND ROUTINE_NAME IN ("artifact_control_request_submit","artifact_control_request_status")
   AND ROUTINE_TYPE="PROCEDURE")=2,
 "ok", "missing")' | grep -qx ok
"${MYSQL[@]}" -e 'START TRANSACTION;
  INSERT INTO artifact_control_request(idempotency_key,command,payload)
  VALUES("00000000-0000-0000-0000-000000000029","validate","{}");
  SET @request_id = LAST_INSERT_ID();
  INSERT INTO artifact_control_result(request_id,result_code,message)
  VALUES(@request_id,"ok","migration smoke test");
  ROLLBACK;'
