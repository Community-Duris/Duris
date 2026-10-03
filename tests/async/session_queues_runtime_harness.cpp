#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "net/mccp.h"
#include "net/network_readiness.h"
#include "net/session_input.h"
#include "net/telnet.h"
#include "net/ttype.h"
#include "net/unicode.h"
#include "net/websocket.h"
#include <cjson/cJSON.h>
#include <gnutls/gnutls.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// Queue allocation is observable; protocol framing and Unicode use real code.
static size_t queue_allocations;
#undef CREATE
#undef RECREATE
#undef FREE
#define CREATE(result, type, number, tag)                      \
	do                                                     \
	{                                                      \
		++queue_allocations;                           \
		result = (type *)calloc(number, sizeof(type)); \
		assert(result);                                \
	} while (false)
#define RECREATE(result, type, number)                                     \
	do                                                                 \
	{                                                                  \
		++queue_allocations;                                       \
		result = (type *)realloc(result, sizeof(type) * (number)); \
		assert(result);                                            \
	} while (false)
#define FREE(pointer)              \
	do                         \
	{                          \
		free(pointer);     \
		pointer = nullptr; \
	} while (false)
#define PROFILE_START(name) \
	do                  \
	{                   \
	} while (false)
#define PROFILE_END(name) \
	do                \
	{                 \
	} while (false)

extern "C"
{
	P_desc descriptor_list = nullptr;
	int sql_pool_is_active(void)
	{
		return 0;
	}
}
unsigned long long ne_event_tick = 1;
long receivedbytes;
static bool transaction_busy, creation_busy, active_item;
static int closed, oob_dispatched, wire_result;
static std::string delivered;
struct game_loop_pulse_context
{
	uint64_t loop_tick = 0;
	uint64_t prompts_us = 0;
};
static uint64_t loop_monotonic_us()
{
	return 1;
}
static uint64_t latency_trace_elapsed_us(uint64_t, uint64_t)
{
	return 0;
}
static void latency_trace_record(const char *, uint64_t, uint64_t) {}
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}
void banlog(int, const char *, ...) {}
int bannedsite(char *, int)
{
	return 0;
}
int IS_MORPH(P_char)
{
	return 0;
}
void panic_corruption(const char *, const char *, ...)
{
	abort();
}
bool persistence_mode_requires_mysql(void)
{
	return false;
}
int is_desc_valid(P_desc)
{
	return 1;
}
void resolve_descriptor_hostname_async(const char *, int) {}
bool item_movement_transaction_player_busy(P_char)
{
	return transaction_busy;
}
bool bulk_get_player_busy(P_char)
{
	return false;
}
bool currency_transaction_player_busy(P_char)
{
	return false;
}
bool collector_transaction_player_busy(P_char)
{
	return false;
}
bool collector_service_player_busy(P_char)
{
	return false;
}
bool item_creation_grant_blocks_commands(P_char)
{
	return creation_busy;
}
bool item_action_active(P_char)
{
	return active_item;
}
bool input_allowed_while_casting(P_char, const char *text)
{
	return !strcmp(text, "abort");
}
bool input_allowed_while_item_moving(const char *text)
{
	return !strcmp(text, "look");
}
bool input_allowed_while_currency_pending(const char *text)
{
	return !strcmp(text, "look");
}
bool input_allowed_while_item_and_currency_pending(const char *text)
{
	return !strcmp(text, "look");
}
void format_to_snoopers(const char *from, char *to)
{
	strcpy(to, from);
}
void make_prompt(P_desc) {}
int send_ga(P_desc)
{
	return 0;
}
int telnet_flush_output(P_desc)
{
	return 0;
}
int write_to_descriptor(P_desc, const char *text)
{
	delivered += text;
	return wire_result;
}
int compress_start(P_desc, int)
{
	return 0;
}
void gmcp_handle_negotiation(P_desc, int)
{
	++oob_dispatched;
}
void gmcp_handle_input(P_desc, const char *, size_t)
{
	++oob_dispatched;
}
void ttype_handle_negotiation(P_desc, int) {}
void ttype_handle_subnegotiation(P_desc, const unsigned char *, int) {}
char *json_build_gmcp_message(const char *, const char *)
{
	return nullptr;
}
void ws_send_system(P_desc, const char *, const char *) {}
void ws_cmd_game(P_desc, cJSON *);
void ws_handle_command(P_desc d, const char *cmd, cJSON *data)
{
	if (!strcmp(cmd, "game"))
		ws_cmd_game(d, data);
	else if (!strcmp(cmd, "account_info"))
		++oob_dispatched;
	else
		queue_websocket_input(d, cmd);
}
static void process_line(P_desc, char *);
void close_socket(P_desc d)
{
	++closed;
	descriptor_list = d->next;
	flush_queues(d);
	if (d->websocket)
		websocket_free(d);
}

