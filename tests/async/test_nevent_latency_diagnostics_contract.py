from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")

assert '"NEVENT SLOW:' in source
assert "slowest_name" in source
assert "slowest_us" in source
assert "scanned" in source
assert "executed" in source
assert "clock_gettime(CLOCK_MONOTONIC, &callback_started" in source
assert "clock_gettime(CLOCK_MONOTONIC, &callback_finished" in source
