# Plan 2: money holdings, transfers, issuance, and expenses

## Latest shared implementation evidence — 2026-10-07

The [literal-capacity qualification](../LITERAL_POOL_CAPACITY_RECOVERY_QUALIFICATION_2026-10-07.md)
passes the original private cold recovery cases on both SQL engines. The narrow
fix is integrated locally. Both maintained original 740-provider production builds and actual pool
controls pass. Root authenticates the complete source and native export, with
exact owned cleanup. The earlier 55-second bind-filesystem link timeout remains
preserved; the Linux successor changes only its bounded production-link budget,
keeping original control execution limits and behavior.

The [auction proof record](../NATIVE_AUCTION_RETAINED_PROOF_PROGRESS_2026-10-07.md)
now identifies the actual relisting failure: the custody INSERT returns 1062
because a claimed historical row retains the globally unique item UID. Private
additive schema64 preserves that history and enforces one unclaimed listing.
Both-engine schema measurement and original producer reruns remain open.

Post-ACK coin boot integration is privately source-reviewed, including native
publication, connection cleanup and legacy-history fencing. Its original strict
SQL production build passes all 741 providers, including nine freshly compiled
translation units and a fresh full link. Flatfile, retained proof and genuine
coin command/ACK/two-cold-boot qualification remain open. Existing inactive
behavior and activation gates remain closed.
These results do not complete this plan or the joint R1–R8 release requirements.


## Current native auction proof progress — 2026-10-07

[Private component progress](../NATIVE_AUCTION_RETAINED_PROOF_PROGRESS_2026-10-07.md)
passes original native-v2 LIST, settlement, partial pickup and exact durable
rejected replay. The next original v1 LIST171 returns retryable EAGAIN; its
internal predicate remains unobserved. Both strict753-provider builds and nine
contracts pass on private d2 source; both-engine component completion, physical
journeys and maintained producer integration remain open. The separately
reviewed post-SQL-proof recovery diagnostic also passes both original753 builds
and nine contracts; runtime recovery qualification remains pending. Existing
inactive behavior, declined spell path and release gates remain preserved.

## Current shared producer qualification — 2026-10-06

The [shared integration record](../SHARED_PRODUCER_INTEGRATION_PROGRESS_2026-10-06.md)
and [current checkpoint](../EXPERIMENTAL_REVIEW_CHECKPOINT.md) own the latest
combined candidate status. Wallet/source handling, complete literal UID forests,
auction compound effects and retained publication are composed together privately.
Both original strict753-provider production builds and all nine contracts pass
on private4f8332ce, with independent primary source/cache authentication. The
first full-tree SQL fixture refuses before auction cases; source review finds
its required genuine template catalog absent. A faithful setup successor
is being prepared. Exact runtime refusal attribution remains open. The genuine cold
shop recovery previously established an empty object pool; the narrow reservation
fix passes both genuine actual-mm profile units. Original cold reaches ready
promotion, then refuses at combined save-start/admission-finish; exact first
bool attribution and complete cold recovery remain open.
These build results do not qualify every writer or complete this Plan. Existing
inactive behavior, refusal gates and the declined spell path remain preserved.


Start from add-double-entry HEAD 49af585c4. Develop and test in inactive mode
against direct typed owners and isolated backend fixtures; Plan 1's final
activation is not a prerequisite. This plan owns general money writer policy
and adapters. Shops, auctions, collector, death, and crafting composites belong
to [Plan 4](04_COMPOUND_DOMAINS.md). See [R2 and R3](../REMAINING_REQUIREMENTS.md).

## Result

Every ordinary money movement and supply change has durable counterparties,
actual denomination effects, a balanced root, and a source identity where
required. The source of new money and the destination of destroyed money are
queryable by reason and operation.

## Starting files and first checks

Inspect src/economy/currency_transaction.c,
src/economy/coin_transfer_accounting.c, src/cmd/actobj.c,
src/cmd/actoth.c, and src/economy/cardgames.c. Start with
python3 tests/async/test_coin_transfer_accounting.py,
python3 tests/async/test_coin_transfer_shared_bank_accounting.py, and
python3 tests/async/test_currency_completion_retention.py. Use
tests/async/run_currency_transaction_schema_mysql.sh with a disposable
database for native SQL acceptance.

## Work

1. Complete wallet/shared-bank/coin-pile account lifetime and native-revision
   adapters on SQL, then flatfile. Preserve denomination changes, including
   making change. Finish all supported pile create, merge, split, pickup, drop,
   and destruction paths; movement of a pile's custody is Plan 3 unless it
   changes value. Verify the current SQL coin component through the pooled
   path with Plan 1's fixture, or supply a failing integration test now.
2. Classify and bind peer transfers and group split recipients, including
   morphs and remainder. Preserve current per-child and command boundaries;
   do not silently promise a new global bulk atomicity rule. Resolve NPC money
   only after a durable holding lifetime/policy is specified, otherwise refuse
   active-epoch writes before native mutation.
3. Map rewards, loot (including mob death coin generation), quest/chaos/epic
   grants, bartender quest rewards, and admin grants to versioned issuance policy.
   Identify each logical source event before accepting a new operation ID. A failed
   reward that stages a claim must not mint again when collected.
4. Blackjack is permanently deprecated under active epochs. Retain the active-epoch
   refusal guard; do not implement table/round stake recovery. Schedule post-release
   removal of legacy card game sources and zone objects.
5. Make every unsupported cash writer fail at admission in an active epoch.
   Classify direct assignments and special procedures from the writer census,
   removing dead writers only with reachability evidence.

## Independent acceptance

