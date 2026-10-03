#include "net/transport_protocol.h"
#include "core/utils.h"
#include "account/account.h"
#include <cassert>
#include <climits>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <sys/select.h>
#include <sys/wait.h>

using namespace duris_transport;

bool is_world = true;
unsigned long long ne_event_tick = 100;
struct index_data *mob_index = nullptr;
channel ipc_link;
void nonblock(int fd)
{
	assert(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == 0);
}
/* PRODUCTION_SESSION_ELIGIBILITY */
/* PRODUCTION_WORLD_CONFIGURE */

static void inherited_high_descriptor()
{
	int fds[2];
	assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, fds) == 0);
	const pid_t child = fork();
	assert(child >= 0);
	if (!child)
	{
		const int high_fd = fcntl(fds[0], F_DUPFD, FD_SETSIZE + 32);
		assert(high_fd >= FD_SETSIZE);
		const std::string descriptor = std::to_string(high_fd);
		const std::string parent = std::to_string(getppid());
		assert(setenv("DURIS_TRANSPORT_FD", descriptor.c_str(), 1) == 0);
		assert(setenv("DURIS_TRANSPORT_PARENT", parent.c_str(), 1) == 0);
		is_world = false;
		assert(transport_world_configure() && is_world && ipc_link.fd == high_fd);
		_exit(0);
	}
	int status = 0;
	assert(waitpid(child, &status, 0) == child);
	assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
	close(fds[0]);
	close(fds[1]);
}

static void session_eligibility()
{
	descriptor_data descriptor{};
	char_data character{};
	pc_only_data player{};
	acct_entry account{};
	char name[] = "Fixture", account_name[] = "fixture";
	character.only.pc = &player;
	character.player.name = name;
	SET_POS(&character, POS_STANDING + STAT_NORMAL);
	player.pid = 42;
	account.acct_name = account_name;
	descriptor.descriptor = 1;
	descriptor.transport_session = 9;
	descriptor.connected = CON_PLAYING;
	descriptor.character = &character;
	descriptor.account = &account;
	assert(transport_descriptor_eligible(&descriptor));
	// Authentication can reload an account while an existing body is playing.
	descriptor.account_request = reinterpret_cast<account_request *>(1);
	assert(!transport_descriptor_eligible(&descriptor));
	descriptor.account_request = nullptr;
	descriptor.password_request = reinterpret_cast<password_request *>(1);
	assert(!transport_descriptor_eligible(&descriptor));
	descriptor.password_request = nullptr;
	descriptor.player_load_request_id = 1;
	assert(!transport_descriptor_eligible(&descriptor));
	descriptor.player_load_request_id = 0;
	descriptor.network_close_pending = 1;
	assert(!transport_descriptor_eligible(&descriptor));
	descriptor.network_close_pending = 0;
	// Unstarted client work is replayable; unowned or partially expanded work is not.
	txt_block input{};
	descriptor.input.head = &input;
	assert(!transport_descriptor_eligible(&descriptor));
	descriptor.transport_command = 1;
	assert(transport_descriptor_eligible(&descriptor));
	descriptor.transport_command_started = true;
	assert(!transport_descriptor_eligible(&descriptor));
	descriptor.input.head = nullptr;
	descriptor.connected = CON_MAIN_MENU;
	assert(!transport_descriptor_eligible(&descriptor));
}

static void pairs(int (&fds)[2], channel &sender, channel &receiver)
{
	assert(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, fds) == 0);
	assert(sender.attach(fds[0], getpid()));
	assert(receiver.attach(fds[1], getpid()));
}

int main()
{
	session_eligibility();
	inherited_high_descriptor();
	writer w;
	w.number(0x0102030405060708ULL);
	w.string("session");
	assert(w.bytes.substr(0, 8) == std::string("\1\2\3\4\5\6\7\10", 8));
	reader r{ w.bytes };
	assert(r.number() == 0x0102030405060708ULL && r.string(7) == "session" && r.done());
	reader truncated{ w.bytes };
	truncated.number();
	assert(truncated.string(6).empty() && !truncated.valid);

	int fds[2];
	channel sender, receiver;
	pairs(fds, sender, receiver);
	assert(sender.queue(type::input, 17, 42, 2, "command"));
	unsigned messages = 0;
	assert(receiver.receive(
		[&](const message &m)
		{
			assert(m.kind == type::input && m.session == 17 && m.sequence == 42 &&
			       m.detail == 2 && m.payload == "command");
			++messages;
			return true;
		}));
	assert(messages == 1);
	// Backpressure on the IPC socket is retained and bounded, including large messages.
	const std::string large(payload_limit, 'x');
	unsigned admitted = 0;
	while (sender.queue(type::output, 17, ++admitted, 1, large))
		assert(sender.pending_bytes <= sender.queued_bytes &&
		       sender.queued_bytes <= channel_limit);
	assert(admitted <= 4 && sender.pending_bytes <= channel_limit && !sender.failed);
	assert(!sender.queue(type::output, 17, 1, 1, std::string(payload_limit + 1, 'x')));
	close(fds[0]);
	close(fds[1]);

	// An unsupported version and an excessive declared length fail before allocation.
	for (bool wrong_version : { true, false })
	{
		channel tx, rx;
		pairs(fds, tx, rx);
		writer header;
		header.bytes = "DTP1";
		header.number(wrong_version ? version + 1 : version, 2);
		header.number(static_cast<unsigned>(type::input), 2);
		header.number(1);
		header.number(1);
		header.number(wrong_version ? 0 : payload_limit + 1, 4);
		header.number(1, 4);
		assert(send(fds[0], header.bytes.data(), header.bytes.size(), MSG_NOSIGNAL) ==
		       static_cast<ssize_t>(header_size));
		assert(!rx.receive(
			[](const message &)
			{
				std::abort();
				return true;
			}));
		assert(rx.incoming.size() == header_size);
		close(fds[0]);
		close(fds[1]);
	}

	// Possession of a duplicated fd is insufficient: credentials must identify the child/parent.
	channel tx, rx;
	pairs(fds, tx, rx);
	const pid_t other = fork();
	assert(other >= 0);
	if (!other)
	{
		close(fds[1]);
		tx.queue(type::input, 1, 1, 1, "forged");
		_exit(0);
	}
	waitpid(other, nullptr, 0);
	assert(!rx.receive(
		[](const message &)
		{
			std::abort();
			return true;
		}));
	close(fds[0]);
	close(fds[1]);
	std::cout
		<< "Transport framing, bounds, backpressure, version and peer authentication passed\n";
}
