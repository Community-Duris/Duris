#include "account/account_async.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "net/ws_handlers.h"

#include <openssl/crypto.h>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using Clock = std::chrono::steady_clock;
static const auto game_thread = std::this_thread::get_id();
P_acct account_list = nullptr;
static std::string output;
static int auth_failures = 0, verified_names = 0, password_submissions = 0;
static std::string submitted_password, submitted_hash;
void write_to_q(const char *value, struct txt_q *, const int)
{
	output += value;
}
void ws_send_auth_failed(P_desc, const char *)
{
	++auth_failures;
}
static bool ws_player_auth_attempt(P_desc, int)
{
	return true;
}
void echo_on(P_desc) {}
void echo_off(P_desc) {}
bool account_recovery_enabled()
{
	return false;
}
void verify_account_name(P_desc, char *)
{
	++verified_names;
}
bool _parse_name(char *name, char *out, bool)
{
	strcpy(out, name);
	return false;
}
void close_socket(P_desc d)
{
	account_async_cancel(d); // Same lifecycle hook as production close_socket.
	d->account = free_account(d->account);
	*d = {};
}
char *str_dup(const char *value)
{
	assert(std::this_thread::get_id() == game_thread);
	auto *copy = static_cast<char *>(
		__malloc(strlen(value) + 1, MEM_TAG_STRING, __FILE__, __LINE__));
	return strcpy(copy, value);
}
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}
password_login_job *password_login_submit(const char *password, const char *hash, int upgrade)
{
	assert(std::this_thread::get_id() == game_thread && upgrade == 0);
	submitted_password = password;
	submitted_hash = hash;
	++password_submissions;
	return reinterpret_cast<password_login_job *>(1);
}
extern "C" MYSQL *sql_pool_acquire()
{
	assert(false);
	return nullptr;
}
extern "C" void sql_pool_release(MYSQL *) {}
extern "C" void sql_pool_discard_connection(MYSQL *) {}

