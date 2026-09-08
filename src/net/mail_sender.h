#pragma once

/*
 * Best-effort SMTP sender for account recovery mail (libcurl, one worker thread).
 *
 * The worker receives immutable DTOs and never touches engine state; the main
 * thread submits rendered messages and pulls completions.  Not curl.h/mail.h on
 * purpose: src/ is on the include path and would shadow system headers.  No
 * engine includes, so the live harness links this module alone.
 */

#include <cstddef>
#include <cstdint>
#include <string>

struct mail_sender_config
{
	std::string host;
	long port = 587;
	bool tls = true;
	std::string username; /* both or neither; require tls */
	std::string password;
	std::string from;
	std::string from_domain; /* text after the at-sign in from; Message-ID right-hand side */
	long connect_timeout_ms = 10000;
	long total_timeout_ms = 20000;
};

struct mail_job
{
	uint64_t request_id = 0;
	std::string to;
	std::string message; /* fully rendered RFC 5322 text, CRLF; cleansed after send */
};

enum class mail_outcome : uint8_t
{
	sent,
	retryable_failure,
	terminal_failure
};

struct mail_result
{
	uint64_t request_id = 0;
	mail_outcome outcome = mail_outcome::terminal_failure;
	int curl_code = 0; /* CURLcode */
	long smtp_code = 0; /* CURLINFO_RESPONSE_CODE */
};

enum class mail_submit_outcome : uint8_t
{
	accepted,
	capacity_exceeded,
	invalid,
	unavailable
};

struct mail_sender_health
{
	bool running = false;
	bool stop_pending = false;
	uint64_t queued = 0, inflight = 0, submitted = 0, sent = 0, retryable_failures = 0,
		 terminal_failures = 0, dropped_capacity = 0, dropped_at_shutdown = 0,
		 high_water = 0;
	int last_curl_code = 0;
	long last_smtp_code = 0;
};

using mail_send_fn = mail_result (*)(const mail_job &, const mail_sender_config &, void *);

constexpr size_t MAIL_SENDER_MAX_PENDING = 256;
constexpr size_t MAIL_SENDER_MAX_COMPLETIONS = 256;

/* Boot-time snapshot from MAIL_* (getenv + libcurl URL parser only; never logs; the
 * category string never carries a value). */
bool mail_sender_config_from_env(mail_sender_config *out, const char **rejected_category);
/* Pure structural address check: the only address rule mail_sender.c itself applies. */
bool mail_sender_address_is_safe(const char *address);
/* Pure: RFC 5322 headers + body, CRLF, 7-bit.  False on unsafe addresses, CR/LF in the subject,
 * bare CR/LF or 8-bit bytes in the body, or a RAND_bytes failure. */
bool mail_sender_render_message(const std::string &to, const std::string &from,
				const std::string &from_domain, const std::string &subject,
				const std::string &body, std::string *out);
/* Pure: outcome from (CURLcode, CURLINFO_RESPONSE_CODE); the SMTP class decides first when non-zero. */
mail_outcome mail_sender_classify(int curl_code, long smtp_code);

bool mail_sender_init(const mail_sender_config &config, mail_send_fn send = nullptr,
		      void *context = nullptr);
bool mail_sender_running(void);
uint64_t mail_sender_next_request_id(void); /* never 0 */
mail_submit_outcome mail_sender_submit(uint64_t request_id, const std::string &to,
				       const std::string &subject, const std::string &body);
size_t mail_sender_pulse(mail_result *results_out, size_t capacity);
void mail_sender_shutdown(void);
mail_sender_health mail_sender_health_copy(void);
void mail_sender_reset_for_tests(void);

/* Worker-side libcurl send; exported so the live harness can call it directly. */
mail_result mail_sender_send_libcurl(const mail_job &job, const mail_sender_config &config,
				     void *unused);
