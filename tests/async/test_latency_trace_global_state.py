#!/usr/bin/env python3
"""Latency state must be process-global when the header is included widely."""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (SRC / "latency_trace.h").read_text()
makefile = (SRC / "Makefile").read_text()
impl = (SRC / "latency_trace.c").read_text()

assert "extern latency_entry _latency_buf" in header
assert "extern pthread_mutex_t _latency_mutex" in header
assert "extern _latency_section _latency_sections" in header
assert "latency_entry _latency_buf" in impl
assert "pthread_mutex_t _latency_mutex" in impl
assert "latency_trace.o" in makefile

print("process-global latency trace checks passed")
