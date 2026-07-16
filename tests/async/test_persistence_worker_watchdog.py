#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
queue_c = (ROOT / "src/persistence_queue.c").read_text()
utility_c = (ROOT / "src/utility.c").read_text()
raw_c = (ROOT / "src/sql_persistence_raw.c").read_text()
test_c = (ROOT / "tests/async/test_persistence.c").read_text()

checks = []
checks.append(("persistence_queue.c defines item/scalar/large worker_stuck helpers",
               all(name in queue_c for name in [
                   "int persistence_item_event_worker_stuck(int threshold_secs)",
                   "int persistence_scalar_event_worker_stuck(int threshold_secs)",
                   "int persistence_large_event_worker_stuck(int threshold_secs)",
               ])))
checks.append(("worker stop paths use bounded join and keep stop gate set on timeout",
               queue_c.count("stop did not complete within %d sec; keeping stop gate set") >= 3 and
               "pthread_timedjoin_np" in queue_c and
               all(domain in queue_c for domain in ["domain=item_event", "domain=scalar_event", "domain=large_event"])))
checks.append(("stale heartbeat quarantines every generation instead of declaring pthread exit",
               queue_c.count("_worker_stop_pending_flag = 1;") >= 3 and
               "Heartbeat staleness is not proof that the pthread exited" in queue_c and
               all(stale_clear not in queue_c for stale_clear in [
                   "persistence_item_event_worker_is_running = 0;\n          running = 0;",
                   "persistence_scalar_event_worker_is_running = 0;\n          running = 0;",
                   "persistence_large_event_worker_is_running = 0;\n          running = 0;",
               ])))
checks.append(("worker loop marks in_write around callbacks",
               queue_c.count("_worker_in_write = 1;") >= 3 and queue_c.count("_worker_in_write = 0;") >= 3))
checks.append(("item event producer refreshes watchdog heartbeat",
               "persistence_worker_heartbeat_check(0);" in utility_c and
               "persistence_record_item_event" in utility_c))
checks.append(("raw SQL executor drains multi-statement result sets",
               "mysql_more_results" in raw_c and "mysql_next_result" in raw_c and "mysql_store_result" in raw_c))
checks.append(("scalar stale-heartbeat regression test is present",
               "worker_scalar_stale_heartbeat_shutdown_fallback" in test_c and
               "stale-heartbeat helper should report a stuck worker before stop" in test_c and
               "elapsed_ms < 150" in test_c))

checks.append(("worker start paths never clear quarantine via pthread_kill ESRCH",
               "pthread_kill(persistence_item_event_worker_thread, 0) == ESRCH" not in queue_c and
               "pthread_kill(persistence_scalar_event_worker_thread, 0) == ESRCH" not in queue_c and
               "pthread_kill(persistence_large_event_worker_thread, 0) == ESRCH" not in queue_c and
               queue_c.count("Quarantine is cleared only by the stop path after a successful join.") == 3 and
               queue_c.count("_worker_stop_pending_flag;\n  if (stuck)") >= 3))
checks.append(("worker stop paths join quarantined generations even after is_running clears",
               all(f"was_running = persistence_{domain}_event_worker_is_running ||" in queue_c
                   for domain in ["item", "scalar", "large"])))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll persistence watchdog checks passed.")
