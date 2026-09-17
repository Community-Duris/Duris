from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def source(relative: str) -> str:
    return (ROOT / relative).read_text()


def test_training_dummy_is_registered_as_a_player_command():
    interp = source("src/cmd/interp.c")
    header = source("src/cmd/interp.h")

    assert '#define CMD_DUMMY 863' in header
    assert '"dummy",' in interp
    assert 'CMD_Y(CMD_DUMMY, STAT_DEAD + POS_PRONE, do_training_dummy, 0, FALSE);' in interp


def test_training_dummy_has_fixed_and_custom_profiles():
    dummy = source("src/combat/training_dummy.c")

    assert 'training_dummy_create(room, home, 56, RACE_GREY, CLASS_CLERIC' in dummy
    assert 'dummy spawn [level] [race] [class] [min|mid|max]' in dummy
    assert 'training_dummy_gear_ac' in dummy
    assert 'training_dummy_gear_save' in dummy
    assert 'dummy->only.npc->training_dummy_fixed = fixed;' in dummy


def test_training_dummy_is_non_hostile_and_records_damage_without_hp_loss():
    fight = source("src/combat/fight.c")

    assert 'if (training_dummy_is(ch) || training_dummy_is(victim))' in fight
    assert 'if (!training_dummy_is(victim))\n\t\tremember(victim, ch);' in fight
    assert 'training_dummy_record_damage(victim, recorded_damage);' in fight
    assert 'return DAM_NONEDEAD;' in fight


def test_bootstrap_runs_after_world_continents_are_ready():
    db = source("src/world/db.c")

    assert 'assign_continents();\n\ttraining_dummy_bootstrap();' in db
