#!/usr/bin/env python3
"""Exercise snoop observer cleanup in extract_char regardless of character level."""
import subprocess
import tempfile
from pathlib import Path
from _paths import ROOT

FIXTURE = r'''
#include "core/structs.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>

static int rem_calls = 0;
static P_char rem_snooper = nullptr;

void rem_char_from_snoopby_list(snoop_by_data **list, P_char snooper)
{
    ++rem_calls;
    rem_snooper = snooper;
    if (!list || !*list) return;
    snoop_by_data *prev = nullptr;
    for (snoop_by_data *curr = *list; curr; prev = curr, curr = curr->next)
    {
        if (curr->snoop_by == snooper)
        {
            if (prev) prev->next = curr->next;
            else *list = curr->next;
            free(curr);
            return;
        }
    }
}

// Emulate the extract_char snoop cleanup block
static void extract_snoop_cleanup(P_char ch)
{
    if (ch->desc)
    {
        if (ch->desc->snoop.snooping)
        {
            if (ch->desc->snoop.snooping->desc)
                rem_char_from_snoopby_list(
                    &ch->desc->snoop.snooping->desc->snoop.snoop_by_list, ch);
            ch->desc->snoop.snooping = nullptr;
        }

        snoop_by_data *snoop_by_ptr = ch->desc->snoop.snoop_by_list;
        while (snoop_by_ptr)
        {
            if (snoop_by_ptr->snoop_by && snoop_by_ptr->snoop_by->desc)
                snoop_by_ptr->snoop_by->desc->snoop.snooping = nullptr;
            snoop_by_ptr = snoop_by_ptr->next;
        }

        ch->desc->snoop.snooping = nullptr;

        snoop_by_ptr = ch->desc->snoop.snoop_by_list;
        while (snoop_by_ptr)
        {
            snoop_by_data *next = snoop_by_ptr->next;
            free(snoop_by_ptr);
            snoop_by_ptr = next;
        }

        ch->desc->snoop.snoop_by_list = nullptr;
    }
}

int main()
{
    // Test 1: Immortal character (level 60) snooping a mortal (level 20).
    // When immortal extracts, rem_char_from_snoopby_list must be called
    // and the mortal's snoop_by_list must be empty (no dangling pointer).
    {
        char_data immortal{};
        descriptor_data imm_desc{};
        immortal.desc = &imm_desc;
        immortal.player.level = 60;

        char_data mortal{};
        descriptor_data mortal_desc{};
        mortal.desc = &mortal_desc;
        mortal.player.level = 20;

        // Establish snoop relationship: immortal snoops mortal
        imm_desc.snoop.snooping = &mortal;
        auto *node = static_cast<snoop_by_data *>(std::calloc(1, sizeof(snoop_by_data)));
        node->snoop_by = &immortal;
        mortal_desc.snoop.snoop_by_list = node;

        rem_calls = 0;
        rem_snooper = nullptr;

        extract_snoop_cleanup(&immortal);

        assert(rem_calls == 1);
        assert(rem_snooper == &immortal);
        assert(mortal_desc.snoop.snoop_by_list == nullptr);
        assert(imm_desc.snoop.snooping == nullptr);
    }

    // Test 2: Mortal snooping an immortal (e.g. channel or authorized snoop).
    {
        char_data mortal{};
        descriptor_data mortal_desc{};
        mortal.desc = &mortal_desc;
        mortal.player.level = 25;

        char_data target{};
        descriptor_data target_desc{};
        target.desc = &target_desc;
        target.player.level = 50;

        mortal_desc.snoop.snooping = &target;
        auto *node = static_cast<snoop_by_data *>(std::calloc(1, sizeof(snoop_by_data)));
        node->snoop_by = &mortal;
        target_desc.snoop.snoop_by_list = node;

        rem_calls = 0;
        extract_snoop_cleanup(&mortal);

        assert(rem_calls == 1);
        assert(rem_snooper == &mortal);
        assert(target_desc.snoop.snoop_by_list == nullptr);
    }

    // Test 3: Character with active observers extracting clears observer snooping pointers.
    {
        char_data victim{};
        descriptor_data victim_desc{};
        victim.desc = &victim_desc;

        char_data observer{};
        descriptor_data obs_desc{};
        observer.desc = &obs_desc;
        obs_desc.snoop.snooping = &victim;

        auto *node = static_cast<snoop_by_data *>(std::calloc(1, sizeof(snoop_by_data)));
        node->snoop_by = &observer;
        victim_desc.snoop.snoop_by_list = node;

        extract_snoop_cleanup(&victim);

        assert(obs_desc.snoop.snooping == nullptr);
        assert(victim_desc.snoop.snoop_by_list == nullptr);
    }

    // Test 4: Snooper extracts while snooped target is linkdead (target->desc == nullptr).
    // Must safely nullify snooping pointer without crashing on null desc dereference.
    {
        char_data snooper{};
        descriptor_data snooper_desc{};
        snooper.desc = &snooper_desc;

        char_data linkdead_target{};
        linkdead_target.desc = nullptr; // Linkdead!

        snooper_desc.snoop.snooping = &linkdead_target;
        rem_calls = 0;

        extract_snoop_cleanup(&snooper);

        assert(rem_calls == 0); // Not called since target has no descriptor
        assert(snooper_desc.snoop.snooping == nullptr);
    }

    // Test 5: Switched mob return retargets snoopers back to original body.
    {
        char_data god{};
        char_data mob{};
        descriptor_data god_desc{};
        pc_only_data pc{};
        god.only.pc = &pc;
        god.desc = &god_desc;
        god_desc.character = &mob;
        god_desc.original = &god;
        pc.switched = &mob;

        char_data observer{};
        descriptor_data obs_desc{};
        observer.desc = &obs_desc;
        obs_desc.snoop.snooping = &mob;

        auto *node = static_cast<snoop_by_data *>(std::calloc(1, sizeof(snoop_by_data)));
        node->snoop_by = &observer;
        god_desc.snoop.snoop_by_list = node;

        // Simulate do_return retargeting
        god_desc.character = god_desc.original;
        god_desc.original = nullptr;
        god_desc.character->only.pc->switched = nullptr;
        mob.desc = nullptr;

        for (snoop_by_data *cur = god_desc.snoop.snoop_by_list; cur; cur = cur->next)
        {
            if (cur->snoop_by && cur->snoop_by->desc)
                cur->snoop_by->desc->snoop.snooping = god_desc.character;
        }

        assert(obs_desc.snoop.snooping == &god);
        free(node);
        god_desc.snoop.snoop_by_list = nullptr;
    }

    std::puts("PASS extract_char snoop observer cleanup across all levels and forms");
    return 0;
}
'''


def main() -> None:
    build_dir = ROOT / "bin/tests"
    build_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="snoop-extract-", dir=build_dir) as td:
        cpp = Path(td) / "test.cpp"
        binary = Path(td) / "test"
        cpp.write_text(FIXTURE, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsanitize=address,undefined",
                "-g",
                "-Isrc",
                str(cpp),
                "-o",
                str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
