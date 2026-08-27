# DurisWeb Integration Review

Date: 2026-08-27

Status: Assessment only; no code changes made

Scope: The server-side surface that the (external, not-in-this-repo) DurisWeb
website consumes — the WebSocket listener, the JSON command/event protocol, the
GMCP data feed, the shared HMAC service authentication, and the HTTP health
probe. Covers integration shape, code and security quality, and an estimate of
the reverse-engineering effort needed to rebuild the website from this code
alone.

Files examined: `src/websocket.c` / `.h`, `src/ws_handlers.c` / `.h`,
`src/ws_auth.h`, `src/gmcp.c` / `.h`, `src/json_utils.c` / `.h`,
`src/mccp.c` (`write_to_descriptor`), `src/comm.c` (accept/greet/output loop),
`src/ansi.c`, `src/account.c`, `.env.example`, `docs/reference/api/health.md`,
`tests/async/test_websocket_*.py`, `tests/async/test_durisweb_secret_config.py`,
`tests/async/test_health_endpoint_contract.py`,
`tests/async/websocket_runtime_harness.cpp`.

## 1. How the integration is shaped

There is no HTTP API. The entire website integration rides on a single
non-TLS TCP listener (`DURIS_WEBSOCKET_PORT`, default `4050`, bound to
`LISTEN_ADDRESS`) that speaks three things:

1. **RFC 6455 WebSocket** for browser game clients (`src/websocket.c`).
2. **A one-shot HTTP `GET /health`** readiness probe on the same port, answered
   and closed before any upgrade (`websocket_send_health_response`,
   documented in `docs/reference/api/health.md`).
3. **The same WebSocket, authenticated as a service**, for the DurisWeb backend
   itself — a privileged peer that receives event fan-out and can issue admin
   commands.

Two distinct consumers therefore share one protocol, separated only by the
`durisweb_verified` / `durisweb_backend` flags on `struct descriptor_data`
(`src/structs.h:1796-1799`).

### 1.1 Transport and framing

`websocket_accept` marks the descriptor `websocket = 1` and parks it in
`CON_GET_TERM` until the HTTP upgrade completes (`src/comm.c:2925-2952`).
Frames are parsed in `websocket_parse_frame`; `permessage-deflate` is
negotiated with `server_no_context_takeover; client_no_context_takeover`.
Limits are explicit and enforced: 64 KB frames, 1 MB messages, 1 MB buffered
input, 4 MB output queue with a 4 KB control-frame reserve, 64 frames per read,
64 KB per flush. Ping every 30s, 60s timeout, 30s handshake timeout.

### 1.2 Inbound protocol (browser to server)

Messages are JSON objects dispatched by `ws_handle_command`
(`src/ws_handlers.c:3262`) on a `cmd` string with a `data` payload:

| Group | Commands |
|---|---|
| Account | `login`, `register`, `logout`, `account_info`, `change_email`, `change_password` |
| Character select | `enter`, `delete_character` |
| Chargen | `chargen_options`, `roll_stats`, `add_bonus`, `swap_stats`, `validate_name`, `get_hometowns`, `create_character` |
| Play | `game` (string or `{command}`) |
| Misc | `rested_bonus`, `poll_list`, `poll_view`, `poll_vote` |
| Service-only | `durisweb_auth`, `admin_delete_character`, `request_wholist` |

Anything unrecognized is treated as a raw game command and pushed onto the
descriptor input queue when `connected == CON_PLAYING`, so the web client is a
full MUD client, not a restricted menu.

### 1.3 Outbound protocol (server to browser)

Envelopes are all `{"type": ...}`:

- `auth` (`status`: `success` | `reconnected` | `failed`) with account name and
  character list
- `text` — `{"type":"text","category":"info","data":"..."}`
- `system` — `{status, message}`, including the `kicked` status
- `account`, `poll_list`, `poll_view`, `poll_vote`
- `gmcp` — `{"type":"gmcp","package":"Room.Info","data":{...}}`
  (`json_build_gmcp_message`, `src/json_utils.c`)

