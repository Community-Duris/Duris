# Event Loop Hotspots

**Date:** August 25, 2026
**Status:** Fixed, measured, deployed

Follow-up to `crash-investigation-stuck-command-gate.md`. With the wheel no longer
losing events, the remaining lag came from a handful of callbacks that each ate most
of a pulse's 25ms budget on their own. A budget check happens *between* callbacks, so
one slow job overruns the pulse regardless of policy — these had to be made cheaper.

## Measured before (38+ samples per callback, from `NEVENT BUDGET` log lines)

| callback | n | avg | max |
|---|---|---|---|
| `generic_char_event` | 38 | 17.8 ms | 24.1 ms |
| `event_write_statistic` | 1 | 24.1 ms | 24.1 ms |
| `event_artifact_check_poof_sql` | 18 | 5.3 ms | 10.5 ms |

## Fixes

**`generic_char_event` (`handler.c`)** — swept every character in the game in one
callback. Split into 4 slices, one per invocation, rescheduled at a quarter of the old
delay. The slice comes from a hash of the character's address, so it is stable for the
character's lifetime: every character is still visited exactly once per 20 s, nobody is
skipped or done twice. The mob sanity check still runs on every pass. **17.8 ms → 6.3 ms
average.**

**`sql_trace_enabled()` (`sql.c`)** — both branches assigned `true`, so SQL tracing was
permanently on regardless of the `SQL_TRACE` environment variable, writing two log lines
(each an open/append/close) for every query the game runs. This was the bulk of
`event_write_statistic`'s cost — the INSERT itself measures ~1.3 ms on a warm connection.
Now opt-in, as the code intended. **`event_write_statistic` no longer appears as a slow
callback at all.**

**GETDBG traces (`actobj.c`)** — 36 unconditional `logit(LOG_DEBUG, ...)` calls left over
from the pickup investigation, firing ~10 lines per `get` by every character in the game,
mobs included. Kept, but gated behind `GET_TRACE`, matching the `SQL_TRACE` pattern.

**`event_artifact_check_poof_sql` (`artifact.c`)** — runs every 12 s and issued a SELECT
*and* a clearing UPDATE unconditionally, so the common case (nothing to poof) still paid
for a write and its commit. The SELECT now uses the same predicate as the UPDATE, with
not-in-game rows skipped in the loop, and the UPDATE only runs when the SELECT returned
rows. Same semantics, one round trip instead of two in the steady state.

## Result

Debug log volume dropped from ~1 MB per 20 minutes (one player online) to ~1 KB per boot.
No callback now exceeds ~12 ms; the deferred backlog sits at ~250 with the 25 ms time
budget as the binding limit, and `NEVENT SLOW` (>50 ms loop) has not fired since.

## Known-benign, deliberately not changed

- `Heaven has invalid number: 1 (should be 0)` at boot — `recalc_zone_numbers()` finding
  and correcting a zone number that disagrees with its lowest room vnum. Self-healing;
  the fix would be world-data surgery on zone numbering with a wide blast radius.
- `PERSISTENCE: worker_unavailable_flat_fallback` (5 lines) at boot — item events fired
  during world load are written to the flat fallback and replayed before the workers
  start ("replayed 8 fallback persistence events; 0 remain queued"). Working as designed,
  nothing lost.
- Mob log `RIDICULOUS damage` / `M cmd not executed` notices — area data, not code.
- 246 `-Wformat-truncation=` compiler warnings — pre-existing legacy `snprintf` into fixed
  buffers, safe by construction. Fixing them is a mass edit across ~40 files.
