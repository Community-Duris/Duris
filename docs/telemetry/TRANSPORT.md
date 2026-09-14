# Bounded telemetry transport (#262)

This module is the in-process handoff between the gameplay/runtime producer and
#261's repository. It is deliberately a transport, not a durable spool and not
a second repository. A successful enqueue means that a value was copied into
bounded RAM; it does not mean that SQL committed it.

## Ownership and lifecycle

- There is one fixed-capacity SPSC FIFO. The producer is the runtime/game side;
  the sequential consumer is the externally owned #265 worker.
- `telemetry_transport_init()` initializes bounded RAM only and never creates a
  thread or invokes repository callbacks. The first active worker pulse initializes
  the repository, including when no records are queued. If startup was cancelled
  by stop and no work is pending, the worker publishes stopped without opening an
  unnecessary connection. Failed initialization uses
  bounded backoff. The coordinator starts and joins the worker separately.
- `telemetry_transport_pulse()` and `telemetry_transport_drain_until()` are
  worker-only. `telemetry_transport_request_stop()` only publishes atomic stop
  state and returns; it does not call SQL or wait.
- `telemetry_transport_shutdown()` is legal after the worker has exited and been
  joined. It releases logical queue state and leaves a stopped cached health
  snapshot. It is not a cancellation mechanism for arbitrary blocking connector
  I/O. Repository deadline/cancellation and connector teardown remain the
  lifetime coordinator/repository responsibility; no transport thread is
  detached.
- `telemetry_transport_quiesce_for_tests()` and
  `telemetry_transport_resume_for_tests()` are private coordinator/test seams.
  Quiesce rejects new admissions without discarding already retained work.

A stop request wins over a later init, including an invalid configuration, until
the lifetime owner completes shutdown. An enqueue already inside its bounded producer operation is counted
until it returns, so shutdown cannot publish stopped while that producer may
still be writing a slot. Drain reports at least one pending item while such an
admission is outstanding, even before its slot is published. Health queue depth
counts capacity reservations, including a producer slot being copied before head
publication; it is not a physical-publication or durability count. Worker drain publishes stopped
when stopped admission and all pending work are resolved; this never substitutes
for joining the worker before repository/transport teardown.

## Admission and capacity

`queue_capacity` is the total number of retained records, including records in
the immutable worker batch. `control_reserve` is inside that total. Detail
records can use at most `queue_capacity - control_reserve`; control records
(lifecycle, checkpoint, gap, and configuration) can use the remaining slots.
Control is finite and can also be rejected when the entire queue is retained.

Admission performs only fixed-size validation, a clock read, atomics, and a
record copy. It does not allocate, lock, hash, perform SQL, touch the
filesystem, or wait for a worker. The queue never overwrites a retained slot.
The producer head is published after the slot copy. The consumer tail advances
only after the repository outcome has resolved.

Invalid records are rejected as `rejected_invalid`. Within a lifecycle, the first
admitted record fixes the producer identity. Later admissions require that same
identity and strictly increasing sequence numbers; repeated accepted keys are
rejected, whether their payload matches or differs. Repository retries are not
producer re-enqueues: the worker retains and resubmits its immutable copy.
A rejected, never-admitted key may be retried before any newer key is accepted.
The producer must keep that retry value unchanged. Gaps in sequence are legal.
Saturation updates bounded rejection metadata but never closes the control
reserve or invents a record using the rejected value's key.

## Flush and immutable batches

A worker pulse flushes when any of these is true:

1. retained rows reach `max_batch_records`;
2. the configured byte budget reaches its fixed-record limit;
3. the oldest admitted record is at least `flush_oldest_after_usec` old (or its
   admission clock was unavailable/backwards, in which case flush conservatively);
4. the worker is draining/stopping or explicitly forcing a drain.

