# Incident Report & Investigation: `wear all` Server Crash

**Incident Date/Time:** August 25, 2026 at 13:14:03 IDT  
**Severity:** High (Crash / SIGSEGV 139)  
**Status:** In Progress / Root Causes Identified & Fix in Development  
**Target Subsystems:** Character affects system (`affects.c`), Object lifecycle (`db.c`, `handler.c`), Equipment wearing logic (`actobj.c`)

---

## 1. Executive Summary

At 13:14:03 IDT, the MUD server crashed with a segmentation fault (`SIGSEGV`, exit status 139). The automated service supervisor (`duris-mud.service` / `cycle_mud.sh`) detected the termination and restarted the game at 13:14:17 IDT.

Investigation of `cmd.debug`, `debug`, and system logs identified that player `Amoz` (PID 58, Level 56 Dragon Dragoon) triggered the crash upon executing `wear all` in room `96537` after retrieving high-end equipment prototypes and artifacts from a portable hole (`#19780`).

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

Four distinct vulnerabilities in affect processing, array bounding, and slot resolution were identified during code inspection and trace analysis:

### 3.1. Out-of-Bounds Memory Read in `affect_modify` (`src/affects.c:1040`)
- **Location:** [`src/affects.c:1027-1041`](file:///home/aiwithapex/projects/duris/src/affects.c#L1027-L1041)
- **Defect:** 
  ```c
  void affect_modify(int loc, int mod, unsigned long *bitv, int from_eq)
  {
      if (bitv)
      {
          ...
          SET_BIT(TmpAffs.BV_2, bitv[1]);
          SET_BIT(TmpAffs.BV_3, bitv[2]);
          SET_BIT(TmpAffs.BV_4, bitv[3]);
          SET_BIT(TmpAffs.BV_5, bitv[4]);
          SET_BIT(TmpAffs.BV_6, bitv[5]); // <-- OUT OF BOUNDS
      }
  ```
- **Analysis:** `struct obj_data` ([`src/structs.h:540-545`](file:///home/aiwithapex/projects/duris/src/structs.h#L540-L545)) and `struct affected_type` ([`src/structs.h:2340`](file:///home/aiwithapex/projects/duris/src/structs.h#L2340)) define exactly 5 bitvector banks (`bitvector` through `bitvector5`, indices `0..4`). When `affect_modify` is called from `all_affects` via `&(obj->bitvector)`, accessing `bitv[5]` reads past the bitvector fields into adjacent object struct members (`affected[0]` and pointers), injecting corrupted bits into `TmpAffs.BV_6` and corrupting memory during continuous affect resolution.

### 3.2. Unbounded Array Access in `calculate_hitpoints2` (`src/affects.c:347-351`)
- **Location:** [`src/affects.c:347-351`](file:///home/aiwithapex/projects/duris/src/affects.c#L347-L351)
- **Defect:** 
  ```c
  if (obj->affected[j].location == APPLY_CON_RACE && stat_factor[obj->affected[j].modifier].Con > racial_con)
  {
      race       = obj->affected[j].modifier;
      racial_con = stat_factor[race].Con;
  }
  ```
- **Analysis:** If an object's `APPLY_CON_RACE` modifier is negative or exceeds `LAST_RACE` (100), `stat_factor[modifier]` reads unmapped memory outside the `stat_factor[]` array bounds during hitpoint calculation.

### 3.3. Unchecked `R_num` Indexing in `free_obj` (`src/db.c:3958`)
- **Location:** [`src/db.c:3958`](file:///home/aiwithapex/projects/duris/src/db.c#L3958)
- **Defect:**
  ```c
  if (obj_index[obj->R_num].func.obj == barb)
  ```
- **Analysis:** Dynamic objects or objects without an instantiated prototype retain `obj->R_num = -1`. While `extract_obj` guards its decrement with `if (obj->R_num >= 0)`, `free_obj` unconditionally indexed `obj_index[-1]`, creating a crash risk whenever memory-pooled objects are released.

### 3.4. Invalid Equipment Slot Assignment in `wear()` (`src/actobj.c:4968`)
- **Location:** [`src/actobj.c:4966-4978`](file:///home/aiwithapex/projects/duris/src/actobj.c#L4966-L4978)
- **Defect:** When a character equips multiple `ITEM_HOLD` items, if `HOLD` (slot 18) is already occupied, the logic falls back to:
  ```c
  equip_char(ch, obj_object, ch->equipment[HOLD] ? WIELD : HOLD, !showit);
  ```
- **Analysis:** Placing non-weapon objects (such as containers, wands, or staves) into the `WIELD` (`PRIMARY_WEAPON`) slot corrupts weapon state assumptions in combat pulse and damage multiplier calculations (`class_hitpoints` / `combat_by_class`).

---

## 4. Remediation Plan

1. **Remove `bitv[5]` Read in `affect_modify`:**
   Restrict bitvector copying to valid banks `bitv[0]` through `bitv[4]`.
2. **Add Bounds Checking in `calculate_hitpoints2`:**
   Ensure `obj->affected[j].modifier >= 0 && obj->affected[j].modifier <= LAST_RACE` before indexing `stat_factor[]`.
3. **Guard Prototype Index in `free_obj`:**
   Add `if (obj->R_num >= 0 && obj_index[obj->R_num].func.obj == barb)`.
4. **Enforce Weapon-Only Slots in `actobj.c`:**
   Prevent `wear` from putting non-weapon items into `PRIMARY_WEAPON` or secondary weapon slots when resolving `ITEM_HOLD`.
5. **Regression Testing:**
   Implement a focused test in `tests/async/` verifying multi-item wear, artifact affect recalculation, and dragon race stat calculations.

---

## 5. Next Actions

- Implement code modifications across `src/affects.c`, `src/db.c`, and `src/actobj.c`.
- Build the binary (`make -C src`).
- Run the regression test suite.
- Commit and push to `origin/master`.
- Restart `duris-mud.service` to apply the patch cleanly.
