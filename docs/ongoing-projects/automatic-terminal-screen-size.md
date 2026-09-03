# Idea: automatic terminal screen size

Status: idea; not implemented

## Current behavior

Duris uses a 40-line default for new characters, missing persisted values, and
the `toggle screensize 0` reset. Existing characters keep their saved setting,
and players can still select a value from 12 through 48. Paging reserves four
rows for prompts and controls, so the 40-line default displays up to 36 content
lines per page.

The database schema's historical `DEFAULT 24` is not the runtime default. The
server initializes and explicitly writes `screen_length`, so changing that
sealed migration history is unnecessary. A separate data migration would be
needed only if we decide to overwrite existing players' saved preferences.

## Ideal future behavior

Negotiate the terminal's current dimensions with Telnet NAWS (Negotiate About
Window Size, option 31) and react to resize events. This replaces guesswork with
the actual number of rows while retaining 40 as a safe fallback.

Suggested precedence:

1. An explicit player preference, such as `screensize 35`.
2. A valid live NAWS height when the player selects `screensize auto`.
3. The 40-line fallback when the client does not support NAWS.

Persist the preference mode (`auto` or manual), not the last detected terminal
height. That lets a character follow different window sizes and devices without
silently losing a deliberate manual setting.

## Protocol and safety notes

- Send `IAC DO NAWS` without delaying the greeting or login flow. Negotiation is
  optional and must never introduce a timeout.
- Extend the existing Telnet parser to handle fragmented `WILL`, `WONT`, and
  `IAC SB NAWS width-high width-low height-high height-low IAC SE` sequences,
  including escaped `IAC` bytes.
- Accept resize updates throughout a session. Apply them on the normal network
  event loop so paging and the ANSI infobar cannot observe a partial update.
- Reject zero, incomplete, or absurd dimensions. Clamp usable height to a
  documented range before subtracting the pager's four reserved rows.
- Negotiate on plain Telnet and Telnet-over-TLS connections. Do not emit Telnet
  negotiation bytes on WebSocket sessions; the web client should report its
  viewport through its own metadata channel.
- Treat width as captured metadata initially. Any automatic line wrapping based
  on width should be a separate change because output formatting has many legacy
  assumptions.

## Delivery outline

1. Add a descriptor-level NAWS state machine and unit tests for byte-by-byte,
   fragmented, escaped, refused, malformed, and repeated negotiation sequences.
2. Add `screensize auto` while preserving `screensize N` and the 40-line
   fallback. Store the selected mode with an additive, guarded migration.
3. Exercise paging and ANSI infobar positioning at minimum, normal, and large
   row counts, including mid-page resize events.
4. Run socket-level login tests on plain Telnet and TLS to prove the account
   prompt remains immediate when NAWS is accepted, refused, or ignored.
5. Add equivalent viewport reporting for the WebSocket client, then make auto
   mode the default for new characters after compatibility telemetry is clean.

Protocol reference: [RFC 1073: Telnet Window Size Option](https://datatracker.ietf.org/doc/html/rfc1073).
