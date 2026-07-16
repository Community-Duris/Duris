from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
QUEUE = (ROOT / "src/persistence_queue.c").read_text()

workers = (
    "persistence_item_event_worker_main",
    "persistence_scalar_event_worker_main",
    "persistence_large_event_worker_main",
)

assert "#include <mysql.h>" in QUEUE
assert "mysql_thread_init" in QUEUE
assert "mysql_thread_end" in QUEUE

for worker in workers:
    start = QUEUE.index(f"static void *{worker}(void *unused)")
    next_worker = QUEUE.find("\nstatic void *", start + 1)
    body = QUEUE[start:] if next_worker < 0 else QUEUE[start:next_worker]
    assert "mysql_thread_init" in body, f"missing mysql_thread_init in {worker}"
    assert "mysql_thread_end" in body, f"missing mysql_thread_end in {worker}"

print("mysql worker thread lifecycle checks passed")
