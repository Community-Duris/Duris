# Output profiles and reusable recipes

Issue #281 provides the versioned configuration registry consumed by the
[completed-message renderer](OUTPUT_WORD_STYLING.md). It uses the existing cJSON
dependency. The numeric `duris.properties` table cannot represent dictionaries,
references, palettes, or typed recipe parameters, so these live in one JSON file.

Set `DURIS_OUTPUT_PROFILES_FILE` to an absolute path to that file. Startup reads it
once, after numeric properties and before gameplay. An absent/empty setting leaves
the registry unconfigured. A bad initial file produces one startup diagnostic and
Preserve fallback. Both MariaDB and flatfile builds support the same registry.

The [versioned sample](../examples/output-profiles-v1.json) demonstrates all six
recipe kinds, an explicit word dictionary, base/role colors, and channel defaults.
The long room-description caller opts in explicitly (#282).
[Chat](CHAT_COLORIZATION.md) and [world](WORLD_COLORIZATION.md) guides identify the
other adopted delivery boundaries. Unlisted callers retain Preserve. The complete [scenery sample](../examples/scenery-profiles-v1.json)
and [animation guide](SCENERY_COLORIZATION.md) describe that route. A configured
channel never causes the queue to classify or recolor messages.

## Schema and publication

The required root fields are `version`, `revision`, `recipes`, `dictionaries`,
`profiles`, and `channels`. Only schema `version: 1` is supported. `revision` is an
operator-supplied integer from 1 through 4,294,967,295 identifying the configuration.
Increment it when changing a file. Loading a previous valid revision is supported
for deliberate rollback; it is not a monotonic gameplay revision or replay key.

Every section is validated before a new immutable snapshot is published. Unknown
fields, duplicate fields, invalid types, unsupported versions, missing references,
invalid colors, and out-of-range parameters reject the entire candidate. Failed
initialization leaves no snapshot. Failed reload retains the last successful
snapshot and returns its revision with a component-specific diagnostic.

`OutputProfileRegistry::reload_file` performs a bounded file read and delegates to
the same parser as `reload_json`. Control code can use either explicit reload API.
Replace files atomically before requesting reload. Rendering and resolution never
read a file, a database, the environment, a clock, or gameplay RNG.

`output_profile_registry()` is the server's registry. Readers obtain immutable
shared snapshots; publication atomically replaces the current pointer. Resolved
`OutputContext` values retain their snapshot in `snapshot_owner`, keeping their
dictionary valid through copies, reloads, and destruction of a local registry.
Existing queued/paged/snooped output remains frozen display bytes. Reload cannot
recolor an already queued message. Caller-supplied span storage must still remain
alive for the duration of a send, as required by the original renderer API.

## Names, dictionaries, and palettes

Recipe, dictionary, and profile names normalize ASCII uppercase to lowercase and
use `[a-z][a-z0-9_.-]{0,47}`. References use that same normalization. Duplicate
normalized names fail validation, including `river` alongside `RIVER`.

A dictionary maps exact words to recipe names. Keys normalize ASCII case and may
contain letters, digits, underscores, and an apostrophe between word characters.
They have 1..64 bytes. Explicit inflections are separate entries: `water`, `waters`,
and `water's` are independent keys. Whitespace, non-ASCII keys, leading/trailing
apostrophes, substring patterns, and stemming rules are unsupported. Duplicate
normalized words are rejected. Matching uses the renderer's existing whole-token
and authored-style protection rules.

`output_palette_choices()` is the single catalog for validation and future command
parsing/hints. Its foreground names are `blue`, `green`, `cyan`, `red`, `magenta`,
`yellow`, `white`, `gray`, and the seven `bright_` variants from `bright_blue` through
`bright_white`. Names are case-insensitive. V1 accepts named foreground colors only;
numeric attributes, ANSI strings, black, backgrounds, blink, and invisible styles
are rejected. Omit an optional base/role color to inherit existing presentation.

## Recipes and profiles

A recipe has `kind` and `palette`, plus these optional bounded parameters:

| Field | Meaning | Default / valid range |
| --- | --- | --- |
| `kind` | Solid color or output-driven effect | `solid`, `flow`, `shimmer`, `flicker`, `pulse`, `glint`; required |
| `palette` | Ordered named foreground colors | Required; 1..16 entries |
| `stable_index` | Representative palette entry for Static and motion-off | 0; must index the palette |
| `step_every` | Eligible new outputs per phase step | 1; integer 1..1024 |
| `width` | Band/highlight width in visible characters | 1; integer 1..32, clamped to the word by the effect |
| `chance_percent` | Deterministic cosmetic-hash threshold | 20; integer 0..100 |

Solid recipes require exactly one foreground and reject motion parameters.
Flow, shimmer, flicker, pulse, and glint are implemented by the completed-message
renderer. Static and motion-off use the precomputed representative foreground;
Animated uses the supplied recipient/channel sequence. This registry does not
advance sequences. The sample dictionary remains intentionally small; the
[scenery guide](SCENERY_COLORIZATION.md) covers the full 64-word palette and exact
recipe semantics. Immutable snapshots cannot be copied or moved: share them via
the registry so borrowed recipe pointers remain tied to their owning snapshot.

A profile requires `policy` (`preserve`, `static`, or `animated`) and may specify
`dictionary`, `base`, and `roles`. Roles accept `sender` and `entity` foregrounds.
The resolver exposes those attributes for callers to place in explicit protected
spans. It does not infer which words are names or change recipient visibility.
Existing authored attributes and Authored spans continue to outrank added styles.

## Channels and recipient resolution

`output_channel_choices()` provides stable names and enum IDs. Existing IDs 0..4
retain their values; append new IDs without renumbering old ones. `Unspecified`
and the `Count` sentinel cannot be configured. The registered routes are:

- `room.description`, `room.title`, `room.inspect`, `room.exits`, `room.auras`,
  `room.occupants`, `items.list`.
- `chat.say`, `chat.tell`, `chat.whisper`, `chat.ask`, `chat.shout`, `chat.yell`,
  `chat.group`, `chat.guild`, `chat.alliance`, `chat.petition`, `chat.project`,
  `chat.page`, `chat.racewar`, `chat.immortal`, `chat.auction`, `chat.nchat`,
  `chat.jchat`, `chat.wizmsg`, and the existing `chat.generic` tag.
- `social`, `weather`, `combat.incoming`, `combat.outgoing`, `combat.observed`,
  the existing `combat.generic` tag, `prompt`, and `system.feedback`.

Routes map known channels to known profiles. Missing routes resolve to Preserve.
An unmapped channel cannot acquire a dictionary from a recipient preference.
Identifiers do not enable or restore gameplay channels, including retired ones.

Resolution is pure and proceeds in this order:

1. A caller's explicit Preserve is an absolute veto. Invalid caller modes also
   preserve. Passing Static or Animated authorizes profile resolution; those two
   values are not a ceiling on the server profile's chosen policy.
2. Find the server profile for the channel; absent snapshot/route means Preserve.
3. Apply that recipient's explicit Preserve/Static/Animated choice, or use the
   server policy for Default. An explicit recipient choice can override a server
   Preserve default when the caller has opted in and a profile exists.
4. If recipient motion is disabled, demote any resulting Animated policy to Static.
   Authored-style protection applies independently of all these choices.

`OutputProfilePreferences::reset(channel)` removes just that channel override.
It does not clear the global motion restriction or another channel/player's choice.
Preference persistence and player commands remain owned by issue #283.

```cpp
auto selected = resolve_output_profile(output_profile_registry().snapshot(),
    OutputChannel::RoomDescription, OutputPolicy::Static, recipient_preferences);
send_to_char(description, viewer, LOG_PUBLIC, selected.context);
// selected.context owns its dictionary through the complete send.
// For known entity/sender ranges, add caller-owned OutputStyleSpan entries using
// selected.entity_attr / selected.sender_attr before sending.
```

## Bounds, extensions, and validation

One file is at most 256 KiB with at most 12 JSON nesting levels, 128 recipes,
32 dictionaries, 64 profiles, 2,048 words per dictionary and 8,192 words total.
Named lookups and the representative word frames are constructed during loading.
Profiles share dictionary frames within a snapshot. No per-message dictionary
construction is required. Snapshots retained by callers intentionally retain their
own bounded configuration until the last reference is released.

Add channel/color choices to the public catalogs so config parsing and future
command hints agree. Unknown enum/string values must keep the conservative fallback.
New recipe fields or incompatible semantics require an explicit schema-version
decision; do not silently reinterpret existing V1 files or bypass strict validation.

```sh
python3 tests/async/test_output_profiles.py
SANITIZE=1 python3 tests/async/test_output_profiles.py
python3 tests/async/test_word_output_style.py
python3 tests/async/test_word_output_integration.py
make -C src -j4
make test-all TEST_JOBS=4
```

The production parser/resolver harness checks the catalogs, normalization and
duplicate rejection, invalid types/references/versions/ranges, capacity boundaries,
preference precedence and reset, protected styling, file failures, snapshot lifetime,
and coherent publication with concurrent readers. It uses disposable JSON files and
no game account or database.
