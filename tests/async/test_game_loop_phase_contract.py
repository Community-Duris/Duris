#!/usr/bin/env python3
"""Pin the game-thread pulse phase boundaries and their compatibility order."""

from _paths import ROOT, SRC
from contract_text import contains, count, index


comm = (SRC / "net" / "comm.c").read_text(encoding="utf-8")
phase_doc = (ROOT / "docs" / "network" / "GAME_LOOP_PHASES.md").read_text(encoding="utf-8")
session_header = (SRC / "net" / "session_input.h").read_text(encoding="utf-8")


PHASES = [
    "run_connection_phase",
    "run_session_input_phase",
    "run_output_phase",
    "run_event_phase",
    "run_recurring_persistence_phase",
    "run_activity_phase",
    "run_combat_phase",
    "run_pulse_reset_phase",
]


def function_region(name: str, next_name: str) -> str:
    start = index(comm, name)
    end = index(comm, next_name, start)
    return comm[start:end]


for phase in PHASES:
    assert count(comm, f"static {'bool' if phase == 'run_connection_phase' else 'void'} {phase}(") == 1
    assert contains(phase_doc, f"`{phase}`")


connection = function_region("static bool run_connection_phase", "static void run_session_input_phase")
session = function_region("static void run_session_input_phase", "static void run_output_phase")
output = function_region("static void run_output_phase", "static void run_event_phase")
events = function_region("static void run_event_phase", "static void run_recurring_persistence_phase")
recurring = function_region("static void run_recurring_persistence_phase", "static void run_activity_phase")
activity = function_region("static void run_activity_phase", "static void run_combat_phase")
combat = function_region("static void run_combat_phase", "static void run_pulse_reset_phase")
reset = function_region("static void run_pulse_reset_phase", "/**\n * Run network and simulation pulses")

for needle in ("persistence_log_poll();", "checkpointing();", "select(",
               "drain_new_connections(s, 0, \"Telnet\");", "process_input(point);"):
    assert contains(connection, needle)
assert contains(connection, "return false;")
assert not contains(connection, "critical_command_coordinator_pulse")

for needle in ("session_input_authentication_pending(point)",
               "descriptor_latency.finish();", "repair_session_command_gate(t_ch)",
               "select_session_input(point, t_ch, comm);",
               "dispatch_session_input(point, t_ch, comm, route, &command_latency)"):
    assert contains(session, needle)
assert index(session, "descriptor_latency.finish();") < index(session, "repair_session_command_gate")

for needle in ("telnet_flush_output(point)", "process_output(point)",
               "websocket_flush_output(point)", "point->ws_control_output_len"):
    assert contains(output, needle)

for needle in ("ne_events();", "item_creation_grant_prepare_pulse();",
               "artifact_mana_pulse();", "device_actions_pulse();"):
    assert contains(events, needle)
assert index(events, "ne_events();") < index(events, "item_creation_grant_prepare_pulse();")

for needle in ("if (!(pulse % 2))", "critical_command_coordinator_pulse(",
               "critical_gameplay_handle_completions(",
               "auction_transaction_publish_outbox();", "player_save_pipeline_pulse();",
               "redis_world_recovery_pulse();", "maintenance_scheduler_pulse("):
    assert contains(recurring, needle)

for needle in ("ship_activity();", "short_affect_update();", "wimps_in_approve_queue();"):
    assert contains(activity, needle)
for needle in ("perform_violence();", "display_map_room(", "map_look(t_ch, MAP_AUTOMAP);",
               "gmcp_send_group_status(t_ch);", "move_regen(t_ch"):
    assert contains(combat, needle)
for needle in ("nevent_advance_tick();", "affect_update();", "point_update();",
               "latency_trace_record(\"total_tick\"", "select(0, (fd_set *)0"):
    assert contains(reset, needle)

loop_start = index(comm, "while (!shutdownflag)")
loop_end = index(comm, "\n\tif (_copyover)", loop_start)
loop = comm[loop_start:loop_end]
calls = [f"{phase}(context)" for phase in PHASES]
positions = [index(loop, call) for call in calls]
assert positions == sorted(positions)
assert contains(loop, "if (!run_connection_phase(context))")

# The orchestration boundary makes the important cross-phase relationships
# executable as source contracts: output remains before events, and recurring
# durable completion work remains after events and output.
assert index(loop, "run_output_phase(context)") < index(loop, "run_event_phase(context)")
assert index(loop, "run_event_phase(context)") < index(loop, "run_recurring_persistence_phase(context)")
assert index(loop, "run_session_input_phase(context)") < index(loop, "run_output_phase(context)")

for needle in ("session_input_route::pager", "session_input_route::editor",
               "session_input_route::playing", "session_input_route::nanny",
               "session_input_route_dispatches"):
    assert contains(session_header, needle)
selection = comm[index(comm, "static session_input_route select_session_input"):
                 index(comm, "static void dispatch_session_input")]
for needle in ("casting_input_for_descriptor", "get_casting_cmd_from_q",
               "get_playing_cmd_from_q", "item_creation_grant_blocks_commands"):
    assert contains(selection, needle)

print("game-loop phase source contracts passed")
