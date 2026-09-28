#!/usr/bin/env python3
"""Regression contracts for studioproc commands and movement lifetime safety."""
from _paths import SRC
from pathlib import Path

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "studioproc.c").read_text()
proclib = (SRC / "studioproclib.c").read_text()

command_start = index(source, "static int sp_do_command(")
command_end = index(source, "static int sp_execute(", command_start)
command = source[command_start:command_end]
assert contains(command, "if (!MIN_POS(ch, cmd_info[cmd].minimum_position))")
assert not contains(command, "GET_POS(ch) < cmd_info[cmd].minimum_position")

execute_start = index(source, "static int sp_execute(")
execute_end = index(source, "static int sp_fire(", execute_start)
execute = source[execute_start:execute_end]
self_membership = index(execute, "if (self && !char_in_list(self))")
self_liveness = index(execute, "!IS_ALIVE(self)", self_membership)
actor_membership = index(execute, "if (actor && !char_in_list(actor))")
actor_liveness = index(execute, "!IS_ALIVE(actor)", actor_membership)
assert self_membership < self_liveness
assert actor_membership < actor_liveness

run_start = index(source, "static int sp_run(")
run_end = index(source, "static int sp_event_for_cmd(", run_start)
run = source[run_start:run_end]
assert contains(
	run,
	"if (cx->self_ch && !char_in_list(cx->self_ch))\n\t\tcx->self_ch = NULL;",
)
assert contains(
	run,
	"if (cx->actor && !char_in_list(cx->actor))\n\t\tcx->actor = NULL;",
)
fire_return = index(run, "one = sp_fire(t, cx);")
self_post_fire_membership = index(run, "!char_in_list(cx->self_ch)", fire_return)
self_post_fire_liveness = index(run, "!IS_ALIVE(cx->self_ch)", fire_return)
actor_post_fire_membership = index(run, "!char_in_list(cx->actor)", fire_return)
actor_post_fire_liveness = index(run, "!IS_ALIVE(cx->actor)", fire_return)
assert self_post_fire_membership < self_post_fire_liveness
assert actor_post_fire_membership < actor_post_fire_liveness

mob_start = index(source, "int studioproc_mob(")
obj_start = index(source, "int studioproc_obj(", mob_start)
room_start = index(source, "int studioproc_room(", obj_start)
mob_proc = source[mob_start:obj_start]
obj_proc = source[obj_start:room_start]
room_proc = source[room_start:index(source, "void studioproc_speech(", room_start)]
assert index(mob_proc, "!sp_on_game_thread()") < index(mob_proc, "!char_in_list(mob)")
assert index(mob_proc, "!char_in_list(mob)") < index(mob_proc, "GET_VNUM(mob)")
assert index(mob_proc, "(*rec->prev_mob)(mob, actor, cmd, arg)") < index(
	mob_proc, "if (!char_in_list(mob))"
)
assert contains(obj_proc, "if (actor && char_in_list(actor))")
assert index(obj_proc, "!sp_on_game_thread()") < index(obj_proc, "if (actor && char_in_list(actor))")
assert index(obj_proc, "char_in_list(vict)") < index(obj_proc, "IS_ALIVE(vict)")
assert index(room_proc, "!char_in_list(actor)") < index(room_proc, "rec->prev_room")

speech_start = index(source, "void studioproc_speech(")
give_start = index(source, "void studioproc_give(", speech_start)
kill_start = index(source, "void studioproc_kill(", give_start)
hour_start = index(source, "static void sp_hour_event(", kill_start)
speech = source[speech_start:give_start]
give = source[give_start:kill_start]
kill = source[kill_start:hour_start]
assert index(speech, "!sp_on_game_thread()") < index(speech, "!char_in_list(ch)")
assert index(speech, "!char_in_list(ch)") < index(speech, "!IS_PC(ch)")
assert "if (!IS_ALIVE(ch) || ch->in_room != room)" not in speech
normalized_speech = " ".join(speech.split())
assert normalized_speech.count(
	"!char_in_list(ch) || !IS_ALIVE(ch) || ch->in_room != room"
) >= 8
assert index(give, "!sp_on_game_thread()") < index(give, "!char_in_list(vict)")
assert index(give, "!char_in_list(vict)") < index(give, "!IS_NPC(vict)")
assert index(kill, "!sp_on_game_thread()") < index(kill, "!char_in_list(killer)")
assert kill.count("!char_in_list(killer) || !IS_ALIVE(killer)") >= 3

attack_one_start = index(source, "static void sp_attack_one(")
do_attack_start = index(source, "static void sp_do_attack(", attack_one_start)
do_command_start = index(source, "static int sp_do_command(", do_attack_start)
attack_one = source[attack_one_start:do_attack_start]
do_attack = source[do_attack_start:do_command_start]
assert index(attack_one, "!char_in_list(vict)") < index(attack_one, "!IS_ALIVE(vict)")
room_loop = index(do_attack, "for (k = world[room].people; k; k = k->next_in_room)")
membership_check = index(do_attack, "!char_in_list(k)", room_loop)
target_snapshot = index(do_attack, "target_ids.push_back(k->runtime_id);", membership_check)
target_loop = index(do_attack, "for (const uint64_t target_id : target_ids)", target_snapshot)
target_resolve = index(do_attack, "k = find_character_by_runtime_id(target_id);", target_loop)
attack_call = index(do_attack, "sp_attack_one(cx, a, k);", target_resolve)
assert room_loop < membership_check < target_snapshot < target_loop < target_resolve < attack_call
self_refresh = index(do_attack, "find_character_by_runtime_id(self_id)", attack_call)
self_recheck = index(do_attack, "!IS_ALIVE(cx->self_ch)", self_refresh)
assert attack_call < self_refresh < self_recheck
assert index(do_attack, "find_character_by_runtime_id(actor_id)", attack_call) > attack_call
assert index(do_attack, "find_character_by_runtime_id(prime_id)", attack_call) > attack_call
assert index(do_attack, "find_character_by_runtime_id(tank_id)", attack_call) > attack_call

speech_mob_loop = index(speech, "for (k = world[room].people; k; k = next_k)")
speech_next_link = index(speech, "next_k = k->next_in_room;", speech_mob_loop)
assert index(speech, "!char_in_list(k)", speech_mob_loop) < speech_next_link

transfer_start = index(source, "case SP_A_TRANSFER:")
goto_start = index(source, "case SP_A_GOTO:", transfer_start)
damage_start = index(source, "case SP_A_DAMAGE:", goto_start)
transfer = source[transfer_start:goto_start]
goto = source[goto_start:damage_start]
assert contains(transfer, "if (char_to_room(actor, rr, -1))")
assert contains(transfer, "else cx->actor = NULL;")
assert not contains(transfer, "if (!char_to_room(actor, rr, -1))")
assert contains(goto, "if (char_to_room(self, rr, -1))")
assert contains(goto, "cx->self_ch = NULL;")
assert contains(goto, "return ret | SP_X_SELFGONE;")
assert not contains(goto, "if (!char_to_room(self, rr, -1))")

transport_start = index(proclib, "int proclibobj_transporter(")
transport_end = index(proclib, "int proclib_obj_cmd_bridge(", transport_start)
transport = proclib[transport_start:transport_end]
assert contains(
	transport,
	"else if (char_in_list(ch) && IS_ALIVE(ch) && ch->in_room == NOWHERE)",
)

print("studioproc command and movement contracts passed")
