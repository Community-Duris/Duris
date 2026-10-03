"""Structural training-dummy guards runnable by the standalone regression gate."""

from pathlib import Path
import unittest
from _paths import extract_function
from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]


def source(relative: str) -> str:
    return (ROOT / relative).read_text()


def test_training_dummy_is_registered_as_a_player_command():
    interp = source("src/cmd/interp.c")
    header = source("src/cmd/interp.h")

    assert '#define CMD_DUMMY 864' in header
    assert '"dummy",' in interp
    assert 'CMD_Y(CMD_DUMMY, STAT_DEAD + POS_PRONE, do_training_dummy, 0, FALSE);' in interp


def test_training_dummy_has_fixed_and_custom_profiles():
    dummy = source("src/combat/training_dummy.c")

    assert 'avail_hometowns[home][race] != 1 || !creation_race_enabled(race)' in dummy
    assert 'creation_class_align(race, class_index) == 5' in dummy
    assert 'candidate = guild_locations[home][0];' in dummy
    assert 'training_dummy_create(room, home, 56, race,' in dummy
    assert 'training_dummy_default_race_for_room(ch->in_room)' in dummy
    assert 'No player-creation race starts in this room; specify a race explicitly.' in dummy
    assert 'dummy spawn [level] [race] [class] [min|mid|max]' in dummy
    assert 'training_dummy_gear_ac' in dummy
    assert 'training_dummy_gear_save' in dummy
    assert 'dummy->only.npc->training_dummy_fixed = fixed;' in dummy


def test_dummy_appearance_tracks_its_profile():
    dummy = source("src/combat/training_dummy.c")

    assert 'training_dummy_refresh_description(dummy);' in dummy
    assert 'race_names_table[race].ansi' in dummy
    assert 'class_names_table[class_index].ansi' in dummy
    assert 'GET_LEVEL(dummy)' in dummy
    assert 'TRAINING_DUMMY_GEAR_MIN:' in dummy
    assert 'TRAINING_DUMMY_GEAR_MID' in dummy
    assert 'TRAINING_DUMMY_GEAR_MAX:' in dummy
    assert 'dummy->player.size = race_size(race);' in dummy


def test_spawn_rooms_keep_the_dummy_without_triggering_justice_or_unsafe_combat():
    handler = source("src/world/handler.c")
    justice = source("src/combat/justice.c")
    fight_move = source("src/classes/new_skills.c")
    fight = source("src/combat/fight.c")
    spells = source("src/net/sparser.c")

    assert 'if (!training_dummy_is(ch) && IS_INVADER(ch))' in handler
    assert '(GET_MASTER(ch) == NULL) && !training_dummy_is(ch)' in handler
    assert 'if (training_dummy_is(ch))\n\t\treturn;' in justice
    assert 'CHAR_IN_SAFE_ROOM(ch) && !training_dummy_is(victim)' in fight_move
    assert 'CHAR_IN_SAFE_ROOM(ch) && !training_dummy_is(victim)' in fight
    assert 'safe_room_spell_target_allowed(ch, spl, target_data->t_char)' in spells
    assert 'safe_room_spell_target_allowed(ch, arg->spell, tar_char)' in spells


def test_nonpet_damage_and_reflective_shields_cannot_turn_dummy_into_a_tank():
    fight = source("src/combat/fight.c")
    fight_move = source("src/classes/new_skills.c")
    fighting = extract_function("fight_state.c", "void set_fighting(")

    assert fight.count('training_dummy_is(victim) && !training_dummy_target_allowed(ch, victim)') >= 3
    assert fight.count('if (training_dummy_is(ch))\n\t\treturn DAM_NONEDEAD;') >= 3
    assert 'if (!training_dummy_target_allowed(ch, victim))' in fight_move
    assert 'training_dummy_retarget_nonpet(ch, victim);' in fight_move
    assert 'if (training_dummy_is(victim))\n\t\treturn DAM_NONEDEAD;' in fight
    assert contains(fighting, 'if (training_dummy_is(ch)) return;')


