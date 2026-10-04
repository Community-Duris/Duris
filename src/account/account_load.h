#ifndef DURIS_ACCOUNT_LOAD_H
#define DURIS_ACCOUNT_LOAD_H

#include <mysql.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// All worker inputs/results own their storage. No live account or descriptor crosses here.
constexpr size_t ACCOUNT_LOAD_MAX_PENDING = 64;
constexpr size_t ACCOUNT_LOAD_MAX_IPS = 4096;
constexpr size_t ACCOUNT_LOAD_MAX_CHARACTERS = 1024;
constexpr size_t ACCOUNT_LOAD_MAX_BYTES = 1024 * 1024;
constexpr uint64_t ACCOUNT_LOAD_TIMEOUT_USEC = 30 * 1000 * 1000;

struct account_load_ip
{
	std::string hostname, address;
	unsigned long count = 0;
};

struct account_load_character
{
	int pid = 0;
	std::string name;
	unsigned long count = 0;
	long last = 0;
	char blocked = 0, racewar = 0;
	int level = 0, race = 0;
	unsigned int primary_class = 0, secondary_class = 0;
	int last_room = 0;
	long last_save = 0;
};

struct account_load_snapshot
{
	std::string name, email, password, confirmation;
	char blocked = 0, confirmed = 0, confirmation_sent = 0;
	long last = 0, good = 0, evil = 0;
	unsigned long flags[4] = {};
	std::vector<account_load_ip> ips;
	std::vector<account_load_character> characters;
	uint64_t telemetry_account_token = 0, telemetry_environment_id = 0, telemetry_season_id = 0;
};

enum class account_load_outcome
{
	loaded,
	not_found,
	unavailable,
	repair_failed,
	load_failed,
	limit_exceeded,
	timed_out,
	cancelled
};

struct account_load_request
{
	uint64_t id = 0;
	uint64_t deadline_usec = 0;
	std::string name;
	uint64_t telemetry_environment_id = 0, telemetry_season_id = 0;
};

struct account_load_result
{
	uint64_t id = 0;
	std::string request_name;
	account_load_outcome outcome = account_load_outcome::load_failed;
	unsigned int error = 0;
	int repaired = 0;
	account_load_snapshot snapshot;
};

uint64_t account_load_now_usec();
uint64_t account_load_next_id();
// Shared with synchronous read_account callers; the caller owns the transaction.
// A zero deadline retains the existing synchronous repair behavior.
int account_load_repair(MYSQL *connection, const char *name, uint64_t deadline_usec = 0);
// Repair and read one transaction on the supplied, exclusively owned connection.
account_load_result account_load_execute(MYSQL *connection, const account_load_request &request);

struct account_load_job;
using account_load_execute_fn = account_load_result (*)(const account_load_request &, void *);
bool account_load_worker_init(account_load_execute_fn execute = nullptr, void *context = nullptr);
account_load_job *account_load_submit(account_load_request request);
bool account_load_poll(account_load_job *job, account_load_result *result);
void account_load_release(account_load_job *job);
// Cancels queued work, joins the in-flight transaction, and destroys all results.
// Descriptor continuations must be cancelled before this call.
void account_load_worker_shutdown();

#endif
