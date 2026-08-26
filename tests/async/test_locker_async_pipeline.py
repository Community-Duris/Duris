#!/usr/bin/env python3
"""Source-contract checks for the async locker snapshot/pulsed pipeline."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

files = {
    "async_c": (SRC / "locker_async.c").read_text(encoding="utf-8", errors="replace"),
    "async_h": (SRC / "locker_async.h").read_text(encoding="utf-8", errors="replace"),
    "lockers": (SRC / "storage_lockers.c").read_text(encoding="utf-8", errors="replace"),
    "interp": (SRC / "interp.c").read_text(encoding="utf-8", errors="replace"),
    "comm": (SRC / "comm.c").read_text(encoding="utf-8", errors="replace"),
    "makefile": (SRC / "Makefile").read_text(encoding="utf-8", errors="replace"),
}


def check(name, cond, detail=""):
    if cond:
        print(f"  PASS  {name}")
        return True
    print(f"  FAIL  {name} {detail}")
    return False


def main():
    ok = True
    print("locker async pipeline checks")

    ok &= check("header mark_dirty returns int",
                "int locker_async_mark_dirty" in files["async_h"])
    ok &= check("pulse budget is 1",
                "LOCKER_ASYNC_SNAPSHOTS_PER_PULSE 1" in files["async_h"])
    ok &= check("max inflight is 1",
                "LOCKER_ASYNC_MAX_INFLIGHT        1" in files["async_h"]
                or "LOCKER_ASYNC_MAX_INFLIGHT 1" in files["async_h"])
    ok &= check("obj lock only while DIRTY",
                "state == LCHK_DIRTY && g_slots[i].user_pid == pid" in files["async_c"])
    ok &= check("terminal priority selection",
                "oldest_terminal" in files["async_c"] and "start_one_snapshot(oldest_terminal)" in files["async_c"])
    ok &= check("worker uses pool connection",
                "sql_persistence_connection" in files["async_c"])
    ok &= check("worker never uses live P_obj walk for apply",
                "apply_sql_script" in files["async_c"]
                and "sql_observed_execute_at" in files["async_c"]
                and "mysql_real_query" not in files["async_c"])
    ok &= check("snapshot builds DELETE public then inserts",
                "DELETE FROM locker_items WHERE locker_id=" in files["async_c"]
                and "INSERT INTO locker_items" in files["async_c"])
    ok &= check("leave marks terminal dirty",
                'locker_async_mark_dirty(chLocker, ch, 1, "leave-terminal")' in files["lockers"])
    ok &= check("save_locker_char marks nonterminal dirty",
                'locker_async_mark_dirty(chLocker, ch, 0, "save_locker_char-nonterminal")' in files["lockers"])
    ok &= check("re-entry counts async busy",
                "locker_async_name_busy" in files["lockers"])
    ok &= check("interp blocks obj cmds while locked",
                "locker_async_player_obj_locked" in files["interp"]
                and "Your belongings are being secured for storage" in files["interp"])
    ok &= check("boot starts locker worker",
                "locker_async_init" in files["comm"])
    ok &= check("pulse wired into game loop",
                "locker_async_pulse" in files["comm"])
    ok &= check("shutdown drain",
                "locker_async_shutdown" in files["comm"] and "locker_async_drain" in files["comm"])
    ok &= check("makefile builds locker_async.o",
                "locker_async.o" in files["makefile"])
    ok &= check("restore view after seal",
                "locker_async_restore_snapshot_view" in files["async_c"]
                and "locker_async_restore_snapshot_view" in files["lockers"])
    ok &= check("prepare snapshot walk before seal",
                "locker_async_prepare_snapshot" in files["async_c"]
                and "locker_async_prepare_snapshot" in files["lockers"])
    ok &= check("inflight coalesce rebuild flag",
                "rebuild_objects" in files["async_c"])
    ok &= check("terminal extract requires durable_ok",
                "terminal_not_durable" in files["async_c"] and
                "refusing extract of locker char" in files["async_c"] and
                "if (durable_ok && chLocker)" in files["async_c"])
    ok &= check("ambiguity prefers non-descriptor locker char",
                "name lookup ambiguous" in files["async_c"] and
                "!ch->desc" in files["async_c"])

    if ok:
        print("All locker async pipeline checks passed.")
        return 0
    print("locker async pipeline checks FAILED")
    return 1


if __name__ == "__main__":
    sys.exit(main())
