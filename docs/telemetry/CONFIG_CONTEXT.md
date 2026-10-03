# Telemetry effective configuration context (#263)

This module owns the immutable, value-only configuration context that must be
admitted before a later telemetry interval can use its `config_id`. It does not
read SQL, write files, inspect live characters, or change gameplay balances.
The state and all callbacks are caller-owned and game-thread bounded.

## Capture boundary and effective allowlist

`telemetry_config_snapshot_build()` receives the build/content/classifier/policy
versions, season/environment namespace, timing controls, backend and enabled
state from the configuration owner. It also receives a typed property reader and,
for enabled snapshots, a bounded resolver for a preloaded reviewed property
catalog. The resolver is not optional at the usable enabled-snapshot boundary.
The reader returns the already-effective `float32` value produced by the game's
`get_property()` overload; this preserves the existing float storage and the
integer overload's truncation behavior. It is never passed a property array or a
secret-bearing environment object.

The reviewed registry is intentionally small. The `stable_name` values are
module metadata, not values copied into the public snapshot or telemetry row.
Only entries with `effective` or `hardcoded_effective` roles contribute to the
`property_version`; the unavailable marker is also retained in the typed
registry so missing context is visible.

| Stable entry | Source/effective behavior | Registry role |
|---|---|---|
| `rested.xp_multiplier` | fixed constant `1.5f` in the XP path; applies when enabled or explicitly staff-granted | effective hard-coded |
| `wellrested.xp_multiplier` | fixed constant `2.0f` in the XP path; applies when enabled or explicitly staff-granted | effective hard-coded |
| `rested.enabled` | `get_property("exp.rested.enabled", 1)`; an absent property defaults to `1`; `0` suppresses ordinary automatic rested tiers, with an explicit staff-granted immortal/newbie affect as an exception | effective property |
| `rested.resurrect_exempt` | resurrect branch bypasses rested modifiers | effective hard-coded |
| `trophy.exp.zoneTrophy.observe` | `get_property(..., 0)`; current properties file is `0` | effective property |
| `trophy.min_level` | hard-coded level `25` | effective hard-coded |
| `trophy.max_level` | `MAXLVLMORTAL`, currently `56` | effective hard-coded |
| `trophy.excludes_illithid` | `IS_ILLITHID()` rejects observation | effective hard-coded |
| `trophy.excludes_pc_pets` | `IS_PC_PET()` rejects the opponent | effective hard-coded |
| `trophy.xp_penalty` | observation records accepted XP and adds no penalty | effective hard-coded, false |
| `payout.epic.touch.maxPayoutFactor` | `get_property(..., 10.0)` | effective property |
| `payout.epic.touch.PayoutFactor` | `get_property(..., 1.0)` | effective property |
| `payout.epic.zone.alignmentMod` | `get_property(..., 0.10)` coefficient; current file value is `0.20` | effective property |
| `payout.epic.alignment.minPercentage` | `get_property(..., 0.10)` floor; current file value is `0.15` | effective property |
| `payout.zone_alignment_context` | zone alignment comes from the database at payout time | unavailable context |

The multiplier entries are the configured gameplay constants, not alternate
disabled-state values. The `rested.enabled` entry is the automatic/default
applicability gate: disabling it does not rewrite `1.5f` or `2.0f` to a no-op
multiplier, and the telemetry context does not invent a no-op hard-coded row.
With the gate at `0`, ordinary automatic rested tiers are suppressed, but an
explicit staff-granted immortal/newbie affect remains an effective exception.
Rested XP flags therefore identify actual application: they are emitted when
the gate is enabled or when the matching staff-marked affect exception applies.
A capture with the gate at `0` still records the same multiplier constants plus a
distinct effective gate value.

The registry also names these maintained properties so they cannot be mistaken
for effective reward controls:

- `epic.freqMod.tick.waitSecs` (fallback `3600`, file `3600`)
- `epic.freqMod.tick.add` (fallback `0.002`, file `0.002`)
- `epic.freqMod.touch.sub` (fallback `0.10`, file `0.20`)
- `epic.freqMod.min` (fallback `0.40`, file `0.05`)
- `epic.freqMod.max` (fallback `2.00`, file `1.55`)

