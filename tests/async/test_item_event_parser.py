from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/sql.c").read_text()
start = source.index("bool sql_persistence_write_item_event_line")
end = source.index("\nbool sql_persistence_write_scalar_event_line", start)
body = source[start:end]
assert "static bool persistence_decimal_field" in source
assert "int seen_ts" in body
assert "!seen_ts || !seen_event" in body
assert "persistence_decimal_field(ts_usec, false)" in body
assert "persistence_decimal_field(actor_id, true)" in body
assert '"PERSISTENCE_ITEM_EVENT|"' in body
assert 'strtok_r(record, "|"' in body
assert 'field, "ts"' in body
assert 'field, "item_uid"' in body
assert 'INSERT INTO persistence_item_events' in body
assert 'ts_usec, event_q' in body
assert "ON DUPLICATE KEY UPDATE id=id" in body
assert "query_len = snprintf" in body
assert "query_len >= (int)sizeof(query)" in body
assert "SHA2(CONCAT_WS" in body
assert "sql_persistence_execute_raw(query)" in body
assert "sql_persistence_execute_raw(line)" not in body
print("item event parser checks passed")
