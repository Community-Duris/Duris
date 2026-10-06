#include "persistence/economic_sql_baseline_transaction.h"
#include "persistence/economic_sql_pending_claim_source.h"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <climits>
#include <cstring>
#include <memory>
#include <new>
#include <openssl/sha.h>
#include <type_traits>

#ifdef __NO_MYSQL__
unsigned int
economic_sql_baseline_verify_known_retained_in_transaction(MYSQL *, const critical_operation_id &,
							   economic_baseline_batch *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_baseline_verify_retained_in_transaction(MYSQL *, const critical_command &,
								  uint64_t *) noexcept
{
	return ENOTSUP;
}
unsigned int economic_sql_baseline_transaction::initialize(MYSQL *, const critical_operation_id &,
							   const critical_operation_id &,
							   const economic_account_key &,
							   const critical_operation_id &)
{
	return ENOTSUP;
}
critical_apply_result economic_sql_baseline_transaction::run(MYSQL *, const critical_command &,
							     const economic_prepared_baseline *)
{
	return { critical_apply_outcome::retryable_failure, 0, ENOTSUP };
}
#else
namespace
{
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
fields receipt_identity(const critical_command &command)
{
	std::vector<uint8_t> bytes, keys;
	const auto encoded = critical_command_encode(command, &bytes);
	require(encoded == critical_command_codec_result::ok,
		encoded == critical_command_codec_result::overflow ? ENOMEM : EINVAL);
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (size_t i = 0; i < 8; ++i)
			keys.push_back(static_cast<uint8_t>(key.id >> (8 * i)));
	}
	return { { "operation_id", id(command.operation_id) },
		 { "command_hash", hex(hash(bytes)) },
		 { "keys_hash", hex(hash(keys)) },
		 { "command_type", std::to_string(static_cast<uint16_t>(command.type)) },
		 { "schema_version", std::to_string(command.schema_version) },
		 { "payload_version", std::to_string(command.payload_version) } };
}
void reserve(MYSQL *connection, const critical_command &command)
{
	auto values = receipt_identity(command);
	values.emplace_back("status", "0");
	values.emplace_back("result_payload", "X''");
	insert(connection, "critical_operation_inbox", values);
}

void membership(MYSQL *connection, const critical_operation_id &lineage,
		const critical_operation_id &epoch)
{
	require(!critical_operation_id_is_zero(lineage) && !critical_operation_id_is_zero(epoch),
		EINVAL);
	(void)read(connection,
		   "SELECT lineage FROM economic_lineage_state WHERE lineage=" + id(lineage) +
			   " LOCK IN SHARE MODE",
		   1);
	(void)read(connection,
		   "SELECT epoch FROM economic_epoch WHERE lineage=" + id(lineage) +
			   " AND epoch=" + id(epoch) + " LOCK IN SHARE MODE",
		   1);
}
std::string scope(const critical_operation_id &lineage, const critical_operation_id &epoch)
{
	return "lineage=" + id(lineage) + " AND epoch=" + id(epoch);
}
std::string key(const economic_account_key &value)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> bytes;
	checked(economic_account_key_encode(value, &bytes));
	return hex(bytes);
}
struct control
{
	uint64_t revision;
	std::string last;
};
control book(MYSQL *connection, const economic_baseline_batch &batch, bool writing)
{
	membership(connection, batch.lineage, batch.epoch);
	const auto where = scope(batch.lineage, batch.epoch);
	auto row = read(
		connection,
		"SELECT opening_account,revision,last_operation_id,creating_operation_id FROM economic_baseline_control WHERE " +
			where + (writing ? " FOR UPDATE" : " LOCK IN SHARE MODE"),
		4);
	require(row[0] && row[0]->size() == ECONOMIC_ACCOUNT_KEY_BYTES);
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> opening;
	checked(economic_account_key_encode(batch.opening_account, &opening));
	require(std::equal(opening.begin(), opening.end(),
			   reinterpret_cast<const uint8_t *>(row[0]->data())),
		EEXIST);
	require(row[3] && row[3]->size() == 16);
	const auto creator = hex(std::span(reinterpret_cast<const uint8_t *>(row[3]->data()), 16));
	count(connection, "critical_operation_inbox",
	      "operation_id=" + creator +
		      " AND status=1 AND result_code=0 AND failure_stage=0 AND committed_at IS NOT NULL",
	      1);
	control result{ integer<uint64_t>(row[1]), "NULL" };
	if (!result.revision)
		require(!row[2]);
	else
	{
		require(row[2] && row[2]->size() == 16);
		result.last = hex(std::span(reinterpret_cast<const uint8_t *>(row[2]->data()), 16));
		count(connection, "economic_baseline_witness",
		      where + " AND operation_id=" + result.last +
			      " AND book_revision=" + std::to_string(result.revision),
		      1);
		count(connection, "critical_operation_inbox",
		      "operation_id=" + result.last +
			      " AND status=1 AND result_code=0 AND failure_stage=0 AND OCTET_LENGTH(result_payload)=0 AND durable_revision=" +
			      std::to_string(result.revision) + " AND committed_at IS NOT NULL",
		      1);
	}
	return result;
}
void coins(fields &values, const std::string &prefix, const economic_coin_vector &value)
{
	constexpr const char *names[] = { "copper", "silver", "gold", "platinum" };
	for (size_t i = 0; i < 4; ++i)
		values.emplace_back(prefix + names[i], std::to_string(value[i]));
}
// Expected rows are generated only from a regenerated baseline plan/witness.
// Indexed root predicates plus at most 64 alternatives bound each verification
// statement; no scan of another epoch's complete reservation book is required.
void rows(MYSQL *connection, const std::string &table, const std::string &where,
	  const std::vector<fields> &values, bool append)
{
	constexpr size_t batch_size = 64;
	for (size_t offset = 0; offset < values.size(); offset += batch_size)
	{
		const size_t end = std::min(values.size(), offset + batch_size);
		std::string names, data, match;
		for (size_t i = offset; i < end; ++i)
		{
			if (i > offset)
			{
				data += ',';
				match += " OR ";
			}
			data += '(';
			match += '(' + predicate(values[i]) + ')';
			for (size_t j = 0; j < values[i].size(); ++j)
			{
				if (j)
					data += ',';
				data += values[i][j].second;
				if (i == offset)
				{
					if (j)
						names += ',';
					names += values[i][j].first;
				}
			}
			data += ')';
		}
		require(data.size() + names.size() < 128 * 1024 && match.size() < 128 * 1024,
			E2BIG);
		if (append)
		{
			execute(connection,
				"INSERT INTO " + table + "(" + names + ") VALUES" + data);
			require(mysql_affected_rows(connection) == end - offset);
		}
		count(connection, table, where + " AND (" + match + ")", end - offset);
	}
	count(connection, table, where, values.size());
}
void evidence_values(MYSQL *connection, const critical_operation_id &operation,
		     std::span<const uint8_t> canonical_intent,
		     std::optional<uint64_t> accepted_at_usec,
		     const economic_prepared_baseline &prepared,
		     const economic_accounting_plan &plan, uint64_t revision, bool append)
{
	std::vector<uint8_t> encoded, witness;
	checked(economic_plan_encode(plan, &encoded));
	checked(economic_baseline_encode(prepared, &witness));
	const auto &m = plan.metadata;
	const auto &b = prepared.witness();
	require(m.source_event && plan.children.empty() && plan.item_events.empty());
	const auto contract_error = economic_sql_pending_claim_source_contract(connection);
	require(!contract_error, contract_error);
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source;
	checked(economic_source_event_encode(*m.source_event, &source));
	fields op = { { "operation_id", id(operation) },
		      { "lineage", id(m.lineage) },
		      { "epoch", id(m.epoch) },
		      { "original_operation_id", "NULL" },
		      { "accounting_version", std::to_string(m.version) },
		      { "writer_id", std::to_string(m.writer_id) },
		      { "policy_version", std::to_string(m.policy_version) },
		      { "compiler_version", std::to_string(m.compiler_version) },
		      { "actor_kind", std::to_string(static_cast<unsigned>(m.actor_kind)) },
		      { "actor_id", std::to_string(m.actor_id) },
		      { "reason", std::to_string(static_cast<unsigned>(m.reason)) },
		      { "source_event", hex(source) },
		      { "realized_price_copper", "NULL" },
		      { "intent_digest", hex(m.intent_digest) },
		      { "domain_digest", hex(m.domain_digest) },
		      { "plan_digest", hex(hash(encoded)) },
		      { "canonical_intent", hex(canonical_intent) },
		      { "canonical_plan", hex(encoded) },
		      { "outcome", "1" },
		      { "result_code", "0" },
		      { "account_count", std::to_string(plan.accounts.size()) },
		      { "posting_count", std::to_string(plan.postings.size()) },
		      { "child_count", "0" },
		      { "item_event_count", "0" },
		      { "before_witness_count", std::to_string(plan.items_before.size()) },
		      { "after_witness_count", std::to_string(plan.items_after.size()) } };
	if (append)
		insert(connection, "economic_accounting_operation", op);
	count(connection, "economic_accounting_operation", predicate(op), 1);
	const auto root = "operation_id=" + id(operation);
	std::vector<fields> effects, postings, reservations;
	for (size_t i = 0; i < plan.accounts.size(); ++i)
	{
		const auto &a = plan.accounts[i];
		fields f = { { "operation_id", id(operation) },
			     { "account_index", std::to_string(i) },
			     { "account_key", key(a.key) } };
		coins(f, "before_", a.before);
		coins(f, "after_", a.after);
		f.emplace_back("before_revision", std::to_string(a.before_revision));
		f.emplace_back("after_revision", std::to_string(a.after_revision));
		effects.push_back(std::move(f));
	}
	for (size_t i = 0; i < plan.postings.size(); ++i)
	{
		const auto &a = plan.postings[i];
		fields f = { { "operation_id", id(operation) },
			     { "line_index", std::to_string(i) },
			     { "event_index", std::to_string(a.event_index) },
			     { "account_index", std::to_string(a.account_index) },
			     { "child_index", std::to_string(a.child_index) },
			     { "copper_value", std::to_string(a.copper) } };
		coins(f, "delta_", a.delta);
		postings.push_back(std::move(f));
	}
	rows(connection, "economic_accounting_account_effect", root, effects, append);
	rows(connection, "economic_accounting_coin_posting", root, postings, append);
	fields claim = { { "lineage", id(m.lineage) },
			 { "source_event", hex(source) },
			 { "operation_id", id(operation) },
			 { "outcome", "1" } };
	if (append)
		insert(connection, "economic_accounting_source_claim", claim);
	count(connection, "economic_accounting_source_claim", predicate(claim), 1);
	count(connection, "economic_accounting_source_claim", root, 1);
	fields batch = { { "operation_id", id(operation) },
			 { "lineage", id(m.lineage) },
			 { "epoch", id(m.epoch) },
			 { "book_revision", std::to_string(revision) },
			 { "witness_version", std::to_string(b.witness_version) },
			 { "holding_count", std::to_string(b.holdings.size()) },
			 { "item_count", std::to_string(b.items.size()) },
			 { "witness_digest", hex(hash(witness)) },
			 { "canonical_witness", hex(witness) } };
	if (append)
	{
		// The normalized EAI1 binding intentionally omits original admission time.
		// Retain that native preimage fact with this same immutable witness/root
		// transaction; SQL creation time is never a substitute.
		require(accepted_at_usec && *accepted_at_usec, EINVAL);
		batch.emplace_back("claim_origin_version", "1");
		batch.emplace_back("command_accepted_at_usec", std::to_string(*accepted_at_usec));
		insert(connection, "economic_baseline_witness", batch);
	}
	// Historical NULL witnesses remain original-ID replay compatible. The
	// explicit retained timestamp check below never backfills committed rows.
	count(connection, "economic_baseline_witness", predicate(batch), 1);
	// NULL is an explicit historical contract, never an invitation to backfill
	// old EAB1/EAB2 operations. New lifecycle money openings bind this marker
	// through their versioned request and require it for complete admission.
	const auto origin_marker =
		read(connection,
		     "SELECT claim_origin_version FROM economic_baseline_witness WHERE " + root +
			     " LOCK IN SHARE MODE",
		     1);
	const bool money_opening = std::any_of(
		b.holdings.begin(), b.holdings.end(),
		[](const auto &holding)
		{
			return holding.account.kind == economic_account_kind::pending_claim ||
			       holding.account.kind == economic_account_kind::auction_escrow;
		});
	if (money_opening)
	{
		const auto parent = read(
			connection,
			"SELECT COUNT(*) FROM economic_sql_lifecycle_installation WHERE operation_id=" +
				id(b.preparation_id),
			1);
		const auto parents = integer<uint64_t>(parent[0]);
		require(parents <= 1);
		// Historical NULL roots without a money lifecycle remain unknown. A
		// declared new origin requires its authentic original V2 preparation;
		// dropping SQL origin rows or the timestamp cannot downgrade that root.
		if (parents || origin_marker[0])
		{
			require(parents == 1 && accepted_at_usec && *accepted_at_usec &&
					origin_marker[0] &&
					integer<uint16_t>(origin_marker[0]) == 1,
				ENODATA);
			const std::string_view domain = "DURIS-SQL-LIFECYCLE-V2";
			std::vector<uint8_t> request(domain.begin(), domain.end());
			const auto number = [&](uint64_t value)
			{
				for (size_t i = 0; i < 8; ++i)
					request.push_back(static_cast<uint8_t>(value >> (8 * i)));
			};
			for (const auto &value : { b.preparation_id, b.lineage, b.epoch })
			{
				number(value.bytes.size());
				request.insert(request.end(), value.bytes.begin(),
					       value.bytes.end());
			}
			number(b.actor_id);
			number(*accepted_at_usec);
			const auto request_hash = hex(hash(request));
			count(connection,
			      "economic_sql_lifecycle_installation p JOIN critical_operation_inbox i ON i.operation_id=p.operation_id",
			      "p.operation_id=" + id(b.preparation_id) + " AND p.lineage=" +
				      id(b.lineage) + " AND p.epoch=" + id(b.epoch) +
				      " AND p.native_boundary_digest=" + hex(b.boundary_digest) +
				      " AND p.request_digest=" + request_hash +
				      " AND p.phase IN (1,2) AND (p.baseline_operation_id IS NULL OR p.baseline_operation_id=" +
				      id(operation) + ") AND i.command_hash=" + request_hash +
				      " AND i.keys_hash=" + hex(hash({})) + " AND i.command_type=" +
				      std::to_string(static_cast<uint16_t>(
					      critical_command_type::economic_baseline)) +
				      " AND i.schema_version=" +
				      std::to_string(CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) +
				      " AND i.payload_version=1 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND i.durable_revision=0 AND OCTET_LENGTH(i.result_payload)=0 AND i.committed_at IS NOT NULL",
			      1);
		}
	}
	if (origin_marker[0])
	{
		require(integer<uint16_t>(origin_marker[0]) == 1);
		for (size_t index = 0; index < b.holdings.size(); ++index)
		{
			const auto &holding = b.holdings[index];
			if (holding.account.kind != economic_account_kind::pending_claim)
				continue;
			require(holding.account.context_id == 0 && holding.balance[0] >= 0 &&
					static_cast<uint64_t>(holding.balance[0]) <= UINT_MAX &&
					!holding.balance[1] && !holding.balance[2] &&
					!holding.balance[3],
				ERANGE);
			if (!holding.balance[0])
				continue;
			require(index < UINT16_MAX, E2BIG);
			const auto mapped = read(
				connection,
				"SELECT native_id FROM economic_account_mapping WHERE mapping_id=" +
					std::to_string(holding.account.authority_id) +
					" AND lineage=" + id(m.lineage) +
					" AND account_kind=5 AND context_id=0 AND backend_kind=1 AND locator_kind=5 "
					"LOCK IN SHARE MODE",
				1);
			const auto pid = integer<uint32_t>(mapped[0]);
			require(pid, EILSEQ);
			const auto amount = static_cast<uint64_t>(holding.balance[0]);
			const auto slot = static_cast<uint16_t>(index + 1);
			if (append)
			{
				const auto staged = economic_sql_pending_claim_source_stage(
					connection, operation, slot, holding.account, pid, amount);
				require(!staged, staged);
			}
		}
		const auto origin_error =
			economic_sql_pending_claim_source_verify_baseline(connection, operation, b);
		require(!origin_error, origin_error);
	}

	auto reservation = [&](unsigned kind, uint64_t identity)
	{
		reservations.push_back({ { "lineage", id(m.lineage) },
					 { "epoch", id(m.epoch) },
					 { "identity_kind", std::to_string(kind) },
					 { "identity_id", std::to_string(identity) },
					 { "operation_id", id(operation) } });
	};
	for (const auto &a : b.holdings)
		reservation(1, a.account.authority_id);
	for (const auto &a : b.items)
		reservation(2, a.snapshot.uid);
	rows(connection, "economic_baseline_reservation",
	     scope(m.lineage, m.epoch) + " AND " + root, reservations, append);
	for (const char *table :
	     { "economic_accounting_child", "economic_accounting_item_reference", "currency_ledger",
	       "item_ownership_ledger", "critical_outbox" })
		count(connection, table, root, 0);
	count(connection, "economic_accounting_child", "child_operation_id=" + id(operation), 0);
}
void evidence(MYSQL *connection, const critical_command &command,
	      const economic_prepared_baseline &prepared, uint64_t revision, bool append)
{
	economic_accounting_plan plan;
	checked(economic_baseline_command_plan(command, prepared, &plan));
	evidence_values(connection, command.operation_id, command.accounting_intent,
			command.accepted_at_usec, prepared, plan, revision, append);
}

