# DurisWeb service integration

DurisWeb connects to the WebSocket listener as a privileged service peer. The
public endpoint must be HTTPS/WSS terminated by a local reverse proxy; the game
server's production WebSocket listener is loopback-only.

## Authentication

1. Send `{"type":"cmd","cmd":"durisweb_challenge","data":{}}`.
2. Read `{"type":"durisweb_challenge","nonce":"<64 hex>","expiresIn":30}`.
3. Compute the lowercase hex HMAC-SHA256 of `<unix-minute>:<nonce>` with
   `DURISWEB_SECRET`.
4. Within 30 seconds send
   `{"type":"cmd","cmd":"durisweb_auth","data":{"sig":"<64 hex>"}}`.

The nonce is random and connection-bound. Signature verification consumes it on
success or failure. Request a fresh challenge before retrying authentication,
including a retry signed with the previous key. The server accepts the adjacent
minute on either side for clock skew. GMCP peers request the same challenge with
`Core.Hello {"requestAuthChallenge":true}` and receive
`Core.AuthChallenge {"nonce":"...","expiresIn":30}` before sending a second
`Core.Hello` with `sig`.

For zero-downtime key rotation, deploy the new key as `DURISWEB_SECRET`, retain
the old key temporarily as `DURISWEB_SECRET_PREVIOUS`, switch the backend, then
remove the previous key. During rotation, the server checks the signature against
the current and optional previous keys using the same challenge. A usable current
key must be configured; the previous key is additional.

Clients should sign with the current key first and make at most one retry with
the previous key after an authentication rejection, using a fresh challenge.
Do not loop between credentials. See
[WebSocket and proxy settings](../../operations/CONFIGURATION.md#websocket-and-proxy-settings)
for key requirements.

## Hook toggles

Operators can disable the MUD-gated hooks listed below individually. Ids are
shared with the DurisWeb repository and are defined there at
`backend/src/hooks/registry.ts`.

Eight ids are gated on the MUD side: `auction_new`, `auction_bid`,
`auction_close`, `player_presence`, `mud_shutdown`, `wholist`,
`admin_delete_character`, and `donation_delivery`.

Each maps to a `durisweb.hook.<id>` key in `lib/duris.properties`. Values are
floats; anything `>= 0.5` counts as enabled, and a missing key defaults to
enabled so an older properties file cannot disable a live integration. Change
one at runtime with `properties set durisweb.hook.<id> 0.000` -- no restart.

Hook-management commands and `durisweb_auction_remove` require service
authentication but have no individual hook toggle.

A disabled hook emits nothing at the source. Broadcasts return before building
a payload; `admin_delete_character` returns an explicit error to the caller,
since a request path has someone waiting on a response; `donation_delivery`
discards up to eight queued notices per nominal one-second poll, logging one
summary only when it drops any. Changing the toggle does not immediately flush
the entire queue; notices can remain queued on re-enable. See the
[donation event envelope](donation-events.md) for the delivery contract.

`connection_log` is deliberately **not** gated here. The lines DurisWeb parses
out of `logs/log/comm` are ordinary `LOG_COMM` operational logs the MUD writes
for its own purposes, so suppressing them would remove admin-facing records to
control a web integration. That toggle lives on the DurisWeb side and stops
ingestion, not logging.

Request current state with:

```json
{"type":"cmd","cmd":"durisweb_hook_state","data":{}}
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

`durisweb_hook_state` requires an authenticated service connection and closes an
unauthorized connection. `durisweb_hook_set` and `durisweb_auction_remove` use the
same close behavior. An unauthorized `request_wholist` sends an authorization
error and returns.

Set one MUD-owned hook with the authenticated service command:

```json
{
  "type": "cmd",
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
  "type": "cmd",
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

`success` reports that the command was **accepted**, not that it committed.
After commit, the auction publisher uses an `auction_close` WebSocket message
with `data.reason` set to `"removed"` and the auction ID in `data.id`. This
broadcast carries no `requestId`; it is separate from the admission
acknowledgement.

Receiving the notification depends on the `auction_close` hook being enabled
and a live WebSocket service connection. Disabling that hook suppresses the
notification while the removal command can still proceed. Failure to observe a
notification does not establish that removal failed.

Removal carries no actor wallet, so it runs through the same actor-less
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

1. Read the opaque instance from `<REDIS_NAMESPACE>:season:<epoch>:presence:current`; if the key is missing,
   no valid published presence generation is available.
2. Scan only `<REDIS_NAMESPACE>:season:<epoch>:presence:session:<instance>:*` and read the matching JSON values.
   Missing keys are expired/offline sessions and must be ignored.
3. Read `<REDIS_NAMESPACE>:season:<epoch>:presence:current` again after the scan. If it changed, discard the result and
   retry against the new instance.

The pointer and session keys use a 180-second TTL. The background worker's
intended renewal interval is 60 seconds while it owns the generation and has
active sessions; this is not a guaranteed refresh deadline. An unavailable worker,
Redis outages, or failed renewal can let keys expire while players remain
connected. Consumers must distinguish an unavailable feed from an empty, current
published list.

Never combine keys from different instances, deployments, environments, or
season epochs. The `<REDIS_NAMESPACE>:season:<epoch>:player` pub/sub channel remains a
transition hint; the expiring key set is the current-state source. A season change requires
discarding all old keys and subscribing to the new channel.

Browser login is limited to five attempts per minute and registration to three
attempts per five minutes, both per connection and client address. Login uses a
generic credential failure, site bans apply to WebSocket connections, and
new-character bans apply to WebSocket character creation.
