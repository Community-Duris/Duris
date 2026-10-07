#!/usr/bin/env python3
"""Native pure owner bytes plus explicit SQL boundary inspection negatives.

These cells are constructed fixtures. Positive database commit, source capture,
retention and publication qualification must use the native SQL owner journey.
"""
from copy import deepcopy
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import canonical_reward_contract as c
from scripts.telemetry.canonical_reward_auction import qualify_money_claim, inspect_settlement_credit
from scripts.telemetry.canonical_reward_source import RetainedSource, replay_retained_bank_cut, source_cut_digest
from scripts.telemetry.canonical_reward_publication import make_generation


def native_boundary(parts):
    intent, raw, payload = (bytes.fromhex(part) for part in parts)
    plan = c.decode_plan(raw)
    op = plan.metadata["operation_id"]
    root = dict(plan.metadata, canonical_plan=raw, canonical_intent=intent,
                plan_digest=hashlib.sha256(raw).digest(), outcome=1, result_code=0)
    inbox = dict(operation_id=op, command_type=7, schema_version=2, payload_version=1,
                 status=1, result_code=0, failure_stage=0, result_payload=payload,
                 durable_revision=struct.unpack_from("<Q", payload, 118)[0],
                 command_hash=bytes([13]) * 32, keys_hash=bytes([14]) * 32)
    effects, postings = [], []
    for index, account in enumerate(plan.accounts):
        row = dict(operation_id=op, account_index=index, account_key=account.key,
                   before_revision=account.before_revision, after_revision=account.after_revision)
        for prefix, values in (("before_", account.before), ("after_", account.after)):
            row.update({prefix + name: value for name, value in zip(c.COINS, values, strict=True)})
        effects.append(row)
    for index, posting in enumerate(plan.postings):
        row = dict(operation_id=op, line_index=index, event_index=posting.event_index,
                   account_index=posting.account_index, child_index=posting.child_index,
                   copper_value=posting.copper_value)
        row.update({"delta_" + name: value for name, value in zip(c.COINS, posting.delta, strict=True)})
        postings.append(row)
    event, auction, status, seller, winner = struct.unpack_from("<B4I", payload, 1)
    ledger = dict(operation_id=op, event_type=event, auction_id=auction,
                  auction_revision=inbox["durable_revision"], actor_pid=0 if event == 3 else 100,
                  counterparty_pid=seller, value_delta=struct.unpack_from("<q", payload, 30)[0],
                  final_price=struct.unpack_from("<q", payload, 22)[0], item_count=0)
    return dict(root=root, inbox=inbox, effects=effects, postings=postings, ledger=ledger)


def claim_boundary(exports):
    sources = {bundle["root"]["operation_id"]: bundle for bundle in map(native_boundary, exports[:2])}
    claim = native_boundary(exports[2])
    plan = c.decode_plan(claim["root"]["canonical_plan"])
    wallet, pending = plan.accounts
    op = claim["root"]["operation_id"]
    allocations = []
    for source, bundle in sources.items():
        native = c.decode_plan(bundle["root"]["canonical_plan"])
        amount = next(account.delta[0] for account in native.accounts if account.kind == 5)
        allocations.append(dict(source_operation_id=source, source_slot=2, lineage=plan.metadata["lineage"],
                                claim_mapping_id=pending.authority_id, beneficiary_pid=100, amount=amount,
                                claim_operation_id=op))
    legacy = dict(operation_id=op, pid=100, bank_id=3, reason_type=11, reason_id=0, source_site=1,
                  wallet_revision=9, bank_revision=3)
    for prefix, values in (("wallet_delta_", wallet.delta), ("bank_delta_", (0,) * 4),
                           ("wallet_after_", wallet.after), ("bank_after_", (5, 0, 0, 0))):
        legacy.update({prefix + name: value for name, value in zip(c.COINS, values, strict=True)})
    claims = [dict(lineage=plan.metadata["lineage"], source_event=plan.metadata["source_event"],
                   operation_id=op, outcome=1)]
    return [claim["root"], legacy, claim["inbox"], claim["effects"], claim["postings"], claims,
            claim["ledger"], allocations, sources]


def rebind_intent(root, change):
    """Keep binary/hash structure coherent while testing owner-specific facts."""
    intent = bytearray(root["canonical_intent"])
    change(intent)
    raw = bytearray(root["canonical_plan"])
    raw[152:184] = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
    root.update(canonical_intent=bytes(intent), canonical_plan=bytes(raw),
                plan_digest=hashlib.sha256(raw).digest(), intent_digest=bytes(raw[152:184]))


