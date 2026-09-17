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
    magic = source("src/magic/magic.c")
    dummy = source("src/combat/training_dummy.c")

    assert 'if (training_dummy_is(victim))' in bard
    assert 'The training dummy has no mind to charm.' in bard
    assert '!training_dummy_capture_target_allowed(mob)' in necromancy
    assert '!training_dummy_capture_target_allowed(ch)' in necromancy
    assert 'The training dummy has no mind to charm.' in magic
    assert 'cannot be charmed' in dummy


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
    magic = source("src/magic/magic.c")

    assert 'bool training_dummy_spellup_target_allowed(P_char caster, P_char target);' in header
    assert 'return !caster || !IS_NPC(caster);' in dummy
    assert 'if (!training_dummy_spellup_target_allowed(ch, candidate))' in mobact
    assert 'void spell_globe' in magic
    assert 'void spell_fireshield' in magic
    assert 'training_dummy_spellup_target_allowed' not in magic


def test_dummy_cannot_be_used_as_a_shape_clone_disguise_or_capture_source():
    dummy = source("src/combat/training_dummy.c")
    header = source("src/combat/training_dummy.h")
    shapechange = source("src/cmd/actnew.c")
    clone = source("src/cmd/actwiz.c")
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
    assert 'if (!training_dummy_disguise_target_allowed(target))' in disguise
    assert 'The training dummy cannot be captured.' in capture
    assert '!training_dummy_capture_target_allowed(mob)' in pets
    assert 'training_dummy_capture_target_allowed(pet)' in pet_restore
    assert 'return !training_dummy_is(target);' in dummy


def test_bootstrap_runs_after_world_continents_are_ready():
    db = source("src/world/db.c")

    assert 'assign_continents();\n\ttraining_dummy_bootstrap();' in db
