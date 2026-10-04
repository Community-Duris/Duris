#!/usr/bin/env python3
"""Build a source-anchored economy/item writer coverage matrix from the draft registry."""
from __future__ import annotations

import argparse
import ast
from collections import Counter
from functools import lru_cache
import importlib.util
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "docs/persistence/economy_accounting/writers.json"
OUTPUT = ROOT / "docs/persistence/economy_accounting/writer_coverage_matrix.json"
VALIDATOR = ROOT / "scripts/validate_economy_accounting.py"

# The earlier rows use two descriptive Python anchors and one field name rather
# than executable function names. Correct the source locator without rewriting
# that draft registry. The auction settlement wrapper is `finalize_auction`.
FUNCTION_FIXES = {
    "auction.settle": "finalize_auction",
    "backup.capture": "backup",
    "restore.qualification": "restore",
}
SOURCE_FILE_FIXES = {
    "special.money_changer": "src/economy/currency_exchange_proc.c",
    "special.smelter": "src/specs/specs.alatorin.c",
    "special.witch_doctor": "src/specs/specs.heavens.c",
    "special.llyren": "src/specs/specs.clfhaven.c",
    "staff.zone_reset": "src/cmd/staff_world_control.c",
}

OFFLINE_WRITERS = {
    "repair.player_item_payload": "Reconstructs missing physical payload for an existing selected player UID under an approved offline plan and runtime exclusion. Custody, root, parent, revision and supply remain unchanged; this is neither issuance nor an economic adjustment.",
    "import.legacy_dump": "Guarded local import replaces native rows from a legacy dump; it is an offline migration boundary, not a gameplay credit.",
    "import.currency_baselines": "Creates opening wallet/bank baseline witnesses for an authorized cutover; it must not alter live holdings or masquerade as issuance.",
    "restore.qualification": "Restores into an isolated qualification target; it is not permission to promote that target to live authority.",
}
DORMANT_WRITERS = {
    "currency.bank_single_projection": "The single-denomination bank publisher has no in-tree caller. If revived, it would change a live PC projection without a revision fence; require a committed bank identity/revision or refuse.",
    "currency.compat_sql_bank": "Direct SQL writer definition has no in-tree caller outside its own wrapper path; it is not a current gameplay route.",
    "auction.legacy_definitions": "Legacy offer mutator is definition-only in the current tree; the current offer path is separately routed.",
    "auction.finalize_legacy": "Legacy settlement mutator is definition-only in the current tree; no in-tree callsite found.",
    "auction.bid_legacy": "Legacy bid mutator is definition-only in the current tree; no in-tree callsite found.",
    "auction.money_claim_legacy": "Legacy pickup debits and refunds pending claims around a separate wallet credit; no in-tree caller was found.",
    "auction.money_claim_compensation": "The rejected-credit callback is referenced only by the definition-only legacy pickup.",
    "boon.legacy_cash_completion": "Legacy cash boon completion is definition-only in the current tree; no in-tree callsite found.",
    "item.list_foods_debug": "The template-scanning debug function has no in-tree caller; keep its object allocations unreachable.",
    "item.old_descend_equipment": "do_old_descend has no in-tree caller; the registered command dispatches do_descend instead.",
    "item.quit_direct_drop": "do_quit has no in-tree caller; CMD_QUIT is registered to do_camp. Its direct drop/extract branch must stay unreachable or be fenced before revival.",
    "world.random_disabled_chest_potions": "The random-zone chest level-potion branch is behind a literal false condition; enabling it requires a typed creation source and chest owner root.",
    "player.confiscate_item_dormant": "Compiled rent confiscation helper has no in-tree callsite; its direct child movement and item extraction require a debt root if revived.",
    "player.confiscate_all_dormant": "Compiled bulk rent confiscation helper has no in-tree callsite; its direct inventory extraction requires bounded UID retirement if revived.",
}
NON_WRITERS = {
    'recovery.sql_ordinary_drop_receipt_observation': 'Reads exact committed inbox, historical economic root/payload and outbox inside caller-owned native locks; no native mutation, missing-command apply or transaction/ACK ownership.',
    'recovery.runtime_owner_revision_observation': 'Reads only an existing serialized owner-cache revision without inserting or hydrating missing metadata; no native custody or economic effect.',
    'recovery.ordinary_drop_graph_observation': 'Observes exact original SQL-drop receipt and current epoch/season/custody/literal graph together with complete physical/runtime placement; no construction, repair, enrollment, issuance/destruction or publication ACK.',
    "recovery.inert_literal_staging": "Allocates only private discard-only literal memory with a retained UID; no UID issuance, native holdings/custody, global object-list/index-count changes or registry admission. Final proof/enrollment is a separate owner.",
    "recovery.inert_literal_cleanup": "Releases only unpublished stage memory/strings/descriptions without extraction, effect/procedure/event callbacks or native/runtime custody retirement.",
    "coin.retained_pile_rendering": "Compares denomination-dependent text and weight on a stack-local zero-add renderer; it neither admits a UID nor changes a native holding.",
    "recovery.sql_exact_room_stage_cleanup": "Clears original UIDs only in the detached exact room tree before extracting its rejected root; durable payload and custody remain retained. Cleanup is not retirement.",
    "macro.checked_item_publication_declaration": "The checked obj_to_char prototype declares an interface; only its implementation and callers can publish a live item.",
    "account.cleanup_temp_char": "restoreCharOnly loads a temporary PC solely for account/browser display; every in-tree caller frees that temporary character graph, so extracting its copied items is not a durable custody retirement.",
    "player.new_character_zero": "init_char assigns an initial zero wallet to a newly allocated PC before its first durable baseline; no existing holding is retired.",
    "morph.new_body_zero": "morph clears the cash on a freshly read NPC body before publishing it as the player morph; the original PC wallet remains separate.",
    "training.dummy_zero": "training_dummy_create freshly reads a template and calls training_dummy_apply_profile before room placement; the template cash is discarded before admission.",
    "nexus.guardian_zero": "load_guardian clears cash on a freshly read guardian before room placement; no earlier admitted guardian balance is changed.",
    "nexus.sage_zero": "load_sage clears cash on a freshly read sage before room placement; no earlier admitted sage balance is changed.",
    "guildhall.golem_zero": "EntranceRoom::init clears cash on a freshly read golem before room placement; no earlier admitted golem balance is changed.",
    "spell.summoned_beast_zero": "summon_beast_common clears a freshly read beast before setup_pet and room placement; no existing mob balance is changed.",
    "special.animated_skeleton_zero": "animated_skeleton clears two freshly read undead clones before room placement; the original skeleton cash is not changed at these sites.",
    "macro.clear_money_definition": "CLEAR_MONEY is a preprocessor definition, not an executed writer by itself; its three call sites have separate route rows.",
    "macro.add_coins_declaration": "The add_coins prototype declares a helper; actual pile effects are classified at its definition and callers.",
    "macro.difficulty_scale_declaration": "The two difficulty_scale_coins prototypes declare helpers; conversion and definitions are classified separately.",
    "macro.money_helper_declarations": "The transact, ADD_MONEY, SUB_MONEY, and insert_money_pickup declarations have no executable effect; helper definitions and calls have separate routes.",
    "macro.economic_submit_declarations": "The command-builder and typed-submit prototypes declare interfaces; executable definitions and gameplay callers have separate routes.",
    "coin.command_builder": "Builds and validates a frozen coin command in memory; no native wallet, pile, or UID changes occur here.",
    "currency.command_builder": "Builds a frozen currency command in memory; no native wallet or bank changes occur here.",
    "item.command_builder": "Builds a frozen item transfer command in memory; no native UID custody changes occur here.",
    "macro.item_constructor_declarations": "MakeScrap, create_money, and instantiate_object_template prototypes declare functions; no item is admitted at these sites.",
    "world.object_template_allocation": "instantiate_object_template builds an unpublished object; admission occurs only when a caller assigns a live owner and source-qualified UID.",
    "coin.pile_description_staging": "prepare_coin_pile rebuilds text on a temporary rendered object with zero added coins; no admitted pile value changes at this site.",
    "player.bank_load_result": "Reads a bank row into the player-load result buffer; it neither changes the native account_banks row nor credits a live wallet.",
    "ship.zero_initialization": "A newly allocated ship starts with an empty coffer; no pre-existing holding is debited or credited.",
    "lifecycle.sql_soft_delete": "Visibility/tombstone update only; the source note explicitly does not retire holdings or custody.",
    "lifecycle.quiescence": "Drain/fence preflight only; it does not change an economic holding or item owner.",
    "backup.capture": "Copies authorities and journals to backup; it does not mutate native holdings or custody.",
    "backup.retention": "Prunes retained backup generations, not live economy or item authority.",
    "restore.erasure_gate": "Read-only tombstone preflight; no native authority write.",
    "lifecycle.archive": "Execution authorization scaffold; canonical destructive execution is disabled.",
    "lifecycle.export": "Export readiness validation only; no native holding/custody write.",
    "lifecycle.erasure": "Erasure readiness validation only; no canonical deletion execution.",
    "item.arrow_reset_temporary": "Prototype-object/text-pointer handling only; no item transfer, durable UID custody, or economic holding change.",
    "kingdom.workshop_props": "`prop_vnum` is a data member used to materialize room props, not a source function or player-held asset route.",
    "kingdom.node_candidate_allocation": "The reload sweep's read_object creates an unpublished node candidate; room admission occurs at the separate obj_to_room site.",
    "kingdom.material_candidate_allocation": "Personal gathering reads a material template into an unowned candidate; only the later bag publication admits its UID.",
    "kingdom.material_rejected_candidate": "A bag-capacity rejection extracts only the fresh unowned material candidate before publication; the spent node charge is a separate writer effect.",
    "item.prototype_weight_probe": "obj_prototype_weight reads and frees a fresh prototype only to inspect its weight; the object never receives a live owner.",
    "item.creation_candidate_reject": "obj_to_char frees only a still-unowned creation candidate after admission fails; an object with an authoritative ownership row is preserved for recovery.",
    "item.salvage_candidate_allocation": "Salvage read_object calls allocate unpublished material, essence and recipe candidates; the grant helper is the separate admission boundary.",
    "item.salvage_grant_reject_cleanup": "A rejected salvage grant frees its still-unowned candidate before any accepted ownership publication; this is not retirement of the carried source item.",
    "coin.wallet_pile_stage_cleanup": "The rejected wallet-to-pile command frees its provisional pile before publication; the completion callback frees only a NOWHERE pile on rejection or when the committed owner is offline.",
    "death.corpse_compaction_stage_cleanup": "Compaction allocates a bone pile in NOWHERE before the corpse release; rejected/stale submissions free that unpublished pile.",
    "item.forage_doodle_probe": "The scribing/memorizing forage joke reads a fresh template solely for a message, then extracts it before any live owner is assigned.",
    "recovery.sql_diff_proto_probe": "The SQL item diff formatter reads and frees a fresh prototype for comparison; it never publishes that object.",
    "recovery.sql_temp_char_cleanup": "Migration frees a temporary, unpublished character graph after conversion; this does not retire selected live custody.",
    "recovery.sql_corpse_stage_cleanup": "A corpse or item rejected by owner/hydration checks is extracted before room publication; no selected custody row is retired.",
    "recovery.sql_shopkeeper_stage_cleanup": "Failed shopkeeper catalog restore frees only the staged NPC/item graph before publication.",
    "recovery.sql_saved_item_stage_cleanup": "Rejected saved-item materialization is freed before publication or after a failed room placement; retained source rows remain authoritative.",
    "world.read_object_factory": "read_object instantiates a template into NOWHERE; durable item admission occurs only in a caller with a source and selected owner.",
    "world.zone_reset_stage_cleanup": "Reset branches free freshly allocated objects rejected by artifact, chance, destination or equipment checks before any live owner is assigned.",
    "item.poison_recipe_probe": "Poison recipe display reads and frees sample ingredient/vial templates without giving them to a character.",
    "item.encrust_virtual_jewel_stage": "Virtual Chaos-pouch Encrust allocates and discards a temporary recipe descriptor; genuine physical input retirement and retained-pouch counter changes belong to the typed craft operation.",
    "item.craft_rejected_stage_cleanup": "Frees only detached provisional craft outputs after rejection; admitted input custody is unchanged.",
    "recovery.sql_player_runtime_rejected_stage": "Discards this failed load attempt's provisional player graph without retiring durable custody.",
    "item.fix_material_probe": "Fix reads and frees a sample material template to describe the required component; the carried component is consumed separately.",
    "artifact.cache_display_probe": "Artifact cache rendering reads and frees fresh templates for names; it never publishes their UIDs.",
    "artifact.flat_list_display_probe": "Flat-file artifact listing reads and frees fresh templates for names; loaded dummy characters are separately unloaded.",
    "artifact.ground_restore_rejected_stage": "A freshly instantiated ground artifact is freed before placement when the saved room is invalid.",
    "artifact.dummy_character_unload": "All in-tree nuke_eq calls pass loaded dummy characters; their temporary equipment and inventory graph is unloaded without retiring selected durable custody.",
    "artifact.files_import_template_stage": "Import instantiates a provisional artifact and frees it when no owner is selected or no super grant is requested.",
    "artifact.clear_template_probe": "Staff artifact clear uses a fresh template only to name the artifact; its separate tracking-row deletion is not an item retirement at this site.",
    "artifact.poof_template_probe": "Staff artifact poof uses a fresh template only to validate and display the vnum before selecting the real artifact.",
    "artifact.timer_template_probe": "Staff timer adjustment reads a fresh template only for validation/name display; the non-artifact return currently leaks that provisional object.",
    "artifact.swap_first_template_probe": "Staff swap reads and frees a provisional copy of the old vnum before looking up the admitted live artifact.",
    "artifact.swap_second_template_stage": "Staff swap instantiates a provisional replacement before all checks; several early returns leave it unowned in the global object list.",
    "artifact.bind_template_probe": "Periodic binding maintenance reads and frees a fresh template for logging, not item custody.",
    "artifact.fixit_template_probe": "Binding repair keeps provisional templates alive through reporting and extracts each existing template once afterward.",
    "artifact.npc_restore_rejected_stage": "A freshly instantiated NPC artifact is freed before placement when its saved mob vnum has no matching live mob.",
    "artifact.player_display_probe": "Staff player artifact listings read and free fresh templates for display only.",
    "world.random_sigil_factory": "create_sigil returns a modified but unpublished template; its caller supplies the room owner and source event.",
    "world.lab_relic_probe": "Lab creation reads and frees a relic template only to check its vnum against artifact tracking.",
    "world.lab_relic_stage_reject": "Lab creation frees a fresh relic candidate when artifact tracking already claims that vnum, before an NPC owner is assigned.",
    "world.npc_recovery_candidate_stage": "NPC recovery frees fresh reset-template candidates rejected by artifact ownership, respawn or load checks before any NPC owner is assigned.",
    "world.copyover_item_candidate_stage": "Copyover reads a provisional template and replaces its temporary UID with the saved UID before any room or parent publication.",
    "world.copyover_item_rollback": "Copyover removes only this attempt’s staged or partly published object graph after a failed recovery plan; native custody is not retired.",
    "world.copyover_item_stage_cleanup": "Copyover frees detached or partly linked objects when one snapshot graph cannot be materialized; selected custody remains unchanged.",
    "recovery.flat_corpse_stage_cleanup": "Flat-file corpse/room restore frees staged candidates or rolls back a failed boot publication without retiring saved custody rows.",
    "recovery.flat_corpse_materialization_cleanup": "Flat-file corpse materialization frees detached candidates after prototype, money or lifecycle hydration failure.",
    "recovery.flat_room_materialization_cleanup": "Flat-file room materialization frees detached item candidates after coin-pile allocation failure.",
    "recovery.flat_catalog_publish_rollback": "Flat-file catalog restore frees this attempt’s staged objects after allocation or publication failure, leaving saved UID authority intact.",
    "item.enhance_base_probe": "Enhancement reads and frees a fresh prototype only to compare its unmodified stat modifier.",
    "item.enhance_material_name_probe": "Enhancement reads and frees a fresh material template only to display its name.",
    "item.superior_target_probe": "Superior enhancement reads and frees a fresh target template only to calculate material requirements.",
    "item.mod_enhance_description_probe": "Modifier enhancement reads and frees a fresh prototype only to rebuild the retained source item’s description.",
    "item.enhance_index_probe": "Boot enhancement indexing reads and frees prototype objects without publishing any of them.",
    "player.object_save_template_probe": "Recursive item serialization reads and frees a fresh prototype only to compute differences from a retained object.",
    "player.single_item_save_template_probe": "Single-item serialization reads and frees a fresh comparison prototype; persistent UID assignment to the passed item is classified separately.",
    "player.confiscate_disabled_children": "The second confiscation branch’s child-relocation code is enclosed in #if 0 and has no executable effect.",
    "recovery.pet_disabled_extract": "The pet save extraction loop is enclosed in #if 0; the active branch re-equips the pet inventory.",
    "movement.frost_ice_stage_cleanup": "make_ice frees a fresh ice candidate only when neither the current nor saved room is valid, before any room publication.",
    "movement.faerie_reward_candidate_reject": "Random bag reward selection frees freshly instantiated templates rejected by wear, artifact, rarity or value checks before player publication.",
    "locker.access_check_temp_unload": "Racewar access comparison unloads item copies on a temporary restored character; selected native locker/player custody is not retired.",
    "spell.room_creation_rejected_stage": "Failed typed room creation frees only a still-unowned NOWHERE candidate before publication.",
    "spell.player_creation_rejected_stage": "Failed typed player creation frees only a still-unowned NOWHERE candidate before publication.",
    "death.corpseform_disabled": "spell_corpseform returns unconditionally with a disabled message before its corpse child movement and extraction body.",
    "special.flying_citadel_unreachable_move": "flying_citadel returns FALSE unconditionally before the room-to-room object movement; the two calls cannot execute in this build.",
}
PROJECTION_ROUTES = {
    "coin.retained_room_projection": "Publishes a committed original-UID ordinary-room pile with retained exact literal/custody/result checks. Uncertain handler/materializer stages remain held; no second accounting root is created.",
    "recovery.sql_exact_room_hydration": "Projects a complete literal graph after successful schema-2 provenance, current season, UID/revision/topology and exact staged-byte checks; no accounting root is created.",
    "recovery.sql_exact_room_placement": "Installs retained placement without replaying decay, falling, redirection or gameplay drop effects; no new custody or issuance is authorized.",
    "quest.durable_offering_publication": "Removes the live offering objects only after the committed item-destruction result is checked; it must retain a recoverable quest reward obligation.",
    "currency.bank_live_projection": "Publishes validated bank authority into retained endpoint and connected shared-account player views; no account_banks row or source balance changes here.",
    "currency.wallet_live_projection": "Publishes a committed wallet result or validated stale authority to live PC and GMCP state with a monotonic revision check; no new native value is created here.",
    "player.load_economy_projection": "Materializes wallet and bank vectors from the validated player-load result into a newly loaded PC; no new native value is created here.",
    "player.flatfile_baseline_projection": "After a new-player flat-file baseline is committed and read back, copies its wallet/bank revisions and denominations into the live PC; no second credit is created here.",
    "player.legacy_flatfile_load": "Parses a legacy character file into a PC projection. This is an authorized load only when that file is the selected, complete native source for the active epoch.",
    "player.sql_status_projection": "Loads SQL player wallet and initializes the bank display to zero pending a separate account-bank load; publication requires one complete selected-authority result.",
    "player.sql_bank_live_load": "Loads an account-bank row into a PC, but clears the live bank first and one login caller ignores failure; publication requires a successful fenced load.",
    "recovery.copyover_npc_gold_projection": "Restores the legacy copyover gold field after generated-NPC wallet decode; both sources must agree before publication.",
    "ship.hydration": "Restores the retained ship coffer value from a flat-file record into runtime state; it is not new issuance.",
    "ship.sql_hydration": "Restores the retained ship coffer value from a SQL row into runtime state; it is not new issuance.",
    "world.generated_npc_hydration": "Recovery projection of encoded NPC state; never fresh issuance.",
    "world.copyover_item_materialization": "Projects exact saved UIDs and parent/room ownership from a selected copyover snapshot after authoritative graph validation.",
    "death.resurrection_publication": "Publishes the already-committed corpse lifecycle result; not a second custody/economic commit.",
    "recovery.pet_hydration": "Projects saved durable pet identity and inventory into runtime state; not item creation.",
    "recovery.flat_corpse_coin_materialization": "Projects an existing flatfile corpse/room coin balance into a runtime pile; no new issuance is authorized.",
    "recovery.flat_shopkeeper_cash_materialization": "Projects retained flatfile shop cash into a detached keeper before publication; no trade or new coin issuance occurs here.",
    "recovery.sql_shopkeeper_cash_materialization": "Projects retained SQL shop cash into a staged keeper before publication; no trade or new coin issuance occurs here.",
    "item.pet_give_publication": "Projects a committed player/pet item handoff into the live object graph; stale live extraction does not destroy the durable UID.",
    "item.command_publication": "Projects committed get, drop and give handoffs into live object lists; no second owner event is authorized.",
    "item.empty_publication": "Projects a committed bulk container empty result or restores stale live topology; no new owner event is authorized.",
    "item.weight_relink": "Temporarily relinks one item's runtime location while changing weight; the selected durable owner must remain unchanged.",
    "item.equipment_wear": "Projects carried-to-equipped slot changes under the same player owner; selected slot state must be retained.",
    "item.equipment_remove": "Projects equipped-to-carried slot changes under the same player owner; selected slot state must be retained.",
    "coin.wallet_pile_publication": "The coin transfer callback applies the committed pile owner result before placing the live pile in the character inventory.",
    "death.corpse_release_live": "Projects a committed corpse release into room item lists, after validating the result and applying runtime custody changes.",
    "death.corpse_raise_recovery_live": "Projects recovered items and removes the stale corpse after an already committed raise; money piles are discarded because the wallet transaction consumed them.",
    "death.corpse_resurrection_item_live": "Projects a committed resurrection item drop after matching actor, UID and live topology.",
    "death.corpse_nested_release_live": "Projects committed corpse content movement into the enclosing container and removes the old corpse.",
    "death.corpse_destruction_live": "Removes a live corpse only after the durable destruction result and runtime custody checks succeed.",
    "death.corpse_transient_cleanup": "Removes transient corpse children after a committed release and its discarded UID result are applied.",
    "death.corpse_committed_money_cleanup": "Removes old coin and transient copies only after the corpse wallet/custody result consumes them.",
    "death.resurrection_committed_player_drop": "Publishes the already committed PC resurrection disposition of target inventory and equipment; transient retirement must appear in that result.",
    "death.resurrection_committed_corpse_cleanup": "Publishes the already committed corpse content return, spent coin-pile cleanup and corpse retirement.",
    "item.pet_teardown_unload": "Unloads live pet objects whose durable owner row remains the pet or player; no destruction event is authorized.",
    "item.ascension_equipment_relink": "After committed ascension, worn items are moved to the same player's carrying list; no new owner event is authorized.",
    "item.pet_hidden_equipment_relink": "Moves hidden NPC helper gear from worn to carried on the same player-owned pet, retaining its UID and child graph; no player grant or retirement is authorized. Selected-backend custody and save/publication proof remain unqualified.",
    "recovery.sql_player_item_hydration": "Restores player_items only after validating a strict positive decimal saved UID and matching the player owner before prototype allocation; unknown templates can still leave a partial inventory.",
    "recovery.sql_locker_item_hydration": "Restores locker_items only after validating a strict positive decimal saved UID and matching its locker/chest owner before prototype allocation; invalid rows are retained and skipped.",
    "recovery.sql_private_chest_hydration": "Restores a private-chest item only after validating a strict positive decimal saved UID and matching its locker/chest owner before prototype allocation, then publishes it into the chest.",
    "recovery.sql_corpse_hydration": "Restores corpse items and a room corpse from SQL after corpse lifecycle hydration and item owner checks.",
    "recovery.sql_saved_item_hydration": "Restores a saved room object graph after owner, source-row and duplicate UID checks; source handoff retirement is separately acknowledged.",
    "world.zone_reset_equip_relink": "Reset replaces an occupied NPC equipment slot by moving its previous item into the same NPC carrying list.",
    "recovery.flat_room_item_projection": "Publishes retained flat-file room-item UIDs and owner revision from the selected room ownership catalog.",
    "player.flat_terminal_inventory_unload": "After flat-file terminal save, unloads only the live inventory graph while selected saved item custody remains authoritative.",
    "player.sql_terminal_inventory_unload": "SQL player save temporarily unequips items and later restores them or unloads the live graph only after terminal save succeeds.",
    "recovery.legacy_object_restore": "Projects saved player/corpse/container items from a selected legacy record; missing saved UIDs must not become fresh active-epoch identities.",
    "recovery.single_item_decode": "Decodes a saved item and restores its UID only when the serialized UID flag exists; publication needs selected identity proof.",
    "recovery.pet_save_equipment_relink": "Pet serialization temporarily unequips and then re-equips the same items under the same pet owner.",
    "movement.key_break_committed_publication": "The callback removes the exact live key only after the typed item destruction command committed; no second custody event is authorized.",
    "movement.drag_room_relink": "Dragging relinks the same room object after a movement command; an admitted UID retains its identity while the room owner changes.",
    "locker.chest_item_restore": "Places a loaded retained locker item into its display chest without creating a new UID or changing selected locker ownership.",
    "locker.public_items_save_relink": "Stages public locker items on the temporary locker character for save while selected durable locker custody should remain unchanged.",
    "locker.public_items_room_restore": "Projects saved locker items into chests or the room after load, sort or rollback; retained UID and coin-pile identity must match selected authority.",
    "combat.disarm_slot_relink": "Moves a worn weapon into the same owner’s inventory after combat disarm or fumble; no owner event is intended.",
    "spell.snakes_committed_arrow_cleanup": "Removes live arrow copies after the exact typed batch-retirement result commits; no second owner event is authorized.",
    "range.gather_quiver_relink": "Temporarily unequips and re-equips the same quiver under the same player owner and slot during gather.",
    "death.raise_nested_exclusion_cleanup": "After a committed raise, removes live transient or consumed coin-pile copies listed in the durable result; no second retirement is authorized.",
    "death.raise_committed_publication": "Publishes the committed corpse raise by moving retained children, cleaning consumed copies and removing the old corpse.",
    "mob.thief_weapon_relink": "NPC thief tactics move an existing weapon between carrying and equipment slots under the same NPC owner.",
    "mob.better_object_relink": "NPC item ranking moves displaced equipment into the same NPC inventory before wearing a replacement.",
    "mob.hunter_weapon_relink": "NPC hunt tactics move a carried backstab weapon into a slot and return displaced gear to the same NPC inventory.",
    "special.disarm_pick_gloves_slot_relink": "The gloves move a struck character's equipped weapon to the same character's carrying list without changing its owner.",
    "artifact.good_evil_sword_slot_relink": "The sword shifts other equipment to the same owner's inventory and equips itself in the primary weapon slot.",
}

