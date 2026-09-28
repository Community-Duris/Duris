# Death-conflict evidence envelope

## Implemented boundary

Snapshot wire format **10** is a read/encode format for bounded, lossless death-conflict
observations. It extends the existing death body with an evidence envelope, inside the
same player PID, player save revision, and death operation ID. Ordinary capture still
emits format 7; terminal-death capture still emits format 8. The save worker and SQL and
flatfile **save** repositories still refuse format 10 as an input. The separate SQL
archive writer captures it without applying a death. A new, dormant terminal-state
owner can commit that derived evidence together with a format-8 request's terminal
state; it is not selected by normal gameplay. A strictly disposable `TEST_MUD`
selector is available for the combined acceptance journey described below. The
DB cold-load refusal and authenticated read-only recovery adapter are implemented
below. **This is not a production-qualified death-release fix, accounting activation,
or item restoration.** The explicitly gated disposable journey has qualified the
limited retention-to-menu path described under "Disposable runtime qualification".

Legacy format 7/8 fixture bytes are pinned to hashes measured with the pre-extension
encoder. Formats 1–6 keep their existing decode paths. An evidence-bearing value
cannot be downgraded to format 8: encoding rejects it instead of discarding evidence.

## Preserved observations

The extension contains five table-shaped blocks in this fixed order:

1. `player_items`
2. `player_item_affects`
3. `player_item_extra_descr`
4. `item_current_owner`
5. `item_owner_revision`

Each block stores ordered column names, then rectangular rows of nullable byte
strings. SQL NULL, an empty string, embedded NULs, and arbitrary field bytes remain
distinct. Physical row IDs, original object UIDs, container references, revisions,
owner kind/ID/context, and authority state remain observations; encoding does not
parse, repair, renumber, deduplicate, or turn them into ownership grants. Duplicate
UID observations and contradictory topology are deliberately representable.

The auxiliary item tables are mandatory blocks even when empty: deleting a physical
`player_items` row can cascade to its affects and extra descriptions. The parent
row's `container_id` is mandatory metadata for the same reason. The codec does not
prove that a caller has captured the complete affected graph.

The containing snapshot stays limited to 4 MiB. Evidence has at most 64 columns per
table and 64 bytes per column name, with unique identifier-shaped names and required
identity/topology columns. Evidence rows also consume the snapshot decoder's global
8,192-row budget. Field values may exceed the ordinary 4-KiB snapshot string limit;
SQL TEXT-sized values are preserved, within the overall byte budget. Malformed,
truncated, oversized, or structurally incomplete envelopes fail without publishing
partial output or truncating data to fit.

## Dormant SQL retention and query interface

`src/player/player_death_conflict_repository.{h,c}` provides the archive-only
`player_death_conflict_retain` writer and PID-scoped list/detail queries. It has
**no gameplay admission caller**. This archive-only API does not replace
`player_snapshot_repository_apply`, advance a player revision, create a death
disposition, clear inventory, change custody, move money, instantiate objects, or
permit extraction. The normal conflicted-death refusal remains intact.

The writer:

- Requires a valid immutable format-8 death request and an exclusively borrowed,
  idle, autocommit SQL connection with automatic reconnect disabled.
- Holds the existing maintenance/writer fence and a SERIALIZABLE transaction while
  locking the player revision, physical player-item rows and their auxiliary rows,
  a bounded closure of referenced custody roots/parents/descendants, and the relevant
  owner-revision rows. Foreign custody is evidence, never a grant; foreign physical
  inventory is not imported into this player's archive.
- Reads column bytes with `CAST(... AS BINARY)` and explicit lengths, so connection
  charset, SQL NULL, empty fields and embedded NULs cannot silently alter evidence.
  Size/row preflights reject an unrepresentable result rather than truncate it.
- Inserts an immutable `player_death_conflict_evidence` row with the original
  operation, player/revision/corpse identity, observed source revision, canonical
  request digest, payload digest and format-10 payload. There is no REPLACE/UPDATE
  path. Operation, player/revision and player/corpse collisions refuse reuse.
