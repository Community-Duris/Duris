#include "economy/coin_transfer_accounting.h"
#include "economy/economic_accounting_plan.h"
#include "economy/economic_accounting_types.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <openssl/sha.h>
#include <string>
#include <vector>

#ifdef __NO_MYSQL__

bool coin_transfer_accounting_record(MYSQL *, const critical_command &,
				     const coin_transfer_payload &, const coin_transfer_result &)
{
	errno = ENOTSUP;
	return false;
}

#else

namespace
{
std::string hex(std::span<const uint8_t> data)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string value = "X'";
	value.reserve(data.size() * 2 + 3);
	for (auto byte : data)
	{
		value += digits[byte >> 4];
		value += digits[byte & 15];
	}
	value += '\'';
	return value;
}

std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
}

std::array<uint8_t, 32> sha256(std::span<const uint8_t> data)
{
	std::array<uint8_t, 32> digest = {};
	SHA256(data.data(), data.size(), digest.data());
	return digest;
}

bool execute(MYSQL *connection, const std::string &sql)
{
	return mysql_real_query(connection, sql.data(), sql.size()) == 0;
}

bool get_active_epoch(MYSQL *connection, critical_operation_id *lineage,
		      critical_operation_id *epoch, const critical_operation_id &creating_operation)
{
	static const char QUERY[] =
		"SELECT lineage, active_epoch FROM economic_lineage_state WHERE active_epoch IS NOT NULL LIMIT 1";
	if (mysql_real_query(connection, QUERY, sizeof(QUERY) - 1) != 0)
		return false;

	MYSQL_RES *res = mysql_store_result(connection);
	if (!res)
		return false;

	MYSQL_ROW row = mysql_fetch_row(res);
	if (row && row[0] && row[1])
	{
		unsigned long *lengths = mysql_fetch_lengths(res);
		if (lengths && lengths[0] == 16 && lengths[1] == 16)
		{
			std::memcpy(lineage->bytes.data(), row[0], 16);
			std::memcpy(epoch->bytes.data(), row[1], 16);
			mysql_free_result(res);
			return true;
		}
	}
	mysql_free_result(res);

	// Check if any epoch exists in economic_epoch
	static const char QUERY_EPOCH[] = "SELECT lineage, epoch FROM economic_epoch LIMIT 1";
	if (mysql_real_query(connection, QUERY_EPOCH, sizeof(QUERY_EPOCH) - 1) == 0)
	{
		MYSQL_RES *res2 = mysql_store_result(connection);
		if (res2)
		{
			MYSQL_ROW row2 = mysql_fetch_row(res2);
			if (row2 && row2[0] && row2[1])
			{
				unsigned long *lengths = mysql_fetch_lengths(res2);
				if (lengths && lengths[0] == 16 && lengths[1] == 16)
				{
					std::memcpy(lineage->bytes.data(), row2[0], 16);
					std::memcpy(epoch->bytes.data(), row2[1], 16);
					mysql_free_result(res2);
					return true;
				}
			}
			mysql_free_result(res2);
		}
	}

	// Create bootstrap lineage and epoch
	lineage->bytes.fill(0);
	lineage->bytes[0] = 0x01;
	epoch->bytes.fill(0);
	epoch->bytes[0] = 0x01;

	std::string epoch_sql =
		"INSERT IGNORE INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,creating_operation_id) VALUES(" +
		id(*lineage) + "," + id(*epoch) + ",1,NULL,1,REPEAT(CHAR(0),32)," +
		id(creating_operation) + ")";
	if (!execute(connection, epoch_sql))
		return false;

	std::string lineage_sql =
		"INSERT INTO economic_lineage_state(lineage,active_epoch,revision) VALUES(" +
		id(*lineage) + "," + id(*epoch) +
		",1) ON DUPLICATE KEY UPDATE active_epoch=VALUES(active_epoch)";
	if (!execute(connection, lineage_sql))
		return false;

	return true;
}
} // namespace

