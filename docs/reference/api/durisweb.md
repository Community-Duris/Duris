# DurisWeb service integration

DurisWeb connects to the WebSocket listener as a privileged service peer. The
public endpoint must be HTTPS/WSS terminated by a local reverse proxy; the game
server's production WebSocket listener is loopback-only.

## Authentication

1. Send `{"cmd":"durisweb_challenge","data":{}}`.
2. Read `{"type":"durisweb_challenge","nonce":"<64 hex>","expiresIn":30}`.
3. Compute the lowercase hex HMAC-SHA256 of `<unix-minute>:<nonce>` with
   `DURISWEB_SECRET`.
4. Within 30 seconds send
   `{"cmd":"durisweb_auth","data":{"sig":"<64 hex>"}}`.

The nonce is random, connection-bound, single-use, and invalidated after an
authentication attempt. The server accepts the adjacent minute on either side
for clock skew. GMCP peers request the same challenge with
`Core.Hello {"requestAuthChallenge":true}` and receive
`Core.AuthChallenge {"nonce":"...","expiresIn":30}` before sending a second
`Core.Hello` with `sig`.

For zero-downtime key rotation, deploy the new key as `DURISWEB_SECRET`, retain
the old key temporarily as `DURISWEB_SECRET_PREVIOUS`, switch the backend, then
remove the previous key. The backend signs with the current key first and makes
exactly one retry with the previous key after an authentication rejection; it
does not loop between credentials.

## Hook toggles

Every event and service command the MUD serves can be disabled individually by
an operator. Ids are shared with the DurisWeb repository and are defined there
at `backend/src/hooks/registry.ts`.

Eight ids are gated on the MUD side: `auction_new`, `auction_bid`,
`auction_close`, `player_presence`, `mud_shutdown`, `wholist`,
`admin_delete_character`, and `donation_delivery`.

Each maps to a `durisweb.hook.<id>` key in `lib/duris.properties`. Values are
floats; anything `>= 0.5` counts as enabled, and a missing key defaults to
enabled so an older properties file cannot disable a live integration. Change
one at runtime with `properties set durisweb.hook.<id> 0.000` -- no restart.

A disabled hook emits nothing at the source. Broadcasts return before building
a payload; `admin_delete_character` returns an explicit error to the caller,
since a request path has someone waiting on a response; `donation_delivery`
drains its queue and drops, logging one line per pulse, so events cannot
accumulate and flood on re-enable.

`connection_log` is deliberately **not** gated here. The lines DurisWeb parses
out of `logs/log/comm` are ordinary `LOG_COMM` operational logs the MUD writes
for its own purposes, so suppressing them would remove admin-facing records to
control a web integration. That toggle lives on the DurisWeb side and stops
ingestion, not logging.

Request current state with:

```json
{"cmd":"durisweb_hook_state","data":{}}
```

The response, also pushed unsolicited whenever a `durisweb.hook.*` property
changes via `properties set` or `properties reload`:

```json
{
  "type": "hook_state",
  "schema_version": 1,
  "hooks": {
    "auction_new": {"enabled": true},
    "auction_bid": {"enabled": true},
    "auction_close": {"enabled": true},
    "player_presence": {"enabled": true},
    "mud_shutdown": {"enabled": true},
    "wholist": {"enabled": true},
    "admin_delete_character": {"enabled": true},
    "donation_delivery": {"enabled": true}
  }
}
```

`durisweb_hook_state` requires an authenticated service connection. An
unauthenticated descriptor sending it is closed, as with other service commands.

Set one MUD-owned hook with the authenticated service command:

```json
{
  "cmd": "durisweb_hook_set",
  "data": {
    "requestId": "durisweb_hook_set_42_1788264000000",
    "hook": "auction_new",
    "enabled": false,
    "actor": "operator-account"
  }
}
```

