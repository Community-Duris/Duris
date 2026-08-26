# Incident Report & Investigation: `wear all` Server Crash

**Incident Date/Time:** August 25, 2026 at 13:14:03 IDT  
**Severity:** High (Crash / SIGSEGV 139)  
**Status:** Resolved / Fixed & Validated  
**Target Subsystems:** Character affects system (`affects.c`), Object lifecycle (`db.c`, `handler.c`), Equipment wearing logic (`actobj.c`), Character stats (`actinf.c`), Macro safety (`utils.h`), Struct definitions (`structs.h`)

---

## 1. Executive Summary

At 13:14:03 IDT, the MUD server crashed with a segmentation fault (`SIGSEGV`, exit status 139). The automated service supervisor (`duris-mud.service` / `cycle_mud.sh`) detected the termination and restarted the game at 13:14:17 IDT.

Investigation of `cmd.debug`, `debug`, and system logs identified that player `Amoz` (PID 58, Level 56 Dragon Dragoon) triggered the crash upon executing `wear all` in room `96537` after retrieving high-end equipment prototypes and artifacts from a portable hole (`#19780`).

All root causes and adjacent memory vulnerabilities have been remediated, verified with targeted regression tests, compiled into `bin/server/dms_new`, and validated against the entire test suite.

---

## 2. Event Timeline

