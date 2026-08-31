#!/usr/bin/env python3
from _paths import rel
from pathlib import Path
import sys

checks = [
    (
        rel("actoth.c"),
        'persistence_schedule_character_save(ch, 1, 2, "autosave")',
        'persistence_schedule_character_save(ch, 1, 2, "autosave")',
        1,
    ),
    (
        rel("comm.c"),
        'shutdown_cancelled=1',
        'if (!_pwipe && !persistence_save_all_characters_terminal(RENT_CRASH))',
        1,
    ),
    (
        rel("actnew.c"),
        'Failed to save %s after room move.',
        'if (!do_save_silent(ch, 1))',
        2,
    ),
    (
        rel("tradeskill.c"),
        'Failed to save %s after tradeskill change.',
        'if (!do_save_silent(pl, 1))',
        5,
    ),
    (
        rel("epic_skills.c"),
        'Failed to save %s after epic skill purchase.',
        'if (!do_save_silent(pl, 1))',
        1,
    ),
    (
        rel("nexus_stones.c"),
        'Failed to save %s after nexus sage training.',
        'if (!do_save_silent(pl, 1))',
        1,
    ),
    (
        rel("magic.c"),
        'Failed to save %s after soulbind.',
        'if (!do_save_silent(victim, 1))',
        1,
    ),
    (
        rel("actoth.c"),
        'Failed to save %s after new character setup.',
        'if (!do_save_silent(ch, 1))',
        1,
    ),
]

root = Path(__file__).resolve().parents[2]
ok = True
for rel, msg, guard, min_count in checks:
    text = root.joinpath(rel).read_text()
    g = text.count(guard)
    m = text.count(msg)
    print(f'{rel}: guard={g} msg={m}')
    if g < min_count or m < 1:
        print(f'missing save guard in {rel}')
        ok = False

sys.exit(0 if ok else 1)