#include "production_session_queues.inc"

static void check_accounting(const txt_q &queue)
{
	size_t bytes = 0, entries = 0;
	const txt_block *last = nullptr;
	for (auto *block = queue.head; block; block = block->next)
	{
		bytes += strlen(block->text) + 1;
		++entries;
		assert(entries <= SESSION_OUTPUT_MAX_ENTRIES);
		last = block;
	}
	assert(bytes == queue.bytes && entries == queue.entries && last == queue.tail);
}
static void expect_command(P_desc d, const char *expected)
{
	char text[MAX_INPUT_LENGTH];
	assert(get_from_q(&d->input, text));
	assert(!strcmp(text, expected));
	check_accounting(d->input);
}
static void assert_empty(P_desc d)
{
	check_accounting(d->input);
	check_accounting(d->output);
	assert(!d->input.entries && !d->input.bytes && !d->input.overflowed);
	assert(!d->output.entries && !d->output.bytes && !d->output.overflowed);
	assert(!d->oob_input_entries && !d->oob_input_bytes && !d->oob_input_overflowed);
}
struct connection
{
	descriptor_data descriptor{};
	char_data character{};
	pc_only_data player{};
	int sockets[2];
	explicit connection(bool websocket)
	{
		assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
		assert(fcntl(sockets[0], F_SETFL, O_NONBLOCK) == 0);
		descriptor.descriptor = sockets[0];
		descriptor.connected = CON_PLAYING;
		descriptor.character = &character;
		character.desc = &descriptor;
		character.only.pc = &player;
		descriptor.websocket = websocket;
		descriptor.ws_handshake_done = websocket;
		descriptor.ws_state = WS_STATE_OPEN;
	}
	~connection()
	{
		flush_queues(&descriptor);
		if (descriptor.websocket)
			websocket_free(&descriptor);
		close(sockets[0]);
		close(sockets[1]);
	}
	void receive(const std::string &message, int opcode = WS_OPCODE_TEXT)
	{
		std::string bytes = message;
		if (descriptor.websocket)
		{
			bytes.clear();
			bytes += char(0x80 | opcode);
			if (message.size() < 126)
				bytes += char(0x80 | message.size());
			else
			{
				assert(message.size() <= 65535);
				bytes += char(0xfe);
				bytes += char(message.size() >> 8);
				bytes += char(message.size());
			}
			const char mask[] = { 0x11, 0x22, 0x33, 0x44 };
			bytes.append(mask, 4);
			for (size_t i = 0; i < message.size(); ++i)
				bytes += message[i] ^ mask[i % 4];
		}
		assert(send(sockets[1], bytes.data(), bytes.size(), 0) == (ssize_t)bytes.size());
		// Each receive models a connection boundary; OOB logical tick budgets
		// remain independently controlled by the scenarios below.
		descriptor.network_input_remaining = descriptor.websocket ? WS_INPUT_BUFFER_SIZE :
									    MAX_QUEUE_LENGTH - 1;
		assert(process_input(&descriptor) == 0);
		if (descriptor.websocket)
			websocket_dispatch_pending_input(&descriptor);
		check_accounting(descriptor.input);
	}
};