1. **13:06–13:11 IDT**: Immortal `Zusuk` executed prototype evaluation commands (`eqrate`) and loaded several high-tier items into room `96535`/`96537`:
   - `#59` (*The blood-splattered Relic of the Undead Kings*) – Multi-slot artifact
   - `#68` (*The ancient Relic of the Spider Queen LLoth*) – Artifact
   - `#51006` (*Tiamat's poison tail stinger*)
   - `#71226` (*the charred armor named 'Devastation'*)
   - `#44192` (*the sapphire necklace of Insanity*, loaded 2x)
   - `#51003` (*the staff of Hellspawn*)
2. **13:12:12 IDT**: `Amoz` picked up container `#19780` (*portable hole*) containing the loaded objects.
3. **13:14:02 IDT**: `Amoz` ran `take all hole`, extracting `#59`, `#51003`, `#19740` (wands), and potions into inventory.
4. **13:14:03 IDT**: `Amoz` entered `wear all`.
5. **13:14:03 IDT**: Server encountered `SIGSEGV` (signal 11) during equipment affect application and terminated.
6. **13:14:17 IDT**: `cycle_mud.sh` completed crash dump rotation and restarted the MUD.

---

## 3. Root Cause Analysis

Four distinct vulnerabilities in affect processing, array bounding, and slot resolution were identified and resolved:

### 3.1. Out-of-Bounds Memory Read in `affect_modify` (`src/affects.c:1040`)
- **Location:** [`src/affects.c:1042-1056`](file:///home/aiwithapex/projects/duris/src/affects.c#L1042-L1056)
- **Defect:** `affect_modify` attempted to read `bitv[5]` and write to `TmpAffs.BV_6`.
- **Analysis:** `struct obj_data` ([`src/structs.h:540-545`](file:///home/aiwithapex/projects/duris/src/structs.h#L540-L545)) and `struct affected_type` ([`src/structs.h:1218-1222`](file:///home/aiwithapex/projects/duris/src/structs.h#L1218-L1222)) define exactly 5 bitvector banks (`bitvector` through `bitvector5`, indices `0..4`). Accessing `bitv[5]` read past the bitvector array into adjacent struct fields (`affected[0]` and pointers), corrupting memory and crashing the server during continuous affect resolution.
- **Fix:** Removed `SET_BIT(TmpAffs.BV_6, bitv[5]);` and removed `BV_6` from `struct hold_data` in [`src/structs.h`](file:///home/aiwithapex/projects/duris/src/structs.h). Corrected logging parameter from `loc` to `mod` across `APPLY_*_RACE` switch cases.

### 3.2. Unbounded Array Access in `calculate_hitpoints2`, `apply_affs`, `affect_total`, and `do_score`
- **Location:** [`src/affects.c:340-355, 684-770, 1638`](file:///home/aiwithapex/projects/duris/src/affects.c#L340-L355), [`src/actinf.c:3995-4045`](file:///home/aiwithapex/projects/duris/src/actinf.c#L3995-L4045)
- **Defect:** `stat_factor[]` and `combat_by_race[]` (size `[LAST_RACE + 1]`) were indexed with raw character race and equipment `affected[].modifier` without range verification.
- **Analysis:** When evaluating stats or equipment with unexpected modifiers or custom race values, negative or out-of-bounds race indices read invalid memory outside table allocations.
- **Fix:** Added `BOUNDED(0, race, LAST_RACE)` checks in `calculate_hitpoints2`, `apply_affs`, `affect_total`, and `do_score`.

### 3.3. Unchecked `R_num` Indexing in `free_obj`, `actobj.c`, and `utils.h`
- **Location:** [`src/db.c:3958`](file:///home/aiwithapex/projects/duris/src/db.c#L3958), [`src/actobj.c:3953, 5408, 5454, 5545, 5623, 5681, 5808`](file:///home/aiwithapex/projects/duris/src/actobj.c), [`src/utils.h:460-462`](file:///home/aiwithapex/projects/duris/src/utils.h#L460-L462)
- **Defect:** `obj_index[obj->R_num]` was accessed without verifying `obj->R_num >= 0`.
- **Analysis:** Dynamic objects or uninstantiated items have `R_num = -1`. Accessing `obj_index[-1]` corrupts memory or segfaults during object destruction, item search, and wear/remove checks.
- **Fix:** Added `obj->R_num >= 0` guards in `free_obj`, `do_wear`, `do_grab`, `do_remove`, `do_search`, and within the `OBJ_VNUM(obj)` / `GET_OBJ_PROC(obj)` macros in `utils.h`.

### 3.4. Invalid Equipment Slot Assignment in `wear()` (`src/actobj.c:4915-4981`)
- **Location:** [`src/actobj.c:4824-4981`](file:///home/aiwithapex/projects/duris/src/actobj.c#L4824-L4981)
- **Defect:** When `HOLD` (slot 18) was occupied, `wear()` fell back to equipping non-weapon `ITEM_HOLD` items into `WIELD` (`PRIMARY_WEAPON`), `WIELD3`, or `WIELD4`.
- **Analysis:** Equipping non-weapon objects into weapon slots broke core combat invariants and corrupted combat round and damage calculations.
- **Fix:** Refactored `case 13 (HOLD)` in `wear()` to reject equipping when `HOLD` is already occupied, and hardened `case 12 (WIELD)` to verify secondary and 4-handed weapon slot capacity before calling `execute_wear`.

---

## 4. Implementation Summary & Touched Files

| File | Changes Made |
| :--- | :--- |
| [`src/structs.h`](file:///home/aiwithapex/projects/duris/src/structs.h#L2386) | Removed out-of-bounds `BV_6` bitvector field from `struct hold_data`. |
| [`src/affects.c`](file:///home/aiwithapex/projects/duris/src/affects.c) | Removed `bitv[5]` read from `affect_modify`; bounded `stat_factor` and `combat_by_race` lookups in `calculate_hitpoints2`, `apply_affs`, and `affect_total`; corrected `mod` logging parameter. |
| [`src/db.c`](file:///home/aiwithapex/projects/duris/src/db.c#L3958) | Guarded `obj->R_num >= 0` in `free_obj` before checking `obj_index[obj->R_num].func.obj == barb`. |
| [`src/actobj.c`](file:///home/aiwithapex/projects/duris/src/actobj.c#L4824-L4981) | Fixed `wear()` `case 12` and `case 13` to prevent non-weapon equipping into weapon slots; guarded `R_num >= 0` across all `obj_index` lookups. |
| [`src/actinf.c`](file:///home/aiwithapex/projects/duris/src/actinf.c#L3995-L4045) | Guarded character race and equipment modifier bounds in `do_score` before indexing `stat_factor`. |
| [`src/utils.h`](file:///home/aiwithapex/projects/duris/src/utils.h#L460-L462) | Hardened `OBJ_VNUM` and `GET_OBJ_PROC` macros to return `-1`/`NULL` if `R_num < 0`. |
| [`tests/async/test_wear_all_regression.py`](file:///home/aiwithapex/projects/duris/tests/async/test_wear_all_regression.py) | Added comprehensive regression contract covering all 4 vulnerability domains. |

---

## 5. Verification & Test Results

- **Targeted Regression Suite:**
  `python3 tests/async/test_wear_all_regression.py` — **14/14 checks PASSED**.
- **Full Async Test Suite:**
  All 120+ async test contracts (`tests/async/test_*.py`) — **100% PASSED**.
- **Build Verification:**
  `make -C src` compiled cleanly with `g++ -std=c++20`, generating executable `bin/server/dms_new`.
