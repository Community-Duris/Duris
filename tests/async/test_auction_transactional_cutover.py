#!/usr/bin/env python3
"""Command-codec, route, schema, and publication contracts for auction cutover."""

from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


HARNESS = r'''
#include "auction_command.h"
#include <cassert>
#include <cstring>

int main()
{
    auction_command_payload payload = {};
    payload.action = auction_action::list;
    payload.actor_pid = 42;
    payload.racewar = 1;
    memcpy(payload.account_name.data(), "auction-test", 12);
    memcpy(payload.actor_name.data(), "Seller", 6);
    payload.expected_wallet_revision = 7;
    payload.expected_bank_revision = 9;
    payload.start_price = 1000;
    payload.buy_price = 5000;
    payload.listing_fee = 1020;
    payload.closing_fee_basis_points = 300;
    payload.bid_extension_seconds = 300;
    payload.end_time = 2000000000;
    payload.item_count = 2;
    payload.items[0] = {101, 3, 77};
    payload.items[1] = {102, 4, 77};
    memcpy(payload.object_blob.data(), "blob", 5);
    payload.object_blob_size = 5;
    memcpy(payload.object_short.data(), "a blade", 7);

    critical_operation_id id = {};
    assert(critical_operation_id_generate(&id));
    critical_command command = {};
    assert(auction_command_build(&command, id, payload, critical_source_site::command,
                                 critical_deadline_class::interactive));
    auction_command_payload decoded = {};
    assert(auction_command_decode_payload(command, &decoded));
    assert(decoded.item_count == 2 && decoded.items[1].item_uid == 102);
    assert(decoded.closing_fee_basis_points == 300);
    assert(command.keys.size() == 5);

    auction_command_result full_result = {};
    full_result.action = auction_action::list;
    full_result.event_type = auction_event_type::listed;
    full_result.item_count = AUCTION_COMMAND_MAX_ITEMS;
    for (size_t index = 0; index < full_result.item_count; ++index)
    {
        full_result.item_uids[index] = 1000 + index;
        full_result.item_revisions[index] = 10 + index;
    }
    std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> encoded_result = {};
    assert(auction_command_encode_result(full_result, &encoded_result));
    auction_command_result decoded_result = {};
    assert(auction_command_decode_result(encoded_result.data(), encoded_result.size(),
                                         &decoded_result));
    assert(decoded_result.item_count == AUCTION_COMMAND_MAX_ITEMS);

    critical_command tampered = command;
    tampered.expected_revisions.pop_back();
    assert(!auction_command_decode_payload(tampered, &decoded));
    tampered = command;
    tampered.keys.back().id = 999;
    assert(!auction_command_decode_payload(tampered, &decoded));

    payload.item_count = AUCTION_COMMAND_MAX_ITEMS + 1;
    assert(!auction_command_build(&command, id, payload, critical_source_site::command,
                                  critical_deadline_class::interactive));
    return 0;
}
'''


def section(source: str, start: str, end: str) -> str:
    return source[source.index(start):source.index(end, source.index(start))]