They are `maintained_but_unused` and are excluded from `property_version`: the
payout frequency-modifier path is commented out. A change to one of those
values must not create a false effective payout identity. Runtime zone
alignment is likewise not replaced with a guessed static value; it is explicitly
unavailable for this snapshot.

The payout's base stone value, configured group cap, present in-room player
count, racewar side and database-backed zone alignment remain event-time
inputs. The static snapshot records the max-payout factor, payout factor,
alignment coefficient and minimum alignment floor; it does not claim to
reconstruct a historical payout or group membership from current state.

The allowlist contains no credentials, hostnames, paths, arbitrary property
names, raw property text, JSON, account data or secrets. A required live reader
that is absent or cannot provide an effective value returns
`missing_property_context`; the owner may intentionally select
`declared_defaults` for a reviewed bootstrap/default capture. That choice is
explicit in the input and never silently reuses an old snapshot. Capture also
computes a full effective-property digest, including the effective entry IDs,
roles, kinds and values. Source provenance and maintained-but-unused fields are
not identity: explicit defaults and identical live values resolve identically. An enabled snapshot is rejected as
`property_catalog_unavailable` unless the injected resolver returns that exact
digest with a nonzero stable namespace, stable catalog version and nonzero
property version. A resolver refusal is `property_registry_invalid`.

## Canonical identity and golden compatibility

The public contract's frozen encoding is exactly 70 bytes. The implementation
writes unsigned big-endian integers with no delimiters in this order:

1. `schema_version` (16 bits)
2. `build_version`, `content_version`, `property_version`,
   `classifier_version`, `policy_version` (32 bits each)
3. `season_id`, `environment_id` (64 bits each)
4. `interval_usec`, `checkpoint_interval_usec`, `active_window_usec` (64 bits each)
5. `context_segments_per_minute` (32 bits), `pulse_slot_count` (16 bits),
   `backend` (8 bits), `enabled` (8 bits)

SHA-256 is computed once over those bytes. `config_id`, `revision`,
`effective_utc_usec`, the digest and padding are excluded. Thus publication time
and local ordering do not change the content identity. The property registry
uses its own fixed typed encoding and keeps the complete digest in the private
capture boundary. The public 32-bit `property_version` is only usable when the
preloaded catalog has resolved that complete digest to a stable namespace/version;
the module never allocates a counter or treats a digest prefix as sufficient
historical identity. Distinct full digests that would share a catalog version
must be refused by the catalog resolver rather than allowed to alias.

The focused golden fixture uses the reviewed current values and these identity
inputs:

- build `0x01020304`, content `0x05060708`
- classifier `0x11121314`, policy `0x15161718`
- season `0x0102030405060708`, environment `0x1112131415161718`
- interval `60,000,000`, checkpoint `120,000,000`, active window `300,000,000`
- context cap `8`, pulse slots `16`, SQL backend enabled
- effective properties: rested enabled `1` (the reviewed fallback), observe `0`,
  max factor `10`, payout factor `1`, alignment coefficient `0.20`, alignment
  floor `0.15`

It produces:

```text
property_version = 4128692423
config_id        = 6473875177026978697
fingerprint      = 59d7d32c66e983892b115dda333d75ec73500e13e015c7e3fb90a2bdb4b1f65f
```

The C++ harness independently rebuilds the 70-byte field sequence and compares
its SHA-256 result with this fixture. It also verifies that changing only
revision/effective UTC preserves the identity and changing an effective payout
value changes the identity. Changing only the effective rested gate to `0`
produces a distinct property/config identity:

```text
property_version = 3224700872
config_id        = 2889668470079446183
fingerprint      = 281a2a29d5bb5ca73c5cad6c15887484547e4425c3510306a7e11a93b8f15b70
```

The default builder maps the first eight digest bytes, interpreted big-endian,
to a nonzero `config_id` (with a deterministic fallback if that word is zero).
A durable owner may instead pass an existing namespace mapping. Publication
always recomputes and checks the full fingerprint; a known `config_id` paired
with a different fingerprint is an explicit identity collision, never a new
local counter value. The in-memory collision guard is fixed at 64 entries. If
that guard fills, publication degrades rather than evicting evidence or
inventing an identifier.

