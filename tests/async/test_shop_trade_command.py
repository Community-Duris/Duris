#!/usr/bin/env python3
"""Strict codec and fence regression for recoverable shop trades."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "shop_trade_command.h"

#include <cassert>
#include <cstring>

namespace
{
critical_operation_id operation()
{
	critical_operation_id id = {};
	id.bytes[0] = 0xa5;
	id.bytes.back() = 0x68;
	return id;
}

shop_trade_payload trade(shop_trade_action action)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = 42;
	payload.shop_id = 0;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "ShopTester");
	payload.price = 1250;
	payload.expected_wallet_revision = 7;
	payload.expected_bank_revision = 8;
	payload.expected_shop_revision = 9;
	payload.selected_item_uid = 100;
	payload.item_count = 2;
	const bool creates = action == shop_trade_action::buy_produced;
	const uint64_t revision = creates ? ITEM_TRANSFER_ABSENT_REVISION : 5;
	const item_custody_state state = creates ? item_custody_state::absent :
							 item_custody_state::active;
	payload.items[0] = { 100, 100, 0, revision, 500, state };
	payload.items[1] = { 101, 100, 100, revision, 501, state };
	payload.item_blob_size = 4;
	payload.item_blob[0] = 0x44;
	payload.item_blob[1] = 0x55;
	payload.item_blob[2] = 0x66;
	payload.item_blob[3] = 0x77;
	return payload;
}
} // namespace

int main()
{
	shop_trade_payload payload = trade(shop_trade_action::buy_existing);
	critical_command command = {};
	assert(shop_trade_command_build(&command, operation(), payload,
					critical_source_site::command,
					critical_deadline_class::interactive));
	assert(command.type == critical_command_type::shop_trade);
	command.accepted_at_usec = 1;
	assert(critical_command_valid(command));

	std::vector<uint8_t> command_bytes;
	assert(critical_command_encode(command, &command_bytes) ==
	       critical_command_codec_result::ok);
	critical_command command_copy = {};
	assert(critical_command_decode(command_bytes.data(), command_bytes.size(), &command_copy) ==
	       critical_command_codec_result::ok);
	shop_trade_payload decoded = {};
	assert(shop_trade_command_decode_payload(command_copy, &decoded));
	assert(decoded.action == shop_trade_action::buy_existing && decoded.shop_id == 0 &&
	       decoded.item_count == 2 && decoded.items[1].parent_item_uid == 100 &&
	       decoded.item_blob_size == 4 && decoded.item_blob[3] == 0x77);

	command_copy.expected_revisions.back().revision++;
	assert(!shop_trade_command_decode_payload(command_copy, &decoded));

	shop_trade_payload produced = trade(shop_trade_action::buy_produced);
	assert(shop_trade_command_build(&command, operation(), produced,
					critical_source_site::command,
					critical_deadline_class::interactive));
	produced.items[0].expected_state = item_custody_state::active;
	assert(!shop_trade_command_build(&command, operation(), produced,
					 critical_source_site::command,
					 critical_deadline_class::interactive));

	shop_trade_result result = {};
	result.action = shop_trade_action::sell_store;
	result.wallet = { { 1, 2, 3, 4 } };
	result.bank = { { 5, 6, 7, 8 } };
	result.wallet_revision = 10;
	result.bank_revision = 11;
	result.shop_revision = 12;
	result.player_owner_revision = 13;
	result.shop_owner_revision = 14;
	result.item_count = 2;
	result.item_uids[0] = 100;
	result.item_uids[1] = 101;
	result.item_revisions[0] = 6;
	result.item_revisions[1] = 6;
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> encoded_result = {};
	assert(shop_trade_command_encode_result(result, &encoded_result));
	shop_trade_result decoded_result = {};
	assert(shop_trade_command_decode_result(encoded_result.data(), encoded_result.size(),
						&decoded_result));
	assert(decoded_result.shop_revision == 12 && decoded_result.item_uids[1] == 101 &&
	       decoded_result.wallet.amount[3] == 4);
	encoded_result.back() = 1;
	assert(!shop_trade_command_decode_result(encoded_result.data(), encoded_result.size(),
						 &decoded_result));
	return 0;
}
'''


with tempfile.TemporaryDirectory(prefix="duris-shop-trade-command-") as temp_dir:
    source = Path(temp_dir) / "shop_trade_command_test.cpp"
    binary = Path(temp_dir) / "shop_trade_command_test"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "src/shop_trade_command.c",
            "src/item_transfer_command.c",
            "src/currency_command.c",
            "src/critical_command.c",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

print("[PASS] bounded recoverable shop-trade command codec")