- Replays against the original request and stored payload, not newly observed SQL.
  A lost COMMIT reply is `commit_unknown`, resolved using the original operation and
  request after reconnect. It is never converted into a new logical death.

The read API verifies both digests and all indexed identities against the decoded
payload. List queries paginate by save revision with a fixed 25-case maximum. Both
queries predicate on PID and preserve caller output on failure. The game adapter
obtains PID from a trusted character entry belonging to the authenticated account,
rechecks SQL membership, and projects bounded sanitized summaries. **The underlying
repository calls alone are not authentication, UI, or delivery authority.**

Accounting cutover refuses any unresolved archive before installing a baseline.
The archive is not an authoritative economic source or opening holding. There is
not yet a resolution record, so no stored case may be silently considered closed.

## Stable live terminal-death request

The fight retry path resumes an admitted request before generating another operation
ID or temporary wallet object. The pipeline keeps the original format-8 snapshot,
revision, operation, corpse UID and wallet-pile UID across timeouts, terminal worker
failures and ambiguous completions. Requeue copies that retained request rather than
recapturing the live character. Initial capture/admission failures release their
unused fence and pin without authorizing extraction.

The retained copy and queued copy are charged against the existing snapshot-count
and encoded-byte budgets. An unresolved pin refuses ordinary capture, revision marks,
hydration and fence replacement for that PID. The death completion gate requires the
original revision, full component mask and an applied/already-applied result at that
exact durable revision. Stale/newer unrelated completions and journal-only durability
cannot release it. Exact database acknowledgement or pipeline shutdown/reset clears
the retained pin; ordinary non-death terminal calls still capture a fresh intent.

The worker also requires that exact successful death result before invoking the
journal ACK hook. A claimed success at a different durable revision becomes a
failed completion with the original revision still queued for retry. Journal
restart replay likewise blocks a stale or non-exact result without checkpointing
the death record. Ordinary non-death acknowledgement policy is unchanged.

This is live request retention, **not** a new ownership or recovery authority. It
neither selects the dormant SQL terminal owner nor provides cold-load/UI admission.
Restart recovery and unassisted release still require the combined integration gate.
The existing terminal-result numeric values remain unchanged.

## Dormant terminal-state owner (SQL only)

`player_death_conflict_apply` and its pooled adapter are separate from archive-only
retention. **The normal save pipeline still does not select this adapter.**
Normal selection and release require stable request/fence integration, a cold-load
gate, authenticated read-only visibility, and the unassisted runtime acceptance
journey.

For that acceptance work only, a SQL build compiled with `TEST_MUD` can select the
adapter when **all** of these process-environment conditions hold:

- `DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY=1`;
- `TEST_DB_DISPOSABLE=1`;
- `DB_HOST` is exactly `127.0.0.1` or `localhost`;
- `DB_NAME` is `corpse_journey_test_` followed by exactly 12 lowercase hexadecimal
  characters, matching the disposable combat fixture's generated schema name.

Missing or malformed values preserve the ordinary SQL owner. Builds without
`TEST_MUD` cannot select this route, and flatfile selection is unchanged. This
selector does not replace database target allowlists, authorize use of an existing
schema, or activate accounting. Both live worker initialization and journal replay
use the same selector. `test_death_conflict_selection.py` executes the actual
selector across build modes and valid/invalid fixture environments; this is wiring
coverage, **not** successful death-release or socket/restart evidence.

The owner accepts the original format-8 request, not caller-supplied format-10
evidence. It preserves normal native death application; only the exact custody
payload mismatch can enter the terminal-conflict transaction. An existing case
must replay its original operation and complete payload identity. A different case
for the same PID refuses admission.

For a conflict, one maintenance-serialized SQL transaction:

1. Locks and captures the unchanged physical payload and custody observations, or
   verifies the exact existing immutable archive.
2. Inserts and reads back the bounded format-10 archive and both digests.
3. Requires the locked player revision to match the archive's source revision and
   the wallet revision to match the request. Wallet conversion must already have
   completed: all requested/current wallet balances and the temporary pile UID
   must be zero. This owner never debits money or creates another coin pile.
