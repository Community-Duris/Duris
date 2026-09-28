#include "economy/coin_transfer_accounting.h"
#include "economy/economic_accounting_intent.h"
#include "economy/economic_accounting_plan.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cerrno>
#include <memory>
#include <new>
#include <optional>
#include <openssl/sha.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint32_t CURRENCY_CHILD_DOMAIN = COIN_TRANSFER_OPERATION_DOMAIN;
struct failure
{
	unsigned int code;
};
void require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}
void checked(economic_accounting_error error)
{
	require(error == economic_accounting_error::ok,
		error == economic_accounting_error::capacity ? ENOMEM : EINVAL);
}
#ifndef __NO_MYSQL__
std::string hex(std::span<const uint8_t> data)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string value = "X'";
	value.reserve(data.size() * 2 + 3);
	for (const auto byte : data)
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
#endif
uint64_t little_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(bytes[offset + byte]) << (8 * byte);
	return value;
}
void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t byte = 0; byte < 8; ++byte)
		bytes->push_back(static_cast<uint8_t>(value >> (byte * 8)));
}
#ifndef __NO_MYSQL__
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) ? mysql_errno(connection) : EIO };
}
using cells = std::vector<std::optional<std::string>>;
cells read(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(result.get()) == 1, ENOENT);
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths && mysql_num_fields(result.get()) == columns);
	cells values;
	for (size_t index = 0; index < columns; ++index)
		values.push_back(row[index] ? std::optional<std::string>(
						      std::string(row[index], lengths[index])) :
					      std::nullopt);
	return values;
}
template <typename T> T integer(const std::optional<std::string> &cell)
{
	require(cell.has_value());
	T value = 0;
	const auto parsed = std::from_chars(cell->data(), cell->data() + cell->size(), value);
	require(parsed.ec == std::errc{} && parsed.ptr == cell->data() + cell->size());
	return value;
}
void count(MYSQL *connection, const std::string &table, const std::string &where, uint64_t expected)
{
	require(integer<uint64_t>(read(connection,
				       "SELECT COUNT(*) FROM " + table + " WHERE " + where,
				       1)[0]) == expected);
}
std::string predicate(const std::vector<std::pair<std::string, std::string>> &values)
{
	std::string sql;
	for (const auto &[name, value] : values)
	{
		if (!sql.empty())
			sql += " AND ";
		sql += name + " <=> " + value;
	}
	return sql;
}
void insert(MYSQL *connection, const std::string &table,
	    const std::vector<std::pair<std::string, std::string>> &values)
{
	std::string names, data;
	for (const auto &[name, value] : values)
	{
		if (!names.empty())
		{
			names += ',';
			data += ',';
		}
		names += name;
		data += value;
	}
	execute(connection, "INSERT INTO " + table + "(" + names + ") VALUES(" + data + ")");
	require(mysql_affected_rows(connection) == 1);
}
void coin_fields(std::vector<std::pair<std::string, std::string>> *values,
		 const std::string &prefix, const economic_coin_vector &coins)
{
	static constexpr std::array<const char *, 4> names = { "copper", "silver", "gold",
							       "platinum" };
	for (size_t index = 0; index < names.size(); ++index)
		values->emplace_back(prefix + names[index], std::to_string(coins[index]));
}
#endif
struct identity
{
	economic_frozen_intent intent;
	coin_transfer_payload payload;
	economic_account_key source, destination;
	currency_command_payload source_change, destination_change;
};
critical_command admission_projection(const critical_command &root)
{
	auto projection = root;
	projection.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	projection.accounting_intent.clear();
	projection.publication_required = false;
	return projection;
}
identity decode(const critical_command &root)
{
	require(root.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			root.type == critical_command_type::coin_transfer &&
			critical_command_envelope_valid(root),
		EPROTONOSUPPORT);
	identity value;
	require(coin_transfer_command_decode_payload(root, &value.payload), EINVAL);
	const auto &source = value.payload.source.change;
	const auto &destination = value.payload.destination.change;
	// A typed coin root supports only two ordinary wallet accounts. Item/pile
	// endpoints need the separate authenticated custody/account adapter.
	require(source.type == critical_command_type::account_bank &&
			destination.type == critical_command_type::account_bank,
		EACCES);
	require(currency_command_decode_payload(source, &value.source_change) &&
			currency_command_decode_payload(destination, &value.destination_change),
		EINVAL);
	for (const auto *change : { &value.source_change, &value.destination_change })
	{
		require(change->reason == currency_reason_type::coin_transfer, EACCES);
		require(std::all_of(change->bank_delta.amount.begin(),
				    change->bank_delta.amount.end(),
				    [](int64_t amount) { return amount == 0; }),
			EACCES);
		require(source.expected_revisions.size() == 2 &&
				destination.expected_revisions.size() == 2 && change->pid > 0 &&
				change->pid <= INT32_MAX,
			EINVAL);
	}
	checked(economic_intent_decode(root.accounting_intent, &value.intent));
	const auto &facts = value.intent.admission.facts;
	require(facts.size() == ECONOMIC_WALLET_COIN_TRANSFER_FACT_BYTES, EINVAL);
	const auto &metadata = value.intent.admission.metadata;
	value.source = { metadata.lineage, economic_account_kind::wallet, little_u64(facts, 0), 0 };
	value.destination = { metadata.lineage, economic_account_kind::wallet, little_u64(facts, 8),
			      0 };
	std::vector<uint8_t> expected;
	checked(coin_transfer_accounting_intent(admission_projection(root), metadata.epoch,
						value.source, value.destination, &expected));
	require(expected == root.accounting_intent, EACCES);
	return value;
}
[[maybe_unused]] void validate_payload(const coin_transfer_payload &payload,
				       const economic_account_key &source,
				       const economic_account_key &destination,
				       const currency_command_result *results,
				       economic_accounting_plan *plan)
{
	require(plan && results, EINVAL);
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	const economic_account_key accounts[] = { source, destination };
	// Match the native second-child rebase; wallet fences remain on each endpoint.
	coin_transfer_result result_pair = {};
	result_pair.wallets[0] = results[0];
	result_pair.wallets[1] = results[1];
	critical_command destination_after_source = {};
	for (const auto *endpoint : endpoints)
		require(endpoint->change.type == critical_command_type::account_bank &&
				endpoint->change.keys.size() == 2 &&
				endpoint->change.expected_revisions.size() == 2,
			EILSEQ);
	require(coin_transfer_command_destination_after_source(payload, result_pair,
							       &destination_after_source),
		EILSEQ);
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		const auto &result = results[index];
		currency_command_payload change = {};
		require(currency_command_decode_payload(endpoint.change, &change), EILSEQ);
		const auto expected_wallet_revision =
			endpoint.change.expected_revisions[0].revision;
		const auto expected_bank_revision =
			index == 1 ? destination_after_source.expected_revisions[1].revision :
				     endpoint.change.expected_revisions[1].revision;
		require(expected_wallet_revision != UINT64_MAX &&
				expected_bank_revision != UINT64_MAX,
			EILSEQ);
		if (index == 0)
			require(result.bank_revision ==
					endpoint.change.expected_revisions[1].revision + 1,
				EILSEQ);
		else
			require(result.bank_revision == expected_bank_revision + 1, EILSEQ);
		require(result.wallet_revision == expected_wallet_revision + 1 &&
				result.wallet.amount == std::array<int64_t, 4>{ endpoint.after[0],
										endpoint.after[1],
										endpoint.after[2],
										endpoint.after[3] },
			EILSEQ);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			require(static_cast<int64_t>(endpoint.after[denomination]) -
						static_cast<int64_t>(
							endpoint.before[denomination]) ==
					change.wallet_delta.amount[denomination],
				EILSEQ);
		plan->accounts.push_back({ accounts[index],
					   { endpoint.before[0], endpoint.before[1],
					     endpoint.before[2], endpoint.before[3] },
					   { endpoint.after[0], endpoint.after[1],
					     endpoint.after[2], endpoint.after[3] },
					   expected_wallet_revision,
					   result.wallet_revision });
	}
}
#ifndef __NO_MYSQL__
economic_accounting_plan make_plan(const critical_command &root, const identity &value,
				   const coin_transfer_result &result)
{
	economic_accounting_plan plan;
	checked(economic_intent_plan_metadata(root, value.intent, &plan.metadata));
	const currency_command_result wallet_results[] = { result.wallets[0], result.wallets[1] };
	validate_payload(value.payload, value.source, value.destination, wallet_results, &plan);
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		economic_child_link child;
		child.operation_id = endpoints[index]->change.operation_id;
		child.domain = CURRENCY_CHILD_DOMAIN;
		child.discriminator = index;
		plan.children.push_back(child);
	}
	for (size_t index = 0; index < 2; ++index)
	{
		economic_coin_posting posting;
		posting.event_index = index;
		posting.account_index = index;
		posting.child_index = static_cast<uint16_t>(index + 1);
		for (size_t denomination = 0; denomination < 4; ++denomination)
			posting.delta[denomination] =
				static_cast<int64_t>(endpoints[index]->after[denomination]) -
				static_cast<int64_t>(endpoints[index]->before[denomination]);
		checked(economic_coin_value(posting.delta, &posting.copper));
		plan.postings.push_back(posting);
	}
	checked(economic_plan_normalize(&plan));
	return plan;
}
std::vector<std::pair<std::string, std::string>>
operation_fields(const critical_command &root, const economic_frozen_intent &intent,
		 unsigned int result_code, const economic_accounting_plan *plan)
{
	economic_plan_metadata metadata;
	checked(economic_intent_plan_metadata(root, intent, &metadata));
	std::vector<uint8_t> encoded;
	if (plan)
		checked(economic_plan_encode(*plan, &encoded));
	economic_digest plan_digest = {};
	if (plan)
		SHA256(encoded.data(), encoded.size(), plan_digest.data());
	return { { "operation_id", id(root.operation_id) },
		 { "lineage", id(metadata.lineage) },
		 { "epoch", id(metadata.epoch) },
		 { "original_operation_id", "NULL" },
		 { "accounting_version", std::to_string(metadata.version) },
		 { "writer_id", std::to_string(metadata.writer_id) },
		 { "policy_version", std::to_string(metadata.policy_version) },
		 { "compiler_version", std::to_string(metadata.compiler_version) },
		 { "actor_kind", std::to_string(static_cast<uint8_t>(metadata.actor_kind)) },
		 { "actor_id", std::to_string(metadata.actor_id) },
		 { "reason", std::to_string(static_cast<uint16_t>(metadata.reason)) },
		 { "source_event", "NULL" },
		 { "intent_digest", hex(metadata.intent_digest) },
		 { "domain_digest", hex(metadata.domain_digest) },
		 { "plan_digest", plan ? hex(plan_digest) : "NULL" },
		 { "canonical_intent", hex(root.accounting_intent) },
		 { "canonical_plan", plan ? hex(encoded) : "NULL" },
		 { "outcome", result_code ? "2" : "1" },
		 { "result_code", std::to_string(result_code) },
		 { "account_count", plan ? std::to_string(plan->accounts.size()) : "0" },
		 { "posting_count", plan ? std::to_string(plan->postings.size()) : "0" },
		 { "child_count", plan ? std::to_string(plan->children.size()) : "0" },
		 { "item_event_count", "0" },
		 { "before_witness_count", "0" },
		 { "after_witness_count", "0" } };
}
std::vector<std::pair<std::string, std::string>>
effect_fields(const critical_operation_id &root, size_t index,
	      const economic_account_effect &effect)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key = {};
	checked(economic_account_key_encode(effect.key, &key));
	std::vector<std::pair<std::string, std::string>> values = { { "operation_id", id(root) },
								    { "account_index",
								      std::to_string(index) },
								    { "account_key", hex(key) } };
	coin_fields(&values, "before_", effect.before);
	coin_fields(&values, "after_", effect.after);
	values.emplace_back("before_revision", std::to_string(effect.before_revision));
	values.emplace_back("after_revision", std::to_string(effect.after_revision));
	return values;
}
std::vector<std::pair<std::string, std::string>>
posting_fields(const critical_operation_id &root, size_t index,
	       const economic_coin_posting &posting)
{
	std::vector<std::pair<std::string, std::string>> values = {
		{ "operation_id", id(root) },
		{ "line_index", std::to_string(index) },
		{ "event_index", std::to_string(posting.event_index) },
		{ "account_index", std::to_string(posting.account_index) },
		{ "child_index", std::to_string(posting.child_index) },
		{ "copper_value", std::to_string(posting.copper) }
	};
	coin_fields(&values, "delta_", posting.delta);
	return values;
}
void verify_plan_rows(MYSQL *connection, const critical_command &root,
		      const economic_accounting_plan *plan, bool append)
{
	const std::string where = "operation_id=" + id(root.operation_id);
	if (plan)
	{
		for (size_t index = 0; index < plan->children.size(); ++index)
		{
			const auto &child = plan->children[index];
			const auto values = std::vector<std::pair<std::string, std::string>>{
				{ "operation_id", id(root.operation_id) },
				{ "child_index", std::to_string(index + 1) },
				{ "child_operation_id", id(child.operation_id) },
				{ "domain_id", std::to_string(child.domain) },
				{ "discriminator", std::to_string(child.discriminator) },
				{ "parent_index", std::to_string(child.parent_index) },
				{ "relationship", std::to_string(child.relationship) },
				{ "receipt_operation_id", id(child.operation_id) }
			};
			if (append)
				insert(connection, "economic_accounting_child", values);
			count(connection, "economic_accounting_child", predicate(values), 1);
		}
		for (size_t index = 0; index < plan->accounts.size(); ++index)
		{
			const auto values =
				effect_fields(root.operation_id, index, plan->accounts[index]);
			if (append)
				insert(connection, "economic_accounting_account_effect", values);
			count(connection, "economic_accounting_account_effect", predicate(values),
			      1);
		}
		for (size_t index = 0; index < plan->postings.size(); ++index)
		{
			const auto values =
				posting_fields(root.operation_id, index, plan->postings[index]);
			if (append)
				insert(connection, "economic_accounting_coin_posting", values);
			count(connection, "economic_accounting_coin_posting", predicate(values), 1);
		}
	}
	count(connection, "economic_accounting_child", where, plan ? plan->children.size() : 0);
	count(connection, "economic_accounting_account_effect", where,
	      plan ? plan->accounts.size() : 0);
	count(connection, "economic_accounting_coin_posting", where,
	      plan ? plan->postings.size() : 0);
	for (const char *table : { "economic_accounting_item_reference",
				   "economic_accounting_source_claim", "item_ownership_ledger" })
		count(connection, table, where, 0);
}
unsigned int error_code(const failure &error)
{
	return error.code;
}
void lock_native_wallet_before(MYSQL *connection, const coin_transfer_endpoint &endpoint)
{
	currency_command_payload command = {};
	require(currency_command_decode_payload(endpoint.change, &command), EINVAL);
	const auto row = read(
		connection,
		"SELECT copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
			std::to_string(command.pid) + " FOR UPDATE",
		5);
	const auto revision = integer<uint64_t>(row[4]);
	if (revision == endpoint.change.expected_revisions[0].revision)
	{
		for (size_t denomination = 0; denomination < 4; ++denomination)
			require(integer<int64_t>(row[denomination]) ==
					endpoint.before[denomination],
				EILSEQ);
	}
}
#endif

} // namespace

