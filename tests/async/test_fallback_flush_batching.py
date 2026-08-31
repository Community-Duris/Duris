#!/usr/bin/env python3
"""Fallback flushes must retain bounded recovery batches and drain in chunks."""
from _paths import SRC
from pathlib import Path
import re

source = (SRC / "utility.c").read_text()
compact = re.sub(r"\s+", " ", source)

for domain in ("item", "scalar"):
    start = source.index(f"int persistence_flush_{domain}_events")
    end = source.index("\n}\n", start) + 3
    body = source[start:end]
    assert "PERSISTENCE_FLUSH_BATCH_MAX" in body, domain
    assert "max_events > PERSISTENCE_FLUSH_BATCH_MAX" in body, domain

assert "while (persistence_scalar_event_queue_pending() > 0)" in source
assert "while (persistence_item_event_queue_pending() > 0)" in source
assert "persistence_flush_scalar_events(PERSISTENCE_FLUSH_BATCH_MAX)" in compact
assert "persistence_flush_item_events(PERSISTENCE_FLUSH_BATCH_MAX)" in compact

print("bounded fallback flush checks passed")
