from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()
start = source.index("static int persistence_large_event_log_writer")
end = source.index("\n}\n", start) + 3
body = source[start:end]
replay_start = source.index("int persistence_replay_fallback_events")
replay_end = source.index("\n}\n", replay_start) + 3
replay = source[replay_start:replay_end]
assert "sql_persistence_write_large_event_line(large_sql)" in replay
assert "PERSISTENCE_LARGE_EVENT_PREFIX, prefix_len" in body
assert "memcpy(fallback_line + prefix_len, line, line_len + 1)" in body
assert "free(fallback_line)" in body
assert '"large_event"' in body
assert '"worker_fallback"' in body
print("large-event fallback checks passed")
