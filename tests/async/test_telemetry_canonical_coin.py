"""Focused inspection negatives; native positive authority is a separate SQL journey."""
from dataclasses import replace
import hashlib
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from scripts.telemetry import canonical_reward_contract as c
from scripts.telemetry.canonical_reward_coin import qualify_coin_root, COIN_CHILD_DOMAIN
from scripts.telemetry.canonical_reward_source import RetainedSource, replay_retained_bank_cut, source_cut_digest
from scripts.telemetry.canonical_reward_publication import make_generation
from test_telemetry_reward_projection import bank_evidence


def wallet_boundary():
    """Explicit byte/SQL boundary fixture; no native commit or gameplay claim."""
    original, legacy, old_inbox, effects, postings, _ = bank_evidence(disposition=c.Disposition.TRANSFER)
    op = original["operation_id"]
    intent = bytearray(original["canonical_intent"][:272])
    struct.pack_into("<I", intent, 8, 272)
    struct.pack_into("<I", intent, 12, 5)
    struct.pack_into("<H", intent, 24, 3)
    struct.pack_into("<I", intent, 104, 16)
    struct.pack_into("<2Q", intent, 256, 71, 81)
    header = bytearray(original["canonical_plan"][:256])
    struct.pack_into("<I", header, 84, 5)
    struct.pack_into("<H", header, 96, 3)
    header[152:184] = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
    struct.pack_into("<6I", header, 216, 2, 2, 2, 0, 0, 0)
    accounts = bytearray(original["canonical_plan"][256:496])
    struct.pack_into("<H", accounts, 138, 1)
    struct.pack_into("<Q", accounts, 148, 0)
    children = sorted((hashlib.sha256(op + struct.pack("<IQ", COIN_CHILD_DOMAIN, index)).digest()[:16], index)
                      for index in (0, 1))
    child_index = {endpoint: index + 1 for index, (_, endpoint) in enumerate(children)}
    postbytes = bytearray(original["canonical_plan"][496:])
    for index in (0, 1):
        struct.pack_into("<H", postbytes, index * 48 + 6, child_index[index])
    raw = bytes(header + accounts + postbytes) + b"".join(
        child + struct.pack("<IQHH", COIN_CHILD_DOMAIN, endpoint, 0, 1) for child, endpoint in children)
    plan = c.decode_plan(raw)
    root = dict(plan.metadata, canonical_plan=raw, canonical_intent=bytes(intent),
                plan_digest=hashlib.sha256(raw).digest(), outcome=1, result_code=0)
    effects[1]["account_key"] = plan.accounts[1].key
    for posting in postings:
        posting["child_index"] = child_index[posting["account_index"]]
    child_rows, wallets, inboxes, endpoint_results = [], [], [], {}
    for index, (child, endpoint) in enumerate(children, 1):
        child_rows.append(dict(operation_id=op, child_index=index, child_operation_id=child,
            domain_id=COIN_CHILD_DOMAIN, discriminator=endpoint, parent_index=0, relationship=1, receipt_operation_id=child))
        account = plan.accounts[endpoint]
        wallet = dict(legacy, operation_id=child, pid=7 + endpoint, reason_type=16,
                      wallet_revision=account.after_revision)
        for prefix, vector in (("wallet_delta_", account.delta), ("bank_delta_", (0,) * 4),
                               ("wallet_after_", account.after)):
            wallet.update({prefix + name: value for name, value in zip(c.COINS, vector)})
        wallets.append(wallet)
        result = struct.pack("<8q2Q", *account.after, *c._vector(wallet, "bank_after_"), account.after_revision, 10)
        inboxes.append(dict(old_inbox, operation_id=child, schema_version=1, result_payload=result))
        endpoint_results[endpoint] = result + bytes(48)
    inboxes.append(dict(old_inbox, command_type=17, result_payload=endpoint_results[0] + endpoint_results[1]))
    return [root, inboxes, effects, postings, [], child_rows, wallets, [], []]


def retained_cut(evidence):
    event = qualify_coin_root(*evidence)
    table_rows = {
        "economic_accounting_operation": [evidence[0]], "critical_operation_inbox": evidence[1],
        "economic_accounting_account_effect": evidence[2], "economic_accounting_coin_posting": evidence[3],
        "economic_accounting_child": evidence[5], "currency_ledger": evidence[6],
    }
    sources = []
    for reference in event.references:
        row = next(row for row in table_rows[reference.table]
                   if hashlib.sha256(c.encode_source_payload(row)).digest() == reference.payload_digest)
        sources.append(RetainedSource(reference, c.encode_source_payload(row)))
    sources = tuple(sorted(sources, key=lambda source: (source.reference.table, source.reference.key)))
    selected = (event.identity.operation_id,)
    return replay_retained_bank_cut(1000, selected, sources, source_cut_digest(1000, selected, sources))


