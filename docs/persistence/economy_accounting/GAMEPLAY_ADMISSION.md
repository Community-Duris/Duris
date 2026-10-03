# Gameplay admission projection

This increment connects freshly constructed currency commands to a **private,
lifecycle-issued admission projection**. It does not enable accounting at boot,
assert complete game coverage, or authorize a caller-supplied epoch.

`economic_gameplay_authority` retains an immutable lineage, selected epoch,
verified lifecycle receipt identity and native wallet/shared-bank lifetime
mappings. Only the SQL/native lifecycle transaction owners are friends of its
private installer. Receipt verification, complete native enumeration, coverage
admission, maintenance ownership and safe publication are the lifecycle owner's
responsibility. A receipt ID alone is not proof.

## Runtime path

`currency_transaction_submit_identified` first builds the ordinary immutable
currency command. Before journal submission, `prepare_currency`:

- leaves a genuinely legacy runtime command byte-for-byte unchanged;
- in active mode, freezes typed ATM deposit/withdraw intents using the installed
  wallet PID and canonical bank-name/racewar mapping;
- refuses an unsupported writer or unresolved mapping before admission, with no
  native balance change and no fallback to an unaccounted operation;
- preserves already-frozen historical commands rather than assigning them the
  currently selected epoch or a recreated account's new lifetime.

The resulting schema-2 command uses existing held-publication submission. Native
execution still validates locked current authority, the frozen policy, retained
inbox and accounting evidence in the same transaction. The read-only cache does
not supply balances or authorize storage writes.

The existing currency publication path installs committed balances before its
publication acknowledgement. A checkpoint failure retains the original operation
and callback, without a replacement debit. After restart, the retained command
restores an offline continuation and publishes on player entry before releasing
its fence. A malformed or uncertain result never becomes a reported rejection
of an already committed transaction.

Prewritten/replayed commands are not silently upgraded: their exact durable bytes
remain their identity. Other producer adapters must freeze their intent before
their own durable acceptance boundary.

Prepared submission enforces the same boundary: an active runtime cannot enqueue
a new schema-1 command, including a locker payment with a producer-written
`accepted_at_usec`. Locker payment preparation checks admission before its caller
writes the durable request, so an unsupported payment does not create an endless
new request/retry cycle. Already-pending commands can attach without resubmission;
validated coordinator replay remains separate and frozen schema-2 commands retain
their original intent. Lifecycle cutover must resolve or refuse outstanding
legacy producer requests, including locker receipts; a timestamp alone cannot
prove that a payment was previously admitted or paid.

## Validation boundary

`test_economic_gameplay_authority.py` compiles both backend modes under ASan/UBSan
and exercises legacy byte preservation, typed conversion, case-consistent bank
lookup, duplicate/foreign mapping refusal, output preservation, missing coverage,
account recreation, historical intent preservation and atomic cache replacement.
Its private test installer is **not native lifecycle evidence**.

`test_currency_completion_retention.py` links the production currency producer,
cache, codecs and publication path. The added producer scenarios check delayed
visible balance updates, held submission, offline replay and acknowledgement
retry without resubmission. Its coordinator/live endpoints are controlled; it is
not a complete DB-backed ATM journey. `test_currency_input_queue.py` retains the
legacy queued-command/coin behavior check with the added dependency linked.
Prepared-payment cases exercise the real locker payment builder and distinguish
active wallet/bank refusals (both before and after producer preparation), working
legacy payments, and attachment/publication of an already-pending legacy command.

## Remaining integration gate

No boot/config toggle or public installation setter is introduced. Native
lifecycle receipt verification must call the private installer only after the
appropriate writer set is covered. Active unsupported writers remain explicit
refusals; **do not activate a whole game on this ATM-only producer**. The source
and native lifecycle batches, item/corpse composite roots, remaining economic
writers, and actual activated command/restart journeys still determine feature
acceptance. Missing/corrupt durable authority must stop at native boot/admission,
not be reinterpreted as an empty legacy cache.
