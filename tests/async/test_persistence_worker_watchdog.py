#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
queue_c = (ROOT / "src/persistence_queue.c").read_text()
utility_c = (ROOT / "src/utility.c").read_text()
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
checks.append(("worker loop marks in_write around callbacks",
               queue_c.count("_worker_in_write = 1;") >= 3 and queue_c.count("_worker_in_write = 0;") >= 3))
checks.append(("item event producer refreshes watchdog heartbeat",
               "persistence_worker_heartbeat_check(0);" in utility_c and
               "persistence_record_item_event" in utility_c))
checks.append(("scalar stale-heartbeat regression test is present",
               "worker_scalar_stale_heartbeat_shutdown_fallback" in test_c and
               "stale-heartbeat helper should report a stuck worker before stop" in test_c and
               "elapsed_ms < 150" in test_c))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll persistence watchdog checks passed.")
