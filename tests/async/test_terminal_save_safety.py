#!/usr/bin/env python3
"""Fail-closed source contracts for destructive character save transitions."""

from pathlib import Path

root = Path(__file__).resolve().parents[2]
read = lambda name: (root / "src" / name).read_text()

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

# Ghost extraction lives in actwiz.c and uses the shared terminal helper twice.
actwiz = read("actwiz.c")
checks["ghost extraction gate"] = actwiz.count(
    "persistence_save_character_terminal(vict, RENT_LINKDEAD)"
) == 2

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())
print("terminal save safety checks passed")