# Current gameplay commands still build schema 1. The named families below have
# a source-backed typed critical-command path, sometimes alongside direct legacy
# effects. Everything else is direct/legacy, projection, or an offline tool.
SCHEMA1_IDS = {
    "item.poison_mix", "item.encrust_transform", "item.encrust_failure_destroy",
    "class.drannak_pvp_store", "item.craft_submit", "item.craft_publication",
    "item.sql_craft_output_component",
    "currency.coin_steal",
    "ship.coffer_claim", "ship.insurance_fallback",
    "special.money_changer", "special.smelter", "special.rentacleric",
    "special.witch_doctor", "special.llyren", "quest.reward",
    "quest.requirements", "world_quest.full_reward", "world_quest.kill_reward",
    "achievement.reward", "death.wallet_disposition", "death.corpse_creation",
    "death.resurrection_publication", "currency.card_game_payout",
    "staff.load", "staff.storage_repair", "currency.wallet_credit",
    "currency.credit_pending_claim", "currency.wallet_debit",
    "currency.bank_payment", "currency.bank_reward", "currency.deposit",
    "currency.withdraw", "currency.split", "currency.identify",
    "currency.sql_apply", "currency.flat_apply", "coin.player_give",
    "coin.container_put", "coin.pile_get", "coin.floor_drop", "coin.npc_give",
    "coin.compensation", "coin.flat_apply", "item.command_movement",
    "item.bulk_movement", "item.movement_submit", "item.trusted_steal",
    "shop.buy_existing", "shop.buy_produced", "shop.sell_store",
    "shop.sell_destroy", "shop.stock_cleanup", "shop.keeper_cash",
    "shop.repair", "shop.flat_apply", "collector.collect",
    "collector.purchase", "collector.expire", "collector.cancel",
    "collector.boundary_children", "collector.flat_apply", "auction.list",
    "auction.bid", "auction.settle", "auction.money_claim",
    "auction.item_claim", "auction.remove", "auction.sql_apply",
    "auction.flat_apply", "boon.cash_completion", "crafting.recipe",
    "crafting.forge", "crafting.smith", "crafting.refine",
    "crafting.epic_store", "kingdom.store_purchase", "kingdom.store_refund",
    "item.creation_completion", "item.salvage_material_downgrade",
    "combat.sql_outcome", "collector.sql_apply", "item.sql_custody_apply",
    "death.corpse_sql_apply", "death.restitution_sql_apply",
}
MIXED_SCHEMA1_IDS = {
    "currency.coin_steal",
    "ship.coffer_claim", "ship.insurance_fallback",
    "special.money_changer", "special.smelter", "special.rentacleric",
    "special.witch_doctor", "special.llyren", "quest.reward",
    "quest.requirements", "world_quest.full_reward", "world_quest.kill_reward",
    "achievement.reward", "death.wallet_disposition", "death.corpse_creation",
    "currency.card_game_payout", "currency.wallet_credit",
    "currency.credit_pending_claim", "currency.wallet_debit", "currency.split",
    "coin.npc_give", "coin.compensation", "shop.buy_produced",
    "shop.sell_destroy", "shop.keeper_cash", "shop.repair",
    "crafting.recipe", "crafting.forge", "crafting.smith", "crafting.refine",
    "crafting.epic_store", "kingdom.store_purchase", "kingdom.store_refund",
}
SCHEMA2_CRAFT_IDS = {
    "item.poison_mix", "item.encrust_transform", "item.encrust_failure_destroy",
    "class.drannak_pvp_store", "item.craft_submit", "chaos.pouch_collection",
}
SCHEMA2_ALCHEMY_QUALIFIED_IDS = {
    "item.poison_mix", "item.encrust_transform", "item.encrust_failure_destroy",
    "class.drannak_pvp_store",
}
SCHEMA2_ITEM_TRANSFER_IDS = SCHEMA2_CRAFT_IDS | {
    "item.command_movement", "item.bulk_movement", "item.movement_submit",
    "item.trusted_steal", "item.creation_completion",
    "death.corpse_creation", "death.resurrection_publication",
    "quest.durable_offering_submission",
}
SCHEMA2_SQL_COIN_COMPONENT_IDS = {"coin.sql_accounting"}
SCHEMA2_SQL_SHOP_COMPONENT_IDS = {
    "shop.sql_native_item_events", "shop.sql_native_balances",
}
SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS = {"item.sql_custody_apply"}
SCHEMA2_SQL_ROOM_PAYLOAD_COMPONENT_IDS = {"item.sql_room_payload_record"}
MIXED_SPELL_ITEM_IDS = {
    "spell.minor_creation_fallback", "spell.flame_blade_grant",
    "spell.shield_grant", "spell.food_grant",
    "spell.insect_mandrake_fallback_sink", "spell.doom_blade_grant",
    "spell.snakes_direct_arrow_sink",
}
SOURCE_QUALIFIED_SPELL_GRANT_IDS = {
    "spell.minor_creation_fallback", "spell.flame_blade_grant",
    "spell.shield_grant", "spell.food_grant", "spell.doom_blade_grant",
}
SQL_ROUTE_TARGETS = {
    "combat.sql_outcome": {
        "holding_effect": "Participant wallet reward deltas and bank revision fences with legacy currency_ledger rows; balanced reward issuance is not proven here.",
        "custody_effect": "None at these SQL sites.",
        "native_state_targets": ["player_data wallet and revision", "account_banks bank_revision", "currency_ledger"],
    },
    "auction.money_claim_compensation": {
        "holding_effect": "Restores a pending auction claim after asynchronous wallet credit rejection; this direct SQL credit is separate from the earlier claim debit.",
        "custody_effect": "None at this callback site.",
        "native_state_targets": ["auction_money_pickups.money", "pending wallet credit result"],
    },
    "collector.sql_apply": {
        "holding_effect": "Purchase changes the buyer wallet and bank revision and appends currency_ledger; custody-only actions do not debit a buyer.",
        "custody_effect": "Changes item_current_owner and appends item_ownership_ledger for the held-item transfer.",
        "native_state_targets": ["player_data wallet", "account_banks bank_revision", "currency_ledger", "item_current_owner", "item_ownership_ledger"],
    },
    "item.sql_custody_apply": {
        "holding_effect": "Updates a money object's coin_payload when the typed item transfer changes its payload; any monetary source/sink requires a matching root account effect.",
        "custody_effect": "Creates or moves native UID ownership/history under the existing item root. This transaction also owns scoped immutable room payload and transferred player projection retirement; no second custody authority.",
        "native_state_targets": ["item_current_owner owner, revision and coin_payload", "item_ownership_ledger", "sql_room_item_payload through sql_room_item_payload_record", "retire_player_projection removes selected transferred player_items"],
    },
    "death.corpse_sql_apply": {
        "holding_effect": "Ensures the destination bank row and applies the corpse wallet plan within the lifecycle transaction; bank-row creation alone is not issuance.",
        "custody_effect": "Materializes or retires saved-item storage while a separate typed item handoff changes UID owner/revision.",
        "native_state_targets": ["player_data wallet", "account_banks mapping/revision", "saved_items", "item_current_owner", "item_ownership_ledger"],
    },
    "death.restitution_sql_apply": {
        "holding_effect": "No coin effect at the linked SQL sites.",
        "custody_effect": "Delivers selected UIDs to a recipient and appends item_ownership_ledger under expected revisions.",
        "native_state_targets": ["item_current_owner", "item_ownership_ledger", "player death restitution plan"],
    },
    "player.death_snapshot_quarantine": {
        "holding_effect": "No coin effect at this SQL site.",
        "custody_effect": "Marks disputed player-held UIDs quarantined with increased owner/item revisions; retains the items rather than destroying them.",
        "native_state_targets": ["item_current_owner.state and item_revision", "item_owner_revision", "player_death_custody"],
    },
    "recovery.saved_sql_store": {
        "holding_effect": "No coin effect at these SQL sites.",
        "custody_effect": "Replaces saved_items storage rows for an already identified room object tree; UID admission/movement must be proven by the caller.",
        "native_state_targets": ["saved_items and child rows", "room object UID/owner source"],
    },
    "recovery.saved_sql_delete": {
        "holding_effect": "No coin effect at this SQL site.",
        "custody_effect": "Deletes a saved_items storage row; this does not establish whether the UID moved or was destroyed.",
        "native_state_targets": ["saved_items", "item_current_owner and item_ownership_ledger expected from caller"],
    },
}
BUILDER_EVIDENCE = {
    "currency": ["src/economy/currency_command.c:270"],
    "coin": ["src/economy/coin_transfer_command.c:194"],
    "item": ["src/item/item_transfer_command.c:1077"],
    "shop": ["src/economy/shop_trade_command.c:431"],
    "collector": ["src/economy/collector_command.c:592"],
    "auction": ["src/economy/auction_command.c:336"],
    "boon": ["src/economy/boon_reward_command.c:52"],
}