def test_training_dummy_is_non_hostile_and_records_damage_without_hp_loss():
    fight = source("src/combat/fight.c")
    retaliation = extract_function("fight_state.c", "int attack_back(")

    assert contains(retaliation, 'if (training_dummy_is(ch) || training_dummy_is(victim))')
    assert contains(retaliation, 'return DAM_NONEDEAD;')
    assert 'if (!training_dummy_is(victim))\n\t\tremember(victim, ch);' in fight
    assert 'training_dummy_record_damage(victim, recorded_damage);' in fight
    assert 'return DAM_NONEDEAD;' in fight


def test_training_dummy_has_no_item_or_currency_state():
    dummy = source("src/combat/training_dummy.c")
    handler = source("src/world/handler.c")
    command = source("src/cmd/actobj.c")

    assert 'for (int slot = 0; slot < MAX_WEAR; ++slot)' in dummy
    assert 'for (P_obj object = dummy->carrying; object;)' in dummy
    assert 'GET_PLATINUM(dummy) = 0;' in dummy
    assert 'GET_GOLD(dummy) = 0;' in dummy
    assert 'GET_SILVER(dummy) = 0;' in dummy
    assert 'GET_COPPER(dummy) = 0;' in dummy
    assert 'P_char dummy_owner = training_dummy_item_owner(obj_to);' in handler
    assert 'if (training_dummy_is(vict))' in command
    assert 'The training dummy refuses every item.' in command
    assert 'The training dummy refuses coins and all other offerings.' in command


def test_training_dummy_is_anchored_and_has_no_social_links():
    dummy = source("src/combat/training_dummy.c")
    handler = source("src/world/handler.c")
    follow = source("src/net/sparser.c")
    movement = source("src/cmd/actmove.c")
    group = source("src/guild/group.c")

    assert 'bool training_dummy_can_enter_room(P_char ch)' in dummy
    assert 'bool training_dummy_can_leave_room(P_char ch)' in dummy
    assert 'training_dummy_begin_removal(ch);' in handler
    assert 'if (training_dummy_is(ch) || training_dummy_is(leader))' in follow
    assert 'The training dummy cannot follow or be followed.' in movement
    assert 'The training dummy is anchored and cannot be dragged.' in movement
    assert 'if (training_dummy_is(victim))' in group
    assert 'if (training_dummy_is(leader) || training_dummy_is(member))' in group
    assert 'The training dummy cannot join or lead a group.' in group


def test_training_dummy_cannot_be_charmed_or_converted_to_a_pet():
    bard = source("src/classes/bard.c")
    necromancy = source("src/classes/necromancy.c")
    charm = extract_function("spell_charm.c", "void charm_generic(")
    command_undead = extract_function("spell_status_control.c", "void spell_command_undead(")
    psionics = source("src/classes/psionics.c")
    dummy = source("src/combat/training_dummy.c")

    assert 'if (training_dummy_is(victim))' in bard
    assert 'The training dummy has no mind to charm.' in bard
    assert '!training_dummy_capture_target_allowed(mob)' in necromancy
    assert '!training_dummy_capture_target_allowed(ch)' in necromancy
    assert contains(charm, 'if (training_dummy_is(victim))')
    assert 'The training dummy has no mind to charm.' in charm
    assert contains(command_undead, 'if (training_dummy_is(victim))')
    assert 'The training dummy cannot be commanded.' in command_undead
    assert 'The training dummy has no mind to awe.' in psionics
    assert 'The training dummy has no mind to dominate.' in psionics
    assert 'cannot be charmed' in dummy


def test_training_dummy_cannot_be_summoned_or_ridden():
    summonable = extract_function("spell_conjuration.c", "int Summonable(")
    mount = source("src/classes/mount.c")

    assert contains(summonable, 'if (training_dummy_is(ch)) return FALSE;')
    assert 'The training dummy is anchored and cannot be ridden.' in mount


def test_nonpet_npcs_cannot_use_the_dummy_as_a_tank():
    utility = source("src/core/utility.c")
    mob_combat = source("src/mob/mobact.c")
    fight = source("src/combat/fight.c")
    header = source("src/combat/training_dummy.h")

    assert 'bool training_dummy_target_allowed(P_char attacker, P_char victim);' in header
    assert 'if (IS_NPC(ch) && !IS_PC_PET(ch) && training_dummy_is(target))' in utility
    assert 'training_dummy_retarget_nonpet(ch, GET_OPPONENT(ch));' in mob_combat
    assert 'training_dummy_retarget_nonpet(ch, vict);' in mob_combat
    assert 'training_dummy_note_attacker(victim, ch);' in fight
    assert 'training_dummy_retarget_nonpet(ch, victim);' in fight