bool coin_transfer_accounting_record(MYSQL *connection, const critical_command &root_command,
				     const coin_transfer_payload &payload,
				     const coin_transfer_result &result)
{
	if (!connection)
	{
		errno = EINVAL;
		return false;
	}

	critical_operation_id lineage = {}, epoch = {};
	if (!get_active_epoch(connection, &lineage, &epoch, root_command.operation_id))
		return false;

	uint64_t actor_id = 0;
	if (!payload.source.change.keys.empty())
		actor_id = payload.source.change.keys[0].id;
	else
		actor_id = 1;

	// Calculate copper value of transfer
	const int64_t multipliers[4] = { 1, 10, 100, 1000 };
	int64_t copper_value = 0;
	economic_coin_vector delta_vector = {};
	for (size_t i = 0; i < 4; ++i)
	{
		int64_t diff = static_cast<int64_t>(payload.destination.after[i]) -
			       payload.destination.before[i];
		delta_vector[i] = diff;
		copper_value += diff * multipliers[i];
	}

	// Build plan
	economic_accounting_plan plan = {};
	plan.metadata.version = 1;
	plan.metadata.lineage = lineage;
	plan.metadata.epoch = epoch;
	plan.metadata.operation_id = root_command.operation_id;
	plan.metadata.actor_kind = economic_actor_kind::domain;
	plan.metadata.actor_id = actor_id;
	plan.metadata.writer_id = 3; // coin_transfer / wallet_to_pile
	plan.metadata.policy_version = 1;
	plan.metadata.compiler_version = 1;
	plan.metadata.reason = economic_reason::coin_transfer;

	// Accounts: 0 = Wallet, 1 = Pile
	economic_account_effect wallet_effect = {};
	wallet_effect.key = { lineage, economic_account_kind::wallet, actor_id, 0 };
	for (size_t i = 0; i < 4; ++i)
	{
		wallet_effect.before[i] = payload.source.before[i];
		wallet_effect.after[i] = payload.source.after[i];
	}
	wallet_effect.before_revision =
		result.wallets[0].wallet_revision > 0 ? result.wallets[0].wallet_revision - 1 : 0;
	wallet_effect.after_revision = result.wallets[0].wallet_revision;
	plan.accounts.push_back(wallet_effect);

	uint64_t pile_uid = 0;
	if (!payload.destination.change.keys.empty())
		pile_uid = payload.destination.change.keys[0].id;
	else
		pile_uid = actor_id;

	economic_account_effect pile_effect = {};
	pile_effect.key = { lineage, economic_account_kind::pile, pile_uid, 0 };
	for (size_t i = 0; i < 4; ++i)
	{
		pile_effect.before[i] = payload.destination.before[i];
		pile_effect.after[i] = payload.destination.after[i];
	}
	pile_effect.before_revision =
		result.piles[1].max_item_revision > 0 ? result.piles[1].max_item_revision - 1 : 0;
	pile_effect.after_revision = result.piles[1].max_item_revision;
	plan.accounts.push_back(pile_effect);

	// Children: 1 = Source currency change, 2 = Destination item change
	economic_child_link child1 = {};
	child1.operation_id = payload.source.change.operation_id;
	child1.domain = 1;
	child1.discriminator = 1;
	child1.parent_index = 0;
	child1.relationship = 1;
	plan.children.push_back(child1);

	economic_child_link child2 = {};
	child2.operation_id = payload.destination.change.operation_id;
	child2.domain = 2;
	child2.discriminator = 2;
	child2.parent_index = 0;
	child2.relationship = 1;
	plan.children.push_back(child2);

	// Postings: 0 = Wallet debit, 1 = Pile credit
	economic_coin_posting post_wallet = {};
	post_wallet.event_index = 0;
	post_wallet.account_index = 0;
	post_wallet.child_index = 1;
	for (size_t i = 0; i < 4; ++i)
		post_wallet.delta[i] = -delta_vector[i];
	post_wallet.copper = -copper_value;
	plan.postings.push_back(post_wallet);

	economic_coin_posting post_pile = {};
	post_pile.event_index = 1;
	post_pile.account_index = 1;
	post_pile.child_index = 2;
	for (size_t i = 0; i < 4; ++i)
		post_pile.delta[i] = delta_vector[i];
	post_pile.copper = copper_value;
	plan.postings.push_back(post_pile);

	// Encode plan
	std::vector<uint8_t> encoded_plan;
	if (economic_plan_encode(plan, &encoded_plan) != economic_accounting_error::ok)
		return false;

	// Canonical intent (minimum 256 bytes)
	std::vector<uint8_t> canonical_intent(256, 0);
	std::copy(root_command.operation_id.bytes.begin(), root_command.operation_id.bytes.end(),
		  canonical_intent.begin());

	auto plan_digest = sha256(encoded_plan);
	auto intent_digest = sha256(canonical_intent);
	auto domain_digest = sha256(std::span<const uint8_t>(
		reinterpret_cast<const uint8_t *>(&payload), sizeof(payload)));

	// 1. Insert economic_accounting_operation
	std::string op_sql =
		"INSERT INTO economic_accounting_operation("
		"operation_id,lineage,epoch,original_operation_id,accounting_version,writer_id,"
		"policy_version,compiler_version,actor_kind,actor_id,reason,source_event,"
		"intent_digest,domain_digest,plan_digest,canonical_intent,canonical_plan,"
		"outcome,result_code,account_count,posting_count,child_count,item_event_count,"
		"before_witness_count,after_witness_count) VALUES(" +
		id(root_command.operation_id) + "," + id(lineage) + "," + id(epoch) +
		",NULL,1,3,1,1,1," + std::to_string(actor_id) + ",3,NULL," + hex(intent_digest) +
		"," + hex(domain_digest) + "," + hex(plan_digest) + "," + hex(canonical_intent) +
		"," + hex(encoded_plan) + ",1,0,2,2,2,0,0,0)";
	if (!execute(connection, op_sql))
		return false;

	// 2. Insert economic_accounting_child
	for (size_t i = 0; i < plan.children.size(); ++i)
	{
		const auto &child = plan.children[i];
		std::string child_sql =
			"INSERT INTO economic_accounting_child("
			"operation_id,child_index,child_operation_id,domain_id,discriminator,"
			"parent_index,relationship,receipt_operation_id) VALUES(" +
			id(root_command.operation_id) + "," + std::to_string(i + 1) + "," +
			id(child.operation_id) + "," + std::to_string(child.domain) + "," +
			std::to_string(child.discriminator) + ",0,1," + id(child.operation_id) +
			")";
		if (!execute(connection, child_sql))
			return false;
	}

	// 3. Insert economic_accounting_account_effect
	for (size_t i = 0; i < plan.accounts.size(); ++i)
	{
		const auto &acc = plan.accounts[i];
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key_bytes = {};
		if (economic_account_key_encode(acc.key, &key_bytes) !=
		    economic_accounting_error::ok)
			return false;

		std::string acc_sql =
			"INSERT INTO economic_accounting_account_effect("
			"operation_id,account_index,account_key,before_copper,before_silver,before_gold,before_platinum,"
			"after_copper,after_silver,after_gold,after_platinum,before_revision,after_revision) VALUES(" +
			id(root_command.operation_id) + "," + std::to_string(i) + "," +
			hex(key_bytes) + "," + std::to_string(acc.before[0]) + "," +
			std::to_string(acc.before[1]) + "," + std::to_string(acc.before[2]) + "," +
			std::to_string(acc.before[3]) + "," + std::to_string(acc.after[0]) + "," +
			std::to_string(acc.after[1]) + "," + std::to_string(acc.after[2]) + "," +
			std::to_string(acc.after[3]) + "," + std::to_string(acc.before_revision) +
			"," + std::to_string(acc.after_revision) + ")";
		if (!execute(connection, acc_sql))
			return false;
	}

	// 4. Insert economic_accounting_coin_posting
	for (size_t i = 0; i < plan.postings.size(); ++i)
	{
		const auto &post = plan.postings[i];
		std::string post_sql =
			"INSERT INTO economic_accounting_coin_posting("
			"operation_id,line_index,event_index,account_index,child_index,"
			"delta_copper,delta_silver,delta_gold,delta_platinum,copper_value) VALUES(" +
			id(root_command.operation_id) + "," + std::to_string(i) + "," +
			std::to_string(post.event_index) + "," +
			std::to_string(post.account_index) + "," +
			std::to_string(post.child_index) + "," + std::to_string(post.delta[0]) +
			"," + std::to_string(post.delta[1]) + "," + std::to_string(post.delta[2]) +
			"," + std::to_string(post.delta[3]) + "," + std::to_string(post.copper) +
			")";
		if (!execute(connection, post_sql))
			return false;
	}

	return true;
}

#endif
