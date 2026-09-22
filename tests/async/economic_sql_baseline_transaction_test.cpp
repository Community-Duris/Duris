#include "persistence/economic_sql_baseline_transaction.h"
#include <algorithm>
#include <atomic>
#include <barrier>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

class economic_sql_baseline_test_access
{
    public:
	static constexpr auto initialize = &economic_sql_baseline_transaction::initialize;
	static constexpr auto apply = &economic_sql_baseline_transaction::apply;
	static constexpr auto reconcile = &economic_sql_baseline_transaction::reconcile;
};
using owner = economic_sql_baseline_test_access;
critical_operation_id ident(uint64_t n)
{
	critical_operation_id value = {};
	for (size_t i = 0; i < 8; ++i)
		value.bytes[i] = static_cast<uint8_t>(n >> (8 * i));
	return value;
}
economic_digest digest(uint8_t n)
{
	economic_digest value = {};
	value[0] = n;
	return value;
}
economic_baseline_batch batch(uint64_t sequence = 0, size_t holdings = 2, size_t items = 2)
{
	economic_baseline_batch b;
	b.lineage = ident(777001);
	b.epoch = ident(777002);
	b.preparation_id = ident(777004);
	b.actor_id = 7;
	b.batch_index = sequence;
	b.opening_account = { b.lineage, economic_account_kind::opening, 9001, 0 };
	b.boundary_digest = digest(11);
	b.coverage_digest = digest(12);
	for (size_t i = 0; i < holdings; ++i)
		b.holdings.push_back({ { b.lineage, economic_account_kind::wallet, i + 1, 0 },
				       { 1, 2, 3, 4 },
				       i,
				       digest(1) });
	for (size_t i = 0; i < items; ++i)
		b.items.push_back({ { 100001 + i,
				      { { item_owner_type::player, 7, 0 },
					100001 + i,
					0,
					i,
					item_custody_state::active } },
				    digest(2) });
	return b;
}
economic_prepared_baseline prepare(const economic_baseline_batch &b)
{
	std::optional<economic_prepared_baseline> value;
	assert(economic_baseline_prepare(b, &value) == economic_accounting_error::ok);
	return std::move(*value);
}
critical_command command(const economic_prepared_baseline &p)
{
	critical_command c;
	assert(economic_baseline_command_build(p, 123456, &c) == economic_accounting_error::ok);
	return c;
}
#ifdef __NO_MYSQL__
int main()
{
	auto p = prepare(batch());
	auto c = command(p);
	assert(owner::initialize(nullptr, p.witness().lineage, p.witness().epoch,
				 p.witness().opening_account, ident(777003)) == ENOTSUP);
	assert(owner::apply(nullptr, c, p).error_code == ENOTSUP);
	assert(owner::reconcile(nullptr, c).error_code == ENOTSUP);
	std::cout << "client-free baseline owner refusal PASS\n";
}
#else
bool instrument = false, after_write = false, faulted = false;
size_t query_seen = 0, query_target = 0, allocation_seen = 0, allocation_target = 0;
std::vector<size_t> writes;
extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" unsigned int __real_mysql_errno(MYSQL *);
extern "C" int __wrap_mysql_real_query(MYSQL *c, const char *sql, unsigned long size)
{
	if (!instrument || (size == 8 && !memcmp(sql, "ROLLBACK", 8)))
		return __real_mysql_real_query(c, sql, size);
	faulted = false;
	++query_seen;
	const bool write = size >= 6 && (!memcmp(sql, "INSERT", 6) || !memcmp(sql, "UPDATE", 6) ||
					 !memcmp(sql, "COMMIT", 6));
	if (write && !query_target && !allocation_target)
		writes.push_back(query_seen);
	const bool fail = query_target && query_seen == query_target;
	if (fail && !(after_write && write))
	{
		faulted = true;
		return 1;
	}
	const auto result = __real_mysql_real_query(c, sql, size);
	if (fail)
	{
		faulted = true;
		return 1;
	}
	return result;
}
extern "C" unsigned int __wrap_mysql_errno(MYSQL *c)
{
	return faulted ? 2013 : __real_mysql_errno(c);
}
extern "C" void *__real__Znwm(size_t);
extern "C" void *__real__Znam(size_t);
extern "C" void *__wrap__Znwm(size_t n)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znwm(n);
}
extern "C" void *__wrap__Znam(size_t n)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znam(n);
}
std::set<std::string> root_ids;
std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char d[] = "0123456789abcdef";
	std::string out = "X'";
	for (auto b : bytes)
	{
		out += d[b >> 4];
		out += d[b & 15];
	}
	return out + "'";
}
std::string id(const critical_operation_id &value)
{
	return hex(value.bytes);
}
const char *env(const char *key)
{
	auto value = getenv(key);
	assert(value && *value);
	return value;
}
MYSQL *connect_fixture()
{
	assert(!strcmp(env("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"));
	assert(!strcmp(env("DB_HOST"), "127.0.0.1"));
	assert(!getenv("DB_SOCKET") || !*getenv("DB_SOCKET"));
	const std::string schema = env("DB_NAME");
	assert(schema.starts_with("economic_schema_test_") &&
	       schema.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
		       std::string::npos);
	auto *c = mysql_init(nullptr);
	assert(c);
	unsigned timeout = 10, protocol = MYSQL_PROTOCOL_TCP;
	assert(!mysql_options(c, MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(c, MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(c, MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(!mysql_options(c, MYSQL_OPT_PROTOCOL, &protocol));
	assert(mysql_real_connect(c, "127.0.0.1", env("DB_USER"), env("DB_PASSWD"), schema.c_str(),
				  static_cast<unsigned>(strtoul(env("DB_PORT"), nullptr, 10)),
				  nullptr, 0));
	constexpr char isolation[] = "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED";
	assert(!mysql_real_query(c, isolation, sizeof(isolation) - 1));
	return c;
}
void sql(MYSQL *c, const std::string &text)
{
	if (mysql_real_query(c, text.data(), text.size()))
	{
		std::cerr << "synthetic SQL fixture failed (" << mysql_errno(c) << ")\n";
		abort();
	}
}
std::string scalar(MYSQL *c, const std::string &text)
{
	sql(c, text);
	auto *res = mysql_store_result(c);
	assert(res && mysql_num_rows(res) == 1 && mysql_num_fields(res) == 1);
	auto row = mysql_fetch_row(res);
	assert(row && row[0]);
	std::string out(row[0], mysql_fetch_lengths(res)[0]);
	mysql_free_result(res);
	return out;
}
std::string native(MYSQL *c)
{
	std::string out;
	for (const char *table : { "player_data", "account_banks", "item_current_owner",
				   "item_ownership_ledger", "currency_ledger" })
	{
		sql(c, std::string("SELECT * FROM ") + table);
		auto *r = mysql_store_result(c);
		assert(r);
		while (auto row = mysql_fetch_row(r))
		{
			auto n = mysql_fetch_lengths(r);
			for (size_t i = 0; i < mysql_num_fields(r); ++i)
			{
				out += row[i] ? std::string(row[i], n[i]) : "<NULL>";
				out += '|';
			}
			out += '\n';
		}
		mysql_free_result(r);
	}
	return out;
}
void track(const critical_command &c)
{
	root_ids.insert(id(c.operation_id));
}
void clean(MYSQL *c)
{
	instrument = false;
	allocation_target = 0;
	faulted = false;
	sql(c, "ROLLBACK");
	const std::string lineage = id(ident(777001));
	sql(c, "SELECT operation_id FROM economic_accounting_operation WHERE lineage=" + lineage);
	auto *retained = mysql_store_result(c);
	assert(retained);
	while (auto row = mysql_fetch_row(retained))
	{
		auto sizes = mysql_fetch_lengths(retained);
		assert(row[0] && sizes[0] == 16);
		root_ids.insert(hex(std::span(reinterpret_cast<const uint8_t *>(row[0]), 16)));
	}
	mysql_free_result(retained);

	for (const char *table : { "economic_baseline_reservation", "economic_baseline_witness",
				   "economic_baseline_control" })
		sql(c, std::string("DELETE FROM ") + table + " WHERE lineage=" + lineage);
	for (const char *table :
	     { "economic_accounting_coin_posting", "economic_accounting_account_effect",
	       "economic_accounting_child", "economic_accounting_item_reference" })
		sql(c,
		    std::string("DELETE child FROM ") + table +
			    " child JOIN economic_accounting_operation parent ON child.operation_id=parent.operation_id WHERE parent.lineage=" +
			    lineage);
	sql(c, "DELETE FROM economic_accounting_source_claim WHERE lineage=" + lineage);
	sql(c, "DELETE FROM economic_accounting_operation WHERE lineage=" + lineage);
	sql(c, "DELETE FROM economic_lineage_state WHERE lineage=" + lineage);
	sql(c, "DELETE FROM economic_epoch WHERE lineage=" + lineage);
	for (const auto &op : root_ids)
		sql(c, "DELETE FROM critical_operation_inbox WHERE operation_id=" + op);
	sql(c, "DELETE FROM critical_operation_inbox WHERE operation_id=" + id(ident(777003)));
}
void epoch(MYSQL *c, const economic_baseline_batch &b, uint64_t ordinal = 1)
{
	sql(c,
	    "INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) VALUES(" +
		    id(b.lineage) + "," + id(b.epoch) + "," + std::to_string(ordinal) +
		    ",1,REPEAT(CHAR(1),32)," + id(ident(777003)) + ")");
}
void setup(MYSQL *c)
{
	clean(c);
	auto b = batch();
	sql(c, "START TRANSACTION");
	sql(c,
	    "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_code,durable_revision,result_payload,committed_at) VALUES(" +
		    id(ident(777003)) +
		    ",REPEAT(CHAR(1),32),REPEAT(CHAR(2),32),1,1,1,1,0,0,X'',CURRENT_TIMESTAMP(6))");
	epoch(c, b);
	sql(c, "INSERT INTO economic_lineage_state(lineage,active_epoch) VALUES(" + id(b.lineage) +
		       ",NULL)");
	assert(!owner::initialize(c, b.lineage, b.epoch, b.opening_account, ident(777003)));
	sql(c, "COMMIT");
}
void successful(critical_apply_result result, uint64_t revision, bool replay = false)
{
	if (result.error_code)
		std::cerr << "baseline result code " << result.error_code << "\n";
	assert(!result.error_code && result.durable_revision == revision && !result.result_size &&
	       result.failure_stage == critical_failure_stage::none);
	assert(result.outcome == (replay ? critical_apply_outcome::already_applied :
					   critical_apply_outcome::applied));
}
void failed(const critical_apply_result &r)
{
	assert(r.outcome == critical_apply_outcome::retryable_failure ||
	       r.outcome == critical_apply_outcome::ambiguous_commit);
	assert(r.error_code && !r.durable_revision && !r.result_size);
}
void absent(MYSQL *c, const critical_command &op)
{
	assert(scalar(c, "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
				 id(op.operation_id)) == "0");
	assert(scalar(c, "SELECT revision FROM economic_baseline_control WHERE lineage=" +
				 id(ident(777001))) == "0");
}
void basic(MYSQL *c)
{
	setup(c);
	auto b = batch();
	auto p = prepare(b);
	auto op = command(p);
	track(op);
	const auto before = native(c);
	assert(owner::initialize(c, b.lineage, b.epoch, b.opening_account, ident(777003)) ==
	       ENOTCONN);
	sql(c, "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ");
	assert(owner::apply(c, op, p).error_code == EPERM);
	absent(c, op);
	sql(c, "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");

	failed(owner::reconcile(c, op));
	absent(c, op);
	sql(c, "START TRANSACTION");
	assert(owner::apply(c, op, p).error_code == EBUSY);
	assert(c->server_status & SERVER_STATUS_IN_TRANS);
	sql(c, "ROLLBACK");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	assert(!mysql_options(c, MYSQL_OPT_RECONNECT, &reconnect));
	assert(owner::apply(c, op, p).error_code == EPERM);
	reconnect = false;
	assert(!mysql_options(c, MYSQL_OPT_RECONNECT, &reconnect));
	successful(owner::apply(c, op, p), 1);
	assert(native(c) == before);
	successful(owner::apply(c, op, p), 1, true);
	successful(owner::reconcile(c, op), 1, true);
	auto changed = op;
	++changed.accepted_at_usec;
	failed(owner::reconcile(c, changed));
	auto other = batch(1);
	other.holdings[0].account.kind = economic_account_kind::pile;
	auto q = prepare(other);
	auto qo = command(q);
	track(qo);
	failed(owner::apply(c, qo, q));
	assert(scalar(c, "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
				 id(qo.operation_id)) == "0");
	other = batch(2, 0, 0);
	q = prepare(other);
	qo = command(q);
	track(qo);
	successful(owner::apply(c, qo, q), 2);
	successful(owner::reconcile(c, op), 1, true);
	// Supplied current preparation cannot overwrite a retained original receipt.
	successful(owner::apply(c, op, q), 1, true);
	auto later = batch(3);
	for (auto &h : later.holdings)
		h.account.authority_id += 100;
	for (auto &i : later.items)
	{
		i.snapshot.uid += 100;
		i.snapshot.position.root_uid += 100;
	}
	q = prepare(later);
	qo = command(q);
	track(qo);
	successful(owner::apply(c, qo, q), 3);
	sql(c,
	    "UPDATE economic_lineage_state SET active_epoch=NULL,revision=revision+1 WHERE lineage=" +
		    id(b.lineage));
	successful(owner::reconcile(c, op), 1, true);
	auto new_epoch = batch(4);
	new_epoch.epoch = ident(777005);
	q = prepare(new_epoch);
	qo = command(q);
	track(qo);
	sql(c, "START TRANSACTION");
	epoch(c, new_epoch, 2);
	assert(!owner::initialize(c, new_epoch.lineage, new_epoch.epoch, new_epoch.opening_account,
				  ident(777003)));
	sql(c, "COMMIT");
	successful(owner::apply(c, qo, q), 1);
	successful(owner::reconcile(c, op), 1, true);
	assert(native(c) == before);
	clean(c);
}
void corruption(MYSQL *c)
{
	auto p = prepare(batch());
	auto op = command(p);
	track(op);
	const auto root = "operation_id=" + id(op.operation_id);
	const auto lineage = "lineage=" + id(ident(777001));
	const std::vector<std::string> damage = {
		"UPDATE economic_baseline_witness SET canonical_witness=INSERT(canonical_witness,121,1,CHAR(99)) WHERE " +
			root,
		"UPDATE economic_baseline_witness SET witness_digest=REPEAT(CHAR(9),32) WHERE " +
			root,
		"UPDATE economic_accounting_account_effect SET after_copper=after_copper+1 WHERE " +
			root + " AND account_index=0",
		"UPDATE economic_accounting_coin_posting SET copper_value=copper_value+1 WHERE " +
			root + " AND line_index=0",
		"DELETE FROM economic_baseline_reservation WHERE " + root +
			" AND identity_kind=1 AND identity_id=1",
		"INSERT INTO economic_baseline_reservation(lineage,epoch,identity_kind,identity_id,operation_id) VALUES(" +
			id(ident(777001)) + "," + id(ident(777002)) + ",1,99999," +
			id(op.operation_id) + ")",
		"UPDATE economic_baseline_control SET opening_account=REPEAT(CHAR(2),40) WHERE " +
			lineage,
		"UPDATE critical_operation_inbox SET durable_revision=durable_revision+1 WHERE " +
			root,
		"UPDATE economic_accounting_operation SET canonical_plan=INSERT(canonical_plan,1,1,CHAR(0)) WHERE " +
			root,
		"DELETE FROM economic_accounting_source_claim WHERE " + root,
		"UPDATE economic_baseline_control SET last_operation_id=" + id(ident(777003)) +
			" WHERE " + lineage
	};
	for (const auto &query : damage)
	{
		setup(c);
		successful(owner::apply(c, op, p), 1);
		sql(c, query);
		failed(owner::reconcile(c, op));
	}
	clean(c);
}
void faults(MYSQL *c)
{
	auto p = prepare(batch(0, 1, 0));
	auto op = command(p);
	track(op);
	setup(c);
	query_seen = 0;
	query_target = 0;
	writes.clear();
	instrument = true;
	successful(owner::apply(c, op, p), 1);
	instrument = false;
	const auto total = query_seen;
	const auto write_points = writes;
	assert(total && !write_points.empty());
	for (size_t target = 1; target <= total; ++target)
	{
		setup(c);
		query_seen = 0;
		query_target = target;
		after_write = false;
		instrument = true;
		auto r = owner::apply(c, op, p);
		instrument = false;
		query_target = 0;
		faulted = false;
		failed(r);
		absent(c, op);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	}
	for (auto target : write_points)
	{
		setup(c);
		query_seen = 0;
		query_target = target;
		after_write = true;
		instrument = true;
		auto r = owner::apply(c, op, p);
		instrument = false;
		query_target = 0;
		faulted = false;
		failed(r);
		auto *fresh = connect_fixture();
		if (r.outcome == critical_apply_outcome::ambiguous_commit)
			successful(owner::reconcile(fresh, op), 1, true);
		else
			absent(fresh, op);
		mysql_close(fresh);
	}
	std::cout << total << " before-query and " << write_points.size()
		  << " hidden-write-ack faults PASS" << std::endl;
	clean(c);
}
void allocations(MYSQL *c)
{
	setup(c);
	auto p = prepare(batch(0, 1, 0));
	auto op = command(p);
	track(op);
	size_t apply_failures = 0, replay_failures = 0;
	for (size_t target = 1; target < 100000; ++target)
	{
		allocation_seen = 0;
		allocation_target = target;
		auto result = owner::apply(c, op, p);
		allocation_target = 0;
		if (result.outcome == critical_apply_outcome::applied)
		{
			successful(result, 1);
			break;
		}
		failed(result);
		if (result.error_code != ENOMEM)
			std::cerr << "allocation target " << target << " error "
				  << result.error_code << std::endl;
		assert(result.error_code == ENOMEM);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
		absent(c, op);
		++apply_failures;
	}
	assert(apply_failures > 0 && apply_failures < 99999);
	for (size_t target = 1; target < 100000; ++target)
	{
		allocation_seen = 0;
		allocation_target = target;
		auto result = owner::reconcile(c, op);
		allocation_target = 0;
		if (result.outcome == critical_apply_outcome::already_applied)
		{
			successful(result, 1, true);
			break;
		}
		failed(result);
		if (result.error_code != ENOMEM)
			std::cerr << "allocation target " << target << " error "
				  << result.error_code << std::endl;
		assert(result.error_code == ENOMEM);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
		++replay_failures;
	}
	assert(replay_failures > 0 && replay_failures < 99999);
	successful(owner::reconcile(c, op), 1, true);
	std::cout << apply_failures << " apply and " << replay_failures
		  << " replay allocation faults PASS" << std::endl;
	clean(c);
}
void concurrency(MYSQL *c)
{
	for (unsigned mode = 0; mode < 3; ++mode)
	{
		setup(c);
		auto p = prepare(batch(0, 1, 0));
		auto left = command(p);
		track(left);
		auto b = batch(mode ? 1 : 0, 1, 0);
		if (mode == 1)
			b.holdings[0].account.authority_id = 99;
		auto q = prepare(b);
		auto right = command(q);
		track(right);
		std::barrier gate(3);
		critical_apply_result a = {}, z = {};
		auto work = [&](const critical_command &op,
				const economic_prepared_baseline &prepared,
				critical_apply_result &out)
		{
			assert(!mysql_thread_init());
			auto *connection = connect_fixture();
			gate.arrive_and_wait();
			out = owner::apply(connection, op, prepared);
			mysql_close(connection);
			mysql_thread_end();
		};
		std::thread first(work, std::cref(left), std::cref(p), std::ref(a));
		std::thread second(work, std::cref(right), std::cref(q), std::ref(z));
		gate.arrive_and_wait();
		first.join();
		second.join();
		if (mode < 2 && (a.error_code || z.error_code))
			std::cerr << "baseline concurrency mode=" << mode
				  << " left(outcome/code/revision)="
				  << static_cast<unsigned>(a.outcome) << "/" << a.error_code << "/"
				  << a.durable_revision
				  << " right=" << static_cast<unsigned>(z.outcome) << "/"
				  << z.error_code << "/" << z.durable_revision << std::endl;
		// InnoDB may choose either first contender as a deadlock victim.
		// Retry only the observed 1213 contract, after both first attempts have
		// joined, on a fresh session and with the exact original command.
		auto retry_deadlock = [&](const critical_command &op,
					  const economic_prepared_baseline &prepared,
					  critical_apply_result &out)
		{
			for (unsigned attempt = 2;
			     attempt <= 3 &&
			     out.outcome == critical_apply_outcome::retryable_failure &&
			     out.error_code == 1213;
			     ++attempt)
			{
				std::cout << "baseline concurrency mode=" << mode
					  << " transient1213 original-ID attempt=" << attempt
					  << std::endl;
				auto *retry = connect_fixture();
				out = owner::apply(retry, op, prepared);
				mysql_close(retry);
			}
		};
		retry_deadlock(left, p, a);
		retry_deadlock(right, q, z);
		if (mode == 0)
		{
			assert(!a.error_code && !z.error_code && a.durable_revision == 1 &&
			       z.durable_revision == 1);
			assert((a.outcome == critical_apply_outcome::applied) !=
			       (z.outcome == critical_apply_outcome::applied));
		}
		else if (mode == 1)
		{
			assert(!a.error_code && !z.error_code &&
			       a.durable_revision + z.durable_revision == 3);
		}
		else
		{
			assert(bool(a.error_code) != bool(z.error_code));
			assert((a.error_code ? a : z).error_code == 1062);
		}
		if (!a.error_code)
			successful(owner::reconcile(c, left), a.durable_revision, true);
		if (!z.error_code)
			successful(owner::reconcile(c, right), z.durable_revision, true);
		clean(c);
	}
	std::cout << "concurrent same-ID, disjoint and overlapping batches PASS" << std::endl;
}
void reconcile_faults(MYSQL *c)
{
	setup(c);
	auto p = prepare(batch(0, 1, 0));
	auto op = command(p);
	track(op);
	successful(owner::apply(c, op, p), 1);
	query_target = 0;
	query_seen = 0;
	instrument = true;
	successful(owner::reconcile(c, op), 1, true);
	instrument = false;
	const auto total = query_seen;
	for (size_t target = 1; target <= total; ++target)
	{
		query_seen = 0;
		query_target = target;
		after_write = false;
		instrument = true;
		auto result = owner::reconcile(c, op);
		instrument = false;
		query_target = 0;
		faulted = false;
		failed(result);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	}
	successful(owner::reconcile(c, op), 1, true);
	std::cout << total << " replay query faults PASS" << std::endl;
	clean(c);
}
void initialization_and_exhaustion(MYSQL *c)
{
	auto p = prepare(batch(0, 1, 0));
	auto op = command(p);
	track(op);
	for (const char *damage :
	     { "status=0", "result_code=5", "failure_stage=1", "committed_at=NULL" })
	{
		setup(c);
		sql(c, std::string("UPDATE critical_operation_inbox SET ") + damage +
			       " WHERE operation_id=" + id(ident(777003)));
		failed(owner::apply(c, op, p));
		absent(c, op);
	}
	setup(c);
	sql(c, "START TRANSACTION");
	const auto &b = p.witness();
	assert(owner::initialize(c, b.lineage, b.epoch, b.opening_account, ident(777003)) == 1062);
	sql(c, "ROLLBACK");
	successful(owner::apply(c, op, p), 1);
	sql(c,
	    "UPDATE critical_operation_inbox SET durable_revision=18446744073709551615 WHERE operation_id=" +
		    id(op.operation_id));
	sql(c,
	    "UPDATE economic_baseline_witness SET book_revision=18446744073709551615 WHERE operation_id=" +
		    id(op.operation_id));
	sql(c, "UPDATE economic_baseline_control SET revision=18446744073709551615 WHERE lineage=" +
		       id(b.lineage));
	auto next = batch(1, 1, 0);
	next.holdings[0].account.authority_id = 99;
	auto q = prepare(next);
	auto qo = command(q);
	track(qo);
	auto result = owner::apply(c, qo, q);
	failed(result);
	assert(result.error_code == ERANGE);
	assert(scalar(c, "SELECT COUNT(*) FROM critical_operation_inbox WHERE operation_id=" +
				 id(qo.operation_id)) == "0");
	clean(c);
}
void maximum(MYSQL *c)
{
	setup(c);
	auto b = batch(0, ECONOMIC_BASELINE_MAX_HOLDINGS, ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES);
	auto p = prepare(b);
	auto op = command(p);
	track(op);
	const auto before = native(c);
	successful(owner::apply(c, op, p), 1);
	successful(owner::reconcile(c, op), 1, true);
	assert(native(c) == before);
	clean(c);
}
int main()
{
	auto *c = connect_fixture();
	if (const auto *value = getenv("DURIS_BASELINE_CONCURRENCY_ROUNDS"))
	{
		char *end = nullptr;
		const auto rounds = strtoul(value, &end, 10);
		assert(end && !*end && rounds >= 1 && rounds <= 100);
		for (unsigned long round = 0; round < rounds; ++round)
		{
			std::cout << "baseline concurrency diagnostic round " << round + 1 << "/"
				  << rounds << std::endl;
			concurrency(c);
		}
		clean(c);
		mysql_close(c);
		return 0;
	}
	basic(c);
	corruption(c);
	faults(c);
	reconcile_faults(c);
	concurrency(c);
	allocations(c);
	initialization_and_exhaustion(c);
	maximum(c);
	clean(c);
	mysql_close(c);
	std::cout << "SQL baseline atomic retention and replay PASS\n";
}
#endif
