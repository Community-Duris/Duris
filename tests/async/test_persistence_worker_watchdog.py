#!/usr/bin/env python3
from pathlib import Path
import sys
from contract_text import contains, count

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
               contains(queue_c, "pthread_timedjoin_np") and
               all(domain in queue_c for domain in ["domain=item_event", "domain=scalar_event", "domain=large_event"])))
checks.append(("stale heartbeat quarantines every generation instead of declaring pthread exit",
               queue_c.count("_worker_stop_pending_flag = 1;") >= 3 and
               contains(queue_c, "Heartbeat staleness is not proof that the pthread exited") and
               all(stale_clear not in queue_c for stale_clear in [
                   "persistence_item_event_worker_is_running = 0;\n          running = 0;",
                   "persistence_scalar_event_worker_is_running = 0;\n          running = 0;",
                   "persistence_large_event_worker_is_running = 0;\n          running = 0;",
               ])))
checks.append(("worker loop marks in_write around callbacks",
               queue_c.count("_worker_in_write = 1;") >= 3 and queue_c.count("_worker_in_write = 0;") >= 3))
checks.append(("item event producer refreshes watchdog heartbeat",
               contains(utility_c, "persistence_worker_heartbeat_check(0);") and
               contains(utility_c, "persistence_record_item_event")))
checks.append(("raw SQL executor is retired fail closed",
               contains(raw_c, "return false;") and
               not contains(raw_c, "mysql_real_query") and
               not contains(raw_c, "sql_observed_execute_at")))
checks.append(("scalar stale-heartbeat regression test is present",
               contains(test_c, "worker_scalar_stale_heartbeat_shutdown_fallback") and
               contains(test_c, "stale-heartbeat helper should report a stuck worker before stop") and
               contains(test_c, "elapsed_ms < 150")))

checks.append(("worker start paths never clear quarantine via pthread_kill ESRCH",
               not contains(queue_c, "pthread_kill(persistence_item_event_worker_thread, 0) == ESRCH") and
               not contains(queue_c, "pthread_kill(persistence_scalar_event_worker_thread, 0) == ESRCH") and
               not contains(queue_c, "pthread_kill(persistence_large_event_worker_thread, 0) == ESRCH") and
               count(queue_c, "Quarantine is cleared only by the stop path after a successful join.") == 3 and
               count(queue_c, "_worker_stop_pending_flag;\nif (stuck)") >= 3))
checks.append(("worker stop paths join quarantined generations even after is_running clears",
               all(contains(queue_c, f"was_running = persistence_{domain}_event_worker_is_running ||")
                   for domain in ["item", "scalar", "large"])))
checks.append(("worker_running ESRCH quarantines instead of clearing is_running",
               count(queue_c, "ESRCH is not a safe reap proof") >= 3 and
               all(not contains(queue_c, bad) for bad in [
                   "if (kill_rc == ESRCH)\n{\npersistence_item_event_worker_is_running = 0;",
                   "if (kill_rc == ESRCH)\n{\npersistence_scalar_event_worker_is_running = 0;",
                   "if (kill_rc == ESRCH)\n{\npersistence_large_event_worker_is_running = 0;",
               ])))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll persistence watchdog checks passed.")
