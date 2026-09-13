# Terminal and structured chat colorization

The supported structured client is [Community-Duris/DurisWebApp](https://github.com/Community-Duris/DurisWebApp).
Its implementation is [DurisWebApp PR #44](https://github.com/Community-Duris/DurisWebApp/pull/44), paired with
[server #288](https://github.com/Community-Duris/Duris/issues/288). The
[release report](COLORIZATION_RELEASE.md) records integration tests and actual-client screenshots.

## Delivery and compatibility

Say, tell/reply and player guild chat attach owned `OutputChatMessage` metadata to their existing recipient
delivery. `send_to_char` sends `Comm.Channel` only after admitting that terminal message; `act` keeps its
existing ignore, hearing, position and visibility gates. The body is the result of the existing single
`language_CRYPT` call, never the original hidden text. Sender names use the same visibility decision.
Echo-off sends only the original acknowledgement to the sender and no duplicate structured echo. Self-tells
produce one structured event. Automatic guild announcements and other legacy GMCP channels remain unchanged.

Existing `channel`, `sender`, `text` and numeric Unix `timestamp` fields retain their types. `text` contains
the recipient-approved body with its existing authored markup. The server never injects terminal escapes
into that field to implement profile choices. Old clients can ignore the additional `presentation` field.

The optional snapshot is built from the **already frozen terminal markup**, after profile resolution and
bounded fallback. It does not resolve preferences again, call gameplay RNG, advance a sequence or run a timer.
It describes the complete displayed line, including its known template and protected sender/entity names.
A client must replace its usual sender/body rendering with this complete line once, not append a second copy.
The surrounding timestamp/channel label may remain. Default `preserve` uses the client's existing layout.

## Version 1 wire contract

```json
{
  "channel": "say", "sender": "Alice", "text": "hello", "timestamp": 1,
  "presentation": {
    "version": 1, "channelId": 11, "channel": "chat.say", "policy": "static",
    "text": "Alice says 'hello'", "underline": false,
    "runs": [[0, 5, 0, 0], [5, 18, 27, 0]]
  }
}
```

Offsets are half-open **Unicode code point** positions, not UTF-8 byte or UTF-16 code-unit offsets. Runs
are contiguous, nonempty, ordered and cover all `presentation.text`. Each tuple is `[start, end, foreground,
background]`. CR is normalized by the existing ANSI parser; final line endings are omitted. Interior line
breaks and visible content remain. Adjacent dollar signs collapse exactly as in the terminal's
final `delete_doubledollar` pass, except across color transitions that separate their terminal bytes.
`policy` is `preserve`, `static` or `animated`; even an animated packet is
one frozen frame. There are no client animation clocks or repeated preference applications.

Canonical pairs are `say` / `chat.say` / **11**, `tell` / `chat.tell` / **12**, and `gcc` / `chat.guild` / **18**.
Reject mismatched pairs or unknown versions and use the established fields. Do not guess channels from words.

Colors are **0** (terminal default), **16–23** (black, blue, green, cyan, red, magenta, yellow, white), and
**24–31** (the same order, bright). IDs 1–15 are invalid. This is Duris BGR order, not ANSI SGR order. Default
foreground is distinct from white. Foreground brightness corresponds to terminal bold. Background brightness
is the historical blink/underline bit: its low three bits select the background; `underline` freezes the
recipient's underline toggle. The supported web client displays underline when requested and keeps blink
steady, as an accessibility choice. Authored foreground/background colors otherwise take precedence.

Snapshots allow at most 16,384 code points and 4,096 runs. A raw ESC sequence, an oversized frame, failure
to fit either production serializer, or too many runs omits the optional snapshot and retains the
compatible packet. The ordinary command input limit is
1,024 bytes, comfortably inside these bounds. Clients must validate types, bounds, coverage and numeric IDs
before rendering or using stored snapshots. Text must be escaped by the UI framework; run values cannot
supply HTML, CSS strings, URLs or event handlers. There are no style settings in this packet to persist.

## Client theme, history and reset

The Vue client maps each ID through `chatPalette.ts`, with named `--mud-*` CSS variables and explicit dark
palette defaults. A theme can replace values without changing protocol IDs. `--mud-background` and
`--mud-foreground` represent ID 0. Foreground black remains black rather than being silently renamed gray.
The palette uses light blue/magenta alternatives so base choices remain readable on a dark background.

The chat panel and floating tell/guild windows share `ChatPresentation.vue`. A message retains its snapshot
through local chat history reload, which validates the metadata again at render time. New reset messages
use legacy appearance; old messages keep their historical colors. Preferences belong to the server character,
so reconnect needs no client preference cache. One server packet creates one store event and one applicable
history entry, using the existing message IDs. Terminal text is not reinserted as another structured event.

## Verification

`test_word_output_integration.py` compiles the real say/tell/guild implementations, `act`, `send_to_char`,
profile resolution and production JSON serializer. It compares every snapshot run with the exact terminal
frame, with two independent recipient choices, default reset, a fresh descriptor, authored green text,
emoji, angle brackets, literal ampersands, deterministic language transformation, ignore, deafness and echo-off.
The same tests run under ASan/UBSan. `CHAT_PRESENTATION_FIXTURE=/path/to/file.json` exports synthetic packets
without timestamps for the client fixture corpus; it never reads player data.

The client tests consume those production packets in the actual Vue renderer and chat panel, test WebSocket
dispatch/reconnect, and exercise the window manager, persisted history and floating window. Malformed versions,
channels, offsets, colors, control characters and oversize input fall back safely. The
[release report](COLORIZATION_RELEASE.md) adds actual-client screenshots, a real two-character TCP
walkthrough and measured presentation overhead to these runtime assertions.

To roll back presentation, reset character choices (`toggle color reset all`) and disable configured profiles.
The terminal returns to authored output and new structured messages request Preserve. Older packets/history and
clients remain readable; removing this optional field also returns client rendering to its previous layout.
