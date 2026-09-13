# Scenery colors and output-driven animation

Issue #282 adds deterministic recipe frames and explicitly adopts the long room
description send in `src/cmd/actinf.c`. Titles, exits, maps, auras, occupants, brief
mode and visibility gates retain their existing behavior. The default registry is
unconfigured, so existing installations keep their existing appearance.

To opt in on a development server, set `DURIS_OUTPUT_PROFILES_FILE` to the absolute
path of [scenery-profiles-v1.json](../examples/scenery-profiles-v1.json) before boot.
The file configures only `room.description`. Choose `static` in its scenery
profile for a fixed palette, `animated` for new-output motion, or `preserve` to
bypass added styling. The general [profile schema](OUTPUT_PROFILES.md) still
validates the whole file before publication. No per-message configuration reads
are added. The earlier small `output-profiles-v1.json` remains a schema example;
this separate file is the complete scenery palette.

Per-character persistence and the self-guiding player commands are subsequent
deliverables #283 and #284. The resolver already supports a recipient's global
`motion_enabled = false`, which demotes Animated to Static. The room caller uses
server defaults until those preferences are connected; this change does not
expose a premature `toggle color` command. Setting this sample's policy to Static
provides an operator-controlled motion-free preview in the meantime.

## Exact palette

These 64 lowercase ASCII keys list every supported inflection. Matching is
case-insensitive, preserves original characters, and has no occurrence cap.
Possessives, substrings, stemming, and non-ASCII token guessing are not added.
Existing attributes on any part of a token protect the entire word, including
explicit white, backgrounds and partially colored words. Fully authored
paragraphs are preserved. See [the renderer contract](OUTPUT_WORD_STYLING.md).

| Family | Static foreground | Exact keys |
| --- | --- | --- |
| Vegetation | `&+g` | forest forests tree trees moss vine vines grass foliage jungle jungles fern ferns undergrowth shrub shrubs bush bushes |
| Water | `&+B` | water waters river rivers stream streams ocean sea lake waterfall waterfalls |
| Fire | `&+R` | fire fires flame flames lava magma embers |
| Ice | `&+C` | ice icy frost frozen |
| Snow | `&+W` | snow snowy |
| Blood | `&+r` | blood bloody bleeding |
| Remains | `&+W` | bone bones skull skulls skeleton skeletons |
| Magic | `&+m` | magic magical arcane rune runes portal portals |
| Toxic | `&+G` | poison poisonous venom venomous acid acidic |

## Recipes and phase ownership

| Recipe | Frame behavior | Sample settings |
| --- | --- | --- |
| Water flow | Circular blue word with a cyan / bright-cyan band moving forward one character per step | width 2, step every 1 eligible send |
| Ocean / sea / lake | Broader blue / bright-blue / cyan / bright-cyan wave | width 4, step every 2 |
| Foliage shimmer | Sparse green / bright-green patches drift through the word | width 1, step every 2, hash threshold 20% |
| Fire flicker | Red with deterministic bright-red / yellow patches | width 2, step every 1, threshold 30% |
| Ice glint | Cyan with a white glint sweeping across the word during selected cycles | width 1, step every 1, threshold 25% |
| Magic pulse | Whole-word magenta / bright-magenta pulse; palettes with more entries rise and fall | step every 2 |
| Solid | One fixed foreground for snow, blood, remains and toxic words | no motion parameters |

Widths clamp to the word, leaving a base-colored character for multi-character
flow/glint bands. One-character flow cycles its palette; one-character shimmer
samples successive phases. Static always uses `stable_index`, irrespective of
sequence. The chance parameter is a deterministic cosmetic-hash threshold, not a
probability draw from the game's random generator. Small individual words can
have more or fewer highlights than the threshold suggests.

`descriptor_data::output_sequences` holds one unsigned 64-bit counter per stable
channel ID, 280 bytes per connection with the current catalog. It is plain,
zero-initialized session storage; it is never copied into player snapshots or
saved. New connections and copyover allocations start at zero. Switched bodies
use the receiving descriptor, without accessing NPC player storage. The future
character-preference owner is independent of these decorative counters.

