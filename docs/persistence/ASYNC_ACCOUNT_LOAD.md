# Interactive account loading

Entering an account name, and native WebSocket login, submit the account read to
the bounded worker in `src/account/account_load.c`. The game thread allocates
and attaches the account, then polls a descriptor-owned continuation through
the existing authentication input gate. Password verification still uses the
existing password worker after the account read succeeds.

The worker borrows one validated SQL pool connection exclusively. It locks the
account row, runs the existing ownership/tombstone-aware projection repair, and
loads account scalars, IPs, and selectable characters in one repeatable-read
transaction. Repair writes are visible to the following reads. Any required
query or commit failure rolls back and discards the entire snapshot; uncertain
or dirty connections are retired rather than returned for reuse. Synchronous
non-login callers share the same repair implementation and retain their
existing loading paths.

Requests and results own standard-library storage. The worker never receives a
descriptor, account pointer, continuation, or plaintext WebSocket password.
Only the game thread allocates live account strings/list nodes or publishes the
snapshot. Publication validates the request ID, account name, descriptor socket,
account/character identity, login state, and saved credential context. Closing
a descriptor cancels its continuation before freeing account/session memory.
A superseding request cancels its predecessor. Cancelled in-flight work may
finish its bounded transaction and harmless projection repair, but its result
cannot reach a replacement session.

The worker admits at most 64 jobs, counting queued, executing, and unconsumed
results. Reads cap IPs at 4096, characters at 1024, and copied string payload at
1 MiB per snapshot. Requests have a 30-second monotonic deadline, checked before
each query and on game-thread publication. Pool acquisition and configured
client connect/read/write timeouts bound waits inside individual client calls.
An expired or failed read leaves telnet login flushed with its account freed;
WebSocket login frees the account and returns authentication failure. A truly
absent account still reaches telnet registration confirmation. File-only legacy
accounts retain the previous load-failure behavior instead of becoming new
registrations. Saturation or worker unavailability reports busy without a
synchronous fallback.

Shutdown first cancels descriptor continuations, rejects new submissions, drops
queued work, and joins the in-flight transaction before SQL pool teardown.
Cancellation does not interrupt a running MySQL client call; shutdown can wait
for that call's configured network timeout and rollback cleanup.

`python3 tests/async/test_account_load.py` executes the production worker,
session adapter, login handlers, and allocator under ASan/UBSan with injected
database behavior. It exercises delayed-query loop progress, descriptor/socket
reuse, superseded and changed contexts, repair/load failure cleanup, native
WebSocket password submission, queue saturation, owned snapshots, transaction
rollback, row/payload limits, connection retirement, and shutdown. The existing
account projection suite continues to exercise the shared repair queries and
their ownership, baseline, and tombstone predicates.
