# Copyover runtime path and transport validation (#230)

`COPYOVER_STATE_FILE` selects the state file shared by capture, listener-header
inspection, recovery and cleanup. An unset/empty value retains `copyover.dat`
for native deployments. Docker sets `/var/lib/duris/copyover.dat`; the image
already provisions `/var/lib/duris` as UID/GID 10001 with mode 0700. The sibling
`.tmp` file uses the same directory and is renamed after all saves complete.

Failure notices now enter the descriptor output queue. The live output loop
handles Telnet, MCCP, TLS and WebSocket framing, rather than writing plaintext
into a compressed or encrypted stream. The existing worker resume guard and
pre-publication save ordering are retained.

## Reproduction and local checks

Build both backends with `make -C src` and
`make -C src PERSISTENCE_BACKEND=flatfile DMS_BINARY=/absolute/path/dms_flat`.

Run:

```sh
python3 tests/async/test_copyover_failure_runtime.py
python3 tests/async/test_copyover_save_guards.py
python3 tests/async/test_copyover_custody.py
python3 tests/async/test_telnet_output_runtime.py
python3 tests/async/run_copyover_runtime_journey.py /absolute/path/dms_flat
# Start the fixture as root; it drops the actual server to UID/GID 10001.
python3 tests/async/run_copyover_runtime_journey.py /absolute/path/dms_flat --nonroot
```

The live journey creates and saves a synthetic player, promotes its offline
snapshot to the required staff level, and restarts. Its configured state parent
is initially missing, so the real copyover command fails to open the temp file.
The same client then performs `look` and an acknowledged `save`, proving that
both transport and the save worker resumed. Creating the private state directory
allows a second attempt to publish, exec the staged binary, recover and remove
the state file. `look` and another acknowledged `save` run on the original socket.
MCCP is decoded with zlib; malformed plaintext in the stream fails the test.

All four local combinations (plain/MCCP, ordinary/non-root fixture) passed. Both
backend builds, the production failure-path test, existing save guards,
ASan/UBSan custody save/exec/recover test, actual Telnet/TLS/MCCP sender fault
tests and formatter passed. No production files, clients or credentials were
used.

## Remaining deployment validation

The original full-world Docker/TinTin post-success disconnect was not reproduced
by these current-server journeys. Do not attribute that separate symptom to an
unproven account or descriptor change. The tests use a minimal world in the
local build container and explicitly reproduce the reported UID/cwd ownership;
they do not boot the complete release image. The production failure helper is
tested with separate TLS/WebSocket descriptor queues, but full TLS and WebSocket
player sessions across failed/successful copyover remain unverified. Keep the PR
draft for that release-image/client matrix and retain #230 for the remaining
deployment symptom. CI status is not the blocker.
