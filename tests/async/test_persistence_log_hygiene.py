#!/usr/bin/env python3
"""Reject private values and bypasses in reviewed persistence diagnostics."""

from _paths import SRC
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REVIEWED = [
    "sql.c",
    "sql_persistence_raw.c",
    "locker_async.c",
    "sql_player.c",
    "sql_pool.c",
    "account.c",
    "actoth.c",
    "files.c",
    "nanny.c",
    "modify.c",
    "utility.c",
    "ws_handlers.c",
    "account_recovery.c",
    "account_recovery_nanny.c",
    "mail_sender.c",
]

texts = {name: (SRC / name).read_text() for name in REVIEWED}
combined = "\n".join(texts.values())

direct_sites = []
for name, text in texts.items():
    for match in re.finditer(r"\bmysql_real_query\s*\(", text):
        direct_sites.append((name, text.count("\n", 0, match.start()) + 1))

checks = [
    (
        "raw MySQL execution remains confined to sql.c",
        bool(direct_sites) and all(name == "sql.c" for name, _line in direct_sites),
    ),
    ("raw persistence execution is disabled while typed workers remain observed",
     "return false;" in texts["sql_persistence_raw.c"] and
     "sql_observed_execute_at" not in texts["sql_persistence_raw.c"] and
     "sql_observed_execute_at" in texts["locker_async.c"]),
    ("explicit trace labels remain semantic sites", "const struct persistence_query_site semantic_site" in texts["sql.c"] and "(void)label" not in texts["sql.c"]),
    ("fork context uses process origin", "static const pid_t sql_main_process_id = getpid();" in texts["sql.c"]),
    ("private trace file removed", "garp-item-trace" not in combined),
    ("real-persistence marker removed", "real-persistence-test" not in combined),
    ("legacy TRACE marker removed", "[TRACE]" not in combined),
    ("MySQL error prose removed", "mysql_error(" not in combined),
    ("query preview labels removed", not re.search(r'(?:query|sql)(?:=|:| prefix)[^\n"]*%s', combined, re.IGNORECASE)),
    ("pointer formatting removed", not re.search(r'%(?:\d+\$)?p', combined)),
    ("item-description trace removed", not re.search(r'(?:short_)?descr(?:iption)?[^\n]{0,80}(?:trace|logit|fprintf)', combined, re.IGNORECASE)),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit("failed persistence log hygiene checks: " + ", ".join(failed))

print("persistence log hygiene checks passed")
