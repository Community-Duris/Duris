#!/usr/bin/env python3
"""Runtime regression for confirmation-required commands on linkdead players."""

from pathlib import Path
import subprocess
import sys
import tempfile

from _paths import extract_function, source


INTERP = source("cmd/interp.c").read_text(encoding="utf-8")
PREPARE = extract_function(
    "cmd/interp.c",
    "static bool prepare_command_confirmation(P_char character",
)

# Keep the interpreter wired through the tested gate.  In particular, a
# descriptorless, unconfirmed command must return before its handler runs.
dispatch_start = INTERP.index("// Execute the bloody thing!!!")
dispatch_end = INTERP.index("while (*(argument + begin + look_at) == ' ')", dispatch_start)
dispatch = INTERP[dispatch_start:dispatch_end]
assert "if (!prepare_command_confirmation(" in dispatch
assert dispatch.index("if (!prepare_command_confirmation(") < dispatch.index("return;")
assert dispatch.index("return;") < dispatch.index("command_pointer")


HARNESS = r'''
#include <cassert>
#include <cstring>

#define FALSE false
#define CONFIRM_NONE 0
#define CONFIRM_AWAIT 1
#define CONFIRM_DONE 2

struct descriptor_data
{
    unsigned char confirm_state;
    char last_command[256];
};

struct char_data
{
    bool npc;
    descriptor_data *desc;
};

using P_char = char_data *;
#define IS_NPC(character) ((character)->npc)

bool command_confirm = false;
'''

DRIVER = r'''
int main()
{
    char_data linkdead = {false, nullptr};
    assert(!prepare_command_confirmation(&linkdead, "suicide", ""));
    assert(!command_confirm);
    assert(linkdead.desc == nullptr);

    assert(prepare_command_confirmation(&linkdead, "suicide confirm", "confirm"));
    assert(command_confirm);
    assert(linkdead.desc == nullptr);

    descriptor_data descriptor = {};
    char_data linked = {false, &descriptor};
    assert(prepare_command_confirmation(&linked, "suicide", ""));
    assert(!command_confirm);
    assert(descriptor.confirm_state == CONFIRM_AWAIT);
    assert(strcmp(descriptor.last_command, "suicide") == 0);

    descriptor.confirm_state = CONFIRM_DONE;
    assert(prepare_command_confirmation(&linked, "suicide", ""));
    assert(command_confirm);
    assert(descriptor.confirm_state == CONFIRM_NONE);

    descriptor.confirm_state = CONFIRM_AWAIT;
    assert(prepare_command_confirmation(&linked, "suicide confirm", "confirm"));
    assert(command_confirm);
    assert(descriptor.confirm_state == CONFIRM_NONE);

    char_data npc = {true, nullptr};
    assert(prepare_command_confirmation(&npc, "suicide", ""));
    assert(command_confirm);

    assert(!prepare_command_confirmation(nullptr, "suicide", ""));
    assert(!command_confirm);
    return 0;
}
'''


def main() -> int:
    harness = "\n".join([HARNESS, PREPARE, DRIVER])
    with tempfile.TemporaryDirectory() as directory:
        harness_source = Path(directory) / "linkdead_command_confirmation.cpp"
        binary = Path(directory) / "linkdead_command_confirmation"
        harness_source.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-g",
                "-fsanitize=address,undefined",
                str(harness_source),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("linkdead command confirmation regression passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