The GMCP package set is Mudlet/IRE-compatible and is the real state feed:
`Room.Info`, `Room.Map`, `Char.Vitals`, `Char.Status`, `Char.Affects`,
`Char.Skills`, `Char.Items`, `Combat.Update`, `Comm.Channel`, `Quest.Status`,
`Quest.Map`, `Group.Status`, `Ship.Contacts`, `Ship.Info`. Telnet clients get
the identical payloads inside `IAC SB GMCP` — one data model, two transports,
which is the strongest design decision in this integration.

### 1.4 Service channel (server to/from DurisWeb backend)

A backend authenticates with `{"cmd":"durisweb_auth","data":{"sig":"<64 hex>"}}`
or, over telnet GMCP, with `Core.Hello {"sig":...}`. Both paths share
`ws_verify_durisweb_signature` in `src/ws_auth.h`: HMAC-SHA256 of the current
UNIX minute (`time(NULL)/60`) keyed by `DURISWEB_SECRET`, accepted with a plus
or minus one minute skew window and compared with `CRYPTO_memcmp`.

Once verified, the descriptor receives a push feed: `auction_new`,
`auction_bid`, `auction_close`, `player_login`, `player_logout`,
`mud_shutdown`, `wholist`, plus `admin_delete_character` progress/response
messages. It can call `admin_delete_character` and `request_wholist`.

## 2. Code quality

**Good.** The protocol layer is well above the median for this codebase.

- `websocket_parse_frame` is genuinely RFC-conformant: reserved bits and
  opcodes rejected, control frames forced FIN and 125 bytes or fewer, unmasked
  client frames rejected, close codes validated, close reasons UTF-8-validated
  by a hand-written and correct `websocket_valid_utf8` (proper overlong,
  surrogate, and F4-range handling).
- Backpressure is designed, not accidental: separate output queue with a
  reserve for control frames, `WS_OUTPUT_QUEUE_FULL` (`-2`) distinguished from
  a real write error in `comm.c:3369` so a slow browser is throttled rather
  than dropped.
- Config parsing fails closed. `DURIS_WEBSOCKET_PORT` is `strtol`-validated
  with full `errno`/end-pointer checks; `DURISWEB_SECRET` has no default and
  an unset secret rejects every signature.
- All JSON is built with cJSON objects, never string concatenation. All
  inbound fields are type-checked (`cJSON_IsString`, `cJSON_IsNumber`,
  `cJSON_IsArray`) before use. Account and player lookups go through
  `sql_account_exists` / `sql_load_account` / `sql_player_exists`, so no
  attacker-controlled name reaches a filesystem path or a query string.
- `ws_cmd_poll_vote` is a model handler: poll existence, active state,
  duplicate-vote, multi-select, `max_choices`, and per-choice membership are
  all validated server-side before the write.
- `permessage-deflate` is negotiated with no-context-takeover in both
  directions, which sidesteps the memory-amplification failure mode.

**Weak.**

- `ws_handlers.c` is 3,374 lines with a 24-branch `strcmp` chain as its
  dispatcher, despite the file declaring a `ws_cmd_handler` function-pointer
  typedef in its own header that is never used. A table would remove the
  chain and make the command surface self-documenting.
- The rate-limiter is implemented twice, near-identically, in
  `ws_handlers.c:78-98` and `gmcp.c:72-92`. `ws_auth.h` already exists as the
  shared home for exactly this.
- `ws_handlers.c` carries roughly 20 `extern` declarations at file scope
  instead of including the relevant headers, and several handlers declare
  `extern void gmcp_*` inside function bodies (`ws_send_full_game_state`). This
  is fragile — a signature change elsewhere becomes a silent mismatch under C++
  linkage rather than a compile error.
- Broadcast functions are five near-copies of the same
  build-JSON / print / walk-descriptor-list / free block.
- The `text` category is dead. `json_utils.h` defines `TEXT_COMBAT`,
  `TEXT_MOVEMENT`, `TEXT_CHANNEL`, `TEXT_SYSTEM`, but the bulk output path in
  `write_to_descriptor` hardcodes `"category":"info"` for every line of game
  text. The website cannot route or style output by category.
