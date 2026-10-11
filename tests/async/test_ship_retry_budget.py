#!/usr/bin/env python3
"""A crowded harbour cannot trigger an unbounded storage scan on a game-loop pulse."""
from pathlib import Path
import subprocess
import tempfile
from _paths import extract_function

HARNESS = r'''
#include <cassert>
#include <ctime>
#include <string>
#include <vector>
#define LOG_SHIP "ship"
enum class stored_ship_state { none, placed, no_room, unreadable };
static std::vector<std::string> unplaced_ship_owners;
static std::vector<std::string> tried;
static int free_rooms = 0;
static time_t clock_now = 1000;
static stored_ship_state outcome = stored_ship_state::no_room;
time_t time(time_t *output) noexcept { if(output) *output=clock_now; return clock_now; }
int count_free_ship_rooms() { return free_rooms; }
void logit(const char *, const char *, ...) {}
stored_ship_state place_stored_ship(const char *owner) {
 tried.emplace_back(owner); return outcome;
}
''' + extract_function("ships/ship_base.c", "void retry_unplaced_ships(void)") + r'''
int main() {
 for(int i=0;i<1000;++i) unplaced_ship_owners.push_back("Owner"+std::to_string(i));
 retry_unplaced_ships(); assert(tried.empty()); // no queries while the pool is full
 clock_now+=60; free_rooms=10;
 retry_unplaced_ships(); assert((tried==std::vector<std::string>{"Owner0","Owner1"}));
 retry_unplaced_ships(); assert(tried.size()==2); // same minute does no work
 clock_now+=60;
 retry_unplaced_ships(); assert(tried.size()==4 && tried[2]=="Owner2" && tried[3]=="Owner3");
 clock_now+=60; outcome=stored_ship_state::placed;
 retry_unplaced_ships(); assert(tried.size()==6 && unplaced_ship_owners.size()==998);
 assert(tried[4]=="Owner4" && tried[5]=="Owner5"); // erasure does not skip the next owner
 clock_now+=60; outcome=stored_ship_state::none;
 retry_unplaced_ships(); assert(tried.size()==8 && unplaced_ship_owners.size()==996);
 unplaced_ship_owners.clear(); unplaced_ship_owners.push_back("Replacement");
 clock_now+=60; outcome=stored_ship_state::unreadable;
 retry_unplaced_ships(); assert(tried.back()=="Replacement" && unplaced_ship_owners.size()==1);
}
'''
with tempfile.TemporaryDirectory(prefix="ship-retry-budget-") as directory:
    cpp = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    cpp.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                    "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("unplaced-ship retry is bounded, rotates owners, and skips a full room pool")
