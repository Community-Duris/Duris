#!/usr/bin/env python3
"""Fail-closed source contracts for destructive character save transitions."""

from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]
read = lambda name: (SRC / name).read_text()

files = read("files.c")
actoth = read("actoth.c")
affects = read("affects.c")
fight = read("fight.c")
rooms = read("specs.room.c")
limits = read("limits.c")
comm = read("comm.c")
copyover = read("copyover.c")
artifact = read("artifact.c")
lockers = read("storage_lockers.c")
locker_leave = lockers[
    lockers.index("static bool locker_handle_leave"):
    lockers.index("static int locker_handle_save_hook")
]
locker_missing_character = locker_leave[
    locker_leave.index("if (!chLocker)"):
    locker_leave.index("else if (!pLocker->LockerToPFile())")
]
locker_prepare_failure = locker_leave[
    locker_leave.index("else if (!pLocker->LockerToPFile())"):
    locker_leave.index("\n\telse\n", locker_leave.index("else if (!pLocker->LockerToPFile())"))
]

cleanup = files[files.index("// Failed saves always restore"):files.index("return result;", files.index("// Failed saves always restore"))]
assert cleanup.index("if (!persistence_should_extract_terminal_inventory") < cleanup.index("equip_char") < cleanup.index("else")
assert cleanup.index("else") < cleanup.index("extract_obj")

checks = {
    "failed save restores equipment": "persistence_should_extract_terminal_inventory" in cleanup and "equip_char" in cleanup,
    "trusted quit gate": "if (!persistence_save_character_terminal(ch, RENT_INN))" in actoth,
    "camp gate": "if (!persistence_save_character_terminal(ch, RENT_CAMPED))" in affects,
    "death gate": "!persistence_save_character_terminal(ch, RENT_DEATH)" in fight,
    "inn and heaven gates": rooms.count("persistence_save_character_terminal") >= 3,
    "idle rent gate": "if (!persistence_save_character_terminal(i, RENT_LINKDEAD))" in limits,
    "link loss retains retry": "link-loss-retry" in comm and "terminal_save_failed" in comm,
    "ghost extraction gate": actoth.count("persistence_save_character_terminal(vict, RENT_LINKDEAD)") == 0,
    "copyover returns failure": "bool copyover_save(" in copyover and copyover.count("return false;") >= 10,
    "copyover saves before close": copyover.index("persistence_save_character_terminal") < copyover.index("close(d->descriptor)"),
    "shutdown resumes": "goto resume_game_loop;" in comm and "shutdown_cancelled=1" in comm,
    "artifact dummy retention": artifact.count("extract_refused=1") >= 2,
    "legacy locker retention": "terminal_not_durable" in lockers and
                               lockers.index("terminal_not_durable", lockers.index("event_deferredTerminalSave")) <
                               lockers.index("return;", lockers.index("terminal_not_durable", lockers.index("event_deferredTerminalSave"))),
    "locker prepare failure vetoes leave": "return false;" in locker_prepare_failure and
                                            "PFileToLocker" not in locker_prepare_failure and
                                            "writeCharacter" not in locker_prepare_failure and
                                            "extract_char" not in locker_prepare_failure and
                                            "\n\tfree_locker(" not in locker_prepare_failure,
    "missing locker character vetoes leave": "return false;" in locker_missing_character,
}

terminal_helper = actoth[
    actoth.index("bool persistence_save_character_terminal"):
    actoth.index("bool persistence_save_all_characters_terminal")
]
checks["terminal helper uses typed coordinator outcome"] = all(
    token in terminal_helper
    for token in (
        "player_save_pipeline_terminal",
        "database_acknowledged",
        "journal_durable",
        "terminal-save-retry",
    )
) and "do_save_silent" not in terminal_helper and "writeCharacter" not in terminal_helper

player_sql_start = files.index("if (!sql_save_player(ch, type, room))")
player_sql_failure = files[
    player_sql_start:files.index("// Failed saves always restore", player_sql_start)
]
checks["player SQL failure flat fallback writes retired"] = (
    "flat_fallback_retired" in player_sql_failure
    and "persistence_write_character_flat_fallback" not in player_sql_failure
)

flat_terminal_start = files.index("#ifdef __NO_MYSQL__", files.index("int writeCharacter"))
flat_terminal = files[
    flat_terminal_start:files.index("#endif", flat_terminal_start)
]
checks["flat terminal saves require the typed durable outcome"] = all(
    token in flat_terminal
    for token in (
        "player_save_pipeline_terminal",
        "player_save_terminal_result::database_acknowledged",
        "return 0;",
    )
)

# Ghost extraction lives in actwiz.c and uses the shared terminal helper twice.
actwiz = read("actwiz.c")
checks["ghost extraction gate"] = actwiz.count(
    "persistence_save_character_terminal(vict, RENT_LINKDEAD)"
) == 2

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())
print("terminal save safety checks passed")
