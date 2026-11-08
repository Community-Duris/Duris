#!/usr/bin/env python3
"""Exercise the shop quote adapter with do_tell's actual parser and name matcher.

The C++ fixture compiles the current private shop adapter and the unchanged
half_chop/isname and checked_substitute function bodies directly from maintained
server sources, including configured refusal messages. Its
transport sink records bytes only; this is not a game/descriptor/receipt test.
The connected MySQL shop journey remains the actual delivery/persistence test.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def function(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if not depth:
                return text[start:index + 1]
    raise AssertionError("unterminated actual function: " + signature)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--emit-only", action="store_true", help="construct fixture and check sources without compiling or executing C++")
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    src = source_root / "src"
    build = args.build_dir or Path(os.environ.get("BIN_ROOT", ROOT / "bin")) / "tests" / "shop_quote_recipient"
    build.mkdir(parents=True, exist_ok=True)
    shop = (src / "economy/shop.c").read_text()
    comm = (src / "cmd/actcomm.c").read_text()
    interp = (src / "cmd/interp.c").read_text()
    handler = (src / "world/handler.c").read_text()
    utils = (src / "core/utils.h").read_text()
    structs = (src / "core/structs.h").read_text()
    safe = (src / "core/safe_format.c").read_text()
    # Use the authentic fixture shop's configured no-item and keeper-cash bodies.
    shop_data = (source_root / "areas/shp/opalphoenix.shp").read_text()
    fields = [line[:-1] for line in shop_data.splitlines() if line.endswith("~")]
    assert fields[0] == "#70801"
    no_such_item, missing_cash = fields[1], fields[4]
    tell = function(comm, "void do_tell(P_char ch, char *argument,")
    assert "half_chop(argument, name, message);" in tell
    assert "isname(d->character->player.name, name)" in tell
    adapter = function(shop, "static void shop_tell_customer(")
    # All five purchase/sale quote routes must supply the real customer's name.
    for signature, quote in (
        ("static void shop_trade_completion(P_char ch, bool committed", "shop_index[payload.shop_id].message_buy"),
        ("static void shop_creation_grant_completion(P_char ch, uint64_t item_uid, bool committed,", "shop_index[sequence.shop_id].message_buy"),
        ("void shopping_buy(", "shop_index[shop_nr].message_buy"),
        ("static void shop_trade_completion(P_char ch, bool committed", "shop_index[payload.shop_id].message_sell"),
        ("void shopping_sell(", "shop_index[shop_nr].message_sell"),
    ):
        start = shop.rindex(signature) if ("shop_creation_grant_completion" in signature or "shop_trade_completion" in signature) else shop.index(signature)
        body = function(shop[start:], signature)
        quoted = body.index(quote)
        assert re.search(r"shop_tell_customer\(keeper,\s*GET_NAME\(ch\),\s*(?:message|Gbuf1)\)", body[quoted:quoted + 400])
    # Every direct configured-template tell refusal supplies an explicit recipient.
    for signature, field, expected in (
        ("void shopping_buy(", "no_such_item1", 2),
        ("void shopping_sell(", "missing_cash1", 1),
        ("void shopping_peruse(", "no_such_item1", 2),
    ):
        body = function(shop, signature)
        send = (r"checked_substitute\(Gbuf1,\s*MAX_STRING_LENGTH,\s*"
                r"shop_index\[shop_nr\]\." + field +
                r",\s*GET_NAME\(ch\)\);\s*"
                r"shop_tell_customer\(keeper,\s*GET_NAME\(ch\),\s*Gbuf1\);")
        assert len(re.findall(send, body)) == expected
    # Explicitly addressed, non-template prompts keep their original one prefix.
    buy_body = function(shop, "void shopping_buy(")
    assert re.search(r'"%s what do you want to buy\?\?",\s*GET_NAME\(ch\)\);\s*do_tell\(keeper, Gbuf1, 0\);', buy_body)
    lower = re.search(r"^#define LOWER\(c\).*$", utils, re.M).group(0)
    log_sys = re.search(r"^#define LOG_SYS .*?$", structs, re.M).group(0)
    template = r"""
