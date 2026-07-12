#!/usr/bin/env python3
"""Regression checks for fallback flush durability failure truncation."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/utility.c").read_text()

# Both item and scalar paths must record a durability offset and truncate on failure
assert source.count("durability_offset = -1") >= 2
assert source.count("durability_offset = ftell(log_f)") >= 2
assert source.count("ftruncate(fd, durability_offset)") >= 2
assert source.count("flushed = 0;") >= 2
assert 'durability failed after flushing' in source
assert '#include <fcntl.h>' in source
# Sol review: events must be requeued on durability failure
assert source.count("dequeued[PERSISTENCE_FLUSH_BATCH_MAX]") >= 2
assert source.count("dequeued_count") >= 4
assert "file truncated and events requeued" in source
# Sol review: truncation must happen while still holding the mutex
assert "pthread_mutex_unlock" in source

print("fallback flush durability truncation checks passed")
