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

## PR #211 merge-blocker follow-up

Updated the branch with master `ca0822b8` and corrected both password/login
ordering assertions for `get_casting_cmd_from_q(t_ch, &point->input, comm)`.
The first-password completion now frees an existing account password only when
non-null. The runtime harness links `src/core/memory.c`, allocates password
strings through `__malloc`, and releases them through `FREE`; it no longer
substitutes libc `free` for the production allocator. A separate invocation
asserts that the production allocator rejects null with its expected fatal
message. Removing the new guard in a temporary harness reproduces the reported
account-creation failure; the corrected handler passes under ASan/UBSan.

The locker failure contract now follows asynchronous non-owner opens: failed
credential reads, absent credentials, worker submission, conditional credential
revalidation/upgrades, stale chest identity, and rejection before opening.

Follow-up validation passed:

- Full MariaDB development build: `make -C src -j8` (warnings as errors).
- `test_password_async_runtime.py`, including new account creation,
  mismatch/retry, confirmed-account password replacement, and allocator probe.
- `test_login_crash_regressions.py`, `test_locker_result_failures.py`, and
  `test_private_chest_password_hardening.py`.
- Both account-deletion contracts and `test_account_character_delete_runtime.py`.
- `test_account_recovery_contract.py` and `test_account_recovery.py`.
- `test_casting_input_gate_runtime.py` and `test_command_latency_runtime.py`.
- Repository changed-line formatting check in an isolated Linux Git snapshot,
  plus `git diff --check`.

No live Telnet, MariaDB chest, or full WebSocket journey was run for this
follow-up. The account-creation regression uses the actual account handlers,
worker, descriptor adapter, and allocator, with UI and persistence stubs.

## PR #211 registration-race review

Integrated master `7321a402e` before validating the review fix. When a password
request is invalidated, an originally empty-credential account is freed if it is
still the descriptor's original account. This prevents a competing same-name
save from turning a cancelled registration into an authenticated session.
Cancellation resets the descriptor to the account-name state and sends either
WebSocket auth-failed or a Telnet retry prompt with echo restored. A different
account attached by a replacement session is preserved. The existing WebSocket
registration uniqueness check remains intact.

The sanitizer harness reproduces the credential/email/confirmed-state mutation
performed by the same-name account refresh. Both protocols are tested with the
worker still outstanding and with its result already ready. It links the real
account allocation, list management, cleanup, and memory allocator routines;
only protocol output is captured by stubs. It also checks preservation of a
replacement account. Running the new harness against the previous adapter fails
at the assertion that the losing descriptor has no account; the fixed adapter
passes under ASan/UBSan.

Validation passed on the integrated source:

- Full MariaDB development `make -C src -j8`, with warnings as errors.
- Password async runtime, private-chest hardening, login-crash, and locker
  failure regressions.
- Account deletion and deletion-menu contracts, character-deletion runtime,
  account-recovery contracts and core runtime, casting input gate runtime, and
  command-latency runtime.
- Changed-line formatting in an isolated Linux Git snapshot and `git diff --check`.

The previous hosted aggregate run `34551551246` reported 446 passing tests and
six failing journeys. All six failed in `server_build_artifacts.toolchain_key`
when `shutil.which` returned no executable and was passed to `Path`; this is not
evidence of a completed live journey. The helper resolves successfully in the
isolated review container. A new hosted run is still required for the updated
head. The real-character MariaDB chest/password journey requested in the earlier
review has not been completed; focused harnesses do not replace that merge gate.
