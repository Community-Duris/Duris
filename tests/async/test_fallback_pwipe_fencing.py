from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()
start = source.index("int persistence_write_fallback_event_line")
end = source.index("\n}\n", start) + 3
body = source[start:end]

assert "if (_pwipe)" in body
assert '"pwipe_rejected"' in body
assert "fallback event rejected while season reset is active" in body
assert body.index("if (_pwipe)") < body.index("fopen(LOG_EVENT")
assert source.count("persistence_write_fallback_event_line(line, \"item_event\"") == 1
assert source.count("persistence_write_fallback_event_line(line, \"scalar_event\"") == 1
assert source.count('fopen(LOG_EVENT, "a")') == 3

print("fallback pwipe fencing checks passed")