economic_accounting_error
coin_transfer_accounting_intent(const critical_command &root, const critical_operation_id &epoch,
				const economic_account_key &source_wallet,
				const economic_account_key &destination_wallet,
				std::vector<uint8_t> *encoded)
{
	using error = economic_accounting_error;
	if (!encoded || critical_operation_id_is_zero(epoch) ||
	    !economic_account_key_valid(source_wallet) ||
	    !economic_account_key_valid(destination_wallet) ||
	    source_wallet.kind != economic_account_kind::wallet ||
	    destination_wallet.kind != economic_account_kind::wallet || source_wallet.context_id ||
	    destination_wallet.context_id ||
	    source_wallet.lineage.bytes != destination_wallet.lineage.bytes ||
	    source_wallet.authority_id == destination_wallet.authority_id)
		return error::invalid_identity;
	coin_transfer_payload payload;
	if (!coin_transfer_command_decode_payload(root, &payload) ||
	    payload.source.change.type != critical_command_type::account_bank ||
	    payload.destination.change.type != critical_command_type::account_bank)
		return error::unauthorized;
	currency_command_payload source = {}, destination = {};
	if (!currency_command_decode_payload(payload.source.change, &source) ||
	    !currency_command_decode_payload(payload.destination.change, &destination) ||
	    payload.source.change.keys.size() != 2 || payload.destination.change.keys.size() != 2 ||
	    payload.source.change.keys[0].type != critical_entity_type::player ||
	    payload.destination.change.keys[0].type != critical_entity_type::player ||
	    source.reason != currency_reason_type::coin_transfer ||
	    destination.reason != currency_reason_type::coin_transfer ||
	    source.pid != payload.source.change.keys[0].id ||
	    destination.pid != payload.destination.change.keys[0].id ||
	    payload.source.change.expected_revisions.size() != 2 ||
	    payload.destination.change.expected_revisions.size() != 2 ||
	    payload.source.change.expected_revisions[0].revision == UINT64_MAX ||
	    payload.destination.change.expected_revisions[0].revision == UINT64_MAX ||
	    payload.source.change.expected_revisions[1].revision == UINT64_MAX ||
	    payload.destination.change.expected_revisions[1].revision == UINT64_MAX)
		return error::unauthorized;
	for (const auto *change : { &source, &destination })
		if (std::any_of(change->bank_delta.amount.begin(), change->bank_delta.amount.end(),
				[](int64_t amount) { return amount != 0; }))
			return error::unauthorized;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = source_wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = source_wallet.authority_id;
		facts.metadata.writer_id = ECONOMIC_WRITER_WALLET_COIN_TRANSFER;
		facts.metadata.reason = economic_reason::coin_transfer;
		append_u64(&facts.facts, source_wallet.authority_id);
		append_u64(&facts.facts, destination_wallet.authority_id);
		return economic_intent_freeze(root, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

bool coin_transfer_accounting_command_supported(const critical_command &root) noexcept
{
	try
	{
		(void)decode(root);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifdef __NO_MYSQL__
unsigned int coin_transfer_accounting_lock(MYSQL *, const critical_command &,
					   const coin_transfer_payload &,
					   coin_transfer_accounting_context *)
{
	return ENOTSUP;
}
unsigned int coin_transfer_accounting_record(MYSQL *, const critical_command &,
					     const coin_transfer_result &, unsigned int,
					     const coin_transfer_accounting_context &)
{
	return ENOTSUP;
}
unsigned int coin_transfer_accounting_verify_retained(MYSQL *, const critical_command &,
						      unsigned int, const uint8_t *, size_t)
{
	return ENOTSUP;
}
#else
unsigned int coin_transfer_accounting_lock(MYSQL *connection, const critical_command &root,
					   const coin_transfer_payload &payload,
					   coin_transfer_accounting_context *context)
{
	try
	{
		require(connection && context, EINVAL);
		const auto value = decode(root);
		require(payload.source.change.operation_id.bytes ==
					value.payload.source.change.operation_id.bytes &&
				payload.destination.change.operation_id.bytes ==
					value.payload.destination.change.operation_id.bytes,
			EEXIST);
		const auto &metadata = value.intent.admission.metadata;
		std::array<economic_sql_mapping_request, 2> requests = {
			economic_sql_mapping_request{ value.source, PLAYER_LOCATOR,
						      value.source_change.pid },
			economic_sql_mapping_request{ value.destination, PLAYER_LOCATOR,
						      value.destination_change.pid }
		};
		coin_transfer_accounting_context candidate;
		candidate.session_id = mysql_thread_id(connection);
		const auto error = economic_sql_lock_authority(connection, metadata.lineage,
							       metadata.epoch, requests,
							       &candidate.authority);
		require(!error, error);
		require(connection->server_status & SERVER_STATUS_IN_TRANS, ENOTCONN);
		std::array<const coin_transfer_endpoint *, 2> ordered = { &payload.source,
									  &payload.destination };
		std::sort(ordered.begin(), ordered.end(),
			  [](const auto *left, const auto *right)
			  {
				  currency_command_payload a = {}, b = {};
				  if (!currency_command_decode_payload(left->change, &a) ||
				      !currency_command_decode_payload(right->change, &b))
					  return false;
				  return a.pid < b.pid;
			  });
		lock_native_wallet_before(connection, *ordered[0]);
		lock_native_wallet_before(connection, *ordered[1]);
		*context = std::move(candidate);
		return 0;
	}
	catch (const failure &error)
	{
		return error_code(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int coin_transfer_accounting_record(MYSQL *connection, const critical_command &root,
					     const coin_transfer_result &result,
					     unsigned int result_code,
					     const coin_transfer_accounting_context &context)
{
	try
	{
		require(connection && (connection->server_status & SERVER_STATUS_IN_TRANS) &&
				mysql_thread_id(connection) == context.session_id,
			ENOTCONN);
		const auto value = decode(root);
		const auto &metadata = value.intent.admission.metadata;
		economic_sql_authority_snapshot current;
		std::array<economic_sql_mapping_request, 2> requests = {
			economic_sql_mapping_request{ value.source, PLAYER_LOCATOR,
						      value.source_change.pid },
			economic_sql_mapping_request{ value.destination, PLAYER_LOCATOR,
						      value.destination_change.pid }
		};
		const auto lock_error = economic_sql_lock_authority(
			connection, metadata.lineage, metadata.epoch, requests, &current);
		require(!lock_error, lock_error);
		require(current.lineage_revision == context.authority.lineage_revision &&
				current.mappings.size() == context.authority.mappings.size(),
			ESTALE);
		for (size_t index = 0; index < current.mappings.size(); ++index)
			require(current.mappings[index].revision ==
					context.authority.mappings[index].revision,
				ESTALE);
		const bool business_rejection = result_code == ESTALE || result_code == ENOSPC ||
						result_code == ERANGE;
		require(!result_code || business_rejection, EINVAL);
		std::optional<economic_accounting_plan> plan;
		if (!result_code)
		{
			plan = make_plan(root, value, result);
			const auto effect_error = economic_coin_effects_validate(
				plan->accounts, plan->postings, plan->children.size());
			checked(effect_error);
			checked(economic_child_links_validate(root.operation_id, plan->children));
		}
		const auto operation =
			operation_fields(root, value.intent, result_code, plan ? &*plan : nullptr);
		insert(connection, "economic_accounting_operation", operation);
		count(connection, "economic_accounting_operation", predicate(operation), 1);
		verify_plan_rows(connection, root, plan ? &*plan : nullptr, true);
		verify_plan_rows(connection, root, plan ? &*plan : nullptr, false);
		require(connection->server_status & SERVER_STATUS_IN_TRANS, ENOTCONN);
		return 0;
	}
	catch (const failure &error)
	{
		return error_code(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int coin_transfer_accounting_verify_retained(MYSQL *connection,
						      const critical_command &root,
						      unsigned int result_code,
						      const uint8_t *result_payload,
						      size_t result_size)
{
	try
	{
		require(connection && (!result_size || result_payload), EINVAL);
		const auto value = decode(root);
		const bool business_rejection = result_code == ESTALE || result_code == ENOSPC ||
						result_code == ERANGE;
		require(!result_code || business_rejection, EILSEQ);
		std::optional<economic_accounting_plan> plan;
		coin_transfer_result result = {};
		if (!result_code)
		{
			require(coin_transfer_command_decode_result(value.payload, result_payload,
								    result_size, &result),
				EILSEQ);
			plan = make_plan(root, value, result);
		}
		else
			require(result_size == 0, EILSEQ);
		const auto operation =
			operation_fields(root, value.intent, result_code, plan ? &*plan : nullptr);
		count(connection, "economic_accounting_operation", predicate(operation), 1);
		const auto where = "operation_id=" + id(root.operation_id);
		verify_plan_rows(connection, root, plan ? &*plan : nullptr, false);
		if (plan)
		{
			for (const auto *endpoint :
			     { &value.payload.source, &value.payload.destination })
				count(connection, "currency_ledger",
				      "operation_id=" + id(endpoint->change.operation_id), 1);
			count(connection, "critical_outbox", where, 1);
		}
		else
		{
			count(connection, "currency_ledger", where, 0);
			count(connection, "critical_outbox", where, 0);
		}
		for (size_t index = 0; index < 2; ++index)
		{
			const auto &account = index ? value.destination : value.source;
			const auto native_id = index ? value.destination_change.pid :
						       value.source_change.pid;
			count(connection, "economic_account_mapping",
			      predicate({ { "mapping_id", std::to_string(account.authority_id) },
					  { "lineage", id(account.lineage) },
					  { "account_kind", "1" },
					  { "context_id", "0" },
					  { "backend_kind", "1" },
					  { "locator_kind", "1" },
					  { "native_id", std::to_string(native_id) } }),
			      1);
		}
		return 0;
	}
	catch (const failure &error)
	{
		return error_code(error);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
#endif
