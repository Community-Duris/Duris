#!/usr/bin/env python3
"""Queue completion must use per-slot identity, never serialized text."""
from _paths import SRC
from pathlib import Path
import re
from contract_text import contains

source = (SRC / "persistence_queue.c").read_text()

assert contains(source, "unsigned long long *generations;")
assert contains(source, "unsigned long long next_generation;")
assert contains(source, "q->generations[q->tail] = persistence_queue_next_generation(q);")
assert contains(source, "new_generations[i] = q->generations[old_idx];")
assert contains(source, "free(q->generations);")

# Every worker snapshots and validates the exact queue generation it wrote.
for domain in ("item", "scalar", "large"):
    assert contains(source, f"persistence_{domain}_event_worker_generation"), domain
    assert contains(
        source,
        f"persistence_{domain}_event_queue.generations["
        f"persistence_{domain}_event_queue.head] == "
        f"persistence_{domain}_event_worker_generation",
    ), domain

assert not contains(source, "strncmp(line,")
print("persistence queue generation identity checks passed")
