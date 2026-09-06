/*
 * Best-effort SMTP sender for account recovery mail: one worker thread, libcurl only.
 *
 * Everything the worker touches is a value it owns -- the boot-time config snapshot and
 * the job it popped -- so it never reads engine state.  Nothing in this file logs; the
 * caller logs the categories and integer codes returned from here.  No engine header is
 * included so the live harness links this translation unit alone with
 * -lcurl -lcrypto -pthread.
 */

#include "net/mail_sender.h"

#include <curl/curl.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <strings.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace
{
std::once_flag curl_global_once;
std::mutex sender_mutex;
std::condition_variable work_available;
std::deque<mail_job> jobs;
std::deque<mail_result> completions;
std::thread worker;
bool stop_requested = false;
mail_sender_config sender_config;
mail_send_fn send_callback = nullptr;
void *send_context = nullptr;
mail_sender_health health = {};
std::atomic<uint64_t> next_request_id = 1;

void refresh_health_locked()
{
	health.queued = jobs.size();
	health.high_water = std::max(health.high_water, health.queued + health.inflight);
}

void record_result_locked(const mail_result &result)
{
	switch (result.outcome)
	{
	case mail_outcome::sent:
		++health.sent;
		break;
	case mail_outcome::retryable_failure:
		++health.retryable_failures;
		break;
	case mail_outcome::terminal_failure:
		++health.terminal_failures;
		break;
	}
	health.last_curl_code = result.curl_code;
	health.last_smtp_code = result.smtp_code;
}

void worker_main()
{
	for (;;)
	{
		mail_job job;
		mail_send_fn send_fn = nullptr;
		void *context = nullptr;
		{
			std::unique_lock<std::mutex> lock(sender_mutex);
			work_available.wait(lock, [] { return stop_requested || !jobs.empty(); });
			/* Stop wins over the backlog: a dead relay must never stretch the shutdown
			 * join beyond the one send already in flight. */
			if (stop_requested)
				break;
			job = std::move(jobs.front());
			jobs.pop_front();
			send_fn = send_callback;
			context = send_context;
			health.inflight = 1;
			refresh_health_locked();
		}
		mail_result result = send_fn(job, sender_config, context);
		result.request_id = job.request_id;
		OPENSSL_cleanse(job.message.data(), job.message.size());
		{
			std::lock_guard<std::mutex> lock(sender_mutex);
			health.inflight = 0;
			record_result_locked(result);
			completions.push_back(result);
			refresh_health_locked();
		}
	}
}

bool address_byte_allowed(unsigned char c)
{
	if (c < 0x21 || c >= 0x7f)
		return false;
	switch (c)
	{
	case '"':
	case '<':
	case '>':
	case ',':
	case ';':
	case ':':
	case '\\':
	case '(':
	case ')':
	case '[':
	case ']':
		return false;
	default:
		return true;
	}
}

/* A std::string can carry an embedded NUL that the C-string check would stop at. */
bool address_string_is_safe(const std::string &address)
{
	return address.find('\0') == std::string::npos &&
	       mail_sender_address_is_safe(address.c_str());
}

bool domain_is_safe(const std::string &domain)
{
	if (domain.empty() || domain.size() > 253)
		return false;
	for (const char raw : domain)
	{
		const unsigned char c = static_cast<unsigned char>(raw);
		if (c == '@' || !address_byte_allowed(c))
			return false;
	}
	return true;
}

/* Header text: printable ASCII only, so no CR/LF folding, no controls, no 8-bit. */
bool header_text_is_safe(const std::string &text)
{
	for (const char raw : text)
	{
		const unsigned char c = static_cast<unsigned char>(raw);
		if (c < 0x20 || c >= 0x7f)
			return false;
	}
	return true;
}

/* Body: 7-bit, CR only as the first half of a CRLF pair, LF only as the second. */
bool body_is_safe(const std::string &body)
{
	for (size_t index = 0; index < body.size(); ++index)
	{
		const unsigned char c = static_cast<unsigned char>(body[index]);
		if (c == '\r')
		{
			if (index + 1 >= body.size() || body[index + 1] != '\n')
				return false;
			++index;
			continue;
		}
		if (c == '\n' || c >= 0x7f || (c < 0x20 && c != '\t'))
			return false;
	}
	return true;
}

struct read_cursor
{
	const std::string *message;
	size_t offset;
};

size_t read_message(char *buffer, size_t size, size_t nitems, void *userdata)
{
	read_cursor *cursor = static_cast<read_cursor *>(userdata);
	const size_t room = size * nitems;
	if (!cursor || !cursor->message || !room || cursor->offset >= cursor->message->size())
		return 0;
	const size_t count = std::min(room, cursor->message->size() - cursor->offset);
	memcpy(buffer, cursor->message->data() + cursor->offset, count);
	cursor->offset += count;
	return count;
}

bool host_is_loopback(const char *host)
{
	return !strcasecmp(host, "localhost") || !strcmp(host, "127.0.0.1") ||
	       !strcmp(host, "::1") || !strcmp(host, "[::1]");
}

/* Accept a host only when libcurl's own URL parser accepts it as a host part, so a value
 * carrying '/', '@', '?', '#' or whitespace is refused at boot rather than interpreted. */
bool host_for_url(const char *raw, std::string *url_host)
{
	const size_t length = strlen(raw);
	if (length == 0 || length > 253)
		return false;
	for (size_t index = 0; index < length; ++index)
	{
		const unsigned char c = static_cast<unsigned char>(raw[index]);
		if (c <= 0x20 || c >= 0x7f)
			return false;
	}
	std::string candidate = raw;
	/* URLs want IPv6 literals bracketed; operators write the bare form. */
	if (candidate.find(':') != std::string::npos && candidate.front() != '[')
		candidate = "[" + candidate + "]";
	CURLU *url = curl_url();
	if (!url)
		return false;
	const bool accepted = curl_url_set(url, CURLUPART_HOST, candidate.c_str(), 0) == CURLUE_OK;
	curl_url_cleanup(url);
	if (!accepted)
		return false;
	*url_host = std::move(candidate);
	return true;
}

bool credential_is_safe(const char *value)
{
	const size_t length = strlen(value);
	return length >= 1 && length <= 255 && !strchr(value, '\r') && !strchr(value, '\n');
}
} // namespace