def load_validator():
    spec = importlib.util.spec_from_file_location("economy_validator", VALIDATOR)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load source scanner: {VALIDATOR}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@lru_cache(maxsize=32)
def mask_cpp(source: str) -> str:
    """Blank comments/literals while preserving offsets and line numbers."""
    out = list(source)
    i = 0
    state = "code"
    while i < len(source):
        ch = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if ch == "/" and nxt == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = "line_comment"
                continue
            if ch == "/" and nxt == "*":
                out[i] = out[i + 1] = " "
                i += 2
                state = "block_comment"
                continue
            if ch == '"':
                out[i] = " "
                state = "string"
            elif ch == "'":
                out[i] = " "
                state = "char"
        elif state == "line_comment":
            if ch == "\n":
                state = "code"
            else:
                out[i] = " "
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                out[i] = out[i + 1] = " "
                i += 2
                state = "code"
                continue
            if ch != "\n":
                out[i] = " "
        elif state in {"string", "char"}:
            delimiter = '"' if state == "string" else "'"
            if ch == "\\":
                out[i] = " "
                if i + 1 < len(source) and source[i + 1] != "\n":
                    out[i + 1] = " "
                    i += 2
                    continue
            elif ch == delimiter:
                out[i] = " "
                state = "code"
            elif ch != "\n":
                out[i] = " "
        i += 1
    return "".join(out)