4. Persists the request's non-item character state and a format-10 death disposition
   with custody **observations**, then advances the player revision. It skips the
   ordinary inventory/pet projection and death custody-quarantine mutation.
5. Verifies the exact disposition, archive and revision before COMMIT. Only a
   successful commit, or an exact committed replay, returns a database completion.

The transaction preserves player item rows and auxiliary bytes, pet rows/items,
custody and owner revisions, and wallet balances/revision. A status/disposition/
revision failure rolls the new archive and all terminal writes back together. An
archive-only case is not a terminal receipt. A player revision counter alone is
also insufficient: death replay requires the exact disposition, and a format-10
receipt must still match its durable archive. Lost COMMIT replies remain ambiguous
until same-operation replay establishes the committed result.

`player_snapshot_repository_write_retained_death` is only a participant in that
owned transaction; its result is **not** a commit or extraction acknowledgement.
It refuses use outside a transaction and verifies the actual stored archive.

While any case for a PID remains unresolved, later ordinary checkpoints cannot
overwrite that player's state, and the ordinary restitution evidence query refuses
delivery admission. Raw evidence is not an ownership grant. No case-resolution or
delivery API is introduced by this slice.

## DB cold-load refusal and read-only account recovery

Every ordinary SQL character load checks for an unresolved archive before optional
components, items or pets are read. It obtains a shared **primary-key player-row
lock before establishing its repeatable-read snapshot**. Retention obtains an
exclusive lock on that same row before inserting a case, so a previously committed
case is visible and a later retention waits until the read decision finishes. Name
resolution is nonlocking and outside this transaction; the resolved PID is locked
and revalidated before payload reads, avoiding a broad locking name scan or an
unlocked identity after name reassignment. Caller-owned transactions are refused,
not committed or rolled back. The transaction permits locking reads but issues no
mutating SQL.

The check also applies to name-based loads and metadata-only deletion-confirmation
loads. A retained case, failed read, or missing archive schema refuses the load;
status, domain and payload output is cleared rather than handing a partial,
save-capable character to the caller. A confirmed empty archive preserves the normal
load path. The account completion returns a refused character to the authenticated
menu and identifies option **9**, not an empty in-world inventory.

Account option 9 selects only a character from that account's trusted list, by its
displayed ordinal or name. Numeric text is an ordinal, never a raw PID. The adapter
uses the existing asynchronous player-load worker, with item/pet loading disabled:

- `L [after-revision]` lists at most 25 stable case IDs in save-revision order.
- `D <32-hex-case-id>` reads one PID-scoped, hash-checked case and returns a bounded
  summary with captured labels and aggregate evidence counts. Raw SQL cells, payload
  bytes, other owners' physical inventory, and delivery authority are not exposed.
- `C` changes the selected character; `0` cancels a pending read and returns to the
  account menu.

SQL authorization rechecks live membership and the character name inside its read
transaction. A non-NULL account on the character record must agree with that
membership, matching ordinary load authority; only a legacy NULL account permits
association fallback. List and detail reads fail closed when those records disagree.

Successful output requires the same live descriptor, request ID, PID, account and
selected character at completion, with that character still in the account list.
Stale/disconnected results cannot display evidence or retire a newer request.
Failed reads are not displayed as empty successful results. Terminal/MUD control
bytes and format delimiters are stripped at both summary and rendering boundaries.

These records are still unresolved. This surface does not instantiate objects,
change custody, restore money, resolve cases, or authorize terminal extraction.
Repository fixtures and executable callback bodies are separate from a real
authenticated socket journey; a DB restart test is not a game restart qualification.

## Disposable runtime qualification

At `0499544179d338db85463391778bf528d7cbba36`, a fresh MariaDB `TEST_MUD` build
passed the real TCP combat fixture with `--one --require-unassisted-recovery`.
The tested variant uses ordinary coins and no boons. Healthy load/death/loot/reconnect
was followed by an injected missing-payload/stray-row conflict and actual player
death. The game reached an exact durable death acknowledgement and then the account
menu without manual SQL repair. Option 9 list/detail, refused unsafe cold entry,
full game restart and authenticated account reconnect all passed. Retained state
was byte-identical after acknowledgement, cold-entry refusal and restart; the
existing wallet debit matched the native coin projection rather than being mistaken
for missing money.

