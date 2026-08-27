# Phase 02 Crash and Replay Matrix

This matrix is the required failure behavior for every critical gameplay domain. A
release gate must keep the focused coordinator, repository, outbox, copyover, shutdown,
and domain MySQL tests green.

| Boundary | Injected failure | Required result |
|---|---|---|
| Admission | queue full / coordinator unavailable | reject before gameplay success; no custody transition |
| Journal append | allocation, write, fsync, quota, or disk failure | fail closed; operation is not worker-eligible |
| Journal replay | truncated, corrupt, unsupported, or duplicate frame | quarantine/fail closed; duplicate ID attaches once |
| DB acquire/begin | outage, timeout, pool close | retry within bounds; keep journal and entity fences |
| DB apply | deadlock or lock timeout | rollback and bounded retry with the same operation ID |
| DB commit | connection loss / ambiguous result | reconcile inbox by operation ID; never resubmit with a new ID |
| Completion | stale, malformed, offline player, reconnect | exact ID match; retain authoritative result until safe publication |
| Outbox | destination outage, duplicate delivery, worker restart | retry the same operation/event index; destination dedupes |
| Checkpoint | crash before/after journal compaction | replay committed ID as `already_applied`; no repeated mutation |
| Copyover | admission quiesce or drain timeout | refuse unsafe handoff; retain unacknowledged journal records |
| Shutdown | worker stall or DB outage | bounded drain; preserve typed journal and do not report success early |
| Legacy fallback | item/scalar/large record present | hash/count and quarantine only; never execute SQL text |

The 25/50/100/200-client capacity contract keeps one operation per simulated client
below the 1,024-operation coordinator cap and all command payloads below the retained
64 MiB cap. Domain fan-out bounds remain 15 participants/artifacts and 32 boon results.
Outage tests must observe bounded overload/retry counters rather than unbounded memory,
threads, connections, or log payloads.
