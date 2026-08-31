#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
import re
import sys
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
actobj = (SRC / "actobj.c").read_text()
checks = []

fn_match = re.search(r"static bool do_get_try_container_item\(.*?\n\}", actobj, re.S)
if not fn_match:
    checks.append(("do_get_try_container_item function found", False))
    fn = ""
else:
    fn = fn_match.group(0)
    checks.append(("container get helper can defer carry-limit messages", contains(fn, "bool        report_carry_limit")))
    checks.append(("container get helper enforces item-count cap without material exemption", contains(fn, "if (IS_CARRYING_N(ch) < CAN_CARRY_N(ch))") and not contains(fn, "LOWEST_MAT_VNUM") and not contains(fn, "HIGHEST_MAT_VNUM")))
    checks.append(("container get helper stops bulk on item-count cap", re.search(r"carry_n=.*?stop_bulk = TRUE;", fn, re.S) is not None))
    checks.append(("container get helper only prints carry-limit when requested", contains(fn, "if (report_carry_limit)") and fn.count("send_to_char(\"You can't carry any more.") == 2))

bulk_call = re.search(r"do_get_try_container_item\(\s*ch,.*?GETDBG\[get-container-post\].*?\);", actobj, re.S)
checks.append(("bulk get from container defers carry-limit report", bulk_call is not None and contains(bulk_call.group(0), "FALSE")))

single_call = re.search(r"do_get_try_container_item\(\s*ch,.*?GETDBG\[get-container-single-post\].*?\);", actobj, re.S)
checks.append(("single get from container reports carry-limit immediately", single_call is not None and contains(single_call.group(0), "TRUE")))

bulk_summary = re.search(r"if \(total > 1\).*?You got %d items.*?if \(stop_container_bulk\).*?do_get_reject_carry_limit\(ch, fail\);", actobj, re.S)
checks.append(("bulk get reports item total before final carry-limit", bulk_summary is not None))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll actobj get-limit checks passed.")