static void typeahead_and_blocked_growth(bool websocket)
{
	connection c(websocket);
	P_desc d = &c.descriptor;
	std::string paste;
	for (int i = 0; i < 40; ++i)
		paste += "say " + std::to_string(i) + "\r\n";
	c.character.specials.act2 |= PLR2_WAIT;
	if (websocket)
	{
		cJSON *json = cJSON_CreateObject();
		cJSON_AddStringToObject(json, "type", "cmd");
		cJSON_AddStringToObject(json, "cmd", "game");
		cJSON_AddStringToObject(json, "data", paste.c_str());
		char *text = cJSON_PrintUnformatted(json);
		c.receive(text);
		free(text);
		cJSON_Delete(json);
	}
	else
		c.receive(paste);
	assert(d->input.entries == 40 && !d->input.overflowed);
	char command[MAX_INPUT_LENGTH];
	assert(select_session_input(d, &c.character, command) == session_input_route::none);
	c.character.specials.act2 &= ~PLR2_WAIT;
	for (int i = 0; i < 40; ++i)
	{
		assert(select_session_input(d, &c.character, command) ==
		       session_input_route::playing);
		assert(std::string(command) == "say " + std::to_string(i));
		check_accounting(d->input);
	}
	c.character.specials.act2 |= PLR2_WAIT;
	for (size_t i = 0; i < SESSION_INPUT_MAX_ENTRIES + 100; ++i)
	{
		c.receive(websocket ? "north" : "north\n");
		assert(select_session_input(d, &c.character, command) == session_input_route::none);
	}
	assert(d->input.entries == SESSION_INPUT_MAX_ENTRIES && d->input.overflowed);
	size_t allocations = queue_allocations;
	for (int i = 0; i < 100; ++i)
		c.receive(websocket ? "south" : "south\n");
	assert(queue_allocations == allocations);
	delivered.clear();
	assert(process_output(d) == 1);
	assert(delivered.find("Command queue limit") != std::string::npos);
	delivered.clear();
	for (int i = 0; i < 10; ++i)
		c.receive(websocket ? "south" : "south\n");
	assert(process_output(d) == 1 && delivered.empty());
	for (int i = 0; i < 10; ++i)
		expect_command(d, "north");
	for (int i = 0; i < 11; ++i)
		c.receive(websocket ? "north" : "north\n");
	assert(process_output(d) == 1 && delivered.empty());
	if (!websocket)
		assert(std::string(d->last_input) == "north");
	for (size_t i = 0; i < SESSION_INPUT_MAX_ENTRIES; ++i)
		expect_command(d, "north");
	c.receive(websocket ? "east" : "east\n");
	assert(!d->input.overflowed && !d->input.overflow_reported);
	expect_command(d, "east");
	if (!websocket)
	{
		c.receive("!\n");
		expect_command(d, "east");
	}
	flush_queues(d);
	assert_empty(d);
}

static void wire_byte_limits(bool websocket)
{
	connection c(websocket);
	std::string command(MAX_INPUT_LENGTH - 1, 'x');
	for (size_t i = 0; i < SESSION_INPUT_MAX_BYTES / MAX_INPUT_LENGTH; ++i)
		c.receive(websocket ? command : command + "\n");
	assert(c.descriptor.input.bytes == SESSION_INPUT_MAX_BYTES);
	size_t allocations = queue_allocations;
	c.receive(websocket ? "look" : "look\n");
	assert(c.descriptor.input.overflowed && queue_allocations == allocations);
}

static void websocket_routes()
{
	connection c(true);
	c.receive("{\"type\":\"cmd\",\"cmd\":\"game\",\"data\":{\"command\":\"north\\nsouth\"}}");
	c.receive("{\"type\":\"text\",\"data\":\"east\\nwest\"}");
	c.receive("{\"cmd\":\"look\"}");
	c.receive("{\"type\":\"cmd\",\"cmd\":\"score\"}");
	for (const char *expected : { "north", "south", "east", "west", "look", "score" })
		expect_command(&c.descriptor, expected);
	c.receive("{\"type\":\"cmd\",\"cmd\":\"account_info\"}");
	assert(c.descriptor.input.entries == 0 && c.descriptor.oob_input_entries == 2);
}