def source_definition_lines(path: Path, function: str | None) -> list[int]:
    if not function:
        return []
    source = path.read_text(encoding="utf-8", errors="replace")
    if path.suffix == ".py":
        tree = ast.parse(source, filename=str(path))
        return sorted(node.lineno for node in ast.walk(tree)
                      if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and node.name == function)
    code = mask_cpp(source)
    result: list[int] = []
    parts = function.rsplit("::", 1)
    special_member = function.startswith("~") or (len(parts) == 2 and
                      parts[1] in {parts[0].rsplit("::", 1)[-1],
                                   "~" + parts[0].rsplit("::", 1)[-1]})
    boundary = r"(?<!\w)" if function.startswith("~") else r"\b"
    for match in re.finditer(boundary + re.escape(function) + r"\s*\(", code):
        open_paren = code.find("(", match.start())
        depth = 0
        close_paren = -1
        for index in range(open_paren, len(code)):
            if code[index] == "(":
                depth += 1
            elif code[index] == ")":
                depth -= 1
                if depth == 0:
                    close_paren = index
                    break
        if close_paren < 0:
            continue
        tail = close_paren + 1
        while tail < len(code) and code[tail].isspace():
            tail += 1
        while tail < len(code):
            qualifier = re.match(r"(?:const\b|noexcept(?:\s*\([^)]*\))?|override\b|final\b|&|&&)\s*", code[tail:])
            if not qualifier:
                break
            tail += qualifier.end()
        if special_member and tail < len(code) and code[tail] == ":":
            # Constructors can have a member-initializer list before the body.
            # The scanner only needs the body anchor, not the initializer AST.
            body = code.find("{", tail)
            semicolon = code.find(";", tail)
            if body < 0 or (semicolon >= 0 and semicolon < body):
                continue
            tail = body
        if tail >= len(code) or code[tail] != "{":
            continue
        line_start = code.rfind("\n", 0, match.start()) + 1
        prefix = code[line_start:match.start()].strip()
        if not prefix and not special_member:
            previous_end = line_start - 1
            previous_start = code.rfind("\n", 0, previous_end) + 1
            previous = code[previous_start:previous_end].strip()
            if not re.fullmatch(r"[A-Za-z_]\w*(?:::\w+)*", previous):
                continue
        if any(word in prefix for word in ("return", "if ", "while ", "for ", "case ", "=")):
            continue
        result.append(code.count("\n", 0, match.start()) + 1)
    return sorted(set(result))


def source_targets(route_id: str, disposition: str) -> dict:
    if route_id == "repair.player_item_payload":
        return {"holding_effect": "No wallet or bank mutation, issuance, retirement or accounting root.", "custody_effect": OFFLINE_WRITERS[route_id], "native_state_targets": ["player_items and player_item_runtime_state for the retained UID", "player_item_affects and player_item_extra_descr", "player_death_restitution_receipt and player_death_restitution_item payload_repair audit rows"]}
    if disposition == "non_writer_candidate":
        return {"holding_effect": "none in this function for economy/custody scope", "custody_effect": "none", "native_state_targets": []}
    if disposition == "dormant_writer_candidate":
        if route_id == "currency.bank_single_projection":
            return {"holding_effect": "A call would change a connected PC's in-memory bank projection without changing account_banks.",
                    "custody_effect": "None", "native_state_targets": ["in-memory PC bank denomination projection", "GMCP vitals"]}
        return {"holding_effect": "direct writer exists but no in-tree gameplay caller was found", "custody_effect": "legacy behavior if revived; not currently reached in-tree", "native_state_targets": ["legacy auction/bank persistence code; see route detail"]}
    if disposition == "offline_operational_writer":
        return {"holding_effect": OFFLINE_WRITERS[route_id], "custody_effect": "offline import/baseline/restore only; no live publication", "native_state_targets": ["explicit offline database/restore target", "opening wallet/bank/item baseline where applicable"]}
    if route_id in SQL_ROUTE_TARGETS:
        return SQL_ROUTE_TARGETS[route_id]
    if route_id == "currency.bank_live_projection":
        return {"holding_effect": "Copies existing bank authority from a native load, committed completion, or validated stale receipt into retained and connected PC/GMCP projections; no native debit, credit, or issuance.",
                "custody_effect": "None", "native_state_targets": ["in-memory PC bank balance and bank revision projection", "GMCP vitals"]}
    if route_id in {"currency.wallet_live_projection", "player.load_economy_projection"}:
        return {"holding_effect": "Copies retained wallet/bank authority into the live PC projection after a committed result or validated load; no native debit, credit, or issuance.",
                "custody_effect": "None", "native_state_targets": ["in-memory PC wallet/bank denominations and revisions", "GMCP vitals where published"]}
    if route_id == "player.flatfile_baseline_projection":
        return {"holding_effect": "Copies the just-committed flat-file baseline wallet/bank and revisions into the live PC after a successful authority read; no second native balance change.",
                "custody_effect": "None at these eight assignments.",
                "native_state_targets": ["live PC cash[0..3] and bank[0..3] projections", "wallet_revision and bank_revision"]}
    if route_id == "player.legacy_flatfile_load":
        return {"holding_effect": "Loads four wallet denominations from the legacy character file into a PC; the file is not automatically the selected active-epoch authority.",
                "custody_effect": "None at these four assignments.",
                "native_state_targets": ["loaded PC cash[0..3] projection", "legacy character file wallet payload"]}
    if route_id == "player.sql_status_projection":
        return {"holding_effect": "Loads SQL player wallet into the PC and clears its bank projection pending a separate bank load; no native row changes in this function.",
                "custody_effect": "None at these eight assignments.",
                "native_state_targets": ["PC cash[0..3] and bank[0..3] projections", "SQL player_data wallet and wallet_revision"]}
    if route_id == "player.sql_bank_live_load":
        return {"holding_effect": "Clears PC bank projection before an SQL read, then fills it from account_banks only on valid result; a failed read leaves the projection at zero.",
                "custody_effect": "None at these eight assignments.",
                "native_state_targets": ["PC bank[0..3] projection", "account_banks selected account/racewar and bank_revision"]}
    if route_id == "recovery.copyover_npc_gold_projection":
        return {"holding_effect": "Replaces decoded/generated NPC gold with the legacy copyover gold field in either recovery path; no persistent source row is changed here.",
                "custody_effect": "None at these two assignments.",
                "native_state_targets": ["runtime NPC gold projection", "legacy copyover mob gold field"]}
    if route_id == "world.generated_npc_hydration":
        return {"holding_effect": "Copies four decoded NPC wallet denominations onto a newly read mobile during copyover. A later legacy copyover gold assignment may replace the decoded gold.",
                "custody_effect": "No item custody change in this function.",
                "native_state_targets": ["runtime NPC cash[0..3] projection", "encoded generated-NPC wallet snapshot"]}
    if route_id == "world.mobile_scaling":
        return {"holding_effect": "Recomputes NPC denominations from level/template, then applies elite scaling, minimum coin and no-money race/name rules. read_mobile calls it before admission; restorePet calls it again after a pet status load that has cleared saved cash.",
                "custody_effect": "No item custody change in this function.",
                "native_state_targets": ["runtime NPC cash[0..3]", "mobile template cash"]}
    if route_id == "pet.no_cash":
        return {"holding_effect": "PET_NOCASH zeroes all four NPC wallet denominations; callers include newly created summons and existing charm targets, so this cannot be classified as provisional-only.",
                "custody_effect": "The same helper links the NPC as a pet, but these four sites only change its cash.",
                "native_state_targets": ["runtime NPC cash[0..3]"]}
    if route_id == "recovery.pet_cash_discard":
        return {"holding_effect": "Reads four saved pet denominations then immediately zeroes them on a newly read mobile; restorePet later recomputes template cash. Whether the saved pet cash was an admitted holding is unresolved.",
                "custody_effect": "None at these eight assignments.",
                "native_state_targets": ["provisional/restored NPC cash[0..3]", "legacy pet file wallet payload"]}
    if route_id == "recovery.flat_corpse_coin_materialization":
        return {"holding_effect": "Materializes the saved corpse/room coin vector as a runtime pile without adding value to native authority.",
                "custody_effect": "Restores the saved pile UID and parent/room location after verifying the source record.",
                "native_state_targets": ["flatfile corpse/room coin record", "runtime coin pile value and UID"]}
    if route_id in {"world.patrol_clear_cash", "world.justice_guard_clear_cash", "world.zgame_zombie_clear_cash"}:
        return {"holding_effect": "CLEAR_MONEY expands to four NPC denomination zero assignments. The call is reachable after world placement or from a recurring patrol event, so an admitted holding can be cleared.",
                "custody_effect": "None at the macro call.",
                "native_state_targets": ["live NPC cash[0..3]"]}
    if route_id == "world.mobile_template":
        return {"holding_effect": "Parses prototype NPC denominations from the mob file in two legacy format branches; read_mobile later applies conversion/scaling before return. Copyover also calls read_mobile before restoring a saved NPC.",
                "custody_effect": "None at these eight assignments.",
                "native_state_targets": ["newly allocated NPC cash[0..3]", "mob template cash"]}
    if disposition == "runtime_projection_route" and route_id.startswith("ship."):
        return {"holding_effect": "Existing retained ship coffer value is projected into runtime state; no new value is created.",
                "custody_effect": "None", "native_state_targets": ["ships.money or flat-file ship record money", "runtime ShipData::money"]}
    if route_id.startswith("ship."):
        return {"holding_effect": "Durable ship coffer value, paired wallet credit, or an insurance/refund source according to the named route.",
                "custody_effect": "None for money; ship hydration and initialization are classified separately.",
                "native_state_targets": ["ships.money or flat-file ship record money", "runtime ShipData::money", "player_data wallet / pending auction claim where applicable"]}
    if route_id == "currency.coin_steal":
        return {"holding_effect": "Direct victim denomination debit followed by a separate ADD_MONEY credit to the thief; active accounting must refuse before the debit until one root owns both legs.",
                "custody_effect": "None in the coin branch of do_steal.",
                "native_state_targets": ["victim PC.cash[0..3] or NPC cash", "thief PC.cash[0..3] / player_data wallet", "pending auction claim on legacy credit-submission failure"]}
    if route_id.startswith("currency."):
        if route_id in {"currency.wallet_to_pile", "currency.pile_constructor", "currency.pile_add", "currency.room_pile_merge"}:
            return {"holding_effect": "Coin pile value and/or wallet-to-pile conversion; creation may be provisional until a live custody owner is published.", "custody_effect": "P_obj coin pile owner/list and durable item UID/coin payload when admitted.", "native_state_targets": ["PC.cash[0..3]", "P_obj.value[0..3]", "item_current_owner.coin_payload", "player_data.copper/silver/gold/platinum", "account_banks.bank_copper/bank_silver/bank_gold/bank_platinum"]}
        return {"holding_effect": "Player wallet denomination vector and/or shared account-bank denomination vector; preserve revisions and change-making exactly.", "custody_effect": "No item custody unless the route explicitly includes coin-pile movement.", "native_state_targets": ["PC.cash[0..3] / player_data.copper/silver/gold/platinum", "wallet_revision", "account_banks.bank_copper/bank_silver/bank_gold/bank_platinum", "bank_revision", "currency_ledger on applicable SQL paths"]}
    if route_id.startswith("coin."):
        return {"holding_effect": "Wallet denomination vector and durable coin-pile value; NPC recipient/compensation branches may still be in-memory/direct.", "custody_effect": "Coin pile UID and owner/root/parent transfer for floor/container custody.", "native_state_targets": ["PC.cash[0..3] / player_data denomination columns", "P_obj.value[0..3]", "item_current_owner.coin_payload", "item_current_owner owner/root/parent/revision", "item_ownership_ledger for covered custody path"]}
    if route_id.startswith("nq."):
        return {"holding_effect": "Numbered-quest reward credits a player wallet; requirement consumption reduces NPC-held cash after an earlier player give. The object allocation helper changes no coin balance.",
                "custody_effect": "Reward item allocation/publication and required item extraction need one admitted UID history, with provisional allocation separated from live publication.",
                "native_state_targets": ["player wallet / player_data denominations", "NPC quest actor cash", "quest item P_obj carrying and item_current_owner/item_ownership_ledger when admitted"]}
    if route_id.startswith(("item.", "death.", "recovery.")):
        return {"holding_effect": "No coin effect unless named in route detail; item value/identity remains attached to its UID.", "custody_effect": "Live object owner, equipment/container/room/corpse/player/pet/saved-item topology; distinguish transfer, recovery projection, staging rollback, and destruction.", "native_state_targets": ["P_obj carrying/equipment/room/container links", "item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,state)", "item_ownership_ledger(from_owner,to_owner,event_index,item_revision)", "player_items/corpse_items/locker_items/saved_items/player_pet_items where applicable"]}
    if route_id.startswith("auction."):
        return {"holding_effect": "Wallet, accepted bid escrow, seller proceeds/refunds or aggregate pending claim according to action; fee is a distinct sink.", "custody_effect": "Seller/winner/auction-item-pickup custody for item UID and claim retrieval state.", "native_state_targets": ["auctions(status,cur_price,winning_bidder_pid,quantity)", "auction_bid_history", "auction_money_pickups", "auction_item_pickups", "player_data denomination columns", "item_current_owner/item_ownership_ledger where routed"]}
    if route_id.startswith("collector."):
        return {"holding_effect": "Buyer wallet debit on purchase; no seller payout in current contract; other collector routes are custody-only.", "custody_effect": "Collector listing/held-item owner, buyer transfer, quarantine/cancellation, or expiry destruction.", "native_state_targets": ["collector repository/catalog/item state", "item_current_owner/item_ownership_ledger", "player_data denomination columns for purchase"]}
    if route_id.startswith("shop."):
        return {"holding_effect": "Player wallet debit/credit, shopkeeper cash where applicable, and service/repair cost.", "custody_effect": "Stock/shopkeeper/player item movement or produced-item admission; invalid-stock cleanup is a separate destruction branch.", "native_state_targets": ["player_data denomination columns", "shopkeeper_items/shop stock", "P_char keeper cash where used", "item_current_owner/item_ownership_ledger where routed"]}
    if route_id.startswith("lifecycle.") or route_id.startswith("staff.") or route_id.startswith("world."):
        return {"holding_effect": "Conditional NPC/player/bank value retirement or issuance only where the route detail names it; reset/delete is not automatically a sink.", "custody_effect": "World/object/character/account lifecycle, item admission, retirement, storage repair, or projection as described per route.", "native_state_targets": ["P_obj/P_char/world lists", "player_data/account_banks", "player_items/corpse_items/locker_items/saved_items/player_pet_items", "item_current_owner/item_ownership_ledger where durable"]}
    if route_id.startswith(("quest.", "world_quest.", "achievement.", "boon.", "crafting.", "mining.", "kingdom.")):
        return {"holding_effect": "Wallet/epic/guild or realm-material change only where route detail names it; issuance and expense need distinct system accounts.", "custody_effect": "Player inventory/room item creation, consumption, transformation, or refund as described per route.", "native_state_targets": ["PC.cash[0..3] / player_data denomination columns", "player inventory and P_obj containment", "item_current_owner/item_ownership_ledger where durable", "epic/guild/kingdom material or treasury state where applicable"]}
    return {"holding_effect": "See the source-backed route detail.", "custody_effect": "See the source-backed route detail.", "native_state_targets": ["Native function state plus existing repository/flat-file authority where applicable"]}


