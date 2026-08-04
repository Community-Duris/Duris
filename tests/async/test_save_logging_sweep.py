#!/usr/bin/env python3
from pathlib import Path
import sys

checks = [
    (
        'src/actoth.c',
        'persistence_schedule_character_save(ch, 1, 2, "autosave")',
        'persistence_schedule_character_save(ch, 1, 2, "autosave")',
        1,
    ),
    (
        'src/comm.c',
        'Failed to save %s during shutdown.',
        'if (!do_save_silent(point->character, 3))',
        1,
    ),
    (
        'src/actnew.c',
        'Failed to save %s after room move.',
        'if (!do_save_silent(ch, 1))',
        2,
    ),
    (
        'src/tradeskill.c',
        'Failed to save %s after tradeskill change.',
        'if (!do_save_silent(pl, 1))',
        10,
    ),
    (
        'src/epic_skills.c',
        'Failed to save %s after epic skill purchase.',
        'if (!do_save_silent(pl, 1))',
        1,
    ),
    (
        'src/nexus_stones.c',
        'Failed to save %s after nexus sage training.',
        'if (!do_save_silent(pl, 1))',
        1,
    ),
    (
        'src/skills.c',
        'Failed to save %s after racial skill grant.',
        'if (!do_save_silent(ch, 1))',
        10,
    ),
    (
        'src/magic.c',
        'Failed to save %s after soulbind.',
        'if (!do_save_silent(victim, 1))',
        1,
    ),
    (
        'src/actoth.c',
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
