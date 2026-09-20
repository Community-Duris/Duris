#!/usr/bin/env python3
"""Exercise the production rest/stand state transitions in isolation.

The command bodies are extracted from ``actmove.c`` so this harness tests the
compiled production implementation without starting a server or touching a
database.  The source assertions also pin the wake transition that originally
created ``POS_STANDING + STAT_RESTING`` by preserving the standing posture.
"""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function


ACTMOVE = ROOT / "src/cmd/actmove.c"


def assert_wake_producer_is_normalized() -> None:
    source = ACTMOVE.read_text(encoding="utf-8", errors="replace")
    wake = extract_function("actmove.c", "void do_wake(")
    sleep = extract_function("actmove.c", "void do_sleep(")

    # do_sleep may preserve a chosen posture while the character is asleep;
    # do_wake must convert standing sleep into the canonical resting posture.
    assert "SET_POS(ch, GET_POS(ch) + STAT_SLEEPING);" in sleep
    assert "resting_posture(tmp_char) + STAT_RESTING" in wake
    assert "resting_posture(ch) + STAT_RESTING" in wake
    assert "SET_POS(tmp_char, GET_POS(tmp_char) + STAT_RESTING);" not in wake
    assert "SET_POS(ch, GET_POS(ch) + STAT_RESTING);" not in wake
    assert "static int resting_posture(P_char ch)" in source


PRELUDE = r'''
#include "core/structs.h"
#include "world/events.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"

#include <cassert>
#include <cstdio>
#include <string>

static std::string output;
static int attackers;
static int gmcp_calls;
static int recovery_logs;
static int regen_calls;
static int stop_memorizing_calls;

void send_to_char(const char *text, P_char)
{
    output += text ? text : "";
}

void act(const char *text, int, P_char, P_obj, void *, int target)
{
    if (target == TO_CHAR)
        output += text ? text : "";
}

void gmcp_char_vitals(P_char)
{
    ++gmcp_calls;
}

void stop_memorizing(P_char, memorization_stop_reason)
{
    ++stop_memorizing_calls;
}

bool check_crippling_strike(P_char)
{
    return false;
}

int NumAttackers(P_char)
{
    return attackers;
}

void StartRegen(P_char, regen_resource)
{
    ++regen_calls;
}

void telemetry_gameplay_context_changed(P_char)
{
}

void logit(const char *, const char *, ...)
{
    ++recovery_logs;
}

int number(int from, int)
{
    return from;
}
'''


