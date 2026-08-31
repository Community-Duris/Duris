from _paths import SRC
from pathlib import Path
from contract_text import count

source = (SRC / "utility.c").read_text()

assert count(source, "if (fflush(log_f) || fsync(fileno(log_f)))") >= 3
# One worker-path fallback write per domain.  The owner tag is part of the
# pattern: the enqueue-failure path in do_item_event() also writes item_event
# lines, tagged with the acting player rather than "worker".
assert count(source, "persistence_write_fallback_event_line(line, \"item_event\", \"worker\"") == 1
assert count(source, "persistence_write_fallback_event_line(line, \"scalar_event\", \"worker\"") == 1

print("fallback writer durability checks passed")