The worker copies at most the configured row/byte bound into one preallocated
in-flight array. While that batch is unresolved, its queue slots remain counted
against total capacity. Retries call the repository with the same copied keys
and payloads; records admitted later remain behind it and cannot be applied
before it resolves. Retry attempts use a bounded exponent (eight levels), with
1 ms initial, exponential, 1 s maximum backoff. The attempt counter saturates
at its cap; retryable and ambiguous work is retained rather than silently
discarded when that cap is reached, so a possible durable commit is never
skipped. Catch-up is capped at 64 drain iterations. The implementation does
not busy-wait for a future retry deadline.

Repository results are checked before any queue prefix advances:

- early `unavailable`, `disabled`, and `invalid_batch` results must have zero
  input/result/counters and zero sequence bounds;
- committed results must have the exact input count, ordered matching keys,
  matching per-record counters, valid per-record outcomes, and the correct
  `committed` versus `committed_with_rejections` aggregate;
- retryable and ambiguous results must have exact keys/counts and retry outcomes
  for every record, with no committed counters;
- malformed or mismatched results are treated as protocol failure and leave the
  immutable batch in flight. In particular, a corrupt success cannot advance
  the stable prefix.

A `committed_with_rejections` result is already a resolved mixed transaction:
its invalid/conflict rows are counted and the whole batch is released. An
`invalid_batch` result for a multi-record batch is definite batch rejection,
not evidence that the valid rows committed. The worker then isolates bounded
single-record calls in FIFO order. It never retries a successfully committed
mixed batch as a whole.

An ambiguous commit blocks every newer repository call until the same immutable
batch is retried/reconciled. A later `invalid_batch` or `disabled` callback does
not resolve that uncertainty and cannot trigger smaller-batch isolation. It is
not safe to infer that no rows committed.

## Loss records and health

The transport does not allocate producer keys or synthesize gap records. The
producer-only private `telemetry_transport_loss_copy_for_producer()` returns
fixed-size cumulative detail/control rejection counters and the first contiguous
rejected sequence range. Once repetitions or noncontiguous rejections occur,
both bounds become zero (unknown) for the remainder of the lifecycle. It is a
producer-owned value copy, not a worker/health-reader API.

These counters describe failed admission attempts, not proven unique lost rows:
a rejected value can subsequently be admitted. Runtime/coordinator #265 owns the
fresh-key allocator, reported-counter baseline, abandonment decisions and actual
`coverage_gap` publication. Never label every sequence in an uncertain span as
missing or turn retries into proven loss. A conservative report uses unknown
bounds and incomplete-coverage quality. A gap itself uses the normal finite
control reserve and may be rejected; the producer retains its pending report.
Process death before publication leaves an unknown tail. The standalone harness
exercises saturation, later fresh-key gap admission and immutable rejected keys.

Health is a copied atomic snapshot. Counters saturate instead of wrapping;
`queue_depth` includes the retained immutable normal batch and the high-water
mark is bounded by the configured queue capacity. Health reads do not inspect
worker-owned repository state or acquire a gameplay lock. A flat-file authority
returns `flatfile_disabled`, rejects admissions, emits no hidden file, and does
not claim healthy zero activity. If repository initialization disables an already
queued transport, queued observations remain visible in RAM until joined shutdown.
Shutdown counts never-attempted detail/control records as drops and records an
unclosed tail. An unresolved in-flight transaction remains an unknown tail rather
than being falsely classified as definite loss.

## Focused private binding

`telemetry_transport_private.h` is not a production public header. It provides
an injected repository callback set and monotonic clock for focused tests and a
coordinator seam. The callbacks are borrowed and must remain valid through
shutdown. Repository callbacks run on the designated worker; the clock may be
used for producer admission and worker scheduling. The default binding delegates
to the real frozen repository API.

`tests/async/test_telemetry_transport.py` compiles the standalone harness
without SQL, runs normal, Address/Undefined, and Undefined sanitizer variants,
and attempts ThreadSanitizer. It reports the host's actual TSAN result. The
harness covers reserve admission, row/byte/age triggers, bounded loss metadata, immutable
retry, retry-exponent exhaustion, and ambiguity barriers, callback validation,
mixed rejections, bounded isolation, quiesce/resume, stop-before-init,
reinitialization, controlled fake-I/O release after nonblocking stop, and
concurrent producer/worker stress.
