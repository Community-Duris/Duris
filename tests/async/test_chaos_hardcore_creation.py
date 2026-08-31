#!/usr/bin/env python3
"""Regression contract for hardcore creation during chaos."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
NANNY = (ROOT / "src/nanny.c").read_text()
WS = (ROOT / "src/ws_handlers.c").read_text()

policy = "chaos_mud_enabled() && hardcore_config_get()->disable_in_chaos"

select_sex = NANNY[NANNY.index("void select_sex(") : NANNY.index("static void display_available_races", NANNY.index("void select_sex("))]
select_hardcore = NANNY[NANNY.index("void select_hardcore(") : NANNY.index("void select_sex(")]
create_character = WS[WS.index("void ws_cmd_create_character(") :]

assert policy in select_sex
assert policy in select_hardcore
assert policy in create_character
assert '#include "chaos_config.h"' in WS

print("chaos hardcore creation contract passed")
