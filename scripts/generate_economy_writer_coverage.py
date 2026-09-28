#!/usr/bin/env python3
"""Build a source-anchored economy/item writer coverage matrix from the draft registry."""
from __future__ import annotations

import argparse
import ast
from collections import Counter
import importlib.util
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
# Requested worktree/source baseline before analysis artifacts are committed.
SOURCE_SNAPSHOT_COMMIT = "16f79bc90491772b7773d493cd9daf94dbce7676"
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

# Newly found old direct-SQL settlement implementation. No in-tree caller exists.
SUPPLEMENTAL = {
    "id": "auction.finalize_legacy",
    "path": "src/economy/auction_houses.c",
    "symbol": "finalize_auction_legacy",
    "reason": "auction_settle",
    "classification": "Retained direct-SQL auction settlement implementation; definition-only in the current tree.",
    "authority_boundary": "Legacy settlement definition has no in-tree caller; keep closed if revived.",
    "integration_issue": 485,
    "owner": "primary / #485",
    "test_candidates": [],
}

OFFLINE_WRITERS = {
    "import.legacy_dump": "Guarded local import replaces native rows from a legacy dump; it is an offline migration boundary, not a gameplay credit.",
    "import.currency_baselines": "Creates opening wallet/bank baseline witnesses for an authorized cutover; it must not alter live holdings or masquerade as issuance.",
    "restore.qualification": "Restores into an isolated qualification target; it is not permission to promote that target to live authority.",
}
DORMANT_WRITERS = {
    "currency.compat_sql_bank": "Direct SQL writer definition has no in-tree caller outside its own wrapper path; it is not a current gameplay route.",
    "auction.legacy_definitions": "Legacy offer mutator is definition-only in the current tree; the current offer path is separately routed.",
    "auction.finalize_legacy": "Legacy settlement mutator is definition-only in the current tree; no in-tree callsite found.",
}
NON_WRITERS = {
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
}
PROJECTION_ROUTES = {
    "world.generated_npc_hydration": "Recovery projection of encoded NPC state; never fresh issuance.",
    "world.npc_item_hydration": "Rehydrates existing NPC item identities into a live projection; no fresh creation posting.",
    "death.resurrection_publication": "Publishes the already-committed corpse lifecycle result; not a second custody/economic commit.",
    "recovery.pet_hydration": "Projects saved durable pet identity and inventory into runtime state; not item creation.",
}