This is evidence for that disposable selector and variant, not normal-owner
activation, copyover, every death/spell variant, resolved custody, or restoration.
The ordered WIZLOG must reach `death_disposition_recorded` before
`death_disposition_completed` with `extract_refused=0`; earlier expected refusal
alerts alone do not establish either completion or failure. Re-run the fixture on
any subsequent source change that invalidates this contract.

## Integration gates still open

Before enabling this route for ordinary gameplay or expanding beyond the disposable
selector and qualified variant:

- Capture the complete physical payload and auxiliary rows under the same transaction
  as the observed authority/revision state. Check the full cascade-reachable graph,
  including an unexpected cross-player container reference. Do not silently delete
  an unrepresented row or infer ownership from PID membership alone.
- Qualify the pinned request and exact-ACK gate with the SQL owner in the faulted
  gameplay journey. Timeout/retry/restart must keep the original logical identity
  rather than recapture or mint replacement item IDs.
- Preserve unresolved custody/binding conflicts as unresolved. A stored byte copy is
  evidence, not permission to restore an item to the player or extract the live tree.
- Qualify the implemented read-only recovery surface through the authenticated game
  socket, cold reconnect and game restart. Keep reviewed restitution paths closed
  while evidence is unresolved; do not confuse visibility with restoration.
- Make retained data survive rollback, ambiguous commit, reconnect, and restart.
  Prove exact read-back before treating a retention acknowledgement as release-safe.
- Keep the current fail-closed hold on capture limits, failed durable writes, or
  missing recovery routing. Never count a manual SQL repair as unassisted recovery.

`DEATH_CONFLICT_ACCEPTANCE.md` remains the end-to-end release gate.

## Executable coverage

`python3 tests/async/test_player_death_recovery_query.py` checks adapter wiring and
pure identity/sanitization behavior. `test_player_death_recovery_account.py` compiles
the production selector, submission, refusal and callback bodies with controlled
descriptor/queue I/O under ASan/UBSan. It exercises current/stale identities, a newer
request, disconnected/freed descriptors, cancellation, malformed selectors/cursors,
bounded output, and refusal without materializing a character. It does not simulate
a successful socket authentication or claim a live gameplay journey.

`python3 tests/async/test_player_death_recovery_mysql.py` compiles the real load,
query and archive repositories. SQL execution requires `TEST_DB_DISPOSABLE=1` and
an explicitly supplied loopback `economic_schema_test_*` database initialized with
the bootstrap and immutable migrations. Without that fixture it reports runtime
as skipped. The native harness seeds retained cases through the actual archive API,
then tests healthy loads, full/metadata-only PID/name refusal, worker-repository
dispatch, pagination/detail identity, account/name isolation, conflicting association
and character accounts, legacy NULL-account fallback, deleted membership,
sanitization, corruption/missing-schema refusal and caller-owned transaction safety.
Length-prefixed comparisons preserve NULL/empty/binary distinctions while checking
that player/item/pet/custody/wallet/archive rows remain unchanged. The compiled
`--verify-restart` mode reuses that fixture after a real DB restart. Neither mode
invokes the dormant terminal owner or counts a seeded archive as unassisted death
recovery.

The native harness additionally pauses the real loader on one SQL connection while
the real archive owner runs on another. Eight PID/name × full/metadata × ordering
cases verify both a retention commit after `START` being refused by the loader and
a writer timing out on the load lock, then succeeding after release. Two name
reassignment cases refuse an unlocked replacement PID. Healthy case-insensitive
loads, retained-case refusal after DB restart, caller transactions and byte-exact
source preservation remain covered. These barriers wrap client calls only in the
test executable; they do not simulate SQL responses or add production hooks.

SQL-row serialization cannot see an unresolved death that exists only in the save
journal. A separate acquire/release readiness latch therefore refuses all normal
SQL reads **before pool acquisition** and all shared player materialization until
startup replay completes successfully. Both asynchronous and synchronous SQL loads
use the same guarded callback. Gating only materialization is insufficient: a
pre-replay result could otherwise be cached at the account menu until readiness
opens. Read-only recovery list/detail queries retain their independent identity and
authorization checks and remain available while normal loads are fenced.