- `websocket_send_json`'s `type` parameter is unused (`const char * /*type*/`).

**Tests.** Coverage exists and is unusually deliberate for this area:
`websocket_runtime_harness.cpp` exercises the handshake, frame, and
nonblocking-output paths against real socket pairs. However, the bulk of
`test_websocket_protocol_contract.py`, `test_durisweb_secret_config.py`, and
`test_health_endpoint_contract.py` are *source-text contracts* — they assert
that specific literals appear in the `.c` files. These lock in the shape of the
code and are cheap, but they do not prove behavior and they will pass against a
refactor that preserves the string while breaking the logic. There is no
end-to-end test of the JSON command protocol itself: no test drives `login`,
`enter`, `create_character`, or `durisweb_auth` against a running server.

## 3. Security

### 3.1 Done well

- Passwords: bcrypt via `bcrypt_hash_password` / `bcrypt_verify_password`,
  with legacy MD5 (`CRYPT2`) accepted only when `is_bcrypt_hash` says the
  stored hash is old — a correct migration shape.
- The service HMAC uses `CRYPTO_memcmp`, validates signature length and hex
  charset before any comparison, and fails closed on an unset secret.
- **Identity separation is enforced in both directions.** A verified service
  descriptor is refused by `ws_cmd_login` and `ws_cmd_register`; a descriptor
  that already has an account, a character, or is `CON_PLAYING` is refused by
  `ws_cmd_durisweb_auth` and by the GMCP `Core.Hello` path. This closes the
  obvious privilege-escalation route and was clearly thought about.
- Service auth is rate-limited: 5 failures per 60s window, then `CON_EXIT`.
- `X-Forwarded-For` is honored only when `getpeername` matches
  `DURIS_TRUSTED_PROXY_IP` exactly, and the extracted value must pass
  `inet_pton`. This is the correct pattern and is frequently gotten wrong.
- `admin_delete_character` and `request_wholist` both check
  `durisweb_verified` first.
- GMCP `Ship.Contacts` withholds `worldX`/`worldY` from unverified clients
  while still hashing them for change detection (`src/gmcp.c:440-460`) — a
  deliberate anti-cheat measure.

### 3.2 Findings

**H-1 — Site bans do not apply to web clients.** `bannedsite()` is called from
`greet()` (`src/comm.c:2964`), and `greet()` runs only from the `CON_SSLNEGO`
and `CON_TTYPE_NEGO` branches of the game loop (`comm.c:1222`, `comm.c:1237`).
WebSocket descriptors are put into `CON_GET_TERM` (`comm.c:2950`) and never
pass through either. A banned site reaches the game by connecting to the web
client instead of telnet. The new-character ban (`bannedsite(host, 1)`,
enforced at `account.c:2003` and `nanny.c:3260`) is likewise absent from
`ws_cmd_create_character`.

**H-2 — No throttle on `login` or `register`.** `ws_cmd_durisweb_auth` is
rate-limited; player login is not. A single WebSocket connection can issue
unlimited `login` attempts, and each one performs a bcrypt verification —
which is both a credential-stuffing channel and a CPU amplification vector
against a single-threaded game loop. `register` is equally unlimited and
unauthenticated: it creates accounts with `acct_confirmed = 1` and no email
verification ("skip email verification for web clients",
`ws_handlers.c:1240`), a minimum password length of 6, and no proof-of-work or
captcha. Account-table flooding is a single loop.

**M-1 — Account enumeration on login.** `ws_cmd_login` returns
`"Account not found"` before the password check and `"Invalid password"`
after it. Registration similarly distinguishes `"An account with that name
already exists"` from `"Email address is already in use"`, so both account
names and registered email addresses are directly enumerable.