# Current gameplay commands still build schema 1. The named families below have
# a source-backed typed critical-command path, sometimes alongside direct legacy
# effects. Everything else is direct/legacy, projection, or an offline tool.
SCHEMA1_IDS = {
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
    "item.creation_completion",
}
MIXED_SCHEMA1_IDS = {
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
SCHEMA2_ITEM_TRANSFER_IDS = {
    "item.command_movement", "item.bulk_movement", "item.movement_submit",
    "item.trusted_steal", "item.creation_completion",
    "death.corpse_creation", "death.resurrection_publication",
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
    for match in re.finditer(r"\b" + re.escape(function) + r"\s*\(", code):
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
        if tail >= len(code) or code[tail] != "{":
            continue
        line_start = code.rfind("\n", 0, match.start()) + 1
        prefix = code[line_start:match.start()].strip()
        if not prefix or any(word in prefix for word in ("return", "if ", "while ", "for ", "case ", "=")):
            continue
        result.append(code.count("\n", 0, match.start()) + 1)
    return sorted(set(result))


def source_targets(route_id: str, disposition: str) -> dict:
    if disposition == "non_writer_candidate":
        return {"holding_effect": "none in this function for economy/custody scope", "custody_effect": "none", "native_state_targets": []}
    if disposition == "dormant_writer_candidate":
        return {"holding_effect": "direct writer exists but no in-tree gameplay caller was found", "custody_effect": "legacy behavior if revived; not currently reached in-tree", "native_state_targets": ["legacy auction/bank persistence code; see route detail"]}
    if disposition == "offline_operational_writer":
        return {"holding_effect": OFFLINE_WRITERS[route_id], "custody_effect": "offline import/baseline/restore only; no live publication", "native_state_targets": ["explicit offline database/restore target", "opening wallet/bank/item baseline where applicable"]}
    if route_id.startswith("currency."):
        if route_id in {"currency.wallet_to_pile", "currency.pile_constructor", "currency.pile_add", "currency.room_pile_merge"}:
            return {"holding_effect": "Coin pile value and/or wallet-to-pile conversion; creation may be provisional until a live custody owner is published.", "custody_effect": "P_obj coin pile owner/list and durable item UID/coin payload when admitted.", "native_state_targets": ["PC.cash[0..3]", "P_obj.value[0..3]", "item_current_owner.coin_payload", "player_data.copper/silver/gold/platinum", "account_banks.bank_copper/bank_silver/bank_gold/bank_platinum"]}
        return {"holding_effect": "Player wallet denomination vector and/or shared account-bank denomination vector; preserve revisions and change-making exactly.", "custody_effect": "No item custody unless the route explicitly includes coin-pile movement.", "native_state_targets": ["PC.cash[0..3] / player_data.copper/silver/gold/platinum", "wallet_revision", "account_banks.bank_copper/bank_silver/bank_gold/bank_platinum", "bank_revision", "currency_ledger on applicable SQL paths"]}
    if route_id.startswith("coin."):
        return {"holding_effect": "Wallet denomination vector and durable coin-pile value; NPC recipient/compensation branches may still be in-memory/direct.", "custody_effect": "Coin pile UID and owner/root/parent transfer for floor/container custody.", "native_state_targets": ["PC.cash[0..3] / player_data denomination columns", "P_obj.value[0..3]", "item_current_owner.coin_payload", "item_current_owner owner/root/parent/revision", "item_ownership_ledger for covered custody path"]}
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
    elif route_id in SCHEMA1_IDS:
        current = 1
        mode = "schema_1_plus_direct_side_effects" if route_id in MIXED_SCHEMA1_IDS else "schema_1_typed_command_path"
        if route_id == "currency.split":
            mode = "multiple_schema_1_currency_legs_not_atomic_root"
        elif route_id in SCHEMA2_ITEM_TRANSFER_IDS:
            mode = "schema_1_when_inactive_schema_2_item_accounting_when_active"
    else:
        current = None
        mode = "legacy_direct_or_projection_without_a_schema_1_command_at_this_function"
    family = route_id.split(".", 1)[0]
    evidence = list(BUILDER_EVIDENCE.get(family, [])) if current == 1 else []
    evidence.extend(["src/persistence/critical_command.h:9-10", "docs/persistence/economy_accounting/INTENT_DESIGN.md:3-6,10-17"])
    if route_id == "currency.split":
        evidence.append("src/cmd/actoth.c:6232-6257 (recipient ADD_MONEY legs precede sender SUB_MONEY; no root command)")
    schema2_connected = route_id in SCHEMA2_ITEM_TRANSFER_IDS
    item_action_coverage = (
        " and spell creation/component consumption, sticks-to-snakes retirement, and key-break retirement"
        if route_id == "item.movement_submit" else ""
    )
    if schema2_connected:
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
        interpretation = f"Schema 1 remains the inactive/legacy path. When the accounting authority is active, eligible player get/drop/put/give moves, trusted-steal handoffs, typed-source player/room creation grants, and item-action consumption/destruction{item_action_coverage} use a frozen schema-2 intent on SQL and flat-file; each backend stores the item operation, source claim when required, exact custody references and result atomically. Other item destruction/extraction paths and money-valued item creation/destruction remain unsupported." if schema2_connected else "Schema 1 is the current legacy gameplay envelope; schema 2 is bounded/frozen-intent support, with only partial bank/baseline repository paths and the eligible item custody slice connected." if current == 1 else "No schema-2 gameplay writer is connected here; direct legacy paths must be blocked or migrated before activation." if disposition not in {"non_writer_candidate", "dormant_writer_candidate", "offline_operational_writer"} else mode
    if route_id == "currency.split":
        interpretation = "do_split has no atomic root command: recipient credits are individually submitted through schema-1 currency legs before the sender debit. No schema-2 multi-party operation is connected."
    return {
        "current_schema": current,
        "route_mode": mode,
        "accounting_intent_attached_by_current_gameplay_route": schema2_connected,
        "schema_2_gameplay_producer_connected": schema2_connected,
        "evidence": evidence,
        "interpretation": interpretation,
    }


def double_entry(route_id: str, disposition: str, schema: dict) -> dict:
    if disposition == "non_writer_candidate":
        return {"status": "not_applicable", "unified_operation_postings_observed": False, "existing_evidence": [], "note": NON_WRITERS[route_id]}
    if disposition == "dormant_writer_candidate":
        status = "not_executed_in_tree"
    elif disposition == "offline_operational_writer":
        status = "offline_baseline_or_restore_evidence_only"
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
        ] if route_id in SCHEMA2_ITEM_TRANSFER_IDS else [
            "economic_accounting_operation",
            "economic_accounting_account_effect",
            "balanced economic_accounting_coin_posting rows where currency changes",
            "linked item event references for custody-changing roots",
        ]),
        "global_evidence": ["docs/persistence/ECONOMY_ACCOUNTING.md:3-13", "docs/persistence/economy_accounting/DELIVERY_PLAN.md:162-171"],
        "note": ("The typed item-transfer owner persists exact SQL and flat-file custody references; complete writer coverage and route-specific gameplay acceptance are pending, so activation remains blocked."
                 if route_id in SCHEMA2_ITEM_TRANSFER_IDS else
                 "The accounting tables/codec are draft/partial infrastructure; no complete gameplay writer journey is qualified or activated."),
    }


