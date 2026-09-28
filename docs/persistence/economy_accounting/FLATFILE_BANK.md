# Typed flatfile ATM root

Status: executable root transaction component for ATM deposit/withdrawal. It is
not wired into gameplay dispatch. Baseline/coverage activation, native lifecycle
ownership and global legacy receipt fencing are still prerequisites. Other
currency roots and compound transactions remain unfinished.

## Borrowed-lock native reads

This component reuses `42cacc40e` native reads and the standalone `d47c7af0b`
bank owner on retained-metadata PR #606. Identity and domain reads borrow the
already-owned locks. Domain reads recover the shared and legacy journals without
reacquiring the domain mutex. Their native codecs and legacy receipt validation
remain authoritative; failed reads preserve caller outputs.

## Authority and effect proof

`flatfile_accounting_bank_transaction::apply` owns identity then authority locks.
It recovers the shared journal and both legacy domain journal formats, then looks
up the original operation ID before current epoch, name, activity or balance
policy. An exact retained command returns its original result after semantic
verification. Conflicting command bytes, including the accepted timestamp, cannot
reuse that ID. The input command is never rewritten.

For a fresh operation, the owner checks the addressed player's legacy receipt
before current identity or bank policy. A legacy receipt cannot prove schema-2
accounting, so any matching legacy ID conflicts. A missing player file remains
distinct from a valid file with no matching receipt. This local check does not
establish global historical uniqueness; the retained legacy archive remains open.

The frozen intent is regenerated through the shared typed ATM builder, including
writer/reason/actor, epoch, immutable wallet/bank lifetime IDs, racewar and command
binding. Derived native account locators use canonical lowercase ASCII; original
command bytes retain their casing. Fresh effects require the original active
mappings and matching native PID/account/racewar, never a new bank lifetime found
by name. The login-block flag adds no new execution-time policy to an admitted
ATM command. Login/action eligibility remains an admission concern.

The shared currency adapter prepares the exact before/after state using the
existing flatfile revision policy. `currency_flatfile_mutation_writer` is private
to this owner. It rereads the locked authority, compares payload and complete
before-state, regenerates the mutation, and encodes both domain after-images. It
decodes the exact staged bytes and compares their currency state. Restoring only
the old currency fields must reproduce the original canonical player and bank
bytes, proving unrelated fields and every legacy receipt were preserved.

A successful ATM contributes two native domain images plus two retained-evidence
images to one shared authority transaction. A business rejection contributes only
the two evidence images and its unchanged original state/result. Supported durable
rejections are stale revision, insufficient funds and arithmetic/revision overflow.
Corruption, missing authority, unsupported frozen semantics or storage failure do
not become business rejections. The existing exact 32-operation/256 MiB journal
limits apply; neither staging nor commit splits the root.

After commit, the owner rereads and semantically verifies retained evidence,
compares actual domain files with the exact staged bytes, and verifies balances
and revisions while retaining both locks. It acknowledges success only afterward.
No schema-2 operation is appended to the legacy 512-entry/64 KiB player receipts.
Existing receipts remain byte-preserved by the domain codec.

## Historical replay and failure classification

Retained verification checks control lineage, the evidence initialization bitmap,
historical epoch membership, wallet PID and bank lifetime/context against retained
mappings. It ignores current mapping retirement, mutable bank name, selected epoch
and current domain files. The retained epoch reader does not require that the
requested epoch remains active.

For success, the owner reconstructs the original before-state from exactly two
plan accounts, reruns the shared adapter, compares the entire canonical plan, and
checks every result field and the maximum durable revision. Extra cancelling
postings, unrelated effects or altered result bytes fail verification. For a
rejection, it reruns the original business decision against the stored unchanged
state and requires the exact nonzero code. Empty plans alone do not prove a valid
rejection.

Every verified bank receipt must carry failure stage `none`. Completion copies
that stage, and postcommit readback compares it with the staged record. A valid
but non-none stage is rejected as corrupt bank evidence. Accounting result capacity
remains 4096 bytes; legacy native receipt capacity remains 2048 bytes.

Verified retained success returns already_applied; newly verified success returns
applied. Verified durable business rejections return terminal_failure with their
original result. An exact-ID content conflict returns terminal_failure/EEXIST.
Other prepublication failures remain retryable. Once publication is attempted,
failure or failed readback returns ambiguous_commit, preserving the original ID
for recovery. Retained-store lookup allocation failure returns ENOMEM, distinct from a full
retained store; legacy readers exposing only io_error retain EIO. Root code never
retries under a new ID.

## Verification and remaining boundaries

`tests/async/test_flatfile_accounting_bank.py` builds a native ASan/UBSan journey.
It uses synthetic lifecycle metadata and actual native identity/domain codecs.
It covers atomic publication interruption, allocation failure and retry, exact
retained replay beyond the legacy receipt limit, rejection, altered evidence,
retirement/rename/paused-epoch replay, full legacy receipt capacity and competing
players sharing one bank. Canonical successful ATM plans are compared against the
shared adapter's SQL revision policy; this is not a new SQL database journey.

Metadata reads and evidence appends retain their documented bucket/segment bounds.
The currency writer stages exactly two domain files, each at most 64 KiB, and
keeps at most two decoded legacy player receipt vectors of 512 entries. Root
metadata work is bounded but currently repeats selected reads; production latency,
contention and maximum-load qualification remain required. Process interruption
tests do not establish physical power-loss behavior.

No production baseline is created by these APIs. Full semantic restore, admission
and source coverage, lifecycle effects, global legacy archive retention, all other
root/compound adapters and final release qualification remain open. The writer
inventory adds this harness as a test candidate without upgrading any gameplay
route to qualified.

### Current focused evidence

Both native ASan/UBSan suites pass. Native reads cover lock ownership, journal
recovery, exact legacy receipts and allocation failures. Bank tests cover the
journal-only interruption and four image publication boundaries, five forged
receipt variants, 608 allocation failures (224 during/after commit), more than
512 exact-ID replays, full legacy receipt capacity, and shared-bank contention.
The 26 accounting contract tests and source census validator pass; coverage
remains incomplete. Existing native identity and player-domain repository regressions also pass.
Full server builds are reported with this increment's publication evidence.