## Bounded publication and failure semantics

`telemetry_config_state` contains a four-entry FIFO pending publication set, one
last-admitted current-selection snapshot, a fixed 64-entry identity guard and
saturating counters. Each identity entry retains the first canonical snapshot
and its admission bit. A later A→B→A publication therefore emits A's original
immutable row representation while the current-selection result may carry the
new local revision/effective time. Module state uses fixed-size storage with no explicit heap allocation, mutex,
SQL call, file write or process-local property history. SHA-256 library internals
are not claimed to be allocation-free; hashing is outside per-action capture. The injected key allocator owns
the record sequence; #263 does not reconstruct historical properties with a
private counter.

For an enabled SQL snapshot:

1. Validate the full public representation and recompute SHA-256.
2. Remember/check the `(config_id, fingerprint)` pair.
3. Build one typed `configuration` control record.
4. Allocate its replay key through the injected allocator once, in pending FIFO
   order.
5. Call the injected sink with the fixed-size record.

A sink failure retains the same snapshot and, after key allocation, the same
record key for retry. Until admission succeeds, `telemetry_config_snapshot_copy()`
returns an all-zero snapshot and `telemetry_config_status()` returns
`missing_identity`; it does not return the previous config and does not relabel
new intervals with that old ID. The owner must suppress dependent interval and
checkpoint attribution and mark the affected historical context unknown. A
later successful checkpoint can recover cumulative duration, but cannot repair
the unresolved interval's historical config.

A successful newer publication hides the prior snapshot while it is pending and
makes the new snapshot visible only after its control record is admitted. Older
pending records may still be retried with their original identity without
regressing the visible current snapshot. Invalid input, same-revision conflicts,
identity collisions, pending-capacity failure and reload establish a revision
floor; an older pending retry cannot make a stale snapshot visible. A later
valid current capture above that floor is required. If the bounded pending set or
identity guard is full, the new context is rejected/degraded explicitly; no entry
is overwritten. Repeated disabled and flat-file-disabled publications keep their
disabled outcome and emit no record.

A disabled SQL config is a valid `valid_disabled` snapshot but emits no control
record. The flat-file backend is a valid `valid_flatfile_disabled` snapshot and
also emits no hidden file. Both outcomes are exposed through the public runtime
result and disabled quality state. No gameplay path is made dependent on the
telemetry sink.

## Reload observer seam

The parent-owned `src/telemetry/telemetry_config_reload.h` provides the single
observer pair and a no-op-compatible registration surface. This module does not
write that header or edit `properties.c`. #265 can register:

```cpp
auto *state = telemetry_config_global_state(); // after global_init; same public owner
telemetry_config_reload_register(telemetry_config_reload_observer, state);
```

`telemetry_config_reload_observer()` sets the bit, invalidates current visibility,
clears pending records and records the current revision floor. The game-thread
owner clears the bit, captures the post-`apply_properties()` effective values
through its reader, supplies a strictly newer persisted revision and publishes
the built snapshot. This keeps reload notification after all cached property
updates while preventing an old pending retry from reviving stale config.

## Registry evolution, catalog compatibility and restart handoff

The reviewed property registry is append-only. This change adds ID `20` for
`rested.enabled` and leaves IDs `1..19` byte-for-byte in their existing order and
roles. Keep the property snapshot schema and frozen public 70-byte config
encoding unchanged. Adding a property changes the full typed property digest,
so it must receive a new catalog mapping; it must not be represented by changing
the old multiplier entries or by assigning the old property version to the new
digest.

Catalog updates are additive. Retain the sealed historical mapping for the
previous 19-entry digest (`6f49b7e9b16055b6c7d48d83f4a9de789d89adeddabbb4bfc1f6d2c86f6b8792`,
property version `265`, namespace `265`, catalog version `1`) unchanged.
The reviewed default-on digest is
`f616d8c74b8768d1eaab9e007e2289f431dfe7eaf3a57fac7ee95f29da73eca8` with a new
property version (`4128692423` in the focused fixture). If the disabled gate is
admitted, add its distinct digest
`c03507c8b08d29199431ca57d072d642cf145df9c8b868076cf13fd572e784b6` with
another unused property version (`3224700872` in the focused fixture). Use the
catalog's next stable revision for newly reviewed mappings while retaining all
older lines. The loader's full-digest and unique-property-version checks must
continue to reject aliases.

