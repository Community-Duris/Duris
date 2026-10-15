#!/usr/bin/env python3
"""Exercise the shop quote adapter with do_tell's actual parser and name matcher.

The C++ fixture compiles the current private shop adapter and the unchanged
half_chop/isname function bodies directly from maintained server sources. Its
transport sink records bytes only; this is not a game/descriptor/receipt test.
The connected MySQL shop journey remains the actual delivery/persistence test.
"""
import argparse
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
    args = parser.parse_args()
    build = args.build_dir or Path(os.environ.get("BIN_ROOT", ROOT / "bin")) / "tests" / "shop_quote_recipient"
    build.mkdir(parents=True, exist_ok=True)
    shop = (SRC / "economy/shop.c").read_text()
    comm = (SRC / "cmd/actcomm.c").read_text()
    interp = (SRC / "cmd/interp.c").read_text()
    handler = (SRC / "world/handler.c").read_text()
    utils = (SRC / "core/utils.h").read_text()
    structs = (SRC / "core/structs.h").read_text()
    tell = function(comm, "void do_tell(P_char ch, char *argument,")
    assert "half_chop(argument, name, message);" in tell
    assert "isname(d->character->player.name, name)" in tell
    adapter = function(shop, "static void shop_tell_customer(")
    # All five purchase/sale quote routes must supply the real customer's name.
    for signature, quote in (
        ("static void shop_trade_completion_impl(", "shop_index[payload.shop_id].message_buy"),
        ("static void shop_creation_grant_completion(P_char ch, uint64_t item_uid, bool committed,", "shop_index[sequence.shop_id].message_buy"),
        ("void shopping_buy(", "shop_index[shop_nr].message_buy"),
        ("static void shop_trade_completion_impl(", "shop_index[payload.shop_id].message_sell"),
        ("void shopping_sell(", "shop_index[shop_nr].message_sell"),
    ):
        start = shop.rindex(signature) if "shop_creation_grant_completion" in signature else shop.index(signature)
        body = function(shop[start:], signature)
        quoted = body.index(quote)
        assert re.search(r"shop_tell_customer\(keeper,\s*GET_NAME\(ch\),\s*(?:message|Gbuf1)\)", body[quoted:quoted + 400])
    lower = re.search(r"^#define LOWER\(c\).*$", utils, re.M).group(0)
    log_sys = re.search(r"^#define LOG_SYS .*?$", structs, re.M).group(0)
    template = r"""
#include "core/config.h"
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
    generated = build / "shop_quote_recipient.cpp"
    generated.write_text(cpp)
    binary = build / "shop_quote_recipient"
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-O0", "-UNDEBUG",
                    "-I", str(SRC), str(generated), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("shop quote actual-parser regression: PASS")


if __name__ == "__main__":
    main()
