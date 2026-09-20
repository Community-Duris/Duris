#!/usr/bin/env python3
"""Exercise the command-level meditation interruption policy."""

from pathlib import Path
import subprocess
import tempfile

from _paths import SRC


def extract(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unbalanced braces reading {signature}")


interp = (SRC / "cmd" / "interp.c").read_text(encoding="utf-8", errors="replace")
policy = extract(interp, "static bool command_preserves_meditation(")

assert "command_preserves_meditation(" in interp
assert "argument + begin + look_at" in interp
assert "stop_meditation(victim);" in (SRC / "combat" / "fight.c").read_text(
    encoding="utf-8", errors="replace"
)
assert "if (!IS_AFFECTED(ch, AFF_MEDITATE))" in (SRC / "cmd" / "actnew.c").read_text(
    encoding="utf-8", errors="replace"
)

commands = {
    "CMD_LOOK": 1,
    "CMD_GLANCE": 2,
    "CMD_EXITS": 3,
    "CMD_LISTEN": 4,
    "CMD_INVENTORY": 5,
    "CMD_EQUIPMENT": 6,
    "CMD_SCORE": 7,
    "CMD_STAT": 8,
    "CMD_TIME": 9,
    "CMD_WEATHER": 10,
    "CMD_WHO": 11,
    "CMD_READ": 12,
    "CMD_EXAMINE": 13,
    "CMD_GROUP": 14,
    "CMD_PUT": 15,
    "CMD_GET": 16,
}

prelude = """
#include <cassert>
#include <cctype>
#include <cstring>

#define MAX_INPUT_LENGTH 512
#define COIN_NONE -1
typedef unsigned int uint;

""" + "\n".join(f"#define {name} {value}" for name, value in commands.items()) + r'''

static char *one_argument(const char *argument, char *first_arg)
{
    static const char *const fill_words[] = {
        "in", "from", "with", "the", "on", "at", "to", nullptr,
    };
    if (!argument) {
        *first_arg = '\0';
        return nullptr;
    }
    do {
        while (*argument && std::isspace(static_cast<unsigned char>(*argument)))
            ++argument;
        char *word = first_arg;
        while (*argument && *argument > ' ')
            *word++ = static_cast<char>(std::tolower(static_cast<unsigned char>(*argument++)));
        *word = '\0';
        bool fill_word = false;
        for (const char *const *fill = fill_words; *fill; ++fill)
            if (!std::strcmp(first_arg, *fill)) {
                fill_word = true;
                break;
            }
        if (!fill_word)
            return const_cast<char *>(argument);
    } while (*argument || *first_arg);
    *first_arg = '\0';
    return const_cast<char *>(argument);
}

static bool is_number(char *value)
{
    if (!value || !*value)
        return false;
    if (*value == '-')
        ++value;
    for (; *value; ++value)
        if (!std::isdigit(static_cast<unsigned char>(*value)))
            return false;
    return true;
}

static int coin_type(char *value)
{
    static const char *const coins[] = {"copper", "silver", "gold", "platinum"};
    for (const char *coin : coins) {
        const std::size_t length = std::strlen(value);
        if (length && length <= std::strlen(coin) && !std::strncmp(value, coin, length))
            return 0;
    }
    return COIN_NONE;
}
'''

driver = r'''
int main()
{
    const int passive[] = {
        CMD_LOOK, CMD_GLANCE, CMD_EXITS, CMD_LISTEN, CMD_INVENTORY,
        CMD_EQUIPMENT, CMD_SCORE, CMD_STAT, CMD_TIME, CMD_WEATHER, CMD_WHO,
        CMD_READ, CMD_EXAMINE, CMD_GROUP,
    };
    for (int command : passive)
        assert(command_preserves_meditation(command, ""));

    assert(command_preserves_meditation(CMD_PUT, "all.coins bag"));
    assert(command_preserves_meditation(CMD_PUT, "all.coins in bag"));
    assert(command_preserves_meditation(CMD_PUT, "100 gold bag"));
    assert(command_preserves_meditation(CMD_PUT, "25 g in bag"));
    assert(command_preserves_meditation(CMD_PUT, "1 platinum satchel"));
    assert(!command_preserves_meditation(CMD_PUT, "all potions bag"));
    assert(!command_preserves_meditation(CMD_PUT, "100 apples bag"));
    assert(!command_preserves_meditation(CMD_PUT, "100 gold"));
    assert(!command_preserves_meditation(CMD_PUT, "0 gold bag"));
    assert(!command_preserves_meditation(CMD_PUT, "-1 gold bag"));
    assert(!command_preserves_meditation(CMD_PUT, "10000000 gold bag"));
    assert(!command_preserves_meditation(CMD_PUT, ""));
    assert(command_preserves_meditation(CMD_GROUP, ""));
    assert(!command_preserves_meditation(CMD_GROUP, "invite friend"));
    assert(!command_preserves_meditation(CMD_GET, "all coins"));
    return 0;
}
'''

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "meditation_policy.cpp"
    binary = Path(directory) / "meditation_policy"
    source.write_text(prelude + policy + driver, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", str(source), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("meditation interruption policy checks passed")