DRIVER = r'''
static void clear_observations()
{
    output.clear();
    attackers = 0;
    gmcp_calls = 0;
    recovery_logs = 0;
    regen_calls = 0;
    stop_memorizing_calls = 0;
}

static void assert_state(P_char ch, int posture, int status)
{
    assert(GET_POS(ch) == posture);
    assert(static_cast<int>(GET_STAT(ch)) == status);
}

int main()
{
    // The reported state is recoverable through rest and starts regeneration.
    {
        char_data ch{};
        SET_POS(&ch, POS_STANDING + STAT_RESTING);
        clear_observations();
        do_rest(&ch, nullptr, 0);
        assert_state(&ch, POS_SITTING, STAT_RESTING);
        assert(output.find("You sit down and relax.") != std::string::npos);
        assert(gmcp_calls == 1 && regen_calls == 4);
        assert(recovery_logs == 1);
    }

    // A normal repeated rest remains an idempotent refusal.
    {
        char_data ch{};
        SET_POS(&ch, POS_SITTING + STAT_RESTING);
        clear_observations();
        do_rest(&ch, nullptr, 0);
        assert_state(&ch, POS_SITTING, STAT_RESTING);
        assert(output.find("You are already resting.") != std::string::npos);
        assert(gmcp_calls == 0 && regen_calls == 0 && recovery_logs == 0);
    }

    // The mixed state is also recoverable through stand, after all existing
    // command-gate, paralysis, binding, combat, and knockout checks.
    {
        char_data ch{};
        SET_POS(&ch, POS_STANDING + STAT_RESTING);
        clear_observations();
        do_stand(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_NORMAL);
        assert(output.find("You tense up and become more alert.") != std::string::npos);
        assert(gmcp_calls == 1 && stop_memorizing_calls == 1);
        assert(recovery_logs == 1);
    }

    // A normal repeated stand remains an idempotent refusal.
    {
        char_data ch{};
        SET_POS(&ch, POS_STANDING + STAT_NORMAL);
        clear_observations();
        do_stand(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_NORMAL);
        assert(output.find("You are already standing.") != std::string::npos);
        assert(gmcp_calls == 0 && stop_memorizing_calls == 0 && recovery_logs == 0);
    }

    // A standing sleeper must still be told to wake rather than normalized.
    {
        char_data ch{};
        SET_POS(&ch, POS_STANDING + STAT_SLEEPING);
        clear_observations();
        do_stand(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_SLEEPING);
        assert(output.find("You dream of sleepwalking.") != std::string::npos);
        assert(gmcp_calls == 0 && recovery_logs == 0);
    }

    // Preserve the reported recline -> stand -> rest workaround sequence.
    {
        char_data ch{};
        SET_POS(&ch, POS_STANDING + STAT_RESTING);
        clear_observations();
        SET_POS(&ch, POS_PRONE + STAT_RESTING); // do_recline's resulting state
        do_stand(&ch, nullptr, 0);
        do_rest(&ch, nullptr, 0);
        assert_state(&ch, POS_SITTING, STAT_RESTING);
    }

    // Eligibility guards still reject the mixed state without normalizing it.
    {
        char_data ch{};
        SET_POS(&ch, POS_STANDING + STAT_RESTING);
        SET_BIT(ch.specials.affected_by, AFF_BOUND);
        clear_observations();
        do_rest(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_RESTING);
        assert(output.find("Your bonds prevent") != std::string::npos);
        assert(recovery_logs == 0 && regen_calls == 0);

        ch.specials.affected_by = 0;
        ch.specials.fighting = &ch;
        clear_observations();
        do_rest(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_RESTING);
        assert(output.find("final rest") != std::string::npos);
        assert(recovery_logs == 0 && regen_calls == 0);

        ch.specials.fighting = nullptr;
        SET_BIT(ch.specials.affected_by, AFF_KNOCKED_OUT);
        clear_observations();
        do_rest(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_RESTING);
        assert(output.find("becoming conscious") != std::string::npos);
        assert(recovery_logs == 0 && regen_calls == 0);

        ch.specials.affected_by = 0;
        SET_BIT(ch.specials.affected_by2, AFF2_MINOR_PARALYSIS);
        clear_observations();
        do_stand(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_RESTING);
        assert(output.find("can't even twitch") != std::string::npos);
        assert(recovery_logs == 0 && gmcp_calls == 0);

        ch.specials.affected_by2 = 0;
        SET_BIT(ch.specials.act2, PLR2_WAIT);
        clear_observations();
        do_stand(&ch, nullptr, 0);
        assert_state(&ch, POS_STANDING, STAT_RESTING);
        assert(output.empty() && recovery_logs == 0);
    }

    std::puts("posture state regression passed: wake producer, mixed rest/stand recovery, idempotence, workaround, and guards");
}
'''


def main() -> None:
    assert_wake_producer_is_normalized()
    build_root = ROOT / "bin" / "tests"
    build_root.mkdir(parents=True, exist_ok=True)
    helpers = "\n".join(
        extract_function("actmove.c", signature)
        for signature in (
            "static bool standing_and_resting(",
            "static int resting_posture(",
            "static void log_standing_resting_recovery(",
        )
    )
    command_bodies = "\n".join(
        extract_function("actmove.c", signature)
        for signature in ("void do_stand(", "void do_rest(")
    )

    with tempfile.TemporaryDirectory(prefix="posture-state-", dir=build_root) as directory:
        harness = Path(directory) / "harness.cpp"
        binary = Path(directory) / "harness"
        harness.write_text(
            "\n".join((PRELUDE, helpers, command_bodies, DRIVER)),
            encoding="utf-8",
        )
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer",
                f"-I{ROOT / 'src'}",
                str(harness),
                "-o",
                str(binary),
            ],
            cwd=ROOT,
            check=True,
            timeout=120,
        )
        subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    main()