def schema_record(route_id: str, disposition: str) -> dict:
    if disposition in {"non_writer_candidate", "dormant_writer_candidate", "offline_operational_writer"}:
        current = "not_applicable" if disposition == "non_writer_candidate" else "none"
        mode = "no live critical-command gameplay route"
    elif route_id in SCHEMA2_SQL_COIN_COMPONENT_IDS:
        current = 2
        mode = "typed_schema_2_sql_component_without_qualified_gameplay_route"
    elif route_id in SCHEMA2_SQL_SHOP_COMPONENT_IDS:
        current = 2
        mode = "typed_schema_2_sql_shop_native_component_without_qualified_gameplay_route"
    elif route_id in SCHEMA2_SQL_ROOM_PAYLOAD_COMPONENT_IDS:
        current = 2
        mode = "typed_schema_2_sql_payload_component_without_independent_accounting_root"
    elif route_id == "currency.split":
        current = 1
        mode = "schema_1_when_inactive_schema_2_sequential_coin_children_when_active"
    elif route_id in SOURCE_QUALIFIED_SPELL_GRANT_IDS:
        current = None
        mode = "typed_schema_2_for_active_pc_npc_refused_direct_legacy_when_inactive"
    elif route_id in MIXED_SPELL_ITEM_IDS:
        current = None
        mode = "typed_schema_2_for_active_pc_via_shared_item_owner_direct_legacy_for_npc_or_inactive"
    elif route_id in SCHEMA1_IDS:
        current = 1
        mode = "schema_1_plus_direct_side_effects" if route_id in MIXED_SCHEMA1_IDS else "schema_1_typed_command_path"
        if route_id in SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS:
            mode = "schema_1_when_inactive_schema_2_item_repository_component_when_active"
        if route_id in SCHEMA2_ITEM_TRANSFER_IDS:
            mode = "schema_1_when_inactive_schema_2_item_accounting_when_active"
    elif disposition == "runtime_projection_route":
        current = None
        mode = "native_result_or_recovery_projection_without_new_accounting_root"
    else:
        current = None
        mode = "legacy_direct_without_a_schema_1_command_at_this_function"
    family = route_id.split(".", 1)[0]
    evidence = list(BUILDER_EVIDENCE.get(family, [])) if current == 1 else []
    evidence.extend(["src/persistence/critical_command.h:9-10", "docs/persistence/economy_accounting/INTENT_DESIGN.md:3-6,10-17"])
    if route_id == "currency.split":
        evidence.extend([
            "src/cmd/actoth.c:do_split and continue_money_split (active wallet-to-wallet schema-2 coin child per eligible recipient)",
            "tests/async/test_currency_completion_retention.py (retained completion and partial split behavior)",
        ])
    if route_id in SCHEMA2_SQL_COIN_COMPONENT_IDS:
        evidence.extend([
            "src/economy/coin_transfer_accounting.c:947-1017 (SQL native coin effects, balanced postings, item references, source claim and receipt)",
            "tests/async/test_coin_transfer_accounting.py (SQL component harness; not a real player journey)",
        ])
    if route_id in SCHEMA2_SQL_SHOP_COMPONENT_IDS:
        evidence.extend([
            "src/persistence/economic_sql_shop_trade_transaction.c (typed SQL shop root applies native balances and item custody)",
            "tests/async/test_shop_trade_accounting_context.py (focused contract; not a full player journey)",
        ])
    schema2_connected = route_id in SCHEMA2_ITEM_TRANSFER_IDS or route_id == "currency.split"
    if route_id in SCHEMA2_CRAFT_IDS:
        evidence.extend([
            "src/item/item_movement_transaction.c:item_movement_transaction_submit_craft (prepare crafting intent before retained coordinator admission)",
            "src/economy/item_transfer_accounting.c:item_transfer_craft_accounting_effects (exact consumed inputs, output admissions and consumed-input source lifetime)",
            "tests/async/item_transfer_mysql_harness.cpp:check_accounted_craft_conservation (both native SQL engines, references, replay, rejection and zero-output failure)",
            "tests/async/flatfile_craft_conservation_harness.cpp (native authority commit, exact replay and separate-process interrupted recovery)",
            "tests/async/test_publication_retention_runtime.py (active craft held for original actor and output restoration)",
        ])
    item_action_coverage = (
        " and spell creation/component consumption, sticks-to-snakes retirement, and key-break retirement"
        if route_id == "item.movement_submit" else ""
    )
    if route_id in SCHEMA2_ITEM_TRANSFER_IDS:
        evidence.extend([
            "src/item/item_movement_transaction.c:1563-1668,1817-1928 (prepare eligible ordinary, bulk, trusted-steal, sourced creation/destruction, corpse creation/loot, and resurrection item handoffs while accounting is active)",
            "src/economy/economic_gameplay_authority.c:254-295 (freeze the typed schema-2 item intent)",
            "src/economy/item_transfer_accounting.c (bind player/corpse ownership endpoints and actors; restrict creation and non-coin retirement to typed source events)",
            "src/persistence/economic_sql_item_transfer_transaction.c:136-167,352-394,426-482 (SQL source claims, custody references, and retained replay verification)",
            "tests/async/item_transfer_mysql_harness.cpp:692-1002 (SQL sourced player/room creation, ordinary transfer, trusted-steal, retirement references, and replay)",
            "src/flatfile/flatfile_accounting_dispatch.c:8-20 (schema-2 item dispatch to the typed item owner)",
            "src/flatfile/flatfile_item_repository.c:2570-2635,2970-3070 (atomic custody/accounting/reference commit and exact replay verification)",
            "src/flatfile/flatfile_accounting_store.c:641-704 (source claim staging and retained verification)",
            "src/cmd/actmove.c:117-160 (key break submits item-action retirement before extracting the runtime object)",
            "src/world/quest.c:228-231 (quest reward room creation retains the typed source event)",
            "src/magic/spell_conjuration.c (transactional minor creation, create food, conjured item grants, component consumption, and sticks-to-snakes arrow retirement)",
            "src/magic/magic.c and src/magic/spell_item_lifecycle.h (batch spell-component retirement and completion dispatch)",
            "src/magic/spell_conjuration.c, src/magic/spell_spore_cloud.c, src/magic/spells.c, and src/classes/necromancy.c (spell effects resume after component retirement commits)",
            "tests/async/economic_accounting_flatfile_gate_test.cpp:224-381 (sourced room creation/retirement, exact references, replay rejection and journal recovery)",
            "tests/async/item_transfer_accounting_test.cpp (sourced room creation and item-action retirement admission)",
        ])
        if route_id == "death.corpse_creation":
            evidence.append("src/combat/fight.c:703-714 (submit the victim's captured item roots as one corpse custody batch)")
            evidence.append("src/economy/item_transfer_accounting.c (bind corpse PID/save ID to the player actor before admitting the item_move root)")
            evidence.append("tests/async/item_transfer_accounting_test.cpp (corpse batch identity and actor mismatch contracts)")
        elif route_id == "death.resurrection_publication":
            evidence.append("src/world/handler.c:4373-4384 (persist the player-to-room item handoff through item_movement_transaction_submit)")
    if route_id == "death.corpse_creation":
        interpretation = "Schema 1 remains the current envelope. When accounting is active, the player-to-corpse item handoff is an actor-bound schema-2 item_move with exact custody references on SQL and flat-file. Wallet disposition and the separate corpse metadata lifecycle are not covered by this item child."
    elif route_id == "death.resurrection_publication":
        interpretation = "Schema 1 remains the current envelope. The player-to-room item handoff during resurrection uses the schema-2 player-drop item path with exact custody references when accounting is active; corpse lifecycle and other resurrection publication remain separate coverage items."
    else:
        interpretation = f"Schema 1 remains the inactive/legacy path. When the accounting authority is active, eligible player get/drop/put/give moves, trusted-steal handoffs, typed-source player/room creation grants, and item-action consumption/destruction{item_action_coverage} use a frozen schema-2 intent on SQL and flat-file; each backend stores the item operation, source claim when required, exact custody references and result atomically. Other item destruction/extraction paths and money-valued item creation/destruction remain unsupported." if schema2_connected else "Schema 1 is the current legacy gameplay envelope; schema 2 is bounded/frozen-intent support, with only partial bank/baseline repository paths and the eligible item custody slice connected." if current == 1 else "This function projects an existing native result or recovery snapshot; it must prove identity, completeness and non-stale publication, and creates no new accounting root." if disposition == "runtime_projection_route" else "No schema-2 gameplay writer is connected here; direct legacy paths must be blocked or migrated before activation." if disposition not in {"non_writer_candidate", "dormant_writer_candidate", "offline_operational_writer"} else mode
    if route_id == "currency.split":
        interpretation = "Inactive do_split uses a schema-1 sender debit followed by recipient credits. Active do_split submits one balanced schema-2 wallet-to-wallet coin child per eligible recipient, with exact denomination and retained completion. Completed shares remain transferred if a later child fails; there is no atomic multi-party split root. Full backend gameplay qualification remains pending."
    elif route_id in SCHEMA2_SQL_COIN_COMPONENT_IDS:
        interpretation = "The SQL transaction component records typed schema-2 coin effects and balanced postings. Pooled dispatch/reconcile and player-visible publication are separate qualification gates; flat-file coin accounting remains unqualified."
    elif route_id in SCHEMA2_SQL_SHOP_COMPONENT_IDS:
        interpretation = "This SQL component writes native shop balances or item custody within an owning typed schema-2 shop root. Complete route qualification, publication/restart proof and flat-file parity remain separate gates."
    elif route_id in SCHEMA2_SQL_ROOM_PAYLOAD_COMPONENT_IDS:
        interpretation = "Payload-only storage borrows the admitted item root transaction/session/source locks; no new custody, monetary posting or gameplay qualification. Missing literal text, native source or conflicting provenance refuses before mutation."
    elif route_id in SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS:
        interpretation = "The SQL item repository applies schema-1 commands when inactive and can apply a typed schema-2 item transfer under an owning root when active. This repository function is a component, not a gameplay producer or a complete money-valued coin route; root accounting, flat-file parity and playable acceptance remain separate gates."
    elif route_id in SOURCE_QUALIFIED_SPELL_GRANT_IDS:
        interpretation = "Active PC grants retain a source ID independent of the UID through the shared typed item owner. Active NPC casts refuse item publication, while inactive casts use direct legacy delivery. Live backend and restart journeys remain unverified."
    elif route_id in MIXED_SPELL_ITEM_IDS:
        interpretation = "The active PC branch calls the shared typed schema-2 item owner, while NPC and inactive branches directly mutate live custody. That shared component does not qualify the complete spell route or its publication, replay and fallback behavior."
    return {
        "current_schema": current,
        "route_mode": mode,
        "accounting_intent_attached_by_current_gameplay_route": schema2_connected,
        "schema_2_gameplay_producer_connected": schema2_connected,
        "evidence": evidence,
        "interpretation": interpretation,
    }


