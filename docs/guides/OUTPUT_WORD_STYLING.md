# Completed-message word styling

Issue #280 adds an opt-in rendering boundary. Existing callers still use Preserve.
No player preferences, server palette, animation sequence, prompt routing or GMCP
adoption is enabled by this change. [Profile loading and channel resolution](OUTPUT_PROFILES.md)
are provided by #281; [animated scenery and recipient/channel sequences](SCENERY_COLORIZATION.md)
are provided by #282.

## Calling the renderer

Include `net/output_style.h` and pass an explicit `OutputContext`:

```cpp
static const WordColorDictionary scenery = {
    { "water", ATTR_FG(25) }, // bright blue
    { "forest", ATTR_FG(18) } // green
};
OutputContext context{OutputChannel::RoomDescription, OutputPolicy::Static, &scenery};
send_to_char(description, viewer, context);
send_to_char_f(viewer, context, "%s flows through a %s.\r\n", "Water", "forest");
send_to_char(private_description, viewer, LOG_PRIVATE, context);
act("$n watches $T.", false, actor, nullptr, body, TO_ROOM, context);
```

The dictionary is borrowed, immutable during the call, and should be constructed
once by a caller/configuration snapshot. Keys are lowercase ASCII exact words;
values are foreground attributes (`0` or `ATTR_FG(16..31)`). Invalid matched
attributes are ignored. The profile loader rejects invalid/duplicate keys and
enforces palette policy before publication. A default
context and an explicit `OutputPolicy::Preserve` both bypass added styling, retaining
the original bytes entering legacy output handling. The integer log policy keeps
its existing meaning and overloads.

`Static` consumes the supplied fixed foreground dictionary. `Animated` additionally
uses optional immutable recipes and an explicit sequence to construct each word's
frame. Contexts without recipes retain the fixed dictionary behavior. The pure
renderer never advances a sequence, reads time, uses gameplay RNG, or redraws
output. `send_to_char` advances connection/channel state only for accepted eligible
animated sends; profile resolution demotes motion-off to Static. Channel IDs are
stable routing/preference identifiers, not automatic queue classifiers.

The pure `style_dictionary_words` function returns an `AnsiString` without modifying
its input or the dictionary. Its transformation is idempotent. The completed-message
`render_output_message` wrapper returns `false` for Preserve, no change, invalid
metadata, or unsafe expansion; callers then send the original message. No partial
result is accepted. Callers must use the wrapper, not the pure function's result
directly, when sending through the bounded legacy serializers.

## Exact word boundaries and protection

- ASCII letters, digits and underscore form tokens. ASCII matching folds A–Z to
  a–z while retaining each original character and case in the result.
- A straight apostrophe is part of a token only between two word characters.
  `water's` is distinct from `water`; quote marks around `'water'` are separators.
  Two consecutive apostrophes separate tokens. Punctuation and ASCII hyphens
  separate words. There is no stemming, substring matching or cross-send matching.
- Every non-ASCII code point attaches conservatively to the surrounding token,
  including combining marks, Unicode punctuation and curly apostrophes. Such a
  token cannot match an ASCII dictionary key. Thus `wateré`, `water’s`, and
  `water—forest` are all left alone; an ASCII `water-forest` has two eligible words.
  This intentionally favors protecting text over guessing Unicode segmentation.
- Any existing foreground or background attribute on any character protects the
  entire token from dictionary coloring. Explicit white is authored color. A reset
  restores default eligibility and does not mark following text as protected plain.
- Unknown words retain their attributes. Every eligible occurrence is processed;
  there is no match-count cutoff. Existing uniform and gradient `colorize` methods
  are unchanged.

## Span provenance and precedence

`OutputStyleSpan` addresses half-open **byte offsets in the final Duris-markup
message**, after formatting. The wrapper maps these once through the ANSI/UTF-8
parser to the visible-character offsets used by `AnsiStyleSpan` in the pure helper.
Include the complete intended characters in a span. A start inside a UTF-8 character
conservatively includes that character. Invalid bounds or styles bypass rendering.

`Authored` marks protected content, including deliberately plain text. `Sender`
and `Entity` prevent dictionary coloring and may supply their own foreground.
`ChannelBase` is eligible for word matches. No caller needs to insert a base ANSI
code and then erase it to make dictionary matching work.

