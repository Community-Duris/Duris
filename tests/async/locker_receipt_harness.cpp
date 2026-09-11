// The driver injects the production service at SERVICE_BODY below. Currency
// scheduling and networking are controlled; receipts and payment repositories
// are real. Separate process invocations discard every in-memory callback.
#include "item/locker_receipt.h"
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_player_domain_repository.h"
#else
#include "persistence/critical_command_repository.h"
#include <mysql.h>
extern "C" MYSQL *sql_pool_acquire()
{
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" MYSQL *sql_pool_replace_connection(MYSQL *)
{
	return nullptr;
}
#endif
#include <cassert>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

constexpr int CON_PLAYING = 0, ITEM_NOIDENTIFY = 1;
struct descriptor
{
	int connected = 0;
};
struct character
{
	uint32_t pid = 42;
	int racewar = 1, level = 50;
	bool npc = false, fighting = false;
	descriptor *desc = nullptr;
	std::string account = "receipt_account";
};
using P_char = character *;
struct object
{
	P_char owner;
	unsigned extra_flags = 0;
	std::string text;
};
using P_obj = object *;
#define IS_NPC(ch) ((ch)->npc)
#define IS_FIGHTING(ch) ((ch)->fighting)
#define GET_PID(ch) ((ch)->pid)
#define GET_RACEWAR(ch) ((ch)->racewar)
#define GET_LEVEL(ch) ((ch)->level)
#define OBJ_CARRIED_BY(obj, ch) ((obj)->owner == (ch))
#define IS_SET(value, flag) ((value) & (flag))
using completion_fn = void (*)(P_char, bool, const currency_command_result &, unsigned,
			       const uint8_t *, size_t);
P_char live = nullptr;
std::string output, root, mode;
bool bank_purchase = false, admit = true;
unsigned submissions = 0;
completion_fn completion = nullptr;
std::vector<uint8_t> completion_context;
critical_apply_result applied;
std::atomic<bool> hold_writes = false, write_entered = false, fail_writes = false;
bool test_receipt_write(const std::string &directory, const locker_receipt &receipt)
{
	write_entered = true;
	while (hold_writes)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return !fail_writes && locker_receipt_write(directory, receipt);
}
P_char find_player_by_pid(uint32_t pid)
{
	return live && live->pid == pid ? live : nullptr;
}
const char *get_account_name_safe(P_char ch)
{
	return ch->account.c_str();
}
void send_to_char(const char *text, P_char)
{
	output += text;
}
std::string item_lore_description(P_char, P_obj obj)
{
	return obj->text;
}

#ifndef __NO_MYSQL__
MYSQL *db;
void sql(const std::string &query)
{
	if (mysql_query(db, query.c_str()))
	{
		std::cerr << mysql_error(db) << '\n';
		abort();
	}
}
int64_t scalar(const std::string &query)
{
	sql(query);
	MYSQL_RES *result = mysql_store_result(db);
	assert(result);
	MYSQL_ROW row = mysql_fetch_row(result);
	assert(row && row[0]);
	int64_t value = strtoll(row[0], nullptr, 10);
	mysql_free_result(result);
	return value;
}
#endif
currency_command_result balances(P_char ch)
{
	currency_command_result result{};
#ifdef __NO_MYSQL__
	flatfile_player_domain_record record;
	std::string error;
	assert(flatfile_player_domain_load(root + "/authority", ch->pid, ch->account, 1, &record,
					   &error) == flatfile_player_domain_result::ok);
	for (size_t n = 0; n < 4; ++n)
	{
		result.wallet.amount[n] = record.domains.wallet[n];
		result.bank.amount[n] = record.domains.bank[n];
	}
	result.wallet_revision = record.domains.wallet_revision;
	result.bank_revision = record.domains.bank_revision;
#else
	const std::string pid = std::to_string(ch->pid);
	result.wallet.amount[0] = scalar("SELECT copper FROM player_data WHERE pid=" + pid);
	result.bank.amount[0] =
		scalar("SELECT bank_copper FROM account_banks WHERE account_name='" + ch->account +
		       "' AND racewar=1");
	result.wallet_revision = scalar("SELECT wallet_revision FROM player_data WHERE pid=" + pid);
	result.bank_revision =
		scalar("SELECT bank_revision FROM account_banks WHERE account_name='" +
		       ch->account + "' AND racewar=1");
#endif
	return result;
}
void baseline(P_char ch)
{
	std::filesystem::create_directories(root);
	chmod(root.c_str(), 0700);
#ifdef __NO_MYSQL__
	if (!std::filesystem::exists(root + "/authority"))
	{
		std::filesystem::create_directory(root + "/authority");
		chmod((root + "/authority").c_str(), 0700);
		std::filesystem::create_directory(root + "/authority/domains");
		chmod((root + "/authority/domains").c_str(), 0700);
		flatfile_player_domain_record record;
		record.pid = ch->pid;
		record.account_name = ch->account;
		record.racewar = 1;
		record.domains.wallet = { 1000, 0, 0, 0 };
		record.domains.bank = { 1000, 0, 0, 0 };
		std::string error;
		assert(flatfile_player_domain_establish(root + "/authority", record, &error) ==
		       flatfile_player_domain_result::ok);
	}
#else
	assert(getenv("LOCKER_TEST_DATABASE") && getenv("TEST_DB_HOST"));
	db = mysql_init(nullptr);
	assert(db);
	assert(mysql_real_connect(db, getenv("TEST_DB_HOST"), getenv("TEST_DB_USER"),
				  getenv("TEST_DB_PASSWORD"), getenv("LOCKER_TEST_DATABASE"), 3306,
				  nullptr, 0));
	// The runner creates a dedicated disposable schema; each scenario gets its own account.
	ch->account = "receipt_" + std::to_string(std::hash<std::string>{}(root));
	sql("INSERT IGNORE INTO accounts(account_name,password) VALUES('" + ch->account + "','')");
	if (!scalar("SELECT COUNT(*) FROM player_data WHERE account_name='" + ch->account + "'"))
	{
		sql("INSERT INTO player_data(name,account_name,racewar,copper) VALUES('" +
		    ch->account + "','" + ch->account + "',1,1000)");
		sql("INSERT INTO account_banks(account_name,racewar,bank_copper) VALUES('" +
		    ch->account + "',1,1000)");
	}
	ch->pid = scalar("SELECT pid FROM player_data WHERE account_name='" + ch->account + "'");
#endif
}
bool currency_transaction_prepare_identify(P_char ch, int64_t cost, critical_command *command)
{
	if (cost <= 0)
		return false;
	auto state = balances(ch);
	currency_command_payload payload{};
	payload.pid = ch->pid;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), ch->account.c_str());
	payload.reason = bank_purchase ? currency_reason_type::bank_payment :
					 currency_reason_type::wallet_spend;
	(bank_purchase ? payload.bank_delta : payload.wallet_delta).amount[0] = -cost;
	critical_operation_id id;
	assert(critical_operation_id_generate(&id));
	assert(currency_command_build(command, id, payload, state.wallet_revision,
				      state.bank_revision, critical_source_site::command,
				      critical_deadline_class::interactive));
	command->accepted_at_usec = 123456789;
	return critical_command_normalize(command);
}
bool currency_transaction_submit_prepared(P_char ch, const critical_command &command,
					  completion_fn callback, const void *context, size_t size)
{
	if (!admit)
		return false;
	// No payment may be submitted until the identical prepared receipt is durable.
	locker_receipt saved;
	assert(locker_receipt_read(root + "/locker-identification", ch->pid, &saved) ==
	       flatfile_read_result::ok);
	std::vector<uint8_t> first, second;
	assert(critical_command_encode(command, &first) == critical_command_codec_result::ok);
	assert(critical_command_encode(saved.payment, &second) ==
		       critical_command_codec_result::ok &&
	       first == second);
	assert(saved.state == locker_receipt_state::prepared);
	++submissions;
	if (mode == "before-payment")
		_exit(77);
#ifdef __NO_MYSQL__
	applied = flatfile_player_domain_apply(root + "/authority", command);
#else
	applied = critical_command_repository_apply(db, command);
#endif
	if (mode == "after-payment")
		_exit(78);
	completion = callback;
	completion_context.assign(static_cast<const uint8_t *>(context),
				  static_cast<const uint8_t *>(context) + size);
	return true;
}

