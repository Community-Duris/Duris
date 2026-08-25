# Incident Report & Investigation: Relic Pickup (`wear relic badge`) Server Crash

**Incident Date/Time:** August 25, 2026 at 14:00:12 IDT
**Severity:** High (Crash / SIGSEGV 139)
**Status:** Fixed, built, regression-tested
**Target Subsystems:** Random labyrinth / relic system (`random.zone.c`), object special procedures (`specs.assign.c`, `interp.c`)

---

## 1. Executive Summary

Player `Amoz` (PID 58) ran `take all.relic hole`, pulling artifacts `#68` (*Relic of the Spider Queen LLoth*)
and `#59` (*Relic of the Undead Kings*) into inventory, then typed `wear relic badge`. The server died with
`SIGSEGV` before producing any further log output.

Root cause is not in the wear code. It is `reset_lab()` in `src/random.zone.c`, which indexes `world[]` with
the return value of `real_room()` without checking for `NOWHERE` (`-1`). The labyrinth vnums it walks
(`700000+`, `800000+`, `900000+`) are largely absent from the loaded world, so the function performed
thousands of `world[-1]` accesses — writing `world[-1].sector_type` and then walking the garbage pointers in
`world[-1].people` / `world[-1].contents`.

---

## 2. Trigger Path

1. `take all.relic hole` moves relics `#68` and `#59` from the portable hole into inventory. Object special
   procedures do **not** fire for items inside a container, so nothing happens yet.
2. The next command typed by the player runs `special()` (`src/interp.c:1774`) **before** the command itself.
   `special()` walks `ch->carrying`, so `relic_proc()` (`src/random.zone.c`) now fires for both relics.
3. `relic_proc()` runs its one-time initialisation block (`obj->value[6] == 0`), which calls
   `update_relic(ch, obj)`.
4. `update_relic()` ends with `reset_lab(1)` for vnum `#68` and `reset_lab(2)` for vnum `#59`.
5. `reset_lab()` crashes.

The command itself (`wear`, `wear all`, or anything else) is irrelevant — it never executes. The `wear all`
crash investigated earlier the same day has the same signature (first command after taking relics out of the
portable hole) and was most likely this same defect.

## 3. Defect Detail

```c
while (i < 10000)
{
    world[real_room(start_room + i)].sector_type = sector_type;   /* real_room() may return -1 */
    i++;                                                          /* index advanced mid-room   */
    for (vict = world[real_room(start_room + i)].people; ...)     /* -> world[-1].people       */
    for (obj  = world[real_room(start_room + i)].contents; ...)   /* -> world[-1].contents     */
}
world[real_room(entrance_room)].dir_option[DIR_DOWN] = 0;
```

Room availability in the current world (from `areas/wld/*.wld`, cross-checked against `areas/AREA`):

| Lab | vnum range | Missing rooms in range |
|-----|------------|------------------------|
| type 0 | 700000–710000 | 9,359 |
| type 1 | 900000–910000 | 10,000 (`map22.wld` is not listed in `areas/AREA`) |
| type 2 | 800000–810000 | 8,839 |

The `i++` placement is a second, latent bug: the room whose sector type is stamped is not the room whose
contents are drained, and the very first room of each lab was never drained at all.

## 4. Fix

`src/random.zone.c`:

- `reset_lab()` resolves each room once per iteration, skips `NOWHERE`, resolves the entrance room once and
  guards it, rejects unknown lab types instead of using uninitialised vnums, and no longer advances the loop
  index between the sector stamp and the room drain.
- `create_lab()` aborts (with a wizlog) when its start room or map room is not in the world table, and guards
  the `ROOM_NO_TELEPORT` stamp.
- `connect_lab()` and `connect_other()` resolve both endpoints and bail out rather than indexing `world[-1]`.
  These are only reachable through the immortal `randobj map` / `randobj remove` commands, which had the same
  crash available to them.

## 5. Validation

- `make -C src` — clean build (`src/dms_new`).
- `python3 tests/async/test_relic_lab_reset_bounds.py` — new regression test, all checks pass.
- `python3 tests/async/test_wear_all_regression.py` — still passes.
- Not run: live in-game reproduction. The running server is still on the pre-fix `./dms` binary; the fixed
  binary is promoted by `scripts/cycle_mud.sh` on the next restart.
