#!/usr/bin/env python3
"""Fail-closed copyover ordering and live-process recovery contracts."""

from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
copyover = (SRC / "copyover.c").read_text()
comm = (SRC / "comm.c").read_text()

body = copyover[copyover.index("bool copyover_save("):copyover.index(
    "static P_char copyover_load_player", copyover.index("bool copyover_save(")
)]
recover = copyover[copyover.index("static P_char copyover_load_player"):]
save = body.index("persistence_save_character_terminal")
flush = body.index("persistence_flush_all_character_saves")
drain = body.index("player_save_pipeline_drain")
publish = body.index("rename(copyover_tmp, COPYOVER_FILE)")
close = body.index("close(d->descriptor)")
prepare_client = body.index("copyover_prepare_socket(d->descriptor)")
progress_notice = body.index("*** Copyover in progress... ***")
execute = body.index("execl(")

checks = {
    "copyover returns failure": body.count("return false;") >= 10,
    "ships precede characters": body.index("drain_pending_ship_saves") < save,
    "lockers precede characters": body.index("locker_async_drain") < save,
    "connected saves precede remaining flush": save < flush,
    "all saves precede publication": flush < publish,
    "pipeline drain precedes publication": flush < drain < publish,
    "pipeline drain is bounded": "player_save_pipeline_drain(3000)" in body,
    "copyover abort reopens pipeline": "player_save_pipeline_resume();" in
                                       copyover[copyover.index("static void notify_copyover_failure"):
                                                copyover.index("static void raw_write_to_fd", copyover.index("static void notify_copyover_failure"))],
    "publication precedes descriptor close": publish < close,
    "publication precedes client fd mutation": publish < prepare_client,
    "publication precedes progress notice": publish < progress_notice,
    "descriptor close precedes exec": close < execute,
    "failure says server remains live": "server remains live" in body,
    "copyover runs inside live game loop": "copyover_save(s, S, WS)" in
                                           comm[comm.index("void game_loop("):],
    "typed workers stop only after game loop returns": "critical_command_coordinator_shutdown();" in
                                                       comm[comm.index("game_loop(port, sslport);"):],
    "legacy raw workers are not stopped at shutdown": "persistence_stop_scalar_event_worker();" not in
                                                       comm,
    "failure resumes game loop": "goto resume_game_loop;" in comm,
    "shutdown drain is fail closed": "!player_save_pipeline_drain(3000)" in comm and
                                      "pipeline_drain_failed" in comm,
    "no destructive restart fallback": "refusing fallback exit" in comm,
    "copyover reloads SQL player inventory": "request.include_items = true;" in recover and
                                               "request.include_pets = false;" in recover and
                                               "restoreItemsOnly(ch, 0)" not in recover,
    "copyover keeps materialized inventory attached": "reset_char(ch);" not in recover,
    "minimal copyover preserves its world dataset":
        'execl(DMS_RUNTIME_BINARY, "dms", "--minimal", "-C", exec_buf' in body,
}

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())
print("copyover save guards passed")
