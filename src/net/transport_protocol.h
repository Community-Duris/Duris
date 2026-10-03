#ifndef DURIS_TRANSPORT_PROTOCOL_H
#define DURIS_TRANSPORT_PROTOCOL_H

// Private, local protocol. Integers are big endian; no ABI structs or pointers
// cross the boundary. An inherited socketpair has no connectable pathname.
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace duris_transport
{
constexpr uint16_t version = 2;
constexpr size_t header_size = 32;
constexpr size_t payload_limit = 4 * 1024 * 1024 + 2048;
constexpr size_t channel_limit = 8 * 1024 * 1024;
constexpr size_t session_input_limit = 2 * 1024 * 1024;
constexpr size_t frontend_input_limit = 16 * 1024 * 1024;
constexpr size_t frontend_output_limit = 64 * 1024 * 1024;
constexpr size_t session_input_count = 128;
constexpr size_t session_output_limit = 4 * 1024 * 1024;
constexpr size_t session_limit = 256;
constexpr unsigned barrier_timeout_ms = 5000;
constexpr unsigned replacement_timeout_seconds = 180;

enum class type : uint16_t
{
	hello = 1,
	welcome,
	open,
	input,
	ack,
	state,
	output,
	close,
	pause,
	paused,
	commit,
	committed,
	abort,
	ready,
	snapshot,
	metadata
};
enum class input_kind : uint32_t
{
	line = 1,
	websocket = 2,
	gmcp = 3
};
enum class output_kind : uint32_t
{
	text = 1,
	binary = 2,
	ws_text = 3,
	ws_binary = 4,
	ws_close = 5
};

struct writer
{
	std::string bytes;
	void number(uint64_t n, unsigned width = 8)
	{
		for (unsigned i = width; i; --i)
			bytes += static_cast<char>(n >> ((i - 1) * 8));
	}
	void string(const std::string &s)
	{
		number(s.size(), 4);
		bytes += s;
	}
};

struct reader
{
	const std::string &bytes;
	size_t offset = 0;
	bool valid = true;
	uint64_t number(unsigned width = 8)
	{
		if (width > 8 || offset > bytes.size() || width > bytes.size() - offset)
		{
			valid = false;
			return 0;
		}
		uint64_t n = 0;
		for (unsigned i = 0; i < width; ++i)
			n = (n << 8) | static_cast<unsigned char>(bytes[offset++]);
		return n;
	}
	std::string string(size_t limit)
	{
		const size_t n = number(4);
		if (!valid || n > limit || offset > bytes.size() || n > bytes.size() - offset)
		{
			valid = false;
			return {};
		}
		std::string result = bytes.substr(offset, n);
		offset += n;
		return result;
	}
	bool done() const { return valid && offset == bytes.size(); }
};

struct message
{
	type kind;
	uint64_t session = 0;
	uint64_t sequence = 0;
	uint32_t detail = 0;
	std::string payload;
};

class channel
{
    public:
	int fd = -1;
	pid_t peer = -1;
	bool failed = false;
	size_t pending_bytes = 0;
	size_t queued_bytes = 0;
	std::deque<std::string> outgoing;
	size_t sent = 0;
	std::string incoming;

	bool attach(int socket, pid_t expected_peer)
	{
		int pass = 1;
		int socket_type = 0;
		socklen_t length = sizeof(socket_type);
		if (socket < 0 || expected_peer <= 1 ||
		    getsockopt(socket, SOL_SOCKET, SO_TYPE, &socket_type, &length) ||
		    socket_type != SOCK_STREAM ||
		    setsockopt(socket, SOL_SOCKET, SO_PASSCRED, &pass, sizeof(pass)))
			return false;
		struct sockaddr_storage address
		{
		};
		length = sizeof(address);
		if (getsockname(socket, reinterpret_cast<sockaddr *>(&address), &length) ||
		    address.ss_family != AF_UNIX)
			return false;
		fd = socket;
		peer = expected_peer;
		return true;
	}

	bool flush()
	{
		size_t budget = 65536;
		while (!failed && !outgoing.empty() && budget)
		{
			const auto &front = outgoing.front();
			const size_t remaining = front.size() - sent;
			const size_t count = remaining < budget ? remaining : budget;
			const ssize_t n =
				send(fd, front.data() + sent, count, MSG_NOSIGNAL | MSG_DONTWAIT);
			if (n < 0 && (errno == EAGAIN || errno == EINTR))
				break;
			if (n <= 0)
			{
				failed = true;
				break;
			}
			sent += n;
			pending_bytes -= n;
			budget -= n;
			if (sent == front.size())
			{
				queued_bytes -= front.size();
				outgoing.pop_front();
				sent = 0;
			}
		}
		return !failed;
	}

	bool queue(type kind, uint64_t session = 0, uint64_t sequence = 0, uint32_t detail = 0,
		   const std::string &payload = {})
	{
		if (failed || payload.size() > payload_limit ||
		    payload.size() + header_size > channel_limit - queued_bytes)
			return false;
		writer w;
		w.bytes = "DTP1";
		w.number(version, 2);
		w.number(static_cast<uint16_t>(kind), 2);
		w.number(session);
		w.number(sequence);
		w.number(payload.size(), 4);
		w.number(detail, 4);
		w.bytes += payload;
		pending_bytes += w.bytes.size();
		queued_bytes += w.bytes.size();
		outgoing.push_back(std::move(w.bytes));
		return flush();
	}

	bool receive(const std::function<bool(const message &)> &consume)
	{
		std::array<char, 65536> buffer{};
		alignas(cmsghdr) std::array<char, CMSG_SPACE(sizeof(ucred))> control{};
		iovec io{ buffer.data(), buffer.size() };
		msghdr packet{};
		packet.msg_iov = &io;
		packet.msg_iovlen = 1;
		packet.msg_control = control.data();
		packet.msg_controllen = control.size();
		const ssize_t n = recvmsg(fd, &packet, MSG_DONTWAIT);
		if (n > 0)
		{
			bool authenticated = false;
			for (cmsghdr *c = CMSG_FIRSTHDR(&packet); c; c = CMSG_NXTHDR(&packet, c))
			{
				if (c->cmsg_level != SOL_SOCKET ||
				    c->cmsg_type != SCM_CREDENTIALS ||
				    c->cmsg_len != CMSG_LEN(sizeof(ucred)))
					continue;
				ucred credentials{};
				memcpy(&credentials, CMSG_DATA(c), sizeof(credentials));
				authenticated = credentials.pid == peer &&
						credentials.uid == geteuid();
			}
			if (!authenticated || (packet.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) ||
			    incoming.size() + static_cast<size_t>(n) >
				    payload_limit + header_size + buffer.size())
				failed = true;
			else
				incoming.append(buffer.data(), n);
		}
		else if (!n || (errno != EAGAIN && errno != EINTR))
			failed = true;
		unsigned budget = 64;
		while (!failed && budget-- && incoming.size() >= header_size)
		{
			const std::string header = incoming.substr(0, header_size);
			reader r{ header, 4 };
			const uint64_t wire_version = r.number(2);
			const uint64_t wire_type = r.number(2);
			message m{};
			m.kind = static_cast<type>(wire_type);
			m.session = r.number();
			m.sequence = r.number();
			const size_t size = r.number(4);
			m.detail = r.number(4);
			if (header.compare(0, 4, "DTP1") || wire_version != version ||
			    wire_type < static_cast<uint16_t>(type::hello) ||
			    wire_type > static_cast<uint16_t>(type::metadata) ||
			    size > payload_limit)
			{
				failed = true;
				break;
			}
			if (incoming.size() < header_size + size)
				break;
			m.payload = incoming.substr(header_size, size);
			incoming.erase(0, header_size + size);
			if (!consume(m))
				failed = true;
		}
		return !failed;
	}
};
}
#endif