// SERVICE_BODY

void tick()
{
	locker_identify_pulse();
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
template <class Predicate> void until(Predicate predicate)
{
	for (int n = 0; n < 5000 && !predicate(); ++n)
		tick();
	assert(predicate());
}
void finish(unsigned override_error = 0)
{
	assert(completion);
	currency_command_result result{};
	bool committed = applied.outcome == critical_apply_outcome::applied ||
			 applied.outcome == critical_apply_outcome::already_applied;
	if (committed)
		assert(currency_command_decode_result(applied.result_payload.data(),
						      applied.result_size, &result));
	completion(live, committed && !override_error, result,
		   override_error ? override_error : applied.error_code, completion_context.data(),
		   completion_context.size());
}
void drained()
{
	until([] { return requests.empty(); });
}

int main(int argc, char **argv)
{
	assert(argc == 4);
	root = argv[1];
	mode = argv[2];
	bank_purchase = std::string(argv[3]) == "bank";
	descriptor descriptor;
	character ch;
	ch.desc = &descriptor;
	live = &ch;
	baseline(&ch);
	assert(locker_identify_init(root.c_str()));
	object obj{ &ch, 0, "ORIGINAL SWORD STATS" };
	if (mode == "replay")
	{
		locker_identify_replay(&ch);
		drained();
		assert(submissions == 0);
		assert(output.find("ORIGINAL SWORD STATS") != std::string::npos);
	}
	else
	{
		if (mode == "recover")
			locker_identify_replay(&ch);
		else
			locker_identify(&ch, &obj, 175);
		until([] { return submissions == 1; });
		assert(output.empty());
		locker_identify(&ch, &obj, 175);
		assert(submissions == 1);
		output.clear();
		obj.text = "DIFFERENT STATS";
		obj.owner = nullptr;
		finish();
		if (mode == "after-receipt")
		{
			requests.at(ch.pid)->io.wait();
			_exit(79);
		}
		drained();
		assert(output.find("ORIGINAL SWORD STATS") != std::string::npos);
		assert(output.find("DIFFERENT STATS") == std::string::npos);
		output.clear();
		finish();
		tick();
		assert(output.empty());
	}
	auto state = balances(&ch);
	assert(state.wallet.amount[0] == (bank_purchase ? 1000 : 825));
	assert(state.bank.amount[0] == (bank_purchase ? 825 : 1000));
	locker_receipt saved;
	assert(locker_receipt_read(receipt_directory, ch.pid, &saved) == flatfile_read_result::ok);
	assert(saved.state == locker_receipt_state::paid);
	if (mode == "normal")
	{
		// Unknown payment results retain prepared evidence, never publish lore.
		output.clear();
		obj.owner = &ch;
		obj.text = "SECOND";
		locker_identify(&ch, &obj, 100);
		until([] { return submissions == 2; });
		finish(EIO);
		assert(output.find("SECOND") == std::string::npos);
		live = nullptr;
		drained();
		live = &ch;
		output.clear();
		locker_identify_replay(&ch);
		until([] { return submissions == 3; });
		assert(applied.outcome == critical_apply_outcome::already_applied);
		finish();
		drained();
		assert(output.find("SECOND") != std::string::npos);
		// A receipt belonging to another account cannot be delivered or replaced.
		output.clear();
		auto account = ch.account;
		ch.account = "someone_else";
		locker_identify_replay(&ch);
		drained();
		assert(output.find("SECOND") == std::string::npos);
		ch.account = account;
		// Admission waits preserve intent and prevent a replacement purchase.
		output.clear();
		admit = false;
		locker_identify(&ch, &obj, 1);
		until([&] { return requests.at(ch.pid)->stage == phase::submitting; });
		assert(submissions == 3);
		live = nullptr;
		drained();
		live = &ch;
		admit = true;
		output.clear();
		obj.text = "UNPURCHASED REPLACEMENT";
		locker_identify(&ch, &obj, 2);
		until([] { return submissions == 4; });
		finish();
		drained();
		assert(output.find("UNPURCHASED REPLACEMENT") == std::string::npos);
		// Bounds, checksum, PID, file permissions, symlink and FIFO refusals.
		std::vector<uint8_t> bytes;
		assert(locker_receipt_encode(saved, &bytes));
		locker_receipt decoded;
		assert(!locker_receipt_decode(bytes, ch.pid + 1, &decoded));
		bytes.back() ^= 1;
		assert(!locker_receipt_decode(bytes, ch.pid, &decoded));
		saved.text = std::string(65537, 'x');
		assert(!locker_receipt_write(receipt_directory, saved));
		const auto path = receipt_directory + "/" + std::to_string(ch.pid) + ".receipt";
		chmod(path.c_str(), 0644);
		assert(locker_receipt_read(receipt_directory, ch.pid, &decoded) ==
		       flatfile_read_result::invalid);
		std::filesystem::remove(path);
		assert(symlink("/etc/passwd", path.c_str()) == 0);
		assert(locker_receipt_read(receipt_directory, ch.pid, &decoded) ==
		       flatfile_read_result::invalid);
		std::filesystem::remove(path);
		assert(mkfifo(path.c_str(), 0600) == 0);
		assert(locker_receipt_read(receipt_directory, ch.pid, &decoded) ==
		       flatfile_read_result::invalid);
		std::filesystem::remove(path);
		// Definitive insufficient-funds failure is durable and never grants text.
		output.clear();
		obj.text = "TOO EXPENSIVE";
		locker_identify(&ch, &obj, 100000);
		until([] { return submissions == 5; });
		assert(applied.error_code == ENOSPC);
		finish();
		drained();
		assert(output.find("TOO EXPENSIVE") == std::string::npos);
		assert(locker_receipt_read(receipt_directory, ch.pid, &decoded) ==
			       flatfile_read_result::ok &&
		       decoded.state == locker_receipt_state::failed);
		output.clear();
		locker_identify_replay(&ch);
		drained();
		assert(submissions == 5);
		ch.fighting = true;
		locker_identify(&ch, &obj, 1);
		ch.fighting = false;
		ch.level = 49;
		obj.extra_flags = ITEM_NOIDENTIFY;
		locker_identify(&ch, &obj, 1);
		obj.extra_flags = 0;
		obj.text = std::string(65537, 'x');
		locker_identify(&ch, &obj, 1);
		assert(requests.empty());
		// A stuck storage worker must not charge or stall the game pulse.
		obj.text = "BLOCKED WRITE";
		write_entered = false;
		hold_writes = true;
		locker_identify(&ch, &obj, 1);
		until([] { return write_entered.load(); });
		const auto start = std::chrono::steady_clock::now();
		for (int n = 0; n < 10000; ++n)
			locker_identify_pulse();
		assert(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(100));
		assert(submissions == 5);
		// A failed prepare write never submits payment.
		fail_writes = true;
		hold_writes = false;
		until([&] { return !requests.at(ch.pid)->io.valid(); });
		assert(submissions == 5);
		live = nullptr;
		drained();
		live = &ch;
		fail_writes = false;
	}
	locker_identify_shutdown();
#ifndef __NO_MYSQL__
	mysql_close(db);
#endif
	std::cout << mode << " " << (bank_purchase ? "bank" : "wallet")
		  << " receipt/payment checks passed\n";
}