`requestId` must be a non-empty string of at most 128 bytes, `hook` must be one
of the exact eight MUD-gated ids above, and `enabled` must be a JSON boolean.
The current backend adds `requestId` and supplies the authenticated website
actor; the MUD never treats `actor` as authorization.

The MUD updates the game-thread property, rewrites `lib/duris.properties`
through `lib/duris.properties.new` plus rename, applies the property, and pushes
the complete `hook_state` frame. It then acknowledges the request:

```json
{
  "type": "durisweb_hook_set",
  "success": true,
  "requestId": "durisweb_hook_set_42_1788264000000",
  "hook": "auction_new",
  "enabled": false
}
```

On failure, `success` is false and `error` is a bounded operational message.
The pushed state frame may arrive before the acknowledgement; clients must
correlate the acknowledgement by `requestId` and confirm the observed state.
Unlike the in-game `properties set` command, this service command persists
automatically and does not require `properties save`.

## Authoritative auction removal

Administrative removal of an auction listing is a MUD-owned operation. The
website must not update `auctions` or insert pickup rows itself: the MUD's
critical command locks the auction, advances its revision, and stages every item
back to the seller in one transaction.

```json
{
  "cmd": "durisweb_auction_remove",
  "data": {
    "requestId": "durisweb_auction_remove_7_1788264000000",
    "auctionId": 1234
  }
}
```

`requestId` must be a non-empty string of at most 128 bytes and `auctionId` must
be an integral unsigned 32-bit number from 1 to 4294967295; a fractional value is
rejected rather than truncated. The MUD acknowledges submission:

```json
{
  "type": "durisweb_auction_remove",
  "success": true,
  "requestId": "durisweb_auction_remove_7_1788264000000",
  "auctionId": 1234
}
```

`success` reports that the command was **accepted**, not that it committed. The
committed outcome arrives on the existing auction event stream as a `removed`
event. Removal carries no actor wallet, so it runs through the same actor-less
background path as auction expiry, and repeating the request for an auction that
is no longer open is rejected by the repository. A retry is therefore safe.

Bidding and buy-now are deliberately **not** exposed here. Those commands lock
the bidder's live wallet through `expected_wallet_revision`, which only an
online character carries, so a website-originated bid would need an offline
wallet custody contract that does not exist yet.

## Authorization and data

An authenticated service receives auction, player-presence, shutdown, and
wholist events and may request `request_wholist` or
`admin_delete_character`. Player and service identities cannot share a
connection. Five failed service-auth attempts in 60 seconds close it.

WebSocket and Redis presence payloads omit account names, IP addresses, client
metadata, and invisible staff by default. Set
`DURISWEB_PRIVATE_PRESENCE=TRUE` only when the backend has an explicit
operational need and matching access and retention controls.

Redis presence is an expiring generation inside the active SQL season namespace. Read the
single active `season_epoch` from `season_reset_state` and keep that value fixed for the
complete read. A consumer must:

1. Read the opaque instance from `<REDIS_NAMESPACE>:season:<epoch>:presence:current`; a missing key means
   nobody is online.
2. Scan only `<REDIS_NAMESPACE>:season:<epoch>:presence:session:<instance>:*` and read the matching JSON values.
   Missing keys are expired/offline sessions and must be ignored.
3. Read `<REDIS_NAMESPACE>:season:<epoch>:presence:current` again after the scan. If it changed, discard the result and
   retry against the new instance.

Both the pointer and session keys have a 180-second TTL and are refreshed every 60
seconds by the game server's background worker. Never combine keys from different
instances, deployments, environments, or season epochs. The
`<REDIS_NAMESPACE>:season:<epoch>:player` pub/sub channel remains a
transition hint; the expiring key set is the current-state source. A season change requires
discarding all old keys and subscribing to the new channel.

Browser login is limited to five attempts per minute and registration to three
attempts per five minutes, both per connection and client address. Login uses a
generic credential failure, site bans apply to WebSocket connections, and
new-character bans apply to WebSocket character creation.