**M-2 — No `Origin` validation on the handshake.** The handshake parser reads
`Upgrade`, `Connection`, `Sec-WebSocket-Key`, `Sec-WebSocket-Version`,
`User-Agent`, `Sec-WebSocket-Extensions`, and `X-Forwarded-For` — never
`Origin`. Any web page can open a cross-origin WebSocket to the port (browsers
do not apply the same-origin policy to WebSocket). Since session state lives on
the connection rather than in a cookie, this is not classic CSWSH, but it does
mean any site can drive the login/register endpoints from a visitor's browser
and IP, which compounds H-2 and M-1.

**M-3 — The listener is plaintext.** `websocket_init` creates a bare
`AF_INET6` socket; there is no TLS on the WebSocket path (the SSL session is
attached on `conn_type == 1`, the telnet path). Passwords in `login`,
`register`, and `change_password` cross this socket in cleartext JSON. This is
survivable only if a TLS-terminating reverse proxy is mandatory in front of
port 4050 — but nothing in the code enforces it, `DURIS_TRUSTED_PROXY_IP` is
optional, and `README.md` does not state the requirement. A misconfigured
deployment silently ships plaintext credentials.

**M-4 — Minute-granularity HMAC with a one-minute skew window and no nonce.** A
captured `sig` is replayable for up to roughly 3 minutes by anyone who observes
it, and the signature is over the timestamp alone — it binds no connection, no
request, and no direction. Combined with M-3, a passive observer on the
backend-to-server link gets full service privileges, including
`admin_delete_character`. The signature also grants blanket authority, so there
is no per-operation scoping.

**M-5 — The service feed carries player IP addresses.** `player_login` and the
`wholist` response include `ip` (`d->host`), account name, character, level,
race, class, faction, and client string. In `ws_send_wholist_to_client`, the
visibility filter is `if (d->character && !who_visible_to(...))` — the backend
descriptor has no character, so the guard is skipped entirely and wizinvis
staff are enumerated with their IPs. The website therefore holds a live PII
feed that the game's own visibility rules do not gate. If it is scoped for
admin-only display that is defensible, but the server side grants it
unconditionally to anything holding the shared secret.

**L-1 — A single shared secret with no rotation path.** One
`DURISWEB_SECRET` authenticates every backend; there is no key id, no second
accepted key, and therefore no zero-downtime rotation.

**L-2 — Verbose service logging.** `admin_delete_character` emits per-step
progress strings including account and character names, and `statuslog(56, ...)`
records login hosts. Fine operationally, worth knowing for log retention.

## 4. Reverse-engineering the website from this code

**Verdict: the server-facing contract is fully recoverable; the website is not.**
The integration code is a complete and unambiguous specification of the wire
protocol, and a functional web client can be rebuilt from it. What cannot be
recovered is everything the website is *besides* a MUD client.

### 4.1 What the code fully determines (no guessing)

- Every inbound command name, its exact JSON field names, types, and
  server-side validation rules — the dispatcher and each handler read as a
  spec.
- Every outbound envelope and the full field list of each payload.
- The complete GMCP package set and each package's JSON shape, since
  `json_build_room_info` and friends construct them field by field.
- The full authentication and session state machine: register, login,
  character list, `enter`, `CON_PLAYING`, plus the linkdead-reconnect path
  (`ws_cmd_login` re-attaches an online character and replays
  `Room.Info`/`Room.Map`/vitals/status/affects/quest, then queues a `look`),
  and the duplicate-session kick.
- The service contract: HMAC construction, the event set, and the two
  privileged commands.
- Protocol limits and timeouts, which the client must respect.
- Chargen data, which is server-driven: `chargen_options` returns races,
  classes, alignments, restriction notes, and hometowns, so the client renders
  whatever the server offers rather than hardcoding tables.

### 4.2 What must be rebuilt or inferred

- **An ANSI renderer.** Game text reaches the browser as JSON-escaped raw SGR
  sequences: `comm.c` runs `AnsiString::term()` (emitting real ESC-`[`-`m`
  sequences), and `json_escape_ansi_string` strips only the internal
  `&+R`-style codes, so the escapes survive and are encoded as `[...`.
  The client must parse and style them. Note the asymmetry: structured fields
  such as `chargen_options`'s per-race and per-class `ansi` strings carry the
  *raw `&`-code* form instead, so the site needs **two** color parsers.
