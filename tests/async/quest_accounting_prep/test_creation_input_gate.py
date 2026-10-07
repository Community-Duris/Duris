#!/usr/bin/env python3
"""Execute current creation-request queue gates with explicit busy-state seams.

Reuse the maintained queue harness's types/reduced command table, without its
unrelated coin publication harness. No debit, coordinator or native race proof.
"""

import ast
import json
from pathlib import Path
import subprocess
import tempfile

from case_data import ROOT, digest
from _paths import extract_function


def run():
    shared = ROOT / "tests/async/test_currency_input_queue.py"
    tree = ast.parse(shared.read_text())
    prelude = next(ast.literal_eval(node.value) for node in tree.body
        if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and
            target.id == "PRELUDE" for target in node.targets))
    prelude = prelude.split("P_char character_list", 1)[0]
    # Append QUEST to the reduced command table with its matching test index.
    # Production classification/search functions are extracted unchanged below.
    sentinel = '\t"\\n"\n};'
    assert sentinel in prelude
    prelude = prelude.replace(sentinel, '\t"quest", "\\n"\n};\n#define CMD_QUEST 39')
    boundaries = r'''
bool currency_busy=false;
bool item_movement_transaction_player_busy(P_char) { return false; }
bool bulk_get_player_busy(P_char) { return false; }
bool collector_transaction_player_busy(P_char) { return false; }
bool collector_service_player_busy(P_char) { return false; }
bool currency_transaction_player_busy(P_char) { return currency_busy; }
bool input_allowed_while_item_moving(const char *) { return true; }
void logit(const char *,const char *,...) {}
void __free(void *memory,const char *,int) { free(memory); }
'''
    functions = "\n".join(extract_function(path, signature) for path, signature in [
        ("cmd/interp.c", "int old_search_block(const char *argument"),
        ("cmd/interp.c", "bool cmd_depends_on_currency_transaction(int cmd)"),
        ("cmd/interp.c", "static int input_command_number(const char *input)"),
        ("cmd/interp.c", "static bool input_is_currency_dependent_speech(const char *input)"),
        ("cmd/interp.c", "static bool input_is_currency_dependent_confirmation(const char *input)"),
        ("cmd/interp.c", "bool input_allowed_while_currency_pending(const char *input)"),
        ("cmd/interp.c", "bool input_allowed_while_item_and_currency_pending(const char *input)"),
        ("net/comm.c", "int get_from_q(struct txt_q *queue, char *dest)"),
        ("net/comm.c", "static int get_filtered_cmd_from_q(struct txt_q *queue, char *dest,"),
        ("net/comm.c", "int get_item_movement_cmd_from_q(struct txt_q *queue, char *dest)"),
        ("net/comm.c", "int get_pending_transaction_cmd_from_q(struct txt_q *queue, char *dest,"),
        ("net/comm.c", "static int get_playing_cmd_from_q(P_char character, struct txt_q *queue,"),
    ])
    main = r'''
void push(txt_q *queue,const char *text) {
 auto entry=static_cast<txt_block*>(calloc(1,sizeof(txt_block)));
 assert(entry);entry->text=strdup(text);assert(entry->text);
 if(queue->tail) queue->tail->next=entry;else queue->head=entry;
 queue->tail=entry;queue->bytes+=strlen(text)+1;++queue->entries;
}
int main() {
 assert(!input_allowed_while_currency_pending("ask woodseer quest"));
 assert(!input_allowed_while_currency_pending("  ASK woodseer quest"));
 assert(input_allowed_while_currency_pending("quest share friend"));
 assert(input_allowed_while_currency_pending("quest reset friend"));
 char_data actor={};txt_q queue={};char output[MAX_INPUT_LENGTH]={};
 push(&queue,"ask woodseer quest");push(&queue,"quest share friend");push(&queue,"look");
 currency_busy=true;
 assert(get_playing_cmd_from_q(&actor,&queue,output) && !strcmp(output,"quest share friend"));
 assert(get_playing_cmd_from_q(&actor,&queue,output) && !strcmp(output,"look"));
 assert(!get_playing_cmd_from_q(&actor,&queue,output));
 assert(queue.entries==1 && queue.head==queue.tail && !strcmp(queue.head->text,"ask woodseer quest"));
 currency_busy=false;
 assert(get_playing_cmd_from_q(&actor,&queue,output) && !strcmp(output,"ask woodseer quest"));
 assert(queue.entries==0 && !queue.head && !queue.tail && queue.bytes==0);
}
'''
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="quest-prep-creation-queue-", dir=build_root) as directory:
        cpp, executable = Path(directory) / "queue.cpp", Path(directory) / "queue"
        cpp.write_text(prelude + boundaries + functions + main)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql", str(cpp),
                        "-o", str(executable)], cwd=ROOT, check=True)
        subprocess.run([str(executable)], check=True)
    return dict(result="PASS: actual dequeue defers ASK until busy clears; QUEST remains eligible",
        authority="component queue proof; busy-state seam and reduced command table; no native race",
        source_hashes={path: digest(ROOT / path) for path in
            ("src/cmd/interp.c", "src/net/comm.c", "tests/async/test_currency_input_queue.py")})


if __name__ == "__main__":
    print(json.dumps(run(), indent=2))
