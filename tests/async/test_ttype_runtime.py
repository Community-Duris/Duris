#!/usr/bin/env python3
"""Runtime regressions for RFC 1091 terminal type and MTTS negotiation."""

import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "ttype.h"
#include "telnet.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static unsigned char sent[32];
static size_t sent_len;
static int writes;

int write_to_descriptor_binary(P_desc, const unsigned char *data, size_t len)
{
	if (len > sizeof(sent))
		return -1;
	std::memcpy(sent, data, len);
	sent_len = len;
	writes++;
	return 0;
}

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

static void reset_output()
{
	std::memset(sent, 0, sizeof(sent));
	sent_len = 0;
	writes = 0;
}

static void submit(P_desc d, const char *value)
{
	unsigned char input[256];
	size_t len = std::strlen(value);
	input[0] = TELQUAL_IS;
	std::memcpy(input + 1, value, len);
	ttype_handle_subnegotiation(d, input, static_cast<int>(len + 1));
}

int main()
{
	int tests = 0;

	// 1. Negotiation is suppressed for null and WebSocket descriptors.
	{
		descriptor_data websocket = {};
		websocket.websocket = 1;
		reset_output();
		ttype_negotiate(nullptr);
		ttype_negotiate(&websocket);
		require(writes == 0 && websocket.ttype_state == TTYPE_NONE, 1);
	}
	tests++;

	// 2. Plain Telnet negotiation sends the exact RFC option request.
	{
		descriptor_data d = {};
		reset_output();
		ttype_negotiate(&d);
		const unsigned char expected[] = { IAC, DO, TELOPT_TTYPE };
		require(d.ttype_state == TTYPE_SENT_DO && writes == 1, 2);
		require(sent_len == sizeof(expected) && std::memcmp(sent, expected, sizeof(expected)) == 0,
			3);
	}
	tests++;

	// 3. WILL starts cycling, clears stale state, and sends TTYPE SEND.
	{
		descriptor_data d = {};
		std::strcpy(d.ttype_last, "STALE");
		reset_output();
		ttype_handle_negotiation(&d, WILL);
		const unsigned char expected[] = { IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE };
		require(d.ttype_state == TTYPE_CYCLING && d.ttype_last[0] == '\0', 4);
		require(sent_len == sizeof(expected) && std::memcmp(sent, expected, sizeof(expected)) == 0,
			5);
	}
	tests++;

	// 4. WONT terminates negotiation without emitting another request.
	{
		descriptor_data d = {};
		reset_output();
		ttype_handle_negotiation(&d, WONT);
		require(d.ttype_state == TTYPE_COMPLETE && writes == 0, 6);
	}
	tests++;

	// 5. Subnegotiation is ignored outside cycling state and on WebSockets.
	{
		descriptor_data d = {};
		unsigned char input[] = { TELQUAL_IS, 'X', 'T', 'E', 'R', 'M' };
		reset_output();
		ttype_handle_subnegotiation(&d, input, sizeof(input));
		d.ttype_state = TTYPE_CYCLING;
		d.websocket = 1;
		ttype_handle_subnegotiation(&d, input, sizeof(input));
		require(d.client_name[0] == '\0' && writes == 0, 7);
	}
	tests++;

	// 6. A first client name is normalized, stored, and followed by another request.
	{
		descriptor_data d = {};
		d.ttype_state = TTYPE_CYCLING;
		reset_output();
		submit(&d, "mudlet");
		require(std::strcmp(d.client_name, "MUDLET") == 0, 8);
		require(std::strcmp(d.ttype_last, "MUDLET") == 0 && writes == 1, 9);
	}
	tests++;

	// 7. Repeating the immediately previous value completes the cycle.
	{
		descriptor_data d = {};
		d.ttype_state = TTYPE_CYCLING;
		submit(&d, "MUDLET");
		reset_output();
		submit(&d, "MUDLET");
		require(d.ttype_state == TTYPE_COMPLETE && writes == 0, 10);
	}
	tests++;

	// 8. Returning to the first value after another value also completes.
	{
		descriptor_data d = {};
		d.ttype_state = TTYPE_CYCLING;
		submit(&d, "MUDLET");
		submit(&d, "XTERM-256COLOR");
		reset_output();
		submit(&d, "MUDLET");
		require(d.ttype_state == TTYPE_COMPLETE && writes == 0, 11);
	}
	tests++;

	// 9. Valid MTTS flags drive UTF-8 and legacy charset selection.
	{
		descriptor_data utf8 = {};
		utf8.ttype_state = TTYPE_CYCLING;
		submit(&utf8, "MTTS 269");
		require(utf8.mtts_flags == 269 && HAS_UTF8(&utf8) && HAS_TRUECOLOR(&utf8), 12);
		require(utf8.cp437 == 0, 13);

		descriptor_data legacy = {};
		legacy.ttype_state = TTYPE_CYCLING;
		submit(&legacy, "MTTS 1");
		require(legacy.mtts_flags == MTTS_ANSI && legacy.cp437 == 1, 14);
	}
	tests++;

	// 10. Empty, signed, overflowing, and trailing-junk MTTS fields are rejected.
	for (const char *value : { "MTTS ", "MTTS -1", "MTTS +4", "MTTS 65536",
				  "MTTS 4JUNK", "MTTS 4 " })
	{
		descriptor_data d = {};
		d.ttype_state = TTYPE_CYCLING;
		submit(&d, value);
		require(d.mtts_flags == 0, 15);
	}
	tests++;

	// 11. Embedded controls, NULs, and non-ASCII bytes fail closed.
	for (const unsigned char *input : {
		     reinterpret_cast<const unsigned char *>("\0X\nY"),
		     reinterpret_cast<const unsigned char *>("\0X\0Y"),
		     reinterpret_cast<const unsigned char *>("\0X\xffY") })
	{
		descriptor_data d = {};
		d.ttype_state = TTYPE_CYCLING;
		reset_output();
		ttype_handle_subnegotiation(&d, input, 4);
		require(d.ttype_state == TTYPE_COMPLETE && d.client_name[0] == '\0' && writes == 0,
			16);
	}
	tests++;

	// 12. Empty and oversized values fail closed instead of stalling or truncating.
	{
		descriptor_data empty = {};
		empty.ttype_state = TTYPE_CYCLING;
		unsigned char only_is[] = { TELQUAL_IS };
		ttype_handle_subnegotiation(&empty, only_is, sizeof(only_is));
		require(empty.ttype_state == TTYPE_COMPLETE, 17);

		descriptor_data oversized = {};
		oversized.ttype_state = TTYPE_CYCLING;
		unsigned char input[129];
		input[0] = TELQUAL_IS;
		std::memset(input + 1, 'A', sizeof(input) - 1);
		ttype_handle_subnegotiation(&oversized, input, sizeof(input));
		require(oversized.ttype_state == TTYPE_COMPLETE && oversized.client_name[0] == '\0',
			18);
	}
	tests++;

	// 13. Charset fallback honors TLS, known legacy terminals, and modern defaults.
	{
		descriptor_data tls = {};
		tls.sslses = reinterpret_cast<gnutls_session_t>(1);
		std::strcpy(tls.client_name, "ZMUD");
		check_cp437(&tls);
		require(tls.cp437 == 0, 19);

		descriptor_data legacy = {};
		std::strcpy(legacy.client_name, "CMUD");
		check_cp437(&legacy);
		require(legacy.cp437 == 1, 20);

		descriptor_data modern = {};
		std::strcpy(modern.client_name, "MUDLET");
		check_cp437(&modern);
		require(modern.cp437 == 0, 21);
	}
	tests++;

	std::printf("%d terminal-type runtime cases passed\n", tests);
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ttype-runtime-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{SRC}",
            str(harness),
            str(SRC / "ttype.c"),
            "-lbsd",
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("terminal-type runtime regressions passed")
