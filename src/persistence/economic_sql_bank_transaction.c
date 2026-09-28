#include "persistence/economic_sql_bank_transaction.h"
#include "persistence/economic_accounting_repository.h"
#include "economy/currency_sql_mutation_writer.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <limits>
#include <new>
#include <optional>
#include <openssl/sha.h>
#include <string>
#include <strings.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
constexpr uint16_t PLAYER_LOCATOR = 1;
constexpr uint16_t BANK_LOCATOR = 2;
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
uint64_t little_u64(std::span<const uint8_t> input, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(input[offset + byte]) << (8 * byte);
	return value;
}
struct bank_identity
{
	economic_frozen_intent intent;
	currency_command_payload payload = {};
	economic_account_key wallet, bank;
};
bank_identity decode(const critical_command &command)
{
	require(command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			critical_command_envelope_valid(command),
		EPROTONOSUPPORT);
	bank_identity value;
	require(currency_command_decode_payload(command, &value.payload), EINVAL);
	checked(economic_intent_decode(command.accounting_intent, &value.intent));
	const auto &facts = value.intent.admission.facts;
	require(facts.size() == ECONOMIC_BANK_FACT_BYTES, EINVAL);
	const auto &metadata = value.intent.admission.metadata;
	value.wallet = { metadata.lineage, economic_account_kind::wallet, little_u64(facts, 0), 0 };
	value.bank = { metadata.lineage, economic_account_kind::bank, little_u64(facts, 8),
		       little_u64(facts, 16) };
	std::vector<uint8_t> expected;
	auto admission_command = command;
	admission_command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	admission_command.accounting_intent.clear();
	checked(economic_bank_transfer_intent(admission_command, metadata.epoch, value.wallet,
					      value.bank, &expected));
	// Enforces writer/reason/source/actor/policy/facts and the complete immutable
	// command binding before any SQL. ATM capabilities never mint source events.
	require(expected == command.accounting_intent, EACCES);
	return value;
}
}

