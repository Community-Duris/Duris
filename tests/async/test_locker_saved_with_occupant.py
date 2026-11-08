#!/usr/bin/env python3
"""A locker is saved with the character inside it.

writeCharacter() runs the locker's post-save hook, which saves the locker its occupant
is in and applies the locker's idle rule. Queued saves (the 30-second checkpoint, every
terminal save and writeCharacter()'s own queued branch) returned before it, so a locker
was saved only when its occupant walked out: what a player dropped in it was lost if
the session ended inside (shutdown, idle rent, link loss) or the server crashed. Each
queued save now runs the hook. The locker slot keeps the user by pid and the locker
by name, since a terminal save's caller extracts the user straight after.
"""
from pathlib import Path

SRC = Path(__file__).resolve().parents[2] / "src"


def body(text, signature):
    start = text.index(signature)
    return text[start:text.index("\n}\n", start) + 3]


files = (SRC / "core/files.c").read_text()
hook = body(files, "void locker_post_save_hook(P_char ch)")
assert "IS_ROOM(ch->in_room, ROOM_LOCKER)" in hook and "world[ch->in_room].funct" in hook
assert "(-81)" in hook
write = body(files, "int writeCharacter(P_char ch, int type, int room)")
queued = write[write.index("player_save_pipeline_is_nonterminal_type(type))"):]
assert queued.index("player_save_pipeline_request(") < queued.index("locker_post_save_hook(ch);") < \
    queued.index("return queued == player_save_pipeline_result::queued")
assert write.count("locker_post_save_hook(ch);") == 3
assert "(-81)" not in write
print("[PASS] writeCharacter() runs the locker post-save hook on each of its paths")

terminal = body((SRC / "cmd/actoth.c").read_text(), "static bool persistence_save_character_terminal_with_policy(")
assert terminal.index("player_save_pipeline_terminal(") < terminal.index("locker_post_save_hook(ch);")
checkpoint = (SRC / "persistence/persistence_checkpoint.c").read_text()
assert checkpoint.count("player_save_pipeline_checkpoint_dirty(") == \
    checkpoint.count("locker_post_save_hook(character);") == 2
print("[PASS] terminal saves and checkpoints save the locker a character is in")

locker = (SRC / "persistence/locker_async.c").read_text()
slot = locker[locker.index("struct locker_async_slot\n{"):locker.index("};", locker.index("struct locker_async_slot\n{"))]
assert "P_char" not in slot
start = body(locker, "static int start_one_snapshot(")
assert "chLocker = find_locker_char_by_name(s->locker_name);" in start
assert "chUser = find_char_by_pid(s->user_pid);" in start
print("[PASS] a locker slot holds no character pointer across pulses")