bool mail_sender_config_from_env(mail_sender_config *out, const char **rejected_category)
{
	std::call_once(curl_global_once, [] { (void)curl_global_init(CURL_GLOBAL_DEFAULT); });
	const auto reject = [rejected_category](const char *category)
	{
		if (rejected_category)
			*rejected_category = category;
		return false;
	};
	if (!out)
		return reject("not configured");

	const char *enabled = getenv("MAIL_ENABLED");
	if (!enabled || !*enabled || !strcasecmp(enabled, "FALSE"))
		return reject("not configured");
	if (strcasecmp(enabled, "TRUE"))
		return reject("MAIL_ENABLED must be TRUE or FALSE");

	const char *host = getenv("MAIL_HOST");
	if (!host || !*host)
		return reject("MAIL_HOST missing");
	std::string url_host;
	if (!host_for_url(host, &url_host))
		return reject("MAIL_HOST invalid");

	long port = 587;
	const char *port_text = getenv("MAIL_PORT");
	if (port_text && *port_text)
	{
		errno = 0;
		char *end = nullptr;
		port = strtol(port_text, &end, 10);
		/* strtol skips leading whitespace and accepts a sign; a port is bare digits. */
		if (*port_text < '0' || *port_text > '9' || errno == ERANGE || end == port_text ||
		    *end || port < 1 || port > 65535)
			return reject("MAIL_PORT invalid");
	}

	bool tls = true;
	const char *tls_text = getenv("MAIL_TLS");
	if (tls_text && *tls_text)
	{
		if (!strcasecmp(tls_text, "TRUE"))
			tls = true;
		else if (!strcasecmp(tls_text, "FALSE"))
			tls = false;
		else
			return reject("MAIL_TLS must be TRUE or FALSE");
	}
	if (!tls && !host_is_loopback(host))
		return reject("MAIL_TLS=FALSE requires a loopback MAIL_HOST");

	const char *username = getenv("MAIL_USERNAME");
	const char *password = getenv("MAIL_PASSWORD");
	const bool have_username = username && *username;
	const bool have_password = password && *password;
	if (have_username != have_password)
		return reject("MAIL_USERNAME and MAIL_PASSWORD must be set together");
	if (have_username)
	{
		if (!tls)
			return reject("credentials require MAIL_TLS=TRUE");
		if (!credential_is_safe(username) || !credential_is_safe(password))
			return reject("MAIL_USERNAME or MAIL_PASSWORD invalid");
	}

	const char *from = getenv("MAIL_FROM");
	if (!from || !*from)
		return reject("MAIL_FROM missing");
	if (!mail_sender_address_is_safe(from))
		return reject("MAIL_FROM invalid");

	/* Copy only once every rule has passed, so no rejected path leaves a secret in a
	 * half-filled snapshot. */
	mail_sender_config parsed;
	parsed.host = std::move(url_host);
	parsed.port = port;
	parsed.tls = tls;
	if (have_username)
	{
		parsed.username = username;
		parsed.password = password;
	}
	parsed.from = from;
	parsed.from_domain = parsed.from.substr(parsed.from.find('@') + 1);
	*out = std::move(parsed);
	return true;
}

