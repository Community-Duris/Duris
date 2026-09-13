#!/usr/bin/env python3
"""Auction acknowledgements publish both custody revisions before the next command."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, rel


def function(source, signature):
    start = source.index(signature)
    depth = 0
    for index in range(source.index("{", start), len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError("unbalanced function")


source = (SRC / "auction_transaction.c").read_text()
declarations = source[source.index("struct pending_auction"):source.index("enum class outbox_publication_state")]
publication = function(source, "bool publish(std::unordered_map<std::string, pending_auction>::iterator")
harness = r'''
#include "economy/auction_transaction.h"
#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <string>
#include <unordered_map>

bool currency_transaction_publish_balances(P_char, const char *, uint8_t,
    const currency_vector &, const currency_vector &, uint64_t, uint64_t) { return true; }
''' + declarations + publication + r'''

static uint64_t revision(item_owner_identity owner) {
    uint64_t value;
    assert(item_ownership_runtime_owner_revision(owner, &value));
    return value;
}

static void acknowledge(auction_action action, uint32_t actor, uint64_t player_revision,
                        uint64_t auction_revision, uint64_t item_revision,
                        bool committed = true) {
    pending_auction entry = {};
    entry.actor_pid = actor;
    entry.payload.action = action;
    entry.payload.actor_pid = actor;
    entry.payload.items[0] = {200, item_revision - 1, 391};
    entry.payload.item_count = 1;
    auction_command_result result = {};
    result.action = action;
    result.auction_id = 7;
    result.player_owner_revision = player_revision;
    result.auction_owner_revision = auction_revision;
    result.item_count = 1;
    result.item_uids[0] = 200;
    result.item_revisions[0] = item_revision;
    std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> bytes;
    assert(auction_command_encode_result(result, &bytes));
    entry.completed.outcome = committed ? critical_apply_outcome::applied : critical_apply_outcome::terminal_failure;
    entry.completed.result_size = bytes.size();
    std::copy(bytes.begin(), bytes.end(), entry.completed.result_payload.begin());
    pending.emplace("test", entry);
    char_data character = {};
    assert(publish(pending.find("test"), &character) == committed);
    assert(pending.empty());
}

int main() {
    const item_owner_identity seller = {item_owner_type::player, 42, 0};
    const item_owner_identity buyer = {item_owner_type::player, 43, 0};
    const item_owner_identity auction = {item_owner_type::auction, 7, 0};
    assert(item_ownership_runtime_hydrate({100, 100, 0, seller, 1, 4, 390, item_custody_state::active}));
    assert(item_ownership_runtime_hydrate({200, 200, 0, seller, 1, 4, 391, item_custody_state::active}));
    acknowledge(auction_action::list, 42, 5, 1, 2, false);
    assert(revision(seller) == 4 && revision(auction) == 0);
    acknowledge(auction_action::list, 42, 5, 1, 2);
    // prepare_coin_pile obtains the next bag put's expected revision here.
    assert(revision(seller) == 5 && revision(auction) == 1);
    item_ownership_runtime_entry bag, item;
    assert(item_ownership_runtime_lookup(100, &bag) && bag.owner.id == 42 && bag.item_revision == 1);
    assert(item_ownership_runtime_lookup(200, &item) && item.owner.type == item_owner_type::auction);
    acknowledge(auction_action::claim_item, 43, 1, 2, 3);
    assert(revision(buyer) == 1 && revision(auction) == 2);
    assert(item_ownership_runtime_lookup(200, &item) && item.owner.id == 43);
    // Another published transaction may have advanced the source owner first.
    assert(item_ownership_runtime_hydrate_owner(buyer, 4));
    acknowledge(auction_action::list, 43, 2, 3, 4);
    assert(revision(buyer) == 4 && revision(auction) == 3);
    item_ownership_runtime_reset();
    puts("auction source/destination custody publication: ok");
}
'''

with tempfile.TemporaryDirectory(prefix="auction-publication-") as temporary:
    cpp = Path(temporary) / "publication.cpp"
    binary = Path(temporary) / "publication"
    cpp.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
        "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
        "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql", str(cpp),
        rel("auction_command.c"), rel("currency_command.c"),
        rel("critical_command.c"), rel("item_transfer_command.c"),
        rel("item_ownership_runtime.c"), rel("player_snapshot_codec.c"),
        "-Wl,--gc-sections", "-lcrypto", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
