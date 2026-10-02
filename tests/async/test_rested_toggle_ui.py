#!/usr/bin/env python3
"""Focused account/WebSocket presentation checks for the rested feature gate."""

from __future__ import annotations

import json
import os
import re
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, SRC
from _source_contract import function_body


ACCOUNT = (SRC / "account" / "account.c").read_text(encoding="utf-8", errors="replace")
WS = (SRC / "net" / "ws_handlers.c").read_text(encoding="utf-8", errors="replace")
RESTED_HEADER = SRC / "world" / "rested.h"
HARNESS = Path(__file__).with_name("rested_toggle_ui_harness.cpp")

assert RESTED_HEADER.exists(), "parent-provided world/rested.h is required"
rested_header = RESTED_HEADER.read_text(encoding="utf-8", errors="replace")
assert '"exp.rested.enabled"' in rested_header
assert re.search(r'get_property\(\s*"exp\.rested\.enabled"\s*,\s*1\s*\)', rested_header)

for source in (ACCOUNT, WS):
    assert '#include "world/rested.h"' in source

menu = function_body(ACCOUNT, r"\bvoid\s+display_account_menu\s*\(")
load_display = function_body(ACCOUNT, r"\bint\s+load_char_display_data\s*\(")
check_rested = function_body(ACCOUNT, r"\bvoid\s+check_rested_bonus\s*\(")
rested_ws = function_body(WS, r"\bvoid\s+ws_cmd_rested_bonus\s*\(")
capabilities = function_body(WS, r"\bstatic\s+void\s+ws_add_account_capabilities\s*\(")
assert menu and load_display and check_rested and rested_ws and capabilities

# Telnet keeps the numeric slot stable but hides the feature when disabled. A
# direct typed 8 remains a compatibility path with an explicit explanation.
assert "selection > 8" in menu
assert "case 8:" in menu
assert "if (rested_bonus_enabled())" in menu
assert menu.index("if (rested_bonus_enabled())") < menu.index(
    '"&+C8) Check rested bonus&n\\r\\n"'
)
case8 = menu[menu.index("case 8:") : menu.index("default:", menu.index("case 8:"))]
assert "if (!rested_bonus_enabled())" in case8
assert "currently disabled" in case8
assert "display_account_menu(d, NULL)" in case8

# Loading a character for the ordinary account display never calculates a
# rested status while the switch is off; enabled behavior remains in the gate.
assert "info->rested_status = NULL;" in load_display
assert load_display.index("info->rested_status = NULL;") < load_display.index(
    "if (rested_bonus_enabled())"
)
rested_calculation = load_display[load_display.index("if (rested_bonus_enabled())") :]
assert "offline_seconds" in rested_calculation
assert "time(0)" in rested_calculation
assert "offline_seconds" not in load_display[: load_display.index("if (rested_bonus_enabled())")]

# The account command also fails closed before walking character saves.
assert check_rested.index("if (!rested_bonus_enabled())") < check_rested.index(
    "load_char_display_data"
)
assert "currently disabled" in check_rested
assert "display_account_menu(d, NULL)" in check_rested

# The authenticated WebSocket endpoint keeps its old character fields when
# enabled, adds an explicit flag, and avoids restore/time work when disabled.
assert rested_ws.index("if (!d->account)") < rested_ws.index("rested_bonus_enabled()")
assert 'cJSON_AddBoolToObject(result_data, "enabled", enabled)' in rested_ws
assert "if (enabled)" in rested_ws
assert rested_ws.index("if (enabled)") < rested_ws.index("restoreCharOnly")
assert rested_ws.index("if (enabled)") < rested_ws.index("time(0)")
assert 'cJSON_AddItemToObject(result_data, "characters", characters)' in rested_ws
assert 'ws_send_account_message(d, "rested_bonus", result_data, NULL)' in rested_ws

# The account-menu payload advertises the same capability so a separate web
# client can hide its control without probing the feature endpoint first.
assert 'cJSON_AddBoolToObject(capabilities, "restedBonus", rested_bonus_enabled())' in capabilities
auth_success = WS[WS.index("void ws_send_auth_success") : WS.index("void ws_send_reconnect_success")]
return_to_menu = WS[WS.index("void ws_send_return_to_menu") : WS.index("/* polls */")]
assert "ws_add_account_capabilities(data);" in auth_success
assert "ws_add_account_capabilities(data_obj);" in return_to_menu


def compile_harness(binary: Path) -> None:
    command = [
        os.environ.get("CXX", "g++"),
        "-std=c++20",
        "-O0",
        "-ffunction-sections",
        "-fdata-sections",
        f"-I{SRC}",
        "-I/usr/include/mysql",
        str(HARNESS),
        str(SRC / "net" / "ws_handlers.c"),
        "-Wl,--gc-sections",
        "-lcjson",
        "-lm",
        "-lcrypto",
        "-lssl",
        "-pthread",
        "-o",
        str(binary),
    ]
    subprocess.run(command, cwd=ROOT, check=True)


def run_harness(binary: Path, *args: str) -> tuple[dict, int, int]:
    result = subprocess.run(
        [str(binary), *args], cwd=ROOT, check=True, capture_output=True, text=True
    )
    lines = result.stdout.splitlines()
    assert len(lines) == 3, result.stdout
    payload = json.loads(lines[0])
    restore_calls = int(lines[1].split("=", 1)[1])
    property_reads = int(lines[2].split("=", 1)[1])
    return payload, restore_calls, property_reads


with tempfile.TemporaryDirectory(prefix="duris-rested-toggle-ui-") as temp_dir:
    binary = Path(temp_dir) / "rested-toggle-ui"
    compile_harness(binary)

    default_payload, default_restores, default_reads = run_harness(binary)
    assert default_payload["action"] == "rested_bonus"
    assert default_payload["data"]["enabled"] is True
    assert default_payload["data"]["characters"]
    assert default_payload["data"]["characters"][0]["restedPercent"] == 100
    assert default_restores == 1
    assert default_reads == 1

    disabled_payload, disabled_restores, disabled_reads = run_harness(binary, "0")
    assert disabled_payload["action"] == "rested_bonus"
    assert disabled_payload["data"] == {"enabled": False, "characters": []}
    assert disabled_restores == 0
    assert disabled_reads == 1

    unauth_payload, unauth_restores, unauth_reads = run_harness(binary, "unauth")
    assert unauth_payload == {
        "type": "account",
        "action": "error",
        "error": "Not logged in",
    }
    assert unauth_restores == 0
    assert unauth_reads == 0

print("rested account/WebSocket presentation toggle checks passed")
