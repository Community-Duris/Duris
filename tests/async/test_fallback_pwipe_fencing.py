from pathlib import Path
from contract_text import contains, count, find, index

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()
start = index(source, "int persistence_write_fallback_event_line")
end = source.index("\n}\n", start) + 3
body = source[start:end]

assert contains(body, "if (_pwipe)")
assert contains(body, '"pwipe_rejected"')
assert contains(body, "fallback event rejected while season reset is active")
assert index(body, "if (_pwipe)") < index(body, "fopen(LOG_EVENT")
# One worker-path fallback write per domain; the owner tag is part of the
# pattern, since the enqueue-failure path also writes item_event lines.
assert count(source, "persistence_write_fallback_event_line(line, \"item_event\", \"worker\"") == 1
assert count(source, "persistence_write_fallback_event_line(line, \"scalar_event\", \"worker\"") == 1
assert count(source, 'fopen(LOG_EVENT, "a")') == 3

print("fallback pwipe fencing checks passed")