The latch defaults closed and remains closed during startup, failed initialization,
blocked replay and shutdown. Replay publication is serialized with shutdown under
the pipeline mutex. This is deliberately global, not a PID-scoped exception: an
unresolved journal can deny unrelated normal loads until a successful restart/replay.
It grants no ownership and does not select the native owner or activate accounting.

`python3 tests/async/test_death_journal_load_fence.py` combines the actual journal
and materializer with a controlled readiness latch. The complementary
`test_death_journal_pipeline_lifecycle.py` runs the real SQL-build dispatcher,
workers, journal, latch and materializer. It reopens the same durable death across
retryable, terminal, ambiguous, stale and wrong-revision results, then exactly
acknowledges it. Barriers prove startup refusal, unchanged character/status/item/pet
hydration on refusal, empty failed read results, successful replay/read admission,
shutdown refusal and preserved recovery-query routing. The SQL apply callback and
connection acquisition are controlled boundaries in that test; it does **not**
claim live SQL retention or network/copyover journey acceptance.

Neither gate makes a later delete safe after an earlier metadata confirmation.
The SQL deletion path therefore takes the same exclusive primary-key player-row
lock immediately after starting its transaction and **before** soft deletion or
artifact/locker/account/custody cleanup. Its first consistent read checks for an
unresolved case. Missing identity, retained evidence, or a failed read refuses the
operation and leaves cleanup for the existing rollback path. Direct physical
`sql_delete_player()` calls own the same guarded transaction; callers already in a
transaction must have guarded that PID before their first consistent read. This
call-order contract must be preserved by future callers. No resolution or authority
grant is inferred from the absence of a case.

The authenticated web-admin handler must also honor this boundary. In either
SQL-primary mode, a failed `restoreCharOnly()` defers deletion without raw-PID soft
deletion or account-list cleanup. After a successful restore, only the typed
`character_delete_result::deleted` outcome permits account unlink/write and a
deletion-success response. Refusal and reconciliation-required return correlated
errors without further account cleanup. Reconciliation-required may follow a lost
COMMIT reply or already-committed cleanup, so it must not promise that SQL account
data was unchanged. Flatfile-primary retains its separate compatibility path.

`test_ws_admin_delete_character_runtime.py` executes the actual handler and
temporary-character cleanup with controlled stores under ASan/UBSan. It covers
both SQL restore failures, typed refusal/reconciliation/success, auth and hook
refusals, request correlation, and flatfile compatibility. Independent bypass
controls must make each of the two SQL safety assertions fail. This handler test
does not exercise WebSocket transport or substitute for the real SQL deletion
serialization fixtures.

Read cleanup is also part of the admission boundary. Failed/lost START, uncertain
COMMIT, failed ROLLBACK, and dirty/exceptional load results cannot leave a reusable
snapshot in the pool or publish buffered character/recovery data. The lease owner
marks an uncertain handle for destruction on release, preserving its lifetime until
the borrower has finished cleanup. A successful rollback can preserve a healthy
lease. Discarded capacity is reopened lazily through the normal validated factory,
not permanently lost: healthy slots are preferred; one empty slot is reserved
while reconnecting outside the pool mutex; failed/throwing opens release that
reservation. Shutdown waits for the reservation and rejects any late replacement.
The existing runtime exclusion check still applies to new handles.

`test_player_read_transaction_cleanup.py` runs production query/pipeline/pool code
with controlled protocol faults. `test_sql_pool_discard_recovery.py` exercises
repeated discard, reopen failure/exception, concurrent healthy borrowers, shutdown,
and exclusion refusal. `test_sql_pool_discard_mysql.py` additionally uses real SQL
sessions to verify rollback on close, killed-session retirement, and committed
writes from replacement borrowers. Its connection factory is fixture-supplied;
this is pool plumbing evidence, not normal game boot qualification.