uint64_t verify(MYSQL *connection, const critical_command &command)
{
	// Original inbox identity is checked before decoding any retained intent.
	auto row = read(
		connection,
		"SELECT status,durable_revision,result_code,failure_stage,result_payload,committed_at FROM critical_operation_inbox WHERE operation_id=" +
			id(command.operation_id) + " FOR UPDATE",
		6);
	count(connection, "critical_operation_inbox", predicate(receipt_identity(command)), 1);
	require(integer<unsigned>(row[0]) == 1, EAGAIN);
	const auto revision = integer<uint64_t>(row[1]);
	require(revision && !integer<unsigned>(row[2]) && !integer<unsigned>(row[3]) && row[4] &&
		row[4]->empty() && row[5]);
	auto stored = read(
		connection,
		"SELECT canonical_witness,book_revision,command_accepted_at_usec,claim_origin_version FROM economic_baseline_witness WHERE operation_id=" +
			id(command.operation_id),
		4);
	require(stored[0] && stored[0]->size() <= ECONOMIC_BASELINE_MAX_BYTES &&
		integer<uint64_t>(stored[1]) == revision);
	std::optional<economic_prepared_baseline> prepared;
	checked(economic_baseline_decode(
		std::span(reinterpret_cast<const uint8_t *>(stored[0]->data()), stored[0]->size()),
		&prepared));
	const auto current = book(connection, prepared->witness(), false);
	require(revision <= current.revision);
	// The initial bounded witness read supplies routing/decoding only. Retain
	// native inbox -> lineage/epoch -> book -> witness lock order, and bind
	// every decoded byte plus optional historical state to the locked row.
	const auto locked = read(
		connection,
		"SELECT canonical_witness,book_revision,command_accepted_at_usec,claim_origin_version FROM economic_baseline_witness WHERE operation_id=" +
			id(command.operation_id) + " LOCK IN SHARE MODE",
		4);
	require(locked == stored);
	// Old NULL rows still require the original full command_hash above and all
	// canonical evidence below. A present invalid/mismatched value may never
	// take that compatibility path or be replaced with the caller's timestamp.
	if (locked[2])
	{
		const auto accepted_at_usec = integer<uint64_t>(locked[2]);
		require(accepted_at_usec && accepted_at_usec == command.accepted_at_usec);
	}
	evidence(connection, command, *prepared, revision, false);
	return revision;
}
struct transaction
{
	MYSQL *connection;
	unsigned long session;
	bool started = false;
	~transaction()
	{
		if (started && mysql_thread_id(connection) == session)
			(void)mysql_real_query(connection, "ROLLBACK", 8);
	}
};
}
unsigned int economic_sql_baseline_verify_retained_in_transaction(
	MYSQL *connection, const critical_command &command, uint64_t *durable_revision) noexcept
{
	try
	{
		require(connection && durable_revision &&
				command.type == critical_command_type::economic_baseline &&
				command.schema_version ==
					CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
				critical_command_envelope_valid(command),
			EINVAL);
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
				!reconnect,
			EPERM);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const auto revision = verify(connection, command);
		active(connection, session);
		*durable_revision = revision;
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
	catch (...)
	{
		return EIO;
	}
}
unsigned int economic_sql_baseline_verify_known_retained_in_transaction(
	MYSQL *connection, const critical_operation_id &operation,
	economic_baseline_batch *optional_witness) noexcept
{
	try
	{
		require(connection && !critical_operation_id_is_zero(operation), EINVAL);
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
				!reconnect,
			EPERM);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		const auto receipt = read(
			connection,
			"SELECT status,durable_revision,result_code,failure_stage,result_payload,committed_at,"
			"command_type,schema_version,payload_version,OCTET_LENGTH(command_hash),OCTET_LENGTH(keys_hash) "
			"FROM critical_operation_inbox WHERE operation_id=" +
				id(operation) + " LOCK IN SHARE MODE",
			11);
		const auto revision = integer<uint64_t>(receipt[1]);
		require(integer<unsigned>(receipt[0]) == 1 && revision &&
			integer<unsigned>(receipt[2]) == 0 && integer<unsigned>(receipt[3]) == 0 &&
			receipt[4] && receipt[4]->empty() && receipt[5] &&
			integer<unsigned>(receipt[6]) ==
				static_cast<unsigned>(critical_command_type::economic_baseline) &&
			integer<unsigned>(receipt[7]) ==
				CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			integer<unsigned>(receipt[8]) == 1 && integer<unsigned>(receipt[9]) == 32 &&
			integer<unsigned>(receipt[10]) == 32);
		const auto stored = read(
			connection,
			"SELECT canonical_witness,book_revision,command_accepted_at_usec,claim_origin_version "
			"FROM economic_baseline_witness WHERE operation_id=" +
				id(operation),
			4);
		require(stored[0] && stored[0]->size() <= ECONOMIC_BASELINE_MAX_BYTES &&
			integer<uint64_t>(stored[1]) == revision);
		std::optional<economic_prepared_baseline> prepared;
		checked(economic_baseline_decode(
			std::span(reinterpret_cast<const uint8_t *>(stored[0]->data()),
				  stored[0]->size()),
			&prepared));
		require(prepared->plan().metadata.operation_id.bytes == operation.bytes);
		if (stored[2])
		{
			// A real retained timestamp permits full original identity proof,
			// including historical NULL origin markers. No timestamp is invented.
			critical_command original;
			checked(economic_baseline_command_build(
				*prepared, integer<uint64_t>(stored[2]), &original));
			require(verify(connection, original) == revision);
		}
		else
		{
			require(!stored[3]); // New origin markers must retain their real timestamp.
			const auto current = book(connection, prepared->witness(), false);
			require(revision <= current.revision);
			const auto locked = read(
				connection,
				"SELECT canonical_witness,book_revision,command_accepted_at_usec,claim_origin_version "
				"FROM economic_baseline_witness WHERE operation_id=" +
					id(operation) + " LOCK IN SHARE MODE",
				4);
			require(locked == stored);
			const auto encoded_intent = read(
				connection,
				"SELECT canonical_intent FROM economic_accounting_operation WHERE operation_id=" +
					id(operation) + " LOCK IN SHARE MODE",
				1);
			require(encoded_intent[0] &&
				encoded_intent[0]->size() <=
					CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES);
			const auto intent_bytes = std::span(
				reinterpret_cast<const uint8_t *>(encoded_intent[0]->data()),
				encoded_intent[0]->size());
			economic_frozen_intent intent;
			checked(economic_intent_decode(intent_bytes, &intent));
			auto expected_intent = intent;
			expected_intent.admission.metadata = prepared->plan().metadata;
			expected_intent.admission.facts.clear();
			expected_intent.admission.facts_version = 1;
			std::vector<uint8_t> canonical_intent;
			checked(economic_intent_encode(expected_intent, &canonical_intent));
			require(canonical_intent.size() == intent_bytes.size() &&
				std::equal(canonical_intent.begin(), canonical_intent.end(),
					   intent_bytes.begin()));
			// Validate the fixed EBC1 payload domain directly. This is value
			// framing, not reconstruction of an unknown command header/time.
			std::vector<uint8_t> payload(ECONOMIC_BASELINE_COMMAND_BYTES, 0);
			payload[0] = 'E';
			payload[1] = 'B';
			payload[2] = 'C';
			payload[3] = '1';
			auto put = [](std::vector<uint8_t> &bytes, size_t offset, uint64_t value,
				      size_t width)
			{
				for (size_t i = 0; i < width; ++i)
					bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
			};
			put(payload, 4, 1, 2);
			put(payload, 6, ECONOMIC_BASELINE_COMMAND_BYTES, 2);
			put(payload, 8, stored[0]->size(), 4);
			const auto witness_hash =
				hash(std::span(reinterpret_cast<const uint8_t *>(stored[0]->data()),
					       stored[0]->size()));
			std::copy(witness_hash.begin(), witness_hash.end(), payload.begin() + 16);
			std::vector<uint8_t> domain(8 + payload.size(), 0);
			put(domain, 0,
			    static_cast<uint16_t>(critical_command_type::economic_baseline), 2);
			put(domain, 2, 1, 2);
			put(domain, 4, payload.size(), 4);
			std::copy(payload.begin(), payload.end(), domain.begin() + 8);
			static constexpr char tag[] = "DURIS-ECONOMIC-DOMAIN-V1";
			domain.insert(domain.begin(), tag, tag + sizeof(tag));
			require(hash(domain) == intent.domain_digest);
			auto plan = prepared->plan();
			static_cast<economic_operation_metadata &>(plan.metadata) =
				intent.admission.metadata;
			checked(economic_intent_digest(intent, &plan.metadata.intent_digest));
			plan.metadata.domain_digest = intent.domain_digest;
			evidence_values(connection, operation, intent_bytes, std::nullopt,
					*prepared, plan, revision, false);
			// The unknown full command hash/header remains unobserved. This
			// API authenticates retained known witness values, never that header.
		}
		std::optional<economic_baseline_batch> result;
		if (optional_witness)
			result = prepared->witness();
		static_assert(std::is_nothrow_move_assignable_v<economic_baseline_batch>);
		active(connection, session);
		if (optional_witness)
			*optional_witness = std::move(*result);
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
	catch (...)
	{
		return EIO;
	}
}
unsigned int economic_sql_baseline_transaction::initialize(
	MYSQL *connection, const critical_operation_id &lineage, const critical_operation_id &epoch,
	const economic_account_key &opening, const critical_operation_id &creating_operation)
{
	try
	{
		require(connection, EINVAL);
		const auto session = mysql_thread_id(connection);
		active(connection, session);
		require(economic_account_key_valid(opening) &&
				opening.kind == economic_account_kind::opening &&
				opening.lineage.bytes == lineage.bytes &&
				!critical_operation_id_is_zero(creating_operation),
			EINVAL);
		membership(connection, lineage, epoch);
		count(connection, "critical_operation_inbox",
		      "operation_id=" + id(creating_operation) + " AND status IN (0,1)", 1);
		insert(connection, "economic_baseline_control",
		       { { "lineage", id(lineage) },
			 { "epoch", id(epoch) },
			 { "opening_account", key(opening) },
			 { "creating_operation_id", id(creating_operation) } });
		active(connection, session);
		return 0;
	}
	catch (const failure &e)
	{
		return e.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
critical_apply_result
economic_sql_baseline_transaction::run(MYSQL *connection, const critical_command &command,
				       const economic_prepared_baseline *prepared)
{
	bool committing = false;
	try
	{
		require(connection && command.type == critical_command_type::economic_baseline &&
				command.schema_version ==
					CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
				critical_command_envelope_valid(command),
			EINVAL);
		require(!(connection->server_status & SERVER_STATUS_IN_TRANS), EBUSY);
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		require(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
				!reconnect,
			EPERM);
		// The verified runtime connection contract is READ COMMITTED. In a
		// repeatable-read snapshot, a replay can observe a newer locked book
		// and older witness rows after waiting behind another batch.
		const char *server = mysql_get_server_info(connection);
		require(server, ENOTCONN);
		auto isolation = read(connection,
				      strstr(server, "MariaDB") ?
					      "SELECT @@SESSION.tx_isolation" :
					      "SELECT @@SESSION.transaction_isolation",
				      1);
		require(isolation[0] && *isolation[0] == "READ-COMMITTED", EPERM);
		transaction owner{ connection, mysql_thread_id(connection) };
		owner.started = true;
		execute(connection, "START TRANSACTION");
		active(connection, owner.session);
		bool replay = !prepared;
		if (prepared)
		{
			try
			{
				reserve(connection, command);
			}
			catch (const failure &e)
			{
				if (e.code != 1062)
					throw;
				replay = true;
			}
		}
		if (replay)
		{
			// Distinguish changed command identity from damaged retained evidence.
			(void)read(
				connection,
				"SELECT operation_id FROM critical_operation_inbox WHERE operation_id=" +
					id(command.operation_id) + " FOR UPDATE",
				1);
			require(integer<uint64_t>(read(
					connection,
					"SELECT COUNT(*) FROM critical_operation_inbox WHERE " +
						predicate(receipt_identity(command)),
					1)[0]) == 1,
				EEXIST);
			const auto revision = verify(connection, command);
			active(connection, owner.session);
			return { critical_apply_outcome::already_applied, revision, 0 };
		}
		economic_accounting_plan plan;
		checked(economic_baseline_command_plan(command, *prepared, &plan));
		auto current = book(connection, prepared->witness(), true);
		require(current.revision < UINT64_MAX, ERANGE);
		const auto root = "operation_id=" + id(command.operation_id);
		for (const char *table :
		     { "economic_accounting_operation", "economic_baseline_witness",
		       "currency_ledger", "item_ownership_ledger", "critical_outbox" })
			count(connection, table, root, 0);
		evidence(connection, command, *prepared, current.revision + 1, true);
		execute(connection,
			"UPDATE economic_baseline_control SET revision=revision+1,last_operation_id=" +
				id(command.operation_id) + " WHERE " +
				scope(plan.metadata.lineage, plan.metadata.epoch) +
				" AND revision=" + std::to_string(current.revision) +
				" AND last_operation_id <=> " + current.last);
		require(mysql_affected_rows(connection) == 1);
		execute(connection,
			"UPDATE critical_operation_inbox SET status=1,result_code=0,failure_stage=0,durable_revision=" +
				std::to_string(current.revision + 1) +
				",result_payload=X'',committed_at=CURRENT_TIMESTAMP(6) WHERE " +
				predicate(receipt_identity(command)) + " AND status=0");
		require(mysql_affected_rows(connection) == 1);
		const auto revision = verify(connection, command);
		require(revision == current.revision + 1);
		active(connection, owner.session);
		committing = true;
		execute(connection, "COMMIT");
		owner.started = false;
		return { critical_apply_outcome::applied, revision, 0 };
	}
	catch (const failure &e)
	{
		return { committing ? critical_apply_outcome::ambiguous_commit :
				      critical_apply_outcome::retryable_failure,
			 0, e.code == ENOENT ? unsigned(EAGAIN) : e.code };
	}
	catch (const std::bad_alloc &)
	{
		return { committing ? critical_apply_outcome::ambiguous_commit :
				      critical_apply_outcome::retryable_failure,
			 0, ENOMEM };
	}
}
#endif
critical_apply_result
economic_sql_baseline_transaction::apply(MYSQL *connection, const critical_command &command,
					 const economic_prepared_baseline &prepared)
{
	return run(connection, command, &prepared);
}
critical_apply_result economic_sql_baseline_transaction::reconcile(MYSQL *connection,
								   const critical_command &command)
{
	return run(connection, command, nullptr);
}
