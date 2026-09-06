/*
 * Drives src/net/mail_sender.c's REAL libcurl SMTP path against a fake relay on
 * 127.0.0.1 (tests/async/test_account_recovery_smtp_live.py).  Links mail_sender.c alone:
 * no engine header, no stub.  The marker code goes to the file named on the command line
 * and never to stdout; the driver reads it back and looks for it in the relay's captured
 * DATA, and for its absence in everything this process printed.
 *
 *   mail_sender_live_harness <host> <port> <from> <to> <code-file> <section> [total-ms]
 *
 *   direct  render a message and call mail_sender_send_libcurl once
 *   worker  init / submit / pulse / shutdown round trip through the worker thread
 *   crlf    a recipient carrying CRLF: render refuses, send_libcurl refuses before libcurl
 *
 * Every section prints "outcome=<n> curl=<n> smtp=<n>" (outcome: 0 sent, 1 retryable,
 * 2 terminal) and "ok: <section>"; the optional total timeout lets the stall case finish
 * in seconds while proving the join bound follows CURLOPT_TIMEOUT_MS.
 */

#include "net/mail_sender.h"

#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace
{
constexpr char reset_subject[] = "Duris account password reset";

void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		mail_sender_shutdown(); /* a joinable worker at exit would std::terminate */
		exit(1);
	}
}

void passed(const char *section)
{
	std::cout << "ok: " << section << '\n';
}

long parse_long(const char *text, const char *what)
{
	errno = 0;
	char *end = nullptr;
	const long value = strtol(text, &end, 10);
	require(*text && errno == 0 && end != text && !*end && value > 0, what);
	return value;
}

/* 32 lowercase hex digits from RAND_bytes, the same shape the core issues. */
std::string make_code()
{
	unsigned char random_bytes[16];
	require(RAND_bytes(random_bytes, sizeof random_bytes) == 1, "RAND_bytes failed");
	static const char hex[] = "0123456789abcdef";
	std::string code;
	for (unsigned char byte : random_bytes)
	{
		code.push_back(hex[byte >> 4]);
		code.push_back(hex[byte & 0x0f]);
	}
	OPENSSL_cleanse(random_bytes, sizeof random_bytes);
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

/* The core's body shape: fixed prose, the code in 8-8-8-8 groups, no line starting '.'. */
std::string render_body(const std::string &dashed_code)
{
	std::string body =
		"Someone asked to reset the password of the Duris account Player.\r\n\r\n";
	body += "Reset code: " + dashed_code + "\r\n\r\n";
	body += "Type it at the prompt within 15 minutes. It works once.\r\n";
	body += "If you did not request this, ignore this message; nothing changes.\r\n";
	return body;
}

void print_result(const mail_result &result)
{
	std::cout << "outcome=" << static_cast<int>(result.outcome) << " curl=" << result.curl_code
		  << " smtp=" << result.smtp_code << '\n';
}
} // namespace

int main(int argc, char **argv)
{
	if (argc < 7)
	{
		std::cerr
			<< "usage: mail_sender_live_harness <host> <port> <from> <to> <code-file> "
			   "<section> [total-timeout-ms]\n";
		return 2;
	}

	mail_sender_config config;
	config.host = argv[1];
	config.port = parse_long(argv[2], "port is not a positive integer");
	config.tls = false; /* loopback only, as the config rule permits */
	config.from = argv[3];
	const size_t at = config.from.find('@');
	require(at != std::string::npos && at + 1 < config.from.size(), "from carries no domain");
	config.from_domain = config.from.substr(at + 1);
	config.total_timeout_ms = argc > 7 ? parse_long(argv[7], "timeout is not positive") : 20000;
	config.connect_timeout_ms = std::min(10000L, config.total_timeout_ms);

	const std::string to = argv[4];
	const std::string section = argv[6];
	const std::string code = make_code();
	{
		std::ofstream file(argv[5], std::ios::trunc);
		file << code << '\n';
		require(file.good(), "cannot write the code file");
	}
	std::string body = render_body(dashed(code));

	if (section == "direct")
	{
		mail_job job;
		job.request_id = 1;
		job.to = to;
		require(mail_sender_render_message(to, config.from, config.from_domain,
						   reset_subject, body, &job.message),
			"render refused a well-formed message");
		const mail_result result = mail_sender_send_libcurl(job, config, nullptr);
		OPENSSL_cleanse(job.message.data(), job.message.size());
		print_result(result);
		passed("direct");
	}
	else if (section == "worker")
	{
		require(mail_sender_init(config), "mail sender did not start");
		const uint64_t request_id = mail_sender_next_request_id();
		require(mail_sender_submit(request_id, to, reset_subject, body) ==
				mail_submit_outcome::accepted,
			"submit was not accepted");
		mail_result results[4];
		size_t count = 0;
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
		while (count == 0 && std::chrono::steady_clock::now() < deadline)
		{
			count = mail_sender_pulse(results, 4);
			if (!count)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		require(count == 1, "the worker produced no completion within 30 s");
		require(results[0].request_id == request_id,
			"completion carries the wrong request id");
		print_result(results[0]);
		const mail_sender_health health = mail_sender_health_copy();
		std::cout << "sent=" << health.sent << " retryable=" << health.retryable_failures
			  << " terminal=" << health.terminal_failures << '\n';
		mail_sender_shutdown();
		require(!mail_sender_running(), "sender still running after shutdown");
		passed("worker");
	}
	else if (section == "crlf")
	{
		const std::string smuggled = to + "\r\nRCPT TO:<other@duris.test>";
		std::string rendered;
		require(!mail_sender_render_message(smuggled, config.from, config.from_domain,
						    reset_subject, body, &rendered),
			"render accepted a recipient carrying CRLF");
		/* A sound message behind a poisoned envelope recipient: the belt in the worker
		 * path must refuse it before libcurl is ever initialised. */
		mail_job job;
		job.request_id = 2;
		job.to = smuggled;
		require(mail_sender_render_message(to, config.from, config.from_domain,
						   reset_subject, body, &job.message),
			"render refused a well-formed message");
		const mail_result result = mail_sender_send_libcurl(job, config, nullptr);
		OPENSSL_cleanse(job.message.data(), job.message.size());
		print_result(result);
		require(result.outcome == mail_outcome::terminal_failure && result.curl_code == 0,
			"a CRLF recipient must be refused with curl_code 0");
		passed("crlf");
	}
	else
	{
		std::cerr << "unknown section\n";
		OPENSSL_cleanse(body.data(), body.size());
		return 2;
	}

	OPENSSL_cleanse(body.data(), body.size());
	return 0;
}