static void byte_limits_and_dequeues()
{
	descriptor_data d{};
	std::string command(MAX_INPUT_LENGTH - 1, 'x');
	for (size_t i = 0; i < SESSION_INPUT_MAX_BYTES / MAX_INPUT_LENGTH; ++i)
		write_to_q(command.c_str(), &d.input, 0);
	assert(d.input.bytes == SESSION_INPUT_MAX_BYTES);
	size_t allocations = queue_allocations;
	write_to_q("", &d.input, 0);
	assert(d.input.overflowed && queue_allocations == allocations);
	flush_queues(&d);
	assert_empty(&d);
	for (size_t i = 0; i < SESSION_INPUT_MAX_ENTRIES + 1; ++i)
		write_to_q("", &d.input, 0);
	assert(d.input.entries == SESSION_INPUT_MAX_ENTRIES &&
	       d.input.bytes == SESSION_INPUT_MAX_ENTRIES);
	flush_queues(&d);
	command += 'x';
	allocations = queue_allocations;
	queue_websocket_input(&d, command.c_str());
	assert(!d.input.head && d.input.overflowed && queue_allocations == allocations);
	flush_queues(&d);
	d.input.bytes = SIZE_MAX;
	write_to_q("look", &d.input, 0);
	assert(d.input.bytes == SIZE_MAX && !d.input.head && queue_allocations == allocations);
	d.input = {};

	char_data character{};
	d.character = &character;
	character.desc = &d;
	d.connected = CON_PLAYING;
	write_to_q("north", &d.input, 0);
	write_to_q("abort", &d.input, 0);
	write_to_q("look", &d.input, 0);
	char text[MAX_INPUT_LENGTH];
	character.specials.affected_by2 |= AFF2_CASTING;
	assert(select_session_input(&d, &character, text) == session_input_route::playing);
	assert(!strcmp(text, "abort"));
	check_accounting(d.input);
	character.specials.affected_by2 &= ~AFF2_CASTING;
	transaction_busy = true;
	assert(select_session_input(&d, &character, text) == session_input_route::playing);
	assert(!strcmp(text, "look"));
	check_accounting(d.input);
	assert(select_session_input(&d, &character, text) == session_input_route::none);
	transaction_busy = false;
	creation_busy = true;
	assert(select_session_input(&d, &character, text) == session_input_route::none);
	creation_busy = false;
	expect_command(&d, "north");
	write_to_q("look", &d.input, 0);
	assert(get_pending_transaction_cmd_from_q(&d.input, text, false, true));
	check_accounting(d.input);
	write_to_q("look", &d.input, 0);
	assert(get_pending_transaction_cmd_from_q(&d.input, text, true, true));
	check_accounting(d.input);
	write_to_q("abort", &d.input, 0);
	assert(get_casting_cmd_from_q(&character, &d.input, text));
	check_accounting(d.input);
	flush_queues(&d);
	assert_empty(&d);
}

static void output_limits_and_disconnect(bool websocket)
{
	connection c(websocket);
	P_desc d = &c.descriptor;
	d->connected = CON_NAME;
	write_to_q("hello", &d->output, 1);
	write_to_q(" world", &d->output, 2);
	assert(d->output.entries == 1 && d->output.bytes == 12);
	check_accounting(d->output);
	delivered.clear();
	assert(process_output(d) == 1 && delivered == "hello world");
	check_accounting(d->output);
	std::string chunk(MAX_STRING_LENGTH - 1, 'x');
	for (size_t i = 0; i < SESSION_OUTPUT_MAX_BYTES / MAX_STRING_LENGTH; ++i)
		write_to_q(chunk.c_str(), &d->output, 1);
	assert(d->output.bytes == SESSION_OUTPUT_MAX_BYTES);
	size_t allocations = queue_allocations;
	write_to_q("overflow", &d->output, 1);
	write_to_q("again", &d->output, 1);
	assert(d->output.overflowed && queue_allocations == allocations);
	check_accounting(d->output);
	// A non-writable socket must still be closed at the safe output boundary.
	d->network_revents = 0;
	descriptor_list = d;
	int before = closed;
	game_loop_pulse_context ctx;
	run_output_phase(ctx);
	assert(closed == before + 1 && d->write_failed && !descriptor_list);
	assert_empty(d);
	if (websocket)
	{
		unsigned char frame[128];
		ssize_t length = recv(c.sockets[1], frame, sizeof(frame), MSG_DONTWAIT);
		assert(length >= 4 && frame[0] == 0x88 && frame[2] == 0x03 && frame[3] == 0xf0);
	}

	d->websocket = websocket;
	d->ws_state = WS_STATE_OPEN;
	std::string large(MAX_INPUT_LENGTH, 'x');
	for (size_t i = 0; i < SESSION_OUTPUT_MAX_ENTRIES / 2; ++i)
	{
		write_to_q(large.c_str(), &d->output, 1);
		write_to_q("a", &d->output, 1);
	}
	assert(d->output.entries == SESSION_OUTPUT_MAX_ENTRIES);
	assert(d->output.bytes < SESSION_OUTPUT_MAX_BYTES);
	allocations = queue_allocations;
	// Merging uses bytes without consuming another entry, even at entry capacity.
	write_to_q("b", &d->output, 1);
	assert(!d->output.overflowed && d->output.entries == SESSION_OUTPUT_MAX_ENTRIES);
	assert(queue_allocations == allocations + 1);
	allocations = queue_allocations;
	write_to_q(large.c_str(), &d->output, 1);
	assert(d->output.overflowed && queue_allocations == allocations);
	check_accounting(d->output);
	flush_queues(d);
	assert_empty(d);
	std::string styled;
	for (int i = 0; i < 5000; ++i)
		styled += "&+ra&+gb";
	for (int i = 0; i < 20; ++i)
		write_to_q(styled.c_str(), &d->output, 1);
	assert(d->output.bytes < SESSION_OUTPUT_MAX_BYTES && !d->output.overflowed);
	assert(process_output(d) == -1 && d->output.overflowed);
	check_accounting(d->output);
	flush_queues(d);
	assert_empty(d);
	write_to_q("transport full", &d->output, 1);
	wire_result = WS_OUTPUT_QUEUE_FULL;
	assert(process_output(d) == -1); // Never silently discard failed framing.
	wire_result = 0;
	flush_queues(d);
	assert_empty(d);
}