`send_to_char` borrows the receiving channel's sequence to render a candidate,
then increments it once if a frame containing an eligible non-solid, multi-color
recipe is accepted. A word's fixed FNV-1a seed and that sequence determine the
frame. Repeated occurrences (including differently cased words) share it within
the message. Room IDs, text position, elapsed time, other recipients and unrelated
channels never participate. Unsigned wrap is defined; no clock or gameplay RNG
is consulted. `step_every` divides the channel counter, so a solid-only, protected,
no-match, Preserve, Static, invalid or individually oversized send does not consume
a step. An accepted shimmer/glint phase may have no highlights and still consumes
one eligible send.

Frozen queue/pager/snoop output never rerenders or advances a counter. A completed
paged command can still fall back to its originals if aggregate expansion is
unsafe; that does not rewind counters for earlier accepted sends. This maintains
the existing whole-command visible-text guarantee without replaying animation.
The pure `render_output_message` function takes an explicit `context.sequence`
and returns an optional `animated_match` flag; it never mutates session state.
Both serializers and pager capacity must accept the entire result before that
flag is returned. Original messages and original privacy policies remain the
inputs to player logging.

## Repeatable source-corpus audit

Run from a Linux development checkout with the normal compiler/cJSON dependencies:

```sh
python3 scripts/audit_scenery_palette.py
```

The script reads each numeric room record's title and long-description tilde
fields in `areas/wld/*.wld`, excludes empty descriptions, and normalizes CRLF to
LF. It excludes and reports oversized/NUL-containing descriptions. Exit text and
extra-description fields are not sampled. It sends NUL-delimited descriptions to
the production ANSI parser and static word renderer, counting contiguous changed
token runs. Duplicate room records are retained for occurrence/record counts;
distinct-string counts use exact normalized description bytes, including markup
and whitespace. P95 uses the nearest-rank method among affected records.

At base commit `9e0bfac62`, this method reports 446 files, 58,509 nonempty records,
28,130 distinct description strings, **26,418 eligible occurrences in 11,266
records**, and 5,513 distinct affected strings. The affected-record median is 2
and p95 is 6. No oversized/NUL records were excluded. Occurrence and affected-record
counts match the issue's earlier audit. This script reports both exact and
stripped distinct-string counts; the stripped counts reproduce the issue's
28,111 distinct descriptions and 5,505 distinct affected descriptions. These are offline source
coverage estimates, not live room or traffic counts.

## Preview fixtures and validation

[scenery-transcript.txt](../examples/scenery-transcript.txt) and the
[alternate transcript](../examples/scenery-transcript-alternate.txt) record original,
Static, and four successive Animated frames for forest, river, volcanic, ice,
fully authored and partially styled prose. The test regenerates the transcript
from the production renderer and compares it byte for byte after checkout newline normalization. These are synthetic
content fixtures, with no player/account data. All original visible words remain
unchanged. The [preview script](../../scripts/preview_scenery_palette.py) emits
terminal ANSI for both a dark standard palette and an alternate palette:

```sh
python3 scripts/preview_scenery_palette.py
```

The alternate palette changes automatic water tones to cyan/blue and foliage to
cyan/bright-cyan while authored red/green remain protected. Both versions use the
same production registry and renderer; only the configuration palettes differ.
The script writes no configuration or world changes.

```sh
python3 tests/async/test_scenery_animation.py
python3 tests/async/test_word_output_integration.py
python3 tests/async/test_output_profiles.py
SANITIZE=1 python3 tests/async/test_scenery_animation.py
SANITIZE=1 python3 tests/async/test_word_output_integration.py
SANITIZE=1 python3 tests/async/test_output_profiles.py
make -C src -j8
./scripts/format.sh --check
```

The focused suites cover all 64 keys, exact boundaries, partially styled and
protected words, all six recipe kinds, consecutive identical output, repeated
words, newlines, short/long words, width clamping, phase wrap, motion-off, static
frames, frozen paging, original logging, connection/channel isolation, retained
snapshot/recipe lifetime, invalid borrowed recipes, and expansion rejection.