def activation_policy(route_id: str, disposition: str, schema: dict) -> dict:
    if disposition == "non_writer_candidate":
        decision = "not_a_playable_economy_or_custody_writer"
        policy = NON_WRITERS[route_id]
    elif disposition == "dormant_writer_candidate":
        decision = "keep_unreachable_or_block_if_reactivated"
        policy = DORMANT_WRITERS[route_id] + " Any reactivation must route through the typed schema-2 writer before native mutation."
    elif disposition == "offline_operational_writer":
        decision = "maintenance_only_no_live_admission"
        policy = OFFLINE_WRITERS[route_id] + " Require a quiesced boundary, exact retained lineage/epoch, source snapshot witness and idempotent resume; fail closed on unknown rows."
    elif route_id in PROJECTION_ROUTES:
        decision = "allow_projection_only_with_committed_identity"
        policy = PROJECTION_ROUTES[route_id] + " Preserve existing UID/root/parent and receipt; emit no new issuance posting. Refuse mismatched/missing identity instead of minting or silently changing owner."
    else:
        decision = "block_until_typed_schema2_accounting"
        policy = "Reject before the first native/live mutation unless one immutable typed operation binds actor, source event, native identity/revisions, exact coin postings and ordered custody events; commit domain state, evidence and receipt atomically on the selected backend, then publish. Do not fall back to schema 1 or direct legacy mutation."
        if route_id in {"item.extraction", "item.scrap", "world.zone_purge", "lifecycle.sql_reset", "lifecycle.character_delete", "lifecycle.flat_character_delete", "lifecycle.sql_character_delete", "lifecycle.sql_account_delete", "lifecycle.flat_account_delete", "staff.purge", "staff.zone_reset", "lifecycle.reset_entry"}:
            policy += " For lifecycle/destruction paths, distinguish true retirement from staging, unload, rollback, reset reconstruction and recovery; only true destruction gets a linked custody tombstone and explicit coin sink."
        if route_id in {"currency.split"}:
            policy += " Split all participant effects under one root operation with every player fenced/validated before any recipient credit; current separate schema-1 credits and later sender debit are not atomic."
        if route_id.startswith("auction."):
            policy += " Keep accepted escrow, seller claim/proceeds, fee and item winner/return custody under the same auction root; no implicit reimbursement."
    return {"decision": decision, "must_block_on_activation": decision in {"block_until_typed_schema2_accounting", "keep_unreachable_or_block_if_reactivated"}, "required_policy": policy}