- For each supported route, a disposable SQL test asserts native before/after
  vectors and revisions, exact account effects, at least two postings when
  value changes, zero copper sum, source claim where required, and one receipt.
  Repeat on MySQL and MariaDB; port the same behavior to flatfile.
- Fault tests cover denomination overflow, negative holdings, changed replay,
  source-event reuse with a new ID, endpoint failure, pile publication failure,
  lost reply, and restart. Player journeys cover ATM, change, drop/pickup, split
  remainder/morph and claims. Blackjack remains deprecated under active epochs:
  qualify refusal before wager, wallet or pending-payout mutation. Active blackjack
  push/win/loss and round-recovery journeys are outside the supported product;
  existing inactive legacy regression coverage remains separate.
- Focused money regressions and both server builds pass. Report routes that
  remain intentionally unsupported with executable refusal tests.

## Boundary and handoff

The plan exports typed money effects and documented source/sink policy to Plan 4.
It does not change item UID ownership or invent a finite NPC/keeper treasury.
The independently testable deliverable is an inactive but fully evidenced set
of money routes; whole-game activation waits for the other plans.


### Ordinary room coin producer slice integrated; unqualified

Local merge af900753 contains independent implementation6b6b7c10c and the
optional post-ACK staging release contract96e2af83a. Active ordinary-room single-root
coin drop and pickup now use a retained native publication adapter. Original UID,
canonical before/after literal bytes, denominations, custody and exact result
revisions must agree. Native materialization, amount updates and placement keep
explicit started/returned states; uncertain effects remain held. Every ACK retry
revalidates physical evidence and the current wallet body. Work is one attempt per
pulse; notifications and bulk continuation follow durable ACK and owner extraction.
Inactive/schema1 paths remain separate, unsupported active placements refuse and
callback-free cold replay stays held. Production Makefile registration is primary-owned.

Final BEFORE /opt/duris-accounting-coin-publication-before-final-18fbd004fc3e/source
manifest SHA-25648792b5f103e286305ec1c4282b0e864ef494cd8d7566347311b2a41aa2235c7
pins1238 files. Candidate /opt/duris-accounting-coin-publication-committed-3457636af08c/source
manifest45432924e1c0b872d1f9eb3b4916ac93fc033706dfd74089fc145af6a10860ba pins1240.
Prepared24physical+15owner cases per SQL-header/flat profile are unexecuted; BEFORE
uses --scope owner, AFTER --scope all. Their native capture/codec/runtime custody
plus controlled placement/render/materializer seams do not qualify actual actobj
handlers, either database or flatfile recovery. Original300-second compile and
30-second per-case bounds remain unmeasured. Four initial review findings were
corrected; primary final-pin source review remains separate from qualification.

Open: semantic writer/central owner registration, actual native producer and
backend journeys, cold routing, actorless hydration and explicit uncertain native
effect recovery. No source/fixture inventory is promoted to R1-R8 or route evidence.


### Final integrated coin review corrections; unqualified

Final-pin review found four bounded source gaps in6b6b7c10c. Corrections now bind
committed wallet denominations and exact revision advances, plus every native pile
result identity/revision field, to the original immutable command before any live
projection. Postcallback paths preserve an already-blocked canonical receipt
conflict. Every physical publication/ACK attempt rechecks PID/account/race and
current body identity before and after callbacks. Partial pickup also requires
canonical opening weight before rendering the after image. Source pins are
currency_transaction.c94f5bfe23da10bbd25ea0d18ff1a9ba934300f31d91d63bdacc3c1341dcf0dbe
and coin_physical_publication.c00fbca73cf3d4fc49f51c36e59423ec5224ef520539b0ac7d0ae077faa4d532b.
Independent read-only review confirms the four corrections; no runtime checks ran.

The command contains bank revision and zero bank delta, not an opening bank
vector. This verifier checks bank range/revision and retains the authoritative
immutable native result/reconciliation contract for its balances; it does not
invent an independent bank proof from live projection. Generic currency linkage
remains independent of the native world module. Inactive/schema1 and wallet-only
behavior remain outside these physical-schema2 checks. Prepared regression cases,
actual producer/recovery journeys, cold ownership and full Plan2 acceptance remain
required; no issue/route is marked qualified or release-ready.

### October4 physical coin/shared owner source milestone; unqualified

Typed SQL and flat physical coin recovery now connects the existing complete
retained original/completion proof, locked current native wallet/bank/pile/room
literals, inert projection and reserved save/publication owner. Covered flat
ordinary-save revision reads and direct execution permits use existing authority.
Central flat coin admission stays closed until native active bootstrap and this
plan's original qualification pass. Other money writers and source policies
remain required. See the consolidated integration report for source inputs;
new native/gameplay/restart checks wait for this major plan's readiness batch.


### October 4 flat runtime source prerequisite integrated; unqualified

Configured-root boot verifies the retained lifecycle and complete current native
mapping/wallet/bank census before selecting the accounting admission projection
and owned save replay. It creates no baseline or activation. Successful shutdown
clears only after the original save/coordinator owners close; copyover and refused
shutdown retain it. Central flat coin admission remains closed. Native v3 format
compatibility and matching Plan5 reader integration, then this plan's original
backend/gameplay/recovery batch, remain required.

### October7 ordinary coin endpoint-counter regression qualified


[Focused qualification](../COIN_RECOVERY_OWNER_COUNTER_QUALIFICATION_2026-10-07.md)
repairs both locked endpoint counters and exact item owner_revision. Original
source fails three regressions; corrected maintained source passes all ten native
ASan/UBSan cases. Authority/world seams limit this evidence to cache projection.
Post-ACK SQL room coin boot restoration and full Plan2 qualification remain open.
