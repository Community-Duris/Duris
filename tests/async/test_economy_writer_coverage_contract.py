#!/usr/bin/env python3
"""Negative activation contract for a real, currently unguarded economy writer."""
from __future__ import annotations

import json
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import generate_economy_writer_coverage as coverage  # noqa: E402

MATRIX = ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json"


class SplitEconomyActivationContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.artifact = json.loads(MATRIX.read_text(encoding="utf-8"))
        cls.routes = {route["id"]: route for route in cls.artifact["routes"]}

    def test_machine_counts_reconcile_to_route_rows(self) -> None:
        routes = self.artifact["routes"]
        counts = self.artifact["counts"]
        self.assertEqual(len(routes), counts["matrix_rows"])
        self.assertEqual(len({route["id"] for route in routes}), len(routes))
        self.assertEqual(sum(route["current_critical_command_schema"]["current_schema"] == 1 for route in routes),
                         counts["routes_with_current_schema_1_path"])
        self.assertEqual(sum(route["current_critical_command_schema"]["schema_2_gameplay_producer_connected"] for route in routes),
                         counts["routes_with_schema_2_gameplay_producer"])
        self.assertEqual(sum(route["double_entry_evidence"]["unified_operation_postings_observed"] for route in routes),
                         counts["routes_with_unified_double_entry_evidence"])
        self.assertEqual(sum(route["counts_as_runtime_writer"] for route in routes),
                         counts["runtime_mutation_and_projection_routes"])
        self.assertEqual(sum(route["counts_as_operational_writer"] for route in routes),
                         counts["offline_operational_writer_routes"])
        self.assertEqual(sum(route["disposition"] == "dormant_writer_candidate" for route in routes),
                         counts["dormant_writer_candidates"])
        self.assertEqual(sum(route["disposition"] == "non_writer_candidate" for route in routes),
                         counts["non_writer_or_out_of_scope_candidates"])

    def test_checked_item_placement_refactor_keeps_all_sites_classified(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                owners.setdefault(tuple(site), set()).add(route["id"])
        checked = [row for row in coverage.load_validator().scan_sources(ROOT)
                   if row["family"] == "item_publication" and
                   "obj_to_char_checked(" in row["excerpt"]]
        self.assertEqual(len({(row["path"], row["line"]) for row in checked}), 9)
        expected = {
            ("src/cmd/actobj.c", 482): "item.command_publication",
            ("src/cmd/actobj.c", 710): "item.command_publication",
            ("src/cmd/actobj.c", 769): "item.pet_give_publication",
            ("src/cmd/actobj.c", 1339): "item.legacy_get",
            ("src/cmd/actobj.c", 6281): "item.legacy_give",
            ("src/cmd/actobj.c", 7845): "item.equipment_remove",
            ("src/world/handler.c", 1856): "item.obj_to_char_admission",
            ("src/world/handler.c", 2029): "item.obj_to_char_admission",
            ("src/world/handler.h", 15): "macro.checked_item_publication_declaration",
        }
        for row in checked:
            site = (row["path"], row["line"], row["family"])
            self.assertEqual(owners[site], {expected[(row["path"], row["line"])]})
        self.assertEqual(self.routes["item.obj_to_char_admission"]["source"]["function"],
                         "obj_to_char_checked")
        self.assertEqual(self.routes["macro.checked_item_publication_declaration"]
                         ["disposition"], "non_writer_candidate")

    def test_sql_coin_component_is_observed_without_claiming_release(self) -> None:
        route = self.routes["coin.sql_accounting"]
        self.assertEqual(route["source"]["function"], "coin_transfer_accounting_record")
        self.assertEqual(route["current_critical_command_schema"]["current_schema"], 2)
        self.assertFalse(route["current_critical_command_schema"]["schema_2_gameplay_producer_connected"])
        self.assertTrue(route["double_entry_evidence"]["unified_operation_postings_observed"])
        self.assertEqual(route["double_entry_evidence"]["status"],
                         "sql_component_balanced_postings_without_playable_route")
        self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertFalse(self.artifact["coverage_complete"])
        self.assertEqual(self.artifact["playable_release_status"], "BLOCKED")

    def test_ship_coffer_writers_are_separate_from_hydration(self) -> None:
        for route_id in ("ship.combat_reward", "ship.coffer_claim", "ship.insurance_fallback",
                         "ship.sql_save", "ship.sql_delete"):
            route = self.routes[route_id]
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["reachability_evidence"])
            self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        for route_id in ("ship.hydration", "ship.sql_hydration"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
        self.assertEqual(self.routes["ship.zero_initialization"]["disposition"], "non_writer_candidate")
        census = coverage.load_validator().scan_sources(ROOT)
        ship_sites = [row for row in census if row["family"] == "ship_coffer_assignment"]
        self.assertEqual(len(ship_sites), 6)
        self.assertTrue(any(row["path"] == "src/sql/sql_player.c" and
                            "insert into ships" in row["excerpt"]
                            for row in census if row["family"] == "sql_economy"))

    def test_numbered_quest_and_coin_steal_direct_writers_are_blocked(self) -> None:
        expected = {
            "nq.reward": ("nq_reward_player", "quest_reward"),
            "nq.requirement_consumption": ("nq_test_single_action", "quest_cost"),
            "nq.item_allocation": ("nq_create_item", "item_create"),
            "currency.coin_steal": ("do_steal", "wallet_transfer"),
        }
        for route_id, (function, reason) in expected.items():
            route = self.routes[route_id]
            self.assertEqual(route["source"]["function"], function)
            self.assertEqual(route["reason"], reason)
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
            self.assertFalse(route["double_entry_evidence"]["unified_operation_postings_observed"])
            self.assertTrue(any("source-order contract only" in item
                                for item in route["refusal_source_evidence"]))
        self.assertEqual(self.routes["currency.coin_steal"]["current_critical_command_schema"]["route_mode"],
                         "schema_1_plus_direct_side_effects")
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        linked = {row["id"]: row["sites"] for row in registry["writers"]}
        self.assertEqual(len(linked["nq.requirement_consumption"]), 5)
        self.assertEqual(sum(site[2] == "direct_cash_assignment"
                             for site in linked["special.smelter"]), 2)
        self.assertEqual(sum(site[2] == "direct_cash_assignment"
                             for site in linked["currency.card_game_payout"]), 3)

    def test_coin_steal_refuses_active_epoch_before_victim_debit(self) -> None:
        source = coverage.mask_cpp((ROOT / "src/cmd/actoth.c").read_text(encoding="utf-8"))
        steal = source[source.index("void do_steal("):source.index("void do_split(")]
        guard = steal.index("economic_gameplay_authority::active()")
        refusal = steal.index("return;", guard)
        victim_lookup = steal.index("get_char_room_vis(ch, victim_name)")
        debit = steal.index("victim->points.cash[vcoins]--")
        credit = steal.index("ADD_MONEY(ch")
        self.assertLess(guard, refusal)
        self.assertLess(refusal, victim_lookup)
        self.assertLess(refusal, debit)
        self.assertLess(debit, credit)

    def test_numbered_quest_refuses_economic_actions_before_consumption_or_reward(self) -> None:
        source = coverage.mask_cpp((ROOT / "src/cmd/nq.c").read_text(encoding="utf-8"))
        requirement = source[source.index("int nq_test_single_action("):
                             source.index("struct nq_instance *nq_accept_quest(")]
        reward = source[source.index("void nq_reward_player("):
                        source.index("int nq_test_single_action(")]
        admission = source[source.index("int nq_action_check_all("):
                           source.index("void nq_char_death(")]
        guard = admission.index("economic_gameplay_authority::active()")
        refusal = admission.index("continue;", guard)
        self.assertLess(refusal, admission.index("nq_test_single_action(action, instance"))
        self.assertIn("action->cash != 0", admission[guard:refusal])
        self.assertIn("action->reward->cash != 0", admission[guard:refusal])
        self.assertIn("extract_obj(components[found]", requirement)
        self.assertIn("ch->points.cash[3] +=", reward)
        self.assertIn("mob_cash -= action->cash;", requirement)
        self.assertIn("nq_create_item(item)", reward)

    def test_smelter_refuses_active_cash_and_ore_before_mutation(self) -> None:
        route = self.routes["special.smelter"]
        self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertTrue(route["refusal_source_evidence"])
        source = coverage.mask_cpp((ROOT / "src/specs/specs.alatorin.c").read_text(encoding="utf-8"))
        smelter = source[source.index("int smelter("):source.index("void finish_smelt(",
                                                               source.index("int smelter("))]
        first_guard = smelter.index("economic_gameplay_authority::active()")
        first_refusal = smelter.index("return TRUE;", first_guard)
        cash_debit = smelter.index("pl->points.cash[type] -= amount")
        ore_move = smelter.index("obj_from_char(obj)")
        self.assertLess(first_refusal, cash_debit)
        self.assertLess(first_refusal, ore_move)

    def test_blackjack_refuses_active_wagers_and_pending_payout(self) -> None:
        route = self.routes["currency.card_game_payout"]
        self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertTrue(route["refusal_source_evidence"])
        source = (ROOT / "src/economy/cardgames.c").read_text(encoding="utf-8")
        start = source.index("int blackjack_table(")
        table = source[start:source.index("void event_dealersturn(", start)]
        guard = table.index("economic_gameplay_authority::active()")
        periodic = table.index("if (cmd == CMD_PERIODIC && obj->value[0] == BJ_DEALERSTURN)", guard)
        offer = table.index("if (cmd == CMD_OFFER)", periodic)
        say = table.index("if (cmd == CMD_SAY && *argument)", offer)
        for keyword in ('"deal"', '"stay"', '"fold"', '"hit"'):
            self.assertIn(keyword, table[say:])
        self.assertLess(periodic, table.index("ch->points.cash[obj->value[2]] +="))
        self.assertLess(offer, table.index("SUB_MONEY(ch, betamt"))
        self.assertIn("return FALSE;", table[periodic:offer])

    def test_magic_deck_dealer_bust_uses_live_synchronous_settlement(self) -> None:
        source = (ROOT / "src/specs/specs.gellz.c").read_text(encoding="utf-8")
        deck = source[source.index("int magic_deck("):source.index("// End GELLZ_ magic_deck")]
        immediate = deck[deck.index("if (dealer_total > 21)"):
                         deck.index("else if (player_total > dealer_total)")]
        self.assertIn("do_win(ch, bettype, 2 * betamt, 1);", immediate)
        self.assertNotIn("game_on = BJ_DEALERSTURN", deck)
        self.assertNotIn("if (cmd == CMD_PERIODIC", deck)

    def test_sql_and_flatfile_player_item_transfer_producer_is_documented_without_overstating_coverage(self) -> None:
        for route_id in ("item.command_movement", "item.bulk_movement", "item.movement_submit",
                         "item.trusted_steal", "item.creation_completion"):
            route = self.routes[route_id]
            schema = route["current_critical_command_schema"]
            evidence = route["double_entry_evidence"]
            self.assertTrue(schema["schema_2_gameplay_producer_connected"])
            self.assertTrue(schema["accounting_intent_attached_by_current_gameplay_route"])
            self.assertEqual(schema["current_schema"], 1,
                             "the inactive accounting gate still uses the legacy schema-1 path")
            self.assertIn("SQL and flat-file", schema["interpretation"])
            self.assertIn("source claim", schema["interpretation"])
            self.assertIn("exact custody references", schema["interpretation"])
            self.assertEqual(evidence["status"],
                             "typed_schema2_item_custody_reference_without_coin_effect")
            self.assertFalse(evidence["unified_operation_postings_observed"],
                             "custody-only movement must not be reported as a coin double-entry posting")
            self.assertTrue(any("economic_accounting_item_reference" in item
                                for item in evidence["legacy_domain_evidence"]))

    def test_corpse_item_handoffs_use_actor_bound_item_references(self) -> None:
        create = self.routes["death.corpse_creation"]
        resurrection = self.routes["death.resurrection_publication"]
        for route in (create, resurrection):
            schema = route["current_critical_command_schema"]
            evidence = route["double_entry_evidence"]
            self.assertTrue(schema["schema_2_gameplay_producer_connected"])
            self.assertTrue(schema["accounting_intent_attached_by_current_gameplay_route"])
            self.assertEqual(schema["current_schema"], 1,
                             "the inactive authority still submits the legacy envelope")
            self.assertEqual(evidence["status"],
                             "typed_schema2_item_custody_reference_without_coin_effect")
            self.assertFalse(evidence["unified_operation_postings_observed"])
            self.assertIn("exact custody references", schema["interpretation"])
            self.assertIn("separate", schema["interpretation"])
            self.assertTrue(any("economic_accounting_item_reference" in item
                                for item in evidence["legacy_domain_evidence"]))
        creation_evidence = " ".join(create["current_critical_command_schema"]["evidence"])
        self.assertIn("src/combat/fight.c", creation_evidence)
        self.assertIn("actor", creation_evidence)
        self.assertIn("corpse", create["current_critical_command_schema"]["interpretation"])

    def test_flatfile_item_evidence_is_journaled_with_custody(self) -> None:
        for route_id in ("item.command_movement", "item.bulk_movement", "item.movement_submit",
                         "item.trusted_steal", "item.creation_completion",
                         "death.corpse_creation", "death.resurrection_publication"):
            route = self.routes[route_id]
            evidence = route["double_entry_evidence"]
            self.assertIn("flat-file authority journal", " ".join(evidence["required_atomic_evidence"]))
            self.assertIn("flat-file player transfers", " ".join(evidence["legacy_domain_evidence"]))

    def test_key_break_retirement_does_not_claim_all_extraction_paths(self) -> None:
        movement = self.routes["item.movement_submit"]
        interpretation = movement["current_critical_command_schema"]["interpretation"]
        self.assertIn("key-break retirement", interpretation)
        self.assertIn("Other item destruction/extraction paths", interpretation)
        extraction = self.routes["item.extraction"]
        self.assertFalse(extraction["current_critical_command_schema"]
                         ["schema_2_gameplay_producer_connected"])
        self.assertTrue(extraction["blocking_policy_after_activation"]
                        ["must_block_on_activation"])

    def test_component_spell_retirement_uses_the_typed_item_lifecycle_route(self) -> None:
        route = self.routes["item.movement_submit"]
        evidence = " ".join(route["current_critical_command_schema"]["evidence"])
        self.assertIn("src/magic/spell_conjuration.c", evidence)
        helper = (ROOT / "src/magic/magic.c").read_text(encoding="utf-8")
        self.assertRegex(helper, r"spell_consume_components\([\s\S]*?item_movement_transaction_submit_batch\(")
        self.assertRegex(helper, r"item_transfer_reason::destruction[\s\S]*?economic_source_kind::spell_consumption")
        legacy = helper[helper.index("int get_spell_component("):helper.index("namespace\n{")]
        self.assertRegex(legacy, r"economic_gameplay_authority::active\(\)\s*&&\s*ch\s*&&\s*IS_PC\(ch\)\)\s*return 0;")
        consumers = (
            "src/magic/spell_conjuration.c", "src/magic/spell_spore_cloud.c",
            "src/magic/spells.c", "src/classes/necromancy.c",
        )
        for source in consumers:
            self.assertIn("spell_consume_components(", (ROOT / source).read_text(encoding="utf-8"))

    def test_do_split_uses_sequential_balanced_children_after_activation(self) -> None:
        route = self.routes["currency.split"]
        self.assertEqual(route["source"]["file"], "src/cmd/actoth.c")
        self.assertEqual(route["source"]["function"], "do_split")
        self.assertEqual(route["source"]["definition_lines"],
                         coverage.source_definition_lines(ROOT / route["source"]["file"],
                                                          route["source"]["function"]))
        self.assertEqual(route["current_critical_command_schema"]["current_schema"], 1)
        self.assertEqual(route["current_critical_command_schema"]["route_mode"],
                         "schema_1_when_inactive_schema_2_sequential_coin_children_when_active")
        self.assertTrue(route["current_critical_command_schema"]["schema_2_gameplay_producer_connected"])
        self.assertTrue(route["double_entry_evidence"]["unified_operation_postings_observed"])
        self.assertFalse(route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertEqual(route["blocking_policy_after_activation"]["decision"],
                         "allow_sequential_schema2_coin_children")

        raw = (ROOT / route["source"]["file"]).read_text(encoding="utf-8")
        code = coverage.mask_cpp(raw)
        declaration = re.search(r"\bvoid\s+do_split\s*\(", code)
        self.assertIsNotNone(declaration)
        assert declaration is not None
        body_start = code.find("{", declaration.end())
        self.assertGreaterEqual(body_start, 0)
        depth = 0
        body_end = -1
        for index in range(body_start, len(code)):
            if code[index] == "{":
                depth += 1
            elif code[index] == "}":
                depth -= 1
                if depth == 0:
                    body_end = index
                    break
        self.assertGreater(body_end, body_start)
        body = code[body_start:body_end + 1]
        active = body.find("if (economic_gameplay_authority::active())")
        legacy_debit = body.find("SUB_MONEY(ch")
        legacy_credit = body.find("ADD_MONEY(gl->ch")
        self.assertGreaterEqual(active, 0)
        self.assertLess(active, legacy_debit)
        self.assertLess(legacy_debit, legacy_credit)
        self.assertIn("continue_money_split(split_id)", body[active:legacy_debit])
        self.assertIn("currency_transaction_submit_coin(actor, payload, money_split_child_completion",
                      code)
        self.assertIn("Completed shares remain transferred", route["current_critical_command_schema"]["interpretation"])

    def test_unmatched_lexical_sites_keep_matrix_incomplete(self) -> None:
        self.assertFalse(self.artifact["coverage_complete"])
        self.assertEqual(self.artifact["lexical_census"]["unmapped_current_unique_sites"], 0)
        self.assertEqual(self.artifact["playable_release_status"], "BLOCKED")

    def test_bank_load_result_is_read_only_and_local_buffer_is_not_a_writer(self) -> None:
        route = self.routes["player.bank_load_result"]
        self.assertEqual(route["disposition"], "non_writer_candidate")
        self.assertEqual(route["source"]["function"], "load_bank")
        self.assertIn("account_banks", route["exclusion_reason"])
        census = coverage.load_validator().scan_sources(ROOT)
        direct = [row for row in census if row["family"] == "direct_cash_assignment"]
        self.assertFalse(any(row["path"] == "src/economy/collector_repository.c" and
                             "uint64_t bank[6]" in row["excerpt"] for row in direct))
        self.assertTrue(any(row["path"] == "src/player/player_load_repository.c" and
                            "result->domains.bank[index]" in row["excerpt"] for row in direct))

    def test_money_helper_mutations_and_bank_publication_are_classified_separately(self) -> None:
        credit = self.routes["currency.wallet_credit"]
        debit = self.routes["currency.wallet_debit"]
        bank = self.routes["currency.bank_live_projection"]
        single = self.routes["currency.bank_single_projection"]
        wallet = self.routes["currency.wallet_live_projection"]
        load = self.routes["player.load_economy_projection"]
        self.assertEqual(credit["disposition"], "runtime_mutation_route")
        self.assertEqual(debit["disposition"], "runtime_mutation_route")
        self.assertEqual(bank["disposition"], "runtime_projection_route")
        self.assertEqual(single["disposition"], "dormant_writer_candidate")
        self.assertEqual(wallet["disposition"], "runtime_projection_route")
        self.assertEqual(load["disposition"], "runtime_projection_route")
        self.assertEqual(bank["blocking_policy_after_activation"]["decision"],
                         "allow_projection_only_with_committed_identity")
        self.assertIn("bank revision", bank["blocking_policy_after_activation"]["required_policy"])
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        sites = {row["id"]: row["sites"] for row in registry["writers"]}
        self.assertEqual(sum(site[2] == "coin_assignment"
                             for site in sites["currency.wallet_credit"]), 4)
        self.assertEqual(sum(site[2] == "coin_assignment"
                             for site in sites["currency.wallet_debit"]), 8)
        self.assertEqual(len(sites["currency.bank_live_projection"]), 4)
        self.assertEqual(len(sites["currency.bank_single_projection"]), 4)
        self.assertEqual(len(sites["currency.wallet_live_projection"]), 4)
        self.assertEqual(len(sites["player.load_economy_projection"]), 8)
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)}
        for route_id in ("currency.wallet_credit", "currency.wallet_debit",
                         "currency.bank_live_projection", "currency.bank_single_projection",
                         "currency.wallet_live_projection", "player.load_economy_projection"):
            self.assertTrue(all(tuple(site) in current for site in sites[route_id]))

    def test_npc_coin_generation_pet_clear_and_copyover_recovery_are_distinct(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        sites = {row["id"]: row["sites"] for row in registry["writers"]}
        expected = {
            "world.mobile_scaling": ("src/mob/mobconv.c", {247, 248, 249, 250, 257, 262, 276, 284}),
            "pet.no_cash": ("src/classes/necromancy.c", {256, 257, 258, 259}),
            "world.generated_npc_hydration": ("src/world/generated_npc_runtime.c", {106, 107, 108, 109}),
        }
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)}
        for route_id, (path, lines) in expected.items():
            self.assertEqual({tuple(site) for site in sites[route_id]
                              if site[2] == "coin_assignment"},
                             {(path, line, "coin_assignment") for line in lines})
            self.assertTrue(all(tuple(site) in current for site in sites[route_id]))
        for route_id in ("world.mobile_scaling", "pet.no_cash"):
            route = self.routes[route_id]
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
            self.assertIn("provisional", route["blocking_policy_after_activation"]["required_policy"])
        recovery = self.routes["world.generated_npc_hydration"]
        self.assertEqual(recovery["disposition"], "runtime_projection_route")
        self.assertIn("gold overwrite", recovery["blocking_policy_after_activation"]["required_policy"])
        self.assertIn("NPC cash", " ".join(recovery["native_effects"]["native_state_targets"]))

    def test_legacy_restore_cash_has_explicit_source_and_publication_gates(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        sites = {row["id"]: row["sites"] for row in registry["writers"]}
        expected = {
            "world.mobile_template": ("src/world/db.c", {2281, 2282, 2283, 2284, 2672, 2673, 2674, 2675}),
            "player.flatfile_baseline_projection": ("src/core/files.c", {1872, 1873, 1874, 1875, 1877, 1878, 1879, 1880}),
            "player.legacy_flatfile_load": ("src/core/files.c", {2426, 2427, 2428, 2429}),
            "recovery.pet_cash_discard": ("src/core/files.c", {4691, 4692, 4693, 4694, 4696, 4697, 4698, 4699}),
            "recovery.copyover_npc_gold_projection": ("src/persistence/copyover.c", {1633, 2144}),
        }
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)}
        for route_id, (path, lines) in expected.items():
            self.assertEqual({tuple(site) for site in sites[route_id]
                              if site[2] == "coin_assignment"},
                             {(path, line, "coin_assignment") for line in lines})
            self.assertTrue(all(tuple(site) in current for site in sites[route_id]))
        for route_id in ("world.mobile_template", "recovery.pet_cash_discard"):
            route = self.routes[route_id]
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
        for route_id in ("player.flatfile_baseline_projection", "player.legacy_flatfile_load",
                         "recovery.copyover_npc_gold_projection"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
        for route_id in ("player.legacy_flatfile_load", "world.generated_npc_hydration",
                         "recovery.copyover_npc_gold_projection"):
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("saved pet", self.routes["recovery.pet_cash_discard"]
                      ["native_effects"]["holding_effect"])
        self.assertIn("inconsistent decoded and legacy gold", self.routes
                      ["recovery.copyover_npc_gold_projection"]
                      ["blocking_policy_after_activation"]["required_policy"])

    def test_direct_coin_sites_and_clear_money_calls_are_classified(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)}
        mapped = {tuple(site) for route in registry["writers"] for site in route.get("sites", [])}
        for family in ("coin_assignment", "direct_cash_assignment",
                       "coin_bulk_mutation", "ship_coffer_assignment"):
            family_sites = {site for site in current if site[2] == family}
            self.assertTrue(family_sites, family)
            self.assertFalse(family_sites - mapped, family)
        shop_restore = self.routes["recovery.flat_shopkeeper_cash_materialization"]
        self.assertEqual(shop_restore["disposition"], "runtime_projection_route")
        self.assertEqual(shop_restore["blocking_policy_after_activation"]["decision"],
                         "block_until_projection_proof")
        self.assertFalse(shop_restore["double_entry_evidence"]
                         ["unified_operation_postings_observed"])
        self.assertIn("retained flatfile shop cash", shop_restore["projection_rule"])
        sql_shop_restore = self.routes["recovery.sql_shopkeeper_cash_materialization"]
        self.assertEqual(sql_shop_restore["disposition"], "runtime_projection_route")
        self.assertEqual(sql_shop_restore["blocking_policy_after_activation"]["decision"],
                         "block_until_projection_proof")
        self.assertFalse(sql_shop_restore["double_entry_evidence"]
                         ["unified_operation_postings_observed"])
        self.assertIn("legacy NULL cash", sql_shop_restore["blocking_policy_after_activation"]
                      ["required_policy"])
        clear_calls = {
            ("src/mob/mobpatrol.c", 89, "money_helper"),
            ("src/combat/justice.c", 258, "money_helper"),
            ("src/specs/specs.venthix.c", 918, "money_helper"),
        }
        self.assertTrue(clear_calls <= current & mapped)
        self.assertEqual(self.routes["macro.clear_money_definition"]["disposition"],
                         "non_writer_candidate")
        for route_id in ("world.patrol_clear_cash", "world.justice_guard_clear_cash",
                         "world.zgame_zombie_clear_cash"):
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertEqual(self.routes["player.sql_bank_live_load"]["blocking_policy_after_activation"]
                         ["decision"], "block_until_projection_proof")
        for route_id in ("macro.add_coins_declaration", "macro.difficulty_scale_declaration",
                         "coin.pile_description_staging"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        self.assertIn("transient", self.routes["coin.container_put"]["source_classification"])

    def test_direct_sql_economy_sites_have_named_routes(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["family"] == "sql_economy"}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[2] == "sql_economy":
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review newly detected native SQL writes")
        self.assertEqual(owners[("src/economy/auction_houses.c", 935, "sql_economy")],
                         {"auction.money_claim_compensation"})
        self.assertEqual(owners[("src/economy/auction_houses.c", 2966, "sql_economy")],
                         {"auction.money_claim_legacy"})
        self.assertEqual(owners[("src/sql/sql_player.c", 9561, "sql_economy")],
                         {"recovery.saved_sql_delete"})
        self.assertEqual(owners[("src/sql/sql_player.c", 10982, "sql_economy")],
                         {"recovery.saved_sql"})
        shop_path = "src/persistence/economic_sql_shop_trade_transaction.c"
        for line in (888, 908, 941):
            self.assertEqual(owners[(shop_path, line, "sql_economy")],
                             {"shop.sql_native_item_events"})
        self.assertEqual(owners[(shop_path, 1075, "sql_economy")],
                         {"shop.sql_native_balances"})
        for route_id in ("shop.sql_native_item_events", "shop.sql_native_balances"):
            route = self.routes[route_id]
            self.assertEqual(route["current_critical_command_schema"]["current_schema"], 2)
            self.assertFalse(route["current_critical_command_schema"]
                             ["schema_2_gameplay_producer_connected"])
            self.assertTrue(route["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_quest_offering_sites_keep_consumption_and_publication_distinct(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/world/quest.c"
        owners = {}
        for writer in registry["writers"]:
            for site in writer.get("sites", []):
                if site[0] == path:
                    owners.setdefault(tuple(site), set()).add(writer["id"])
        self.assertEqual(owners[(path, 1554, "economic_submit")],
                         {"quest.durable_offering_submission"})
        self.assertEqual(owners[(path, 917, "item_lifecycle")],
                         {"quest.durable_offering_publication"})
        for line, family in ((876, "item_lifecycle"), (876, "item_publication"),
                             (878, "item_lifecycle"), (1050, "item_lifecycle"),
                             (1050, "item_publication"), (1052, "item_lifecycle")):
            self.assertEqual(owners[(path, line, family)],
                             {"quest.disappearing_npc_cleanup"})
        self.assertTrue(self.routes["quest.durable_offering_submission"]
                        ["current_critical_command_schema"]["schema_2_gameplay_producer_connected"])
        self.assertEqual(self.routes["quest.durable_offering_publication"]["disposition"],
                         "runtime_projection_route")
        self.assertFalse(next(writer for writer in registry["writers"]
                              if writer["id"] == "quest.artifact_turnin")["sites"])

    def test_bandage_consumption_sites_keep_authority_and_live_projection_distinct(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/economy/tradeskill.c"
        owners = {}
        for writer in registry["writers"]:
            for site in writer.get("sites", []):
                if site[0] == path:
                    owners.setdefault(tuple(site), set()).add(writer["id"])
        self.assertEqual(owners[(path, 1222, "economic_submit")],
                         {"tradeskill.bandage_durable_submission"})
        self.assertEqual(owners[(path, 1019, "item_lifecycle")],
                         {"tradeskill.bandage_durable_publication"})
        self.assertEqual(owners[(path, 1238, "item_lifecycle")],
                         {"tradeskill.bandage"})
        self.assertTrue(self.routes["tradeskill.bandage_durable_submission"]
                        ["blocking_policy_after_activation"]["must_block_on_activation"])

    def test_sql_components_do_not_claim_a_playable_root(self) -> None:
        component = self.routes["item.sql_custody_apply"]
        self.assertEqual(component["double_entry_evidence"]["status"],
                         "typed_schema2_item_repository_component_without_gameplay_producer")
        self.assertFalse(component["current_critical_command_schema"]
                         ["schema_2_gameplay_producer_connected"])
        self.assertFalse(component["double_entry_evidence"]["unified_operation_postings_observed"])
        for route_id in ("combat.sql_outcome", "collector.sql_apply", "death.corpse_sql_apply",
                         "death.restitution_sql_apply", "player.death_snapshot_quarantine",
                         "recovery.saved_sql_store", "recovery.saved_sql_delete"):
            route = self.routes[route_id]
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["blocking_policy_after_activation"]["must_block_on_activation"])
            self.assertFalse(route["double_entry_evidence"]["unified_operation_postings_observed"])
        self.assertIn("retains", self.routes["player.death_snapshot_quarantine"]
                      ["native_effects"]["custody_effect"])
        for route_id in ("auction.money_claim_legacy", "auction.money_claim_compensation"):
            self.assertEqual(self.routes[route_id]["disposition"], "dormant_writer_candidate")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_money_helpers_are_linked_without_reviving_legacy_definitions(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["family"] == "money_helper"}
        linked = {tuple(site) for row in registry["writers"] for site in row.get("sites", [])}
        self.assertTrue(current)
        self.assertFalse(current - linked, "review newly detected shared money-helper calls")
        for route_id in ("auction.legacy_definitions", "auction.bid_legacy",
                         "auction.finalize_legacy", "auction.money_claim_legacy",
                         "auction.money_claim_compensation", "boon.legacy_cash_completion"):
            self.assertEqual(self.routes[route_id]["disposition"], "dormant_writer_candidate")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_typed_submit_sites_are_linked_without_counting_builders_as_writers(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["family"] == "economic_submit"}
        linked = {tuple(site) for row in registry["writers"] for site in row.get("sites", [])}
        self.assertTrue(current)
        self.assertFalse(current - linked, "review newly detected typed submit or builder sites")
        for route_id in ("macro.economic_submit_declarations", "coin.command_builder",
                         "currency.command_builder", "item.command_builder"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("player.chaos_starter_bank", "player.chaos_starter_kit",
                         "item.obj_to_char_admission", "gambling.slot_machine",
                         "world_quest.payment", "item.synthetic_submit"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertEqual(self.routes["auction.money_claim_legacy"]["disposition"],
                         "dormant_writer_candidate")

    def test_salvage_separates_candidates_from_live_retirement(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/item/salvage.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path:
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new salvage custody sites")
        self.assertEqual(owners[(path, 32, "item_lifecycle")],
                         {"item.salvage_grant_reject_cleanup"})
        self.assertEqual(owners[(path, 215, "item_publication")],
                         {"item.salvage_input_retirement"})
        for route_id in ("item.salvage_candidate_allocation",
                         "item.salvage_grant_reject_cleanup"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("item.salvage_grant", "item.salvage_input_retirement",
                         "item.salvage_scientific_tools_consumption"):
            route = self.routes[route_id]
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
            self.assertFalse(route["double_entry_evidence"]
                             ["unified_operation_postings_observed"])
        tool = next(row for row in registry["writers"]
                    if row["id"] == "item.salvage_scientific_tools_consumption")
        self.assertEqual(tool["sites"], [])  # Semantic helper call is outside the scanner.
        self.assertIn("vnum_from_inv(ch, crafting_scientific_tools_vnum(), 1)",
                      (ROOT / path).read_text(encoding="utf-8"))

    def test_kingdom_harvest_sites_separate_world_nodes_and_personal_materials(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/kingdom/kingdom_harvest.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path:
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new kingdom harvest item sites")
        self.assertEqual(owners[(path, 1128, "item_publication")], {"kingdom.node_spawn"})
        self.assertEqual(owners[(path, 1942, "item_lifecycle")],
                         {"kingdom.realm_harvest"})
        self.assertEqual(owners[(path, 2154, "item_publication")],
                         {"kingdom.personal_gather"})
        self.assertEqual(owners[(path, 2149, "item_lifecycle")],
                         {"kingdom.material_rejected_candidate"})
        for route_id in ("kingdom.node_candidate_allocation",
                         "kingdom.material_candidate_allocation",
                         "kingdom.material_rejected_candidate"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("kingdom.node_reap", "kingdom.node_periodic_retirement",
                         "kingdom.node_shutdown_unload", "kingdom.node_spawn",
                         "kingdom.realm_harvest", "kingdom.personal_gather"):
            route = self.routes[route_id]
            self.assertEqual(route["disposition"], "runtime_mutation_route")
            self.assertTrue(route["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
            self.assertFalse(route["double_entry_evidence"]
                             ["unified_operation_postings_observed"])

    def test_coin_pile_and_scrap_calls_have_source_classifications(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        sites = {(row["path"], row["line"], row["family"])
                 for row in coverage.load_validator().scan_sources(ROOT)
                 if row["family"] == "item_lifecycle" and
                 any(name in row["excerpt"] for name in
                     ("create_money(", "MakeScrap(", "instantiate_object_template("))}
        linked = {tuple(site) for row in registry["writers"] for site in row.get("sites", [])}
        self.assertTrue(sites)
        self.assertFalse(sites - linked)
        for route_id in ("macro.item_constructor_declarations", "world.object_template_allocation"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        projection = self.routes["recovery.flat_corpse_coin_materialization"]
        self.assertEqual(projection["disposition"], "runtime_projection_route")
        self.assertEqual(projection["blocking_policy_after_activation"]["decision"],
                         "block_until_projection_proof")
        for route_id in ("spell.transfer_wellness_pile", "ship.treasure_chest_coin",
                         "special.golem_shatter_cash", "special.kobold_stone_cash"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_object_command_item_calls_separate_legacy_effects_from_publication(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/cmd/actobj.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        linked = {tuple(site) for row in registry["writers"] for site in row.get("sites", [])}
        self.assertTrue(current)
        self.assertFalse(current - linked)
        for route_id in ("item.legacy_get", "item.legacy_bulk_drop", "item.legacy_drop",
                         "item.legacy_put", "item.legacy_give", "item.drink_consumption",
                         "item.food_consumption", "item.poison_consumption",
                         "staff.nowhere_item_recovery", "item.bulk_transient_drop"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        for route_id in ("item.pet_give_publication", "item.command_publication",
                         "item.empty_publication", "item.weight_relink",
                         "item.equipment_wear", "item.equipment_remove"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        self.assertEqual(self.routes["item.list_foods_debug"]["disposition"],
                         "dormant_writer_candidate")
        self.assertIn("UID", self.routes["item.junk_reward"]["native_effects"]["custody_effect"])
        self.assertEqual(self.routes["macro.money_helper_declarations"]["disposition"],
                         "non_writer_candidate")
        self.assertEqual(self.artifact["counts"]["supplemental_candidates_found"], 0)
        for route_id in ("shop.legacy_transact", "guild.deposit", "guild.withdraw",
                         "ship.cargo_sale", "ship.cargo_purchase", "death.legacy_resurrect_wallet",
                         "travel.ferry_ticket", "item.junk_reward"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_world_handler_item_sites_distinguish_cleanup_projection_and_retirement(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/world/handler.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/world/handler.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new handler item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/world/handler.c", 3385, "item_lifecycle")],
                         {"item.extraction"})
        self.assertEqual(owners[("src/world/handler.c", 4001, "item_publication")],
                         {"death.corpse_compaction_bone_grant"})
        self.assertEqual(owners[("src/world/handler.c", 4370, "item_lifecycle")],
                         {"death.resurrection_money_pile"})
        for route_id in ("item.prototype_weight_probe", "item.creation_candidate_reject",
                         "coin.wallet_pile_stage_cleanup", "death.corpse_compaction_stage_cleanup"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("coin.wallet_pile_publication", "death.corpse_release_live",
                         "death.corpse_raise_recovery_live", "death.corpse_resurrection_item_live",
                         "death.corpse_nested_release_live", "death.corpse_destruction_live",
                         "death.corpse_transient_cleanup", "death.corpse_committed_money_cleanup",
                         "item.pet_teardown_unload"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("item.extraction", "item.legacy_decay", "item.character_teardown",
                         "item.transfer_inventory", "death.corpse_compaction_bone_grant",
                         "death.resurrection_money_pile"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_other_commands_item_sites_keep_foraging_and_steal_fallback_visible(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/cmd/actoth.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/cmd/actoth.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new other-command item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/cmd/actoth.c", 1347, "item_publication")],
                         {"item.forage_food_creation"})
        self.assertEqual(owners[("src/cmd/actoth.c", 3969, "item_publication")],
                         {"item.legacy_steal_fallback"})
        self.assertEqual(self.routes["item.forage_doodle_probe"]["disposition"],
                         "non_writer_candidate")
        for route_id in ("item.quit_direct_drop", "item.old_descend_equipment"):
            self.assertEqual(self.routes[route_id]["disposition"], "dormant_writer_candidate")
        self.assertEqual(self.routes["item.ascension_equipment_relink"]["disposition"],
                         "runtime_projection_route")
        for route_id in ("item.forage_food_creation", "item.forage_tree_creation",
                         "item.legacy_steal_fallback", "item.quaff_consumption",
                         "item.recite_legacy_consumption", "item.donation_transfer",
                         "item.donation_excess_destroy"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_sql_player_item_loads_separate_restore_stage_from_new_stock(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/sql/sql_player.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/sql/sql_player.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new SQL item load sites")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/sql/sql_player.c", 10224, "item_publication")],
                         {"recovery.sql_shopkeeper_catalog"})
        self.assertEqual(owners[("src/sql/sql_player.c", 11364, "item_publication")],
                         {"recovery.sql_saved_item_hydration"})
        for route_id in ("recovery.sql_diff_proto_probe", "recovery.sql_temp_char_cleanup",
                         "recovery.sql_corpse_stage_cleanup",
                         "recovery.sql_shopkeeper_stage_cleanup",
                         "recovery.sql_saved_item_stage_cleanup"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("recovery.sql_player_item_hydration",
                         "recovery.sql_locker_item_hydration",
                         "recovery.sql_private_chest_hydration",
                         "recovery.sql_corpse_hydration",
                         "recovery.sql_saved_item_hydration"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        self.assertEqual(self.routes["recovery.sql_shopkeeper_catalog"]["disposition"],
                         "runtime_mutation_route")
        self.assertIn("does not read obj_uid",
                      self.routes["recovery.sql_shopkeeper_catalog"]["source_classification"])
        self.assertIn("requires a strict positive decimal saved UID",
                      self.routes["recovery.sql_locker_item_hydration"]["source_classification"])
        self.assertIn("strict positive decimal saved UID",
                      self.routes["recovery.sql_player_item_hydration"]["source_classification"])

    def test_zone_reset_item_sites_separate_unpublished_rejection_from_grant(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/world/db.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/world/db.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new zone reset item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        for route_id in ("world.read_object_factory", "world.zone_reset_stage_cleanup"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("world.zone_reset_container_grant", "world.zone_reset_room_grant",
                         "world.zone_reset_npc_grant", "world.zone_reset_npc_equip"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("player held",
                      self.routes["world.zone_reset_container_grant"]["source_classification"])
        self.assertEqual(self.routes["world.zone_reset_equip_relink"]["disposition"],
                         "runtime_projection_route")

    def test_alchemist_item_sites_separate_recipe_probes_from_consumption(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/classes/salchemist.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/classes/salchemist.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new alchemist item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        for route_id in ("item.poison_recipe_probe", "item.encrust_virtual_jewel_stage",
                         "item.fix_material_probe"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("item.poison_mix",
                         "item.encrust_failure_destroy", "item.encrust_transform",
                         "item.fix_material_consumption", "item.smelt_single",
                         "item.smelt_double", "item.npc_alchemist_vial_grant",
                         "item.thrusted_aura_decay", "item.enchant_failure_destroy"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        retired = {row["id"] for row in registry["retired_routes_551_661"]}
        self.assertTrue({"item.potion_mix", "item.potion_ingredients_sink",
                         "item.npc_alchemist_potion_grant", "item.poison_ingredients_sink",
                         "item.encrust_generated_counter_orphan"} <= retired)
        self.assertTrue(retired.isdisjoint(self.routes))
        for route_id in ("item.poison_mix", "item.encrust_transform", "item.craft_submit",
                         "class.drannak_pvp_store", "item.npc_alchemist_vial_grant"):
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "refuse_before_allocation_until_native_source_and_root_exist")

    def test_artifact_item_sites_separate_display_boot_restore_and_live_replacement(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/guild/artifact.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/guild/artifact.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new artifact item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/guild/artifact.c", 883, "item_publication")],
                         {"artifact.ground_restore_unqualified_creation"})
        self.assertEqual(owners[("src/guild/artifact.c", 4973, "item_publication")],
                         {"artifact.npc_restore_unqualified_creation"})
        self.assertEqual(owners[("src/guild/artifact.c", 4265, "item_lifecycle")],
                         {"artifact.swap_replacement"})
        for route_id in ("artifact.cache_display_probe", "artifact.dummy_character_unload",
                         "artifact.swap_second_template_stage", "artifact.fixit_template_probe"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("artifact.ground_restore_unqualified_creation",
                         "artifact.npc_restore_unqualified_creation", "artifact.limbo_salvage",
                         "artifact.poof_retirement", "artifact.war_forced_drop",
                         "artifact.swap_replacement"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("releases it exactly once if present", self.routes["artifact.fixit_template_probe"]
                      ["source_classification"])

    def test_mob_behavior_item_sites_separate_world_creation_from_equipment_relink(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/mob/mobact.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/mob/mobact.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new NPC behavior item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/mob/mobact.c", 1165, "item_publication")],
                         {"mob.corpse_dig_creation"})
        self.assertEqual(self.routes["mob.corpse_dig_creation"]["disposition"],
                         "runtime_mutation_route")
        for route_id in ("mob.thief_weapon_relink", "mob.better_object_relink",
                         "mob.hunter_weapon_relink"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")

    def test_random_zone_item_sites_distinguish_coin_issuance_and_reset(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/world/random.zone.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/world/random.zone.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new random-zone item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/world/random.zone.c", 426, "item_publication")],
                         {"world.random_chest_coin_issue"})
        self.assertEqual(owners[("src/world/random.zone.c", 1427, "item_lifecycle")],
                         {"world.lab_reset_destroy"})
        for route_id in ("world.random_sigil_factory", "world.lab_relic_probe",
                         "world.lab_relic_stage_reject"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        self.assertEqual(self.routes["world.random_disabled_chest_potions"]["disposition"],
                         "dormant_writer_candidate")
        for route_id in ("world.random_chest_coin_issue", "world.random_quest_item_sink",
                         "world.lab_reset_destroy", "world.lab_reset_preserve"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        source = (ROOT / "src/world/random.zone.c").read_text()
        self.assertIn("random_zone_obj = read_object(real_object(3), REAL)", source)
        self.assertIn("random_zone_obj->value[3] = number(1, 5) * i * 5", source)
        self.assertIn("#define VOBJ_COINS 3", (ROOT / "src/world/vnum.obj.h").read_text())

    def test_recovery_item_sites_separate_selected_uid_from_template_regeneration(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        paths = {"src/world/world_recovery_pipeline.c", "src/world/world_recovery_npc_items.c",
                 "src/world/world_singletons.c", "src/flatfile/flatfile_corpse_restore.c"}
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] in paths and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] in paths and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new recovery item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/world/world_recovery_npc_items.c", 101,
                                 "item_publication")], {"world.npc_item_hydration"})
        self.assertEqual(self.routes["world.npc_item_hydration"]["disposition"],
                         "runtime_mutation_route")
        self.assertIn("fresh item UIDs", self.routes["world.npc_item_hydration"]
                      ["source_classification"])
        self.assertEqual(self.routes["world.copyover_item_materialization"]["disposition"],
                         "runtime_projection_route")
        self.assertEqual(self.routes["recovery.flat_room_item_projection"]["disposition"],
                         "runtime_projection_route")
        self.assertEqual(self.routes["recovery.flat_corpse_shell_unqualified"]["disposition"],
                         "runtime_mutation_route")
        self.assertIn("does not select or assign a retained corpse obj_uid",
                      self.routes["recovery.flat_corpse_shell_unqualified"]["source_classification"])
        for route_id in ("world.copyover_item_materialization",
                         "recovery.flat_room_item_projection"):
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("world.npc_item_hydration", "world.shopkeeper_duplicate_transfer",
                         "world.shopkeeper_duplicate_destroy",
                         "world.shopkeeper_produced_stock_grant",
                         "recovery.flat_corpse_shell_unqualified"):
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_corpse_spell_item_sites_keep_legacy_npc_paths_visible(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/magic/spell_corpse_lifecycle.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if (site[0] == "src/magic/spell_corpse_lifecycle.c" and
                        site[2] in ("item_lifecycle", "item_publication")):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new corpse spell item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/magic/spell_corpse_lifecycle.c", 494,
                                 "item_lifecycle")], {"death.legacy_resurrect_wallet"})
        self.assertEqual(owners[("src/magic/spell_corpse_lifecycle.c", 898,
                                 "item_lifecycle")], {"death.legacy_lesser_resurrect_wallet"})
        for route_id in ("death.resurrection_committed_player_drop",
                         "death.resurrection_committed_corpse_cleanup"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("death.unmaking_direct", "death.legacy_resurrect_player_drop",
                         "death.legacy_resurrect_corpse_transfer",
                         "death.legacy_lesser_player_drop",
                         "death.legacy_lesser_corpse_transfer", "death.corpse_portal_move"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])

    def test_enhancement_item_sites_link_fees_to_transforms_and_materials(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/item/enhance.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/item/enhance.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new enhancement item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[("src/item/enhance.c", 283, "item_lifecycle")],
                         {"item.enhance_transform"})
        self.assertEqual(owners[("src/item/enhance.c", 1099, "item_publication")],
                         {"item.thanksgiving_turkey_grant"})
        for route_id in ("item.enhance_base_probe", "item.enhance_material_name_probe",
                         "item.superior_target_probe", "item.mod_enhance_description_probe",
                         "item.enhance_index_probe"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("item.enhance_transform", "item.mod_enhance_material_sink",
                         "item.thanksgiving_turkey_grant", "item.essence_npc_death_grant",
                         "item.npc_reset_material_fallback"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("generated-use record failure", self.routes["item.superior_enhance_fee"]
                      ["source_classification"])
        self.assertIn("changes source affected fields before debiting",
                      self.routes["item.mod_enhance_fee"]["source_classification"])

    def test_legacy_file_item_sites_separate_save_unload_restore_and_dead_code(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == "src/core/files.c" and
                   row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == "src/core/files.c" and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertTrue(current)
        self.assertFalse(current - owners.keys(), "review new legacy file item calls")
        shared_finish = {("src/core/files.c", line, "item_lifecycle")
                         for line in (1744, 1750)}
        self.assertTrue(all(len(owners[site]) == (2 if site in shared_finish else 1)
                            for site in current))
        for site in shared_finish:
            self.assertEqual(owners[site], {"player.flat_terminal_inventory_unload",
                                            "player.sql_terminal_inventory_unload"})
        self.assertEqual(owners[("src/core/files.c", 3690, "item_publication")],
                         {"recovery.legacy_object_restore"})
        for route_id in ("player.object_save_template_probe",
                         "player.single_item_save_template_probe",
                         "player.confiscate_disabled_children", "recovery.pet_disabled_extract"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("player.confiscate_item_dormant", "player.confiscate_all_dormant"):
            self.assertEqual(self.routes[route_id]["disposition"], "dormant_writer_candidate")
        for route_id in ("player.flat_terminal_inventory_unload",
                         "player.sql_terminal_inventory_unload",
                         "recovery.legacy_object_restore", "recovery.single_item_decode",
                         "recovery.pet_save_equipment_relink"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        uid_route = self.routes["player.single_item_uid_assignment"]
        self.assertEqual(uid_route["disposition"], "runtime_mutation_route")
        self.assertTrue(uid_route["blocking_policy_after_activation"]["must_block_on_activation"])
        self.assertIn('persistence_assign_item_uid(obj, "write_one_object")',
                      (ROOT / "src/core/files.c").read_text())

    def test_movement_item_sites_separate_committed_breaks_from_direct_sinks(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/cmd/actmove.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new movement item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 111, "item_lifecycle")],
                         {"movement.key_break_committed_publication"})
        self.assertEqual(owners[(path, 128, "item_lifecycle")],
                         {"movement.key_break_direct"})
        for route_id in ("movement.frost_ice_stage_cleanup",
                         "movement.faerie_reward_candidate_reject"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        for route_id in ("movement.key_break_committed_publication",
                         "movement.drag_room_relink"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("movement.key_break_direct", "movement.frost_ice_grant",
                         "movement.faerie_reward_grant", "movement.faerie_bag_sink",
                         "movement.pick_break_direct"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("missing-UID case can include a PC",
                      self.routes["movement.key_break_direct"]["source_classification"])

    def test_locker_item_sites_separate_saved_custody_from_fixture_lifecycle(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/item/storage_lockers.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new locker item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        def site(family, excerpt):
            matches = [row for row in registry["census"] if row["path"] == path and
                       row["family"] == family and row["excerpt"] == excerpt]
            self.assertEqual(len(matches), 1)
            return (path, matches[0]["line"], family)

        self.assertEqual(owners[site("item_lifecycle", "extract_obj(tmp_object);")],
                         {"locker.reused_room_item_sweep"})
        self.assertEqual(owners[site("item_publication", "obj_to_room(content, room);")],
                         {"locker.chest_content_spill"})
        self.assertEqual(self.routes["locker.access_check_temp_unload"]["disposition"],
                         "non_writer_candidate")
        for route_id in ("locker.chest_item_restore", "locker.public_items_save_relink",
                         "locker.public_items_room_restore"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("locker.leave_corpse_eject", "locker.public_chest_fixture",
                         "locker.chest_content_spill", "locker.chest_fixture_teardown",
                         "locker.private_chest_fixture", "locker.reused_room_item_sweep",
                         "locker.artifact_rejection_return"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        lines = (ROOT / path).read_text(encoding="utf-8").splitlines()
        for route_id, signature in (("locker.chest_fixture_teardown", "LockerChest::~LockerChest"),
                                    ("locker.private_chest_fixture", "PrivateChest::PrivateChest")):
            definitions = self.routes[route_id]["source"]["definition_lines"]
            self.assertTrue(definitions)
            self.assertTrue(all(signature in lines[line - 1] for line in definitions))

    def test_new_command_item_sites_include_in_place_key_changes(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/cmd/actnew.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new command item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 3968, "item_publication")],
                         {"combat.throw_potion_cast_sink"})
        self.assertEqual(self.routes["combat.disarm_slot_relink"]["disposition"],
                         "runtime_projection_route")
        for route_id in ("craft.lock_template_sink", "craft.make_lock_payload",
                         "craft.make_key_payload", "craft.make_key_pick_break",
                         "combat.throw_potion_slip_sink", "combat.throw_potion_slip_room",
                         "combat.throw_potion_cast_sink"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertEqual(self.routes["craft.make_lock_payload"]["source"]["definition_lines"],
                         [3096])
        self.assertEqual(self.routes["craft.make_key_payload"]["source"]["definition_lines"],
                         [3178])
        self.assertIn("early return for over-level magic",
                      self.routes["combat.throw_potion_cast_sink"]["source_classification"])

    def test_conjuration_item_sites_separate_typed_pc_grants_from_fallback(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/magic/spell_conjuration.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new conjuration item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 2279, "item_lifecycle")],
                         {"spell.snakes_committed_arrow_cleanup"})
        for route_id in ("spell.room_creation_rejected_stage",
                         "spell.player_creation_rejected_stage"):
            self.assertEqual(self.routes[route_id]["disposition"], "non_writer_candidate")
        self.assertEqual(self.routes["spell.snakes_committed_arrow_cleanup"]["disposition"],
                         "runtime_projection_route")
        for route_id in ("spell.avatar_orb_sink", "spell.avatar_orb_grant",
                         "spell.minor_creation_fallback", "spell.flame_blade_grant",
                         "spell.shield_grant", "spell.food_grant",
                         "spell.insect_mandrake_fallback_sink", "spell.doom_blade_grant",
                         "spell.snakes_direct_arrow_sink"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertEqual(self.routes["spell.flame_blade_grant"]["current_critical_command_schema"]
                         ["route_mode"],
                         "typed_schema_2_for_active_pc_npc_refused_direct_legacy_when_inactive")
        self.assertIn("remaining detached arrows can be left unconsumed",
                      self.routes["spell.snakes_direct_arrow_sink"]["source_classification"])

    def test_ranged_item_sites_keep_ammunition_payload_and_uid_together(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/combat/range.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new ranged item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 924, "item_lifecycle")], {"item.scrap"})
        self.assertEqual(owners[(path, 1552, "item_lifecycle")],
                         {"range.load_weapon_ammunition"})
        self.assertEqual(self.routes["range.gather_quiver_relink"]["disposition"],
                         "runtime_projection_route")
        for route_id in ("range.gather_arrow_transfer", "range.fire_missile_transfer",
                         "range.throw_weapon_drop", "range.load_weapon_ammunition"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("partially consumed stack keeps its UID",
                      self.routes["range.load_weapon_ammunition"]["source_classification"])

    def test_guildhall_fixture_sites_record_unretired_detach(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/guild/guildhall_rooms.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new guildhall fixture calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 682, "item_publication")],
                         {"guildhall.town_portal_fixture_detach"})
        names = ("entrance_door", "inn_board", "heartstone", "window", "fountain",
                 "bank_counter", "town_portal", "library_tome", "cargo_board")
        for name in names:
            grant = self.routes[f"guildhall.{name}_fixture_grant"]
            detach = self.routes[f"guildhall.{name}_fixture_detach"]
            self.assertEqual(grant["disposition"], "runtime_mutation_route")
            self.assertEqual(detach["disposition"], "runtime_mutation_route")
            self.assertTrue(grant["blocking_policy_after_activation"]["must_block_on_activation"])
            self.assertTrue(detach["blocking_policy_after_activation"]["must_block_on_activation"])
            self.assertIn("without extracting or retiring", detach["source_classification"])
            self.assertTrue(grant["source"]["definition_lines"])
            self.assertTrue(detach["source"]["definition_lines"])

    def test_necromancy_item_sites_separate_committed_raise_from_clone_and_fallback(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/classes/necromancy.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new necromancy item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 1294, "item_publication")],
                         {"death.saved_corpse_clone"})
        self.assertEqual(owners[(path, 1501, "item_lifecycle")],
                         {"death.raise_committed_publication"})
        self.assertEqual(self.routes["death.corpseform_disabled"]["disposition"],
                         "non_writer_candidate")
        for route_id in ("death.raise_nested_exclusion_cleanup",
                         "death.raise_committed_publication"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("death.saved_corpse_clone", "death.saved_corpse_timed_cleanup",
                         "death.exhume_corpse_grant", "death.summon_host_corpse_grant",
                         "death.wall_of_bones_direct", "death.compact_corpse_direct",
                         "death.legacy_raise_item_recipient"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("recursively clones every contained item",
                      self.routes["death.saved_corpse_clone"]["source_classification"])
        self.assertIn("if pile allocation fails the corpse remains emptied",
                      self.routes["death.compact_corpse_direct"]["source_classification"])

    def test_heavens_special_item_sites_keep_rewards_sinks_and_dead_code_distinct(self) -> None:
        registry = json.loads((ROOT / "docs/persistence/economy_accounting/writers.json").read_text())
        path = "src/specs/specs.heavens.c"
        current = {(row["path"], row["line"], row["family"])
                   for row in coverage.load_validator().scan_sources(ROOT)
                   if row["path"] == path and row["family"] in ("item_lifecycle", "item_publication")}
        owners = {}
        for route in registry["writers"]:
            for site in route.get("sites", []):
                if site[0] == path and site[2] in ("item_lifecycle", "item_publication"):
                    owners.setdefault(tuple(site), set()).add(route["id"])
        self.assertEqual(current, owners.keys(), "review new Heavens special item calls")
        self.assertTrue(all(len(owners[site]) == 1 for site in current))
        self.assertEqual(owners[(path, 5840, "item_publication")],
                         {"special.treasure_chest_detach"})
        self.assertEqual(owners[(path, 5727, "item_lifecycle")],
                         {"gambling.slot_coupon_grant"})
        self.assertEqual(self.routes["special.flying_citadel_unreachable_move"]["disposition"],
                         "non_writer_candidate")
        for route_id in ("special.disarm_pick_gloves_slot_relink",
                         "artifact.good_evil_sword_slot_relink"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_projection_route")
            self.assertEqual(self.routes[route_id]["blocking_policy_after_activation"]["decision"],
                             "block_until_projection_proof")
        for route_id in ("special.treasure_chest_potion_grant",
                         "special.treasure_chest_detach", "gambling.slot_coupon_grant",
                         "artifact.monolith_absorb", "artifact.holy_weapon_owner_transfer",
                         "death.dracolich_death_drop"):
            self.assertEqual(self.routes[route_id]["disposition"], "runtime_mutation_route")
            self.assertTrue(self.routes[route_id]["blocking_policy_after_activation"]
                            ["must_block_on_activation"])
        self.assertIn("never extracted",
                      self.routes["special.treasure_chest_detach"]["source_classification"])
        self.assertIn("item grants precede",
                      self.routes["gambling.slot_coupon_grant"]["native_effects"]["holding_effect"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
