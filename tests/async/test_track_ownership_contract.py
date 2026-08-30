#!/usr/bin/env python3
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "src" / "map.h").read_text()
track = (ROOT / "src" / "track.c").read_text()
mapping = (ROOT / "src" / "map.c").read_text()

assert "#define TRACK_OWNER_PID_VALUE 1" in header
assert "#define IS_OWN_TRACK(ch, track)" in header
assert "track->value[TRACK_OWNER_PID_VALUE] = GET_PID(ch);" in track
assert track.count("!IS_OWN_TRACK(ch, obj)") >= 2
assert "(OBJ_VNUM(obj) == VNUM_TRACKS) && !IS_OWN_TRACK(ch, obj)" in mapping

print("[PASS] player tracks retain ownership and self-owned tracks stay hidden")