bool mail_sender_address_is_safe(const char *address)
{
	if (!address)
		return false;
	const size_t length = strlen(address);
	if (length == 0 || length > 320)
		return false;
	size_t at = length;
	bool dot_after_at = false;
	for (size_t index = 0; index < length; ++index)
	{
		const unsigned char c = static_cast<unsigned char>(address[index]);
		if (!address_byte_allowed(c))
			return false;
		if (c == '@')
		{
			if (at != length)
				return false;
			at = index;
		}
		else if (c == '.' && at != length)
			dot_after_at = true;
	}
	return at != length && at > 0 && at + 1 < length && dot_after_at;
}

bool mail_sender_render_message(const std::string &to, const std::string &from,
				const std::string &from_domain, const std::string &subject,
				const std::string &body, std::string *out)
{
	if (!out)
		return false;
	out->clear();
	if (!address_string_is_safe(to) || !address_string_is_safe(from) ||
	    !domain_is_safe(from_domain) || !header_text_is_safe(subject) || !body_is_safe(body))
		return false;

	unsigned char entropy[16] = {};
	if (RAND_bytes(entropy, sizeof entropy) != 1)
		return false;
	static const char hex[] = "0123456789abcdef";
	char message_id[sizeof entropy * 2 + 1] = {};
	for (size_t index = 0; index < sizeof entropy; ++index)
	{
		message_id[index * 2] = hex[entropy[index] >> 4];
		message_id[index * 2 + 1] = hex[entropy[index] & 0x0f];
	}

	const time_t now = time(nullptr);
	struct tm utc = {};
	char date[64] = {};
	if (!gmtime_r(&now, &utc) ||
	    strftime(date, sizeof date, "%a, %d %b %Y %H:%M:%S +0000", &utc) == 0)
		return false;

	std::string rendered;
	/* Reserve everything up front: a reallocation mid-append would leave a copy of
	 * the code (or the address) in a heap block nothing cleanses. */
	rendered.reserve(320 + from.size() + to.size() + from_domain.size() + subject.size() +
			 body.size());
	rendered.append("Date: ").append(date).append("\r\n");
	rendered.append("From: ").append(from).append("\r\n");
	rendered.append("To: ").append(to).append("\r\n");
	rendered.append("Subject: ").append(subject).append("\r\n");
	rendered.append("Message-ID: <").append(message_id).append("@").append(from_domain);
	rendered.append(">\r\n");
	rendered.append("MIME-Version: 1.0\r\n");
	rendered.append("Content-Type: text/plain; charset=utf-8\r\n");
	rendered.append("Content-Transfer-Encoding: 7bit\r\n");
	rendered.append("\r\n");
	rendered.append(body);
	if (!body.empty() && (body.size() < 2 || body.compare(body.size() - 2, 2, "\r\n") != 0))
		rendered.append("\r\n");
	*out = std::move(rendered);
	return true;
}

mail_outcome mail_sender_classify(int curl_code, long smtp_code)
{
	/* The SMTP class decides first: libcurl reports an RCPT 550 as CURLE_SEND_ERROR and a
	 * post-DATA 451 as CURLE_WEIRD_SERVER_REPLY, so the curl code alone would misfile both. */
	if (smtp_code != 0)
	{
		if (smtp_code >= 500 && smtp_code <= 599)
			return mail_outcome::terminal_failure;
		if (smtp_code >= 400 && smtp_code <= 499)
			return mail_outcome::retryable_failure;
		if (smtp_code >= 200 && smtp_code <= 399 && curl_code == CURLE_OK)
			return mail_outcome::sent;
		return mail_outcome::terminal_failure;
	}
	switch (curl_code)
	{
	case CURLE_OK:
		return mail_outcome::sent;
	case CURLE_COULDNT_RESOLVE_HOST:
	case CURLE_COULDNT_RESOLVE_PROXY:
	case CURLE_COULDNT_CONNECT:
	case CURLE_OPERATION_TIMEDOUT:
	case CURLE_RECV_ERROR:
	case CURLE_SEND_ERROR:
	case CURLE_WEIRD_SERVER_REPLY:
	case CURLE_GOT_NOTHING:
		return mail_outcome::retryable_failure;
	default:
		return mail_outcome::terminal_failure;
	}
}