def build() -> dict:
    registry = json.loads(REGISTRY.read_text(encoding="utf-8"))
    validator = load_validator()
    census = validator.scan_sources(ROOT)
    source_commit = SOURCE_SNAPSHOT_COMMIT
    baseline_census = registry["census"]
    unique_key = lambda row: (row["path"], row["line"], row["family"])
    baseline_sites = {unique_key(row) for row in baseline_census}
    current_sites = {unique_key(row) for row in census}
    mapped_sites = {tuple(site) for row in registry["writers"] for site in row.get("sites", [])}
    family_counts = dict(sorted(Counter(row["family"] for row in census).items()))

    candidates = list(registry["writers"])
    candidates.append(SUPPLEMENTAL)
    routes = []
    for raw in candidates:
        route_id = raw["id"]
        file_path = SOURCE_FILE_FIXES.get(route_id, raw["path"])
        function = FUNCTION_FIXES.get(route_id, raw["symbol"])
        if function.startswith("def "):
            function = function[4:]
        if route_id == "kingdom.workshop_props":
            function = None
        locations = source_definition_lines(ROOT / file_path, function)
        if route_id != "kingdom.workshop_props" and not locations:
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
        details = source_targets(route_id, disposition)
        if route_id == "currency.split":
            details = {
                "holding_effect": "Issues one schema-1 currency credit leg per eligible group recipient, then submits sender debit for total given; remainder stays with sender. Recipient credits happen before sender debit, so failed debit can leave net issuance.",
                "custody_effect": "None; this route moves wallet value, not item UIDs.",
                "native_state_targets": ["eligible recipients' PC.cash[0..3] / player_data denomination columns", "sender PC.cash[0..3] / player_data denomination columns", "wallet_revision and currency_ledger on each applicable SQL leg"],
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
            route["reachability_evidence"] = "Definition exists; no in-tree callsite was found in the current source search."
        if route_id in NON_WRITERS:
            route["exclusion_reason"] = NON_WRITERS[route_id]
        if route_id in PROJECTION_ROUTES:
            route["projection_rule"] = PROJECTION_ROUTES[route_id]
        routes.append(route)

    dispositions = Counter(route["disposition"] for route in routes)
    counts = {
        "draft_registry_rows": len(registry["writers"]),
        "supplemental_candidates_found": 1,
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
        "coverage_complete": False,
        "playable_release_status": "BLOCKED",
        "method": {
            "candidate_source": "docs/persistence/economy_accounting/writers.json (draft registry), plus one supplemental legacy settlement writer found during source review",
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
            "interpretation": "Lexical occurrences are candidates, not writer routes. The 115-row draft maps only 33 unique sites; 2,646 current unique sites remain unmapped, so semantic completeness is unproven and release must remain blocked.",
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
            "Current function inventory is not codebase-complete: 2,646 unique lexical candidate sites are unmapped; classify real writers vs projections/cleanup/staging before release.",
            "No route in this matrix has unified operation-level double-entry evidence; legacy currency/item/domain ledgers are not substitutes.",
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
            "The matrix reconciles the 115 draft rows and one newly found dormant legacy auction settlement definition; it does not establish that these are all real writers.",
            "Most 2,679 current lexical sites are likely shared helpers, staging, recovery or unrelated mutations; they have not been semantically classified here.",
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
        OUTPUT.write_text(encoded, encoding="utf-8")
    print("writer coverage matrix: " + json.dumps(artifact["counts"], sort_keys=True))
    print("lexical census: " + json.dumps({key: artifact["lexical_census"][key] for key in (
        "current_occurrences", "current_unique_path_line_family_sites", "mapped_sites_still_present",
        "unmapped_current_unique_sites", "draft_to_current_unique_sites_added", "draft_to_current_unique_sites_removed")}, sort_keys=True))
    print(f"artifact={OUTPUT.relative_to(ROOT)} coverage_complete={artifact['coverage_complete']} release={artifact['playable_release_status']}")


if __name__ == "__main__":
    main()
