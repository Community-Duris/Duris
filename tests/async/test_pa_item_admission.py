#!/usr/bin/env python3
"""Entry-boundary regressions for active item movement admission."""

from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile
import unittest

from _paths import extract_function
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]


class ItemAdmissionBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "src/item/item_movement_transaction.h").read_text(
            encoding="utf-8"
        )
        cls.refusal = extract_function(
            "item/item_movement_transaction.c",
            "bool refuse_active_item_submission(",
        )
        cls.single = extract_function(
            "item/item_movement_transaction.c",
            "bool item_movement_transaction_submit(",
        )
        cls.batch = extract_function(
            "item/item_movement_transaction.c",
            "bool item_movement_transaction_submit_batch(",
        )

    def test_compiled_single_boundary_does_not_treat_rejected_result_as_predicate(self):
        # Compile the actual helper and callsite: source-presence checks alone
        # missed a false rejection result used as a true/false refusal predicate.
        helper_call = self.single.index("refuse_active_item_submission(")
        start = self.single.rfind("\tif (", 0, helper_call)
        stop = self.single.index("\tconst item_owner_identity effective_from", helper_call)
        self.assertGreaterEqual(start, 0)
        guard = self.single[start:stop]
        reject_helper = extract_function(
            "item/item_movement_transaction.c", "bool reject_with("
        )
        reject_enum = re.search(
            r"enum class item_movement_reject\s*\{[^}]+\};", self.header
        )
        assert reject_enum is not None, "actual item rejection enum not found"
        program = "#include <cassert>\n" + reject_enum.group(0) + r"""
enum class item_transfer_reason { creation, other };
namespace economic_gameplay_authority {
bool enabled = false;
bool active() { return enabled; }
}
int continuations = 0;
""" + reject_helper + "\n" + self.refusal + r"""
bool actual_single_guard(bool retained_owner_identity, bool owner_matches_request,
                         bool sourced_creation, item_transfer_reason reason,
                         item_movement_reject *reject)
{
    const bool accounting_active = economic_gameplay_authority::active();
    const bool adopted = retained_owner_identity;
""" + guard + r"""
    ++continuations;
    return true;
}
int main()
{
    const auto unset = static_cast<item_movement_reject>(-1);
    for (int bits = 0; bits < 32; ++bits) {
        const bool active = bits & 1;
        const bool retained = bits & 2;
        const bool matches = bits & 4;
        const bool creation = bits & 8;
        const bool sourced = creation && !retained && (bits & 16);
        economic_gameplay_authority::enabled = active;
        continuations = 0;
        auto rejection = unset;
        const bool accepted = actual_single_guard(
            retained, matches, sourced, creation ? item_transfer_reason::creation
                                       : item_transfer_reason::other, &rejection);
        if (!active || ((retained && matches) || sourced)) {
            assert(accepted && continuations == 1 && rejection == unset);
            continue;
        }
        assert(!accepted && continuations == 0);
        const auto expected = retained && !matches
            ? item_movement_reject::owner_mismatch
            : !retained && !creation ? item_movement_reject::missing_owner_identity
                                    : item_movement_reject::active_accounting_unsupported;
        assert(rejection == expected);
        // Batch callers return the helper result directly; refusal must remain
        // false, rather than changing the helper to repair only the single path.
        rejection = unset;
        assert(!refuse_active_item_submission(true, retained, matches, creation, &rejection));
        assert(rejection == expected);
    }
}
"""
        with tempfile.TemporaryDirectory(prefix="duris-item-admission-") as temp:
            source = Path(temp) / "admission.cpp"
            binary = Path(temp) / "admission"
            source.write_text(program, encoding="utf-8")
            subprocess.run(
                shlex.split(os.environ.get("CXX", "g++")) + [
                    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    str(source), "-o", str(binary),
                ], check=True, timeout=60,
            )
            subprocess.run([str(binary)], check=True, timeout=10, cwd=temp)

    def test_central_refusal_preserves_inactive_and_separates_unsupported_from_missing(self):
        self.assertIn("if (!accounting_active)\n\t\treturn false;", self.refusal)
        self.assertIn("item_movement_reject::missing_owner_identity", self.refusal)
        self.assertIn("item_movement_reject::active_accounting_unsupported", self.refusal)
        self.assertIn("item_movement_reject::owner_mismatch", self.refusal)
        self.assertIn("if (!retained_owner_identity && !new_creation)", self.refusal)
        self.assertIn("active_accounting_unsupported", self.header)
        self.assertIn("missing_owner_identity", self.header)

    def test_single_submission_refuses_before_creation_fallback_or_command_submission(self):
        admission = self.single.index("refuse_active_item_submission(")
        self.assertLess(admission, self.single.index("const item_owner_identity effective_from"))
        self.assertLess(admission, self.single.index("capture_absent(root"))
        self.assertLess(admission, self.single.index("item_transfer_command_build"))
        self.assertLess(admission, self.single.index("critical_command_coordinator_submit"))
        self.assertIn("reason == item_transfer_reason::creation", self.single[:admission + 400])
        self.assertIn(".reason = adopted ? reason : item_transfer_reason::creation", self.single)

    def test_absent_noncreation_and_ambiguous_batch_owners_are_refused_at_entry(self):
        self.assertIn("missing_owner_identity", self.refusal)
        batch_admission = self.batch.index("if (economic_gameplay_authority::active())")
        self.assertLess(batch_admission, self.batch.index("item_transfer_command_build"))
        self.assertLess(batch_admission, self.batch.index("critical_command_coordinator_submit"))
        self.assertLess(batch_admission, self.batch.index("item_ownership_runtime_owner_revision"))
        self.assertIn("!item_ownership_runtime_lookup(root->obj_uid, &runtime)", self.batch)
        self.assertIn("!item_owner_identity_valid(runtime.owner)", self.batch)
        self.assertIn("item_owner_identity_equal(runtime.owner, from_owner)", self.batch)

    def test_new_creation_is_unsupported_not_missing_identity_and_destruction_is_not_creation(self):
        self.assertIn("reason == item_transfer_reason::creation", self.single)
        self.assertRegex(self.single, r"reason == item_transfer_reason::creation,\s*reject\)")
        self.assertIn("if (creation)", self.batch)
        self.assertIn("reason == item_transfer_reason::creation;", self.batch)
        self.assertTrue(contains(self.batch, "return refuse_active_item_submission(true, false, false, true, reject);"))
        # For a missing retained row, only explicit creation reaches the
        # unsupported-source-admission refusal; destruction is not issuance.

    def test_static_quest_refuses_legacy_gift_before_live_transfer(self):
        quester = extract_function("world/quest.c", "int quester(")
        gift = quester[quester.index("if (cmd == CMD_GIVE)"):]
        refusal = gift.index("if (economic_gameplay_authority::active())")
        self.assertLess(refusal, gift.index("do_give(pl, arg, -4)"))
        self.assertLess(refusal, gift.index("quest_completion(qcp, ch, pl)"))

    def test_generic_player_candidate_needs_source_before_active_admission(self):
        publication = extract_function("world/handler.c", "obj_to_char_result obj_to_char_checked(")
        candidate = publication.index("if (!has_authoritative_ownership && creation_candidate &&")
        grant = publication.index("item_creation_grant_submit_to_player(", candidate)
        self.assertIn("!economic_gameplay_authority::active()",
                      publication[candidate:grant])

    def test_staff_storage_refuses_unprojected_sql_mutations_before_allocation(self):
        storage = extract_function("cmd/actwiz.c", "void do_storage(")
        guard = storage.index("if (economic_gameplay_authority::active() &&")
        allocation = storage.index("read_object(")
        self.assertLess(guard, allocation)
        self.assertIn("persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY",
                      storage[guard:allocation])
        for subcommand in ("new", "delete", "remove"):
            self.assertIn(f'!strcmp(subcmd, "{subcommand}")', storage[guard:allocation])
        self.assertIn("return;", storage[guard:allocation])
        for mutation in ("writeSavedItem(", "obj_from_obj(", "extract_obj("):
            self.assertLess(guard, storage.index(mutation))

    def test_unidentified_delayed_grants_refuse_before_reservation_or_allocation(self):
        for symbol, allocation in (
            ("void event_summon_book(", "read_object(31"),
            ("void event_summon_totem(", "read_object(417"),
        ):
            event = extract_function("classes/new_skills.c", symbol)
            self.assertLess(event.index("economic_gameplay_authority::active()"),
                            event.index(allocation))
        for symbol in ("void do_summon_book(", "void do_summon_totem("):
            command = extract_function("classes/new_skills.c", symbol)
            self.assertLess(command.index("economic_gameplay_authority::active()"),
                            command.index("add_event("))

        wind = extract_function("classes/ethermancer.c", "void grant_wind_blade(")
        self.assertLess(wind.index("economic_gameplay_authority::active()"),
                        wind.index("read_object("))
        seed = extract_function("combat/chaos.c", "static void chaos_pouch_test_seed(")
        self.assertLess(seed.index("economic_gameplay_authority::active()"),
                        seed.index("read_object("))

    def test_reward_retirement_paths_refuse_unaccounted_active_epoch_mutation(self):
        for symbol, mutation in (
            ("static void purge_expired_grants(", "clear_saved_grant("),
            ("static void dismiss_player_grant(", "retire_saved_reward_instance("),
            ("static int remove_grants(", "sql_begin_transaction("),
        ):
            route = extract_function("account/account_reward.c", symbol)
            self.assertLess(route.index("economic_gameplay_authority::active()"),
                            route.index(mutation))
        corpse = extract_function("account/account_reward.c",
                                  "void account_bound_reward_prepare_player_corpse(")
        self.assertIn("!economic_gameplay_authority::active()", corpse)
        self.assertLess(corpse.index("!economic_gameplay_authority::active()"),
                        corpse.index("dissolve_reward_containers("))
        command = extract_function("account/account_reward.c", "void do_divineclaim(")
        remove = command[command.index('if (!strcasecmp(first, "remove"))'):]
        self.assertLess(remove.index("economic_gameplay_authority::active()"),
                        remove.index("grants_for_removal("))

    def test_zone_reset_refuses_every_item_opcode_before_allocation(self):
        helper = extract_function("world/db.c", "static bool reset_command_issues_item(")
        reset = extract_function("world/db.c", "void reset_zone(")
        gate = reset.index("reset_command_issues_item(ZCMD.command)")
        self.assertLess(gate, reset.index("switch (ZCMD.command)"))
        self.assertLess(gate, reset.index("read_object("))
        program = "#include <cassert>\n" + helper + r"""
int main()
{
    for (char command : "BCAOPGE")
        if (command)
            assert(reset_command_issues_item(command));
    for (char command : "SYMFRD!")
        if (command)
            assert(!reset_command_issues_item(command));
}
"""
        with tempfile.TemporaryDirectory(prefix="duris-reset-items-") as temp:
            source = Path(temp) / "reset_items.cpp"
            binary = Path(temp) / "reset_items"
            source.write_text(program, encoding="utf-8")
            subprocess.run(
                shlex.split(os.environ.get("CXX", "g++")) + [
                    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    str(source), "-o", str(binary),
                ], check=True, timeout=60,
            )
            subprocess.run([str(binary)], check=True, timeout=10, cwd=temp)

    def test_unsourced_npc_item_rewards_are_withheld_in_active_epoch(self):
        generated = extract_function("world/random.mob.c", "P_char create_random_mob(")
        reward = generated[generated.index("// give the mob 1-3 random items!"):]
        self.assertLess(
            reward.index("!economic_gameplay_authority::active()"),
            reward.index("create_random_eq_new("),
        )
        death = extract_function("combat/fight.c", "void die(")
        for call in (
            "thanksgiving_proc(ch)", "christmas_proc(ch)",
            "enhance_on_eligible_npc_death(ch, killer)",
            "read_object(VOBJ_DRAGON_SCALE", "read_object(400230",
            "random_recipe(killer, ch)", "check_random_drop(killer, ch, TRUE)",
        ):
            location = death.index(call)
            guard = death.rfind("economic_gameplay_authority::active()", 0, location)
            self.assertGreaterEqual(guard, 0, call)
            self.assertLess(location - guard, 800, call)

    def test_random_quest_and_staff_random_command_refuse_before_mutation(self):
        quest = extract_function("world/random.zone.c", "int random_quest_mob_proc(")
        branch = quest[quest.index("if (OBJ_VNUM(obj) == VOBJ_RANDOM_ARMOR") :]
        gate = branch.index("if (economic_gameplay_authority::active())")
        self.assertLess(gate, branch.index("gain_epic(pl, EPIC_RANDOM_ZONE"))
        self.assertLess(gate, branch.index("obj_from_char(obj)"))
        self.assertLess(gate, branch.index("create_stones()"))
        staff = extract_function("item/randobj.c", "void do_randobj(")
        self.assertLess(
            staff.index("if (economic_gameplay_authority::active())"),
            staff.index("create_material(ch, ch)"),
        )

    def test_npc_ship_treasure_refuses_before_any_item_allocation(self):
        loader = extract_function("ships/ship_npc.c", "P_obj load_treasure_chest(")
        self.assertLess(
            loader.index("if (economic_gameplay_authority::active())"),
            loader.index("read_object("),
        )
        spawn = extract_function("ships/ship_npc.c", "bool load_cyrics_revenge()")
        self.assertLess(
            spawn.index("if (economic_gameplay_authority::active())"),
            spawn.index("create_npc_ship("),
        )
        revenge = extract_function("ships/ship_npc.c", "bool load_cyrics_revenge_crew(")
        self.assertLess(
            revenge.index("if (economic_gameplay_authority::active())"),
            revenge.index("load_npc_ship_crew_member("),
        )
        self.assertLess(revenge.index("if (!chest)"), revenge.index("obj_to_obj(fragment, chest)"))

    def test_item_dropping_spells_refuse_before_live_custody_changes(self):
        remove = extract_function("magic/spell_item_enhancement.c", "void spell_remove_curse(")
        guard = remove.index("if (!obj && victim && economic_gameplay_authority::active())")
        self.assertLess(guard, remove.index("affect_from_char(victim, SPELL_CURSE)"))
        self.assertLess(guard, remove.index("unequip_char(victim, e_pos)"))
        self.assertLess(guard, remove.index("obj_from_char(t_obj)"))
        self.assertIn("victim->equipment[slot]", remove[guard:remove.index("if (obj)")])
        self.assertIn("victim->carrying", remove[guard:remove.index("if (obj)")])
        recall = extract_function("magic/spell_travel.c", "void spell_word_of_recall(")
        guard = recall.index("if (economic_gameplay_authority::active() &&")
        self.assertGreater(guard, recall.index("if ((loc_nr == NOWHERE)"))
        self.assertLess(guard, recall.index('act("&+W$n utters a single word and disappears.'))
        self.assertLess(guard, recall.index("unequip_char(victim, e_pos)"))
        self.assertLess(guard, recall.index("char_from_room(victim)"))
        self.assertIn("victim->equipment[slot]", recall[guard:recall.index("if (ch == victim)", guard)])

    def test_final_teleport_charge_refuses_before_durable_item_retirement(self):
        teleport = extract_function("magic/spell_travel.c", "bool check_item_teleport(")
        guard = teleport.index("obj->value[2] == 1 && economic_gameplay_authority::active()")
        self.assertIn("item_command_uses_durable_ownership(obj)", teleport[guard:])
        self.assertLess(guard, teleport.index("teleport_to(ch, to_room"))
        self.assertLess(guard, teleport.index("--obj->value[2]"))
        self.assertLess(guard, teleport.index("extract_obj(obj, TRUE)"))

    def test_beholder_disintegration_preserves_durable_equipment(self):
        beam = extract_function("magic/beh_magic.c", "void spell_beholder_disintegrate(")
        guard = beam.index("item_command_uses_durable_ownership(obj)")
        self.assertLess(beam.index("economic_gameplay_authority::active()"), guard)
        self.assertLess(guard, beam.index("obj->condition -= number(1, 10)"))
        self.assertLess(guard, beam.index("unequip_char_dale(obj)"))
        self.assertLess(guard, beam.index("extract_obj(obj)"))

    def test_unsourced_spell_room_objects_refuse_before_replacement_or_creation(self):
        moonstone = extract_function("magic/spell_portals.c", "void spell_moonstone(")
        moon_gate = moonstone.index("economic_gameplay_authority::active()")
        self.assertLess(moon_gate, moonstone.index("extract_obj(moonstone)"))
        self.assertLess(moon_gate, moonstone.index("read_object(real_object(419)"))
        bloodstone = extract_function("magic/blispells.c", "void spell_bloodstone(")
        blood_gate = bloodstone.index("economic_gameplay_authority::active()")
        self.assertLess(blood_gate, bloodstone.index("extract_obj(bloodstone"))
        self.assertLess(blood_gate, bloodstone.index("read_object(real_object(433)"))
        portal = extract_function("magic/spell_portals.c", "bool spell_general_portal(")
        self.assertLess(portal.index("economic_gameplay_authority::active()"),
                        portal.index("portal1 = read_object("))
        channel = extract_function("magic/spell_conjuration.c", "void cast_channel(")
        self.assertLess(channel.index("economic_gameplay_authority::active()"),
                        channel.index("t_obj = read_object(EVIL_AVATAR_OBJ"))
        continuing = extract_function("magic/spell_conjuration.c", "void spell_channel(")
        self.assertLess(continuing.index("economic_gameplay_authority::active()"),
                        continuing.index("obj->timer[0]++"))
        self.assertLess(continuing.index("economic_gameplay_authority::active()"),
                        continuing.index("extract_obj(obj)"))

    def test_nontransient_periodic_item_generation_refuses_before_allocation(self):
        route = extract_function("magic/affects.c", "void poo(")
        self.assertLess(route.index("economic_gameplay_authority::active()"),
                        route.index("read_object(51, VIRTUAL)"))
        prototype = (ROOT / "areas/world.obj").read_text(encoding="utf-8")
        item = prototype.split("#51\n", 1)[1].split("#52\n", 1)[0]
        flags = [int(value) for value in item.splitlines()[4].split()]
        defines = (ROOT / "src/core/defines.h").read_text(encoding="utf-8")
        transient = re.search(r"#define BIT_20 (\d+)U", defines)
        assert transient is not None
        self.assertFalse(flags[6] & 1, "VNUM 51 must remain untakeable for this route classification")
        self.assertFalse(flags[5] & int(transient.group(1)), "VNUM 51 must remain nontransient")

    def test_spell_created_room_items_refuse_before_unsourced_allocation(self):
        for source, symbol, allocation in (
            ("magic/spell_environment.c", "void spell_create_spring(",
             "read_object(750, VIRTUAL)"),
            ("magic/spell_environment.c", "void spell_divine_font(",
             "read_object(469, VIRTUAL)"),
            ("magic/blispells.c", "void spell_create_pond(",
             "read_object(749, VIRTUAL)"),
            ("magic/spells.c", "\nbool create_walls(",
             "read_object(VOBJ_WALLS, VIRTUAL)"),
            ("magic/smagic.c", "void spell_arieks_shattering_iceball(",
             "read_object(50, VIRTUAL)"),
        ):
            route = extract_function(source, symbol)
            self.assertLess(route.index("economic_gameplay_authority::active()"),
                            route.index(allocation))
        prototypes = (ROOT / "areas/world.obj").read_text(encoding="utf-8")
        defines = (ROOT / "src/core/defines.h").read_text(encoding="utf-8")
        transient = re.search(r"#define BIT_20 (\d+)U", defines)
        assert transient is not None
        for vnum in (50, 469, 749, 750, 759):
            object_text = prototypes.split(f"#{vnum}\n", 1)[1].split("\n#", 1)[0]
            flags = [int(value) for value in object_text.splitlines()[4].split()]
            self.assertFalse(flags[5] & int(transient.group(1)),
                             f"VNUM {vnum} must remain nontransient")
            if vnum == 469:
                self.assertTrue(flags[6] & 1, "VNUM 469 must remain takeable")

    def test_falling_object_refuses_durable_room_and_condition_changes(self):
        route = extract_function("magic/affects.c", "bool falling_obj(")
        guard = route.index("item_command_uses_durable_ownership(obj)")
        self.assertIn("economic_gameplay_authority::active()", route[:guard])
        self.assertLess(guard, route.index("obj->condition -= dam / 10"))
        self.assertLess(guard, route.index("obj->z_cord--"))
        self.assertLess(guard, route.index("obj_from_room(obj)"))
        self.assertLess(guard, route.index("add_event(event_falling_obj"))

    def test_item_damage_and_scrap_refuse_before_durable_mutation(self):
        damage = extract_function("world/condition.c", "int DamageOneItem(")
        guard = damage.index("item_command_uses_durable_ownership(obj)")
        self.assertIn("economic_gameplay_authority::active()", damage[:guard])
        self.assertLess(guard, damage.index("obj->condition -= num"))
        self.assertLess(guard, damage.index("MakeScrap(ch, obj)"))
        scrap = extract_function("world/condition.c", "void MakeScrap(")
        guard = scrap.index("item_command_uses_durable_ownership(obj)")
        self.assertIn("economic_gameplay_authority::active()", scrap[:guard])
        self.assertLess(guard, scrap.index("read_object(9, VIRTUAL)"))
        self.assertLess(guard, scrap.index("unequip_char(wearer, pos)"))
        self.assertLess(guard, scrap.index("extract_obj(obj)"))

    def test_ranged_commands_refuse_unaccounted_durable_item_changes(self):
        gather = extract_function("combat/range.c", "void do_gather(")
        self.assertLess(gather.index("economic_gameplay_authority::active()"),
                        gather.index("unequip_char(ch, i)"))
        self.assertLess(gather.index("economic_gameplay_authority::active()"),
                        gather.index("obj_to_obj(tobj, quiver)"))

        fire = extract_function("combat/range.c", "void do_fire(")
        initial_guard = fire.index("item_command_uses_durable_ownership(quiver)")
        self.assertIn("economic_gameplay_authority::active()", fire[:initial_guard])
        self.assertLess(initial_guard, fire.index("missile->timer[0] ="))
        next_guard = fire.index("item_command_uses_durable_ownership(missile)",
                                initial_guard + 1)
        self.assertLess(next_guard, fire.index("obj_from_obj(missile)"))
        shield_guard = fire.index("item_command_uses_durable_ownership(shield)")
        self.assertLess(shield_guard, fire.index("shield->condition -= number(1, 3)"))

        thrown = extract_function("combat/range.c", "void do_throw(")
        guard = thrown.index("item_command_uses_durable_ownership(weapon)")
        self.assertIn("economic_gameplay_authority::active()", thrown[:guard])
        self.assertLess(guard, thrown.index("melee_damage(ch, vict"))
        self.assertLess(guard, thrown.index("obj_from_char(weapon)"))

        loaded = extract_function("combat/range.c", "void do_load_weapon(")
        guard = loaded.index("item_command_uses_durable_ownership(weapon)")
        self.assertIn("item_command_uses_durable_ownership(missile)", loaded[guard:])
        self.assertLess(guard, loaded.index("extract_obj(missile, TRUE)"))
        self.assertLess(guard, loaded.index("weapon->value[2] += num_ammo"))

    def test_disintegrate_and_spellbind_refuse_unaccounted_item_mutation(self):
        disintegrate = extract_function("magic/spell_direct_attacks.c",
                                        "void spell_disintegrate(")
        guard = disintegrate.index("item_command_uses_durable_ownership(obj)")
        self.assertIn("economic_gameplay_authority::active()", disintegrate[:guard])
        self.assertLess(guard, disintegrate.index("obj->condition -= BOUNDED("))
        self.assertLess(guard, disintegrate.index("unequip_char_dale(obj)"))
        self.assertLess(guard, disintegrate.index("extract_obj(obj)"))

        spellbind = extract_function("classes/salchemist.c", "void do_spellbind(")
        guard = spellbind.index("item_command_uses_durable_ownership(item)")
        self.assertIn("economic_gameplay_authority::active()", spellbind[:guard])
        self.assertLess(guard, spellbind.index("item->condition = item->condition -"))
        self.assertLess(guard, spellbind.index("epic_transaction_submit(ch, -1"))

        committed = extract_function("classes/salchemist.c", "void spellbind_committed(")
        guard = committed.index("item_command_uses_durable_ownership(item)")
        self.assertIn("economic_gameplay_authority::active()", committed[:guard])
        self.assertLess(guard, committed.index("item->condition -= number(1, 20)"))
        self.assertLess(guard, committed.index("SET_BIT(item->extra_flags"))

    def test_forage_refuses_before_unsourced_object_allocation(self):
        forage = extract_function("cmd/actoth.c", "void do_forage(")
        guard = forage.index("economic_gameplay_authority::active()")
        self.assertLess(guard, forage.index("read_object(VOBJ_FORAGE_FIRST"))
        self.assertLess(guard, forage.index("forage_sect(ch,"))

    def test_level_achievement_defers_gift_and_coin_reward(self):
        achievements = extract_function("world/achievements.c",
                                        "void update_achievements(")
        gift_guard = achievements.index("!economic_gameplay_authority::active() && GET_LEVEL(ch) >= 5")
        coin_guard = achievements.index("!economic_gameplay_authority::active() && GET_LEVEL(ch) >= 20")
        self.assertLess(gift_guard, achievements.index("gift = read_object(400222"))
        self.assertLess(gift_guard, achievements.index("paf->modifier = 5"))
        self.assertLess(coin_guard, achievements.index("ADD_MONEY(ch, 100000)"))
        self.assertLess(coin_guard, achievements.index("paf->modifier = 20"))

    def test_consumable_routes_refuse_before_active_item_mutation(self):
        disguise = extract_function("classes/disguise.c", "void do_disguise(")
        disguise_guard = disguise.index("economic_gameplay_authority::active()")
        self.assertLess(disguise.index("if (!*arg)"), disguise_guard)
        self.assertLess(disguise_guard, disguise.index("percent = number(1, 101)"))
        self.assertLess(disguise_guard, disguise.index("unequip_char(ch, HOLD)"))

        store = extract_function("classes/drannak.c", "int pvp_store(")
        purchase = store[store.index('else if (strstr(arg, "1"))'):]
        purchase_guard = purchase.index("economic_gameplay_authority::active()")
        self.assertLess(purchase_guard, purchase.index("read_object(VOBJ_GREATER_ORB_MAGIC"))
        self.assertLess(purchase_guard, purchase.index("item_movement_transaction_submit_craft("))

        conjure = extract_function("classes/drannak.c", "void do_conjure(")
        conjure_guard = conjure.index("economic_gameplay_authority::active()")
        self.assertLess(conjure_guard, conjure.index("chance = dice("))
        self.assertLess(conjure_guard,
                        conjure.index("vnum_from_inv(ch, VOBJ_GREATER_ORB_MAGIC"))

        sight = extract_function("classes/ethermancer.c", "void spell_faerie_sight(")
        self.assertLess(sight.index("faerie_sight_dust_count("),
                        sight.index("spell_consume_components("))
        self.assertLess(sight.index("spell_consume_components("),
                        sight.index("apply_faerie_sight(level, ch, victim, 0, !active)"))

        salvage = extract_function("item/salvage.c", "void do_salvage(")
        salvage_guard = salvage.index("economic_gameplay_authority::active()")
        self.assertLess(salvage_guard, salvage.index("grant_salvage_item(ch,"))
        self.assertLess(salvage_guard, salvage.index("extract_obj(item)"))
        self.assertLess(salvage_guard,
                        salvage.index("vnum_from_inv(ch, crafting_scientific_tools_vnum()"))

    def test_mining_refuses_queued_rewards_and_node_mutation(self):
        mine = extract_function("economy/mining.c", "int mine(")
        self.assertLess(mine.index("economic_gameplay_authority::active()"),
                        mine.index("extract_obj(obj, TRUE)"))
        start_guard = mine.index("economic_gameplay_authority::active()",
                                 mine.index("if (cmd == CMD_MINE)"))
        self.assertLess(start_guard, mine.index("remove_mine_content(obj)"))
        self.assertLess(start_guard, mine.index("add_event(event_mine_check"))

        event = extract_function("economy/mining.c", "void event_mine_check(")
        guard = event.index("economic_gameplay_authority::active()")
        self.assertLess(guard, event.index("--mdata->counter"))
        self.assertLess(guard, event.index("get_gem_from_mine(ch,"))
        self.assertLess(guard, event.index("obj_to_room(ore, ch->in_room)"))
        self.assertLess(guard, event.index("unequip_char(ch, WIELD)"))

        placement = extract_function("economy/mining.c", "bool load_one_mine(int map)\n{")
        self.assertLess(placement.index("economic_gameplay_authority::active()"),
                        placement.index("read_object(mine_data[map].type"))
        admin = extract_function("economy/mining.c", "void do_mine(")
        admin_guard = admin.index("economic_gameplay_authority::active()")
        self.assertLess(admin_guard, admin.index("do_mine(ch, buf2, CMD_MINE)"))
        self.assertLess(admin_guard, admin.index("load_one_mine(i)"))
        self.assertLess(admin_guard, admin.index("extract_obj(tobj, TRUE)"))

        prototypes = (ROOT / "areas/world.obj").read_text(encoding="utf-8")
        defines = (ROOT / "src/core/defines.h").read_text(encoding="utf-8")
        transient = re.search(r"#define BIT_20 (\d+)U", defines)
        assert transient is not None
        for vnum in (193, 434):
            node = prototypes.split(f"#{vnum}\n", 1)[1].split("\n#", 1)[0]
            flags = [int(value) for value in node.splitlines()[4].split()]
            self.assertFalse(flags[5] & int(transient.group(1)))
            self.assertFalse(flags[6] & 1)

    def test_kingdom_harvest_refuses_unsourced_nodes_and_materials(self):
        path = "kingdom/kingdom_harvest.c"
        for symbol, mutation in (
            ("void kingdom_node_reap_room(", "extract_obj(obj)"),
            ("static int kingdom_nodes_reap(", "extract_obj(doomed[i])"),
            ("static bool kingdom_load_one_node(", "read_object(vnum, VIRTUAL)"),
            ("static void kingdom_nodes_reload(", "kingdom_nodes_reap()"),
            ("static void kingdom_harvest_tick(P_char ch, P_char /*victim*/, P_obj, void *data)\n{",
             "kingdom_resource_deposit("),
            ("static void kingdom_gather_tick(P_char ch, P_char /*victim*/, P_obj, void *data)\n{",
             "GET_VITALITY(ch) -= "),
            ("static void kingdom_gather_command(P_char ch, P_obj node)\n{",
             "add_event(kingdom_gather_tick"),
            ("void kingdom_harvest_command(", "kingdom_node_reap_room(rnum)"),
        ):
            body = extract_function(path, symbol)
            self.assertLess(body.index("economic_gameplay_authority::active()"),
                            body.index(mutation), symbol)

        realm_tick = extract_function(
            path,
            "static void kingdom_harvest_tick(P_char ch, P_char /*victim*/, P_obj, void *data)\n{",
        )
        realm_guard = realm_tick.index("economic_gameplay_authority::active()")
        self.assertLess(realm_guard, realm_tick.index("GET_VITALITY(ch) -= "))
        self.assertLess(realm_guard, realm_tick.index("node->value[0]--"))
        gather_tick = extract_function(
            path,
            "static void kingdom_gather_tick(P_char ch, P_char /*victim*/, P_obj, void *data)\n{",
        )
        gather_guard = gather_tick.index("economic_gameplay_authority::active()")
        self.assertLess(gather_guard, gather_tick.index("node->value[0]--"))
        self.assertLess(gather_guard, gather_tick.index("read_object(mat_rnum, REAL)"))

        reload_body = extract_function(path, "static void kingdom_nodes_reload(")
        self.assertIn("kingdom_node_schedule_sweep(region);", reload_body[
            reload_body.index("economic_gameplay_authority::active()"):reload_body.index(
                "kingdom_nodes_reap()")
        ])

        periodic = extract_function(path, "static int kingdom_node_proc(")
        self.assertLess(periodic.index("!economic_gameplay_authority::active()"),
                        periodic.index("extract_obj(obj)"))
        shutdown = extract_function(path, "void kingdom_harvest_shutdown(")
        self.assertLess(shutdown.index("!economic_gameplay_authority::active()"),
                        shutdown.index("extract_obj(doomed[i])"))

        prototypes = (ROOT / "areas/obj/heavens.obj").read_text(encoding="utf-8")
        defines = (ROOT / "src/core/defines.h").read_text(encoding="utf-8")
        transient = re.search(r"#define BIT_20 (\d+)U", defines)
        assert transient is not None
        for vnum in range(477, 485):
            node = prototypes.split(f"#{vnum}\n", 1)[1].split("\n#", 1)[0]
            flags = [int(value) for value in node.splitlines()[4].split()]
            self.assertFalse(flags[5] & int(transient.group(1)))


if __name__ == "__main__":
    unittest.main()
