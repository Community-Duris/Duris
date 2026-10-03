#include "net/network_readiness.h"
#include "net/network_wakeup.h"
#include "net/websocket.h"
#include "net/command_latency.h"
#include "core/utils.h"
#include "net/telnet.h"
#include "net/unicode.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <vector>

P_desc descriptor_list = nullptr;
P_desc next_to_process = nullptr;
int used_descs = 0;
int shutdownflag = 0;
static int tls_direction = 0, handshake_calls = 0, input_calls = 0;
static uint64_t last_read_us = 0;
static int tls_receive_result = GNUTLS_E_AGAIN;
static bool close_next_on_read = false;
static std::map<int, std::string> input;
static std::vector<std::unique_ptr<descriptor_data>> owned;
struct game_loop_pulse_context
{
	int telnet_listener = -1, ssl_listener = -1, websocket_listener = -1;
	bool accept_debug = false;
	unsigned long debug_turn = 0;
	unsigned long *accept_debug_pulse = &debug_turn;
	command_latency_tracker command_latency = {};
};
static uint64_t loop_monotonic_us()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		       std::chrono::steady_clock::now().time_since_epoch())
		.count();
}
static uint64_t latency_trace_elapsed_us(uint64_t start, uint64_t end)
{
	return end - start;
}
static void logit(const char *, const char *, ...) {}
static void prepare_descriptor_latency_event(command_latency_event *, command_latency_kind, P_desc,
					     const char *)
{
}
void command_latency_record(command_latency_tracker *, const command_latency_event *, uint64_t) {}
extern "C" size_t gnutls_record_check_pending(gnutls_session_t)
{
	return 0;
}
extern "C" int gnutls_record_get_direction(gnutls_session_t)
{
	return tls_direction;
}
extern "C" const char *gnutls_strerror(int)
{
	return "injected TLS retry";
}
extern "C" ssize_t gnutls_record_recv(gnutls_session_t, void *buffer, size_t length)
{
	if (tls_receive_result < 0)
		return tls_receive_result;
	assert(length >= 4);
	memcpy(buffer, "tls\n", 4);
	return 4;
}
static void panic_corruption(const char *, const char *, ...)
{
	abort();
}
static int parse_telnet_options(P_desc, char *, int)
{
	return 0;
}
int websocket_process_input(P_desc)
{
	return NETWORK_INPUT_EOF;
}
bool websocket_input_paused(P_desc)
{
	return false;
}
static int ws_boundary_dispatches = 0;
void websocket_dispatch_pending_input(P_desc d)
{
	++ws_boundary_dispatches;
	d->ws_pending_application = nullptr;
}
static void process_line(P_desc d, char *line)
{
	input[d->descriptor] += std::string(line) + "\n";
}
static int ssl_negotiate(gnutls_session_t)
{
	++handshake_calls;
	tls_direction = handshake_calls % 2;
	return handshake_calls >= 3 ? 0 : 1;
}
static void greet(P_desc d)
{
	d->connected = CON_GET_TERM;
}
static void close_socket(P_desc d)
{
	if (next_to_process == d)
		next_to_process = d->next;
	P_desc *entry = &descriptor_list;
	while (*entry && *entry != d)
		entry = &(*entry)->next;
	if (*entry)
		*entry = d->next;
	close(d->descriptor);
	d->descriptor = -1;
	--used_descs;
}
static int telnet_flush_output(P_desc d)
{
	char bytes[16384]{};
	size_t pending = d->telnet_output_len - d->telnet_output_offset;
	ssize_t result = write(d->descriptor, bytes, std::min(pending, sizeof(bytes)));
	if (result < 0)
		return errno == EAGAIN ? 0 : -1;
	d->telnet_output_offset += result;
	return 0;
}
int websocket_flush_output(P_desc)
{
	assert(false);
	return -1;
}
void websocket_close(P_desc, int, const char *)
{
	assert(false);
}
/* PRODUCTION_INPUT_FUNCTION */

