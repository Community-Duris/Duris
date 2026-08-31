#!/usr/bin/env python3
"""Every persistence worker must bring the client library up before using it.

MySQL 8's client refuses mysql_thread_init() until mysql_library_init() has run.
MariaDB Connector/C initialises on demand and never needs it, so a MariaDB
development box cannot see the problem at all. Every worker in this tree treats
a failed mysql_thread_init() as "give up and exit", and a worker that never
started has no connection to report on -- so against a MySQL 8 client the save,
load, maintenance, outbox and locker queues stopped draining in silence.

Pin two things: the raw call is not reachable outside the shared helper, and the
helper still initialises the library exactly once.
"""

from _paths import SRC
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
failures = []


def check(label, condition):
    print(("[PASS] " if condition else "[FAIL] ") + label)
    if not condition:
        failures.append(label)


header = (SRC / "sql_thread_init.h").read_text(encoding="utf-8")
check("the helper initialises the library once", "std::call_once" in header)
check("the helper calls mysql_library_init", "mysql_library_init(0, nullptr, nullptr)" in header)
check("the helper still reports a per-thread failure", "return mysql_thread_init();" in header)
check("the helper has a __NO_MYSQL__ build", "#else" in header and "__NO_MYSQL__" in header)

# No worker may reach past the helper to the raw entry point.
offenders = []
for path in sorted(SRC.rglob("*.c")) + sorted(SRC.rglob("*.h")):
    if path.name == "sql_thread_init.h" or SRC / "no_mysql" in path.parents:
        continue
    body = path.read_text(encoding="utf-8", errors="replace")
    # Drop the helper's own name first so the substring inside it is not a hit.
    if re.search(r"\bmysql_thread_init\s*\(", body.replace("sql_worker_thread_init", "")):
        offenders.append(path.name)
check("no source calls mysql_thread_init() directly: " + (", ".join(offenders) or "none"),
      not offenders)

# Each worker that spawns threads has to route through the helper.
for name in [
    "player_save_worker.c",
    "player_save_pipeline.c",
    "player_load_pipeline.c",
    "maintenance_scheduler.c",
    "critical_outbox.c",
    "critical_command_repository.c",
    "persistence_queue.c",
    "locker_async.c",
]:
    body = (SRC / name).read_text(encoding="utf-8", errors="replace")
    check(f"{name} routes through the helper",
          "sql_worker_thread_init()" in body and '#include "sql/sql_thread_init.h"' in body)

# The behaviour itself: without the library init, MySQL 8 fails and the worker
# would have exited. Compile against whichever client is installed and assert the
# helper succeeds -- on MariaDB this passes either way, on MySQL 8 it is the
# regression.
PROBE = r'''
#include "sql/sql_thread_init.h"

#include <cstdio>
#include <thread>

int main()
{
	int rc = -1;
	std::thread worker([&] {
		rc = sql_worker_thread_init();
#ifndef __NO_MYSQL__
		if (rc == 0)
			mysql_thread_end();
#endif
	});
	worker.join();
	if (rc != 0)
	{
		std::printf("sql_worker_thread_init() failed in a worker thread: %d\n", rc);
		return 1;
	}
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-sql-thread-init-") as directory:
    source = Path(directory) / "probe.cpp"
    binary = Path(directory) / "probe"
    source.write_text(PROBE, encoding="utf-8")
    # Ask the installed client for its own flags the way the other database
    # harnesses do, so this runs against MariaDB or MySQL without editing.
    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread", f"-I{SRC}",
         *cflags, str(source), *libs, "-o", str(binary)],
        check=True,
    )
    completed = subprocess.run([str(binary)], capture_output=True, text=True, timeout=30)
    check("a worker thread can initialise the installed client library",
          completed.returncode == 0)
    if completed.returncode != 0:
        print(completed.stdout.strip())

if failures:
    print("\nFailed regression checks:")
    for item in failures:
        print("- " + item)
    raise SystemExit(1)
print("\nsql worker thread init contract passed")