bool economic_sql_bank_command_supported(const critical_command &command)
{
	try
	{
		(void)decode(command);
		return true;
	}
	catch (const failure &)
	{
		return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

#ifdef __NO_MYSQL__
struct economic_sql_bank_transaction::implementation
{
	currency_command_result result = {};
};
economic_sql_bank_transaction::economic_sql_bank_transaction(std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
economic_sql_bank_transaction::~economic_sql_bank_transaction() = default;
unsigned int
economic_sql_bank_transaction::prepare(MYSQL *, const critical_command &,
				       std::unique_ptr<economic_sql_bank_transaction> *)
{
	return ENOTSUP;
}
unsigned int economic_sql_bank_transaction::apply()
{
	return ENOTSUP;
}
unsigned int economic_sql_bank_transaction::finalize()
{
	return ENOTSUP;
}
unsigned int economic_sql_bank_transaction::verify_root_completion()
{
	return ENOTSUP;
}
const currency_command_result &economic_sql_bank_transaction::result() const
{
	return state_->result;
}
unsigned int economic_sql_bank_transaction::result_code() const
{
	return ENOTSUP;
}
unsigned int economic_sql_bank_verify_retained(MYSQL *, const critical_command &, unsigned int,
					       std::span<const uint8_t>)
{
	return ENOTSUP;
}
#else
namespace
{
using cells = std::vector<std::optional<std::string>>;
using fields = std::vector<std::pair<std::string, std::string>>;
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
void execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw failure{ mysql_errno(connection) };
}
cells read(MYSQL *connection, const std::string &sql, size_t columns)
{
	execute(connection, sql);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	require(bool(result), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	require(mysql_num_rows(result.get()) == 1, ENOENT);
	require(mysql_num_fields(result.get()) == columns);
	auto row = mysql_fetch_row(result.get());
	auto lengths = mysql_fetch_lengths(result.get());
	require(row && lengths);
	cells values;
	for (size_t index = 0; index < columns; ++index)
		require(lengths[index] <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES, E2BIG);
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
std::string predicate(const fields &values)
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
void insert(MYSQL *connection, const std::string &table, const fields &values, bool ignore = false)
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
	execute(connection, std::string(ignore ? "INSERT IGNORE INTO " : "INSERT INTO ") + table +
				    "(" + names + ") VALUES(" + data + ")");
	require(ignore || mysql_affected_rows(connection) == 1);
}
void coins(fields &values, const std::string &prefix, const currency_vector &vector)
{
	constexpr std::array<const char *, 4> names = { "copper", "silver", "gold", "platinum" };
	for (size_t index = 0; index < names.size(); ++index)
		values.emplace_back(prefix + names[index], std::to_string(vector.amount[index]));
}
void active(MYSQL *connection, unsigned long session)
{
	require(connection && (connection->server_status & SERVER_STATUS_IN_TRANS) &&
			mysql_thread_id(connection) == session,
		ENOTCONN);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect,
		EPERM);
}
economic_digest hash(std::span<const uint8_t> bytes)
{
	economic_digest digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest;
}
void inbox(MYSQL *connection, const critical_command &command, bool pending)
{
	std::vector<uint8_t> encoded, keys;
	require(critical_command_encode(command, &encoded) == critical_command_codec_result::ok,
		EINVAL);
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (size_t byte = 0; byte < 8; ++byte)
			keys.push_back(static_cast<uint8_t>(key.id >> (8 * byte)));
	}
	const fields expected = { { "operation_id", id(command.operation_id) },
				  { "command_hash", hex(hash(encoded)) },
				  { "keys_hash", hex(hash(keys)) },
				  { "command_type",
				    std::to_string(static_cast<uint16_t>(command.type)) },
				  { "schema_version", std::to_string(command.schema_version) },
				  { "payload_version", std::to_string(command.payload_version) },
				  { "status", pending ? "0" : "1" },
				  { "failure_stage", "0" } };
	// The owner inserted and holds this unique inbox row before calling prepare.
	// FOR UPDATE also prevents a separate owner from completing it concurrently.
	const auto row = read(connection,
			      "SELECT operation_id FROM critical_operation_inbox WHERE " +
				      predicate(expected) + " FOR UPDATE",
			      1);
	require(row[0].has_value());
}
uint64_t mapping_hint(MYSQL *connection, uint64_t lifetime)
{
	return integer<uint64_t>(
		read(connection,
		     "SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
			     std::to_string(lifetime),
		     1)[0]);
}
currency_command_result balances(MYSQL *connection, const bank_identity &identity, uint64_t bank_id)
{
	const auto player = read(
		connection,
		"SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
			std::to_string(identity.payload.pid) + " FOR UPDATE",
		7);
	const auto bank = read(
		connection,
		"SELECT account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM account_banks WHERE id=" +
			std::to_string(bank_id) + " FOR UPDATE",
		7);
	for (const auto *row : { &player, &bank })
	{
		require((*row)[0].has_value() &&
				(*row)[0]->size() < identity.payload.account_name.size() &&
				(*row)[0]->find('\0') == std::string::npos &&
				!strcasecmp((*row)[0]->c_str(),
					    identity.payload.account_name.data()) &&
				integer<uint64_t>((*row)[1]) == identity.payload.racewar,
			EACCES);
	}
	currency_command_result result = {};
	for (size_t index = 0; index < 4; ++index)
	{
		result.wallet.amount[index] = integer<int64_t>(player[index + 2]);
		// Reject values outside the shared signed domain rather than wrap them.
		const auto amount = integer<uint64_t>(bank[index + 2]);
		require(amount <= uint64_t(INT64_MAX));
		result.bank.amount[index] = static_cast<int64_t>(amount);
	}
	result.wallet_revision = integer<uint64_t>(player[6]);
	result.bank_revision = integer<uint64_t>(bank[6]);
	return result;
}
bool business_error(unsigned int code)
{
	return code == ESTALE || code == ENOSPC || code == ERANGE;
}
bool equal(const currency_command_result &left, const currency_command_result &right)
{
	return left.wallet.amount == right.wallet.amount && left.bank.amount == right.bank.amount &&
	       left.wallet_revision == right.wallet_revision &&
	       left.bank_revision == right.bank_revision;
}
fields ledger(const critical_command &command, const bank_identity &identity, uint64_t bank_id,
	      const currency_command_result &after)
{
	fields result = { { "operation_id", id(command.operation_id) },
			  { "pid", std::to_string(identity.payload.pid) },
			  { "bank_id", std::to_string(bank_id) } };
	coins(result, "wallet_delta_", identity.payload.wallet_delta);
	coins(result, "bank_delta_", identity.payload.bank_delta);
	coins(result, "wallet_after_", after.wallet);
	coins(result, "bank_after_", after.bank);
	result.insert(
		result.end(),
		{ { "wallet_revision", std::to_string(after.wallet_revision) },
		  { "bank_revision", std::to_string(after.bank_revision) },
		  { "reason_type", std::to_string(static_cast<uint16_t>(identity.payload.reason)) },
		  { "reason_id", std::to_string(identity.payload.reason_id) },
		  { "source_site", std::to_string(static_cast<uint16_t>(command.source_site)) } });
	return result;
}
fields operation(const critical_command &command, const bank_identity &identity, unsigned int code,
		 const economic_accounting_plan *plan)
{
	economic_plan_metadata metadata;
	checked(economic_intent_plan_metadata(command, identity.intent, &metadata));
	std::vector<uint8_t> encoded;
	if (plan)
		checked(economic_plan_encode(*plan, &encoded));
	return { { "operation_id", id(command.operation_id) },
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
		 { "plan_digest", plan ? hex(hash(encoded)) : "NULL" },
		 { "canonical_intent", hex(command.accounting_intent) },
		 { "canonical_plan", plan ? hex(encoded) : "NULL" },
		 { "outcome", code ? "2" : "1" },
		 { "result_code", std::to_string(code) },
		 { "account_count", plan ? std::to_string(plan->accounts.size()) : "0" },
		 { "posting_count", plan ? std::to_string(plan->postings.size()) : "0" },
		 { "child_count", "0" },
		 { "item_event_count", "0" },
		 { "before_witness_count", "0" },
		 { "after_witness_count", "0" } };
}
fields effect(const critical_operation_id &root, size_t index, const economic_account_effect &value)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
	checked(economic_account_key_encode(value.key, &encoded));
	fields result = { { "operation_id", id(root) },
			  { "account_index", std::to_string(index) },
			  { "account_key", hex(encoded) } };
	coins(result, "before_", { value.before });
	coins(result, "after_", { value.after });
	result.emplace_back("before_revision", std::to_string(value.before_revision));
	result.emplace_back("after_revision", std::to_string(value.after_revision));
	return result;
}
fields posting(const critical_operation_id &root, size_t index, const economic_coin_posting &value)
{
	fields result = { { "operation_id", id(root) },
			  { "line_index", std::to_string(index) },
			  { "event_index", std::to_string(value.event_index) },
			  { "account_index", std::to_string(value.account_index) },
			  { "child_index", std::to_string(value.child_index) },
			  { "copper_value", std::to_string(value.copper) } };
	coins(result, "delta_", { value.delta });
	return result;
}
void evidence(MYSQL *connection, const critical_command &command, const bank_identity &identity,
	      unsigned int code, const economic_accounting_plan *plan, bool append)
{
	const auto op = operation(command, identity, code, plan);
	if (append)
		insert(connection, "economic_accounting_operation", op);
	count(connection, "economic_accounting_operation", predicate(op), 1);
	const std::string where = "operation_id=" + id(command.operation_id);
	if (plan)
	{
		require(plan->accounts.size() == 2 && plan->postings.size() == 2 &&
			plan->children.empty() && plan->items_before.empty() &&
			plan->items_after.empty() && plan->item_events.empty());
		for (size_t index = 0; index < plan->accounts.size(); ++index)
		{
			const auto row = effect(command.operation_id, index, plan->accounts[index]);
			if (append)
				insert(connection, "economic_accounting_account_effect", row);
			count(connection, "economic_accounting_account_effect", predicate(row), 1);
		}
		for (size_t index = 0; index < plan->postings.size(); ++index)
		{
			const auto row =
				posting(command.operation_id, index, plan->postings[index]);
			if (append)
				insert(connection, "economic_accounting_coin_posting", row);
			count(connection, "economic_accounting_coin_posting", predicate(row), 1);
		}
	}
	count(connection, "economic_accounting_account_effect", where, plan ? 2 : 0);
	count(connection, "economic_accounting_coin_posting", where, plan ? 2 : 0);
	for (const char *table :
	     { "economic_accounting_child", "economic_accounting_item_reference",
	       "economic_accounting_source_claim", "item_ownership_ledger" })
		count(connection, table, where, 0);
}
}

