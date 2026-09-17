# Terminal modes

Duris stores a numeric terminal type for each connection. The supported
selection codes are:

| Code | Mode | Behavior |
| --- | --- | --- |
| `1` | Generic | Plain text without ANSI color or structured terminal markup. |
| `2` | ANSI | ANSI color and the normal ANSI login presentation. |
| `3` | MSP presentation markup | Compatibility mode that frames prompts, maps, and group output for clients that understand those tags. |
| `9` | Quick ANSI | ANSI mode without the normal login greeting. |

TERM_MSP is retained as code `3` so existing characters and clients keep
their saved terminal preference. In this codebase it is a presentation mode,
not an audio implementation: it does not emit `!!SOUND(...)` or
`!!MUSIC(...)` triggers. Clients should not expect sound or music events from
selecting MSP presentation markup.

The markup currently covers these output areas:

- `<prompt>...</prompt>` around prompts;
- `<map>...</map>` and `<automap>...</automap>` around map output;
- `<group>...</group>` around group output.

In-game, terminal presentation can be selected with `TOGGLE terminal ansi`,
`TOGGLE terminal gen`, or `TOGGLE terminal msp`. The login terminal menu and
its help text use the same codes.
