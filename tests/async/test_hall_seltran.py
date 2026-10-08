#!/usr/bin/env python3
"""Load Hall's actual quest data and exercise Seltran's accepted offering.

Reuse the durable-offering fixture's external persistence boundaries, not its
main test. The loader, recipe selection, consumption, completion and reward
recovery callbacks are the server's actual functions. Item grant settlement is
acknowledged explicitly; this does not qualify a live database journey.
"""

import ast
from pathlib import Path
import re
import subprocess
import tempfile

from _paths import extract_function

ROOT = Path(__file__).resolve().parents[2]
fixture = Path(__file__).with_name("test_durable_quest_offering.py")
tree = ast.parse(fixture.read_text(encoding="utf8"))
prefix = []
for node in tree.body:
    prefix.append(node)
    if isinstance(node, ast.Assign) and any(
        isinstance(target, ast.Name) and target.id == "program" for target in node.targets
    ):
        break
else:
    raise AssertionError("durable offering fixture no longer exposes its program")
namespace = {"__file__": str(fixture)}
exec(compile(ast.Module(body=prefix, type_ignores=[]), str(fixture), "exec"), namespace)
program = namespace["program"].split("int main() {", 1)[0]
program = program.replace("quest_data quest_index[1];", "quest_data quest_index[32];")
program = program.replace(
    "struct quest_data {",
    "struct quest_msg_data { char *key_words, *message; bool echoAll; quest_msg_data *next; };\n"
    "struct quest_data { quest_msg_data *quest_message = nullptr;",
)
program = program.replace(
    "void act(const char *, int, P_char, int, P_char, int) { ++messages; }",
    "std::vector<std::string> dialogue;\n"
    "void act(const char *text, int, P_char, int, P_char, int) {\n"
    "    ++messages; dialogue.emplace_back(text);\n}",
)
assert "quest_data quest_index[32]" in program and "dialogue.emplace_back" in program
program = "#include <cstdio>\n#include <cstdlib>\n#include <cctype>\n" + program
program += r'''
constexpr int MAX_QUESTS = 32, MAX_STRING_LENGTH = 8192, LOG_EXIT = 1;
constexpr int QUEST_GOAL_ITEM_TYPE = 2, QUEST_GOAL_UNKNOWN = 0;
int mini_mode = 0;
const char *test_quest_file;
#define QUEST_FILE test_quest_file
#define MINI_QUEST_FILE test_quest_file
#define CREATE(p, type, count, tag) p = new type[count]{}
#define REQUIRED_FSCANF(f, ...) assert(fscanf(f, __VA_ARGS__) == 1)
int real_mobile(int vnum) { return vnum == 77739 ? 11 : 0; }
char *fread_string(FILE *stream) {
    std::string text;
    int ch;
    do { ch = fgetc(stream); } while (ch != EOF && std::isspace(ch));
    while (ch != EOF && ch != '~') { text += static_cast<char>(ch); ch = fgetc(stream); }
    assert(ch == '~');
    return strdup(text.c_str());
}
''' + extract_function("world/quest.c", "int quest_sort_comp(") + "\n" + extract_function(
    "world/quest.c", "void quick_sort_quest_index("
) + "\n" + extract_function("world/quest.c", "static FILE *open_quest_stream(") + "\n" + extract_function(
    "world/quest.c", "void boot_the_quests("
) + r'''
int retired = 0;
P_obj unequip_char(P_char mob, int slot) {
    P_obj item = mob->equipment[slot]; mob->equipment[slot] = nullptr; return item;
}
void extract_char(P_char mob) {
    assert(mob == character_list); character_list = mob->next; ++retired;
}
int main(int argc, char **argv) {
    assert(argc == 2); test_quest_file = argv[1];
    boot_the_quests();
    int quester_id = -1;
    for (int i = 0; i < number_of_quests; ++i)
        if (quest_index[i].quester == 11) quester_id = i;
    assert(quester_id >= 0);
    auto *completion = quest_index[quester_id].quest_complete;
    assert(completion && completion->give->number == 77747);
    assert(completion->receive && completion->receive->number == 77719);
    assert(completion->disappear && !completion->receive->next);
    // Retain the historical nonreward contract; it must no longer win a match.
    assert(completion->next && !completion->next->receive && !completion->next->disappear);
    assert(completion->next->give->number == 77747 && !completion->next->next);

    character actor{}, mob{};
    pc_only actor_pc{}; descriptor_data descriptor;
    actor.pid = 7; actor.only.pc = &actor_pc; actor.desc = &descriptor;
    mob.npc = true; mob.rnum = 11; mob.vnum = 77739; mob.next = &actor;
    character_list = &mob; mob_index[11].virtual_number = 77739;
    world[0].number = 77919;
    object letter{101, 77743, &actor}, hair{102, 77747, &actor};
    letter.next = letter.next_content = &hair;
    actor.carrying = object_list = &letter;
    accounting_active = true;
    // The letter belongs to the earlier recipient. It cannot trigger Seltran.
    assert(!submit_durable_quest_offering(&mob, &actor, quester_id, &letter));
    assert(submissions == 0 && removed == 0 && dialogue.empty());
    assert(submit_durable_quest_offering(&mob, &actor, quester_id, &hair));
    assert(submissions == 1 && removed == 0 && dialogue.empty());
    quest_reward_continuation admitted;
    assert(quest_reward_continuation_decode(saved_continuation.data.data(),
                                           saved_continuation.data.size(), &admitted));
    assert(admitted.completion_index == 0 && admitted.root_count == 1);
    assert(admitted.roots[0] == hair.obj_uid && admitted.reward_count == 1);
    assert(admitted.rewards[0].type == QUEST_GOAL_ITEM &&
           admitted.rewards[0].number == 77719);
    // Capture the retained refusal's actual terms, then simulate its old slot
    // zero. Recovery must use its frozen empty rewards, not today's slot zero.
    quest_durable_context refusal_context;
    memcpy(&refusal_context, saved_context, sizeof(refusal_context));
    refusal_context.completion_index = 1;
    item_transfer_continuation refusal_encoded;
    assert(capture_quest_offering_continuation(&mob, &actor, quester_id, 1,
                                              completion->next, refusal_context,
                                              &refusal_encoded));
    quest_reward_continuation historical_refusal;
    assert(quest_reward_continuation_decode(refusal_encoded.data.data(),
                                           refusal_encoded.data.size(), &historical_refusal));
    assert(historical_refusal.reward_count == 0);
    historical_refusal.completion_index = 0;
    item_transfer_result result; result.operation_id.bytes[0] = 71;
    assert(publish_quest_offering({}, &actor, false, result, 0, saved_context, saved_size));
    complete_quest_offering(&actor, false, result, 0, saved_context, saved_size);
    assert(removed == 0 && queued_grants.empty() && dialogue.empty() && retired == 0);
    assert(publish_quest_offering({}, &actor, true, result, 0, saved_context, saved_size));
    complete_quest_offering(&actor, true, result, 0, saved_context, saved_size);
    assert(removed == 1 && actor.carrying == &letter && !letter.next_content);
    assert(dialogue.size() == 2 && dialogue[0].find("stored this away") != std::string::npos);
    assert(dialogue[1].find("walks away into the ether") != std::string::npos);
    assert(retired == 1 && character_list == &actor && queued_grants.size() == 1);
    assert(queued_grants[0].grant_object->vnum == 77719 && acked == 0);
    auto grant = queued_grants[0];
    grant.completion(&actor, grant.grant_object->obj_uid, true, 0);
    assert(acked == 1);
    // Retrying the same frozen reward after its grant acknowledgment is idempotent.
    quest_reward_recover_pending(&actor, result.operation_id, admitted, 0, 1);
    assert(queued_grants.size() == 1);
    critical_operation_id historical_operation; historical_operation.bytes[0] = 72;
    quest_reward_recover_pending(&actor, historical_operation, historical_refusal);
    assert(queued_grants.size() == 1 && acked == 3 && retired == 1 && dialogue.size() == 2);
}
'''

with tempfile.TemporaryDirectory(prefix="duris-hall-seltran-") as directory:
    scratch = Path(directory)
    cpp, exe = scratch / "hall.cpp", scratch / "hall-test"
    cpp.write_text(program, encoding="utf8")
    subprocess.run(["g++", "-std=c++20", "-O0", str(cpp), "-lcrypto", "-o", str(exe)], check=True)
    quest_file = ROOT / "areas/qst/hall.qst"
    subprocess.run([str(exe), str(quest_file)], check=True)
    # Exercise the historical ordering too: this regression must reject it.
    text = quest_file.read_text(encoding="utf8")
    match = re.search(r"(?m)^#77739\n(Q\n.*?)(?=^S\n)", text, re.S)
    blocks = re.split(r"(?m)(?=^Q\n)", match[1])
    assert len(blocks) == 3 and not blocks[0]
    old = scratch / "old-hall.qst"
    old.write_text(text[:match.start(1)] + blocks[2] + blocks[1] + text[match.end(1):], encoding="utf8")
    rejected = subprocess.run([str(exe), str(old)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    assert rejected.returncode != 0 and b"completion->receive" in rejected.stderr

print("Hall Seltran loader/offering/reward regression passed; original ordering rejected")
