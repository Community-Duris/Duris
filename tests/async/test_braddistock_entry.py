#!/usr/bin/env python3
"""Execute the maintained mansion gate with isolated character/message stubs."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--source-file", type=Path, default=ROOT / "src/specs/specs.braddistock.c")
args = parser.parse_args()
text = args.source_file.read_text(encoding="utf-8")
start = text.index("int braddistock(P_char ch, P_char pl, int cmd,")
body = text.index("{", start)
depth, end = 1, body + 1
while depth:
    depth += (text[end] == "{") - (text[end] == "}")
    end += 1
function = text[start:end]

prefix = r'''
#include <cstdio>
#include <string>
#include <vector>
#define TRUE 1
#define FALSE 0
#define CMD_NORTH 1
#define CMD_SOUTH 3
#define CMD_SET_PERIODIC 900
#define TO_CHAR 2
#define TO_NOTVICT 3
struct Character {
    int level;
    bool trusted;
    bool descriptor;
    std::string output;
};
using P_char = Character *;
#define IS_TRUSTED(ch) ((ch)->trusted)
#define GET_LEVEL(ch) ((ch)->level)
struct Action { std::string text; P_char actor; P_char target; int destination; };
std::vector<Action> actions;
// Mirror send_to_char's actual descriptor boundary; ordinary NPCs cannot read it.
void send_to_char(const char *message, P_char recipient) {
    if (recipient && recipient->descriptor && message) recipient->output += message;
}
void act(const char *message, int, P_char actor, int, P_char target, int destination) {
    actions.push_back({message,actor,target,destination});
}
int failed = 0;
void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr,"%s\n",message); ++failed; }
}
'''
cases = r'''
int main() {
    Character spirit{59,false,false,{}}, player{15,false,true,{}};
    require(braddistock(&spirit,&player,CMD_NORTH,nullptr)==TRUE,
            "level15 northward passage was not blocked");
    require(player.output == "The spirit of Lord Braddistock says 'We don't want your kind around here!'\r\n",
            "entry denial did not deliver a complete speech line to the blocked player");
    require(spirit.output.empty(),"entry denial sent speech to the NPC");
    require(actions.size()==3 && actions[0].destination==TO_NOTVICT &&
            actions[1].destination==TO_CHAR && actions[2].destination==TO_NOTVICT,
            "existing blocked-player and observer actions changed");
    for (const auto &action : actions)
        require(action.actor==&player && action.target==&spirit,
                "refusal action actor or target changed");
    require(actions[1].text=="$N blocks your passage.","passage explanation changed");
    player.output.clear(); actions.clear();
    player.level=14;
    require(braddistock(&spirit,&player,CMD_NORTH,nullptr)==FALSE,
            "level14 admission changed");
    require(player.output.empty() && actions.empty(),"admitted player received a refusal");
    player.level=59;
    require(braddistock(&spirit,&player,CMD_SOUTH,nullptr)==FALSE,
            "non-north command was intercepted");
    player.trusted=true;
    require(braddistock(&spirit,&player,CMD_NORTH,nullptr)==FALSE,
            "trusted-player bypass changed");
    require(braddistock(&spirit,nullptr,CMD_NORTH,nullptr)==FALSE,
            "null combat caller was intercepted");
    require(braddistock(&spirit,&player,CMD_SET_PERIODIC,nullptr)==FALSE,
            "periodic registration behavior changed");
    require(braddistock(&spirit,&spirit,CMD_NORTH,nullptr)==FALSE,
            "spirit intercepted its own command");
    require(player.output.empty() && actions.empty(),"bypass or unrelated command emitted refusal");
    return failed ? 1 : 0;
}
'''
compiler = os.environ.get("CXX", "g++-12")
(ROOT / "bin").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix="braddistock-entry-", dir=ROOT / "bin") as temporary:
    cpp, binary = Path(temporary) / "entry.cpp", Path(temporary) / "entry"
    cpp.write_text(prefix + function + cases, encoding="utf-8")
    subprocess.run([compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("PASS: Braddistock refusal speech reaches the blocked player; gate, bypasses and observer actions retained.")
