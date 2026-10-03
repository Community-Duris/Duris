#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "net/mccp.h"
#include "net/network_readiness.h"
#include "net/telnet.h"
#include "net/ttype.h"
#include "net/unicode.h"
#include "net/websocket.h"

#include <gnutls/gnutls.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

static std::vector<std::string> delivered_gmcp;
static std::vector<std::string> delivered_ttype;
static std::vector<std::string> delivered_commands;
static std::vector<size_t> admitted_oob_lengths;

void panic_corruption(const char *, const char *, ...)
{
	abort();
}

void logit(const char *, const char *, ...) {}

bool admit_session_oob(P_desc, size_t len)
{
	admitted_oob_lengths.push_back(len);
	return true;
}

int compress_start(P_desc, int)
{
	return 0;
}

void gmcp_handle_negotiation(P_desc, int) {}

void gmcp_handle_input(P_desc, const char *data, size_t len);

void ttype_handle_negotiation(P_desc, int) {}

void ttype_handle_subnegotiation(P_desc, const unsigned char *data, int len)
{
	delivered_ttype.emplace_back(reinterpret_cast<const char *>(data), len);
}

int websocket_process_input(P_desc)
{
	return -1;
}

extern "C" ssize_t gnutls_record_recv(gnutls_session_t, void *, size_t)
{
	return GNUTLS_E_AGAIN;
}

extern "C" int gnutls_record_get_direction(gnutls_session_t)
{
	return 0;
}

extern "C" const char *gnutls_strerror(int)
{
	return "synthetic test error";
}

void gmcp_handle_input(P_desc, const char *data, size_t len)
{
	delivered_gmcp.emplace_back(data, len);
}

int parse_telnet_options(P_desc player, char *buf, int buflen);

/* PARSE_TELNET_OPTIONS_IMPLEMENTATION */

static void process_line(P_desc, char *input)
{
	delivered_commands.emplace_back(input);
}

/* PROCESS_INPUT_IMPLEMENTATION */

class test_connection
{
    public:
	descriptor_data descriptor{};
	int sockets[2]{};

	test_connection()
	{
		delivered_gmcp.clear();
		delivered_ttype.clear();
		delivered_commands.clear();
		admitted_oob_lengths.clear();
		assert(pipe(sockets) == 0);
		descriptor.descriptor = static_cast<sh_int>(sockets[0]);
	}

	~test_connection()
	{
		close(sockets[0]);
		close(sockets[1]);
	}

	int send(std::string_view input)
	{
		size_t offset = 0;
		while (offset < input.size())
		{
			const ssize_t written =
				write(sockets[1], input.data() + offset, input.size() - offset);
			assert(written > 0);
			offset += static_cast<size_t>(written);
		}
		descriptor.network_input_remaining = MAX_QUEUE_LENGTH - 1;
		return process_input(&descriptor);
	}
};

static std::string subnegotiation_frame(ubyte option, std::string_view payload)
{
	std::string frame;
	frame.push_back(static_cast<char>(IAC));
	frame.push_back(static_cast<char>(SB));
	frame.push_back(static_cast<char>(option));
	frame.append(payload);
	frame.push_back(static_cast<char>(IAC));
	frame.push_back(static_cast<char>(SE));
	return frame;
}

