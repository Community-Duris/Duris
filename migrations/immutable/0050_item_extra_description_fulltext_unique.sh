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

columns=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name IN ('player_item_extra_descr','player_pet_item_extra_descr')
  AND column_name='description_sha256'
  AND data_type='binary'
  AND column_type='binary(32)'
  AND extra LIKE '%STORED GENERATED%'
  AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(generation_expression,
      CONCAT(CHAR(92),CHAR(39),CHAR(92),CHAR(39)),CONCAT(CHAR(39),CHAR(39))),
      CHAR(96), ''), ' ', ''), '_utf8mb4', ''))
      = 'unhex(sha2(coalesce(description,''''),256))';")
indexes=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM (
  SELECT table_name,index_name,non_unique,
         GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') AS columns_signature,
         SUM(sub_part IS NOT NULL) AS prefix_columns
  FROM information_schema.statistics
  WHERE table_schema=DATABASE()
    AND index_name IN ('uk_item_descr','uk_pet_item_descr')
  GROUP BY table_name,index_name,non_unique
  HAVING non_unique=0 AND prefix_columns=0 AND (
    (table_name='player_item_extra_descr' AND index_name='uk_item_descr'
      AND columns_signature='item_id,keyword,description_sha256') OR
    (table_name='player_pet_item_extra_descr' AND index_name='uk_pet_item_descr'
      AND columns_signature='item_id,keyword,description_sha256')
  )
) exact_indexes;")
player_duplicates=$("${MYSQL[@]}" -e "
SELECT COUNT(*) FROM (
  SELECT item_id,keyword,description_sha256
  FROM player_item_extra_descr
  GROUP BY item_id,keyword,description_sha256
  HAVING COUNT(*)>1
) duplicates;")
pet_duplicates=$("${MYSQL[@]}" -e "
SELECT COUNT(*) FROM (
  SELECT item_id,keyword,description_sha256
  FROM player_pet_item_extra_descr
  GROUP BY item_id,keyword,description_sha256
  HAVING COUNT(*)>1
) duplicates;")

[[ "$columns" == 2 && "$indexes" == 2 && "$player_duplicates" == 0 && "$pet_duplicates" == 0 ]]
printf 'player and pet item extra-description full-value uniqueness verified\n'
