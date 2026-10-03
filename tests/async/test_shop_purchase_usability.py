#!/usr/bin/env python3
"""Execute the real buy parser, command, and shop callbacks with controlled completions.

World lookup, wallet/grant submission and publication are fixtures. Production
parsing, validation, sequencing and fixture_messages are compiled directly from shop.c.
No database, running server, or local player state is used.
"""

from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

SHOP = (ROOT / "src/economy/shop.c").read_text(encoding="utf-8")


def definition(signature):
    start = SHOP.rindex(signature)
    brace = SHOP.index("{", start)
    depth = 0
    for end in range(brace, len(SHOP)):
        depth += (SHOP[end] == "{") - (SHOP[end] == "}")
        if not depth:
            return SHOP[start:end + 1]
    raise AssertionError(signature)


PRELUDE = r'''
#include "core/prototypes.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/utility.h"
#include "economy/shop.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/currency_transaction.h"
#include "economy/shop_trade_runtime.h"
#include "economy/shop_trade_transaction.h"
#include "item/item_movement_transaction.h"
#include "persistence/persistence_mode.h"
#include "world/epic_bonus.h"
#include <cassert>
#include <cstdarg>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

P_obj object_list = nullptr;
static room_data room{};
P_room world = &room;
extern const int top_of_world = 0;
static index_data index_fixture[1]{};
P_index obj_index = index_fixture;
P_index mob_index = index_fixture;
static shop_data shop{};
shop_data *shop_index = &shop;
int number_of_shops = 1;
cha_app_type cha_app[256]{};
char *drinks[] = {const_cast<char *>("water")};
char *item_condition(P_obj) { return const_cast<char *>(""); }
std::string pad_ansi(const char *text, int, bool) { return text; }
static persistence_mode mode;
static bool busy = false, refuse_submit = false, exact = true, accounting_active = false, keeper_cash_recorded = false;
bool economic_gameplay_authority::active() { return accounting_active; }
static int submissions = 0, rooms = 0, tells = 0, refunds = 0;
static std::vector<std::string> fixture_messages;
static obj_data fixture_clones[51]{};
static int creation_index = 0;
static P_obj pending_item = nullptr;
static currency_completion_fn payment_callback = nullptr;
static item_creation_grant_completion_fn grant_callback = nullptr;
static shop_trade_completion_fn trade_callback = nullptr;
static shop_trade_payload trade_payload{};
static uint64_t payment_uid = 0;
static int64_t payment_delta = 0;

void send_to_char(const char *message, P_char) { fixture_messages.emplace_back(message); }
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}
void wizlog(int, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
char *coin_stringv(int value, int) { static char result[64]; if (value) snprintf(result, sizeof(result), "%d copper", value); else result[0] = 0; return result; }
void act(const char *, int, P_char, P_obj, void *, int) { ++rooms; }
void do_tell(P_char, char *, int) { ++tells; }
void mobsay(P_char, const char *message) { fixture_messages.emplace_back(message); }
int real_room(int) { return 0; }
int STAT_INDEX(int stat) { return stat; }
bool ac_can_see_obj(P_char, P_obj, int) { return true; }
char *FirstWord(char *word) { return word; }
void CAP(char *word) { *word = toupper(*word); }
int checked_snprintf(char *buffer, size_t size, const char *format, ...) {
    va_list args; va_start(args, format); const int result = vsnprintf(buffer, size, format, args); va_end(args); return result;
}
int checked_substitute_strings(char *buffer, size_t size, const char *, const char *const *, size_t) { if(size) *buffer = 0; return 0; }
int is_ok(P_char, P_char, int) { return TRUE; }
bool has_innate(P_char, int) { return false; }
int number(int, int) { return 0; }
float get_epic_bonus(P_char, int) { return 0; }
int writeShopKeeper(P_char, int) { return 0; }
void ADD_MONEY(P_char ch, int amount) { GET_COPPER(ch) += amount; if (IS_PC(ch)) ++refunds; }
int SUB_MONEY(P_char ch, int amount, int) { GET_COPPER(ch) -= amount; return 0; }
bool transact(P_char, P_obj, P_char, int) { ++submissions; return true; }
P_obj accept_gem_for_debt(P_char, P_char, int) { return nullptr; }
void sql_shop_sell(P_char, P_obj, int) {}
int container_total_weight(P_obj container) { return container->value[2]; }
bool shop_trade_transaction_player_busy(P_char) { return busy; }
bool item_movement_transaction_player_busy(P_char) { return busy; }
bool currency_transaction_player_busy(P_char) { return busy; }
persistence_mode persistence_mode_get() { return mode; }
bool isname(const char *name, const char *list) { return list && !strcmp(name, list); }
P_obj get_obj_in_list(char *name, P_obj list) {
    for (; list; list = list->next_content) if (isname(name, list->name)) return list;
    return nullptr;
}
P_obj get_obj_in_list_vis(P_char, const char *name, P_obj list, bool) { return get_obj_in_list(const_cast<char *>(name), list); }
int fill_word(char *word) {
    for (const char *fill : {"in", "from", "with", "the", "on", "at", "to"}) if (!strcmp(word, fill)) return true;
    return false;
}
P_obj read_object(int, int) {
    auto &fixture_clone = fixture_clones[creation_index++];
    fixture_clone = {}; fixture_clone.loc_p = LOC_NOWHERE; fixture_clone.obj_uid = 100 + creation_index; fixture_clone.R_num = 0; fixture_clone.weight = 1;
    fixture_clone.name = const_cast<char *>("ration"); fixture_clone.short_description = const_cast<char *>("an iron ration");
    fixture_clone.next = object_list; object_list = &fixture_clone; return &fixture_clone;
}
void extract_obj(P_obj object, int) { if (object_list == object) object_list = object->next; }
void obj_from_char(P_obj object) { object->loc_p = LOC_NOWHERE; }
void obj_to_char(P_obj object, P_char ch) { object->loc_p = LOC_CARRIED; object->loc.carrying = ch; ++IS_CARRYING_N(ch); }
void obj_to_obj(P_obj object, P_obj container) { object->loc_p = LOC_INSIDE; object->loc.inside = container; ++container->value[2]; }
bool shop_trade_runtime_object_matches_payload(P_obj, const shop_trade_payload &) { return exact; }
shop_trade_payload_build_result shop_trade_runtime_build_payload(
    P_char ch, P_char keeper, P_obj selected, P_obj stock, P_obj destination, uint32_t shop_id,
    shop_trade_action action, int64_t price, shop_trade_payload *payload) {
    *payload = {}; payload->player_pid = GET_PID(ch); payload->shop_id = shop_id;
    payload->selected_item_uid = selected->obj_uid; payload->stock_item_uid = stock ? stock->obj_uid : 0;
    payload->target_parent_item_uid = destination ? destination->obj_uid : 0;
    payload->price = price; payload->action = action; pending_item = selected;
    payload->keeper_vnum = GET_VNUM(keeper); payload->expected_keeper_cash = GET_MONEY(keeper);
    return shop_trade_payload_build_result::ok;
}
bool shop_trade_transaction_submit(P_char, const shop_trade_payload &payload, shop_trade_completion_fn callback) {
    if (refuse_submit) return false;
    ++submissions; trade_payload = payload; trade_callback = callback; return true;
}
bool currency_transaction_submit_wallet_value(P_char, int64_t delta, currency_reason_type, int64_t,
    critical_source_site, critical_deadline_class, currency_completion_fn callback, const void *context, size_t) {
    if (refuse_submit) return false;
    ++submissions; payment_delta = delta; memcpy(&payment_uid, context, sizeof(payment_uid));
    payment_callback = callback; return true;
}
bool item_creation_grant_submit_to_player_with_completion(P_char, P_obj object, P_char, P_obj,
    item_creation_grant_completion_fn callback, economic_source_kind, uint64_t) { pending_item = object; grant_callback = callback; return !refuse_submit; }
bool item_creation_grant_submit_to_player(P_char, P_obj, P_char, P_obj, economic_source_kind, uint64_t) { ++submissions; return true; }
'''