bool mail_sender_init(const mail_sender_config &config, mail_send_fn send_fn, void *context)
{
	std::call_once(curl_global_once, [] { (void)curl_global_init(CURL_GLOBAL_DEFAULT); });
	std::lock_guard<std::mutex> lock(sender_mutex);
	if (health.running || worker.joinable())
		return false;
	sender_config = config;
	send_callback = send_fn ? send_fn : mail_sender_send_libcurl;
	send_context = context;
	stop_requested = false;
	health.running = true;
	health.stop_pending = false;
	health.inflight = 0;
	try
	{
		worker = std::thread(worker_main);
	}
	catch (...)
	{
		health.running = false;
		send_callback = nullptr;
		send_context = nullptr;
		return false;
	}
	refresh_health_locked();
	return true;
}

bool mail_sender_running(void)
{
	std::lock_guard<std::mutex> lock(sender_mutex);
	return health.running && !stop_requested;
}

uint64_t mail_sender_next_request_id(void)
{
	uint64_t request_id = 0;
	do
		request_id = next_request_id.fetch_add(1, std::memory_order_relaxed);
	while (!request_id);
	return request_id;
}

mail_submit_outcome mail_sender_submit(uint64_t request_id, const std::string &to,
				       const std::string &subject, const std::string &body)
{
	std::string from;
	std::string from_domain;
	{
		std::lock_guard<std::mutex> lock(sender_mutex);
		if (!health.running || stop_requested)
			return mail_submit_outcome::unavailable;
		from = sender_config.from;
		from_domain = sender_config.from_domain;
	}
	/* 0 is the "nothing queued" sentinel on the account side; a completion carrying it
	 * could never be routed back to its entry. */
	if (!request_id)
		return mail_submit_outcome::invalid;
	mail_job job;
	job.request_id = request_id;
	job.to = to;
	if (!mail_sender_render_message(to, from, from_domain, subject, body, &job.message))
	{
		OPENSSL_cleanse(job.message.data(), job.message.size());
		return mail_submit_outcome::invalid;
	}
	std::lock_guard<std::mutex> lock(sender_mutex);
	if (!health.running || stop_requested)
	{
		OPENSSL_cleanse(job.message.data(), job.message.size());
		return mail_submit_outcome::unavailable;
	}
	if (jobs.size() >= MAIL_SENDER_MAX_PENDING ||
	    completions.size() >= MAIL_SENDER_MAX_COMPLETIONS)
	{
		++health.dropped_capacity;
		OPENSSL_cleanse(job.message.data(), job.message.size());
		return mail_submit_outcome::capacity_exceeded;
	}
	++health.submitted;
	jobs.push_back(std::move(job));
	refresh_health_locked();
	work_available.notify_one();
	return mail_submit_outcome::accepted;
}

size_t mail_sender_pulse(mail_result *results_out, size_t capacity)
{
	if (!results_out || !capacity)
		return 0;
	std::lock_guard<std::mutex> lock(sender_mutex);
	size_t count = 0;
	while (count < capacity && !completions.empty())
	{
		results_out[count++] = completions.front();
		completions.pop_front();
	}
	refresh_health_locked();
	return count;
}

void mail_sender_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(sender_mutex);
		if (!health.running && !worker.joinable())
			return;
		stop_requested = true;
		health.stop_pending = true;
		/* Queued messages carry plaintext codes: count them, scrub them and drop them
		 * before waking the worker, so it exits on the flag with nothing to drain. */
		health.dropped_at_shutdown += jobs.size();
		for (mail_job &job : jobs)
			OPENSSL_cleanse(job.message.data(), job.message.size());
		jobs.clear();
		work_available.notify_all();
	}
	if (worker.joinable())
		worker.join();
	std::lock_guard<std::mutex> lock(sender_mutex);
	/* The worker is gone, so a completion landing here has no reader; a later init must
	 * not hand a stale result to a request id it never issued. */
	completions.clear();
	health.running = false;
	health.stop_pending = false;
	health.inflight = 0;
	refresh_health_locked();
}

