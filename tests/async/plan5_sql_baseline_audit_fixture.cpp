#include "persistence/economic_sql_baseline_transaction.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

class economic_sql_baseline_test_access
{
    public:
	static constexpr auto initialize = &economic_sql_baseline_transaction::initialize;
	static constexpr auto apply = &economic_sql_baseline_transaction::apply;
	static constexpr auto reconcile = &economic_sql_baseline_transaction::reconcile;
};
using owner = economic_sql_baseline_test_access;

static critical_operation_id ident(uint64_t value)
{
	critical_operation_id result = {};
	for (size_t i = 0; i < 8; ++i)
		result.bytes[i] = static_cast<uint8_t>(value >> (8 * i));
	return result;
}

static std::string hex(const critical_operation_id &value)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string result;
	for (auto byte : value.bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	return result;
}

static economic_baseline_batch batch(unsigned index)
{
	economic_baseline_batch result;
	result.lineage = ident(771001);
	result.epoch = ident(771002 + index);
	result.preparation_id = ident(771005 + index);
	result.actor_id = 7;
	result.batch_index = index;
	result.opening_account = { result.lineage, economic_account_kind::opening, 99, 0 };
	result.boundary_digest[0] = 11;
	result.coverage_digest[0] = 12;
	economic_digest source = {};
	source[0] = 13;
	for (unsigned i = 0; i <= index; ++i)
	{
		result.holdings.push_back(
			{ { result.lineage, economic_account_kind::wallet, 7 + i, 0 },
			  { 7 + i, 0, 0, 0 },
			  4 + i,
			  source });
		result.items.push_back({ { 81 + i,
					   { { item_owner_type::player, 7 + i, 0 },
					     81 + i,
					     0,
					     3 + i,
					     item_custody_state::active,
					     static_cast<uint16_t>(5 + i) } },
					 source });
	}
	return result;
}

#ifndef __NO_MYSQL__
static void sql(MYSQL *connection, const std::string &query)
{
	if (mysql_real_query(connection, query.data(), query.size()))
	{
		std::cerr << "private baseline fixture SQL error " << mysql_errno(connection)
			  << '\n';
		std::abort();
	}
}

static void successful(const critical_apply_result &result, bool replay)
{
	assert(!result.error_code && result.durable_revision == 1 && !result.result_size &&
	       result.failure_stage == critical_failure_stage::none);
	assert(result.outcome == (replay ? critical_apply_outcome::already_applied :
					   critical_apply_outcome::applied));
}
#endif

int main(int argc, char **argv)
{
	const bool replay = argc == 2 && !strcmp(argv[1], "--reconcile");
	assert(argc == 1 || replay);
#ifndef __NO_MYSQL__
	assert(getenv("TEST_DB_DISPOSABLE") && !strcmp(getenv("TEST_DB_DISPOSABLE"), "1"));
	assert(getenv("ENVIRONMENT") && !strcmp(getenv("ENVIRONMENT"), "test"));
	assert(getenv("DB_NAME") && !strcmp(getenv("DB_NAME"), "duris_restore"));
	assert(getenv("DB_SOCKET") &&
	       std::string(getenv("DB_SOCKET")).starts_with("/plan5-restore-baseline-"));
	auto *connection = mysql_init(nullptr);
	assert(connection);
	unsigned protocol = MYSQL_PROTOCOL_SOCKET;
	assert(!mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol));
	assert(mysql_real_connect(connection, "localhost", getenv("DB_USER"), getenv("DB_PASSWD"),
				  "duris_restore", 0, getenv("DB_SOCKET"), 0));
	sql(connection, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
	const auto creator = hex(ident(771004));
	if (!replay)
	{
		sql(connection, "START TRANSACTION");
		sql(connection,
		    "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload,committed_at) VALUES(UNHEX('" +
			    creator +
			    "'),REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,X'',CURRENT_TIMESTAMP(6))");
		sql(connection,
		    "INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(UNHEX('" +
			    hex(ident(771001)) + "'),NULL)");
	}
#endif
	std::string operations;
	for (unsigned i = 0; i < 2; ++i)
	{
		const auto witness = batch(i);
		std::optional<economic_prepared_baseline> prepared;
		assert(economic_baseline_prepare(witness, &prepared) ==
		       economic_accounting_error::ok);
		critical_command command;
		assert(economic_baseline_command_build(*prepared, 123456, &command) ==
		       economic_accounting_error::ok);
#ifdef __NO_MYSQL__
		assert(owner::initialize(nullptr, witness.lineage, witness.epoch,
					 witness.opening_account, ident(771004)) == ENOTSUP);
		assert(owner::apply(nullptr, command, *prepared).error_code == ENOTSUP);
		assert(owner::reconcile(nullptr, command).error_code == ENOTSUP);
#else
		if (!replay)
		{
			sql(connection,
			    "INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES(UNHEX('" +
				    hex(witness.lineage) + "'),UNHEX('" + hex(witness.epoch) +
				    "')," + std::to_string(i + 1) +
				    ",1,REPEAT(CHAR(1),32),UNHEX('" + creator + "'))");
			assert(!owner::initialize(connection, witness.lineage, witness.epoch,
						  witness.opening_account, ident(771004)));
			sql(connection, "COMMIT");
			successful(owner::apply(connection, command, *prepared), false);
		}
		successful(owner::apply(connection, command, *prepared), true);
		successful(owner::reconcile(connection, command), true);
		if (!replay && i == 0)
			sql(connection, "START TRANSACTION");
#endif
		operations += (i ? "," : "") + std::string("\"") + hex(command.operation_id) + "\"";
	}
#ifndef __NO_MYSQL__
	mysql_close(connection);
#endif
	std::cout << "{\"lineage\":\"" << hex(ident(771001)) << "\",\"epochs\":[\""
		  << hex(ident(771002)) << "\",\"" << hex(ident(771003)) << "\"],\"operations\":["
		  << operations << "]}\n";
}
