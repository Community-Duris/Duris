#!/usr/bin/env python3
"""Run real send/act/queue/pager functions with production types and boundary stubs."""
import os
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

BUILD = ROOT / "bin/tests"
BUILD.mkdir(parents=True, exist_ok=True)
comm = (ROOT / "src/net/comm.c").read_text()
globals_start = comm.index("#define MAX_COMMAND_OUTPUT")
globals_end = comm.index("#define MIN_SOCKET_BUFFER_SIZE", globals_start)
functions = [comm[globals_start:globals_end]]
for filename, signatures in [
    ("utility.c", ["bool is_ansi_char(", "void CAP(char *str)", "int BOUNDED(int a, int b, int c)"]),
    ("comm.c", ["int get_from_q(", "void write_to_q(", "void escape_act_dollars(", "void delete_doubledollar(",
                "static void finalize_styled_command(", "void process_with_paging(",
                "void send_to_char_f(P_char ch, const char *fmt, ...)",
                "void send_to_char_f(P_char ch, const OutputContext &context, const char *fmt, ...)",
                "void send_to_char(const char *messg, P_char ch)",
                "void send_to_char(const char *messg, P_char ch, const OutputContext &context)",
                "void send_to_char(const char *messg, P_char ch, int log)",
                "void send_to_char(const char *messg, P_char ch, int log, const OutputContext &context)",
                "void act(const char *str, int hide_invisible, P_char ch, P_obj obj, void *vict_obj, int type)",
                "void act(const char *str, int hide_invisible, P_char ch, P_obj obj, void *vict_obj, int type,",
                "void format_to_snoopers(char *from_string, char *to_string)\n{"]),
    ("modify.c", ["char *next_page(", "void free_paging_data(", "void show_string(", "void page_string_real("]),
    ("actcomm.c", ["void do_tell(", "void do_reply(", "int say(", "void do_gcc("]),
    ("objmisc.c", ["bool object_has_magical_proc("]),
    ("affects.c", ["int KludgeDuration("]),
    ("spell_detection.c", ["void spell_detect_magic("]),
    ("actinf.c", ["char *show_obj_to_char(P_obj object, P_char ch, int mode, bool print)",
                  "char *show_obj_to_char(P_obj object, P_char ch, int mode, bool print,",
                  "void list_obj_to_char(P_obj list, P_char ch, int mode, bool show)",
                  "void list_obj_to_char(P_obj list, P_char ch, int mode, bool show,",
                  "void show_exits_to_char(", "void display_room_auras(",
                  "static void show_detected_item_magic(", "bool get_equipment_list("]),
    ("weather.c", ["void send_to_weather_sector("]),
    ("fight.c", ["void dam_message("]),
    ("prompt.c", ["void make_prompt("]),
    ("json_utils.c", ["char *json_build_comm_channel("]),
    ("gmcp.c", ["void gmcp_broadcast_channel("])
]:
    functions.extend(extract_function(filename, signature) for signature in signatures)

look = extract_function("actinf.c", "void new_look(")
room_call = look.index("display_room_auras(ch, room_no)")
description_end = look.index("profile.context);")
assert room_call > description_end
assert "if (vis_mode != 3 && vis_mode != 4)" in look[description_end:room_call]
assert "LISTOBJ_ACTIONDESC | LISTOBJ_STATS" in look
examine = extract_function("actinf.c", "void do_examine(")
assert examine.index("show_detected_item_magic(ch, tmp_object)") < examine.index("// check legend lore")
score = extract_function("actinf.c", "void do_score(")
assert "spell_affect_requires_detect_magic(aff->type)" in score

benchmark = os.environ.get("OUTPUT_BENCHMARK") == "1"

flags = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"] if os.environ.get("SANITIZE") == "1" else []
if benchmark:
    flags += ["-DCOLORIZATION_BENCHMARK", "-Wl,--wrap=malloc", "-Wl,--wrap=calloc", "-Wl,--wrap=realloc"]
with tempfile.TemporaryDirectory(prefix="word-output-", dir=BUILD) as directory:
    temp = Path(directory)
    (temp / "production_output.inc").write_text("\n\n".join(functions))
    binary = temp / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Og", "-g", "-D__NO_MYSQL__", *flags,
        f"-I{ROOT / 'src'}", f"-I{ROOT / 'src/no_mysql'}", f"-I{temp}",
        str(ROOT / "tests/async/word_output_integration_harness.cpp"),
        str(ROOT / "src/net/ansi.c"), str(ROOT / "src/net/unicode.c"),
        str(ROOT / "src/net/chat_presentation.c"),
        str(ROOT / "src/net/output_style.c"), str(ROOT / "src/core/safe_format.c"),
        str(ROOT / "src/net/output_profiles.c"), str(ROOT / "src/player/output_preferences.c"),
        str(ROOT / "src/player/output_message.c"), "-lcjson", "-pthread", "-o", str(binary)
    ], check=True, timeout=120)
    subprocess.run([str(binary)], check=True, timeout=120)
