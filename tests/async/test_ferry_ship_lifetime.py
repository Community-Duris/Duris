#!/usr/bin/env python3
"""Ferry ship rooms and ship-object lifetime, run through the real ferry code under ASan/UBSan.

init_ferries() builds every ferries[] entry.  Each Ferry lists the ship's rooms, and
ticket control, announcements, "look out", and "disembark" only reach listed rooms.
The WaveDancer's range once stopped at 47010, so its hold and cabins (47012-47023)
were outside ticket control.  The boarding room may lie inside the range, as the
WaveDancer's 47011 and the Crimson Fulgur's 47146 do, and must still be listed once.

Each Ferry also keeps a raw pointer to its ship object.  zone_purge() must leave
ferry ships and ticket automats in place (see test_zone_purge_lifetime.py).
Named staff purge also refuses ferry objects. extract_obj() still calls
ferry_forget_object() so that other extraction disables the ferry and puts its
passengers ashore instead of
leaving the pointer dangling.  Ferry::panic() collects the passengers before it
moves them: char_to_room() links a moved passenger into the destination room's
list, so walking next_in_room after the move skipped the rest of the ship room,
and a PC already in the destination room made the walk loop forever.
"""

from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, extract_function

HARNESS = r'''
#include "world/ferry.c"
#include "world/ferryact.c"

#include <cassert>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include "world/vnum.obj.h"

static const int ROOM_COUNT = 400, OBJECT_COUNT = 32;
static room_data test_rooms[ROOM_COUNT] = {};
P_room world = test_rooms;
static index_data test_obj_index[OBJECT_COUNT] = {};
P_index obj_index = test_obj_index;
const char *dirs2[] = { "the north", "the east", "the south", "the west", "above", "below" };
const int rev_dir[] = { 2, 3, 0, 1, 5, 4 };

static std::map<int, int> room_rnums, object_rnums;
static std::map<P_char, std::string> told;
static int looks = 0, last_look_room = NOWHERE;
P_obj object_list = nullptr;
static P_obj named_object = nullptr;
static int purges_allowed = 0, ticket_attempts = 0, spends = 0, refunds = 0;
static bool reject_payment = false, fail_ticket = false;
static std::string room_message;

int real_room0(const int vnum)
{
	auto found = room_rnums.find(vnum);
	if (found != room_rnums.end())
		return found->second;
	const int rnum = static_cast<int>(room_rnums.size()) + 1; // rnum 0 is The Void
	assert(rnum < ROOM_COUNT);
	room_rnums[vnum] = rnum;
	world[rnum].number = vnum;
	return rnum;
}

int real_object0(const int vnum)
{
	auto found = object_rnums.find(vnum);
	if (found != object_rnums.end())
		return found->second;
	const int rnum = static_cast<int>(object_rnums.size()) + 1;
	assert(rnum < OBJECT_COUNT);
	object_rnums[vnum] = rnum;
	obj_index[rnum].virtual_number = vnum;
	return rnum;
}

int real_object(int vnum) { return real_object0(vnum); }

P_obj read_object(int nr, int type)
{
	if (type == VIRTUAL && nr == FERRY_TICKET_VNUM)
	{
		++ticket_attempts;
		if (fail_ticket)
			return nullptr;
	}
	P_obj obj = new obj_data{};
	obj->R_num = type == VIRTUAL ? real_object0(nr) : nr;
	obj->loc_p = LOC_NOWHERE;
	obj_index[obj->R_num].number++;
	return obj;
}

void obj_to_room(P_obj obj, int room)
{
	obj->next_content = world[room].contents;
	world[room].contents = obj;
	obj->loc.room = room;
	obj->loc_p = LOC_ROOM;
}

void obj_from_room(P_obj obj)
{
	P_obj *at = &world[obj->loc.room].contents;
	while (*at && *at != obj)
		at = &(*at)->next_content;
	assert(*at == obj);
	*at = obj->next_content;
	obj->next_content = nullptr;
	obj->loc.room = NOWHERE;
	obj->loc_p = LOC_NOWHERE;
}

void extract_obj(P_obj obj, int)
{
	ferry_forget_object(obj);
	if (OBJ_ROOM(obj))
		obj_from_room(obj);
	obj_index[obj->R_num].number--;
	free(obj->short_description);
	delete obj;
}

// the server's list handling: removal unlinks, arrival goes to the head of the list
void char_from_room(P_char ch)
{
	P_char *at = &world[ch->in_room].people;
	while (*at && *at != ch)
		at = &(*at)->next_in_room;
	assert(*at == ch);
	*at = ch->next_in_room;
	ch->next_in_room = nullptr;
	ch->in_room = NOWHERE;
}

bool char_to_room(P_char ch, int room, int)
{
	assert(ch->in_room == NOWHERE && room > 0);
	ch->next_in_room = world[room].people;
	world[room].people = ch;
	ch->in_room = room;
	return true;
}

void send_to_char(const char *message, P_char ch)
{
	told[ch] += message;
}

void send_to_room(const char *message, int) { room_message += message; }
void act(const char *, int, P_char, P_obj, void *, int) {}
void logit(const char *, const char *, ...) {}
void debug(const char *, ...) {}

int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

void new_look(P_char, const char *, int cmd, int room)
{
	assert(cmd == CMD_LOOKOUT || cmd == CMD_LOOKAFAR);
	looks++;
	last_look_room = room;
}

bool valid_ship_edge(int, int)
{
	return true;
}

bool dijkstra(int, int, valid_edge_func *, vector<int> &path)
{
	path.assign(1, 0);
	return true;
}

P_char read_mobile(int, int)
{
	return nullptr;
}

char *skip_spaces(char *string)
{
	while (isspace(static_cast<unsigned char>(*string)))
		string++;
	return string;
}

// Command lookup and wallet admission are isolated; the production procs run below.
char *one_argument(const char *, char *first)
{
	*first = '\0';
	return nullptr;
}
P_obj get_obj_in_list_vis(P_char, const char *, P_obj, bool)
{
	return named_object;
}
bool isname(const char *name, const char *list)
{
	return strcasecmp(name, list) == 0;
}
char *str_dup(const char *text) { return strdup(text); }
char *coin_stringv(int, int) { static char price[] = "10 platinum"; return price; }
int SUB_MONEY(P_char ch, int amount, int) {
	++spends;
	if (reject_payment) return -1;
	ch->points.cash[0] -= amount;
	return 0;
}
void ADD_MONEY(P_char ch, int amount, const char *) { ++refunds; ch->points.cash[0] += amount; }
void obj_to_char(P_obj obj, P_char ch) { obj->next_content = ch->carrying; ch->carrying = obj; }

// WAGON_PROC

static void named_purge(P_char ch)
{
	P_obj obj;
	char name[] = "object";
// PURGE_PREFIX
		++purges_allowed;
	}
}

static P_char character(bool npc, int room)
{
	P_char ch = new char_data{};
	if (npc)
		ch->specials.act = ACT_ISNPC;
	ch->in_room = NOWHERE;
	char_to_room(ch, room, 0);
	return ch;
}

static std::set<int> vnum_range(int first, int last)
{
	std::set<int> rooms;
	for (int vnum = first; vnum <= last; vnum++)
		rooms.insert(real_room0(vnum));
	return rooms;
}

static P_obj automat_at(int room, int ferry_id = 0)
{
	for (P_obj obj = world[room].contents; obj; obj = obj->next_content)
		if (obj_index[obj->R_num].virtual_number == FERRY_AUTOMAT_OBJ &&
		    (!ferry_id || obj->value[0] == ferry_id))
			return obj;
	return nullptr;
}

int main()
{
	init_ferries();
	assert(ferry_list.size() == 7);

	// every ship room is listed exactly once, boarding room included
	for (Ferry *ferry : ferry_list)
	{
		const std::set<int> unique(ferry->rooms.begin(), ferry->rooms.end());
		assert(unique.size() == ferry->rooms.size());
		assert(ferry->room_num_on_board(ferry->boarding_room_num));
		assert(get_ferry_from_obj(ferry->obj->R_num) == ferry);
	}

	Ferry *wave_dancer = get_ferry(1), *crimson_fulgur = get_ferry(4), *stromvok = get_ferry(7);
	assert(wave_dancer && crimson_fulgur && stromvok && ferry_list.front() == wave_dancer);
	const std::set<int> wave_rooms(wave_dancer->rooms.begin(), wave_dancer->rooms.end());
	assert(wave_rooms == vnum_range(47003, 47023) && wave_dancer->rooms.size() == 21);
	assert(get_ferry_from_room(real_room0(47012)) == wave_dancer); // below deck
	const std::set<int> fulgur_rooms(crimson_fulgur->rooms.begin(), crimson_fulgur->rooms.end());
	assert(fulgur_rooms == vnum_range(47133, 47195) && crimson_fulgur->rooms.size() == 63);
	printf("ferry rooms: 7 ferries list each ship room once; WaveDancer 47003-47023\n");

	// a zone purge keeps every ferry's ship and automats, and nothing else
	const int dock = wave_dancer->route[0].dest_room;
	assert(wave_dancer->cur_room() == dock && automat_at(dock));
	for (Ferry *ferry : ferry_list)
		assert(is_ferry_object(ferry->obj));
	assert(is_ferry_object(automat_at(dock)));
	P_obj crate = read_object(47005, VIRTUAL);
	assert(!is_ferry_object(crate) && !is_ferry_object(nullptr));
	char_data staff{}, buyer{};
	staff.in_room = dock;
	named_object = crate;
	named_purge(&staff);
	assert(purges_allowed == 1);
	for (Ferry *ferry : ferry_list)
	{
		for (P_obj protected_object : {ferry->obj, automat_at(ferry->cur_room(), ferry->id)})
		{
			assert(protected_object);
			named_object = protected_object;
			told[&staff].clear();
			named_purge(&staff);
			assert(purges_allowed == 1);
			assert(told[&staff].find("cannot be purged") != std::string::npos);
			assert(ferry->obj && ferry->cur_state == FRY_STATE_WAITING);
		}
	}

	char ticket_arg[] = "ticket", contraption[] = "contraption";
	P_obj machine = automat_at(stromvok->cur_room(), stromvok->id);
	buyer.points.cash[0] = 20000;
	reject_payment = true;
	assert(ferry_automat_proc(machine, &buyer, CMD_BUY, ticket_arg));
	assert(!buyer.carrying && spends == 1 && ticket_attempts == 0 && GET_MONEY(&buyer) == 20000);
	assert(told[&buyer].find("cannot accept your payment") != std::string::npos);
	assert(told[&buyer].find("receive a ticket") == std::string::npos);
	reject_payment = false;
	buyer.points.cash[0] = 0;
	assert(ferry_automat_proc(machine, &buyer, CMD_BUY, ticket_arg));
	assert(!buyer.carrying && spends == 1 && ticket_attempts == 0);
	buyer.points.cash[0] = 20000;
	fail_ticket = true;
	assert(ferry_automat_proc(machine, &buyer, CMD_BUY, ticket_arg));
	assert(!buyer.carrying && spends == 2 && ticket_attempts == 1 && refunds == 1);
	assert(GET_MONEY(&buyer) == 20000);
	fail_ticket = false;
	for (Ferry *ferry : ferry_list)
	{
		machine = automat_at(ferry->cur_room(), ferry->id);
		buyer.points.cash[0] = 20000;
		told[&buyer].clear();
		assert(ferry_automat_proc(machine, &buyer, CMD_LOOK, contraption));
		assert(told[&buyer].starts_with(ferry->name + " sails from here"));
		assert(ferry_automat_proc(machine, &buyer, CMD_BUY, ticket_arg));
		assert(buyer.carrying && buyer.carrying->value[0] == ferry->id);
		assert(GET_MONEY(&buyer) == 20000 - ferry->ticket_price);
		assert(std::string(buyer.carrying->short_description) ==
		       "a &+Wferry ticket&n for " + ferry->name);
		P_obj ticket = buyer.carrying;
		buyer.carrying = nullptr;
		extract_obj(ticket, TRUE);
		room_message.clear();
		ferry->state_timer = 1;
		ferry->activity();
		assert(room_message.starts_with(ferry->name + " departs for "));
		ferry->cur_state = FRY_STATE_WAITING;
		ferry->state_timer = ferry->wait_time;
	}
	puts("named purge protection, failed/successful ticket purchases and ferry messages passed");

	// The wagon proc receives trimmed arguments, and viewing outside never moves the rider.
	P_obj wagon = read_object(12345, VIRTUAL);
	object_list = wagon;
	wagon->loc.room = dock;
	char_data wagon_rider{};
	wagon_rider.in_room = real_room0(12345);
	for (const char *argument : {"out", "  out", "OUT"})
	{
		char look_arg[16];
		strcpy(look_arg, argument);
		const int previous_looks = looks;
		assert(wagon_exit_room(wagon_rider.in_room, &wagon_rider, CMD_LOOK, look_arg));
		assert(looks == previous_looks + 1 && last_look_room == dock);
		assert(wagon_rider.in_room == real_room0(12345));
	}
	char unrelated[] = "floor", empty[] = "";
	assert(!wagon_exit_room(wagon_rider.in_room, &wagon_rider, CMD_LOOK, unrelated));
	assert(!wagon_exit_room(wagon_rider.in_room, &wagon_rider, CMD_LOOK, empty));
	assert(!wagon_exit_room(wagon_rider.in_room, &wagon_rider, CMD_LOOK, nullptr));
	assert(!wagon_exit_room(wagon_rider.in_room, &wagon_rider, CMD_BUY, ticket_arg));
	object_list = nullptr;
	delete wagon;
	puts("wagon look-out argument handling and stationary view passed");

	// extracting an unrelated object changes no ferry
	extract_obj(crate, TRUE);
	for (Ferry *ferry : ferry_list)
		assert(ferry->obj && ferry->cur_state == FRY_STATE_WAITING);

	// extracting the first ferry's ship with passengers aboard and a PC on the dock:
	// every PC aboard goes ashore once, crew and bystander stay where they are
	const int boarding = wave_dancer->boarding_room_num, below = real_room0(47012);
	P_char first = character(false, boarding), second = character(false, boarding);
	P_char crew = character(true, boarding), below_deck = character(false, below);
	P_char bystander = character(false, dock);
	P_obj ship = wave_dancer->obj;
	const int ship_rnum = ship->R_num;
	extract_obj(ship, TRUE);
	assert(!wave_dancer->obj && wave_dancer->cur_state == FRY_STATE_DISABLED);
	assert(wave_dancer->cur_room() == 0);
	for (P_char passenger : { first, second, below_deck })
	{
		const std::string &messages = told[passenger];
		const size_t flash = messages.find("blinding flash");
		assert(passenger->in_room == dock && flash != std::string::npos);
		assert(messages.find("blinding flash", flash + 1) == std::string::npos);
	}
	assert(crew->in_room == boarding && bystander->in_room == dock && told[bystander].empty());
	int on_dock = 0;
	for (P_char ch = world[dock].people; ch; ch = ch->next_in_room)
		on_dock++;
	assert(on_dock == 4);
	printf("ship extraction: WaveDancer disabled, 3 passengers ashore at its first stop\n");

	// nothing reaches the freed ship afterwards (the other ferries would sail this
	// exitless test world, so only the WaveDancer is ticked)
	for (int second_tick = 0; second_tick < 400; second_tick++)
		wave_dancer->activity();
	assert(wave_dancer->cur_state == FRY_STATE_DISABLED);
	wave_dancer->cur_state = FRY_STATE_WAITING; // without a ship, even a waiting ferry idles
	wave_dancer->state_timer = 5;
	wave_dancer->activity();
	assert(wave_dancer->cur_state == FRY_STATE_WAITING && wave_dancer->state_timer == 5);
	wave_dancer->cur_state = FRY_STATE_DISABLED;
	char_from_room(first);
	char_to_room(first, boarding, 0);
	char out[] = "out", nothing[] = "";
	looks = 0;
	assert(!ferry_room_proc(boarding, first, CMD_LOOK, out));
	assert(!ferry_room_proc(boarding, first, CMD_DISEMBARK, nothing));
	assert(looks == 0 && first->in_room == boarding);
	assert(!get_ferry_from_obj(ship_rnum));
	assert(get_ferry_from_obj(stromvok->obj->R_num) == stromvok);

	// the other ferries still answer "look out" with the room their ship is in
	P_char rider = character(false, stromvok->boarding_room_num);
	assert(ferry_room_proc(stromvok->boarding_room_num, rider, CMD_LOOK, out));
	assert(looks == 1 && last_look_room == stromvok->cur_room());
	printf("after extraction: look out, disembark, activity, and lookups skip the WaveDancer\n");
	return 0;
}
'''

