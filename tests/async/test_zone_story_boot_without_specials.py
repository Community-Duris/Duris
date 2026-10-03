#!/usr/bin/env python3
"""Exercise the native boot owner with and without mobile special procedures."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/world/db.c").read_text()
signature = "static void boot_zone_story_quest_state()"
start = source.index(signature)
brace = source.index("{", start)
depth = 0
for end in range(brace, len(source)):
    depth += (source[end] == "{") - (source[end] == "}")
    if depth == 0:
        body = source[start:end + 1]
        break
else:
    raise AssertionError("missing native boot owner")

# The actual owner must be reached by boot_db even when assignment is skipped.
assert "assign_rooms();\n\t}\n\tboot_zone_story_quest_state();" in source
assert "zone_story_quest_runtime::bootstrap" not in (
    ROOT / "src/specs/specs.assign.c").read_text()

harness = r'''
#include <cassert>
#include <string>
#include <vector>
int no_specials = 0;
constexpr int LOG_DEBUG = 1, LOG_STATUS = 2;
int loaded = 0, bootstrapped = 0, logged = 0;
bool catalog_loaded = false, successful = true;
void boot_the_quests() { ++loaded; catalog_loaded = true; }
void logit(int kind, const char *, ...) {
    ++logged;
    assert(kind == (successful ? LOG_STATUS : LOG_DEBUG));
}
namespace zone_story_quest_runtime {
struct catalog_type { std::vector<int> definitions{1,2}; };
struct service_type { catalog_type &catalog() { static catalog_type result; return result; } };
bool bootstrap(std::string *error) {
    assert(catalog_loaded);
    ++bootstrapped;
    if (!successful) *error = "retained state cannot be loaded";
    return successful;
}
service_type *service() { assert(successful); static service_type result; return &result; }
unsigned content_revision() { return 1; }
}
'''
harness += body
harness += r'''
int main() {
    // Normal boot already loaded quests: no duplicate load or bootstrap.
    catalog_loaded = true;
    boot_zone_story_quest_state();
    assert(loaded == 0 && bootstrapped == 1 && logged == 1);
    no_specials = 1;
    catalog_loaded = false;
    boot_zone_story_quest_state();
    assert(loaded == 1 && bootstrapped == 2 && logged == 2);
    // Failed retained-state loading remains unavailable and logs the failure.
    catalog_loaded = false;
    successful = false;
    boot_zone_story_quest_state();
    assert(loaded == 2 && bootstrapped == 3 && logged == 3);
}
'''
with tempfile.TemporaryDirectory(prefix="duris-zone-story-boot-") as temporary:
    cpp, binary = Path(temporary) / "boot.cpp", Path(temporary) / "boot"
    cpp.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-fsanitize=address,undefined",
                    "-fno-omit-frame-pointer", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("zone-story native boot owner: normal, no-specials and retained-state failure passed")
