#include "currency_transaction.h"
#include "item_ownership_runtime.h"
#include "shop_trade_transaction.h"
#include "utils.h"

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

namespace
{
char_data character = {};
pc_only_data pc = {};
bool player_online = true;
critical_command submitted_command = {};
bool currency_published = false;
item_transfer_payload published_transfer = {};
item_transfer_result published_transfer_result = {};
bool ownership_published = false;
bool shop_revision_published = false;
bool completion_called = false;
bool completion_committed = false;
unsigned int completion_error = 0;

shop_trade_payload trade(shop_trade_action action, uint64_t uid)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = 42;
	payload.shop_id = 7;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "shop-account");
	payload.price = 100;
	payload.expected_wallet_revision = 2;
	payload.expected_bank_revision = 3;
	payload.expected_shop_revision = 4;
	payload.selected_item_uid = uid;
	payload.target_root_item_uid = uid;
	payload.item_count = 1;
	payload.item_blob_size = 1;
	payload.item_blob[0] = 0x5a;
	if (action == shop_trade_action::buy_produced)
	{
		payload.stock_item_uid = 900;
		payload.expected_stock_item_revision = 6;
		payload.stock_vnum = 800;
		payload.items[0] = { uid, uid,
				     0,	  ITEM_TRANSFER_ABSENT_REVISION,
				     800, item_custody_state::absent };
	}
	else
		payload.items[0] = { uid, uid, 0, 5, 800, item_custody_state::active };
	return payload;
}

shop_trade_result result(shop_trade_action action, uint64_t uid, uint64_t player_revision,
			 uint64_t counterparty_revision)
{
	shop_trade_result value = {};
	value.action = action;
	value.wallet = { { 0, 0, 9, 9 } };
	value.bank = { { 0, 0, 0, 0 } };
	value.wallet_revision = 3;
	value.bank_revision = 4;
	value.shop_revision = 5;
	value.player_owner_revision = player_revision;
	value.counterparty_owner_revision = counterparty_revision;
	value.item_count = 1;
	value.item_uids[0] = uid;
	value.item_revisions[0] = action == shop_trade_action::buy_produced ? 1 : 6;
	return value;
}

critical_completion completion(const shop_trade_result &result)
{
	critical_completion value = {};
	value.operation_id = submitted_command.operation_id;
	value.outcome = critical_apply_outcome::applied;
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> encoded = {};
	assert(shop_trade_command_encode_result(result, &encoded));
	value.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), value.result_payload.begin());
	return value;
}

void completed(P_char completed_character, bool committed, const shop_trade_result &,
	       unsigned int error_code, const shop_trade_payload &)
{
	assert(completed_character == &character);
	completion_called = true;
	completion_committed = committed;
	completion_error = error_code;
}
} // namespace

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	submitted_command = std::move(command);
	return critical_submit_result::accepted;
}

P_char find_player_by_pid(int pid)
{
	return player_online && pid == 42 ? &character : nullptr;
}

bool currency_transaction_publish_balances(P_char published_character, const char *account_name,
					   uint8_t racewar, const currency_vector &,
					   const currency_vector &, uint64_t wallet_revision,
					   uint64_t bank_revision)
{
	assert(published_character == &character && !strcmp(account_name, "shop-account") &&
	       racewar == 1 && wallet_revision == 3 && bank_revision == 4);
	currency_published = true;
	return true;
}

bool item_ownership_runtime_apply(const item_transfer_payload &payload,
				  const item_transfer_result &result)
{
	published_transfer = payload;
	published_transfer_result = result;
	ownership_published = true;
	return true;
}

bool shop_trade_runtime_can_advance(uint32_t shop_id, uint64_t expected_revision,
				    uint64_t new_revision)
{
	return shop_id == 7 && expected_revision == 4 && new_revision == 5;
}

bool shop_trade_runtime_advance(uint32_t shop_id, uint64_t expected_revision, uint64_t new_revision)
{
	shop_revision_published =
		shop_trade_runtime_can_advance(shop_id, expected_revision, new_revision);
	return shop_revision_published;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main()
{
	character.only.pc = &pc;
	pc.pid = 42;

	shop_trade_payload produced = trade(shop_trade_action::buy_produced, 300);
	produced.target_root_item_uid = 700;
	produced.target_parent_item_uid = 700;
	produced.expected_target_parent_revision = 5;
	assert(shop_trade_transaction_submit(&character, produced, completed));
	assert(shop_trade_transaction_player_busy(&character));
	assert(!shop_trade_transaction_submit(&character, produced, completed));
	critical_completion produced_completion =
		completion(result(shop_trade_action::buy_produced, 300, 8, 6));
	shop_trade_transaction_handle_completions(&produced_completion, 1);
	assert(currency_published && ownership_published && shop_revision_published &&
	       completion_called && completion_committed && completion_error == 0 &&
	       published_transfer.from_owner.type == item_owner_type::system &&
	       published_transfer.to_owner.type == item_owner_type::player &&
	       published_transfer.target_root_item_uid == 700 &&
	       published_transfer.target_parent_item_uid == 700 &&
	       published_transfer.expected_target_parent_revision == 5 &&
	       published_transfer_result.from_owner_revision == 6 &&
	       published_transfer_result.to_owner_revision == 8 &&
	       !shop_trade_transaction_player_busy(&character));

	currency_published = ownership_published = shop_revision_published = completion_called =
		completion_committed = false;
	const shop_trade_payload destroyed = trade(shop_trade_action::sell_destroy, 301);
	assert(shop_trade_transaction_submit(&character, destroyed, completed));
	critical_completion destroyed_completion =
		completion(result(shop_trade_action::sell_destroy, 301, 9, 2));
	player_online = false;
	shop_trade_transaction_handle_completions(&destroyed_completion, 1);
	assert(shop_trade_transaction_player_busy(&character) && !currency_published &&
	       !ownership_published && !completion_called);
	player_online = true;
	shop_trade_transaction_player_ready(&character);
	assert(currency_published && ownership_published && shop_revision_published &&
	       completion_called && completion_committed &&
	       published_transfer.from_owner.type == item_owner_type::player &&
	       published_transfer.to_owner.type == item_owner_type::destruction &&
	       published_transfer_result.from_owner_revision == 9 &&
	       published_transfer_result.to_owner_revision == 2 &&
	       !shop_trade_transaction_player_busy(&character));

	shop_trade_transaction_reset_for_tests();
}
