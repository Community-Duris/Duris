#!/usr/bin/env python3
"""Boot flags the locker rooms by vnum, so a character saved in one is moved out on login.

The check compared the real room index with the locker vnums 65201-65300. With about
250,000 rooms that flagged 100 unrelated rooms (#107747-#107846) as lockers, with the
locker room proc, and left the real locker rooms unflagged, so the login redirect out of
a locker room never fired and a character saved inside one came back stranded in an
empty room with no exits. The proc is not set at boot: finding a free locker room looks
for a room without it.
"""
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"
boot = (SRC / "world/db.c").read_text()
boot = boot[boot.index("void boot_world(int mini_mode)"):]
boot = boot[:boot.index("\n}\n")]
assert "world[room_nr].number >= 65201 && world[room_nr].number <= 65300" in boot
assert "room_flags |= ROOM_LOCKER;" in boot
assert "(room_nr >= 65201)" not in boot and "storage_locker" not in boot
print("[PASS] boot flags the locker rooms by vnum and leaves their proc unset")

lockers = (SRC / "item/storage_lockers.c").read_text()
assert "world[realNum = real_room0(roomNum)].funct == storage_locker" in lockers
assert "world[realNum].funct = storage_locker;" in lockers
print("[PASS] a locker room is taken while it has the locker proc, set when it is allocated")

materialize = (SRC / "player/player_load_materialize.c").read_text()
redirect = materialize[materialize.index("if (locker_room != NOWHERE && IS_ROOM(locker_room, ROOM_LOCKER))"):]
redirect = redirect[:redirect.index("GET_COPPER(ch)")]
assert "location=locker outcome=redirected" in redirect
# It comes back outside: the load leaves it in no room and enter_game() starts from
# was_in_room.
assert "GET_BIRTHPLACE(ch)" in redirect
assert "ch->specials.was_in_room = world[exit_room].number;" in redirect
assert "ch->in_room" not in redirect
print("[PASS] a character loaded into a locker room is moved out of it")
