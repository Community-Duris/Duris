#!/usr/bin/env python3
"""Contract test for temple donation pit and room container custody tracking.

Verifies that:
1. item_command_resolve_put_destination() recognizes containers located in a room
   (such as donation wells, pits, or altars) and resolves authoritative destination
   custody to {item_owner_type::room, room_vnum, 0}.
2. do_donate() fences against in-flight item and currency transactions, and supports
   both canonical well rooms (WELL_ROOM 55126 and 8003).
3. try_to_donate() fences against in-flight transactions, and routes player durable
   donations through item_movement_transaction_submit():
   - transferring to room custody when under the duplicate limit
   - transferring to destruction custody when exceeding the duplicate limit
   - finalizing physical transfer or destruction only via item_donation_completion()
"""

from pathlib import Path
import sys

from _paths import SRC, extract_function
from contract_text import contains

policy_c = (SRC / "item_command_policy.c").read_text(encoding="utf-8", errors="replace")
actoth_c = (SRC / "actoth.c").read_text(encoding="utf-8", errors="replace")

checks = []

# 1. item_command_resolve_put_destination room container resolution
resolve_fn = extract_function("item_command_policy.c", "bool item_command_resolve_put_destination(")
checks.append((
    "item_command_resolve_put_destination checks room-located containers",
    contains(resolve_fn, "if (OBJ_ROOM(container) && actor->in_room > NOWHERE && container->loc.room == actor->in_room)")
))
checks.append((
    "item_command_resolve_put_destination assigns room owner to room containers",
    contains(resolve_fn, "destination->owner = { item_owner_type::room,")
    and contains(resolve_fn, "world[actor->in_room].number")
    and contains(resolve_fn, "destination->reason = item_transfer_reason::player_put;")
))

# 2. do_donate fences
donate_fn = extract_function("actoth.c", "void do_donate(P_char ch, char *argument, int /*cmd*/)")
checks.append((
    "do_donate fences in-flight item movement and currency transactions",
    contains(donate_fn, "item_movement_transaction_player_busy(ch)")
    and contains(donate_fn, "currency_transaction_player_busy(ch)")
))
checks.append((
    "IN_WELL_ROOM supports both WELL_ROOM and room 8003",
    contains(actoth_c, "#define IN_WELL_ROOM(x)")
    and contains(actoth_c, "WELL_ROOM")
    and contains(actoth_c, "8003")
))

# 3. try_to_donate durable custody transactions
try_donate_fn = extract_function("actoth.c", "void try_to_donate(P_char ch, P_obj obj_to_put)")
checks.append((
    "try_to_donate fences in-flight transactions",
    contains(try_donate_fn, "item_movement_transaction_player_busy(ch)")
    and contains(try_donate_fn, "currency_transaction_player_busy(ch)")
))
checks.append((
    "try_to_donate routes durable player donations through item_movement_transaction_submit",
    contains(try_donate_fn, "if (IS_PC(ch) && item_command_uses_durable_ownership(obj_to_put))")
    and contains(try_donate_fn, "item_movement_transaction_submit(")
))
checks.append((
    "try_to_donate transfers to room custody when under dupes limit",
    contains(try_donate_fn, "item_owner_type::room")
    and contains(try_donate_fn, "world[ch->in_room].number")
    and contains(try_donate_fn, "item_transfer_reason::player_put")
))
checks.append((
    "try_to_donate transfers to destruction custody when exceeding dupes limit",
    contains(try_donate_fn, "item_owner_type::destruction")
    and contains(try_donate_fn, "item_transfer_reason::destruction")
))
checks.append((
    "item_donation_completion finalizes placement or destruction on transaction commit",
    contains(actoth_c, "item_donation_completion")
    and contains(actoth_c, "extract_obj(object);")
    and contains(actoth_c, "obj_to_obj(object, well);")
))

failed = [label for label, ok in checks if not ok]
if failed:
    for f in failed:
        print(f"FAILED: {f}")
    sys.exit(1)

print(f"OK: {len(checks)} donation pit and room container custody checks passed.")