def double_entry(route_id: str, disposition: str, schema: dict) -> dict:
    if route_id in SCHEMA2_SQL_ROOM_PAYLOAD_COMPONENT_IDS:
        return {"status": "sql_payload_component_without_independent_accounting_root", "unified_operation_postings_observed": False, "legacy_domain_evidence": ["Payload participates in the existing typed item root; no new accounting root or posting."], "required_atomic_evidence": ["Complete native gameplay, ACK and cold-boot qualification under the item owner"], "global_evidence": ["src/persistence/sql_room_item_payload.c:sql_room_item_payload_record", "src/item/item_transfer_repository.c:item_transfer_repository_execute_at_offset"], "note": "Source observed only; backend entries unverified and global activation blocked."}
    if disposition == "non_writer_candidate":
        return {"status": "not_applicable", "unified_operation_postings_observed": False, "existing_evidence": [], "note": NON_WRITERS[route_id]}
    if route_id in SCHEMA2_SQL_COIN_COMPONENT_IDS:
        return {
            "status": "sql_component_balanced_postings_without_playable_route",
            "unified_operation_postings_observed": True,
            "legacy_domain_evidence": ["The SQL coin component binds native wallet/pile effects, item references, postings and source claims under one root."],
            "required_atomic_evidence": ["Pooled root dispatch/reconcile", "post-commit player publication", "equivalent flat-file evidence and replay"],
            "global_evidence": ["src/economy/coin_transfer_accounting.c", "tests/async/test_coin_transfer_accounting.py"],
            "note": "Observed only in a SQL component harness; no real player journey or full backend route is qualified.",
        }
    if route_id == "currency.split":
        return {
            "status": "typed_schema2_balanced_child_roots_without_global_split_atomicity",
            "unified_operation_postings_observed": True,
            "legacy_domain_evidence": ["Active split submits an exact-denomination wallet-to-wallet coin transfer for each eligible recipient."],
            "required_atomic_evidence": ["Both-backend gameplay qualification for each child and retained completion", "Explicit acceptance of partial completion rather than global split atomicity"],
            "global_evidence": ["src/cmd/actoth.c:continue_money_split", "src/economy/currency_transaction.c", "tests/async/test_currency_completion_retention.py"],
            "note": "Each accepted child has balanced postings; the whole group split is deliberately sequential and may partially complete.",
        }
    if disposition == "dormant_writer_candidate":
        status = "not_executed_in_tree"
    elif disposition == "offline_operational_writer":
        status = "offline_baseline_or_restore_evidence_only"
    elif route_id in SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS:
        status = "typed_schema2_item_repository_component_without_gameplay_producer"
    elif route_id in SCHEMA2_ITEM_TRANSFER_IDS:
        status = "typed_schema2_item_custody_reference_without_coin_effect"
    elif route_id in PROJECTION_ROUTES:
        status = "projection_only_no_new_postings"
    elif schema["current_schema"] == 1:
        status = "legacy_domain_ledger_only_not_double_entry"
    else:
        status = "no_operation_level_double_entry_evidence"
    existing = []
    item_action_coverage = (
        " and spell component consumption/sticks-to-snakes retirement and key-break retirement"
        if route_id == "item.movement_submit" else ""
    )
    if route_id.startswith(("currency.", "coin.")) and route_id not in {"currency.pile_constructor", "currency.pile_add", "currency.room_pile_merge"}:
        existing.append("currency_ledger captures native wallet/bank deltas, after-vectors and revisions on applicable SQL paths; no balancing counter-account/source posting is present in this route evidence.")
    if route_id.startswith(("item.", "death.", "recovery.", "shop.", "collector.", "auction.")):
        if route_id in SCHEMA2_ITEM_TRANSFER_IDS:
            existing.append("item_ownership_ledger remains the native custody ledger; the accounting proof is the separate typed operation and linked item references.")
        elif route_id in SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS:
            existing.append("The SQL item repository records native UID ownership and ledger events for a typed root; an owning schema-2 transaction must attach exact accounting item references. This component alone is not gameplay or flat-file proof.")
        else:
            existing.append("item_ownership_ledger is custody evidence where the typed item repository runs; it is not a monetary double-entry ledger and does not prove this route is connected.")
    if route_id in SCHEMA2_ITEM_TRANSFER_IDS:
        existing.append(f"Eligible SQL and flat-file player transfers, corpse custody handoffs, sourced player/room creation, and item-action consumption/destruction{item_action_coverage} use the typed schema-2 custody operation, source claim where applicable, exact economic_accounting_item_reference rows, and native custody result. Route-specific gameplay acceptance remains pending; these custody-only roots do not assign market value, and money-valued item creation/destruction remains unsupported until balanced coin postings are added.")
    if route_id.startswith("auction."):
        existing.append("Auction state/pickup rows are domain evidence, not a unified root-operation posting set.")
    return {
        "status": status,
        "unified_operation_postings_observed": False,
        "legacy_domain_evidence": existing,
        "required_atomic_evidence": ([
            "economic_accounting_operation with frozen schema-2 intent",
            "exact economic_accounting_item_reference for each item ownership event",
            "matching item_current_owner and item_ownership_ledger rows",
            "flat-file authority journal covering item_ownership catalog, accounting operation/source claim and item reference bucket",
            "retained critical-command inbox/result for idempotent replay",
        ] if route_id in SCHEMA2_ITEM_TRANSFER_IDS | SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS else [
            "economic_accounting_operation",
            "economic_accounting_account_effect",
            "balanced economic_accounting_coin_posting rows where currency changes",
            "linked item event references for custody-changing roots",
        ]),
        "global_evidence": ["docs/persistence/ECONOMY_ACCOUNTING.md:3-13", "docs/persistence/economy_accounting/DELIVERY_PLAN.md:162-171"],
        "note": ("The typed item-transfer owner persists exact SQL and flat-file custody references; complete writer coverage and route-specific gameplay acceptance are pending, so activation remains blocked."
                 if route_id in SCHEMA2_ITEM_TRANSFER_IDS else
                 "The SQL repository component requires an owning root and exact item-reference evidence; component execution is not a playable route."
                 if route_id in SCHEMA2_ITEM_REPOSITORY_COMPONENT_IDS else
                 "The accounting tables/codec are draft/partial infrastructure; no complete gameplay writer journey is qualified or activated."),
    }


