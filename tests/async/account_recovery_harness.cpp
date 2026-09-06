/*
 * Executes the account-recovery core (src/account/account_recovery.c) together with
 * the mail sender's queue and worker (src/net/mail_sender.c), with no network: the send
 * function is an injected capture, the clock is injected, and the engine symbols the
 * core reaches for -- logit, statuslog, is_valid_email, account_apply_recovered_password
 * -- are the stubs below.  The log stubs RENDER their lines into the file named in
 * argv[1], so the last section can prove that no code, address, host or account name was
 * ever formatted into a log line.  Driven by tests/async/test_account_recovery.py; every
 * section prints "ok: <name>" and the first failed expectation ends the run.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "account/account.h"
#include "account/account_recovery.h"
#include "account/password_hash.h"
#include "net/mail_sender.h"

#include <curl/curl.h>
#include <openssl/crypto.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
std::string log_path;
std::mutex log_mutex;

/* The real loggers format their arguments, so a value passed to them WOULD reach disk;
 * rendering here is what makes the final hygiene section meaningful. */
__attribute__((format(printf, 1, 0))) void append_log(const char *format, va_list args)
{
	char line[1024];
	vsnprintf(line, sizeof line, format, args);
	std::lock_guard<std::mutex> lock(log_mutex);
	std::ofstream file(log_path, std::ios::app);
	file << line << '\n';
}
} // namespace

void logit(const char *, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	append_log(format, args);
	va_end(args);
}

void statuslog(int, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	append_log(format, args);
	va_end(args);
}

/* The core re-checks the address before mailing; this stand-in accepts user@host.tld. */
int is_valid_email(const char *email)
{
	if (!email)
		return 0;
	const char *at = strchr(email, '@');
	if (!at || at == email || !strchr(at + 1, '.'))
		return 0;
	return strchr(email, ' ') == nullptr;
}

namespace
{
account_recovery_apply_outcome scripted_apply = account_recovery_apply_outcome::ok;
unsigned apply_calls = 0;
std::string applied_name;
std::string applied_hash;
unsigned char applied_fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN] = {};
struct descriptor_data *applied_keep = nullptr;
} // namespace

/* account.c's helper, scripted: records what the core handed it and answers as told. */
account_recovery_apply_outcome account_apply_recovered_password(
	const char *acct_name, const char *bcrypt_hash,
	const unsigned char expected_fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN],
	struct descriptor_data *keep_session)
{
	++apply_calls;
	applied_name = acct_name ? acct_name : "";
	applied_hash = bcrypt_hash ? bcrypt_hash : "";
	if (expected_fingerprint)
		memcpy(applied_fingerprint, expected_fingerprint, sizeof applied_fingerprint);
	else
		memset(applied_fingerprint, 0, sizeof applied_fingerprint);
	applied_keep = keep_session;
	return scripted_apply;
}

namespace
{
enum class send_mode
{
	sent,
	retryable,
	terminal,
	block
};

struct captured_job
{
	std::string to;
	std::string message;
};

std::atomic<send_mode> mode{ send_mode::sent };
std::mutex capture_mutex;
std::condition_variable release_cv;
bool released = false;
std::vector<captured_job> captured; /* cleared by fresh() */
std::vector<captured_job> all_captured; /* never cleared: the hygiene section's corpus */

/* Runs on the worker thread.  In block mode it holds the worker until the harness
 * releases it or the sender raises its stop flag; the shutdown case relies on the flag
 * alone, which is exactly what a relay that never answers would look like. */
mail_result capture_send(const mail_job &job, const mail_sender_config &, void *)
{
	{
		std::lock_guard<std::mutex> lock(capture_mutex);
		captured.push_back({ job.to, job.message });
		all_captured.push_back({ job.to, job.message });
	}
	mail_result result;
	result.request_id = job.request_id;
	result.outcome = mail_outcome::sent;
	result.smtp_code = 250;
	switch (mode.load())
	{
	case send_mode::sent:
		break;
	case send_mode::retryable:
		result.outcome = mail_outcome::retryable_failure;
		result.curl_code = CURLE_COULDNT_CONNECT;
		result.smtp_code = 0;
		break;
	case send_mode::terminal:
		result.outcome = mail_outcome::terminal_failure;
		result.curl_code = CURLE_SEND_ERROR;
		result.smtp_code = 550;
		break;
	case send_mode::block:
	{
		std::unique_lock<std::mutex> lock(capture_mutex);
		while (!released && !mail_sender_health_copy().stop_pending)
			release_cv.wait_for(lock, std::chrono::milliseconds(1));
		break;
	}
	}
	return result;
}

constexpr time_t base_time = 1700000000;
time_t fake_now = base_time;

time_t fake_clock()
{
	return fake_now;
}

constexpr unsigned char fixed_fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
	0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
};

/* 60 bytes in the $2b$12$ shape is_bcrypt_hash accepts; the hash of no password. */
constexpr char bcrypt_shaped_hash[] =
	"$2b$12$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0";

/* Everything the log must never carry, recorded as it is handed to the core. */
std::vector<std::string> names_used;
std::vector<std::string> emails_used;
std::vector<std::string> hosts_used;
unsigned host_counter = 0;

/* A failed expectation ends the run; the sender is stopped first because exit() with a
 * joinable worker thread is std::terminate, which would bury the message. */
void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		account_recovery_reset_for_tests();
		exit(1);
	}
}

void passed(const char *section)
{
	std::cout << "ok: " << section << '\n';
}