int main()
{
	{
		test_connection connection;
		const std::string command(MAX_QUEUE_LENGTH - 1, 'x');

		assert(connection.send(std::string_view(command).substr(0, 1000)) == 0);
		assert(delivered_commands.empty());
		assert(connection.send(std::string_view(command).substr(1000)) == 0);
		assert(connection.descriptor.buflen == 0);
		assert(delivered_commands == std::vector<std::string>{ command });
		assert(delivered_gmcp.empty());
	}

	/* A frame longer than a command stays opaque at every receive boundary. */
	std::string long_payload(1500, 'g');
	long_payload.replace(500, 4, "\r\n\b\0", 4);
	const std::string long_frame = subnegotiation_frame(TELOPT_GMCP, long_payload);
	for (size_t split = 1; split < long_frame.size(); split++)
	{
		test_connection connection;
		const std::string wire = long_frame + "look\r\n";

		assert(connection.send(std::string_view(wire).substr(0, split)) == 0);
		assert(connection.descriptor.buflen == static_cast<int>(split));
		assert(delivered_gmcp.empty() && delivered_commands.empty());
		assert(admitted_oob_lengths.empty());

		assert(connection.send(std::string_view(wire).substr(split)) == 0);
		assert(connection.descriptor.buflen == 0);
		assert(delivered_gmcp == std::vector<std::string>{ long_payload });
		assert(delivered_commands == std::vector<std::string>{ "look" });
		assert(admitted_oob_lengths == std::vector<size_t>{ long_frame.size() });
	}

	/* Quoted IAC followed by SE is payload for supported and unknown options. */
	for (ubyte option : { TELOPT_GMCP, TELOPT_TTYPE, TELOPT_NAWS })
	{
		std::string payload(1, TELQUAL_IS);
		payload += "before";
		payload.push_back(static_cast<char>(IAC));
		payload.push_back(static_cast<char>(IAC));
		payload.push_back(static_cast<char>(SE));
		payload += "after";
		payload.append(4, static_cast<char>(IAC));
		const std::string frame = subnegotiation_frame(option, payload);
		const std::string wire = "lo" + frame + "ok\r\n";

		std::string expected(1, TELQUAL_IS);
		expected += "before";
		expected.push_back(static_cast<char>(IAC));
		expected.push_back(static_cast<char>(SE));
		expected += "after";
		expected.append(2, static_cast<char>(IAC));
		for (size_t split = 1; split < frame.size(); split++)
		{
			test_connection connection;
			assert(connection.send(std::string_view(wire).substr(0, 2 + split)) == 0);
			assert(connection.descriptor.buflen == static_cast<int>(2 + split));
			assert(delivered_gmcp.empty() && delivered_ttype.empty());
			assert(delivered_commands.empty() && admitted_oob_lengths.empty());
			assert(connection.send(std::string_view(wire).substr(2 + split)) == 0);
			assert(connection.descriptor.buflen == 0);
			if (option == TELOPT_GMCP)
				assert(delivered_gmcp == std::vector<std::string>{ expected });
			else
				assert(delivered_gmcp.empty());
			if (option == TELOPT_TTYPE)
				assert(delivered_ttype == std::vector<std::string>{ expected });
			else
				assert(delivered_ttype.empty());
			assert(delivered_commands == std::vector<std::string>{ "look" });
			assert(admitted_oob_lengths == std::vector<size_t>{ frame.size() });
		}
	}

	/* A complete frame at capacity must free room for the next command. */
	{
		test_connection connection;
		const std::string payload(MAX_QUEUE_LENGTH - 6, 'g');
		const std::string frame = subnegotiation_frame(TELOPT_GMCP, payload);
		assert(frame.size() == MAX_QUEUE_LENGTH - 1);
		assert(connection.send(std::string_view(frame).substr(0, 2000)) == 0);
		assert(connection.send(std::string_view(frame).substr(2000, 2000)) == 0);
		assert(delivered_gmcp.empty() && delivered_commands.empty());
		assert(connection.send(std::string_view(frame).substr(4000)) == 0);
		assert(connection.descriptor.buflen == 0);
		assert(delivered_gmcp == std::vector<std::string>{ payload });
		assert(admitted_oob_lengths == std::vector<size_t>{ frame.size() });
		assert(connection.send("look\r\n") == 0);
		assert(delivered_commands == std::vector<std::string>{ "look" });
	}

	/* An unfinished frame at capacity fails before protocol bytes become text. */
	{
		test_connection connection;
		std::string unfinished;
		unfinished.push_back(static_cast<char>(IAC));
		unfinished.push_back(static_cast<char>(SB));
		unfinished.push_back(static_cast<char>(TELOPT_GMCP));
		unfinished.append(MAX_QUEUE_LENGTH - 1 - unfinished.size(), 'x');

		assert(connection.send(std::string_view(unfinished).substr(0, 2000)) == 0);
		assert(connection.send(std::string_view(unfinished).substr(2000, 2000)) == 0);
		assert(connection.send(std::string_view(unfinished).substr(4000)) == -1);
		assert(connection.descriptor.buflen < MAX_QUEUE_LENGTH);
		assert(delivered_gmcp.empty() && delivered_commands.empty());
	}

	puts("Telnet input framing, fragmentation, escaped-IAC and size-bound tests passed");
	return 0;
}