#include "core/config.h"
#include "core/safe_format.h"
#include <climits>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
@LOWER@
@LOG_SYS@
struct char_data;
using P_char = char_data *;
static unsigned parse_errors = 0, tell_calls = 0;
static std::string recipient, delivered, target;
static bool recipient_found = false;
void logit(const char *, const char *, ...) { ++parse_errors; }
@HALF_CHOP@
@ISNAME@
void do_tell(P_char, char *argument, int)
{
    char name[MAX_INPUT_LENGTH], message[MAX_STRING_LENGTH];
    half_chop(argument, name, message);
    ++tell_calls;
    recipient = name;
    delivered = message;
    recipient_found = isname(target.c_str(), name);
}
@ADAPTER@
@SUBSTITUTE_COPY@
@CHECKED_SUBSTITUTE@
int main()
{
    target = "Taverek";
    const std::string buy = "Thank you, Taverek, that will be 6 platinum, 5 gold, 4 silver, and 9 copper.";
    const std::string sell = "Taverek, I will give you 4 platinum for that.";
    // Exact old callers parse the configured sentence as a recipient.
    char bare_buy[MAX_INPUT_LENGTH], bare_sell[MAX_INPUT_LENGTH];
    std::snprintf(bare_buy, sizeof(bare_buy), "%s", buy.c_str());
    std::snprintf(bare_sell, sizeof(bare_sell), "%s", sell.c_str());
    do_tell(nullptr, bare_buy, 0);
    assert(recipient == "Thank" && !recipient_found);
    do_tell(nullptr, bare_sell, 0);
    assert(recipient == "Taverek," && !recipient_found);
    for (const auto &quote : {buy, sell, std::string("Quoted 100% literally, Taverek.")})
    {
        tell_calls = 0;
        shop_tell_customer(nullptr, target.c_str(), quote.c_str());
        assert(tell_calls == 1 && recipient == target && recipient_found);
        assert(delivered == quote);
    }
    // A shared caller quote buffer remains intact while do_tell parses its own argument.
    char shared_quote[MAX_STRING_LENGTH];
    std::snprintf(shared_quote, sizeof(shared_quote), "%s", buy.c_str());
    shop_tell_customer(nullptr, target.c_str(), shared_quote);
    assert(delivered == buy && std::string(shared_quote) == buy);
    // Actual configured refusals are DATA templates, expanded by the exact
    // maintained formatter before the same parser/adapter. The original calls
    // misdirect "Do" or "Sorry" instead of delivering to the named customer.
    const std::string no_item_template = @NO_ITEM_TEMPLATE@;
    const std::string cash_template = @CASH_TEMPLATE@;
    for (const auto &format : {no_item_template, cash_template,
                             std::string("Sorry 100%% %n %d, %s.")})
    {
        char shared_refusal[MAX_STRING_LENGTH];
        const int size = checked_substitute(shared_refusal, sizeof(shared_refusal),
                                            format.c_str(), target.c_str());
        const std::string before = shared_refusal;
        assert(size == static_cast<int>(before.size()));
        char old_argument[MAX_INPUT_LENGTH];
        assert(size >= 0 && static_cast<size_t>(size) < sizeof(old_argument));
        std::memcpy(old_argument, shared_refusal, static_cast<size_t>(size) + 1);
        assert(std::string(old_argument) == before);
        do_tell(nullptr, old_argument, 0);
        assert(!recipient_found);
        if (format == no_item_template)
            assert(recipient == "Do" && before == "Do you see that here, Taverek?");
        if (format == cash_template)
            assert(recipient == "Sorry" && before == "Sorry there Taverek, but I don't have the money.");
        if (format == "Sorry 100%% %n %d, %s.")
            assert(before == "Sorry 100% %n %d, Taverek.");
        tell_calls = 0;
        shop_tell_customer(nullptr, target.c_str(), shared_refusal);
        assert(tell_calls == 1 && recipient == target && recipient_found);
        assert(delivered == before && std::string(shared_refusal) == before);
    }
    // Respect the callee's actual input bound: no truncated quote is dispatched.
    const std::string largest(MAX_INPUT_LENGTH - target.size() - 2, 'q');
    tell_calls = 0;
    shop_tell_customer(nullptr, target.c_str(), largest.c_str());
    assert(tell_calls == 1 && recipient_found && delivered == largest);
    const std::string oversized = largest + 'q';
    tell_calls = 0;
    shop_tell_customer(nullptr, target.c_str(), oversized.c_str());
    assert(tell_calls == 0);
    assert(parse_errors == 0);
}
"""
    cpp = template.replace("@LOWER@", lower).replace("@LOG_SYS@", log_sys)
    cpp = cpp.replace("@HALF_CHOP@", function(interp, "void half_chop(char *string, char *arg1, char *arg2)"))
    cpp = cpp.replace("@ISNAME@", function(handler, "bool isname(const char *str, const char *namelist)"))
    cpp = cpp.replace("@ADAPTER@", adapter)
    cpp = cpp.replace("@SUBSTITUTE_COPY@", function(safe, "static void substitute_copy("))
    cpp = cpp.replace("@CHECKED_SUBSTITUTE@", function(safe, "int checked_substitute_strings("))
    cpp = cpp.replace("@NO_ITEM_TEMPLATE@", json.dumps(no_such_item))
    cpp = cpp.replace("@CASH_TEMPLATE@", json.dumps(missing_cash))
    generated = build / "shop_quote_recipient.cpp"
    generated.write_text(cpp)
    if args.emit_only:
        print("shop quote/refusal source contracts and actual-parser fixture construction: PASS; C++ UNRUN")
        return
    binary = build / "shop_quote_recipient"
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O0", "-UNDEBUG",
                    "-I", str(src), str(generated), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("shop quote actual-parser regression: PASS")


if __name__ == "__main__":
    main()
