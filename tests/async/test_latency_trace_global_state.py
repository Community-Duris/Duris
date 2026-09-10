#!/usr/bin/env python3
"""Latency state must be process-global when the header is included widely."""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (SRC / "latency_trace.h").read_text()
makefile = (SRC / "Makefile").read_text()
impl = (SRC / "latency_trace.c").read_text()

assert "_latency_buf" not in header
assert "_latency_buf" not in impl
assert "_latency_head" not in header
assert "_latency_count" not in header
assert "static pthread_mutex_t latency_mutex" in impl
assert "static latency_section latency_sections" in impl
assert "strcmp(latency_sections[index].name, name)" in impl
assert "latency_trace.o" in makefile
assert "LATENCY_TRACE_TICK_UNAVAILABLE" in header
assert "latency_trace_snapshot_capture" in header
assert "latency_trace_snapshot_dump" in header
assert "dropped_section_samples" in header

print("process-global latency trace checks passed")
