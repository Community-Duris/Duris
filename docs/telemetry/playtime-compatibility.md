# Compatibility playtime (#259)

## Contract

Compatibility playtime measures the time a player character remains resident in
the game. It is **not** active-input time, anti-idle time, or the future telemetry
session measure described by #258/#260.

- The live `player.time.played` value remains the loaded or administrator-set
  baseline. `player.time.logon` remains the existing wall-clock session anchor.
- A save captures baseline plus nonnegative elapsed seconds since that anchor.
  It does not advance either live field. Repeated captures and retries therefore
  cannot compound the same session interval.
- An uninitialized/nonpositive anchor, failed clock, or clock before the anchor
  contributes no elapsed seconds. Values saturate at signed `INT_MAX`, matching
  the SQL column. The live baseline is unsigned; corrupt out-of-range values are
  capped, not reinterpreted as recoverable historical playtime.
- A normal load establishes the captured total as the next baseline and resets
  `logon` to the load time. Time while the character is unloaded is excluded.
- A linkdead character still resident in the world continues accumulating time.
  Reattaching its descriptor does not start a new accounting interval. This
  preserves the existing score/logon convention rather than introducing a new
  activity policy in a compatibility fix.
- Staff-set baselines and the existing `EQ_WIPE` offset remain in the stored
  baseline. Display code continues subtracting the offset where it already did.
  This work does not move the equipment-wipe/login workflow or infer missing
  historical offsets.

`player/player_playtime.h` owns the shared calculation. Modern immutable capture
and both legacy SQL status INSERT/UPDATE routes call it. A later synchronous
compatibility write must not replace an elapsed total with the old live baseline.
The retired regular-player flat-file fallback is not revived or redesigned.

## Checkpoint ownership and bounded loss

The existing `persistence_checkpoint.c` owner marks **status only** before a
periodic checkpoint, including otherwise quiet characters. It retains the
existing eight-player continuation batches, runtime-ID re-resolution, and wipe
suppression of new dirty marks. No unrelated gameplay mutation is needed to
persist elapsed time; no new timer or telemetry writer is introduced.

The scheduler starts this job after five seconds and repeats it after thirty
seconds, plus continuation/scheduler delay. Under healthy operation a crash can
lose the still-uncaptured tail since the last checkpoint. This is not a hard
thirty-second durability guarantee: capture refusal, journal/DB backlog, or an
unhealthy scheduler can enlarge the exposure. Existing queue/health diagnostics
remain authoritative. This change does not add main-loop I/O or a new durability
fence. Terminal save and copyover durability policies remain unchanged.

Captured totals travel through existing revisioned repositories. Replaying the
same revision is idempotent; an older revision cannot overwrite a newer total.
Crash recovery replays captured values, not a newly calculated session delta.

## Focused verification

Run only the relevant checks; no full regression gate or GitHub CI is required
for this change's local evidence:

```sh
python3 tests/async/test_player_playtime_capture.py
python3 tests/async/test_playtime_checkpoint.py
python3 tests/async/test_playtime_flatfile.py
# Requires MySQL development headers and mysql_config (use the build container):
python3 tests/async/test_playtime_legacy_sql.py
```

- Controlled-clock capture: 3,600 + 600 = 4,200; repeat saves; terminal/death
  intent; resident linkdead time; reload/offline exclusion; clock/range guards;
  staff baseline and wipe-offset arithmetic; unchanged live time fields.
- Quiet checkpoint: no unrelated dirty components, repeated cycles, eight-player
  continuation boundary preserved.
- Legacy SQL writer: extracted production INSERT/UPDATE with query capture and a
  controlled clock; repeated saves preserve elapsed totals and live clock fields.
  SQL services are stubbed here; the separate journey exercises the real database.
- Real flat-file repository: full baseline, status-only elapsed update, duplicate
  and stale revisions, reload with a fresh session anchor.

With a disposable loopback MariaDB and a fresh branch binary:

```sh
TEST_DB_HOST=127.0.0.1 TEST_DB_USER=<fixture-user> TEST_DB_PASSWORD=<fixture-password> \
  python3 tests/async/test_mysql_playtime_journey.py --server bin/server/dms_new
```

The journey creates its own uniquely named schema, applies the tracked schema and
migrations only there, and removes that schema afterward. It never reads `.env`.
Its SQL repository subtest uses connection-private temporary tables. The gameplay
journey checks explicit/repeated/quiet saves, resident link loss and reconnect,
terminal quit, restart, death/reload, acknowledged-save crash recovery, and
playtime across a real live copyover. A synthetic offline fixture character is
promoted only to invoke the copyover command; the runtime executable is copied
inside the disposable fixture directory.

The minimal fixture's account-menu restoration after copyover is outside this
playtime test. A test development run restored the character and its playtime but
reported `account load failed`; subsequent quit closed the socket rather than
returning to the account menu. That symptom is not diagnosed or fixed here. The
playtime test checks the preserved connection's post-copyover save and then closes
the fixture connection for cleanup; it does not claim account UI correctness.

Build the changed C++ code with `make -C src`. Both MariaDB and flat-file backend
builds must pass; use a separate `DMS_BINARY` for the latter to avoid replacing the
binary used by the SQL journey.

## Historical data and rollout

No migration or historical repair is performed. Old stored totals may omit prior
sessions or reflect legacy/wipe/admin conventions. Do not treat them as verified
active time or backfill guessed seconds. The new calculation affects future
captures only. No production deployment or data mutation is part of this change.

Rollback is a code revert. It does not subtract already retained playtime or
rewrite historical rows. #265 must consume this compatibility contract without
reusing the field as its telemetry session accumulator.