mail_sender_health mail_sender_health_copy(void)
{
	std::lock_guard<std::mutex> lock(sender_mutex);
	refresh_health_locked();
	return health;
}

void mail_sender_reset_for_tests(void)
{
	mail_sender_shutdown();
	std::lock_guard<std::mutex> lock(sender_mutex);
	for (mail_job &job : jobs)
		OPENSSL_cleanse(job.message.data(), job.message.size());
	jobs.clear();
	completions.clear();
	stop_requested = false;
	OPENSSL_cleanse(sender_config.password.data(), sender_config.password.size());
	sender_config = mail_sender_config();
	send_callback = nullptr;
	send_context = nullptr;
	health = mail_sender_health();
	next_request_id = 1;
}

mail_result mail_sender_send_libcurl(const mail_job &job, const mail_sender_config &config, void *)
{
	std::call_once(curl_global_once, [] { (void)curl_global_init(CURL_GLOBAL_DEFAULT); });
	mail_result result = {};
	result.request_id = job.request_id;
	result.outcome = mail_outcome::terminal_failure;
	/* Belt under the main-thread render: an address that could split a command or a
	 * header never reaches the wire, and libcurl is never even initialised for it. */
	if (!address_string_is_safe(job.to) || !address_string_is_safe(config.from))
		return result;

	CURLU *url = curl_url();
	if (!url)
	{
		result.curl_code = CURLE_OUT_OF_MEMORY;
		return result;
	}
	const std::string port = std::to_string(config.port);
	const bool url_ok =
		curl_url_set(url, CURLUPART_SCHEME, config.port == 465 ? "smtps" : "smtp", 0) ==
			CURLUE_OK &&
		curl_url_set(url, CURLUPART_HOST, config.host.c_str(), 0) == CURLUE_OK &&
		curl_url_set(url, CURLUPART_PORT, port.c_str(), 0) == CURLUE_OK;
	if (!url_ok)
	{
		curl_url_cleanup(url);
		result.curl_code = CURLE_URL_MALFORMAT;
		return result;
	}

	CURL *easy = curl_easy_init();
	if (!easy)
	{
		curl_url_cleanup(url);
		result.curl_code = CURLE_FAILED_INIT;
		return result;
	}
	struct curl_slist *recipients = curl_slist_append(nullptr, job.to.c_str());
	if (!recipients)
	{
		curl_easy_cleanup(easy);
		curl_url_cleanup(url);
		result.curl_code = CURLE_OUT_OF_MEMORY;
		return result;
	}

	read_cursor cursor = { &job.message, 0 };
	const curl_read_callback reader = read_message;
	CURLcode setup = CURLE_OK;
	const auto apply = [&setup](CURLcode code)
	{
		if (setup == CURLE_OK)
			setup = code;
	};
	apply(curl_easy_setopt(easy, CURLOPT_CURLU, url));
	apply(curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "smtp,smtps"));
	if (config.tls)
		apply(curl_easy_setopt(easy, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_ALL)));
	if (!config.username.empty() && !config.password.empty())
	{
		apply(curl_easy_setopt(easy, CURLOPT_USERNAME, config.username.c_str()));
		apply(curl_easy_setopt(easy, CURLOPT_PASSWORD, config.password.c_str()));
	}
	apply(curl_easy_setopt(easy, CURLOPT_MAIL_FROM, config.from.c_str()));
	apply(curl_easy_setopt(easy, CURLOPT_MAIL_RCPT, recipients));
	apply(curl_easy_setopt(easy, CURLOPT_UPLOAD, 1L));
	apply(curl_easy_setopt(easy, CURLOPT_READFUNCTION, reader));
	apply(curl_easy_setopt(easy, CURLOPT_READDATA, static_cast<void *>(&cursor)));
	apply(curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, config.connect_timeout_ms));
	apply(curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, config.total_timeout_ms));
	apply(curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L));

	long response = 0;
	if (setup == CURLE_OK)
	{
		const CURLcode performed = curl_easy_perform(easy);
		if (curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response) != CURLE_OK)
			response = 0;
		result.curl_code = performed;
		result.smtp_code = response;
		result.outcome = mail_sender_classify(performed, response);
	}
	else
		result.curl_code = setup;

	curl_slist_free_all(recipients);
	curl_easy_cleanup(easy);
	curl_url_cleanup(url);
	return result;
}
