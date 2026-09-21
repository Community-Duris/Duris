#!/usr/bin/env python3
"""Regression coverage for duplicate shopkeeper callback assignment."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/economy/shop.c").read_text()


def function_body(signature: str) -> str:
    start = SOURCE.index(signature)
    brace = SOURCE.index("{", start)
    depth = 0
    for index in range(brace, len(SOURCE)):
        if SOURCE[index] == "{":
            depth += 1
        elif SOURCE[index] == "}":
            depth -= 1
            if depth == 0:
                return SOURCE[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


production = function_body("void assign_the_shopkeepers(void)")
harness = f"""
#include <cassert>

struct Character {{}};
using P_char = Character *;
using special_function = int (*)(P_char, P_char, int, char *);

struct mobile_index_data
{{
    struct
    {{
        special_function mob = nullptr;
    }} func;
}};

struct shop_data
{{
    int keeper = 0;
    special_function func = nullptr;
}};

mobile_index_data mob_index[2];
shop_data shops[3];
shop_data *shop_index = shops;
int number_of_shops = 3;

#define SHOP_FUNC(index) (shop_index[(index)].func)

int secondary_special(P_char, P_char, int, char *)
{{
    return 1;
}}

int shop_keeper(P_char, P_char, int, char *)
{{
    return 0;
}}

{production}

int main()
{{
    shop_index[0].keeper = 0;
    shop_index[1].keeper = 0;
    shop_index[2].keeper = 1;
    mob_index[0].func.mob = secondary_special;

    assign_the_shopkeepers();
    assert(mob_index[0].func.mob == shop_keeper);
    assert(SHOP_FUNC(0) == secondary_special);
    assert(SHOP_FUNC(1) == secondary_special);
    assert(SHOP_FUNC(1) != shop_keeper);
    assert(mob_index[1].func.mob == shop_keeper);
    assert(SHOP_FUNC(2) == nullptr);

    // Reassignment is idempotent and must never capture the dispatcher.
    assign_the_shopkeepers();
    assert(SHOP_FUNC(0) == secondary_special);
    assert(SHOP_FUNC(1) == secondary_special);
    assert(SHOP_FUNC(1) != shop_keeper);
    assert(SHOP_FUNC(2) == nullptr);
}}
"""

with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / "shopkeeper_secondary_binding.cpp"
    binary = Path(directory) / "shopkeeper_secondary_binding"
    source.write_text(harness)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            str(source),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("Duplicate shopkeepers preserve their secondary callback without recursion")
