#!/usr/bin/env python3
"""Run production board headline handling at the end of an input allocation under ASan."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, source, extract_function
PRELUDE = r'''
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
struct obj_data { obj_data *next_content = nullptr; int R_num = 0, loc_p = 0; struct { int room = -1; } loc; };
struct descriptor_data { char **str = nullptr; int max_str = 0; };
struct char_data {
    int in_room = 0, level = 0;
    descriptor_data *desc = nullptr;
    const char *name = "Writer";
    struct { unsigned act = 0; } specials;
};
typedef obj_data *P_obj;
typedef char_data *P_char;
#define NUM_OF_BOARDS 3
#include "BOARDS_H"
#define FALSE 0
#define TRUE 1
#define MAX_INPUT_LENGTH 1024
#define LOC_ROOM 1
#define TO_ROOM 1
#define PLR_WRITE 1
#define MEM_TAG_STRING 0
#define GET_LEVEL(ch) ((ch)->level)
#define GET_NAME(ch) ((ch)->name)
#define IS_NPC(ch) false
#define SET_BIT(flags, bit) ((flags) |= (bit))
#define CREATE(result, type, num, tag) ((result) = (type *)calloc((num), sizeof(type)))
#define OBJ_IN_ROOM(o, r) ((o) && ((o)->loc_p & LOC_ROOM) && (o)->loc.room == (r))
#define LOG_BOARD "board"
enum { CMD_SET_PERIODIC = -1, CMD_WRITE = 1, CMD_LOOK, CMD_EXAMINE, CMD_READ, CMD_REMOVE };
std::vector<std::string> logged;
void logit(const char *, const char *format, ...) { logged.push_back(format); }
std::vector<std::string> told;
void send_to_char(const char *text, char_data *) { told.push_back(text); }
void act(const char *, int, char_data *, void *, void *, int) {}
char *skip_spaces(char *text) { while (*text == ' ') text++; return text; }
char *msg_storage[INDEX_SIZE];
int msg_storage_taken[INDEX_SIZE];
int num_of_msgs[NUM_OF_BOARDS];
board_msginfo msg_index[NUM_OF_BOARDS][MAX_BOARD_MESSAGES];
board_info_type board_info[NUM_OF_BOARDS] = {
    {89, 0, 0, 0, "", 0}, {48101, 0, 0, 0, "", -1}, {90, 0, 0, 0, "", 1}};
int shown = -1;
int Board_show_board(int board_type, char_data *, char *) { shown = board_type; return 1; }
int Board_display_msg(int, char_data *, char *) { return 1; }
int Board_remove_msg(int, char_data *, char *) { return 1; }
'''
DRIVER = r'''

int main() {
 descriptor_data link; char_data writer; writer.desc=&link;
 char *line = new char[MAX_INPUT_LENGTH];
 memset(line, ' ', MAX_INPUT_LENGTH); strcpy(line+MAX_INPUT_LENGTH-3, "hi");
 Board_write_message(0, &writer, line+strlen("write")); delete[] line;
 assert(num_of_msgs[0]==1 && strstr(MSG_HEADING(0,0), "(Writer)] hi"));
 std::string headline(100,'x'); Board_write_message(0,&writer,headline.data());
 std::string heading=MSG_HEADING(0,1); assert(heading.substr(heading.find("] ")+2)==std::string(70,'x'));
 for(int i=0;i<num_of_msgs[0];i++) free(MSG_HEADING(0,i));
}
'''
harness = PRELUDE.replace("BOARDS_H", str(source("cmd/boards.h"))) + "\n" + "\n".join(
    extract_function("cmd/boards.c", signature) for signature in (
        "int find_slot(void)\n",
        "void Board_write_message(int board_type, struct char_data *ch, char *arg)\n")) + DRIVER
with tempfile.TemporaryDirectory(prefix="boards-headline-") as directory:
    cpp = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    cpp.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                    "-fsanitize=address,undefined", "-fno-pie", "-no-pie", str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("board headlines stay bounded without modifying the input allocation")