/* Built in code, never from MAIL_*: the harness must not depend on the environment. */
mail_sender_config harness_config()
{
	mail_sender_config config;
	config.host = "127.0.0.1";
	config.port = 2525;
	config.tls = false;
	config.from = "noreply@duris.test";
	config.from_domain = "duris.test";
	return config;
}

template <typename Predicate> bool wait_until(Predicate ready, int timeout_ms)
{
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
	while (!ready())
	{
		if (std::chrono::steady_clock::now() >= deadline)
			return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return true;
}

void wait_worker_idle()
{
	require(wait_until(
			[]
			{
				const mail_sender_health health = mail_sender_health_copy();
				return health.queued == 0 && health.inflight == 0;
			},
			5000),
		"the mail worker did not drain its queue within 5 s");
}

/* Idle worker AND empty completion deque: the sender refuses new jobs once 256 results
 * wait to be pulled, exactly as it would if comm.c stopped pulsing. */
void settle()
{
	wait_worker_idle();
	while (account_recovery_pulse() > 0)
		;
}

size_t captured_count()
{
	std::lock_guard<std::mutex> lock(capture_mutex);
	return captured.size();
}

captured_job captured_at(size_t index)
{
	std::lock_guard<std::mutex> lock(capture_mutex);
	require(index < captured.size(), "fewer mails were captured than the section expects");
	return captured[index];
}

void release_worker()
{
	{
		std::lock_guard<std::mutex> lock(capture_mutex);
		released = true;
	}
	release_cv.notify_all();
}

std::string next_host()
{
	++host_counter;
	char text[32];
	snprintf(text, sizeof text, "10.%u.%u.%u", (host_counter >> 16) & 0xffu,
		 (host_counter >> 8) & 0xffu, host_counter & 0xffu);
	return text;
}

account_recovery_request_outcome request(const std::string &name, const char *email, char blocked,
					 const std::string &host, uint64_t *request_id = nullptr)
{
	names_used.push_back(name);
	if (email && strchr(email, '@'))
		emails_used.push_back(email);
	hosts_used.push_back(host);
	return account_recovery_request(name.c_str(), email, blocked, fixed_fingerprint,
					host.c_str(), request_id);
}

account_recovery_check_outcome check(const std::string &name, const std::string &typed,
				     char *normalized_out = nullptr)
{
	char scratch[ACCOUNT_RECOVERY_CODE_BUF] = {};
	const account_recovery_check_outcome outcome = account_recovery_check(
		name.c_str(), typed.c_str(), normalized_out ? normalized_out : scratch);
	OPENSSL_cleanse(scratch, sizeof scratch);
	return outcome;
}

account_recovery_complete_outcome complete(const std::string &name, const char *normalized,
					   const char *hash)
{
	return account_recovery_complete(name.c_str(), normalized, hash, nullptr);
}

/* The 8-8-8-8 form the mail body carries, back to the 32 hex digits the core checks. */
std::string code_from(const std::string &message)
{
	const size_t at = message.find("Reset code: ");
	require(at != std::string::npos, "rendered message carries no reset code line");
	const std::string dashed = message.substr(at + strlen("Reset code: "), 35);
	std::string code;
	for (char c : dashed)
		if (c != '-')
			code.push_back(c);
	require(code.size() == ACCOUNT_RECOVERY_CODE_HEX_LEN, "reset code is not 32 digits");
	for (char c : code)
		require((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'),
			"reset code carries a byte that is not lowercase hex");
	return code;
}

std::string dashed(const std::string &code)
{
	std::string text;
	for (size_t index = 0; index < code.size(); ++index)
	{
		if (index && index % 8 == 0)
			text.push_back('-');
		text.push_back(code[index]);
	}
	return text;
}

std::string uppercase(std::string text)
{
	for (char &c : text)
		if (c >= 'a' && c <= 'z')
			c = static_cast<char>(c - ('a' - 'A'));
	return text;
}

std::string flip_last_nibble(std::string code)
{
	code.back() = code.back() == '0' ? '1' : '0';
	return code;
}

size_t count_of(const std::string &text, const std::string &needle)
{
	size_t total = 0;
	for (size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1))
		++total;
	return total;
}

/* Every section starts from an empty store, an idle worker and the base clock. */
void fresh()
{
	account_recovery_reset_for_tests();
	{
		std::lock_guard<std::mutex> lock(capture_mutex);
		captured.clear();
		released = false;
	}
	mode = send_mode::sent;
	scripted_apply = account_recovery_apply_outcome::ok;
	apply_calls = 0;
	fake_now = base_time;
	account_recovery_set_clock_for_tests(fake_clock);
	require(mail_sender_init(harness_config(), capture_send, nullptr),
		"mail sender did not start");
	require(account_recovery_enabled(), "core does not report enabled once the sender runs");
}

void section_fingerprint()
{
	unsigned char first[ACCOUNT_RECOVERY_FINGERPRINT_LEN];
	unsigned char second[ACCOUNT_RECOVERY_FINGERPRINT_LEN];
	account_recovery_credential_fingerprint(nullptr, nullptr, first);
	account_recovery_credential_fingerprint("", "", second);
	require(memcmp(first, second, sizeof first) == 0, "NULL and empty credentials differ");
	account_recovery_credential_fingerprint("$2b$12$hash", "one@duris.test", first);
	account_recovery_credential_fingerprint("$2b$12$hash", "one@duris.test", second);
	require(memcmp(first, second, sizeof first) == 0, "fingerprint is not deterministic");
	account_recovery_credential_fingerprint("$2b$12$hash", "two@duris.test", second);
	require(memcmp(first, second, sizeof first) != 0, "a different email gave the same digest");
	account_recovery_credential_fingerprint("$2b$12$other", "one@duris.test", second);
	require(memcmp(first, second, sizeof first) != 0, "a different hash gave the same digest");
	account_recovery_credential_fingerprint("$2b$12$hash", nullptr, first);
	account_recovery_credential_fingerprint("$2b$12$hash", "", second);
	require(memcmp(first, second, sizeof first) == 0, "NULL email is not the empty email");
	passed("fingerprint");
}

void section_classify()
{
	require(CURLE_SEND_ERROR == 55 && CURLE_WEIRD_SERVER_REPLY == 8 &&
			CURLE_COULDNT_CONNECT == 7 && CURLE_LOGIN_DENIED == 67 &&
			CURLE_OPERATION_TIMEDOUT == 28,
		"libcurl error numbering changed under the classification pins");
	require(mail_sender_classify(CURLE_SEND_ERROR, 550) == mail_outcome::terminal_failure,
		"RCPT 550 (reported as CURLE_SEND_ERROR) must be terminal");
	require(mail_sender_classify(CURLE_WEIRD_SERVER_REPLY, 451) ==
			mail_outcome::retryable_failure,
		"post-DATA 451 (reported as CURLE_WEIRD_SERVER_REPLY) must be retryable");
	require(mail_sender_classify(CURLE_OK, 250) == mail_outcome::sent, "250 must be sent");
	require(mail_sender_classify(CURLE_COULDNT_CONNECT, 0) == mail_outcome::retryable_failure,
		"a refused connection must be retryable");
	require(mail_sender_classify(CURLE_LOGIN_DENIED, 0) == mail_outcome::terminal_failure,
		"a rejected login must be terminal");
	require(mail_sender_classify(CURLE_OK, 0) == mail_outcome::sent, "CURLE_OK alone is sent");
	require(mail_sender_classify(CURLE_OPERATION_TIMEDOUT, 0) ==
			mail_outcome::retryable_failure,
		"a timeout must be retryable");
	require(mail_sender_classify(CURLE_PEER_FAILED_VERIFICATION, 0) ==
			mail_outcome::terminal_failure,
		"a failed certificate check must be terminal");
	require(mail_sender_classify(CURLE_RECV_ERROR, 421) == mail_outcome::retryable_failure,
		"a 4xx class decides before the curl code");
	passed("classify");
}

void section_render()
{
	const std::string to = "player@duris.test";
	const std::string from = "noreply@duris.test";
	const std::string domain = "duris.test";
	std::string out;
	require(mail_sender_render_message(to, from, domain, "Subject line",
					   "Line one.\r\nLine two.\r\n", &out),
		"a well-formed message was refused");
	require(out.find("Subject: Subject line\r\n") != std::string::npos,
		"Subject header missing");
	require(out.find("From: noreply@duris.test\r\n") != std::string::npos,
		"From header missing");
	require(out.find("To: player@duris.test\r\n") != std::string::npos, "To header missing");
	require(out.find("MIME-Version: 1.0\r\n") != std::string::npos, "MIME-Version missing");
	require(out.find("Content-Type: text/plain; charset=utf-8\r\n") != std::string::npos,
		"Content-Type missing");
	require(out.find("Content-Transfer-Encoding: 7bit\r\n") != std::string::npos,
		"Content-Transfer-Encoding missing");
	require(out.find("Date: ") != std::string::npos, "Date header missing");
	require(out.find("Message-ID: <") != std::string::npos &&
			out.find("@duris.test>\r\n") != std::string::npos,
		"Message-ID does not end in the sender domain");
	require(count_of(out, "\n") == count_of(out, "\r\n"), "a rendered line is not CRLF");
	require(!mail_sender_render_message(to, from, domain, "Subject",
					    "Caf\xC3\xA9"
					    "\r\n",
					    &out),
		"an 8-bit body byte was accepted under 7bit encoding");
	require(!mail_sender_render_message(to, from, domain, "Caf\xC3\xA9", "ok\r\n", &out),
		"an 8-bit subject byte was accepted under 7bit encoding");
	require(!mail_sender_render_message(to, from, domain, "Subject", "bare\nline\r\n", &out),
		"a bare LF was accepted");
	require(!mail_sender_render_message(to, from, domain, "Subject", "bare\rline\r\n", &out),
		"a bare CR was accepted");
	require(!mail_sender_render_message(to, from, domain, "Subject\r\nBcc: x@duris.test",
					    "ok\r\n", &out),
		"CRLF in the subject was accepted");
	require(!mail_sender_render_message("player@duris.test\r\nRCPT TO:<x@duris.test>", from,
					    domain, "Subject", "ok\r\n", &out),
		"CRLF in the recipient was accepted");
	require(!mail_sender_render_message("<player@duris.test>", from, domain, "Subject",
					    "ok\r\n", &out),
		"angle brackets in the recipient were accepted");
	require(mail_sender_address_is_safe("player@duris.test"), "a plain address was refused");
	require(!mail_sender_address_is_safe("player@duris"),
		"a domain without a dot was accepted");
	require(!mail_sender_address_is_safe("@duris.test"), "an empty local part was accepted");
	require(!mail_sender_address_is_safe("a b@duris.test"), "a space was accepted");
	passed("render");
}

void section_canonical_name()
{
	char out[ACCOUNT_RECOVERY_NAME_BUF];
	require(account_recovery_canonical_name("Fraser", out) && !strcmp(out, "fraser"),
		"Fraser does not fold to fraser");
	require(account_recovery_canonical_name("Web_Player14", out) &&
			!strcmp(out, "web_player14"),
		"the WS name class does not canonicalise");
	require(!account_recovery_canonical_name(nullptr, out), "NULL name accepted");
	require(!account_recovery_canonical_name("", out), "empty name accepted");
	require(!account_recovery_canonical_name(" x", out), "a space byte accepted");
	require(!account_recovery_canonical_name("x\x7f", out), "a DEL byte accepted");
	require(!account_recovery_canonical_name("\xC3", out), "an 8-bit byte accepted");
	const std::string sixty_four(ACCOUNT_RECOVERY_NAME_MAX, 'b');
	const std::string sixty_five(ACCOUNT_RECOVERY_NAME_MAX + 1, 'b');
	require(account_recovery_canonical_name(sixty_four.c_str(), out) && out == sixty_four,
		"a 64-byte name was refused");
	require(!account_recovery_canonical_name(sixty_five.c_str(), out),
		"a 65-byte name accepted");
	passed("canonical_name");
}

void section_a_request_renders_mail()
{
	fresh();
	uint64_t request_id = 0;
	require(request("Fraser", "fraser@duris.test", 0, next_host(), &request_id) ==
			account_recovery_request_outcome::queued,
		"a valid request was not queued");
	require(request_id != 0, "queued request reported no request id");
	wait_worker_idle();
	require(captured_count() == 1, "one request did not produce exactly one mail");
	const captured_job job = captured_at(0);
	require(job.to == "fraser@duris.test", "mail was addressed elsewhere");
	require(job.message.find("Subject: Duris account password reset\r\n") != std::string::npos,
		"subject literal missing");
	require(job.message.find("To: fraser@duris.test\r\n") != std::string::npos, "To missing");
	require(job.message.find("From: noreply@duris.test\r\n") != std::string::npos,
		"From missing");
	require(job.message.find("Content-Transfer-Encoding: 7bit\r\n") != std::string::npos,
		"7bit declaration missing");
	const size_t subject_at = job.message.find("Subject: ");
	const size_t subject_end = job.message.find("\r\n", subject_at);
	require(job.message.substr(subject_at, subject_end - subject_at).find('@') ==
			std::string::npos,
		"the Subject carries an address");
	require(count_of(job.message, "\n") == count_of(job.message, "\r\n"),
		"a mail line is not CRLF-terminated");
	const std::string code = code_from(job.message);
	require(job.message.find(dashed(code)) != std::string::npos, "code is not in 8-8-8-8 form");
	require(job.message.find("Duris account Fraser.") != std::string::npos,
		"body does not name the account in Title case");
	const account_recovery_health health = account_recovery_health_copy();
	require(health.requests == 1 && health.queued == 1 && health.live == 1 &&
			health.entries == 1,
		"health counters do not describe one queued live token");
	passed("a_request_renders_mail");
}

void section_b_suppressed_tombstone()
{
	fresh();
	struct variant
	{
		const char *name;
		const char *email;
		char blocked;
	};
	const variant variants[] = { { "Bravo1", nullptr, 0 },
				     { "Bravo2", "", 0 },
				     { "Bravo3", "bad", 0 },
				     { "Bravo4", "bravo4@duris.test", 2 } };
	for (const variant &item : variants)
		require(request(item.name, item.email, item.blocked, next_host()) ==
				account_recovery_request_outcome::suppressed,
			"a request with no destination or a blocked account was not suppressed");
	require(captured_count() == 0, "a suppressed request produced a mail");
	account_recovery_health health = account_recovery_health_copy();
	require(health.tombstones == 4 && health.live == 0,
		"suppressed requests left no tombstones");
	require(health.suppressed_no_destination == 3 && health.suppressed_blocked == 1,
		"suppression counters do not match the variants");

	/* Inside the cooldown a now-valid address changes nothing; after it, a mail goes out. */
	require(request("Bravo1", "bravo1@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::suppressed,
		"a tombstone did not suppress inside the cooldown");
	require(captured_count() == 0, "a cooldown-suppressed request produced a mail");
	health = account_recovery_health_copy();
	require(health.suppressed_cooldown == 1, "cooldown suppression was not counted");
	fake_now += ACCOUNT_RECOVERY_COOLDOWN_SEC + 1;
	require(request("Bravo1", "bravo1@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a request after the cooldown was not queued");
	wait_worker_idle();
	require(captured_count() == 1 && captured_at(0).to == "bravo1@duris.test",
		"the post-cooldown request did not mail the address on file");
	passed("b_suppressed_tombstone");
}

void section_c_canonical_keying()
{
	fresh();
	require(request("Fraser", "fraser@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Fraser was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	char normalized[ACCOUNT_RECOVERY_CODE_BUF] = {};
	require(check("fRASER", code, normalized) == account_recovery_check_outcome::accepted,
		"a case variant of the name did not reach the same token");
	require(normalized == code, "normalized_out differs from the issued code");
	OPENSSL_cleanse(normalized, sizeof normalized);

	require(request("Web_Player14", "web@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a 14-character underscore name was not queued");
	wait_worker_idle();
	const std::string web_code = code_from(captured_at(1).message);
	require(check("web_player14", web_code) == account_recovery_check_outcome::accepted &&
			check("WEB_PLAYER14", web_code) == account_recovery_check_outcome::accepted,
		"the WS name class is not recoverable");

	const std::string sixty_four(ACCOUNT_RECOVERY_NAME_MAX, 'a');
	const std::string sixty_five(ACCOUNT_RECOVERY_NAME_MAX + 1, 'a');
	require(request(sixty_four, "long@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a 64-byte name was refused");
	wait_worker_idle();
	require(captured_count() == 3, "the 64-byte name did not mail");
	require(request(sixty_five, "long@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::invalid_name,
		"a 65-byte name was accepted");
	require(request("Fr\xC3\xA9"
			"d",
			"eight@duris.test", 0,
			next_host()) == account_recovery_request_outcome::invalid_name,
		"a name carrying byte 0xC3 was accepted");
	require(captured_count() == 3, "an invalid name produced a mail");
	require(account_recovery_health_copy().invalid_name == 2, "invalid names were not counted");
	passed("c_canonical_keying");
}

void section_d_check_normalises_without_consuming()
{
	fresh();
	require(request("Delta", "delta@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Delta was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	require(check("Delta", code) == account_recovery_check_outcome::accepted,
		"exact code refused");
	require(check("Delta", uppercase(code)) == account_recovery_check_outcome::accepted,
		"uppercase code refused");
	require(check("Delta", dashed(code)) == account_recovery_check_outcome::accepted,
		"dashed code refused");
	std::string spaced;
	for (size_t index = 0; index < code.size(); ++index)
	{
		if (index && index % 4 == 0)
			spaced.push_back(' ');
		spaced.push_back(code[index]);
	}
	require(check("Delta", spaced) == account_recovery_check_outcome::accepted,
		"space-separated code refused");
	require(check("Delta", flip_last_nibble(code)) == account_recovery_check_outcome::rejected,
		"a one-nibble change was accepted");
	require(check("Delta", "zzz") == account_recovery_check_outcome::rejected,
		"malformed input was accepted");
	require(check("Delta", code) == account_recovery_check_outcome::accepted,
		"check consumed the token");
	const account_recovery_health health = account_recovery_health_copy();
	require(health.accepted == 5 && health.rejected == 2, "check counters do not match");
	passed("d_check_normalises_without_consuming");
}

void section_e_token_attempt_cap()
{
	fresh();
	require(request("Echo", "echo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Echo was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	const std::string wrong = flip_last_nibble(code);
	for (unsigned attempt = 1; attempt < ACCOUNT_RECOVERY_MAX_TOKEN_ATTEMPTS; ++attempt)
		require(check("Echo", wrong) == account_recovery_check_outcome::rejected,
			"an early mismatch did not read as rejected");
	require(check("Echo", wrong) == account_recovery_check_outcome::capped,
		"the fifth mismatch did not cap the token");
	require(account_recovery_health_copy().capped == 1, "the cap was not counted");
	require(check("Echo", code) == account_recovery_check_outcome::rejected,
		"the correct code survived the cap");
	require(request("Echo", "echo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::suppressed,
		"a capped token did not leave a cooldown tombstone");
	require(captured_count() == 1, "a re-request inside the cooldown mailed");
	fake_now += ACCOUNT_RECOVERY_COOLDOWN_SEC + 1;
	require(request("Echo", "echo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a re-request after the cooldown was not queued");
	wait_worker_idle();
	require(captured_count() == 2, "the post-cooldown request did not mail");
	const std::string fresh_code = code_from(captured_at(1).message);
	require(check("Echo", fresh_code) == account_recovery_check_outcome::accepted &&
			check("Echo", code) == account_recovery_check_outcome::rejected,
		"the new code does not replace the capped one");
	passed("e_token_attempt_cap");
}

void section_f_two_phase_complete()
{
	fresh();
	require(request("Foxtrot", "foxtrot@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Foxtrot was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	char normalized[ACCOUNT_RECOVERY_CODE_BUF] = {};
	require(check("Foxtrot", code, normalized) == account_recovery_check_outcome::accepted,
		"the code was not accepted");
	scripted_apply = account_recovery_apply_outcome::ok;
	require(complete("Foxtrot", normalized, bcrypt_shaped_hash) ==
			account_recovery_complete_outcome::ok,
		"complete did not report ok");
	require(apply_calls == 1 && applied_name == "Foxtrot" && applied_hash == bcrypt_shaped_hash,
		"the apply helper did not receive the name and hash");
	require(memcmp(applied_fingerprint, fixed_fingerprint, sizeof fixed_fingerprint) == 0,
		"the apply helper did not receive the fingerprint captured at request time");
	require(applied_keep == nullptr, "keep_session was not passed through");
	require(account_recovery_health_copy().completed == 1, "completion was not counted");
	require(complete("Foxtrot", normalized, bcrypt_shaped_hash) ==
			account_recovery_complete_outcome::rejected,
		"a consumed token completed twice");
	require(apply_calls == 1, "a rejected completion reached the apply helper");
	require(check("Foxtrot", code) == account_recovery_check_outcome::rejected,
		"a consumed token still checks");
	require(request("Foxtrot", "foxtrot@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::suppressed,
		"the tombstone did not survive consumption");
	require(captured_count() == 1, "a re-request after consumption mailed inside the cooldown");
	require(complete("Foxtrot", normalized, "not-a-bcrypt-hash") ==
			account_recovery_complete_outcome::bad_hash,
		"a non-bcrypt hash was not refused");
	OPENSSL_cleanse(normalized, sizeof normalized);
	passed("f_two_phase_complete");
}

void section_g_fenced_and_superseded()
{
	fresh();
	struct scenario
	{
		const char *name;
		const char *email;
		account_recovery_apply_outcome applied;
		account_recovery_complete_outcome expected;
	};
	const scenario scenarios[] = {
		{ "Golf", "golf@duris.test", account_recovery_apply_outcome::fenced,
		  account_recovery_complete_outcome::fenced },
		{ "Hotel", "hotel@duris.test", account_recovery_apply_outcome::superseded,
		  account_recovery_complete_outcome::superseded },
	};
	size_t mails = 0;
	for (const scenario &item : scenarios)
	{
		require(request(item.name, item.email, 0, next_host()) ==
				account_recovery_request_outcome::queued,
			"scenario request was not queued");
		wait_worker_idle();
		const std::string code = code_from(captured_at(mails++).message);
		char normalized[ACCOUNT_RECOVERY_CODE_BUF] = {};
		require(check(item.name, code, normalized) ==
				account_recovery_check_outcome::accepted,
			"scenario code was not accepted");
		scripted_apply = item.applied;
		require(complete(item.name, normalized, bcrypt_shaped_hash) == item.expected,
			"complete did not relay the apply outcome");
		OPENSSL_cleanse(normalized, sizeof normalized);
		require(check(item.name, code) == account_recovery_check_outcome::rejected,
			"the token survived a fenced or superseded completion");
		require(request(item.name, item.email, 0, next_host()) ==
				account_recovery_request_outcome::suppressed,
			"the tombstone did not survive a fenced or superseded completion");
		require(captured_count() == mails, "a re-request inside the cooldown mailed");
	}
	const account_recovery_health health = account_recovery_health_copy();
	require(health.fenced == 1 && health.superseded == 1 && health.completed == 0,
		"fenced/superseded counters do not match");
	passed("g_fenced_and_superseded");
}

void section_h_failures_keep_token()
{
	fresh();
	require(request("India", "india@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"India was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	char normalized[ACCOUNT_RECOVERY_CODE_BUF] = {};
	require(check("India", code, normalized) == account_recovery_check_outcome::accepted,
		"the code was not accepted");
	scripted_apply = account_recovery_apply_outcome::write_failed;
	require(complete("India", normalized, bcrypt_shaped_hash) ==
			account_recovery_complete_outcome::write_failed,
		"write_failed was not relayed");
	scripted_apply = account_recovery_apply_outcome::load_failed;
	require(complete("India", normalized, bcrypt_shaped_hash) ==
			account_recovery_complete_outcome::load_failed,
		"load_failed was not relayed");
	require(check("India", code) == account_recovery_check_outcome::accepted,
		"a save failure killed the token");
	scripted_apply = account_recovery_apply_outcome::ok;
	require(complete("India", normalized, bcrypt_shaped_hash) ==
			account_recovery_complete_outcome::ok,
		"the same code did not complete after the store recovered");
	OPENSSL_cleanse(normalized, sizeof normalized);
	const account_recovery_health health = account_recovery_health_copy();
	require(health.write_failed == 1 && health.load_failed == 1 && health.completed == 1 &&
			apply_calls == 3,
		"failure counters do not match");
	passed("h_failures_keep_token");
}

void section_i_expiry_and_sweep()
{
	fresh();
	require(request("Juliet", "juliet@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Juliet was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	fake_now = base_time + ACCOUNT_RECOVERY_TTL_SEC + 1;
	require(check("Juliet", code) == account_recovery_check_outcome::rejected,
		"an expired code was accepted");
	require(account_recovery_health_copy().entries == 1, "expiry alone erased the entry");
	fake_now = base_time + 1201;
	for (int call = 0; call < 4; ++call)
		account_recovery_pulse();
	const account_recovery_health health = account_recovery_health_copy();
	require(health.entries == 0 && health.expired == 1,
		"the sweep did not erase the dead entry");
	passed("i_expiry_and_sweep");
}

void section_j_latest_wins()
{
	fresh();
	require(request("Kilo", "kilo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Kilo was not queued");
	wait_worker_idle();
	const std::string first = code_from(captured_at(0).message);
	fake_now += ACCOUNT_RECOVERY_COOLDOWN_SEC + 1;
	require(request("Kilo", "kilo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a post-cooldown request was not queued");
	wait_worker_idle();
	require(captured_count() == 2, "the second request did not mail");
	const std::string second = code_from(captured_at(1).message);
	require(first != second, "two issues produced the same code");
	require(check("Kilo", first) == account_recovery_check_outcome::rejected,
		"the replaced code still checks");
	require(check("Kilo", second) == account_recovery_check_outcome::accepted,
		"the latest code does not check");
	passed("j_latest_wins");
}

void section_k_host_window()
{
	fresh();
	const std::string shared = "203.0.113.7";
	const char *names[] = { "Kilo1", "Kilo2", "Kilo3", "Kilo4", "Kilo5" };
	for (const char *name : names)
		require(request(name, "kilo@duris.test", 0, shared) ==
				account_recovery_request_outcome::queued,
			"a request inside the host budget was refused");
	require(request("Kilo6", "kilo@duris.test", 0, shared) ==
			account_recovery_request_outcome::host_limited,
		"the sixth request from one host was not limited");
	wait_worker_idle();
	require(captured_count() == 5, "a host-limited request mailed");
	/* The limited request left the store untouched, so the name is free from elsewhere. */
	require(request("Kilo6", "kilo@duris.test", 0, "198.51.100.9") ==
			account_recovery_request_outcome::queued,
		"a host-limited request left a tombstone");
	require(request("Kilo7", "kilo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a different host was refused");

	/* One subscriber usually holds a whole /64: six hosts in one prefix share a budget. */
	const char *prefix_hosts[] = { "2001:db8:1:2::1", "2001:db8:1:2::2", "2001:db8:1:2::3",
				       "2001:db8:1:2::4", "2001:db8:1:2::5" };
	unsigned index = 0;
	for (const char *host : prefix_hosts)
		require(request("Lima" + std::to_string(++index), "lima@duris.test", 0, host) ==
				account_recovery_request_outcome::queued,
			"a request inside the /64 budget was refused");
	require(request("Lima6", "lima@duris.test", 0, "2001:db8:1:2::6") ==
			account_recovery_request_outcome::host_limited,
		"the sixth address in one /64 was not limited");
	require(request("Lima7", "lima@duris.test", 0, "2001:db8:1:3::1") ==
			account_recovery_request_outcome::queued,
		"a neighbouring /64 was limited");
	wait_worker_idle();
	require(account_recovery_health_copy().host_limited == 2, "host limits were not counted");
	passed("k_host_window");
}

void section_l_capacity_erases_entry()
{
	fresh();
	mode = send_mode::block;
	std::vector<std::string> names;
	for (unsigned index = 0; index <= MAIL_SENDER_MAX_PENDING + 1; ++index)
	{
		char text[32];
		snprintf(text, sizeof text, "queue%04u", index);
		names.push_back(text);
	}
	require(request(names[0], "queue@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"the first request was not queued");
	/* Once the worker holds the first job, exactly MAIL_SENDER_MAX_PENDING more fit. */
	require(wait_until([] { return mail_sender_health_copy().inflight == 1; }, 5000),
		"the worker never picked up the blocking job");
	for (size_t index = 1; index <= MAIL_SENDER_MAX_PENDING; ++index)
		require(request(names[index], "queue@duris.test", 0, next_host()) ==
				account_recovery_request_outcome::queued,
			"a request inside the queue capacity was refused");
	require(mail_sender_health_copy().queued == MAIL_SENDER_MAX_PENDING,
		"the queue does not hold its declared capacity");
	const std::string &overflow = names[MAIL_SENDER_MAX_PENDING + 1];
	require(request(overflow, "queue@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::capacity,
		"a request against a full queue was not reported as capacity");
	account_recovery_health health = account_recovery_health_copy();
	require(health.capacity == 1 && health.entries == MAIL_SENDER_MAX_PENDING + 1,
		"the capacity outcome left an entry behind");
	/* No entry, so no cooldown: the immediate re-request is capacity again, not suppressed. */
	require(request(overflow, "queue@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::capacity,
		"the erased entry's cooldown survived");
	health = account_recovery_health_copy();
	require(health.capacity == 2 && health.entries == MAIL_SENDER_MAX_PENDING + 1,
		"the re-request changed the store");
	release_worker();
	wait_worker_idle();
	require(wait_until(
			[]
			{
				account_recovery_pulse();
				return account_recovery_health_copy().mail_sent ==
				       MAIL_SENDER_MAX_PENDING + 1;
			},
			5000),
		"not every queued mail completed once the worker was released");
	passed("l_capacity_erases_entry");
}

void section_m_pulse_outcomes()
{
	fresh();
	mode = send_mode::terminal;
	require(request("Mike", "mike@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Mike was not queued");
	wait_worker_idle();
	account_recovery_pulse();
	account_recovery_health health = account_recovery_health_copy();
	require(health.mail_terminal == 1 && health.entries == 0,
		"a terminal failure did not erase the entry");
	/* Erased means no tombstone: the player can ask again at once. */
	require(request("Mike", "mike@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"a failed send left the cooldown in place");
	wait_worker_idle();
	account_recovery_pulse();
	health = account_recovery_health_copy();
	require(health.mail_terminal == 2 && health.entries == 0,
		"the second failure was not erased");

	mode = send_mode::retryable;
	require(request("November", "november@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"November was not queued");
	wait_worker_idle();
	account_recovery_pulse();
	health = account_recovery_health_copy();
	require(health.mail_retryable == 1 && health.entries == 0,
		"a retryable failure did not erase the entry");

	mode = send_mode::sent;
	require(request("Oscar", "oscar@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Oscar was not queued");
	wait_worker_idle();
	account_recovery_pulse();
	health = account_recovery_health_copy();
	require(health.mail_sent == 1 && health.entries == 1, "a sent mail erased its entry");

	require(request("Papa", "papa@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Papa was not queued");
	account_recovery_forget("Papa");
	wait_worker_idle();
	account_recovery_pulse();
	health = account_recovery_health_copy();
	require(health.mail_stale == 1 && health.mail_sent == 2 && health.entries == 1,
		"a completion for a forgotten entry was not counted as stale");
	passed("m_pulse_outcomes");
}

void section_n_invalidate_and_forget()
{
	fresh();
	require(request("Quebec", "quebec@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"Quebec was not queued");
	wait_worker_idle();
	const std::string code = code_from(captured_at(0).message);
	account_recovery_invalidate("Quebec");
	require(check("Quebec", code) == account_recovery_check_outcome::rejected,
		"an invalidated token still checks");
	require(request("Quebec", "quebec@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::suppressed,
		"invalidate dropped the cooldown tombstone");
	require(captured_count() == 1, "a re-request after invalidate mailed inside the cooldown");
	account_recovery_forget("Quebec");
	require(request("Quebec", "quebec@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"forget did not free the name at once");
	wait_worker_idle();
	const account_recovery_health health = account_recovery_health_copy();
	require(health.invalidated == 1 && health.forgotten == 1 && captured_count() == 2,
		"invalidate/forget counters do not match");
	passed("n_invalidate_and_forget");
}

void section_o_store_cap_and_host_slots()
{
	fresh();
	std::vector<std::string> names;
	for (unsigned index = 0; index <= ACCOUNT_RECOVERY_MAX_ENTRIES; ++index)
	{
		char text[32];
		snprintf(text, sizeof text, "cap%04u", index);
		names.push_back(text);
	}
	/* The first entry is issued one second earlier so "oldest" is unambiguous. */
	require(request(names[0], "cap@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"the first capped-store request was not queued");
	fake_now += 1;
	for (size_t index = 1; index <= ACCOUNT_RECOVERY_MAX_ENTRIES; ++index)
	{
		require(request(names[index], "cap@duris.test", 0, next_host()) ==
				account_recovery_request_outcome::queued,
			"a distinct name was refused while filling the store");
		/* Keep the job and completion queues well under their caps so a capacity
		 * refusal can never masquerade as (or hide) an eviction. */
		if (index % 64 == 0)
			settle();
	}
	settle();
	account_recovery_health health = account_recovery_health_copy();
	require(health.entries == ACCOUNT_RECOVERY_MAX_ENTRIES, "the store exceeded its cap");
	require(health.evicted_live == 1, "filling the store did not evict exactly one live token");
	/* Every one of the 1025 hosts was distinct, so the last one recycled a live slot. */
	require(health.host_slots_recycled == 1, "the 1025th host did not recycle a slot");
	/* An evicted entry takes its cooldown with it: the name is free at once. */
	require(request(names[0], "cap@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::queued,
		"the evicted name still carries a cooldown");
	settle();
	health = account_recovery_health_copy();
	require(health.entries == ACCOUNT_RECOVERY_MAX_ENTRIES && health.evicted_live == 2 &&
			health.host_slots_recycled == 2,
		"re-requesting the evicted name did not evict and recycle once more");
	passed("o_store_cap_and_host_slots");
}

void section_p_shutdown_join()
{
	fresh();
	mode = send_mode::block;
	const char *names[] = { "Romeo1", "Romeo2", "Romeo3" };
	for (const char *name : names)
		require(request(name, "romeo@duris.test", 0, next_host()) ==
				account_recovery_request_outcome::queued,
			"a pre-shutdown request was not queued");
	require(wait_until(
			[]
			{
				const mail_sender_health health = mail_sender_health_copy();
				return health.inflight == 1 && health.queued == 2;
			},
			5000),
		"the worker did not settle with one job in flight and two queued");
	const auto started = std::chrono::steady_clock::now();
	account_recovery_shutdown();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				     std::chrono::steady_clock::now() - started)
				     .count();
	require(elapsed < 2000, "shutdown did not join the blocked worker within 2 s");
	const mail_sender_health health = mail_sender_health_copy();
	require(health.dropped_at_shutdown == 2,
		"the two queued jobs were not dropped at shutdown");
	require(!health.running && !health.stop_pending, "sender still reports running");
	require(captured_count() == 1, "a queued job reached the send function after stop");
	require(!account_recovery_enabled(), "core reports enabled after shutdown");
	require(request("Romeo4", "romeo@duris.test", 0, next_host()) ==
			account_recovery_request_outcome::disabled,
		"a request after shutdown was not reported as disabled");
	passed("p_shutdown_join");
}

void section_q_log_hygiene()
{
	account_recovery_reset_for_tests();
	std::ifstream file(log_path);
	const std::string log((std::istreambuf_iterator<char>(file)),
			      std::istreambuf_iterator<char>());
	require(!log.empty(), "the log stubs rendered nothing");
	require(log.find("account recovery request=") != std::string::npos,
		"rendered request lines are missing from the log");
	require(log.find("(account=redacted)") != std::string::npos, "redaction marker missing");
	std::vector<captured_job> corpus;
	{
		std::lock_guard<std::mutex> lock(capture_mutex);
		corpus = all_captured;
	}
	require(!corpus.empty(), "no mail was captured across the run");
	for (const captured_job &job : corpus)
	{
		const std::string code = code_from(job.message);
		require(log.find(code) == std::string::npos &&
				log.find(dashed(code)) == std::string::npos,
			"a reset code reached the log");
		require(log.find(job.to) == std::string::npos,
			"a recipient address reached the log");
	}
	for (const std::string &email : emails_used)
		require(log.find(email) == std::string::npos, "an email address reached the log");
	for (const std::string &host : hosts_used)
		require(log.find(host) == std::string::npos, "a host reached the log");
	for (const std::string &name : names_used)
		require(log.find(name) == std::string::npos, "an account name reached the log");
	passed("q_log_hygiene");
}
} // namespace

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "usage: account_recovery_harness <log-file>\n";
		return 2;
	}
	log_path = argv[1];
	{
		std::ofstream truncate(log_path, std::ios::trunc);
		if (!truncate)
		{
			std::cerr << "cannot open the log file\n";
			return 2;
		}
	}

	section_fingerprint();
	section_classify();
	section_render();
	section_canonical_name();
	section_a_request_renders_mail();
	section_b_suppressed_tombstone();
	section_c_canonical_keying();
	section_d_check_normalises_without_consuming();
	section_e_token_attempt_cap();
	section_f_two_phase_complete();
	section_g_fenced_and_superseded();
	section_h_failures_keep_token();
	section_i_expiry_and_sweep();
	section_j_latest_wins();
	section_k_host_window();
	section_l_capacity_erases_entry();
	section_m_pulse_outcomes();
	section_n_invalidate_and_forget();
	section_o_store_cap_and_host_slots();
	section_p_shutdown_join();
	section_q_log_hygiene();

	account_recovery_reset_for_tests();
	std::cout << "account recovery harness passed\n";
	return 0;
}
