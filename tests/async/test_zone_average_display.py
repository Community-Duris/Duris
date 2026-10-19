#!/usr/bin/env python3
"""Execute do_area's basic command branch with real character/zone structures."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

command = extract_function('actoth.c', 'void do_area(')
# Staff search subcommands are unrelated to the basic display being repaired.
command = command[:command.index('// Immortal arguments:')] + '(void)buf; (void)buf2; (void)rest;\n}\n'
fixture = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "world/db.h"
#include "world/world_quest_policy_math.h"
#include <cassert>
#include <string>
static room_data rooms[1] = {};
static zone_data zones[1] = {};
P_room world = rooms;
zone_data *zone_table = zones;
static std::string feedback;
void send_to_char(const char *text, P_char) { feedback += text; }
std::string pad_ansi(const char *text, int, bool) { return text; }
int IS_MORPH(P_char) { return false; }
''' + command + r'''
int main() {
    char_data player = {};
    player.player.level = 20;
    zones[0].name = const_cast<char *>("Fixture zone");
    char argument[] = "";
    auto display = [&](int average) {
        zones[0].avg_mob_level = average;
        feedback.clear();
        do_area(&player, argument, 0);
        assert(zones[0].avg_mob_level == average);
        assert(feedback.size() < MAX_STRING_LENGTH);
        return feedback;
    };
    auto unknown = display(-2);
    assert(unknown.find("unknown") != std::string::npos);
    assert(unknown.find("-2") == std::string::npos);
    assert(unknown.find("Warning") == std::string::npos);
    auto empty = display(-1);
    assert(empty.find("not applicable") != std::string::npos);
    assert(empty.find("-1") == std::string::npos);
    assert(empty.find("Warning") == std::string::npos);
    assert(display(0).find("&n0") != std::string::npos);
    auto populated = display(world_quest_truncated_average_level(544, 34));
    assert(populated.find("&n16") != std::string::npos);
    assert(populated.find("Warning") == std::string::npos);
    assert(display(35).find("Warning") == std::string::npos);
    assert(display(36).find("Warning") != std::string::npos);
    // The next display reads a changed catalog value, with no cached sentinel text.
    assert(display(16).find("Warning") == std::string::npos);
    assert(display(-2).find("unknown") != std::string::npos);
    player.player.level = 62;
    assert(display(16).find("Fixture zone 0") != std::string::npos);
    // Plain/colored names around the response bound must retain the full suffix.
    for (const auto &name : {std::string("Fixture zone"), std::string("&+RRed zone&n"),
                            std::string(65500, 'x'), "&+R" + std::string(65500, 'x') + "&n",
                            std::string(MAX_STRING_LENGTH, 'x')}) {
        zones[0].name = const_cast<char *>(name.c_str());
        for (int level : {20, 62}) {
            player.player.level = level;
            for (int average : {-2, -1, 0, 16, level + 15, level + 16}) {
                const auto result = display(average);
                const std::string label = average >= 0 ? std::to_string(average) :
                                          average == -1 ? "not applicable" : "unknown";
                std::string suffix = level == 62 ? " 0" : "";
                suffix += "\r\n&+LAverage &+rmob&+L level in zone: &n" + label + "\r\n ";
                const bool warning = average >= 0 && average - level > 15;
                if (warning)
                    suffix += "&-L&+RWarning&-L, some mobs in this zone may be very hazardous for you! Travel with care!&n\r\n";
                assert(result.ends_with(suffix));
                assert((result.find("Warning") != std::string::npos) == warning);
                assert(result.starts_with("&+LZone Name: " + name.substr(0, 10)));
            }
        }
    }
    puts("PASS bounded basic area command: plain/colored/oversized names, staff IDs, averages and full warnings");
}
'''
out = ROOT / 'bin/tests'; out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='zone-display-', dir=out) as temporary:
    cpp, binary = Path(temporary) / 'test.cpp', Path(temporary) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-no-pie', '-g', '-Isrc', '-I/usr/include/mysql',
                    str(cpp), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
