#!/usr/bin/env python3
"""Contract test for corpse expiration arbitration and scavenger looting under death conflicts.

Verifies that:
1. corpse_has_death_conflict() checks in-flight corpse lifecycle transactions and active
   corpse-raise save-fenced players for the corpse's PID.
2. Corpse room release and destruction deferrals re-arm decay if corpse_has_death_conflict()
   is true, preventing premature destruction while player state is in flight.
3. submit_corpse_destruction() refuses destruction when corpse_has_death_conflict() is true.
4. Scavenger mobs skip container looting from corpses experiencing an active death conflict.
"""

from pathlib import Path
import sys

from _paths import SRC, extract_function
from contract_text import contains

handler_c = (SRC / "handler.c").read_text(encoding="utf-8", errors="replace")
handler_h = (SRC / "handler.h").read_text(encoding="utf-8", errors="replace")
mobact_c = (SRC / "mobact.c").read_text(encoding="utf-8", errors="replace")

checks = []

# 1. Header declaration
checks.append((
    "handler.h declares corpse_has_death_conflict",
    "bool corpse_has_death_conflict(P_obj);" in handler_h
    or "bool corpse_has_death_conflict(P_obj corpse);" in handler_h
))

# 2. corpse_has_death_conflict implementation
conflict_fn = extract_function("handler.c", "bool corpse_has_death_conflict(P_obj corpse)")
checks.append((
    "corpse_has_death_conflict checks ITEM_CORPSE and PC_CORPSE",
    contains(conflict_fn, "corpse->type != ITEM_CORPSE")
    and contains(conflict_fn, "PC_CORPSE")
))
checks.append((
    "corpse_has_death_conflict checks corpse_lifecycle_transaction_busy",
    contains(conflict_fn, "corpse_lifecycle_transaction_busy(")
))
checks.append((
    "corpse_has_death_conflict checks corpse_raise_player_save_fenced on matching player",
    contains(conflict_fn, "corpse_raise_player_save_fenced(temp_ch)")
    and contains(conflict_fn, "GET_PID(temp_ch) == pid")
))

# 3. submit_corpse_destruction checks death conflict
destruction_fn = extract_function("handler.c", "bool submit_corpse_destruction(P_obj corpse)\n{")
checks.append((
    "submit_corpse_destruction checks corpse_has_death_conflict",
    contains(destruction_fn, "if (corpse_has_death_conflict(corpse))")
    and contains(destruction_fn, "return false;")
))

# 4. persistence_defer_corpse_room_release checks death conflict and rearms
release_fn = extract_function("handler.c", "bool persistence_defer_corpse_room_release(P_obj corpse)")
checks.append((
    "persistence_defer_corpse_room_release rearms on death conflict",
    contains(release_fn, "if (corpse_has_death_conflict(corpse))")
    and contains(release_fn, "rearm_corpse_release(corpse);")
    and contains(release_fn, "return true;")
))

# 5. persistence_defer_corpse_destruction checks death conflict and rearms
destruct_defer_fn = extract_function("handler.c", "bool persistence_defer_corpse_destruction(P_obj corpse)")
checks.append((
    "persistence_defer_corpse_destruction rearms on death conflict",
    contains(destruct_defer_fn, "if (corpse_has_death_conflict(corpse))")
    and contains(destruct_defer_fn, "rearm_corpse_release(corpse);")
    and contains(destruct_defer_fn, "return true;")
))

# 6. Scavenger mobs skip looting during conflict
mundane_fn = extract_function("mobact.c", "void event_mob_mundane(")
checks.append((
    "event_mob_mundane skips looting conflicted corpses",
    contains(mundane_fn, "if (corpse_has_death_conflict(best_obj))")
    and contains(mundane_fn, "goto normal;")
))

failed = [label for label, ok in checks if not ok]
if failed:
    for f in failed:
        print(f"FAILED: {f}")
    sys.exit(1)

print(f"OK: {len(checks)} corpse death conflict arbitration checks passed.")