An already sealed `telemetry_config` row keeps its original property version,
fingerprint, config ID, first revision and effective timestamp. Never rewrite or
delete that row, remove its old catalog line, or reconstruct it from the current
`exp.rested.enabled` value after a restart. A new gate state gets a new immutable
row; it does not mutate historical attribution.

The runtime identity guard is a bounded cache, not durable history. #265/F must
resolve the reviewed registry without a schema change as follows:

1. Build the allowlisted typed registry and its full property digest from the
   reviewed source/defaults and effective reader values.
2. Resolve that digest through a preloaded durable/reviewed catalog callback.
   The callback must return the exact digest, stable namespace, stable catalog
   version and an immutable property version; it must reject duplicate-version
   full-digest collisions and unknown historical entries. It must not allocate a
   process-local counter. Property versions must remain unique across every catalog
   revision used in the same environment. The private stable namespace/catalog
   version is provenance, not an extra field in the frozen public identity.
3. Resolve the full configuration fingerprint against the existing immutable
   `telemetry_config` row owned by #261. If that fingerprint already has a
   namespace `config_id`, reuse that ID and the entire original row representation,
   including its first revision and effective timestamp. Before the first
   publication, call `telemetry_config_state_seed_identity(state, original_row)`
   for the resolved identities. Seeding does not imply transport admission. A
   later current capture keeps its own ordering metadata, while emitted config
   records retain the immutable original row. Do not allocate a new ID on restart.
4. If the fingerprint is new, use the deterministic candidate (or an explicitly
   persisted namespace mapping) and let the worker admit the immutable config row
   before dependent facts. The row's existing fingerprint/version fields are the
   durable configuration resolution point; no new table or migration is needed.
5. If the property catalog cannot resolve the full typed digest, an existing ID
   resolves to a different fingerprint, or the immutable row is absent, expose
   missing context and stop dependent attribution. A `telemetry_config` row alone
   cannot reconstruct arbitrary historical effective property values; never join
   a missing historical row to current live properties.

The reviewed registry definition and its source/default provenance can remain in
F's existing build/reviewed configuration manifest; the telemetry stream carries
only the typed version and fixed snapshot fields. The module does not claim
that its 64-entry runtime guard is a reconstruction of all historical configs.

## Focused verification

```text
python3 tests/async/test_telemetry_config.py
```

The test compiles `telemetry_config.c` and its C++20 harness with `-Wall
-Wextra -Werror -lcrypto`, then runs both a normal and ASan/UBSan harness. It
exercises default/live capture, the default-on/disabled rested gate identities,
full property-digest/catalog refusal, the known 32-bit collision values
(`0x3f00908c` and `0x3f01c902`), the golden
encoding, redaction/allowlisting, changed identities, A→B→A canonical replay,
same-revision conflicts, sink failure/retry, stale ordering, bounded FIFO
pending state, repeated disabled results and reload invalidation. It does not
start a server, connect to SQL, modify properties, run migrations or access
production/shared runtime state. `telemetry_config_review.cc` adds equal-effective
default/live digest, zero-snapshot capture failure, pending revision conflict,
restart row seeding, and public-owner reload regressions.

`python3 tests/async/test_issue263_property_reload.py` separately compiles the real
`properties.c` command/parser together with this module in both supported modes.
It exercises initialize/set/revert/save/reload, permissions and malformed commands,
post-cache notification, native fallback parity, and numeric overflow rejection.
Unrelated game services are stubs; this is not a socket-session or production test.
Use a build environment with the SQL development headers for the SQL variant.

F retains central build/startup/pulse registration and actual catalog loading.
All injected callbacks are game-thread-only and must not reenter/mutate the owner.
A false sink result means definite non-admission. Reload intentionally abandons
unadmitted pending attempts; the shared key owner must handle reservation
cancellation without reusing an allocated replay key for a different payload.
An accepted FIFO drain may concern an older config: dependent capture uses the
public current-snapshot accessor, never the input or result payload alone.