struct economic_sql_bank_transaction::implementation
{
	MYSQL *connection = nullptr;
	unsigned long session = 0;
	critical_command command;
	bank_identity identity;
	economic_sql_authority_snapshot authority;
	std::array<economic_sql_mapping_request, 2> requests;
	uint64_t bank_id = 0;
	std::string checkpoint;
	currency_command_result before = {}, result = {};
	unsigned int code = 0;
	std::optional<economic_prepared_currency> prepared;
	enum class phase
	{
		prepared,
		applied,
		finalized,
		verified,
		failed
	} phase = phase::prepared;
};
economic_sql_bank_transaction::economic_sql_bank_transaction(std::unique_ptr<implementation> state)
	: state_(std::move(state))
{
}
economic_sql_bank_transaction::~economic_sql_bank_transaction() = default;
const currency_command_result &economic_sql_bank_transaction::result() const
{
	return state_->result;
}
unsigned int economic_sql_bank_transaction::result_code() const
{
	return state_->code;
}

unsigned int
economic_sql_bank_transaction::prepare(MYSQL *connection, const critical_command &command,
				       std::unique_ptr<economic_sql_bank_transaction> *transaction)
{
	try
	{
		require(connection && transaction, EINVAL);
		auto state = std::make_unique<implementation>();
		state->connection = connection;
		state->session = mysql_thread_id(connection);
		active(connection, state->session);
		state->identity = decode(command);
		state->command = command;
		inbox(connection, command, true);
		const auto &identity = state->identity;
		state->bank_id = mapping_hint(connection, identity.bank.authority_id);
		require(state->bank_id && state->bank_id <= UINT32_MAX, EILSEQ);
		state->requests = { economic_sql_mapping_request{ identity.wallet, PLAYER_LOCATOR,
								  identity.payload.pid },
				    economic_sql_mapping_request{ identity.bank, BANK_LOCATOR,
								  state->bank_id } };
		const auto error =
			economic_sql_lock_authority(connection, identity.wallet.lineage,
						    identity.intent.admission.metadata.epoch,
						    state->requests, &state->authority);
		require(!error, error);
		state->before = balances(connection, identity, state->bank_id);
		state->result = state->before;
		economic_currency_authority authority = { identity.intent.admission.metadata.epoch,
							  identity.wallet,
							  identity.bank,
							  command.keys[0],
							  command.keys[1],
							  state->before };
		const auto preparation = economic_bank_transfer_prepare(
			command, identity.intent, authority, currency_revision_policy::sql_legacy,
			&state->prepared);
		if (preparation != economic_accounting_error::ok)
		{
			require(preparation != economic_accounting_error::capacity, ENOMEM);
			std::optional<currency_prepared_mutation> domain;
			state->code = currency_prepare_mutation(
				identity.payload, state->before,
				command.expected_revisions[0].revision,
				command.expected_revisions[1].revision,
				currency_revision_policy::sql_legacy, &domain);
			// Invalid native holdings and malformed preparation are unresolved
			// authority failures, never durable business rejections.
			require(business_error(state->code), state->code ? state->code : EINVAL);
		}
		else
			state->result = state->prepared->mutation().after();
		active(connection, state->session);
		critical_operation_id checkpoint_id = {};
		require(critical_operation_id_generate(&checkpoint_id), ENOMEM);
		char checkpoint_hex[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
		require(critical_operation_id_to_hex(checkpoint_id, checkpoint_hex,
						     sizeof(checkpoint_hex)));
		state->checkpoint = std::string("economic_bank_") + checkpoint_hex;
		execute(connection, "SAVEPOINT " + state->checkpoint);
		*transaction = std::unique_ptr<economic_sql_bank_transaction>(
			new economic_sql_bank_transaction(std::move(state)));
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int economic_sql_bank_transaction::apply()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::prepared)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		// A transaction-local marker also detects rollback/restart on the same
		// connection, and invalidates this receipt after an outer savepoint rollback.
		execute(state.connection, "RELEASE SAVEPOINT " + state.checkpoint);
		execute(state.connection, "SAVEPOINT " + state.checkpoint);
		inbox(state.connection, state.command, true);
		require(equal(balances(state.connection, state.identity, state.bank_id),
			      state.before),
			ESTALE);
		const auto where = "operation_id=" + id(state.command.operation_id);
		count(state.connection, "currency_ledger", where, 0);
		count(state.connection, "economic_accounting_operation", where, 0);
		if (state.prepared)
		{
			const auto write_error = currency_sql_mutation_writer::write(
				state.connection, state.command, state.prepared->mutation(),
				static_cast<uint32_t>(state.bank_id));
			require(!write_error, write_error);
		}
		active(state.connection, state.session);
		state.phase = implementation::phase::applied;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int economic_sql_bank_transaction::finalize()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::applied)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		// A transaction-local marker also detects rollback/restart on the same
		// connection, and invalidates this receipt after an outer savepoint rollback.
		execute(state.connection, "RELEASE SAVEPOINT " + state.checkpoint);
		execute(state.connection, "SAVEPOINT " + state.checkpoint);
		inbox(state.connection, state.command, true);
		auto verify_authority = [&]
		{
			economic_sql_authority_snapshot authority;
			const auto error = economic_sql_lock_authority(state.connection,
								       state.authority.lineage,
								       state.authority.epoch,
								       state.requests, &authority);
			require(!error, error);
			require(authority.lineage_revision == state.authority.lineage_revision &&
					authority.mappings.size() ==
						state.authority.mappings.size(),
				ESTALE);
			for (size_t index = 0; index < authority.mappings.size(); ++index)
				require(authority.mappings[index].revision ==
						state.authority.mappings[index].revision,
					ESTALE);
		};
		verify_authority();
		require(equal(balances(state.connection, state.identity, state.bank_id),
			      state.result));
		count(state.connection, "currency_ledger",
		      "operation_id=" + id(state.command.operation_id), state.prepared ? 1 : 0);
		if (state.prepared)
			count(state.connection, "currency_ledger",
			      predicate(ledger(state.command, state.identity, state.bank_id,
					       state.result)),
			      1);
		evidence(state.connection, state.command, state.identity, state.code,
			 state.prepared ? &state.prepared->plan() : nullptr, true);
		evidence(state.connection, state.command, state.identity, state.code,
			 state.prepared ? &state.prepared->plan() : nullptr, false);
		inbox(state.connection, state.command, true);
		verify_authority();
		count(state.connection, "currency_ledger",
		      "operation_id=" + id(state.command.operation_id), state.prepared ? 1 : 0);
		require(equal(balances(state.connection, state.identity, state.bank_id),
			      state.result));
		if (state.prepared)
			count(state.connection, "currency_ledger",
			      predicate(ledger(state.command, state.identity, state.bank_id,
					       state.result)),
			      1);
		active(state.connection, state.session);
		state.phase = implementation::phase::finalized;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int economic_sql_bank_transaction::verify_root_completion()
{
	auto &state = *state_;
	if (state.phase != implementation::phase::finalized)
		return EPERM;
	state.phase = implementation::phase::failed;
	try
	{
		active(state.connection, state.session);
		execute(state.connection, "RELEASE SAVEPOINT " + state.checkpoint);
		std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> payload = {};
		require(currency_command_encode_result(state.result, &payload));
		const auto retained_error = economic_sql_bank_verify_retained(
			state.connection, state.command, state.code, payload);
		require(!retained_error, retained_error);
		economic_sql_authority_snapshot authority;
		const auto authority_error = economic_sql_lock_authority(
			state.connection, state.authority.lineage, state.authority.epoch,
			state.requests, &authority);
		require(!authority_error, authority_error);
		require(authority.lineage_revision == state.authority.lineage_revision &&
				authority.mappings.size() == state.authority.mappings.size(),
			ESTALE);
		for (size_t index = 0; index < authority.mappings.size(); ++index)
			require(authority.mappings[index].revision ==
					state.authority.mappings[index].revision,
				ESTALE);
		require(equal(balances(state.connection, state.identity, state.bank_id),
			      state.result));
		const auto where = "operation_id=" + id(state.command.operation_id);
		count(state.connection, "critical_outbox", where, state.prepared ? 1 : 0);
		if (state.prepared)
			count(state.connection, "critical_outbox",
			      predicate({ { "operation_id", id(state.command.operation_id) },
					  { "event_index", "0" },
					  { "destination", "3" },
					  { "event_type", "1" },
					  { "payload_version", "1" },
					  { "payload", hex(payload) },
					  { "status", "0" },
					  { "attempt_count", "0" },
					  { "last_error_code", "0" } }) +
				      " AND delivered_at IS NULL AND dead_lettered_at IS NULL AND next_attempt_at<=CURRENT_TIMESTAMP(6)",
			      1);
		active(state.connection, state.session);
		state.phase = implementation::phase::verified;
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

unsigned int economic_sql_bank_verify_retained(MYSQL *connection, const critical_command &command,
					       unsigned int result_code,
					       std::span<const uint8_t> result_payload)
{
	try
	{
		require(connection, EINVAL);
		const auto identity = decode(command);
		inbox(connection, command, false);
		count(connection, "critical_operation_inbox",
		      predicate({ { "operation_id", id(command.operation_id) },
				  { "result_code", std::to_string(result_code) },
				  { "result_payload", hex(result_payload) } }),
		      1);
		currency_command_result result = {};
		require(currency_command_decode_result(result_payload.data(), result_payload.size(),
						       &result));
		count(connection, "critical_operation_inbox",
		      predicate({ { "operation_id", id(command.operation_id) },
				  { "durable_revision",
				    std::to_string(std::max(result.wallet_revision,
							    result.bank_revision)) } }),
		      1);
		const auto row = read(
			connection,
			"SELECT canonical_plan FROM economic_accounting_operation WHERE operation_id=" +
				id(command.operation_id),
			1);
		std::optional<economic_prepared_currency> prepared;
		economic_accounting_plan plan;
		if (!result_code)
		{
			require(row[0].has_value() &&
				row[0]->size() <= ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES);
			checked(economic_plan_decode(
				std::span<const uint8_t>(
					reinterpret_cast<const uint8_t *>(row[0]->data()),
					row[0]->size()),
				&plan));
			require(plan.accounts.size() == 2);
			economic_currency_authority authority = {
				identity.intent.admission.metadata.epoch,
				identity.wallet,
				identity.bank,
				command.keys[0],
				command.keys[1],
				{}
			};
			for (const auto &account : plan.accounts)
			{
				if (economic_account_key_equal(account.key, identity.wallet))
				{
					authority.state.wallet.amount = account.before;
					authority.state.wallet_revision = account.before_revision;
				}
				else if (economic_account_key_equal(account.key, identity.bank))
				{
					authority.state.bank.amount = account.before;
					authority.state.bank_revision = account.before_revision;
				}
				else
					throw failure{ EILSEQ };
			}
			checked(economic_bank_transfer_prepare(command, identity.intent, authority,
							       currency_revision_policy::sql_legacy,
							       &prepared));
			checked(prepared->agrees_with(plan));
			require(equal(prepared->mutation().after(), result));
		}
		else
		{
			require(!row[0].has_value() && business_error(result_code));
			// A rejection publishes its unchanged before-state. Reproduce the
			// original native decision from that witness, never current balances.
			// This also rejects codec-valid negative/oversized holdings before
			// revision checks can make them look like a valid stale decision.
			std::optional<currency_prepared_mutation> rejected;
			require(currency_prepare_mutation(identity.payload, result,
							  command.expected_revisions[0].revision,
							  command.expected_revisions[1].revision,
							  currency_revision_policy::sql_legacy,
							  &rejected) == result_code);
		}
		const auto where = "operation_id=" + id(command.operation_id);
		const auto bank_id =
			prepared ?
				integer<uint64_t>(
					read(connection,
					     "SELECT bank_id FROM currency_ledger WHERE " + where,
					     1)[0]) :
				mapping_hint(connection, identity.bank.authority_id);
		require(bank_id && bank_id <= UINT32_MAX);
		if (prepared)
			count(connection, "currency_ledger",
			      predicate(ledger(command, identity, bank_id, result)), 1);
		else
			count(connection, "currency_ledger", where, 0);
		count(connection, "economic_account_mapping",
		      predicate({ { "mapping_id", std::to_string(identity.wallet.authority_id) },
				  { "lineage", id(identity.wallet.lineage) },
				  { "account_kind", "1" },
				  { "context_id", "0" },
				  { "backend_kind", "1" },
				  { "locator_kind", std::to_string(PLAYER_LOCATOR) },
				  { "native_id", std::to_string(identity.payload.pid) } }),
		      1);
		// Retained native bindings survive retirement; no active-epoch,
		// active mapping or current balance is consulted during replay.
		count(connection, "economic_account_mapping",
		      predicate({ { "mapping_id", std::to_string(identity.bank.authority_id) },
				  { "lineage", id(identity.bank.lineage) },
				  { "account_kind", "2" },
				  { "context_id", std::to_string(identity.bank.context_id) },
				  { "backend_kind", "1" },
				  { "locator_kind", std::to_string(BANK_LOCATOR) },
				  { "native_id", std::to_string(bank_id) } }),
		      1);
		evidence(connection, command, identity, result_code,
			 prepared ? &prepared->plan() : nullptr, false);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
#endif
