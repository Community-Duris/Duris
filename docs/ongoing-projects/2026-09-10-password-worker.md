# Interactive password work off the game loop

Issue #208 traced account-creation command overruns to cost-12 bcrypt on the game
thread. All interactive hash and verify calls now use the existing bounded login
worker. Its capacity remains 16 outstanding jobs (queued, running, or unconsumed),
and bcrypt remains at cost 12.

## Covered paths

- Telnet account creation, password confirmation/change, deletion authentication,
  and recovery password/confirmation.
- WebSocket registration, password change (verify then replacement hash in one
  job), and reset completion.
- Private-chest creation/password changes and non-owner opens, including legacy
  SHA-256 verification and bcrypt upgrades.

`password_async` owns continuations and session snapshots on the game thread.
Only password/hash copies cross into the worker. Descriptor teardown cancels its
handle without waiting for bcrypt. Before applying a result, the adapter checks
connection state, account identity/name/password/email, character identity, room,
and (while playing) whether the character is alive. Telnet type-ahead stays queued;
WebSocket dispatch cannot overtake a pending operation. A full queue returns a
retry response and never falls back to synchronous hashing.

Chest continuations reacquire the locker and check its identity, chest identity,
and ownership where required. A non-owner open re-reads the credential; legacy
upgrades use a conditional update and fail closed on a racing password change.
The SQL write APIs accept precomputed bcrypt hashes and reject plaintext.
Registration rechecks name/email availability after hashing. Reset completion
still checks the code and credential fingerprint when applying the result, and
retained WebSocket reset codes are cleansed when the continuation is destroyed.

## Validation

- Full MariaDB development server: `make -C src -j8` with the maintained warning
  profile and warnings treated as errors.
- `test_password_async_runtime.py`: ASan/UBSan, real worker and descriptor adapter,
  actual account-creation/confirmation handlers (with UI/persistence stubs),
  mismatch/retry and password-change save, verification/replacement, legacy chest
  upgrade, and state/credential/disconnect/room/death cancellation. The #191
  recorder asserts no `COMMAND OP SLOW` for the measured submissions/polls.
- `test_private_chest_password_hardening.py`: salts, cost, verification, bounded
  queue, cancellation, stale-hash rejection, and SQL/interactive-path contracts.
- Existing login-crash, command-latency, account-deletion, account-recovery source
  contracts, and account-recovery core runtime harness.
- Repository formatting check and `git diff --check`.

These are isolated build/harness checks. Live MariaDB chest journeys and complete
WebSocket wire journeys were not run. The latency harness does not measure SQL,
account-file writes, or a production tick; those operations remain on the game
thread and are outside this bcrypt change.