class AuctionTransactionalCutoverTests(unittest.TestCase):
    def test_bounded_codec_and_exact_fences(self):
        with tempfile.TemporaryDirectory() as directory:
            harness = Path(directory) / "auction_codec.cpp"
            binary = Path(directory) / "auction_codec"
            harness.write_text(HARNESS)
            subprocess.run(
                ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
                 str(harness), str(SRC / "critical_command.c"),
                 str(SRC / "currency_command.c"), str(SRC / "auction_command.c"),
                 "-lcrypto", "-o", str(binary)],
                check=True,
            )
            subprocess.run([str(binary)], check=True)

    def test_live_routes_submit_commands_and_publish_after_ack(self):
        source = (SRC / "auction_houses.c").read_text()
        source = source[source.index("#else", source.index("#ifdef __NO_MYSQL__")) :]
        routes = {
            "bool auction_offer(P_char": "bool auction_offer_legacy",
            "bool auction_bid(P_char": "bool auction_bid_legacy",
            "bool auction_pickup(P_char": "bool auction_pickup_legacy",
            "bool auction_remove(P_char": "bool auction_remove_legacy",
            "bool finalize_auction(int": "bool finalize_auction_legacy",
        }
        for start, end in routes.items():
            body = section(source, start, end)
            self.assertIn("auction_transaction_submit", body)
            self.assertNotIn("SUB_MONEY", body)
            self.assertNotIn("sql_begin_transaction", body)
        offer_callback = section(source, "void auction_list_completed(",
                                 "void auction_bid_completed(")
        self.assertLess(offer_callback.index("if (!committed)"),
                        offer_callback.index("obj_from_char"))
        self.assertNotIn("ws_broadcast_auction_new", offer_callback)
        publication = section(source, "bool auction_publish_committed_event(",
                              "// syntax: auction offer")
        self.assertIn("ws_broadcast_auction_new", publication)
        self.assertIn("ws_broadcast_auction_bid", publication)
        self.assertIn("ws_broadcast_auction_close", publication)
        transaction = (SRC / "auction_transaction.c").read_text()
        self.assertIn("auction_transaction_outbox_delivery", transaction)
        self.assertIn("outbox_publication_state::published", transaction)

    def test_client_free_read_and_pickup_routes_use_flat_catalog(self):
        source = (SRC / "auction_houses.c").read_text()
        no_mysql = section(source, "#ifdef __NO_MYSQL__", "#else")
        self.assertNotIn("Auctions are disabled", no_mysql)
        list_route = section(no_mysql, "bool auction_list(P_char", "bool auction_info(P_char")
        info_route = section(no_mysql, "bool auction_info(P_char", "bool auction_pickup(P_char")
        pickup_route = section(no_mysql, "bool auction_pickup(P_char", "bool auction_help(P_char")
        self.assertIn("flatfile_auction_list_open", list_route)
        self.assertIn("flatfile_auction_find_open", info_route)
        self.assertIn("flatfile_auction_find_pickup", pickup_route)
        self.assertIn("auction_transaction_submit", pickup_route)
        for route in (list_route, info_route, pickup_route):
            self.assertNotIn("qry(", route)
            self.assertNotIn("MYSQL_", route)

    def test_repository_owns_settlement_claims_ledgers_and_outbox(self):
        repository = (SRC / "auction_repository.c").read_text()
        generic = (SRC / "critical_command_repository.c").read_text()
        for token in (
            "FOR UPDATE", "apply_wallet_delta", "transition_items", "stage_money",
            "stage_items", "auction_item_custody", "auction_ledger",
            "claim_operation_id", "claimed_at=CURRENT_TIMESTAMP(6)",
        ):
            self.assertIn(token, repository)
        auction_branch = generic[generic.index("if (auction_command)"):]
        self.assertLess(auction_branch.index("insert_outbox"),
                        auction_branch.index('execute(connection, "COMMIT")'))
        self.assertLess(auction_branch.index("finish_inbox"),
                        auction_branch.index('execute(connection, "COMMIT")'))

    def test_schema_tools_are_additive_guarded_and_bootstrapped(self):
        migration = (ROOT / "migrations/auction_transactional_cutover.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        for token in (
            "auction_revision", "custody_state", "listing_operation_id",
            "auction_item_custody", "auction_ledger",
            "auction_reconciliation_quarantine", "claim_revision",
        ):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertNotIn("DELETE FROM auctions", migration)
        self.assertIn("auction_transactional_cutover.sql", runner)
        for script in ("apply_auction_transactional_cutover.sh",
                       "reconcile_auction_transactions.sh"):
            path = ROOT / "migrations" / script
            self.assertTrue(path.stat().st_mode & 0o111)
            text = path.read_text()
            self.assertIn("development/local/test", text)


if __name__ == "__main__":
    unittest.main()