def activation_policy(route_id: str, disposition: str, schema: dict) -> dict:
    if disposition == "non_writer_candidate":
        decision = "not_a_playable_economy_or_custody_writer"
        policy = NON_WRITERS[route_id]
    elif route_id in SCHEMA2_ALCHEMY_QUALIFIED_IDS:
        decision = "allow_qualified_native_alchemy"
        policy = "Use the native schema-2 craft owner, exact input/output references, consumed-input source lifetime and durable alchemy publication receipt. Active flatfile, MySQL and MariaDB route journeys qualify retry, retained disconnect, copyover and cold recovery. This scoped proof does not authorize global activation or certify other crafts, NPC vial issuance or character initialization. Refuse unsupported metadata or stale custody before native mutation; never fall back to schema 1 under active authority."
    elif route_id in SCHEMA2_CRAFT_IDS:
        decision = "block_until_active_craft_journeys"
        policy = "Physical crafts, pouch collection and virtual Encrust use the typed schema-2 owner, consumed-input crafting source, exact native retirement/admission and linked references on SQL and flatfile. Retained pouch counters share the native craft commit while preserving the original UID and custody. Native component, replay, rollback and held-publication proofs pass; qualify complete active-epoch server journeys before release."
    elif route_id == "item.npc_alchemist_vial_grant":
        decision = "refuse_before_allocation_until_native_source_and_root_exist"
        policy = "Active authority refuses at gameplay entry before recipe RNG, wait, UID allocation or mutation. Crafting needs a native schema-2 root with exact input/output references; automatic vial issuance needs a durable zone spawn/source identity. The inactive legacy receipt or fresh-spawn marker does not satisfy those obligations."
    elif route_id == "currency.split":
        decision = "allow_sequential_schema2_coin_children"
        policy = "Under active authority, admit only identified player wallets and submit one exact-denomination balanced transfer per eligible recipient. Retain each completion before continuing; stop on failure and report that completed shares remain transferred. The legacy schema-1 branch runs only while accounting is inactive."
    elif route_id in SCHEMA2_SQL_SHOP_COMPONENT_IDS:
        decision = "sql_component_requires_qualified_root"
        policy = "Apply only under the owning typed schema-2 shop root. Qualify pooled apply/reconcile, same-root native and accounting evidence, publication/restart, and flat-file parity before enabling the whole gameplay route."
    elif route_id in SCHEMA2_SQL_ROOM_PAYLOAD_COMPONENT_IDS:
        decision = "sql_component_requires_qualified_root"
        policy = "Apply only under the admitted item root with exact native source, successful retained provenance, current season and immutable bytes. Complete native route/restart qualification before admission; no generic fallback or activation."
    elif disposition == "dormant_writer_candidate":
        decision = "keep_unreachable_or_block_if_reactivated"
        if route_id == "currency.bank_single_projection":
            policy = DORMANT_WRITERS[route_id] + " A live publication requires the selected mapping, committed bank result and non-stale revision."
        else:
            policy = DORMANT_WRITERS[route_id] + " Any reactivation must route through the typed schema-2 writer before native mutation."
    elif disposition == "offline_operational_writer":
        decision = "maintenance_only_no_live_admission"
        policy = OFFLINE_WRITERS[route_id] + " Require a quiesced boundary, exact retained lineage/epoch, source snapshot witness and idempotent resume; fail closed on unknown rows."
    elif route_id in {"player.legacy_flatfile_load", "player.sql_status_projection",
                      "player.sql_bank_live_load", "world.generated_npc_hydration",
                      "recovery.copyover_npc_gold_projection",
                      "recovery.flat_corpse_coin_materialization",
                      "recovery.flat_shopkeeper_cash_materialization",
                      "recovery.sql_shopkeeper_cash_materialization",
                      "item.pet_give_publication", "item.command_publication",
                      "quest.durable_offering_publication",
                      "item.empty_publication", "item.weight_relink",
                      "item.equipment_wear", "item.equipment_remove",
                      "coin.wallet_pile_publication", "death.corpse_release_live",
                      "death.corpse_raise_recovery_live", "death.corpse_resurrection_item_live",
                      "death.corpse_nested_release_live", "death.corpse_destruction_live",
                      "death.corpse_transient_cleanup", "death.corpse_committed_money_cleanup",
                      "death.resurrection_committed_player_drop",
                      "death.resurrection_committed_corpse_cleanup",
                      "item.pet_teardown_unload", "item.ascension_equipment_relink",
                      "recovery.sql_player_item_hydration", "recovery.sql_locker_item_hydration",
                      "recovery.sql_private_chest_hydration", "recovery.sql_corpse_hydration",
                      "recovery.sql_saved_item_hydration", "recovery.sql_exact_room_hydration",
                      "recovery.sql_exact_room_placement", "world.zone_reset_equip_relink",
                      "world.copyover_item_materialization", "recovery.flat_room_item_projection",
                      "mob.thief_weapon_relink", "mob.better_object_relink",
                      "mob.hunter_weapon_relink", "player.flat_terminal_inventory_unload",
                      "player.sql_terminal_inventory_unload", "recovery.legacy_object_restore",
                      "recovery.single_item_decode", "recovery.pet_save_equipment_relink",
                      "movement.key_break_committed_publication", "movement.drag_room_relink",
                      "locker.chest_item_restore", "locker.public_items_save_relink",
                      "locker.public_items_room_restore", "combat.disarm_slot_relink",
                      "spell.snakes_committed_arrow_cleanup", "range.gather_quiver_relink",
                      "death.raise_nested_exclusion_cleanup",
                      "death.raise_committed_publication",
                      "special.disarm_pick_gloves_slot_relink",
                      "artifact.good_evil_sword_slot_relink"}:
        decision = "block_until_projection_proof"
        if route_id == "player.sql_bank_live_load":
            policy = PROJECTION_ROUTES[route_id] + " The login caller ignores a failed bank read after this function has zeroed the PC bank. Refuse publication until the selected bank result and revision are verified."
        elif route_id == "player.sql_status_projection":
            policy = PROJECTION_ROUTES[route_id] + " Refuse publication until the separate selected account-bank load succeeds and wallet/bank revisions form one complete player view."
        elif route_id == "recovery.flat_corpse_coin_materialization":
            policy = PROJECTION_ROUTES[route_id] + " Refuse publication until the selected saved corpse/room identity, exact coin vector, pile UID and complete source receipt are verified."
        elif route_id == "recovery.flat_shopkeeper_cash_materialization":
            policy = PROJECTION_ROUTES[route_id] + " Refuse publication until the selected shop ID, retained cash and revision, and complete shopkeeper ownership record are verified; emit no new posting."
        elif route_id == "recovery.sql_shopkeeper_cash_materialization":
            policy = PROJECTION_ROUTES[route_id] + " Refuse publication until the selected shopkeeper row, cash revision, and complete stock identity are verified; a legacy NULL cash value cannot establish retained treasury authority. Emit no new posting."
        elif route_id.startswith(("item.", "death.", "coin.", "mob.", "movement.", "locker.", "combat.", "spell.", "range.")) or route_id.startswith("recovery.sql_") or route_id in {"world.zone_reset_equip_relink", "world.copyover_item_materialization", "recovery.flat_room_item_projection", "player.flat_terminal_inventory_unload", "player.sql_terminal_inventory_unload", "recovery.legacy_object_restore", "recovery.single_item_decode", "recovery.pet_save_equipment_relink"}:
            policy = PROJECTION_ROUTES[route_id] + " Refuse active-epoch publication until the committed root receipt, exact UID/owner/revision and live topology or slot state are verified."
        else:
            policy = PROJECTION_ROUTES[route_id] + " Current source does not prove the selected active-epoch authority and complete recovery identity, and it does not reject inconsistent decoded and legacy gold before the gold overwrite. Refuse active-epoch publication until that proof is executable; do not treat a recovery overwrite as issuance."
    elif route_id in PROJECTION_ROUTES:
        decision = "allow_projection_only_with_committed_identity"
        if route_id == "currency.bank_live_projection":
            policy = PROJECTION_ROUTES[route_id] + " Require the selected account/racewar mapping and a non-stale native bank revision before publication; emit no new posting."
        elif route_id in {"currency.wallet_live_projection", "player.load_economy_projection", "player.flatfile_baseline_projection", "player.legacy_flatfile_load"}:
            policy = PROJECTION_ROUTES[route_id] + " Require selected native identity, complete read/receipt and non-stale revision before publication; emit no new posting."
        elif route_id == "world.generated_npc_hydration":
            policy = PROJECTION_ROUTES[route_id] + " Require a complete, identity-matched copyover snapshot and resolve the later legacy gold overwrite before treating the recovered wallet as authoritative; emit no new issuance posting."
        elif route_id == "recovery.copyover_npc_gold_projection":
            policy = PROJECTION_ROUTES[route_id] + " Refuse inconsistent decoded and legacy gold or missing source identity; emit no new issuance posting."
        else:
            policy = PROJECTION_ROUTES[route_id] + " Preserve existing UID/root/parent and receipt; emit no new issuance posting. Refuse mismatched/missing identity instead of minting or silently changing owner."
    else:
        decision = "block_until_typed_schema2_accounting"
        policy = "Reject before the first native/live mutation unless one immutable typed operation binds actor, source event, native identity/revisions, exact coin postings and ordered custody events; commit domain state, evidence and receipt atomically on the selected backend, then publish. Do not fall back to schema 1 or direct legacy mutation."
        if route_id in {"item.extraction", "item.scrap", "world.zone_purge", "lifecycle.sql_reset", "lifecycle.character_delete", "lifecycle.flat_character_delete", "lifecycle.sql_character_delete", "lifecycle.sql_account_delete", "lifecycle.flat_account_delete", "staff.purge", "staff.zone_reset", "lifecycle.reset_entry"}:
            policy += " For lifecycle/destruction paths, distinguish true retirement from staging, unload, rollback, reset reconstruction and recovery; only true destruction gets a linked custody tombstone and explicit coin sink."
        if route_id in {"world.mobile_scaling", "world.mobile_template", "pet.no_cash", "recovery.pet_cash_discard"}:
            policy += " Prove the NPC is provisional before this assignment or preserve its admitted source/sink and revision in one root; existing charm targets and pet restoration cannot be assumed fresh."
        if route_id.startswith("auction."):
            policy += " Keep accepted escrow, seller claim/proceeds, fee and item winner/return custody under the same auction root; no implicit reimbursement."
    return {"decision": decision, "must_block_on_activation": decision in {"block_until_active_craft_journeys", "refuse_before_allocation_until_native_source_and_root_exist", "block_until_typed_schema2_accounting", "block_until_projection_proof", "keep_unreachable_or_block_if_reactivated", "sql_component_requires_qualified_root"}, "required_policy": policy}


