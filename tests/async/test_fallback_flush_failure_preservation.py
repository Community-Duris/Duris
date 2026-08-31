from _paths import SRC
from pathlib import Path

source = (SRC / "utility.c").read_text()

for start_marker, enqueue_name, label in (
    ("int persistence_flush_item_events(int max_events)", "persistence_item_event_queue_enqueue(line)", "item"),
    ("int persistence_flush_scalar_events(int max_events)", "persistence_scalar_event_queue_enqueue(line)", "scalar"),
):
    start = source.index(start_marker)
    end = source.index("\n}\n", start) + 3
    body = source[start:end]
    assert "ftell(log_f)" in body, label
    assert "setvbuf(log_f, NULL, _IONBF, 0)" in body, label
    assert "ftruncate(fileno(log_f)" in body, label
    assert enqueue_name in body, label

print("fallback flush failure-preservation checks passed")
