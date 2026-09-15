#!/usr/bin/env python3
"""Focused source-contract regression for the #265 gameplay hook paths."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def text(relative: str) -> str:
    return (SRC / relative).read_text()


def function_slice(source: str, start: str, end: str) -> str:
    begin = source.index(start)
    finish = source.index(end, begin)
    return source[begin:finish]


def main() -> None:
    comm = text("net/comm.c")
    dispatch = function_slice(
        comm,
        "static void dispatch_playing_command",
        "void game_loop",
    )
    assert "telemetry_runtime_game_evidence" not in dispatch
    assert "telemetry_runtime_options_from_environment" in comm
    close_start = comm.index("void close_socket(struct descriptor_data *d)")
    close = comm[close_start : close_start + 6000]
    assert "telemetry_connection_transition_kind::detached" in close
    assert "telemetry_runtime_evidence_kind::linkdead" in close
    assert "d->original && IS_PC(d->original)" in close

    nanny = text("account/nanny.c")
    enter = function_slice(nanny, "void enter_game", "void select_terminal")
    assert "STATE(d) == CON_PLAYING" in enter
    assert "telemetry_runtime_game_enter(ch, d)" in enter
    assert "telemetry_runtime_game_context(ch, d)" in enter
    legacy = function_slice(nanny, "void select_main_menu", "void nanny")
    assert "STATE(d) = CON_PLAYING" in legacy
    assert "telemetry_runtime_game_enter(d->character, d)" in legacy
    assert "telemetry_runtime_game_context(d->character, d)" in legacy

    account = text("account/account.c")
    reconnect = function_slice(account, "int is_char_in_game", "struct acct_chars *find_char_in_list")
    assert "STATE(d) = CON_PLAYING" in reconnect
    assert "telemetry_connection_transition_kind::attached" in reconnect
    assert "telemetry_runtime_game_context(ch, d)" in reconnect
    nanny_reconnect = function_slice(nanny, "void reconnect", "static void finish_legacy_player_login")
    assert "STATE(d) = CON_PLAYING" in nanny_reconnect
    assert "telemetry_connection_transition_kind::attached" in nanny_reconnect
    assert "telemetry_runtime_game_context(tmp_ch, d)" in nanny_reconnect
    ws = text("net/ws_handlers.c")
    ws_attach = function_slice(ws, "if (online_char)", "/* success - show character selection */")
    assert "d->connected = CON_PLAYING" in ws_attach
    assert "telemetry_connection_transition_kind::attached" in ws_attach
    assert "telemetry_runtime_game_context(online_char, d)" in ws_attach

    actoth = text("cmd/actoth.c")
    quit = function_slice(actoth, "void do_quit", "void event_autosave")
    assert "telemetry_session_end_reason::logout" in quit
    assert quit.index("telemetry_runtime_game_session_exit") < quit.index(
        "extract_char_after_terminal_save"
    )

    handler = text("world/handler.c")
    terminal_exit = function_slice(
        handler,
        "void extract_char_after_terminal_save",
        "void extract_char(",
    )
    assert "ch->telemetry_session_sequence != 0U" in terminal_exit
    assert "telemetry_session_end_reason::disconnect" in terminal_exit

    interp = text("cmd/interp.c")
    classifier = function_slice(
        interp,
        "static telemetry_runtime_evidence_kind telemetry_command_evidence_kind",
        "extern char debug_mode",
    )
    assert "communication" in classifier
    assert "interaction" in classifier
    assert "combat_participation" in classifier
    recognized = function_slice(
        interp,
        "static void telemetry_record_recognized_command",
        "void command_interpreter",
    )
    assert "descriptor->str" in recognized
    assert "descriptor->showstr_count" in recognized
    assert "PLR_PAGING_ON" not in recognized, "paging preference is not an active pager"
    assert "CMD_QUIT" in recognized and "CMD_RENT" in recognized and "CMD_CAMP" in recognized
    assert "telemetry_record_recognized_command(ch, exec_char, cmd)" in interp
    assert interp.index("special(exec_char") < interp.index(
        "telemetry_record_recognized_command(ch, exec_char, cmd)"
    )

    movement = text("cmd/actmove.c")
    moved_at = movement.index("if (!char_to_room(ch, new_room, exitnumb))")
    assert movement.index("telemetry_gameplay_context_changed(ch)", moved_at) > moved_at
    assert movement.index("telemetry_gameplay_movement(ch)", moved_at) > moved_at
    assert "IS_AFFECTED5(ch, AFF5_FOLLOWING)" in movement

    limits = text("world/limits.c")
    afk = limits.index("SET_BIT(i->specials.act, PLR_AFK)")
    assert limits.index("telemetry_runtime_game_context(i, i->desc)", afk) > afk

    # Hook payloads are typed values only; no command/input or identity value is
    # allowed at a telemetry_evidence call site.
    hook_files = [
        "net/comm.c",
        "account/account.c",
        "account/nanny.c",
        "net/ws_handlers.c",
        "cmd/actoth.c",
        "cmd/interp.c",
        "cmd/actmove.c",
        "world/handler.c",
        "world/limits.c",
    ]
    for relative in hook_files:
        lines = text(relative).splitlines()
        for index, line in enumerate(lines):
            if "telemetry_runtime_game_evidence" not in line:
                continue
            window = " ".join(lines[index : index + 6])
            assert "argument" not in window
            assert "input" not in window
            assert "GET_NAME" not in window
            assert "player.name" not in window

    inventory = (ROOT / "docs/telemetry/SESSION_LIFECYCLE.md").read_text()
    for required in (
        "close_socket",
        "is_char_in_game",
        "extract_char_after_terminal_save",
        "PLR_PAGING_ON",
        "descriptor->str",
        "group.c",
        "telemetry_runtime_game_context",
    ):
        assert required in inventory

    print("telemetry gameplay hook source paths passed")


if __name__ == "__main__":
    main()
