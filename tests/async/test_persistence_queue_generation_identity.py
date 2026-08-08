#!/usr/bin/env python3
"""Queue completion must use per-slot identity, never serialized text."""
from pathlib import Path
import re

source = (Path(__file__).resolve().parents[2] / "src/persistence_queue.c").read_text()
compact = re.sub(r"\s+", " ", source)

assert "unsigned long long *generations;" in source
assert "unsigned long long next_generation;" in source
assert "q->generations[q->tail] = persistence_queue_next_generation(q);" in source
assert "new_generations[i] = q->generations[old_idx];" in source
assert "free(q->generations);" in source

# Every worker snapshots and validates the exact queue generation it wrote.
for domain in ("item", "scalar", "large"):
    assert f"persistence_{domain}_event_worker_generation" in source, domain
    assert (
        f"persistence_{domain}_event_queue.generations[ "
        f"persistence_{domain}_event_queue.head] == "
        f"persistence_{domain}_event_worker_generation"
    ) in compact, domain

assert "strncmp(line," not in source
print("persistence queue generation identity checks passed")
