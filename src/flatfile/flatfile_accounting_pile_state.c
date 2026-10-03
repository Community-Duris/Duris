#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdio>
#include <new>
#include <openssl/sha.h>

namespace
{
constexpr size_t head_bytes = 4 + 16 + 16 + 8 + 8 + 32 + 1 + 16 + SHA256_DIGEST_LENGTH;

std::string filename(uint64_t uid)
{
	char name[32] = {};
	std::snprintf(name, sizeof(name), "pile-head-%016llx.eph",
		      static_cast<unsigned long long>(uid));
	return name;
}

void number(std::vector<uint8_t> &bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes.push_back(static_cast<uint8_t>(value >> (8 * index)));
}

uint64_t number(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= uint64_t(bytes[offset + index]) << (8 * index);
	return value;
}

bool valid_balance(const economic_coin_vector &balance)
{
	return std::all_of(balance.begin(), balance.end(),
			   [](int64_t value) { return value >= 0 && value <= INT32_MAX; });
}

bool valid(const flatfile_accounting_pile_state &state)
{
	return economic_account_key_valid(state.account) &&
	       state.account.kind == economic_account_kind::pile && state.account.context_id == 0 &&
	       state.item_revision > 0 && !critical_operation_id_is_zero(state.epoch) &&
	       !critical_operation_id_is_zero(state.operation_id) && valid_balance(state.balance) &&
	       (!state.retired || state.balance == economic_coin_vector{});
}

std::vector<uint8_t> encode(const flatfile_accounting_pile_state &state)
{
	std::vector<uint8_t> bytes{ 'E', 'P', 'H', '1' };
	bytes.insert(bytes.end(), state.account.lineage.bytes.begin(),
		     state.account.lineage.bytes.end());
	bytes.insert(bytes.end(), state.epoch.bytes.begin(), state.epoch.bytes.end());
	number(bytes, state.account.authority_id);
	number(bytes, state.item_revision);
	for (auto coin : state.balance)
		number(bytes, static_cast<uint64_t>(coin));
	bytes.push_back(state.retired ? 1 : 0);
	bytes.insert(bytes.end(), state.operation_id.bytes.begin(), state.operation_id.bytes.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	bytes.insert(bytes.end(), digest.begin(), digest.end());
	return bytes;
}

bool decode(std::span<const uint8_t> bytes, uint64_t uid, flatfile_accounting_pile_state *state)
{
	if (bytes.size() != head_bytes || !std::equal(bytes.begin(), bytes.begin() + 4, "EPH1") ||
	    bytes[84] > 1)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size() - digest.size(), digest.data());
	if (!std::equal(digest.begin(), digest.end(), bytes.end() - digest.size()))
		return false;
	flatfile_accounting_pile_state value;
	std::copy_n(bytes.begin() + 4, 16, value.account.lineage.bytes.begin());
	value.account.kind = economic_account_kind::pile;
	std::copy_n(bytes.begin() + 20, 16, value.epoch.bytes.begin());
	value.account.authority_id = number(bytes, 36);
	value.item_revision = number(bytes, 44);
	for (size_t index = 0; index < 4; ++index)
		value.balance[index] = static_cast<int64_t>(number(bytes, 52 + index * 8));
	value.retired = bytes[84] == 1;
	std::copy_n(bytes.begin() + 85, 16, value.operation_id.bytes.begin());
	if (value.account.authority_id != uid || !valid(value))
		return false;
	*state = value;
	return true;
}

flatfile_accounting_status read(const std::string &root, uint64_t uid,
				flatfile_accounting_pile_state *state, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto result = flatfile_read(root + "/economic-evidence", filename(uid), head_bytes,
					  &bytes, error);
	if (result == flatfile_read_result::not_found)
		return flatfile_accounting_status::not_found;
	if (result == flatfile_read_result::io_error)
		return flatfile_accounting_status::io_error;
	if (result != flatfile_read_result::ok || !decode(bytes, uid, state))
		return flatfile_accounting_status::invalid;
	return flatfile_accounting_status::ok;
}
} // namespace

