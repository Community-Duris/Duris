# Durable outage and loss evidence

The SQL telemetry worker persists a producer registration before it qualifies
the repository or allows queue admission. It samples bounded counters about once
per second and attempts a final observation before publishing worker completion.
Gameplay capture performs no journal I/O, allocation or wait. The coherent sample
has four bounded attempts; a busy producer can defer a sample rather than delay
gameplay or publish torn queue/counter totals.

This satisfies the durable evidence requirement recovered from #566. It does not
spool telemetry payloads, restore lost gameplay facts, or establish historical
incident ends. The reviewed incident registry and report coverage integration
are tracked separately as #567 in [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

## Local setup

When SQL telemetry is enabled, set `TELEMETRY_OUTAGE_LEDGER_DIR` to an existing,
absolute directory owned by the server's effective user with mode `0700`.
Provision it alongside the other local journals before enabling capture:

```sh
install -d -m 0700 /your/private/local/runtime/telemetry-outages
export TELEMETRY_OUTAGE_LEDGER_DIR=/your/private/local/runtime/telemetry-outages
```

This path alone does not enable telemetry. Dedicated SQL writer credentials,
schema compatibility and the reviewed property catalog still apply; see
[DATABASE.md](DATABASE.md) and [CONFIG_CONTEXT.md](CONFIG_CONTEXT.md).
The production repository binding requires this directory. Explicitly injected
component-test repositories may omit it; the durable runtime tests provide real
protected directories and exercise the actual journal implementation.

The directory and files are owner-only; leaf symlinks, hard links, unexpected
file permissions and a competing owner are refused. The worker rechecks directory
permissions, the ownership inode and existing ledger protection before mutation.
Descriptors close across exec. The journal contains numeric producer/scope IDs,
timestamps, record-family masks and bounded health counters. It contains no
character/account names, credentials, SQL text, commands or record payloads.

## Interpreting observations

| Phase | Evidence and limits |
| --- | --- |
| `running` | A durable registration or periodic sample. After an unfinished lifetime, the next producer converts this to `unknown_tail` while retaining the original observation time and counters. |
| `clean_drained` | All admitted records were acknowledged, no invalid/conflicting/quarantined record remains, and the queue/inflight batch is empty. This does **not** erase admission rejections, session gaps or earlier outage counters. |
| `abandoned` | The worker terminated with a known count of unattempted queued records and no unresolved inflight batch. Preserve that count and the final sampled boundary. |
| `unknown_tail` | An unfinished lifetime, unresolved inflight commit, or a non-durable tail. The last sample is a watermark, not an exact crash/end time. Do not infer later admissions, an exact loss count, or an inclusive missing sequence span. |

Admission rejection counters count failed admissions; a value may have been
retried. They are not a count of distinct permanently lost gameplay facts.
Counters saturate, so an extreme saturated value is a lower bound. Record sequence
numbers may have gaps from failed allocation/admission and do not measure duration.
Inflight outcomes remain unresolved even when a commit acknowledgement was lost.
No trustworthy final clock/sample leaves a running watermark instead of inventing
a terminal boundary. UTC unknown is exported as `null`.

## Bounded publication and recovery

Wire version 4 (`DMSTLJ04`) stores at most 256 producer lifetimes in an
81,984-byte ledger and recognizes record families 1–12, including shared battles,
disjoint contributions and selected build observations. Readers retain version 1
(`DMSTLJ01`, families 1–9), version 2 (`DMSTLJ02`, families 1–10) and version 3
(`DMSTLJ03`, families 1–11). Both observed and last-failure masks
enforce each version's original family limit, even with a valid checksum. The frame
size, word ordering, checksum and publication bounds are unchanged. An offline
read preserves the original bytes and reports their actual version; a worker's
next valid publication atomically writes v4 while retaining the prior observations.
`outages.owner` is the stable exclusive lock file. Each publication writes an
owner-only `outages.pending` file, synchronizes it, renames it to `outages.ledger`,
and synchronizes the directory. A SHA-256 checksum detects damaged frames;
protected storage provides access control, not the checksum.

A complete pending publication is recovered only when its generation extends
the existing chain, preserves producer identities/scopes, advances cumulative
watermarks, and retains terminal history. Registration may only append an empty
new producer and convert the prior running phase to unknown without extending its
time. Recovery synchronizes the pending file before publication and is idempotent.
After any uncertain publication, the in-memory writer is poisoned until a fresh
open validates the on-disk chain; it cannot retry over the same generation.

Corrupt or truncated pending evidence is retained and capture is refused. Missing,
unsafe, disk-full, conflicting or exhausted storage opens a payload-free telemetry
circuit and refuses further admissions. Normal gameplay startup remains available.
The worker does not automatically delete evidence, evict producers or reconstruct
missing facts. Fixing a storage fault does not resume that producer; use a fresh
runtime after inspecting and preserving evidence.

During normal publication the two frame files occupy at most 163,968 bytes,
plus filesystem metadata and the empty owner file. At the 256-lifetime limit,
stop the telemetry worker, inspect/export the ledger, and retain the entire old
directory in protected archival storage before provisioning a fresh empty one.
Never remove or replace `outages.owner` while a worker is active. Archival evidence
must remain available for incident registration and study provenance; deleting it
is not a recovery procedure.

## Offline evidence export

Stop the telemetry worker before using the POSIX offline reader:

```sh
python3 scripts/telemetry/outage.py /your/private/local/runtime/telemetry-outages
```

The reader takes the same exclusive ownership lock, validates the bounded frame
and protection, and writes sanitized JSON to standard output. It creates no files
and performs no repair. Exit 2 reports refused input/storage with a stable reason
and, for I/O errors, a numeric code. `pending_publication` requires the worker's
validated recovery path or evidence review; the reader will not delete/repair it.
Both unknown-tail end fields are `null`. The separate last-observed timestamps,
queue decomposition and acknowledged/rejected counters retain their actual
meaning. Store exported packets privately, not in the repository.

The reader supplies evidence for a reviewed incident inventory; it does not
automatically register a report gap. See [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md)
for the qualified review/publication path, including independent schemas v3 for
battle association loss, v4 for contribution loss and v5 for selected build loss.
Its evidence establishes a sampled outage watermark. A missing
battle cannot acquire an invented duration or outcome through that review.

## Reproducible local qualification

From a Linux checkout, WSL or a disposable local container with the normal C++20
and OpenSSL development dependencies:

```sh
python3 tests/async/test_telemetry_outage.py
python3 tests/async/test_telemetry_runtime_outage.py
```

The native journal tests use synthetic protected temporary directories and cover
clean/abandoned observations, monotonic watermarks and immutable scopes, producer
reuse, simultaneous process ownership, unsafe/symlink/hard-link storage,
checksum/semantic/truncation/size corruption, ENOSPC write and fsync faults,
interrupted rename/publication, idempotent recovery, real SIGKILL, real exec,
historical-chain refusal, storage changes and the full producer quota. The
offline reader consumes a real native frame and is checked for unchanged bytes,
null unknown ends, active-owner refusal, retained interrupted evidence, original
v1/v2/v3 family limits, v4 build masks and history-preserving atomic upgrade.

The runtime journey runs SQL-header and client-free variants with bounded
synthetic repository faults. It verifies registration before SQL initialization
and admission, clean drain/restart, transient SQL recovery, unresolved commit
shutdown, disk-full startup/checkpoints and worker-only I/O. Existing transport
stress verifies coherent queue/counter samples while gameplay admission is active.
These tests require no production, staging or personal database credentials.