static int process_input(P_desc d)
{
	++input_calls;
	if (close_next_on_read && d->next)
	{
		close_next_on_read = false;
		close_socket(d->next);
	}
	const size_t before = input[d->descriptor].size();
	const int result = transport_process_input(d);
	if (input[d->descriptor].size() > before)
		last_read_us = loop_monotonic_us();
	return result;
}
static P_desc add_client(int descriptor)
{
	auto d = std::make_unique<descriptor_data>();
	d->descriptor = descriptor;
	d->network_input_remaining = MAX_QUEUE_LENGTH - 1;
	fcntl(descriptor, F_SETFL, O_NONBLOCK);
	d->connected = CON_GET_TERM;
	d->next = descriptor_list;
	descriptor_list = d.get();
	++used_descs;
	owned.push_back(std::move(d));
	return descriptor_list;
}
static int drain_new_connections(int listener, int, const char *)
{
	int count = 0;
	for (; count < 32; ++count)
	{
		int descriptor = accept(listener, nullptr, nullptr);
		if (descriptor < 0)
			break;
		if (used_descs >= MAX_CONNECTIONS)
			close(descriptor);
		else
			add_client(descriptor);
	}
	return count;
}

/* PRODUCTION_NETWORK_FUNCTIONS */

static void finish_connection_boundary(bool ready = true)
{
	/* PRODUCTION_CONNECTION_BOUNDARY */
}