flatfile_accounting_status
flatfile_accounting_pile_state_read(const std::string &root, const flatfile_authority_lock &lock,
				    uint64_t uid, flatfile_accounting_pile_state *state,
				    std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !uid || !state)
		return flatfile_accounting_status::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_accounting_status::io_error :
			       flatfile_accounting_status::invalid;
	flatfile_accounting_pile_state candidate;
	const auto status = read(root, uid, &candidate, error);
	if (status == flatfile_accounting_status::ok)
		*state = candidate;
	return status;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}

flatfile_accounting_status flatfile_accounting_pile_state_stage_baseline(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_pile_state &state,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !operations || !valid(state) || state.retired ||
	    operations->size() >= flatfile_authority_transaction_maximum_operations)
		return flatfile_accounting_status::invalid;
	const auto name = filename(state.account.authority_id);
	for (const auto &operation : *operations)
		if (operation.store == flatfile_authority_store::economic_evidence &&
		    operation.filename == name)
			return flatfile_accounting_status::conflict;
	flatfile_accounting_pile_state prior;
	const auto status = flatfile_accounting_pile_state_read(
		root, lock, state.account.authority_id, &prior, error);
	if (status == flatfile_accounting_status::ok)
		return economic_account_key_equal(prior.account, state.account) &&
				       prior.epoch.bytes == state.epoch.bytes &&
				       prior.operation_id.bytes == state.operation_id.bytes &&
				       prior.item_revision == state.item_revision &&
				       prior.balance == state.balance && !prior.retired ?
			       flatfile_accounting_status::already_exists :
			       flatfile_accounting_status::conflict;
	if (status != flatfile_accounting_status::not_found)
		return status;
	auto candidate = *operations;
	candidate.push_back({ flatfile_authority_store::economic_evidence,
			      flatfile_authority_operation_kind::write, name, encode(state) });
	*operations = std::move(candidate);
	return flatfile_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}

flatfile_accounting_status flatfile_accounting_pile_state_stage(
	const std::string &root, const flatfile_authority_lock &lock,
	const economic_account_effect &effect, const critical_operation_id &epoch,
	const critical_operation_id &operation_id, bool retired,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !operations ||
	    effect.key.kind != economic_account_kind::pile || effect.key.context_id != 0 ||
	    !effect.key.authority_id || !economic_account_key_valid(effect.key) ||
	    critical_operation_id_is_zero(epoch) || critical_operation_id_is_zero(operation_id) ||
	    effect.after_revision <= effect.before_revision || !valid_balance(effect.before) ||
	    !valid_balance(effect.after) || (retired && effect.after != economic_coin_vector{}) ||
	    operations->size() >= flatfile_authority_transaction_maximum_operations)
		return flatfile_accounting_status::invalid;
	const auto name = filename(effect.key.authority_id);
	for (const auto &operation : *operations)
		if (operation.store == flatfile_authority_store::economic_evidence &&
		    operation.filename == name)
			return flatfile_accounting_status::conflict;
	flatfile_accounting_pile_state prior;
	const auto status = flatfile_accounting_pile_state_read(root, lock, effect.key.authority_id,
								&prior, error);
	if (effect.before_revision == 0)
	{
		if (effect.before != economic_coin_vector{} || effect.after_revision != 1)
			return flatfile_accounting_status::invalid;
		if (status == flatfile_accounting_status::ok)
			return flatfile_accounting_status::already_exists;
		if (status != flatfile_accounting_status::not_found)
			return status;
	}
	else
	{
		if (status != flatfile_accounting_status::ok)
			return status;
		if (!economic_account_key_equal(prior.account, effect.key) || prior.retired ||
		    prior.item_revision != effect.before_revision || prior.balance != effect.before)
			return flatfile_accounting_status::conflict;
	}
	flatfile_accounting_pile_state next{
		effect.key, epoch, operation_id, effect.after, effect.after_revision, retired
	};
	if (!valid(next))
		return flatfile_accounting_status::invalid;
	auto candidate = *operations;
	candidate.push_back({ flatfile_authority_store::economic_evidence,
			      flatfile_authority_operation_kind::write, name, encode(next) });
	*operations = std::move(candidate);
	return flatfile_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}
