#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "account/password_async.h"
#include "net/command_latency.h"
#include "net/comm.h"
#include <openssl/crypto.h>
#include <cassert>
#include <chrono>
#include <thread>
#include <memory>
#include <string>
#include <type_traits>
#include <openssl/sha.h>

void write_to_q(const char *, struct txt_q *, const int) {}
void echo_on(P_desc) {}
void echo_off(P_desc) {}
bool valid_password(P_desc, char *arg)
{
	return strlen(arg) >= 6;
}
char *str_dup(const char *text)
{
	char *copy =
		static_cast<char *>(__malloc(strlen(text) + 1, MEM_TAG_STRING, __FILE__, __LINE__));
	return strcpy(copy, text);
}
void logit(const char *, const char *message, ...)
{
	fprintf(stderr, "%s\n", message);
}
void verify_new_account_information(P_desc, char *) {}
void display_account_menu(P_desc, char *) {}
static int saves = 0;
int write_account(P_acct)
{
	++saves;
	return 1;
}
void statuslog(int, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}
void account_recovery_invalidate(const char *) {}
using Clock = std::chrono::steady_clock;
static command_latency_tracker tracker = {};
static void record(Clock::time_point start, command_latency_kind kind, int state)
{
	command_latency_event event = {};
	command_latency_event_prepare(&event, kind, state, 1, "test",
				      kind == COMMAND_LATENCY_NANNY ? "nanny" : "open");
	command_latency_record(
		&tracker, &event,
		std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count());
}
static void drain(P_desc d, command_latency_kind kind)
{
	auto deadline = Clock::now() + std::chrono::seconds(15);
	while (d->password_request)
	{
		auto start = Clock::now();
		assert(password_async_pulse(d));
		record(start, kind, STATE(d));
		assert(Clock::now() < deadline);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
static void no_slow(const char *line, void *)
{
	assert(!strstr(line, "COMMAND OP SLOW"));
}
int main(int argc, char **)
{
	// Verify the linked production allocator rejects NULL, unlike libc free.
	if (argc > 1)
	{
		__free(nullptr, __FILE__, __LINE__);
		return 0;
	}
	auto desc = std::make_unique<descriptor_data>();
	P_desc d = desc.get();
	auto acct = std::make_unique<std::remove_pointer_t<P_acct>>();
	d->account = acct.get();
	char name[] = "Test";
	char email[] = "test@example.invalid";
	d->account->acct_name = name;
	d->account->acct_email = email;
	// Exercise the actual account-creation and confirmation handlers, including
	// mismatch/retry and the confirmed account password-change save branch.
	char entered[] = "password-one";
	STATE(d) = CON_GET_NEW_ACCT_PASSWD;
	auto nanny_start = Clock::now();
	get_new_account_password(d, entered);
	record(nanny_start, COMMAND_LATENCY_NANNY, STATE(d));
	assert(d->password_request && !d->account->acct_password && !entered[0]);
	drain(d, COMMAND_LATENCY_NANNY);
	assert(STATE(d) == CON_VERIFY_NEW_ACCT_PASSWD);
	char wrong[] = "different";
	nanny_start = Clock::now();
	verify_new_account_password(d, wrong);
	record(nanny_start, COMMAND_LATENCY_NANNY, STATE(d));
	drain(d, COMMAND_LATENCY_NANNY);
	assert(STATE(d) == CON_GET_NEW_ACCT_PASSWD && !d->account->acct_password);
	strcpy(entered, "password-one");
	get_new_account_password(d, entered);
	drain(d, COMMAND_LATENCY_NANNY);
	strcpy(entered, "password-one");
	verify_new_account_password(d, entered);
	drain(d, COMMAND_LATENCY_NANNY);
	assert(STATE(d) == CON_VERIFY_NEW_ACCT_INFO && saves == 0);
	d->account->acct_confirmed = 1;
	STATE(d) = CON_ACCT_CHANGE_PASSWD;
	strcpy(entered, "password-one");
	get_new_account_password(d, entered);
	drain(d, COMMAND_LATENCY_NANNY);
	strcpy(entered, "password-one");
	verify_new_account_password(d, entered);
	drain(d, COMMAND_LATENCY_NANNY);
	assert(STATE(d) == CON_DISPLAY_ACCT_MENU && saves == 1);
	FREE(d->account->acct_password);
	d->account->acct_password = nullptr;
	std::string hash;
	int calls = 0;
	STATE(d) = CON_GET_NEW_ACCT_PASSWD;
	auto start = Clock::now();
	assert(password_async_start(d, password_work_submit("password-one", nullptr, nullptr, 0, 0),
				    nullptr,
				    [&](P_desc done, int valid, const char *result)
				    {
					    assert(done == d && valid && result);
					    assert(!strncmp(result, "$2b$12$", 7));
					    hash = result;
					    ++calls;
				    }));
	record(start, COMMAND_LATENCY_NANNY, STATE(d));
	assert(calls == 0);
	drain(d, COMMAND_LATENCY_NANNY);
	assert(calls == 1);
	d->account->acct_password = hash.data();
	STATE(d) = CON_VERIFY_NEW_ACCT_PASSWD;
	assert(password_async_start(d, password_login_submit("password-one", hash.c_str(), 0),
				    hash.c_str(),
				    [&](P_desc, int valid, const char *)
				    {
					    assert(valid);
					    ++calls;
				    }));
	drain(d, COMMAND_LATENCY_NANNY);
	assert(calls == 2);
	// Replacement hashing is conditional on successful verification.
	std::string replacement;
	assert(password_async_start(
		d, password_work_submit("wrong", hash.c_str(), "password-two", 0, 0), hash.c_str(),
		[&](P_desc, int valid, const char *result)
		{
			assert(!valid && !result);
			++calls;
		}));
	drain(d, COMMAND_LATENCY_NANNY);
	assert(password_async_start(
		d, password_work_submit("password-one", hash.c_str(), "password-two", 0, 0),
		hash.c_str(),
		[&](P_desc, int valid, const char *result)
		{
			assert(valid && result);
			replacement = result;
		}));
	drain(d, COMMAND_LATENCY_NANNY);
	assert(bcrypt_verify_password("password-two", replacement.c_str()));
	// All session invalidations must discard completion, even if the worker finished.
	for (int scenario = 0; scenario < 4; ++scenario)
	{
		assert(password_async_start(
			d, password_work_submit("password-one", nullptr, nullptr, 0, 0), nullptr,
			[](P_desc, int, const char *) { assert(false); }));
		if (scenario == 0)
			STATE(d) = CON_DISPLAY_ACCT_MENU;
		if (scenario == 1)
			d->account->acct_password = replacement.data();
		if (scenario == 2)
			d->account = nullptr;
		if (scenario == 3)
		{
			password_async_cancel(d);
		}
		drain(d, COMMAND_LATENCY_NANNY);
		d->account = acct.get();
		d->account->acct_password = hash.data();
		STATE(d) = CON_VERIFY_NEW_ACCT_PASSWD;
	}
	auto actor = std::make_unique<std::remove_pointer_t<P_char>>();
	d->character = actor.get();
	actor->in_room = 42;
	SET_POS(actor.get(), POS_STANDING + STAT_NORMAL);
	STATE(d) = CON_PLAYING;
	// Non-owner chest legacy SHA-256 verification and bcrypt upgrade.
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256((const unsigned char *)"password-one", 12, digest);
	char legacy[65];
	for (size_t i = 0; i < sizeof(digest); ++i)
		snprintf(legacy + i * 2, 3, "%02x", digest[i]);
	start = Clock::now();
	assert(password_async_start(d, password_work_submit("password-one", legacy, nullptr, 1, 1),
				    legacy,
				    [&](P_desc, int valid, const char *result)
				    {
					    assert(valid && result && is_bcrypt_hash(result));
					    ++calls;
				    }));
	record(start, COMMAND_LATENCY_PLAYING, STATE(d));
	drain(d, COMMAND_LATENCY_PLAYING);
	for (int scenario = 0; scenario < 3; ++scenario)
	{
		assert(password_async_start(
			d, password_login_submit("password-one", hash.c_str(), 0), hash.c_str(),
			[](P_desc, int, const char *) { assert(false); }));
		if (scenario == 0)
			actor->in_room = 43;
		if (scenario == 1)
			SET_POS(actor.get(), POS_PRONE + STAT_DEAD);
		if (scenario == 2)
			d->character = nullptr;
		drain(d, COMMAND_LATENCY_PLAYING);
		d->character = actor.get();
		actor->in_room = 42;
		SET_POS(actor.get(), POS_STANDING + STAT_NORMAL);
	}
	assert(tracker.slow_count == 0);
	command_latency_report(&tracker, tracker.measured_us, "test", 1, 1, no_slow, nullptr);
	password_async_cancel(d);
	password_login_shutdown();
	puts("PASS: real bcrypt jobs, state/credential/disconnect/room/death cancellation; no COMMAND OP SLOW");
}
