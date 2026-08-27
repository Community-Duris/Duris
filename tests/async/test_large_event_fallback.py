from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()
start = source.index("static int persistence_large_event_log_writer")
end = source.index("\n}\n", start) + 3
body = source[start:end]
replay_start = source.index("int persistence_replay_fallback_events")
replay = source[replay_start:].split("#if 0", 1)[0]
assert "sql_persistence_write_large_event_line" not in replay
assert "persistence_quarantine_fallback_events" in replay
assert "PERSISTENCE_LARGE_EVENT_PREFIX, prefix_len" in body
assert "memcpy(fallback_line + prefix_len, line, line_len + 1)" in body
assert "free(fallback_line)" in body
assert '"large_event"' in body
assert '"worker_fallback"' in body
start_worker = source.index("int persistence_start_large_event_worker")
active_worker = source[start_worker:].split("#if 0", 1)[0]
assert "raw_execution_disabled" in active_worker
assert "return 0" in active_worker
print("large-event fallback retirement checks passed")
