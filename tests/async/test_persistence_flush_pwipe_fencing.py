from pathlib import Path

source = (Path(__file__).resolve().parents[2] / "src/utility.c").read_text()
for name, label in [
    ("int persistence_flush_item_events", "item"),
    ("int persistence_flush_scalar_events", "scalar"),
]:
    start = source.index(name)
    end = source.index("\n}\n", start) + 3
    body = source[start:end]
    assert "pthread_mutex_lock(&persistence_fallback_log_mutex)" in body, label
    assert "if (_pwipe)" in body, label
    assert "pthread_mutex_unlock(&persistence_fallback_log_mutex)" in body, label
    assert "fsync(fileno(log_f))" in body, label

print("queue fallback drain fencing checks passed")