DRIVER = r'''
struct fixture {
    char_data player{}, keeper{}; pc_only_data pc{}; npc_only_data npc{};
    obj_data stock{}, bag{}, rock{};
    fixture(persistence_mode backend) {
        mode = backend; busy = refuse_submit = accounting_active = keeper_cash_recorded = false; exact = true; creation_index = 0;
        submissions = rooms = tells = refunds = 0; fixture_messages.clear(); produced_purchase_sequences.clear();
        payment_callback = nullptr; grant_callback = nullptr; trade_callback = nullptr;
        player.only.pc = &pc; pc.pid = 42; player.player.name = const_cast<char *>("Buyer");
        player.curr_stats.Dex = 200; player.curr_stats.Cha = 100; GET_COPPER(&player) = 1000;
        keeper.only.npc = &npc; keeper.specials.act = ACT_ISNPC; keeper.player.name = const_cast<char *>("Keeper");
        room.people = &keeper; shop = {}; shop.keeper = 0; shop.number_items_produced = 1;
        shop.producing[0] = 0; shop.sell_percent = 1; shop.message_buy = const_cast<char *>("%s %s");
        stock.obj_uid = 10; stock.R_num = 0; stock.cost = 2; stock.weight = 1;
        stock.name = const_cast<char *>("ration"); stock.short_description = const_cast<char *>("an iron ration");
        stock.loc_p = LOC_CARRIED; stock.loc.carrying = &keeper; keeper.carrying = &stock;
        bag.obj_uid = 20; bag.type = ITEM_CONTAINER; bag.value[0] = 50;
        bag.name = const_cast<char *>("backpack"); bag.short_description = const_cast<char *>("a leather backpack");
        bag.loc_p = LOC_CARRIED; bag.loc.carrying = &player; player.carrying = &bag;
        rock.obj_uid = 30; rock.name = const_cast<char *>("rock"); rock.short_description = const_cast<char *>("a rock");
        rock.loc_p = LOC_CARRIED; rock.loc.carrying = &player; bag.next_content = &rock;
        stock.next = &bag; bag.next = &rock; object_list = &stock;
    }
    void buy(const char *command) { char input[MAX_INPUT_LENGTH]; strcpy(input, command); shopping_buy(input, &player, &keeper, 0); }
    void complete(bool committed = true, bool publish = true, unsigned int error = 0) {
        if (mode == PERSISTENCE_MODE_FLATFILE_PRIMARY) {
            const auto payload = trade_payload; auto callback = trade_callback;
            shop_trade_result result{};
            if (committed) { result.shop_revision = 1; result.item_count = 1; GET_COPPER(&player) -= payload.price;
                result.keeper_cash_recorded = keeper_cash_recorded; result.keeper_cash = payload.expected_keeper_cash + payload.price; }
            exact = publish; callback(&player, committed, result, error, payload);
        } else {
            if (payment_callback) {
                auto callback = payment_callback; payment_callback = nullptr;
                if (committed) GET_COPPER(&player) += payment_delta;
                callback(&player, committed, {}, error, reinterpret_cast<const uint8_t *>(&payment_uid), sizeof(payment_uid));
                if (!committed) return;
            }
            if (!produced_purchase_sequences.count(42)) return;
            if (publish) {
                obj_to_char(pending_item, &player);
                if (produced_purchase_sequences.at(42).container_item_uid) { obj_from_char(pending_item); obj_to_obj(pending_item, &bag); }
            }
            auto callback = grant_callback; callback(&player, pending_item->obj_uid, committed, error);
        }
    }
};

int main() {
    const std::string quantity_error = "The quantity must be a whole number between 1 and 50; nothing was purchased or charged.\r\n";
    for (const char *count : {"0", "-1", "+2", "1x", "1.0", "no", "51", "99999999999999999999999999999", ""}) {
        for (const char *prefix : {"ration quantity ", "ration backpack "}) {
            if (!*count && std::string(prefix) == "ration backpack ") continue; // Historical single delivery.
            char input[MAX_INPUT_LENGTH]; snprintf(input, sizeof(input), "%s%s", prefix, count);
            shop_purchase_request request; assert(shop_purchase_parse(input, request) == quantity_error);
        }
    }
    for (const char *command : {"ration quantity 1", "ration quantity 50", "#3 quantity 10 into backpack", "ration backpack 2", "ration backpack", "ration quantity 2   ", "ration quantity 2 into 'leather backpack'"}) {
        char input[MAX_INPUT_LENGTH]; strcpy(input, command); shop_purchase_request request;
        assert(!shop_purchase_parse(input, request)); assert(request.quantity >= 1 && request.quantity <= 50);
        if (std::string(command).find("into") != std::string::npos) assert(*request.destination);
    }
    for (const auto backend : {PERSISTENCE_MODE_FLATFILE_PRIMARY, PERSISTENCE_MODE_MARIADB_PRIMARY}) {
        for (bool produced : {false, true}) {
            for (int item_type : {ITEM_FOOD, ITEM_DRINKCON}) {
                fixture f(backend); f.stock.type = item_type;
                if (!produced) shop.number_items_produced = 0;
                shopping_list(nullptr, &f.player, &f.keeper, 0);
                assert(fixture_messages.size() == 1 && submissions == 0);
                assert((fixture_messages[0].find("each [quantity 1-50]") != std::string::npos) == produced);
                assert((fixture_messages[0].find("Quantity purchase: buy <item> quantity <1-50> [into <container>]") != std::string::npos) == produced);
            }
        }
        for (const char *command : {"ration quantity", "ration quantity 0", "ration backpack 51", "ration quantity 2 into", "ration quantity 2 extra", "ration quantity 2 into backpack extra", "ration backpack 2 extra", "ration quantity 2 from", "ration quantity 2 ''", "ration backpack ''", "ration quantity 2 into missing", "ration quantity 2 into rock"}) {
            fixture f(backend); f.buy(command); assert(submissions == 0 && GET_MONEY(&f.player) == 1000);
            assert(fixture_messages.size() == 1 && fixture_messages[0].find("nothing was purchased or charged") != std::string::npos);
        }
        {
            fixture f(backend); f.bag.value[1] = CONT_CLOSED; f.buy("ration quantity 2 into backpack");
            assert(submissions == 0 && fixture_messages[0] == "Your destination container is closed; nothing was purchased or charged.\r\n");
        }
        {
            fixture f(backend); busy = true; f.buy("ration quantity 2"); assert(submissions == 0);
            assert(fixture_messages.size() == 1 && fixture_messages[0] == "Your previous purchase is still being processed; nothing new was purchased or charged.\r\n");
        }
        {
            fixture f(backend); refuse_submit = true; f.buy("ration quantity 2"); assert(submissions == 0);
            assert((fixture_messages.size() == 1 && fixture_messages[0].find("Purchase stopped: 0 of 2") != std::string::npos) ||
                   (fixture_messages.size() == 1 && fixture_messages[0] == "The shop is busy; nothing was purchased or charged. Please try again.\r\n"));
        }
        for (const char *command : {"ration quantity 2", "ration quantity 2 into backpack", "ration backpack 2", "#1 quantity 2"}) {
            fixture f(backend); f.buy(command); const bool container = std::string(command).find("backpack") != std::string::npos;
            const std::string destination = container ? "a leather backpack" : "your inventory";
            assert(fixture_messages.size() == 1 && fixture_messages[0] == "You order 2 copies of an iron ration at 2 copper each, for 4 copper total.\r\nDestination: " + destination + ".\r\n");
            f.complete(); assert(fixture_messages.size() == 1); assert(rooms == 1 && tells == 0);
            f.complete(); assert(fixture_messages.size() == 2 && rooms == 1 && tells == 0);
            assert(fixture_messages.back() == "Purchase complete: 2 of 2 copies of an iron ration delivered to " + destination + " for 4 copper.\r\n");
            assert(GET_MONEY(&f.player) == 996 && produced_purchase_sequences.empty());
        }
        {
            fixture f(backend); f.buy("ration quantity 5 into backpack"); f.complete(); f.complete();
            f.bag.value[0] = 3; f.complete();
            assert(fixture_messages.size() == 2 && rooms == 1 && GET_MONEY(&f.player) == 994);
            assert(fixture_messages.back() == "Purchase stopped: 3 of 5 copies of an iron ration delivered to a leather backpack for 6 copper; the remaining 2 were not charged because another copy will not fit in your destination container.\r\n");
        }
        {
            fixture f(backend); f.buy("ration quantity 2"); f.complete(false, true, ENOSPC);
            assert(fixture_messages.size() == 2 && rooms == 0 && GET_MONEY(&f.player) == 1000);
            const std::string reason = backend == PERSISTENCE_MODE_FLATFILE_PRIMARY ? "you do not have enough money for another copy" : "your payment was declined";
            assert(fixture_messages.back() == "Purchase stopped: 0 of 2 copies of an iron ration delivered to your inventory for 0 copper; the remaining 2 were not charged because " + reason + ".\r\n");
        }
        {
            fixture f(backend); f.buy("ration quantity 2"); f.complete(true, false);
            assert(fixture_messages.size() == 2 && rooms == 0 && refunds == 0 && GET_MONEY(&f.player) == 998);
            assert(fixture_messages.back() == "Purchase delivery pending: 0 of 2 copies of an iron ration delivered to your inventory; 2 copper charged in total. 1 more copy was charged and is still being delivered. The remaining 1 were not charged.\r\nYour purchase is safe. Please wait a moment or reconnect; do not purchase it again.\r\n");
        }
        {
            fixture f(backend); shop.number_items_produced = 0; f.buy("ration quantity 2");
            assert(submissions == 0 && fixture_messages[0] == "Quantity and container purchases require produced stock; nothing was purchased or charged.\r\n");
        }
        {
            fixture f(backend); f.buy("ration quantity 50");
            for (int copy = 0; copy < 50; ++copy) f.complete();
            assert(submissions == 50 && fixture_messages.size() == 2 && rooms == 1 && tells == 0);
            assert(GET_MONEY(&f.player) == 900);
            assert(fixture_messages.back() == "Purchase complete: 50 of 50 copies of an iron ration delivered to your inventory for 100 copper.\r\n");
        }
        {
            fixture f(backend); f.player.player.level = MAXLVLMORTAL + 1;
            f.buy("ration quantity 2"); f.complete(); f.complete();
            assert(GET_MONEY(&f.player) == 1000 && fixture_messages.size() == 2);
            assert(fixture_messages[0].find("at 0 copper each, for 0 copper total") != std::string::npos);
            assert(fixture_messages.back().find("for 0 copper.") != std::string::npos);
        }
        {
            fixture f(backend); f.buy("ration quantity 1"); f.complete(); assert(fixture_messages.size() == 2);
            assert(fixture_messages.back().find("Purchase complete: 1 of 1") == 0);
        }
    }
    {
        fixture f(PERSISTENCE_MODE_FLATFILE_PRIMARY); keeper_cash_recorded = true;
        f.buy("ration quantity 2"); f.complete(); f.complete();
        assert(GET_MONEY(&f.keeper) == 4); // Native keeper cash is published once, never added again.
    }
    for (const auto backend : {PERSISTENCE_MODE_FLATFILE_PRIMARY, PERSISTENCE_MODE_MARIADB_PRIMARY}) {
        fixture f(backend); accounting_active = true;
        f.buy("ration quantity 2"); assert(submissions == 0 && GET_MONEY(&f.player) == 1000);
        assert(fixture_messages.size() == 1 && fixture_messages[0] ==
            "Shop trades and services are unavailable while economic accounting is active.\r\n");
    }
    assert(shop_purchase_price(0) == "0 copper");
    assert(shop_purchase_price(50LL * std::numeric_limits<int>::max()) == "107374182350 copper");
    puts("shop purchase parser, command and completion messages passed");
}
'''