static void oob_budgets(bool websocket)
{
	connection c(websocket);
	P_desc d = &c.descriptor;
	for (size_t i = 0; i < SESSION_INPUT_MAX_ENTRIES; ++i)
		write_to_q("north", &d->input, 0);
	std::string message = websocket ?
				      "{\"type\":\"gmcp\",\"package\":\"Core.Hello\",\"data\":{}}" :
				      std::string({ char(IAC), char(SB), char(TELOPT_GMCP) }) +
					      "Core.Hello {}" + char(IAC) + char(SE);
	if (websocket)
	{
		c.receive("ping", WS_OPCODE_PING);
		unsigned char pong[32];
		assert(recv(c.sockets[1], pong, sizeof(pong), MSG_DONTWAIT) == 6);
		assert(pong[0] == 0x8a && !memcmp(pong + 2, "ping", 4));
		assert(!d->oob_input_entries && d->input.entries == SESSION_INPUT_MAX_ENTRIES);
	}
	int before = oob_dispatched;
	for (size_t i = 0; i < SESSION_OOB_MAX_ENTRIES; ++i)
		c.receive(message);
	assert(oob_dispatched == before + (int)SESSION_OOB_MAX_ENTRIES);
	assert(d->input.entries == SESSION_INPUT_MAX_ENTRIES && !d->input.overflowed);
	c.receive(message);
	assert(d->oob_input_overflowed && oob_dispatched == before + (int)SESSION_OOB_MAX_ENTRIES);
	descriptor_list = d;
	d->network_revents = 0;
	game_loop_pulse_context ctx;
	before = closed;
	run_output_phase(ctx);
	assert(closed == before + 1);
	assert_empty(d);
}

int main()
{
	for (bool websocket : { false, true })
	{
		typeahead_and_blocked_growth(websocket);
		wire_byte_limits(websocket);
		output_limits_and_disconnect(websocket);
		oob_budgets(websocket);
	}
	websocket_routes();
	byte_limits_and_dequeues();
	descriptor_data d{};
	assert(admit_session_oob(&d, SESSION_OOB_MAX_BYTES));
	assert(!admit_session_oob(&d, 1));
	flush_queues(&d);
	assert_empty(&d);
	assert(admit_session_oob(&d, 100));
	++ne_event_tick;
	assert(admit_session_oob(&d, 100));
	assert(d.oob_input_entries == 1 && d.oob_input_bytes == 100);
	d.oob_input_bytes = SIZE_MAX;
	assert(!admit_session_oob(&d, 1) && d.oob_input_bytes == SIZE_MAX);
	flush_queues(&d);
	puts("Bounded session queue runtime harness passed");
}
