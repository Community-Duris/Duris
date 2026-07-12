from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()

assert source.count("if (fflush(log_f) || fsync(fileno(log_f)))") >= 3
assert source.count("persistence_write_fallback_event_line(line, \"item_event\"") == 1
assert source.count("persistence_write_fallback_event_line(line, \"scalar_event\"") == 1

print("fallback writer durability checks passed")
