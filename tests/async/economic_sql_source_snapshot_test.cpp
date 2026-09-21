#include "persistence/economic_sql_source_snapshot.h"
#include "economy/economic_sql_source_normalize.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <new>
#include <string_view>
#include <type_traits>

#ifdef __NO_MYSQL__
int main()
{
	economic_sql_source_snapshot value, before = value;
	assert(economic_sql_capture_sources(nullptr, {}, &value) == ENOTSUP && value == before);
	economic_sql_normalized_sources normalized;
	normalized.diagnostic_count = 987;
	assert(economic_sql_normalize_sources(value, 512, &normalized) ==
	       economic_accounting_error::corrupt_evidence);
	assert(normalized.diagnostic_count == 987);
	std::cout << "client-free source capture refusal and malformed normalization PASS\n";
}
#else
namespace
{
bool instrument = false, faulted = false, concurrent_write = false, concurrent_ddl = false;
bool disconnect = false, hide_start = false, hide_rollback = false;
size_t query_seen = 0, query_target = 0, allocation_seen = 0, allocation_target = 0;
MYSQL *other = nullptr;
}
extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" unsigned int __real_mysql_errno(MYSQL *);
extern "C" int __wrap_mysql_real_query(MYSQL *c, const char *data, unsigned long length)
{
	faulted = false;
	if (instrument && ++query_seen == query_target)
	{
		faulted = true;
		return 1;
	}
	const std::string_view text(data, length);
	if (c != other && text.starts_with("SELECT COUNT(*)") &&
	    text.find("FROM `account_banks`") != text.npos)
	{
		if (disconnect)
		{
			disconnect = false;
			const auto kill = "KILL CONNECTION " + std::to_string(mysql_thread_id(c));
			assert(!__real_mysql_real_query(other, kill.data(), kill.size()));
		}
		if (concurrent_write)
		{
			concurrent_write = false;
			for (const char *q :
			     { "START TRANSACTION",
			       "UPDATE player_data SET copper=999 WHERE pid=11",
			       "UPDATE account_banks SET bank_copper=999 WHERE id=7",
			       "INSERT INTO player_data(pid,name) VALUES(44,'synthetic_new')",
			       "COMMIT" })
				assert(!__real_mysql_real_query(other, q, std::strlen(q)));
		}
		if (concurrent_ddl)
		{
			concurrent_ddl = false;
			constexpr char q[] = "ALTER TABLE ships ADD COLUMN snapshot_test INT";
			assert(__real_mysql_real_query(other, q, sizeof(q) - 1));
			assert(__real_mysql_errno(other) == 1205);
		}
	}
	const auto result = __real_mysql_real_query(c, data, length);
	if (!result && ((hide_start && text.starts_with("START TRANSACTION")) ||
			(hide_rollback && text == "ROLLBACK")))
	{
		hide_start = hide_rollback = false;
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
	if (instrument && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znwm(n);
}
extern "C" void *__wrap__Znam(size_t n)
{
	if (instrument && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znam(n);
}
const char *env(const char *key)
{
	auto value = std::getenv(key);
	assert(value && *value);
	return value;
}
using connection = std::unique_ptr<MYSQL, decltype(&mysql_close)>;
connection connect_fixture()
{
	assert(!std::strcmp(env("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"));
	assert(!std::strcmp(env("DB_HOST"), "127.0.0.1"));
	assert(!std::getenv("DB_SOCKET") || !*std::getenv("DB_SOCKET"));
	const std::string name = env("DB_NAME");
	assert(name.starts_with("economic_schema_test_") &&
	       name.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
		       std::string::npos);
	connection c(mysql_init(nullptr), mysql_close);
	assert(c);
	unsigned timeout = 10, protocol = MYSQL_PROTOCOL_TCP;
	assert(!mysql_options(c.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(c.get(), MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(c.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(!mysql_options(c.get(), MYSQL_OPT_PROTOCOL, &protocol));
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	assert(!mysql_options(c.get(), MYSQL_OPT_RECONNECT, &reconnect));
	assert(mysql_real_connect(
		c.get(), "127.0.0.1", env("DB_USER"), env("DB_PASSWD"), name.c_str(),
		static_cast<unsigned>(std::strtoul(env("DB_PORT"), nullptr, 10)), nullptr, 0));
	return c;
}
void sql(MYSQL *c, const std::string &text)
{
	if (mysql_real_query(c, text.data(), text.size()))
	{
		std::cerr << "synthetic SQL failed errno=" << mysql_errno(c) << '\n';
		std::abort();
	}
}
std::string scalar(MYSQL *c, const std::string &text)
{
	sql(c, text);
	auto *r = mysql_store_result(c);
	assert(r);
	assert(mysql_num_fields(r) == 1 && mysql_num_rows(r) == 1);
	auto row = mysql_fetch_row(r);
	assert(row && row[0]);
	std::string value(row[0], mysql_fetch_lengths(r)[0]);
	mysql_free_result(r);
	return value;
}
const economic_sql_source_table &table(const economic_sql_source_snapshot &s, const char *name)
{
	auto it = std::find_if(s.tables.begin(), s.tables.end(),
			       [&](const auto &t) { return t.name == name; });
	assert(it != s.tables.end());
	return *it;
}
economic_sql_source_snapshot capture(MYSQL *c)
{
	economic_sql_source_snapshot result;
	auto code = economic_sql_capture_sources(c, {}, &result);
	if (code)
	{
		std::cerr << "capture failed errno=" << code << '\n';
		std::abort();
	}
	assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	return result;
}
std::string money_blob(uint64_t uid, int32_t value = 1, int32_t vnum = 1, int32_t type = ITEM_MONEY)
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = uid;
	item.vnum = vnum;
	item.type = type;
	item.values[0] = value;
	item.values[1] = 2;
	item.values[2] = 3;
	item.values[3] = 4;
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ item }, &blob) ==
	       player_snapshot_codec_result::ok);
	constexpr char hex[] = "0123456789abcdef";
	std::string result = "X'";
	for (auto byte : blob)
	{
		result += hex[byte >> 4];
		result += hex[byte & 15];
	}
	return result + "'";
}
void normalization(MYSQL *c)
{
	using error = economic_accounting_error;
	using issue = economic_sql_normalization_issue;
	using kind = economic_sql_holding_kind;
	using disposition = economic_sql_holding_disposition;
	sql(c,
	    "INSERT INTO auctions(id,seller_pid,status,winning_bidder_pid,cur_price,obj_blob_str) VALUES(6,11,'OPEN',22,100,''),(7,11,'REMOVED',22,100,''),(8,11,'OPEN',0,77,'')");
	const std::string blobs[] = { money_blob(200),
				      money_blob(201),
				      money_blob(202, -1),
				      money_blob(999),
				      "X''",
				      "NULL",
				      money_blob(206, 1, 1, ITEM_CONTAINER),
				      money_blob(207, 1, 2) };
	for (size_t i = 0; i < std::size(blobs); ++i)
	{
		const auto uid = std::to_string(200 + i);
		sql(c,
		    "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,vnum,state,coin_payload) VALUES(" +
			    uid + "," + (i == 0 ? "100,100" : uid + ",NULL") + ",1," +
			    (i == 5 ? "77" : "11") + ",1," + (i == 1 ? "2" : (i == 2 ? "3" : "1")) +
			    "," + blobs[i] + ")");
	}
	sql(c, "UPDATE item_uid_allocator SET next_uid=500 WHERE allocator_id=1");
	const auto input = capture(c);
	assert(!economic_sql_validate_sources(input));
	economic_sql_normalized_sources report;
	assert(economic_sql_normalize_sources(input, 512, &report) == error::ok);
	assert(report.source_digest == input.digest && report.next_uid == 500);
	assert(report.items.size() == 10 && report.owners.size() == 1);
	const auto count = [&](issue code)
	{ return report.issue_counts[static_cast<size_t>(code)]; };
	assert(count(issue::unknown_money) == 3 && count(issue::accounting_overflow) == 1);
	assert(count(issue::negative_holding) == 2 &&
	       count(issue::unavailable_native_revision) == 1);
	assert(count(issue::unresolved_auction) == 1 && count(issue::quarantined_item) == 2);
	assert(count(issue::invalid_coin_payload) == 5 && count(issue::unknown_coin_payload) == 2);
	assert(count(issue::missing_owner_revision) == 1 &&
	       count(issue::uid_outside_allocator) == 0);
	assert(count(issue::open_quarantine) == 1 && count(issue::incomplete_receipt) == 1);
	assert(count(issue::pending_publication) == 1 && count(issue::legacy_item_claim) == 1);
	const auto holding = [&](kind type, uint64_t id) -> const economic_sql_native_holding &
	{
		auto found = std::find_if(report.holdings.begin(), report.holdings.end(),
					  [&](const auto &h)
					  { return h.kind == type && h.native_id == id; });
		assert(found != report.holdings.end());
		return *found;
	};
	assert(std::count_if(report.holdings.begin(), report.holdings.end(),
			     [](const auto &h) { return h.kind == kind::bank; }) == 1);
	assert(holding(kind::wallet, 11).balance && (*holding(kind::wallet, 11).balance)[0] == 7);
	assert(!holding(kind::wallet, 22).balance && !holding(kind::bank, 7).balance);
	assert(holding(kind::bank, 7).native_revision == UINT64_MAX &&
	       !holding(kind::ship, 7).native_revision);
	assert(holding(kind::claim, 22).balance && (*holding(kind::claim, 22).balance)[0] == 100);
	assert(holding(kind::auction, 5).disposition == disposition::history &&
	       !holding(kind::auction, 5).balance);
	assert(holding(kind::auction, 6).disposition == disposition::current &&
	       (*holding(kind::auction, 6).balance)[0] == 100);
	assert(holding(kind::auction, 7).disposition == disposition::unresolved);
	assert(holding(kind::auction, 8).disposition == disposition::not_holding &&
	       !holding(kind::auction, 8).balance);
	assert(holding(kind::pile, 200).disposition == disposition::current);
	assert((holding(kind::pile, 200).balance == economic_coin_vector{ 1, 2, 3, 4 }));
	assert(holding(kind::pile, 201).disposition == disposition::history);
	assert(holding(kind::pile, 202).disposition == disposition::unresolved);
	assert(report.items[2].item.uid == 200 && report.items[2].item.position.root_uid == 100 &&
	       report.items[2].item.position.parent_uid == 100 &&
	       report.items[2].owner_revision == UINT64_MAX);
	for (const auto &h : report.holdings)
		assert(h.source.digest == input.tables[h.source.table].rows[h.source.row].digest);
	economic_sql_normalized_sources limited;
	assert(economic_sql_normalize_sources(input, 1, &limited) == error::ok);
	assert(limited.issue_counts == report.issue_counts &&
	       limited.diagnostic_count == report.diagnostic_count);
	assert(limited.diagnostics.size() == 1 && limited.diagnostics_truncated);
	assert(capture(c) == input);
	for (int change = 0; change < 8; ++change)
	{
		auto broken = input;
		switch (change)
		{
		case 0:
			broken.version = 2;
			break;
		case 1:
			broken.tables.pop_back();
			break;
		case 2:
			broken.tables[0].columns[0] = "different";
			break;
		case 3:
			broken.digest[0] ^= 1;
			break;
		case 4:
			broken.tables[0].rows[0].cells[3] = "8";
			break;
		case 5:
			broken.tables[0].rows[0].digest[0] ^= 1;
			break;
		case 6:
			broken.tables[0].content_digest[0] ^= 1;
			break;
		case 7:
			++broken.cell_bytes;
			break;
		}
		economic_sql_normalized_sources unchanged;
		unchanged.diagnostic_count = 987;
		assert(economic_sql_normalize_sources(broken, 512, &unchanged) ==
		       error::corrupt_evidence);
		assert(unchanged.diagnostic_count == 987 && unchanged.holdings.empty());
	}
	sql(c, "DELETE FROM item_uid_allocator");
	assert(economic_sql_normalize_sources(capture(c), 512, &limited) == error::ok);
	assert(!limited.next_uid &&
	       limited.issue_counts[static_cast<size_t>(issue::allocator_missing_or_invalid)] == 1);
	assert(std::any_of(limited.diagnostics.begin(), limited.diagnostics.end(),
			   [](const auto &d) {
				   return d.issue == issue::allocator_missing_or_invalid &&
					  d.source.row == SIZE_MAX;
			   }));
	sql(c, "INSERT INTO item_uid_allocator(allocator_id,next_uid) VALUES(1,201)");
	assert(economic_sql_normalize_sources(capture(c), 512, &limited) == error::ok);
	assert(limited.issue_counts[static_cast<size_t>(issue::uid_outside_allocator)] == 7);
	sql(c,
	    "UPDATE critical_operation_inbox SET status=1,result_code=1,committed_at=CURRENT_TIMESTAMP");
	sql(c, "UPDATE critical_outbox SET status=1,delivered_at=CURRENT_TIMESTAMP");
	sql(c, "UPDATE auction_item_pickups SET retrieved=1");
	sql(c, "UPDATE item_ownership_quarantine SET repaired_at=CURRENT_TIMESTAMP");
	assert(economic_sql_normalize_sources(capture(c), 512, &limited) == error::ok);
	for (auto code : { issue::incomplete_receipt, issue::pending_publication,
			   issue::legacy_item_claim, issue::open_quarantine })
		assert(limited.issue_counts[static_cast<size_t>(code)] == 0);
	// A representable vector can still overflow the accounting copper value.
	sql(c,
	    "UPDATE account_banks SET bank_copper=0,bank_silver=0,bank_gold=0,bank_platinum=9223372036854775807 WHERE id=7");
	const auto overflow_source = capture(c);
	assert(economic_sql_normalize_sources(overflow_source, 512, &limited) == error::ok);
	assert(limited.issue_counts[static_cast<size_t>(issue::accounting_overflow)] == 1);
	auto bank = std::find_if(limited.holdings.begin(), limited.holdings.end(),
				 [](const auto &h) { return h.kind == kind::bank; });
	assert(bank != limited.holdings.end() && bank->balance && (*bank->balance)[3] == INT64_MAX);
	for (int field = 0; field < 4; ++field)
	{
		economic_sql_source_limits limit;
		if (field == 0)
			limit.maximum_rows = 1;
		if (field == 1)
			limit.maximum_cells = 1;
		if (field == 2)
			limit.maximum_cell_bytes = 1;
		if (field == 3)
			limit.maximum_single_cell_bytes = 1;
		assert(economic_sql_validate_sources(input, limit) == E2BIG);
	}
	instrument = true;
	query_seen = allocation_seen = 0;
	assert(economic_sql_normalize_sources(input, 512, &limited) == error::ok);
	instrument = false;
	const auto allocations = allocation_seen;
	assert(query_seen == 0 && allocations > 100);
	for (size_t target = 1; target <= allocations; ++target)
	{
		economic_sql_normalized_sources unchanged;
		unchanged.diagnostic_count = 987;
		allocation_seen = 0;
		allocation_target = target;
		instrument = true;
		const auto result = economic_sql_normalize_sources(input, 512, &unchanged);
		instrument = false;
		allocation_target = 0;
		assert(result == error::capacity && unchanged.diagnostic_count == 987 &&
		       unchanged.holdings.empty());
	}
	std::cout
		<< "native SQL normalization PASS: " << allocations
		<< " allocation faults; native money, custody, diagnostic bounds and hash integrity\n";
}
int main()
{
	auto c = connect_fixture(), writer = connect_fixture();
	other = writer.get();
	sql(other, "SET SESSION lock_wait_timeout=1");
	const auto empty = capture(c.get());
	economic_sql_normalized_sources empty_report;
	assert(!economic_sql_validate_sources(empty));
	assert(economic_sql_normalize_sources(empty, 512, &empty_report) ==
	       economic_accounting_error::ok);
	assert(empty_report.holdings.empty() && empty_report.items.empty() &&
	       empty_report.owners.empty());
	assert(empty_report.next_uid == 1 && empty_report.diagnostic_count == 0 &&
	       !empty_report.diagnostics_truncated);
	for (size_t limit : { size_t{ 0 }, size_t{ 513 } })
	{
		empty_report.diagnostic_count = 987;
		assert(economic_sql_normalize_sources(empty, limit, &empty_report) ==
		       economic_accounting_error::corrupt_evidence);
		assert(empty_report.diagnostic_count == 987);
	}
	assert(economic_sql_normalize_sources(empty, 512, nullptr) ==
	       economic_accounting_error::corrupt_evidence);
	assert(empty.tables.size() == 18);
	assert(table(empty, "player_data").rows.empty());
	sql(c.get(), "INSERT INTO accounts(account_name) VALUES('synthetic_shared')");
	sql(c.get(),
	    "INSERT INTO player_data(pid,name,account_name,copper,silver,wallet_revision) VALUES(22,'synthetic_two','synthetic_shared',NULL,-1,18446744073709551615),(11,'synthetic_one','synthetic_shared',7,0,0)");
	sql(c.get(),
	    "INSERT INTO account_banks(id,account_name,bank_copper,bank_gold,bank_revision) VALUES(7,'synthetic_shared',18446744073709551615,NULL,18446744073709551615)");
	sql(c.get(), "INSERT INTO ships(id,owner_name,money) VALUES(7,'synthetic_owner',NULL)");
	sql(c.get(),
	    "INSERT INTO auctions(id,seller_pid,status,winning_bidder_pid,cur_price,obj_blob_str) VALUES(5,11,'CLOSED',22,100,X'000102ff')");
	sql(c.get(),
	    "INSERT INTO auction_money_pickups(pid,money,claim_revision) VALUES(22,100,18446744073709551615)");
	sql(c.get(), "INSERT INTO auction_item_pickups(pid,obj_blob_str) VALUES(22,'')");
	sql(c.get(),
	    "INSERT INTO item_current_owner(item_uid,root_item_uid,owner_type,owner_id,item_revision,state,coin_payload) VALUES(100,100,1,11,18446744073709551615,3,X'00ff00')");
	sql(c.get(),
	    "INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id) VALUES(101,100,100,1,11)");
	sql(c.get(),
	    "INSERT INTO item_owner_revision(owner_type,owner_id,revision) VALUES(1,11,18446744073709551615)");
	sql(c.get(),
	    "INSERT INTO item_ownership_quarantine(item_uid,source_table,source_row_id,conflict_code,evidence) VALUES(100,'synthetic',1,1,'unknown legacy')");
	sql(c.get(),
	    "INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload) VALUES(REPEAT('a',16),REPEAT('b',32),REPEAT('c',32),1,2,1,0,X'0001ff')");
	sql(c.get(),
	    "INSERT INTO critical_outbox(operation_id,event_index,destination,event_type,payload_version,payload) VALUES(REPEAT('a',16),0,1,1,1,X'ff0000')");
	auto baseline = capture(c.get());
	assert(baseline == capture(c.get())); // Native rows remain unchanged.
	const auto &wallet = table(baseline, "player_data");
	assert(wallet.rows.size() == 2 && wallet.rows[0].cells[0] == "11" &&
	       wallet.rows[1].cells[0] == "22");
	assert(!wallet.rows[1].cells[3] && wallet.rows[1].cells[4] == "-1");
	assert(wallet.rows[1].cells[7] == "18446744073709551615");
	assert(table(baseline, "account_banks").rows.size() == 1);
	assert(table(baseline, "account_banks").rows[0].cells[3] == "18446744073709551615");
	assert(table(baseline, "auction_item_pickups").rows[0].cells[2] == "");
	assert(table(baseline, "item_current_owner").rows[0].cells[9] ==
	       std::string("\0\xff\0", 3));
	assert(table(baseline, "auctions").rows[0].cells[2] == "CLOSED");
	assert(table(baseline, "critical_operation_inbox").rows[0].cells[11] == "0");
	assert(baseline.digest != empty.digest);
	// NULL and empty locator bytes produce distinct source/row evidence.
	sql(c.get(), "UPDATE player_data SET account_name=NULL WHERE pid=22");
	auto null_locator = capture(c.get());
	sql(c.get(), "UPDATE player_data SET account_name='' WHERE pid=22");
	auto changed = capture(c.get());
	assert(changed.digest != baseline.digest && changed.digest != null_locator.digest);
	assert(table(changed, "player_data").rows[0].digest == wallet.rows[0].digest);
	sql(c.get(), "UPDATE player_data SET account_name='synthetic_shared' WHERE pid=22");
	assert(capture(c.get()) == baseline);
	// The capture changes only its own transaction isolation, not session policy.
	auto isolation = scalar(c.get(), "SELECT VERSION() LIKE '%MariaDB%'") == "1" ?
				 "@@SESSION.tx_isolation" :
				 "@@SESSION.transaction_isolation";
	sql(c.get(), "SET SESSION TRANSACTION ISOLATION LEVEL READ COMMITTED");
	assert(capture(c.get()) == baseline);
	assert(scalar(c.get(), std::string("SELECT ") + isolation) == "READ-COMMITTED");
	concurrent_write = true;
	assert(capture(c.get()) == baseline && !concurrent_write);
	changed = capture(c.get());
	assert(changed.digest != baseline.digest && table(changed, "player_data").rows.size() == 3);
	sql(c.get(), "UPDATE player_data SET copper=7 WHERE pid=11");
	sql(c.get(), "UPDATE account_banks SET bank_copper=18446744073709551615 WHERE id=7");
	sql(c.get(), "DELETE FROM player_data WHERE pid=44");
	assert(capture(c.get()) == baseline);
	concurrent_ddl = true;
	assert(capture(c.get()) == baseline && !concurrent_ddl);
	sql(other, "ALTER TABLE ships ADD COLUMN snapshot_test INT");
	sql(other, "ALTER TABLE ships DROP COLUMN snapshot_test");
	for (int kind = 0; kind < 4; ++kind)
	{
		auto limits = economic_sql_source_limits{};
		if (kind == 0)
			limits.maximum_rows = 1;
		if (kind == 1)
			limits.maximum_cells = 1;
		if (kind == 2)
			limits.maximum_cell_bytes = 1;
		if (kind == 3)
			limits.maximum_single_cell_bytes = 1;
		auto output = baseline;
		assert(economic_sql_capture_sources(c.get(), limits, &output) == E2BIG &&
		       output == baseline);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	}
	auto output = baseline;
	sql(c.get(),
	    "UPDATE item_current_owner SET coin_payload=REPEAT('x',1048577) WHERE item_uid=100");
	assert(economic_sql_capture_sources(c.get(), {}, &output) == E2BIG && output == baseline);
	sql(c.get(), "UPDATE item_current_owner SET coin_payload=X'00ff00' WHERE item_uid=100");
	for (auto flag : { &hide_start, &hide_rollback })
	{
		*flag = true;
		assert(economic_sql_capture_sources(c.get(), {}, &output) != 0 &&
		       output == baseline && !*flag);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	}
	disconnect = true;
	assert(economic_sql_capture_sources(c.get(), {}, &output) != 0 && output == baseline &&
	       !disconnect);
	c = connect_fixture();
	assert(capture(c.get()) == baseline);
	sql(c.get(), "START TRANSACTION");
	assert(economic_sql_capture_sources(c.get(), {}, &output) == EBUSY && output == baseline);
	assert(c->server_status & SERVER_STATUS_IN_TRANS);
	sql(c.get(), "ROLLBACK");
	sql(c.get(), "SET autocommit=0");
	assert(economic_sql_capture_sources(c.get(), {}, &output) == EBUSY);
	sql(c.get(), "SET autocommit=1");
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = true;
	assert(!mysql_options(c.get(), MYSQL_OPT_RECONNECT, &reconnect));
	assert(economic_sql_capture_sources(c.get(), {}, &output) == EPERM);
	reconnect = false;
	assert(!mysql_options(c.get(), MYSQL_OPT_RECONNECT, &reconnect));
	auto limits = economic_sql_source_limits{};
	limits.maximum_rows = 0;
	assert(economic_sql_capture_sources(c.get(), limits, &output) == EINVAL);
	assert(economic_sql_capture_sources(nullptr, {}, &output) == EINVAL);
	sql(c.get(), "RENAME TABLE ships TO source_snapshot_ships");
	assert(economic_sql_capture_sources(c.get(), {}, &output) != 0 && output == baseline);
	sql(c.get(), "CREATE TABLE ships LIKE source_snapshot_ships");
	sql(c.get(), "ALTER TABLE ships ENGINE=MyISAM");
	assert(economic_sql_capture_sources(c.get(), {}, &output) == ENOTSUP && output == baseline);
	sql(c.get(), "DROP TABLE ships");
	sql(c.get(), "CREATE VIEW ships AS SELECT * FROM source_snapshot_ships");
	assert(economic_sql_capture_sources(c.get(), {}, &output) == ENOTSUP && output == baseline);
	sql(c.get(), "DROP VIEW ships");
	sql(c.get(), "RENAME TABLE source_snapshot_ships TO ships");
	instrument = true;
	query_seen = allocation_seen = 0;
	assert(economic_sql_capture_sources(c.get(), {}, &output) == 0);
	instrument = false;
	const auto queries = query_seen, allocations = allocation_seen;
	assert(queries > 50 && allocations > 100);
	for (size_t i = 1; i <= queries; ++i)
	{
		output = baseline;
		query_seen = 0;
		query_target = i;
		instrument = true;
		const auto code = economic_sql_capture_sources(c.get(), {}, &output);
		instrument = false;
		query_target = 0;
		assert(code && output == baseline);
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	}
	size_t failures = 0;
	for (size_t i = 1; i <= allocations; i += std::max(size_t{ 1 }, allocations / 100))
	{
		output = baseline;
		allocation_seen = 0;
		allocation_target = i;
		instrument = true;
		const auto code = economic_sql_capture_sources(c.get(), {}, &output);
		instrument = false;
		allocation_target = 0;
		assert(code == ENOMEM && output == baseline);
		++failures;
		assert(!(c->server_status & SERVER_STATUS_IN_TRANS));
	}
	assert(capture(c.get()) == baseline);
	normalization(c.get());
	std::cout
		<< "native SQL sources PASS: " << queries << " query faults, " << failures
		<< " distributed allocation faults, snapshot/DDL/session/capacity/disconnect/lost-ACK fixtures\n";
}
#endif
