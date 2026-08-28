# Donation event envelope

External donation notices are disabled unless both `REDIS=TRUE` and
`REDIS_DONATION_SUBSCRIBER=TRUE` are configured and `REDIS_DONATION_SECRET` contains at
least 32 bytes. The key must be shared only with the trusted payment-event publisher.

Publish a compact JSON object to `mud:nchat` with these exact fields:

| Field | Contract |
| --- | --- |
| `schema_version` | Integer `1`. |
| `event_id` | Unique 16-64 character identifier containing only letters, digits, `_`, or `-`. |
| `issued_at` | Integer Unix timestamp within 300 seconds of server time. |
| `amount_cents` | Integer from `1` through `100000000`; floating-point amounts are rejected. |
| `currency` | Three uppercase ASCII letters. |
| `is_public` | JSON boolean. |
| `character_name` | Optional string up to 32 bytes; required for public notices. |
| `message` | Optional string up to 256 bytes. |
| `signature` | Lower- or uppercase 64-character hex HMAC-SHA256. |

Names and messages must be printable ASCII and cannot contain `&`, which is the game's
color-control prefix.

Compute the signature over this UTF-8 byte sequence, with literal newline separators and
no trailing newline:

```text
v1
<event_id>
<issued_at>
<amount_cents>
<currency>
<1 when public, otherwise 0>
<character_name or empty>
<message or empty>
```

Use `REDIS_DONATION_SECRET` as the HMAC-SHA256 key and hex-encode the 32-byte digest.
The server compares signatures in constant time, rejects event IDs already seen by the
current process, and retains a bounded window of 256 IDs. The timestamp window limits
replay after restart; the publisher must always create a new stable event ID.

The subscriber worker owns all Redis connection, subscription, socket, validation, and
reconnect work. It retains at most 64 validated events and drops excess events with a
health counter. The simulation thread dequeues at most eight events per game pulse and
performs no Redis work. Reconnect delays are exponential and capped at 60 seconds. Pub/sub
remains at-most-once; if delivery guarantees become necessary, move this envelope to a
durable stream keyed by the same stable event ID.
