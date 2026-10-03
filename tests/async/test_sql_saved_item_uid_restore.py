#!/usr/bin/env python3
"""Require exact retained UIDs before SQL saved-item prototypes are materialized."""

from pathlib import Path
import subprocess
import tempfile

from _paths import source


def function_definition(signature: str) -> str:
    text = source("sql_player.c").read_text(encoding="utf-8")
    start = text.rindex(signature)
    depth = 0
    for end in range(text.index("{", start), len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
            if not depth:
                return text[start:end + 1]
    raise AssertionError(signature)


def main() -> None:
    parse_uid = function_definition("static bool sql_parse_persisted_item_uid(")
    player = function_definition("static bool sql_load_player_items(")
    filtered = function_definition("static P_obj sql_load_locker_items_filtered(")
    private_chest = function_definition("void sql_load_private_chest_items(")

    for loader in (filtered, private_chest):
        parse_at = loader.index("sql_parse_persisted_item_uid(row[20], &saved_uid)")
        owner_at = loader.index("sql_persistence_item_owner_matches_identity(")
        template_at = loader.index("real_object(vnum)")
        allocation_at = loader.index("read_object(rnum, REAL)")
        assert parse_at < owner_at < template_at < allocation_at
        assert "strtoul(row[20]" not in loader

    parse_at = player.index("sql_parse_persisted_item_uid(row[28], &saved_uid)")
    owner_at = player.index("sql_persistence_item_owner_matches(")
    allocation_at = player.index("read_object(vnum, VIRTUAL)")
    assert parse_at < owner_at < allocation_at
    # UID ownership is checked before allocation. A malformed optional runtime
    # payload must still discard the newly allocated object and refuse the load.
    runtime_at = player.index("player_item_snapshot_list_decode(", allocation_at)
    discard_at = player.index("extract_obj(obj, FALSE)", runtime_at)
    assert allocation_at < runtime_at < discard_at
    assert "return false;" in player[discard_at:player.index("player_load_item_runtime_state_apply", discard_at)]

    program = r'''
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <system_error>
''' + parse_uid + r'''
int main() {
    uint64_t uid = 17;
    assert(sql_parse_persisted_item_uid("1", &uid) && uid == 1);
    assert(sql_parse_persisted_item_uid("18446744073709551615", &uid) &&
           uid == 18446744073709551615ULL);
    const char *invalid_values[] = {nullptr, "", "0", "-1", "+1", " 1", "1x",
                                    "18446744073709551616"};
    for (const char *invalid : invalid_values) {
        uid = 17;
        assert(!sql_parse_persisted_item_uid(invalid, &uid) && uid == 17);
    }
    assert(!sql_parse_persisted_item_uid("1", nullptr));
}
'''
    with tempfile.TemporaryDirectory() as directory:
        source_file = Path(directory) / "sql_saved_item_uid_restore.cpp"
        binary = source_file.with_suffix("")
        source_file.write_text(program, encoding="utf-8")
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", str(source_file), "-o", str(binary)],
                       check=True)
        subprocess.run([str(binary)], check=True)
    print("SQL saved-item UID restoration: ok")


if __name__ == "__main__":
    main()