def retained_boundary(rows, *, strict=True, captured=1000):
    inventory = {"economic_accounting_operation": [rows[0]], "currency_ledger": [rows[1]],
        "critical_operation_inbox": [rows[2]], "economic_accounting_account_effect": rows[3],
        "economic_accounting_coin_posting": rows[4], "economic_accounting_source_claim": rows[5],
        "auction_ledger": [rows[6]], "economic_pending_claim_source": rows[7]}
    for bundle in rows[8].values():
        for table, field in (("economic_accounting_operation", "root"), ("critical_operation_inbox", "inbox"),
            ("economic_accounting_account_effect", "effects"), ("economic_accounting_coin_posting", "postings"),
            ("auction_ledger", "ledger")):
            inventory[table].extend(bundle[field] if field in ("effects", "postings") else [bundle[field]])
    sources = []
    for table, family in inventory.items():
        for row in family:
            if table == "economic_pending_claim_source":
                key = row["source_operation_id"] + row["source_slot"].to_bytes(2, "big")
            elif table == "economic_accounting_source_claim":
                key = row["lineage"] + row["source_event"]
            elif table in ("economic_accounting_account_effect", "economic_accounting_coin_posting"):
                key = row["operation_id"] + row["account_index" if table.endswith("effect") else "line_index"].to_bytes(2, "big")
            else:
                key = row["operation_id"]
            sources.append(RetainedSource(c.source_reference(table, key, row), c.encode_source_payload(row)))
    sources = tuple(sorted(sources, key=lambda source: (source.reference.table, source.reference.key)))
    selected = (rows[0]["operation_id"],)
    return replay_retained_bank_cut(captured, selected, sources, source_cut_digest(captured, selected, sources), strict=strict)


class CanonicalAuctionTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        work = ROOT / "bin/tests/canonical-auction-contract"
        work.mkdir(parents=True, exist_ok=True)
        binary = work / "native-auction-contract"
        subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic",
            "-Werror", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-D__NO_MYSQL__", "-Isrc/no_mysql", "-Isrc",
            "tests/async/telemetry_canonical_auction_contract_harness.cpp", "src/economy/auction_money_claim_accounting.c",
            "src/economy/auction_settlement_accounting.c", "src/economy/auction_command.c", "src/economy/currency_command.c",
            "src/economy/economic_accounting_types.c", "src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_intent.c",
            "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
            "src/player/player_snapshot_codec.c", "src/persistence/critical_command.c", "-lcrypto", "-o", str(binary)],
            cwd=ROOT, check=True)
        output = subprocess.check_output([str(binary)], cwd=ROOT, text=True, timeout=30)
        cls.exports = [line.split()[1:] for line in output.splitlines() if line.startswith("TELEMETRY_AUCTION_PLAN ")]
        if len(cls.exports) != 3 or any(len(parts) != 3 for parts in cls.exports):
            raise AssertionError("three actual native owner plans required")

    def fixture(self):
        return claim_boundary(self.exports)

    def test_native_owner_claim_is_one_transfer_with_entire_source_inventory(self):
        rows = self.fixture()
        event = qualify_money_claim(*rows)
        self.assertEqual(event.disposition, c.Disposition.TRANSFER)
        self.assertEqual(event.amount, 491)
        self.assertEqual(event.identity.participant_pid, 0)
        self.assertEqual(event.wallet_lifetime, 11)
        self.assertEqual(c.earned_totals((event, event)), {})
        self.assertEqual(len(c.reconcile_events((event, event))), 1)
        self.assertEqual(len(event.references), 27)
        self.assertEqual(sum(ref.table == "economic_pending_claim_source" for ref in event.references), 2)

    def test_missing_duplicate_reordered_changed_and_wrong_bound_allocations_refuse(self):
        mutations = (
            lambda rows: rows[7].pop(), lambda rows: rows[7].reverse(),
            lambda rows: rows[7].__setitem__(1, deepcopy(rows[7][0])),
            lambda rows: rows[7][1].update(amount=201),
            lambda rows: rows[7][1].update(source_slot=1),
            lambda rows: rows[7][1].update(beneficiary_pid=101),
            lambda rows: rows[7][1].update(claim_mapping_id=14),
            lambda rows: rows[7][1].update(claim_operation_id=None),
            lambda rows: rows[7][1].update(lineage=bytes([99]) * 16),
        )
        for mutation in mutations:
            rows = self.fixture()
            mutation(rows)
            with self.subTest(mutation=mutation), self.assertRaises(c.EvidenceError):
                qualify_money_claim(*rows)

    def test_first_service_source_cannot_substitute_for_second_native_source(self):
        rows = self.fixture()
        rows[8].pop(rows[7][1]["source_operation_id"])
        with self.assertRaises(c.EvidenceError):
            qualify_money_claim(*rows)
        rows = self.fixture()
        rows[8][bytes([88]) * 16] = deepcopy(next(iter(rows[8].values())))
        with self.assertRaises(c.EvidenceError):
            qualify_money_claim(*rows)

    def test_synthetic_or_changed_upstream_native_receipts_refuse(self):
        mutations = (
            lambda source: source["root"].update(writer_id=99),
            lambda source: source["root"].update(canonical_plan=b"fake"),
            lambda source: source["inbox"].update(status=0),
            lambda source: source["ledger"].update(counterparty_pid=101),
            lambda source: source["effects"][1].update(after_copper=999),
            lambda source: source["postings"][1].update(copper_value=999),
            lambda source: source["inbox"].update(durable_revision=2),
        )
        for mutation in mutations:
            rows = self.fixture()
            mutation(next(iter(rows[8].values())))
            with self.subTest(mutation=mutation), self.assertRaises(c.EvidenceError):
                qualify_money_claim(*rows)

    def test_supported_upstream_sale_handles_fee_and_zero_fee_without_issuance(self):
        rows = self.fixture()
        for allocation in rows[7]:
            bundle = rows[8][allocation["source_operation_id"]]
            refs = inspect_settlement_credit(bundle["root"], bundle["inbox"], bundle["effects"],
                                             bundle["postings"], bundle["ledger"], allocation)
            self.assertEqual(len(refs), 9 if allocation["amount"] == 291 else 7)

    def test_claim_compatibility_service_and_result_changes_refuse(self):
        mutations = (
            lambda rows: rows[1].update(reason_type=5),
            lambda rows: rows[1].update(wallet_delta_copper=99),
            lambda rows: rows[1].update(bank_delta_copper=1),
            lambda rows: rows[1].update(pid=101),
            lambda rows: rows[6].update(event_type=3),
            lambda rows: rows[5].clear(),
            lambda rows: rows[2].update(result_payload=rows[2]["result_payload"][:-1] + b"\x01"),
            lambda rows: rows[2].update(durable_revision=10),
        )
        for mutation in mutations:
            rows = self.fixture()
            mutation(rows)
            with self.subTest(mutation=mutation), self.assertRaises(c.EvidenceError):
                qualify_money_claim(*rows)

    def test_exact_facts_digest_not_just_source_count_or_total(self):
        rows = self.fixture()
        rebind_intent(rows[0], lambda intent: intent.__setitem__(304, intent[304] ^ 1))
        with self.assertRaisesRegex(c.EvidenceError, "source_inventory"):
            qualify_money_claim(*rows)

    def test_refuse_native_maximum_when_it_exceeds_projection_reference_budget(self):
        rows = self.fixture()
        rebind_intent(rows[0], lambda intent: struct.pack_into("<I", intent, 300, 4096))
        with self.assertRaisesRegex(c.EvidenceError, "facts_or_capacity"):
            qualify_money_claim(*rows)

    def test_retained_dependencies_and_overlap_publish_one_custody_event(self):
        cuts = tuple(sorted((retained_boundary(self.fixture()), retained_boundary(self.fixture(), captured=1001)),
                            key=lambda cut: cut.source_digest))
        plan = make_generation(bytes([19]) * 16, 2000, cuts, ())
        coverage = c.decode_source_payload(plan.header["coverage_payload"])
        self.assertEqual(len(plan.events), 1)
        self.assertEqual(coverage["transfer_count"], 1)
        self.assertIsNone(coverage["currency_earned_copper"])
        self.assertIn("native_auction_money_claim_settlement", coverage["supported_routes"])
        self.assertNotIn("native_coin_wallet_pile", coverage["supported_routes"])
        self.assertIn("auction_item_claim", coverage["unsupported_routes"])
        self.assertEqual(coverage["account_unknown_count"], 1)

    def test_changed_dependency_quarantines_same_root_without_second_participant_event(self):
        original = retained_boundary(self.fixture())
        rows = self.fixture()
        next(iter(rows[8].values()))["ledger"]["counterparty_pid"] = 101
        broken = retained_boundary(rows, strict=False, captured=1001)
        self.assertEqual(broken.events[0].disposition, c.Disposition.UNKNOWN)
        self.assertEqual(broken.events[0].identity, original.events[0].identity)
        plan = make_generation(bytes([19]) * 16, 2000, tuple(sorted((original, broken), key=lambda cut: cut.source_digest)), ())
        self.assertEqual(len(plan.events), 1)
        self.assertEqual(plan.events[0].disposition, c.Disposition.CONFLICT)
        self.assertIsNone(plan.events[0].amount)

    def test_missing_allocation_cannot_attach_orphan_upstream_receipts(self):
        rows = self.fixture()
        rows[7].pop()
        with self.assertRaises(c.EvidenceError):
            retained_boundary(rows, strict=False)

    def test_bank_history_health_cannot_claim_auction_discovery(self):
        cut = retained_boundary(self.fixture())
        with self.assertRaisesRegex(c.EvidenceError, "history_health_unavailable"):
            make_generation(bytes([19]) * 16, 2000, (cut,), (), health_payloads=(b"bank only",))


if __name__ == "__main__":
    unittest.main()
