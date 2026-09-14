#!/usr/bin/env python3
"""Exercise the engine-free falling policy directly."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT


HARNESS = r'''
#include "world/falling_policy.h"

#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    for (int skill : {-10, 0, 1, 2, 50, 99, 100, 150}) {
        int catches = 0;
        for (int roll = 1; roll <= 100; ++roll)
            catches += falling_climb_catches(true, skill, roll);
        const int clamped = skill < 0 ? 0 : skill > 100 ? 100 : skill;
        assert(catches == clamped / 2);
        assert(!falling_climb_catches(false, skill, 1));
    }

    assert(falling_choose_route(true, true) == falling_route::vertical);
    assert(falling_choose_route(false, true) == falling_route::downward);
    assert(falling_choose_route(false, false) == falling_route::stay);

    assert(falling_should_land(false, false, false, false, false));
    assert(falling_should_land(true, false, false, false, false));
    assert(falling_should_land(true, true, true, false, false));
    assert(falling_should_land(true, true, false, true, false));
    assert(falling_should_land(true, true, false, false, true));
    assert(!falling_should_land(true, true, false, false, false));

    auto speed = falling_advance_speed(1, false, false);
    assert(speed.speed == 31 && speed.motion == falling_motion::continue_falling);
    speed = falling_advance_speed(31, false, false);
    assert(speed.speed == 43 && speed.motion == falling_motion::continue_falling);
    speed = falling_advance_speed(43, false, false);
    assert(speed.speed == 51 && speed.motion == falling_motion::continue_falling);
    speed = falling_advance_speed(250, false, false);
    assert(speed.speed == 250 && speed.motion == falling_motion::continue_falling);
    speed = falling_advance_speed(1, false, true);
    assert(speed.speed == 0 && speed.motion == falling_motion::stopped);
    speed = falling_advance_speed(31, true, false);
    assert(speed.speed == 23 && speed.motion == falling_motion::continue_falling);

    assert(falling_impact_damage(1000, 43, 101, 100, 0, 0) == 171);
    assert(falling_impact_damage(1000, 43, 101, 100, 100, 1) == 85);
    assert(falling_impact_damage(1000, 1, 200, 100, 100, 1) == 1);
    assert(falling_impact_damage(1000, 43, 101, 100, 100, 100) == 171);

    assert(!falling_breaks_floor(43, 20));
    assert(falling_breaks_floor(44, 20));
    assert(falling_breaks_floor(1, 10));
    assert(falling_event_delay(31) == 4);
    assert(falling_event_delay(43) == 2);
    assert(falling_event_delay(51) == 1);

    puts("falling policy boundaries passed");
}
'''


with tempfile.TemporaryDirectory(prefix="duris-falling-policy-") as temporary:
    source = Path(temporary) / "policy.cpp"
    binary = Path(temporary) / "policy"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-Isrc",
            "src/world/falling_policy.c",
            str(source),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)