Dictionary matches are selected against the original attributes and protected
spans first. Remaining default characters receive a role/base foreground. Existing
attributes always win; authored spans suppress all added styles on their own
characters, sender/entity roles outrank channel spans, and channel spans override
the message base. Among equal-priority overlapping spans, the later span wins.
Newlines never acquire base/role attributes. Protection intersecting one character
blocks a dictionary match for the whole word; it does not extend a role's own
foreground outside the supplied span.

The `act` overload performs existing visibility, audibility, altitude, recipient
substitution and capitalization first. It records entity/name/pronoun/item
substitutions as protected spans. `$T`/`$F` remain body text and `$$` keeps the
existing transport escape contract. Caller spans passed to `act` must address the
final recipient text, never template offsets. For layouts whose byte positions
differ by recipient, compose their context/spans at the per-recipient call site.
Recipient preference resolution remains the responsibility of later channel
adoption; this layer never changes another recipient's settings.

## Size, storage and replay

Messages and visible-character transformations are bounded below
`MAX_STRING_LENGTH` (65,536). A render accepts at most 65,536 spans. Matching uses
dictionary lookups; protected-span accounting is linear, and role spans use an
endpoint sweep bounded by `O((characters + spans) log(spans + 1))`. There is no file,
database or network I/O, runtime pointer storage, or global animation state.

Preflight models the legacy Duris and terminal serializers' **early truncation
thresholds**, UTF-8 widths, CRLF expansion, attribute transitions, literal ampersand
escaping and final resets. It reserves room for a snoop prefix on each line and
the largest terminal attribute mode. This is deliberately conservative near the
limit: styling may fall back even when a particular terminal rendering could fit.
The serialized markup must also round-trip to exactly the same characters and
attributes. Raw terminal escape sequences bypass added styling because their
attributes are not represented by the Duris parser.

The send boundary freezes markup before queue merging or pager accumulation.
Switched prefixes remain outside styling. Adjacent writes can still merge in the
queue, but cannot acquire a cross-call word match. Paging and snooping use frozen
bytes and do not apply the current dictionary again. Pager replay uses `LOG_NONE`:
original messages were already logged at accumulation with their original privacy
policy, so replay cannot relog decorated text or turn private/unlogged output into
public log records.

Each candidate must fit the remaining command-output capacity. Once a paged command
contains added styling, it retains a bounded copy of its original accumulated
content. If the added bytes would crowd out a later message that fit originally,
the entire command falls back to original content before paging starts, and added
styling stays off for the remainder of that command. This preserves the legacy
visible-text budget. Before replay begins, the completed command also checks each
actual pager page against queue, markup, terminal and snoop limits. Individually
safe sends can combine into an unsafe page, especially because the legacy pager
does not count literal ampersands toward its column limit. Such a command falls
back to its originals, including any existing accumulation warning, before any
page is emitted. When main-menu output or a changed paging preference bypasses
paging, validation budgets the entire command that replay actually sends. This
final check is linear in the bounded command size.
Original messages exceeding legacy limits still follow the
existing truncation/rejection path; this feature does not promise to recover
content that those paths already discarded.

## Validation

```sh
make -C src -j4
python3 tests/async/test_word_output_style.py
python3 tests/async/test_word_output_integration.py
SANITIZE=1 python3 tests/async/test_word_output_style.py
SANITIZE=1 python3 tests/async/test_word_output_integration.py
python3 tests/async/test_ansi_runtime.py
python3 tests/async/test_unicode_runtime.py
python3 tests/async/test_telnet_output_runtime.py
python3 tests/async/test_websocket_runtime.py
python3 tests/async/test_item_movement_prompt_runtime.py
python3 tests/async/test_pager_contract.py
./scripts/format.sh --check
```

The new suites execute the production renderer and verbatim extracted send, `act`,
queue, pager, capitalization and snoop functions with production types. External
world visibility, allocation and logging sinks are deterministic stubs; no live
gameplay or database is required. They cover mode independence, per-recipient
substitutions, authored spans, original logging/privacy, switched prefixes, frozen
pager refresh, accumulation fallback and serializer limits with UTF-8 and malformed
markup. Existing ANSI tests also cover unchanged uniform/gradient behavior.