# extract_obj() is too entangled to compile here; pin the hook it runs before freeing.
HARNESS = HARNESS.replace("// WAGON_PROC", extract_function("mount.c", "int wagon_exit_room("))
# Execute the unchanged object lookup/refusal prefix; downstream destruction is
# represented by purges_allowed. The lifetime tests execute the real
# ferry_forget_object()/panic() paths through the extraction stub.
purge = extract_function("actwiz.c", "else if ((obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))")
purge_prefix = purge[:purge.index('act("$n destroys $p."')].replace("else if", "if", 1)
HARNESS = HARNESS.replace("// PURGE_PREFIX", purge_prefix)
extract = extract_function("handler.c", "void extract_obj(P_obj obj, int gone_for_good)")
assert "ferry_forget_object(obj);" in extract, "extract_obj must let a ferry drop its ship"
assert extract.index("ferry_forget_object(obj);") < extract.index("free_obj(obj);")

with tempfile.TemporaryDirectory(prefix="duris-ferry-lifetime-") as temporary:
    harness = Path(temporary) / "harness.cpp"
    binary = Path(temporary) / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(["g++", "-std=c++20", "-g", "-O1", "-D__NO_MYSQL__",
                    "-Isrc", "-Isrc/no_mysql", "-I/usr/include/libxml2",
                    "-ffunction-sections", "-fdata-sections",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    str(harness), "-Wl,--gc-sections", "-o", str(binary)], cwd=ROOT, check=True)
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    subprocess.run([str(binary)], check=True, timeout=60, env=environment)
print("ferry ship lifetime passed")