static void clear_clients()
{
	while (descriptor_list)
		close_socket(descriptor_list);
	owned.clear();
	input.clear();
	input_calls = 0;
}
int main()
{
	signal(SIGPIPE, SIG_IGN);
	game_loop_pulse_context context;
	assert(network_wakeup_fd() >= 0);
	assert(fcntl(network_wakeup_fd(), F_GETFD) & FD_CLOEXEC);

	// Real poll wait wakes for input, including descriptors above FD_SETSIZE.
	int pair[2];
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
	int high = fcntl(pair[0], F_DUPFD_CLOEXEC, FD_SETSIZE + 32);
	assert(high >= FD_SETSIZE);
	close(pair[0]);
	P_desc d = add_client(high);
	const uint64_t started = loop_monotonic_us();
	std::atomic<uint64_t> input_published_us = 0;
	std::thread writer(
		[&]
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			input_published_us = loop_monotonic_us();
			assert(write(pair[1], "first\nsecond\n", 13) == 13);
		});
	assert(service_network_turn(context, 250));
	writer.join();
	const uint64_t wakeup = last_read_us - started;
	assert(wakeup < 100000 && input[high] == "first\nsecond\n");
	assert(input_calls == 1); // One bounded read, with command FIFO retained.
	printf("socket_wakeup_us=%llu\n", (unsigned long long)wakeup);
	printf("socket_readiness_latency_us=%llu\n",
	       (unsigned long long)(last_read_us - input_published_us));

	// With no output, a normally writable socket does not wake the idle wait.
	assert(network_write_interest(d) == 0);
	const uint64_t idle_started = loop_monotonic_us();
	const clock_t cpu_started = clock();
	assert(service_network_turn(context, 80));
	const uint64_t idle_elapsed = loop_monotonic_us() - idle_started;
	assert(idle_elapsed >= 70000 && idle_elapsed < 300000);
	assert((double)(clock() - cpu_started) / CLOCKS_PER_SEC < .03);

	// TLS handshake retries run only in GnuTLS's captured direction.
	d->sslses = (gnutls_session_t)1;
	d->connected = CON_SSLNEGO;
	d->tls_read_interest = POLLIN;
	assert(service_network_turn(context, 0));
	assert(handshake_calls == 0); // Writable alone cannot run a READ retry.
	assert(write(pair[1], "x", 1) == 1);
	assert(service_network_turn(context, 0));
	assert(handshake_calls == 1 && d->tls_read_interest == POLLOUT);
	assert(service_network_turn(context, 0));
	assert(handshake_calls == 2 && d->tls_read_interest == POLLIN);
	assert(service_network_turn(context, 0));
	assert(handshake_calls == 3 && d->connected != CON_SSLNEGO);
	// Exercise the actual GnuTLS receive retry capture, including WRITE retry.
	tls_direction = 1;
	assert(service_network_turn(context, 0));
	assert(d->tls_read_interest == POLLOUT);
	tls_direction = 0;
	assert(service_network_turn(context, 0));
	assert(d->tls_read_interest == POLLIN);
	tls_receive_result = 4;
	assert(service_network_turn(context, 0));
	assert(d->tls_read_interest == 0 && input[high] == "first\nsecond\ntls\n");
	d->sslses = nullptr;
	d->tls_read_interest = 0;
	char discard;
	assert(read(high, &discard, 1) == 1);
	clear_clients();
	close(pair[1]);
	// Early EOF cannot steal the original boundary's opportunity to handle
	// newly staged input. Teardown follows at the next connection phase.
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
	d = add_client(pair[0]);
	assert(write(pair[1], "one\ntwo\n", 8) == 8);
	assert(shutdown(pair[1], SHUT_WR) == 0);
	assert(service_network_turn(context, 0));
	assert(service_network_turn(context, 0));
	assert(d->network_close_pending == 2 && input[d->descriptor] == "one\ntwo\n");
	finish_connection_boundary(false);
	assert(d->network_close_pending == 2 && d->descriptor >= 0);
	finish_connection_boundary();
	assert(d->descriptor >= 0 && d->network_close_pending == 1);
	assert(service_network_turn(context, 0));
	finish_connection_boundary();
	assert(d->descriptor == -1 && used_descs == 0);
	clear_clients();
	close(pair[1]);
	// Previously offered input cannot delay link loss for another boundary.
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
	d = add_client(pair[0]);
	assert(write(pair[1], "offered\n", 8) == 8);
	assert(service_network_turn(context, 0));
	finish_connection_boundary();
	assert(shutdown(pair[1], SHUT_WR) == 0);
	assert(service_network_turn(context, 0) && d->network_close_pending == 1);
	finish_connection_boundary();
	assert(d->descriptor == -1 && used_descs == 0);
	clear_clients();
	close(pair[1]);
	// Buffered WebSocket application input also retains its boundary when the
	// current interval needed no socket bytes to finish the prior frame batch.
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
	d = add_client(pair[0]);
	d->websocket = 1;
	d->ws_pending_application = reinterpret_cast<websocket_pending_application *>(1);
	assert(shutdown(pair[1], SHUT_WR) == 0);
	assert(service_network_turn(context, 0));
	assert(d->network_close_pending == 2 && ws_boundary_dispatches == 0);
	finish_connection_boundary();
	assert(d->descriptor >= 0 && !d->ws_pending_application && ws_boundary_dispatches == 1);
	finish_connection_boundary();
	assert(d->descriptor == -1 && used_descs == 0);
	clear_clients();
	close(pair[1]);

	// Expiry stages teardown without a packet. The failed descriptor then stays
	// quiescent until the connection phase can apply link-loss effects.
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
	d = add_client(pair[0]);
	d->sslses = (gnutls_session_t)1;
	d->connected = CON_SSLNEGO;
	d->tls_handshake_deadline_us = loop_monotonic_us() + 20000;
	assert(service_network_turn(context, 250));
	assert(d->network_close_pending && d->descriptor >= 0 && handshake_calls == 3);
	const uint64_t teardown_started = loop_monotonic_us();
	assert(service_network_turn(context, 40));
	assert(loop_monotonic_us() - teardown_started >= 30000 && handshake_calls == 3);
	clear_clients();
	close(pair[1]);

	// Slow readers retain their bytes and do not starve another ready reader.
	int slow[2], quiet[2];
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, slow) == 0);
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, quiet) == 0);
	int small = 1024;
	assert(setsockopt(slow[0], SOL_SOCKET, SO_SNDBUF, &small, sizeof(small)) == 0);
	P_desc quiet_desc = add_client(quiet[0]);
	P_desc slow_desc = add_client(slow[0]);
	slow_desc->telnet_output_len = 1024 * 1024;
	for (int turn = 0; turn < 10; ++turn)
		assert(service_network_turn(context, 0));
	assert(network_transport_pending(slow_desc));
	slow_desc->sslses = (gnutls_session_t)1;
	slow_desc->telnet_tls_retry = 1;
	slow_desc->tls_write_interest = POLLOUT;
	assert(write(slow[1], "blocked\n", 8) == 8);
	const uint64_t blocked_started = loop_monotonic_us();
	const int blocked_reads = input_calls;
	assert(service_network_turn(context, 40));
	assert(loop_monotonic_us() - blocked_started >= 30000 && input_calls == blocked_reads);
	slow_desc->sslses = nullptr;
	slow_desc->telnet_tls_retry = 0;
	slow_desc->tls_write_interest = 0;
	assert(write(quiet[1], "quiet\n", 6) == 6);
	assert(service_network_turn(context, 0));
	assert(input[quiet_desc->descriptor] == "quiet\n");
	assert(network_transport_pending(slow_desc));

	// A perpetually ready producer receives one read before its quiet peer.
	assert(write(slow[1], "busy\n", 5) == 5);
	assert(write(quiet[1], "next\n", 5) == 5);
	const int reads_before = input_calls;
	assert(service_network_turn(context, 0));
	assert(input_calls - reads_before == 2);
	assert(input[quiet_desc->descriptor] == "quiet\nnext\n");
	// A flooding client exhausts the established per-pulse input allowance.
	// Unread kernel bytes then cannot spin poll or fill type-ahead faster than
	// the old loop; a quiet peer still receives its own bounded read.
	std::string flood(MAX_QUEUE_LENGTH * 2, '\n');
	assert(write(slow[1], flood.data(), flood.size()) == static_cast<ssize_t>(flood.size()));
	assert(service_network_turn(context, 0));
	assert(!slow_desc->network_input_remaining && network_read_interest(slow_desc) == 0);
	assert(write(quiet[1], "fair\n", 5) == 5);
	assert(service_network_turn(context, 0));
	assert(input[quiet_desc->descriptor] == "quiet\nnext\nfair\n");
	const uint64_t flood_started = loop_monotonic_us();
	assert(service_network_turn(context, 40));
	assert(loop_monotonic_us() - flood_started >= 30000);
	slow_desc->network_input_remaining = MAX_QUEUE_LENGTH - 1;
	// A reconnect handler may close the descriptor that was next in traversal.
	assert(write(slow[1], "reconnect\n", 10) == 10);
	close_next_on_read = true;
	assert(service_network_turn(context, 0));
	assert(quiet_desc->descriptor == -1 && used_descs == 1);
	clear_clients();
	close(slow[1]);
	close(quiet[1]);

	// A HUP-only event cannot spin while a TLS send waits for another direction.
	// A real pipe supplies this event independently of socket POLLERR/POLLOUT.
	assert(pipe(pair) == 0);
	d = add_client(pair[0]);
	d->sslses = (gnutls_session_t)1;
	d->telnet_output_len = 16;
	d->telnet_tls_retry = 1;
	d->tls_write_interest = POLLOUT;
	close(pair[1]);
	assert(service_network_turn(context, 0));
	assert((d->network_revents & POLLHUP) && d->network_close_pending == 1);
	const uint64_t hung_up_started = loop_monotonic_us();
	assert(service_network_turn(context, 40));
	assert(loop_monotonic_us() - hung_up_started >= 30000 && !input_calls);
	finish_connection_boundary();
	assert(used_descs == 0);
	clear_clients();

	// Preserve select's exception policy for urgent TCP data, using POLLPRI.
	int urgent_listener = socket(AF_INET, SOCK_STREAM, 0);
	assert(urgent_listener >= 0);
	sockaddr_in urgent_address{};
	urgent_address.sin_family = AF_INET;
	urgent_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	assert(bind(urgent_listener, reinterpret_cast<sockaddr *>(&urgent_address),
		    sizeof(urgent_address)) == 0);
	socklen_t urgent_length = sizeof(urgent_address);
	assert(getsockname(urgent_listener, reinterpret_cast<sockaddr *>(&urgent_address),
			   &urgent_length) == 0);
	assert(listen(urgent_listener, 1) == 0);
	int urgent_sender = socket(AF_INET, SOCK_STREAM, 0);
	assert(urgent_sender >= 0);
	assert(connect(urgent_sender, reinterpret_cast<sockaddr *>(&urgent_address),
		       sizeof(urgent_address)) == 0);
	int urgent_receiver = accept(urgent_listener, nullptr, nullptr);
	assert(urgent_receiver >= 0);
	close(urgent_listener);
	d = add_client(urgent_receiver);
	assert(send(urgent_sender, "!", 1, MSG_OOB) == 1);
	assert(service_network_turn(context, 100));
	assert((d->network_revents & POLLPRI) && d->network_close_pending == 1 && !input_calls);
	finish_connection_boundary();
	assert(used_descs == 0);
	clear_clients();
	close(urgent_sender);

	// A worker hint wakes poll promptly and coalesces a full pipe safely.
	std::atomic<bool> complete = false;
	std::atomic<uint64_t> completion_published_us = 0;
	std::thread worker(
		[&]
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			complete = true;
			completion_published_us = loop_monotonic_us();
			network_wakeup_notify();
		});
	const uint64_t completion_started = loop_monotonic_us();
	assert(service_network_turn(context, 250));
	const uint64_t completion_elapsed = loop_monotonic_us() - completion_started;
	worker.join();
	assert(complete && completion_elapsed < 100000);
	for (int index = 0; index < 20000; ++index)
		network_wakeup_notify();
	assert(service_network_turn(context, 0));
	pollfd wake{ network_wakeup_fd(), POLLIN, 0 };
	for (int turn = 0; turn < 8 && poll(&wake, 1, 0); ++turn)
		network_wakeup_drain();
	assert(poll(&wake, 1, 0) == 0);
	printf("completion_wakeup_us=%llu\n", (unsigned long long)completion_elapsed);
	printf("completion_readiness_latency_us=%llu\n",
	       (unsigned long long)(completion_started + completion_elapsed -
				    completion_published_us));

	// Readiness and completions cannot return the deadline wait early.
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
	d = add_client(pair[0]);
	const uint64_t pulse_started = loop_monotonic_us();
	std::thread between_pulses(
		[&]
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			assert(write(pair[1], "one\ntwo\n", 8) == 8);
			network_wakeup_notify();
		});
	network_wait_until(context, pulse_started + OPT_USEC);
	between_pulses.join();
	assert(last_read_us - pulse_started < 100000);
	assert(loop_monotonic_us() - pulse_started >= OPT_USEC);
	assert(input[d->descriptor] == "one\ntwo\n");
	// The caller dispatches one line and one world tick only after this wait.
	assert(network_next_pulse_us(1000000, 1010000) == 1250000);
	assert(network_next_pulse_us(1000000, 2000000) == 2250000);
	assert(network_timeout_ms(1250000, 1000000) == 250);
	assert(network_timeout_ms(1250000, 1250000) == 0);
	clear_clients();
	close(pair[1]);

	// Listener bursts exceed one turn's accept budget without losing peers.
	int listener = socket(AF_INET6, SOCK_STREAM, 0);
	assert(listener >= 0);
	sockaddr_in6 address{};
	address.sin6_family = AF_INET6;
	address.sin6_addr = in6addr_loopback;
	assert(bind(listener, (sockaddr *)&address, sizeof(address)) == 0);
	assert(listen(listener, 128) == 0);
	fcntl(listener, F_SETFL, O_NONBLOCK);
	socklen_t length = sizeof(address);
	assert(getsockname(listener, (sockaddr *)&address, &length) == 0);
	context.telnet_listener = listener;
	std::vector<int> peers;
	for (int index = 0; index < 96; ++index)
	{
		int peer = socket(AF_INET6, SOCK_STREAM, 0);
		assert(connect(peer, (sockaddr *)&address, sizeof(address)) == 0);
		peers.push_back(peer);
	}
	assert(service_network_turn(context, 0) && used_descs == 32);
	assert(service_network_turn(context, 0) && used_descs == 64);
	assert(service_network_turn(context, 0) && used_descs == 96);
	clear_clients();
	for (int peer : peers)
		close(peer);
	close(listener);
	puts("readiness latency, TLS direction, slow readers, fairness, completions, idle wait, "
	     "IPv6 bursts and simulation deadline tests passed");
}