def main():
    epic = extract_function("world/epic.c", "int stat_shops(")
    epic_buy = epic[epic.index("else if (cmd == CMD_BUY)"):]
    assert epic_buy.index("without extra arguments") < epic_buy.index("SUB_MONEY(")
    buy = definition("void shopping_buy(")
    assert buy.index("shop_purchase_parse(arg, request)") < buy.index("persistence_mode_get()")
    listing = definition("void shopping_list(")
    assert 'each [quantity 1-50]' in listing and 'Quantity purchase: buy <item> quantity <1-50> [into <container>]' in listing
    for name in ["help/duris_help.hlp", "help/duris_help_parsed.hlp", "docs/reference/BATCH_ITEM_COMMANDS.md"]:
        help_text = (ROOT / name).read_text(encoding="utf-8")
        assert "buy <item> quantity <1-50>" in help_text and "epic shop" in help_text
        assert "up to 9 of an item" not in help_text
    # Preserve the actual parser/token conventions and callback bodies; only
    # authority submission, world lookup and output are replaced by fixtures.
    functions = ["static bool refuse_unported_shop_mutation(", "static P_obj shop_trade_find_object(", "static P_char shop_trade_find_keeper(",
                 "static bool shop_trade_container_accepts(", "static bool shop_purchase_count(",
                 "static const char *shop_purchase_parse(", "static std::string shop_purchase_price(",
                 "static void shop_purchase_acknowledge(", "static void shop_purchase_report(",
                 "static const char *shop_purchase_stop_reason(", "static bool shop_creation_refund(",
                 "static bool shop_creation_submit_grant(", "static bool shop_creation_submit_produced_continuation(",
                 "static bool shop_trade_submit_produced_continuation(", "static bool shop_trade_submit_invalid_cleanup(",
                 "static bool shop_trade_route_invalid_cleanup(", "static void shop_trade_completion(",
                 "static void shop_creation_grant_completion(", "static void shop_creation_payment_completion(",
                 "void shopping_buy(", "void shopping_list(", "int shop_producing("]
    declarations = "\n".join(definition(name).split("{")[0].rstrip() + ";" for name in functions)
    # Default arguments belong in one declaration only.
    definitions = "\n".join(definition(name) for name in functions).replace(
        'const char *reason = "the shop could not complete delivery"', 'const char *reason').replace(
        'bool delayed = false', 'bool delayed')
    structs = SHOP[SHOP.index("struct produced_purchase_sequence"):SHOP.index("void lore_item(")]
    request = definition("struct shop_purchase_request") + ";"
    harness = "\n".join([PRELUDE, structs, request,
                         extract_function("cmd/interp.c", "char *one_argument("),
                         extract_function("cmd/interp.c", "char *lohrr_chop("),
                         declarations, definitions, DRIVER])
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="shop-usability-", dir=build_root) as directory:
        cpp = Path(directory) / "shop_usability.cpp"
        binary = Path(directory) / "shop_usability"
        cpp.write_text(harness, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-ffunction-sections", "-fdata-sections", "-Isrc", "-I/usr/include/libxml2",
                        "-I/usr/include/mysql", str(cpp), "-Wl,--gc-sections", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