struct query_control
{
	std::mutex mutex;
	std::condition_variable changed;
	bool hold = false, entered = false;
	account_load_outcome outcome = account_load_outcome::loaded;
	char blocked = 0;
};
static account_load_result query(const account_load_request &request, void *raw)
{
	assert(std::this_thread::get_id() != game_thread);
	auto &control = *static_cast<query_control *>(raw);
	std::unique_lock<std::mutex> lock(control.mutex);
	control.entered = true;
	control.changed.notify_all();
	control.changed.wait(lock, [&] { return !control.hold; });
	account_load_result result = {};
	result.id = request.id;
	result.request_name = request.name;
	result.outcome = control.outcome;
	// Even failure results deliberately contain data; none may be published.
	result.snapshot.name = request.name;
	result.snapshot.password = "worker-hash";
	result.snapshot.email = "worker@example.invalid";
	result.snapshot.confirmation = "confirmation";
	result.snapshot.blocked = control.blocked;
	result.snapshot.confirmed = 1;
	result.snapshot.flags[0] = 123;
	result.snapshot.ips.push_back({ "localhost", "127.0.0.1", 2 });
	account_load_character character;
	character.pid = 42;
	character.name = "Hero";
	character.count = 8;
	character.primary_class = 4;
	character.secondary_class = 8;
	character.last_room = 99;
	result.snapshot.characters.push_back(character);
	control.entered = false;
	control.changed.notify_all();
	return result;
}
static void hold(query_control &control)
{
	std::lock_guard<std::mutex> lock(control.mutex);
	control.hold = true;
	control.entered = false;
}
static void entered(query_control &control)
{
	std::unique_lock<std::mutex> lock(control.mutex);
	assert(control.changed.wait_for(lock, std::chrono::seconds(3),
					[&] { return control.entered; }));
}
static void release(query_control &control)
{
	std::unique_lock<std::mutex> lock(control.mutex);
	control.hold = false;
	control.changed.notify_all();
	assert(control.changed.wait_for(lock, std::chrono::seconds(3),
					[&] { return !control.entered; }));
}
static void drain(P_desc d)
{
	const auto deadline = Clock::now() + std::chrono::seconds(3);
	while (d->account_request)
	{
		assert(account_async_pulse(d));
		assert(Clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
static void name(P_desc d, const char *value)
{
	char input[256];
	strcpy(input, value);
	select_accountname(d, input);
}
static account_load_request request()
{
	return { account_load_next_id(), account_load_now_usec() + ACCOUNT_LOAD_TIMEOUT_USEC,
		 "Queued" };
}
int main()
{
	query_control control;
	assert(account_load_worker_init(query, &control));
	auto descriptor = std::make_unique<descriptor_data>();
	P_desc d = descriptor.get();
	STATE(d) = CON_GET_ACCT_NAME;
	d->descriptor = 7;
	hold(control);
	auto start = Clock::now();
	name(d, "Example");
	assert(Clock::now() - start < std::chrono::milliseconds(100));
	entered(control);
	int world_ticks = 0;
	for (int tick = 0; tick < 100; ++tick)
	{
		start = Clock::now();
		assert(account_async_pulse(d));
		assert(Clock::now() - start < std::chrono::milliseconds(100));
		assert(STATE(d) == CON_GET_ACCT_NAME && !d->account->acct_password);
		assert(!d->prompt_mode);
		++world_ticks; // The loop can keep running commands, combat and world ticks.
	}
	assert(world_ticks == 100);
	// Another loaded account must retain its list position during publication.
	P_acct other = allocate_account();
	other->acct_name = str_dup("Other");
	release(control);
	drain(d);
	assert(STATE(d) == CON_GET_ACCT_PASSWD && d->prompt_mode);
	assert(account_list == other && other->next == d->account);
	assert(d->account->num_ips == 1 && d->account->num_chars == 1);
	assert(d->account->acct_flags1 == 123 && d->account->acct_character_list->pid == 42);
	assert(d->account->acct_character_list->secondary_class == 8);
	other = free_account(other);
	close_socket(d);

	// Disconnect and reuse both descriptor address and fd while a query is held.
	STATE(d) = CON_GET_ACCT_NAME;
	d->descriptor = 7;
	hold(control);
	name(d, "Old");
	entered(control);
	close_socket(d);
	STATE(d) = CON_GET_ACCT_NAME;
	d->descriptor = 7;
	name(d, "Replacement");
	release(control);
	drain(d);
	assert(!strcmp(d->account->acct_name, "Replacement"));
	close_socket(d);

	// Same session and account allocation, but a newer request supersedes the old one.
	STATE(d) = CON_GET_ACCT_NAME;
	hold(control);
	name(d, "First");
	entered(control);
	name(d, "Second");
	release(control);
	drain(d);
	assert(!strcmp(d->account->acct_name, "Second"));
	close_socket(d);

	// In-place name, account pointer, fd, state and character changes cancel without
	// touching the replacement context or running the old continuation.
	for (int change = 0; change < 5; ++change)
	{
		STATE(d) = CON_GET_ACCT_NAME;
		hold(control);
		name(d, "Stale");
		entered(control);
		P_acct original = d->account;
		if (change == 0)
		{
			d->account->acct_name = check_and_clear(d->account->acct_name);
			d->account->acct_name = str_dup("Changed");
		}
		else if (change == 1)
		{
			d->account = allocate_account();
			d->account->acct_name = str_dup("Changed");
		}
		else if (change == 2)
			++d->descriptor;
		else if (change == 3)
			STATE(d) = CON_DISPLAY_ACCT_MENU;
		else
			d->character = reinterpret_cast<P_char>(1);
		d->prompt_mode = FALSE;
		assert(account_async_pulse(d) && !d->account_request && !d->prompt_mode);
		assert(!d->account->acct_password);
		if (change == 1)
			free_account(original);
		d->character = nullptr;
		release(control);
		close_socket(d);
	}

	for (auto failure : { account_load_outcome::repair_failed,
			      account_load_outcome::load_failed, account_load_outcome::timed_out })
	{
		control.outcome = failure;
		STATE(d) = CON_GET_ACCT_NAME;
		name(d, "Failure");
		drain(d);
		assert(STATE(d) == CON_FLUSH && !d->account && !account_list);
		close_socket(d);
	}
	control.outcome = account_load_outcome::not_found;
	STATE(d) = CON_GET_ACCT_NAME;
	name(d, "New");
	drain(d);
	assert(STATE(d) == CON_VERIFY_NEW_ACCT_NAME && verified_names == 1);
	assert(!d->account->acct_password);
	close_socket(d);
	control.outcome = account_load_outcome::loaded;
	control.blocked = ACCOUNT_BLOCK_DELETION;
	STATE(d) = CON_GET_ACCT_NAME;
	name(d, "Fenced");
	drain(d);
	assert(STATE(d) == CON_GET_ACCT_PASSWD &&
	       d->account->acct_blocked == ACCOUNT_BLOCK_DELETION);
	close_socket(d);

	// Execute the native WebSocket login continuation and reuse its password worker.
	auto *json = cJSON_Parse("{\"account\":\"example\",\"password\":\"entered-secret\"}");
	assert(json);
	STATE(d) = CON_GET_ACCT_NAME;
	d->websocket = 1;
	ws_cmd_login(d, json);
	cJSON_Delete(json); // Continuation must own the password after frame destruction.
	drain(d);
	assert(d->login_password_job && d->login_password_websocket);
	assert(password_submissions == 1 && submitted_password == "entered-secret" &&
	       submitted_hash == "worker-hash");
	close_socket(d);
	for (auto failure : { account_load_outcome::not_found, account_load_outcome::repair_failed,
			      account_load_outcome::load_failed })
	{
		control.outcome = failure;
		STATE(d) = CON_GET_ACCT_NAME;
		json = cJSON_Parse("{\"account\":\"example\",\"password\":\"secret\"}");
		ws_cmd_login(d, json);
		cJSON_Delete(json);
		drain(d);
		assert(!d->account && !d->login_password_job && STATE(d) == CON_GET_ACCT_NAME);
		close_socket(d);
	}
	assert(auth_failures == 3 && password_submissions == 1);
	control.outcome = account_load_outcome::loaded;
	hold(control);
	auto *first = account_load_submit(request());
	assert(first);
	entered(control);
	std::vector<account_load_job *> jobs{ first };
	for (size_t i = 1; i < ACCOUNT_LOAD_MAX_PENDING; ++i)
	{
		auto *job = account_load_submit(request());
		assert(job);
		jobs.push_back(job);
	}
	assert(!account_load_submit(request()));
	// Saturation leaves telnet and WebSocket login contexts consistent.
	STATE(d) = CON_GET_ACCT_NAME;
	name(d, "Busy");
	assert(STATE(d) == CON_FLUSH && !d->account && !d->account_request);
	close_socket(d);
	STATE(d) = CON_GET_ACCT_NAME;
	json = cJSON_Parse("{\"account\":\"example\",\"password\":\"secret\"}");
	ws_cmd_login(d, json);
	cJSON_Delete(json);
	assert(!d->account && !d->account_request && auth_failures == 4);
	for (auto *job : jobs)
		account_load_release(job);
	// Cancelled queued jobs free capacity without executing. In-flight work remains owned.
	auto *queued = account_load_submit(request());
	assert(queued);
	account_load_release(queued);
	release(control);
	account_load_worker_shutdown();
	assert(!account_load_submit(request()));
	// Request IDs do not reset on worker restart.
	const auto before = account_load_next_id();
	assert(account_load_worker_init(query, &control));
	assert(account_load_next_id() > before);
	account_load_worker_shutdown();
	assert(!account_list);
	puts("account load latency, stale sessions, failures, saturation and shutdown passed");
}