def build() -> dict:
    registry = json.loads(REGISTRY.read_text(encoding="utf-8"))
    validator = load_validator()
    census = validator.scan_sources(ROOT)
    source_commit = registry["source_commit"]
    baseline_census = registry["census"]
    unique_key = lambda row: (row["path"], row["line"], row["family"])
    baseline_sites = {unique_key(row) for row in baseline_census}
    current_sites = {unique_key(row) for row in census}
    mapped_sites = {tuple(site) for row in registry["writers"] for site in row.get("sites", [])}
    family_counts = dict(sorted(Counter(row["family"] for row in census).items()))

    candidates = list(registry["writers"])
    routes = []
    for raw in candidates:
        route_id = raw["id"]
        file_path = SOURCE_FILE_FIXES.get(route_id, raw["path"])
        function = FUNCTION_FIXES.get(route_id, raw["symbol"])
        if function.startswith("def "):
            function = function[4:]
        if route_id in {"kingdom.workshop_props", "macro.clear_money_definition", "macro.add_coins_declaration", "macro.difficulty_scale_declaration", "macro.money_helper_declarations", "macro.economic_submit_declarations", "macro.item_constructor_declarations", "macro.checked_item_publication_declaration"}:
            function = None
        locations = source_definition_lines(ROOT / file_path, function)
        if route_id not in {"kingdom.workshop_props", "macro.clear_money_definition", "macro.add_coins_declaration", "macro.difficulty_scale_declaration", "macro.money_helper_declarations", "macro.economic_submit_declarations", "macro.item_constructor_declarations", "macro.checked_item_publication_declaration"} and not locations:
            raise ValueError(f"source function definition not found for {route_id}: {file_path}:{function}")
        if route_id in NON_WRITERS:
            disposition = "non_writer_candidate"
        elif route_id in OFFLINE_WRITERS:
            disposition = "offline_operational_writer"
        elif route_id in DORMANT_WRITERS:
            disposition = "dormant_writer_candidate"
        elif route_id in PROJECTION_ROUTES:
            disposition = "runtime_projection_route"
        else:
            disposition = "runtime_mutation_route"
        schema = schema_record(route_id, disposition)
        details = (raw["native_effects"] if disposition in {"runtime_mutation_route", "runtime_projection_route"} and raw.get("native_effects")
                   else source_targets(route_id, disposition))
        if route_id == "currency.split":
            details = {
                "holding_effect": "Inactive mode debits the sender before separate recipient credits. Active mode submits one balanced exact-denomination wallet-to-wallet schema-2 child per eligible recipient; completed children remain transferred if a later child fails.",
                "custody_effect": "None; this route moves wallet value, not item UIDs.",
                "native_state_targets": ["eligible recipients' PC.cash[0..3] / player_data denomination columns", "sender PC.cash[0..3] / player_data denomination columns", "wallet_revision and currency_ledger on applicable SQL legs", "economic_accounting_operation and balanced coin postings for each active child"],
            }
        route = {
            "id": route_id,
            "source": {
                "file": file_path,
                "function": function,
                "definition_lines": locations,
                "original_draft_symbol": raw.get("symbol"),
                "anchor_status": "function_definition_verified" if locations else "field_name_not_function",
            },
            "reason": raw.get("reason"),
            "source_classification": raw.get("classification"),
            "disposition": disposition,
            "counts_as_runtime_writer": disposition in {"runtime_mutation_route", "runtime_projection_route"},
            "counts_as_operational_writer": disposition == "offline_operational_writer",
            "native_effects": details,
            "current_critical_command_schema": schema,
            "double_entry_evidence": double_entry(route_id, disposition, schema),
            "blocking_policy_after_activation": activation_policy(route_id, disposition, schema),
            "source_review_limit": "Current function/anchor confirmed; this row does not prove all callers, backend parity, or a complete codebase-wide semantic census.",
        }
        if route_id in DORMANT_WRITERS:
            route["reachability_evidence"] = raw.get("reachability_evidence") or "Definition exists; no in-tree callsite was found in the current source search."
        elif raw.get("reachability_evidence"):
            route["reachability_evidence"] = raw["reachability_evidence"]
        if raw.get("refusal_source_evidence"):
            route["refusal_source_evidence"] = raw["refusal_source_evidence"]
        if route_id in SCHEMA2_ALCHEMY_QUALIFIED_IDS:
            route["backend_qualification"] = raw["backends"]
            route["recovery_evidence"] = raw["evidence"]
        if route_id in NON_WRITERS:
            route["exclusion_reason"] = NON_WRITERS[route_id]
        if route_id in PROJECTION_ROUTES:
            route["projection_rule"] = PROJECTION_ROUTES[route_id]
        routes.append(route)

    dispositions = Counter(route["disposition"] for route in routes)
    counts = {
        "draft_registry_rows": len(registry["writers"]),
        "supplemental_candidates_found": 0,
        "matrix_rows": len(routes),
        "runtime_mutation_and_projection_routes": sum(r["counts_as_runtime_writer"] for r in routes),
        "offline_operational_writer_routes": sum(r["counts_as_operational_writer"] for r in routes),
        "dormant_writer_candidates": dispositions["dormant_writer_candidate"],
        "non_writer_or_out_of_scope_candidates": dispositions["non_writer_candidate"],
        "function_definition_anchors_verified": sum(bool(r["source"]["definition_lines"]) for r in routes),
        "non_function_candidate_anchors": sum(not r["source"]["definition_lines"] for r in routes),
        "routes_with_current_schema_1_path": sum(r["current_critical_command_schema"]["current_schema"] == 1 for r in routes),
        "routes_with_schema_2_gameplay_producer": sum(r["current_critical_command_schema"]["schema_2_gameplay_producer_connected"] for r in routes),
        "routes_with_unified_double_entry_evidence": sum(r["double_entry_evidence"]["unified_operation_postings_observed"] for r in routes),
    }
    if sum(counts[key] for key in ("runtime_mutation_and_projection_routes", "offline_operational_writer_routes", "dormant_writer_candidates", "non_writer_or_out_of_scope_candidates")) != counts["matrix_rows"]:
        raise ValueError("route classifications do not sum to the matrix row count")

    try:
        exact_drift_removed = sum(1 for row in baseline_census if row not in census)
        exact_drift_added = sum(1 for row in census if row not in baseline_census)
    except TypeError:
        exact_drift_removed = exact_drift_added = None
    output = {
        "schema_version": 1,
        "artifact_kind": "source_reviewed_economy_item_writer_coverage_matrix",
        "repository_head": source_commit,
        "source_state": "published_base_with_unpublished_candidate_worktree" if registry.get("candidate_worktree_evidence") else "registry_source_commit",
        "candidate_worktree_evidence": registry.get("candidate_worktree_evidence"),
        "coverage_complete": False,
        "playable_release_status": "BLOCKED",
        "method": {
            "candidate_source": "docs/persistence/economy_accounting/writers.json (source-reviewed registry, including legacy settlement definitions)",
            "semantic_scope": "Economy holdings, coin-pile value/custody, item custody/admission/destruction, relevant lifecycle and offline baseline/restore writers.",
            "count_rule": "Count source-function runtime mutation/projection routes separately from offline writers, dormant definitions, and non-writer/false candidates. A registry row is not proof of complete codebase coverage.",
            "false_positive_rule": "Do not count lexical helper hits, data members, reload projections, backup copies, tombstone checks, or unreferenced legacy definitions as live economic writers without a real mutation/reachable route.",
            "source_reachability": "Function definitions are located exactly; only specifically noted dormant symbols were confirmed definition-only. This matrix does not claim every callsite has been followed.",
        },
        "counts": counts,
        "disposition_counts": dict(sorted(dispositions.items())),
        "lexical_census": {
            "scanner": "scripts/validate_economy_accounting.py:scan_sources (comments and strings masked except SQL discovery)",
            "current_occurrences": len(census),
            "current_unique_path_line_family_sites": len(current_sites),
            "family_occurrences": family_counts,
            "draft_source_commit": registry.get("source_commit"),
            "draft_registry_census_complete": registry.get("census_complete", False),
            "draft_unique_sites": len(baseline_sites),
            "draft_to_current_unique_sites_removed": len(baseline_sites - current_sites),
            "draft_to_current_unique_sites_added": len(current_sites - baseline_sites),
            "mapped_unique_sites_from_draft_rows": len(mapped_sites),
            "mapped_sites_still_present": len(mapped_sites & current_sites),
            "unmapped_current_unique_sites": len(current_sites - mapped_sites),
            "interpretation": (f"Lexical occurrences are candidates, not writer routes. The {len(registry['writers'])}-row registry maps "
                               f"{len(mapped_sites & current_sites)} current unique sites; {len(current_sites - mapped_sites)} "
                               "current unique sites remain unmapped. Complete lexical mapping alone does not prove executable route coverage or release readiness."),
        },
        "global_schema_and_accounting_evidence": {
            "critical_command_schema_1": "src/persistence/critical_command.h:9",
            "critical_command_accounting_schema_2": "src/persistence/critical_command.h:10",
            "existing_gameplay_builders_remain_schema_1": "docs/persistence/economy_accounting/INTENT_DESIGN.md:10-17",
            "no_gameplay_accounting_activation": "docs/persistence/ECONOMY_ACCOUNTING.md:3-13; docs/persistence/economy_accounting/DELIVERY_PLAN.md:162-171",
            "legacy_currency_ledger": "migrations/currency_ledger.sql:44-78 (wallet/bank deltas, after-vectors, revisions; not a general balancing journal)",
            "legacy_item_custody_ledger": "migrations/item_ownership_ledger.sql:113-140 (from/to custody history, not monetary postings)",
            "schema_2_limitation": "Typed schema-2 item custody, typed player/room creation, and sourced spell consumption/item-action retirement are connected on SQL and flat-file; bank/baseline support is partial, while other destruction paths, money-valued item events, broader source bindings, and playable publication/save handoff remain unqualified.",
        },
        "blockers": [
            "Gameplay writers outside the qualified bank/baseline and eligible item custody slices still use schema-1 legs or direct legacy mutations; block those writers after accounting activation until typed schema-2 authorization and atomic evidence are attached.",
            (f"{len(current_sites - mapped_sites)} unique lexical candidate sites remain unmapped; classify real writers vs projections/cleanup/staging before release."
             if current_sites - mapped_sites else
             "Every current lexical candidate is mapped, but route reachability, backend behavior and executable evidence remain unqualified."),
            "The SQL coin component has balanced postings, but no complete gameplay route is qualified; legacy currency/item/domain ledgers are not substitutes.",
            "Every supported MySQL/MariaDB and flat-file backend needs same-root state/evidence/receipt atomicity and verified post-commit publication/reconnect handling.",
            "Baseline/cutover, source-event dedupe, item UID lifetime, lifecycle retirement and restore/reconciliation remain activation blockers.",
        ],
        "top_dependencies": [
            {"priority": 1, "issues": [475], "dependency": "Freeze the complete writer registry, account/reason policy, actor/source provenance and typed intent facts; resolve all unclassified writer candidates."},
            {"priority": 2, "issues": [476, 477, 478], "dependency": "Finish typed schema-2 execution/storage on SQL and flat-file with lifecycle registration, root transaction ownership, exact evidence, dedupe and failure/replay qualification."},
            {"priority": 3, "issues": [479, 480], "dependency": "Quiesced wallet/bank enrollment and baseline/cutover, connect real gameplay producers, then qualify commit-to-live publication/save/reconnect handoff."},
            {"priority": 4, "issues": [481, 482, 483, 484, 485, 486], "dependency": "Integrate every reward/cost, item create/destroy/move, shop, collector, auction, death, reset and recovery journey; boon reward also depends on prerequisite #601 full-result storage fix."},
            {"priority": 5, "issues": [487, 488, 489, 490], "dependency": "Complete reconciliation/corrections, lifecycle/retention and verified restore, then cross-domain fault and measured workload qualification before release."},
        ],
        "known_unknowns": [
            f"The matrix reconciles {len(registry['writers'])} registry rows and one dormant legacy auction settlement definition; it does not establish that these are all real writers.",
            (f"The {len(current_sites - mapped_sites)} unmapped lexical sites may include shared helpers, staging, recovery or unrelated mutations; review each before release."
             if current_sites - mapped_sites else
             "The lexical census is fully mapped; this does not establish that every reachable economic mutation has passed executable review."),
            "No production database or live service was used. Isolated SQL item-transfer tests and a flat-file sanitizer transaction/recovery harness passed; normal gameplay publication/save and remaining writer/backend routes are not qualified.",
            "Per-route source classification is source-level evidence only; transaction atomicity and actual runtime publication remain unverified unless separately qualified by the referenced subsystem tests.",
        ],
        "routes": routes,
    }
    return output


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if the checked-in artifact differs")
    args = parser.parse_args()
    artifact = build()
    encoded = json.dumps(artifact, indent=2, sort_keys=True) + "\n"
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != encoded:
            raise SystemExit("writer coverage matrix is stale; regenerate it")
    else:
        OUTPUT.write_bytes(encoded.encode("utf-8"))
    print("writer coverage matrix: " + json.dumps(artifact["counts"], sort_keys=True))
    print("lexical census: " + json.dumps({key: artifact["lexical_census"][key] for key in (
        "current_occurrences", "current_unique_path_line_family_sites", "mapped_sites_still_present",
        "unmapped_current_unique_sites", "draft_to_current_unique_sites_added", "draft_to_current_unique_sites_removed")}, sort_keys=True))
    print(f"artifact={OUTPUT.relative_to(ROOT)} coverage_complete={artifact['coverage_complete']} release={artifact['playable_release_status']}")


if __name__ == "__main__":
    main()
