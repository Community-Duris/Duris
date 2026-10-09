# Health Endpoint

The WebSocket listener also accepts an unauthenticated HTTP readiness request:

```text
GET /health HTTP/1.1
```

A ready server returns HTTP 200 with:

```json
{"status":"healthy","persistence":"ready"}
```

In MariaDB mode, an uninitialized shared SQL pool produces HTTP 503 Service
Unavailable with:

```json
{"status":"unhealthy","persistence":"unavailable"}
```

The check reports limited readiness for the selected persistence authority.
MariaDB mode checks only whether the shared SQL pool is initialized, without a
database round trip. HTTP 200 does not verify current database connectivity,
available pool connections, or a successful write. In `flatfile-primary`, startup
validates the private authority; the health handler performs no per-request
storage probe.

The response does not reveal the selected mode, configuration, target,
credential, player, or account value. Responses are non-cacheable and the
connection closes after the response.

Run the checked-in probe with:

```bash
scripts/healthcheck.sh
```

The probe selects its URL in this order:

1. A non-empty `DURIS_HEALTH_URL` in the process environment.
2. `DURIS_HEALTH_URL` from the repository's `.env`, if it is a regular,
   non-symlink file owned by the invoking user with owner-only read/write
   permissions (`0600` is the normal setting).
3. `http://127.0.0.1:4050/health` if neither source supplies a non-empty value.

Changing the server's `DURIS_WEBSOCKET_PORT` requires a matching
`DURIS_HEALTH_URL`; the probe does not derive its URL from that port variable.
The probe gives curl a three-second timeout, requires an HTTP-success response,
and accepts only the JSON object `{"status":"healthy","persistence":"ready"}`.
