#!/usr/bin/env python3
"""Fail-closed copyover ordering and live-process recovery contracts."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
copyover = (root / "src/copyover.c").read_text()
comm = (root / "src/comm.c").read_text()

body = copyover[copyover.index("bool copyover_save("):copyover.index(
    "static P_char copyover_load_player", copyover.index("bool copyover_save(")
)]
save = body.index("persistence_save_character_terminal")
flush = body.index("persistence_flush_all_character_saves")
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
    "publication precedes descriptor close": publish < close,
    "publication precedes client fd mutation": publish < prepare_client,
    "publication precedes progress notice": publish < progress_notice,
    "descriptor close precedes exec": close < execute,
    "failure says server remains live": "server remains live" in body,
    "copyover runs inside live game loop": "copyover_save(s, S, WS)" in
                                           comm[comm.index("void game_loop("):],
    "workers stop only after game loop returns": comm.index("game_loop(port, sslport);") <
                                                 comm.index("persistence_stop_scalar_event_worker"),
    "failure resumes game loop": "goto resume_game_loop;" in comm,
    "no destructive restart fallback": "refusing fallback exit" in comm,
}

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())
print("copyover save guards passed")
