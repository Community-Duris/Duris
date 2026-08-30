# Health Endpoint

The WebSocket listener also accepts an unauthenticated HTTP readiness request:

```text
GET /health HTTP/1.1
```

A ready server returns HTTP 200 with:

```json
{"status":"healthy","persistence":"ready"}
```

The check reports readiness for the selected persistence authority. MariaDB mode reads
in-memory database-pool state without a blocking database query; `flatfile-primary` is
ready after its private authority has passed startup validation. The response does not
reveal the selected mode, configuration, target, credential, player, or account value.
Responses are non-cacheable and the connection closes after the response.

Run the checked-in probe with:

```bash
scripts/healthcheck.sh
```

The default URL is `http://127.0.0.1:4050/health`. Override it with
`DURIS_HEALTH_URL`; an isolated server can set `DURIS_WEBSOCKET_PORT` to match.
