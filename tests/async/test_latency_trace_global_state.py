#!/usr/bin/env python3
"""Latency state must be process-global when the header is included widely."""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
header = (SRC / "latency_trace.h").read_text()
makefile = (SRC / "Makefile").read_text()
impl = (SRC / "latency_trace.c").read_text()
persistence_queue = (SRC / "persistence_queue.c").read_text()

assert "_latency_buf" not in header
assert "_latency_buf" not in impl
assert "_latency_head" not in header
assert "_latency_count" not in header
assert "static pthread_mutex_t latency_mutex" in impl
assert "static latency_section latency_sections" in impl
assert "strcmp(latency_sections[index].name, name)" in impl
assert "latency_trace.o" in makefile
assert "LATENCY_TRACE_TICK_UNAVAILABLE" in header
assert "latency_trace_snapshot_take_and_reset" in header
assert "latency_trace_snapshot_dump" in header
assert "dropped_section_samples" in header
assert "dropped_contended_samples" in header
assert "latency_trace_record_nonblocking" in header
assert "pthread_mutex_trylock(&latency_mutex)" in impl
enqueue = persistence_queue[
    persistence_queue.index("int persistence_scalar_event_queue_enqueue") :
    persistence_queue.index("int persistence_scalar_event_queue_dequeue")
]
unlock = enqueue.index("pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);")
for call in ("scalar_enq_drop", "scalar_enq_ok"):
    assert enqueue.index(f'latency_trace_record_nonblocking("{call}"') > unlock


utility = (SRC / "core" / "utility.c").read_text()
fallback_start = utility.index("const uint64_t fallback_finished_us")
fallback_unlock = utility.index(
    "pthread_mutex_unlock(&persistence_fallback_log_mutex);", fallback_start
)
assert 'latency_trace_record("fallback_file_write"' not in utility
assert utility.index(
    'latency_trace_record_nonblocking("fallback_file_write"', fallback_start
) > fallback_unlock

print("process-global latency trace checks passed")