class CanonicalCoinTest(unittest.TestCase):
    def test_one_root_preserves_children_without_earnings_or_inferred_pid(self):
        cut = retained_cut(wallet_boundary())
        event = cut.events[0]
        self.assertEqual(event.disposition, c.Disposition.TRANSFER)
        self.assertEqual(event.amount, 100)
        self.assertEqual(event.identity.participant_pid, 0)
        self.assertEqual(event.wallet_lifetime, 71)
        self.assertEqual(c.earned_totals(cut.events), {})
        self.assertEqual(len(event.references), 12)
        self.assertEqual(cut.coverage, "selected_native_economic_operations")

    def test_missing_or_changed_child_compatibility_and_inbox_refuse(self):
        mutations = (
            lambda rows: rows[1].pop(), lambda rows: rows[5].pop(), lambda rows: rows[6].pop(),
            lambda rows: rows[5][0].update(receipt_operation_id=bytes([77]) * 16),
            lambda rows: rows[6][0].update(wallet_delta_copper=999),
            lambda rows: rows[1][0].update(result_payload=rows[1][1]["result_payload"]),
            lambda rows: rows[1][0].update(durable_revision=9),
            lambda rows: rows[3][0].update(child_index=0),
            lambda rows: rows[0].update(writer_id=4),
        )
        for mutation in mutations:
            rows = wallet_boundary()
            mutation(rows)
            with self.subTest(mutation=mutation), self.assertRaises(c.EvidenceError):
                qualify_coin_root(*rows)

    def test_issuance_claim_cannot_promote_a_balanced_coin_root(self):
        rows = wallet_boundary()
        rows[4].append(dict(lineage=rows[0]["lineage"], source_event=bytes(48), operation_id=rows[0]["operation_id"], outcome=1))
        with self.assertRaises(c.EvidenceError):
            qualify_coin_root(*rows)

    def test_exact_overlap_publishes_one_transfer_and_unknown_dates(self):
        cut = retained_cut(wallet_boundary())
        second = replace(cut, captured_utc_usec=1001,
            source_digest=source_cut_digest(1001, cut.selected_operations, cut.sources))
        plan = make_generation(bytes([19]) * 16, 2000, tuple(sorted((cut, second), key=lambda value: value.source_digest)), ())
        coverage = c.decode_source_payload(plan.header["coverage_payload"])
        self.assertEqual(len(plan.events), 1)
        self.assertIsNone(coverage["currency_earned_copper"])
        self.assertEqual(coverage["transfer_count"], 1)
        self.assertEqual(coverage["account_unknown_count"], 1)
        self.assertIn("native_coin_wallet_pile", coverage["supported_routes"])

    def test_unlinked_legacy_child_is_a_cut_scope_refusal(self):
        cut = retained_cut(wallet_boundary())
        sources = tuple(row for row in cut.sources if row.reference.table != "economic_accounting_child")
        with self.assertRaises(c.EvidenceError):
            replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, sources,
                source_cut_digest(cut.captured_utc_usec, cut.selected_operations, sources), strict=False)

    def test_bank_sweep_health_cannot_claim_coin_history_coverage(self):
        cut = retained_cut(wallet_boundary())
        with self.assertRaisesRegex(c.EvidenceError, "canonical_coin_history_health_unavailable"):
            make_generation(bytes([19]) * 16, 2000, (cut,), (), health_payloads=(b"not coin history",))

    def test_ambiguous_child_parentage_refuses_even_in_quarantine(self):
        cut = retained_cut(wallet_boundary())
        child = next(row for row in cut.sources if row.reference.table == "economic_accounting_child")
        row = c.decode_source_payload(child.payload)
        row["child_index"] = 3
        key = row["operation_id"] + b"\x00\x03"
        extra = RetainedSource(c.source_reference(child.reference.table, key, row), c.encode_source_payload(row))
        sources = tuple(sorted((*cut.sources, extra), key=lambda source: (source.reference.table, source.reference.key)))
        with self.assertRaises(c.EvidenceError):
            replay_retained_bank_cut(1000, cut.selected_operations, sources,
                source_cut_digest(1000, cut.selected_operations, sources), strict=False)


if __name__ == "__main__":
    unittest.main()