def test_npc_spellups_skip_the_dummy_without_blocking_explicit_affects():
    dummy = source("src/combat/training_dummy.c")
    header = source("src/combat/training_dummy.h")
    mobact = source("src/mob/mobact.c")
    explicit_shields = '\n'.join([
        extract_function("spell_globes.c", "void spell_globe("),
        extract_function("spell_elemental_shields.c", "void spell_fireshield("),
    ])

    assert 'bool training_dummy_spellup_target_allowed(P_char caster, P_char target);' in header
    assert 'return !caster || !IS_NPC(caster);' in dummy
    assert 'if (!training_dummy_spellup_target_allowed(ch, candidate))' in mobact
    assert 'void spell_globe' in explicit_shields
    assert 'void spell_fireshield' in explicit_shields
    assert 'training_dummy_spellup_target_allowed' not in explicit_shields
    assert 'if (training_dummy_is(ch))' in mobact
    assert 'periodic mundane event' in mobact


def test_dummy_cannot_be_used_as_a_shape_clone_disguise_or_capture_source():
    dummy = source("src/combat/training_dummy.c")
    header = source("src/combat/training_dummy.h")
    shapechange = source("src/cmd/actnew.c")
    clone = source("src/cmd/actwiz.c")
    clone_spell = source("src/classes/sillusionist.c")
    magic = source("src/magic/magic.c")
    conjuration = source("src/magic/spell_conjuration.c")
    disguise = source("src/classes/disguise.c")
    capture = source("src/classes/new_skills.c")
    pets = source("src/classes/necromancy.c")
    pet_restore = source("src/player/pet_restore_runtime.c")

    assert 'bool training_dummy_shape_target_allowed(P_char target);' in header
    assert 'bool training_dummy_clone_target_allowed(P_char target);' in header
    assert 'bool training_dummy_disguise_target_allowed(P_char target);' in header
    assert 'bool training_dummy_capture_target_allowed(P_char target);' in header
    assert 'The training dummy cannot be used as a shapechange form.' in shapechange
    assert 'if (!training_dummy_clone_target_allowed(mob))' in clone
    assert 'if (!training_dummy_clone_target_allowed(target))' in clone_spell
    assert 'The training dummy cannot be used as a clone form.' in clone_spell
    assert 'P_char make_mirror(P_char ch)\n{\n\tif (training_dummy_is(ch))' in conjuration
    assert 'if (!training_dummy_disguise_target_allowed(target))' in disguise
    assert 'The training dummy cannot be captured.' in capture
    assert '!training_dummy_capture_target_allowed(mob)' in pets
    assert 'training_dummy_capture_target_allowed(pet)' in pet_restore
    assert 'return !training_dummy_is(target);' in dummy


def test_bootstrap_runs_after_world_continents_are_ready():
    db = source("src/world/db.c")

    assert 'assign_continents();\n\ttraining_dummy_bootstrap();' in db


def test_dummy_snapshots_are_skipped_and_recreated_by_bootstrap():
    copyover = source("src/persistence/copyover.c")
    recovery = source("src/world/world_recovery_pipeline.c")

    assert 'copyover_training_dummy_is(ch)' in copyover
    assert '(IS_NPC(mob) && mob->only.npc && mob->only.npc->training_dummy)' in copyover
    assert 'recovery_training_dummy_is(ch)' in recovery
    assert 'ch->only.npc->summoned_instance || recovery_training_dummy_is(ch)' in recovery
    assert 'recovery_training_dummy_is(mob)' in recovery


def load_tests(_loader, _tests, _pattern):
    return unittest.TestSuite(
        unittest.FunctionTestCase(case)
        for name, case in globals().items()
        if name.startswith("test_") and callable(case)
    )


if __name__ == "__main__":
    unittest.main()
