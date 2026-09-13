# Recipient chat colors

The say, tell/reply, guild/gcc, shout, yell, whisper, ask and petition command
delivery paths use the recipient's own channel preferences. Sender echoes resolve
the sender's settings separately. Existing echo, ignore, guild membership, channel
toggle, silence, location, visibility and language gates stay at their original
delivery points. Logs retain their existing public/private/none classification.

`toggle color tell bright cyan` changes future incoming tells, replies and the
character's own tell echo. `toggle color tell default` restores the original
templates, including authored colors. Each channel is independent. Plain names
and other entity substitutions have explicit roles; authored names, language
markers, gradients and inline styles remain protected. Chat defaults do not
enable decorative animation or advance a room's animation sequence.

For messages assembled from known colored templates, `PlayerOutputMessage`
separates literal template fragments from authored body text and entity roles.
It retains the original template for log content and serializer/pager fallback.
Only known template wrappers are replaced; it never strips a player's message
to guess which styling came from the game. Other adopted `act` calls resolve
the profile after choosing each permitted recipient, and protect the actual
recipient-specific name/item/pronoun expansions.

This adoption covers the named player commands. Other channels such as group,
alliance, racewar, auction and immortal broadcasts retain their existing output
until explicitly adopted. The registry does not infer a channel from text or log
flags. A hard Preserve caller bypasses all added styling.

The terminal rendering is distinct from GMCP `Comm.Channel`. Structured client
presentation and consistent transformed message content are tracked by issue
#288; terminal ANSI alone does not complete separate web-pane support.

Validation uses the production send/act/queue/pager code and actual tell/reply
commands. It covers independently colored recipients and echoes, default/reset,
all adopted channel IDs, protected names/body text, unchanged private log content,
ignore and tell toggles, echo-off, language-call count, visibility/silence gates,
static channel isolation, paging, fallback and snooping:

```sh
python3 tests/async/test_word_output_integration.py
SANITIZE=1 python3 tests/async/test_word_output_integration.py
python3 tests/async/test_color_command.py
```