`test_account_character_delete_runtime.py` executes the production deletion body
with controlled cleanup boundaries. `test_player_death_recovery_delete_gate_mysql.py`
executes the production SQL guard/delete functions against InnoDB. The complementary
`test_player_death_recovery_delete_serialization_mysql.py` observes real server
locking queries while a second connection holds the player row, exercising both
commit orders. Its competing publisher implements the row-lock/evidence-insert
boundary, not the complete terminal save. A deliberately premature repeatable-read
snapshot must make its retention-first case fail.

The pool SQL fixture requires an empty, disposable loopback
`death_read_pool_test_<12 lowercase hex>` schema; the deletion serialization fixture
requires `death_delete_gate_test_<12 lowercase hex>`. Both require
`TEST_DB_DISPOSABLE=1`; without it their standalone drivers compile and explicitly
skip SQL runtime. These checks do not prove live terminal extraction, account-menu
socket ordering, or game restart/copyover.

Run `python3 tests/async/test_stable_terminal_death_request.py` for the production
retention/requeue/revision/ACK helpers, and
`python3 tests/async/test_terminal_death_entrypoints.py` for the actual public death
capture/resume/wait functions with controlled capture and completion I/O. They cover
immutable retries, component/revision refusal, memory accounting, repeated capture
failure cleanup beyond fence capacity, degraded-load admission and journal-only
holds. `test_player_save_pipeline.py` separately preserves the ordinary terminal
fresh-intent and degraded-load refusal cases. These focused harnesses are not the
real SQL-worker/gameplay acceptance journey.

Run `python3 tests/async/test_player_death_conflict_evidence_codec.py` for the real
codec harness, or add `DEATH_EVIDENCE_SANITIZERS=1` for ASan/UBSan. The harness covers
byte-exact replay, frozen legacy bytes, NULL/empty/binary fields, auxiliary payloads,
large SQL TEXT, foreign and absent authority, missing payload observations,
operation binding, no promotion into inventory/custody, structural and size limits,
and every truncated prefix of the encoded evidence fixture.

These codec tests alone do not prove SQL collection, durable retention, player
recovery visibility, or successful unassisted terminal release.

`python3 tests/async/test_player_death_conflict_repository.py` compiles the real
repository and exercises explicit flatfile refusal. Its SQL case only runs with
`TEST_DB_DISPOSABLE=1` and a dedicated `economic_schema_test_*` fixture; otherwise
it reports the runtime test as **skipped**, not a database pass. Initialize that
empty fixture with the bootstrap and immutable migrations before running it.
The real SQL harness exercises the original custody-mismatch refusal, archive
read-back, source noninterference, nullable/binary auxiliary fields, duplicate UID
observations, foreign custody closure, immutable replay after source changes,
identity collisions, PID isolation, maintenance refusal, caller-transaction safety,
late insert rollback, corruption rejection, byte/row bounds, pagination, and cold
reconnect. Its `--concurrent-replay` mode synchronizes two independent connections
on identical requests, checks one retained/one replay result and one durable row per
pair, then verifies exact same-operation replay and unchanged authoritative sources.
Its `--verify-restart` mode verifies the same operation after a real DB
restart. `--ambiguous-commit` requires a protocol fault injector which observes a
server COMMIT-OK and drops that reply; without that fault, the mode must fail.

The SQL harness also provides `--terminal-matrix`, `--terminal-atomic`,
`--terminal-concurrent`, `--terminal-ambiguous`, and `--terminal-restart`. Each first-run
mode requires a fresh, task-created schema with the bootstrap and immutable
migrations; the restart mode reuses the preceding committed fixture. They exercise
the real terminal owner, non-item status persistence, one immutable receipt,
unchanged item/auxiliary/pet/custody/wallet rows, exact replay, counter-only and
changed-request refusal, fault rollback, maintenance and caller-transaction refusal,
and same-operation recovery after a dropped COMMIT-OK reply. The ambiguity mode
must fail when no reply is actually dropped. Compile via the existing `compile_sql`
helper; sanitizer flags can be supplied through its `extra_flags` argument.

Neither repository success nor these tests establish the player-facing terminal
release contract. That requires the remaining integration gates above.
