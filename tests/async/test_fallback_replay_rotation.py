from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()
start = source.index("int persistence_replay_fallback_events")
end = source.index("\n}\n", start) + 3
body = source[start:end]

assert "if (fflush(out_f) || fsync(fileno(out_f)))" in body
assert '"%s.persistence-replay.%ld.%ld"' in body
assert "getpid()" in body
assert "if (link(LOG_EVENT, backup_path))" in body
assert body.index("link(LOG_EVENT, backup_path)") < body.index("rename(tmp_path, LOG_EVENT)")
assert "rename(LOG_EVENT, backup_path)" not in body
assert "rename(backup_path, LOG_EVENT)" not in body

print("fallback replay rotation checks passed")
