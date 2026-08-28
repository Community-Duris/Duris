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
remove the previous key.

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