- **Output routing.** Because `category` is always `"info"`, any per-channel
  pane, combat log, or chat split in the real site is client-side heuristics
  over text — unrecoverable from this repo.
- **The whole non-game website.** Forums, news, wiki, ladders, item database,
  donation/store, admin panels, account recovery, email delivery, session
  cookies, and the persistence behind them leave no trace here. The server
  emits auction and player events and answers `wholist`; how the site stores,
  renders, moderates, or aggregates any of it is invisible.
- **The backend's own architecture.** The code proves the site runs a
  persistent WebSocket/GMCP service peer holding `DURISWEB_SECRET`, consuming
  auction and presence events, and driving character deletion with a
  `requestId` correlation id — implying an async job UI. Everything else about
  that service is speculation.
- **UI and layout.** Zero signal. `Room.Map` and `Quest.Map` deliver ASCII map
  buffers and `Ship.Contacts` delivers bearings and ranges, which hints at a
  map pane and a nautical display, but the visual design is unrecoverable.

### 4.3 Effort estimate

| Deliverable | Basis | Rough effort |
|---|---|---|
| Protocol client library (connect, handshake, envelopes, GMCP types) | Fully specified by the code | 2-4 days |
| Playable web client (terminal pane, ANSI renderer, input, vitals, room, map) | Specified; ANSI plus layout is the work | 2-3 weeks |
| Account/chargen flow (register, login, character select, full chargen) | Fully specified, server-driven | 1-2 weeks |
| DurisWeb backend service peer (HMAC auth, event consumption, persistence, `admin_delete_character`) | Contract specified, architecture not | 1-2 weeks |
| The actual website (forums, news, DB, admin, store, auth-at-rest) | No signal in this repo | Not estimable; a greenfield build |

A competent developer with only this repository reaches a working, ugly,
feature-complete web *client* in roughly 4-6 weeks. Reconstructing the website
as it presumably exists is a separate greenfield project of unbounded scope,
because the repo contains no evidence of it.

### 4.4 Security consequence of that recoverability

The protocol's discoverability is itself a finding. Anyone with repo access —
it is a public-facing fork — can build a fully conformant client, which makes
H-1 (ban bypass) and H-2 (unthrottled login/register) directly exploitable
rather than theoretical. The *service* channel is protected only by
`DURISWEB_SECRET`; the algorithm is public, so the entire admin surface rests
on that one environment variable staying secret and on TLS existing in front of
the port (M-3, M-4).

## 5. Recommended order of work

1. **H-1** — call `bannedsite()` on the WebSocket accept path, and add the
   new-character ban check to `ws_cmd_create_character`. Smallest change,
   closes a live bypass.
2. **H-2** — reuse the existing failure-window limiter (lifted into
   `ws_auth.h`) for `login` and `register`, keyed per descriptor and per IP.
3. **M-3** — document the TLS-terminating-proxy requirement in `README.md` and
   refuse to start when the listener is non-loopback with no trusted proxy
   configured.
4. **M-1** — return one generic `"Invalid account or password"` from `login`.
5. **M-5** — decide deliberately whether the site should receive player IPs and
   wizinvis characters; if yes, gate it behind an explicit scope rather than
   bare `durisweb_verified`.
6. **M-4** — bind the service signature to a nonce or to the connection, and
   support a second accepted secret for rotation (**L-1**).
7. **M-2** — add optional `Origin` allow-listing via an env var.
8. Quality: convert the dispatcher to a table using the already-declared
   `ws_cmd_handler` typedef, de-duplicate the limiter and the broadcast
   helpers, replace the `extern` block with header includes, and either wire
   up the `text` categories or delete them.
9. Tests: add an end-to-end protocol test that drives `login`, `enter`, and
   `durisweb_auth` against a running server, to complement the source-text
   contracts.

## Validation

This is a read-only review. Nothing was built, run, or modified. The findings
are from source reading; H-1, M-2, M-3, and M-5 were each traced to their
enforcement (or absence) at the call site and are stated with the file and line
that supports them. They have not been confirmed by running the server.
