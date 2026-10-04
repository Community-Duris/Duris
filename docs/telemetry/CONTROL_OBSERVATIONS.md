# Typed control observations

Record family 13 preserves individual control resolutions and separate target
state evidence. Existing kind-11 counters still measure accepted mutation
operations. Their modifier flags are segment unions. Definition-5 contribution
reports and definition-6 build reports retain their meanings.

This is an intermediate part of the [accepted balance expansion](BALANCE_EXPANSION_PLAN.md).
The native producer inventory, retained control publication and full comparison
suites remain required. Live state observations currently declare partial
duration coverage. They cannot establish time disabled or a complete success
rate. The seven final requirements in [#258](https://github.com/Community-Duris/Duris/issues/258)
remain open.

## Why these observations help

| Balance question | Evidence captured | Remaining qualification |
| --- | --- | --- |
| Does a race or build resist a control effect more often? | Effect family, actual resolved result, source and target context, immutable configuration/build/content versions | Complete attempt producers, comparable opponent strata and coverage-aware denominators |
| Can a group keep a solo target controlled for most of a battle? | Separate target state masks and disjoint measured prefixes; resolution rows never contribute elapsed time | Complete native mutation/removal inventory and reviewed rules for actual action restrictions |
| Did immunity, a saving throw or movement protection decide a zone encounter? | Distinct immunity, saved, resisted and movement-protection results at the observed branch | Distinct zone attempts, objective/failure/recovery evidence and committed reward links |
| Did a patch change control behavior? | Exact versioned records, independent family loss review and immutable replay keys | Retained control generations, repeated-player/team sensitivity and sufficient comparable samples |

An accepted sleep refresh is an operation even when the sleep flag was already
set. Entangle can set an untimed bound flag; its configured ticks are zero in
that branch. Configured scheduler ticks, including signed native declarations,
are preserved as declarations. They are not seconds, elapsed duration or
guarantees that the target remained affected.

Blindness and slow are selected control states with different restrictions from
paralysis, silence or sleep. A union of selected flags is not automatically a
union of time unable to act. Race exceptions, legal commands and removal paths
must be qualified before that interpretation is published.

## Native resolution boundaries

The maintained `blind` and `Stun` helpers and the major/minor paralysis, slow,
sleep, silence and entangle spell bodies report the branch that actually ran.
Saving throws and resistance tests are evaluated once at their existing gate.
Accepted callbacks follow the actual flag/affect mutation. The original
kind-11 accepted-control callback remains intact.

The typed result vocabulary is applied, saved, resisted, immune,
already-present, movement-protection, target-protected, source-ineligible,
target-ineligible, location-ineligible, level-ineligible, class-ineligible,
percentage-rejected and unclassified-rejection. A producer does not manufacture
a result for an actor whose native identity/context cannot be validated.

Flags preserve trusted source, negative-level declaration, actual saving-throw
bypass, accepted sleep refresh, accepted half-duration stun and actual self
targeting. Rejections have zero configured ticks and no accepted-only flags.
Optional battle references are all zero when there is no existing association.
A rejected attempt refreshes existing participation context without enrolling
an outside target, creating a hostile edge or merging battles.

The focused expanded native journey observes 56 typed resolutions: 18 accepted
operations and 12 distinct rejection reasons. Its independent kind-11 journey
still conserves 17 applied and 17 received operations; the accepted self sleep
outside the battle explains the different accepted-resolution total. These
fixture counts are qualification cases, not player balance measurements.

Outer blindness-spell gates and the complete set of other selected-status
producers/removals remain in the native inventory. The current hook set cannot
claim an exhaustive gameplay attempt denominator.

## State, ordering and loss

The pure accumulator has 512 fixed target slots and occupies 213,048 bytes.
Events allocate no memory and retain no gameplay pointers. Entry establishes a
fresh baseline. Equal state samples coalesce; a state change seals a disjoint
prefix. Overlapping effects share one target timeline, so summed family times
must not be mistaken for their union.

Association/context/group/session or configuration cuts preserve only the
actually observed old prefix. A later decision is censored. Departure and
battle closure do not extend state through inactivity grace. Target state rows
carry no guessed source/caster credit. The existing bounded build sampling pass
supplies live point reads; there is no extra world traversal.

Live state coverage is zero with `CONTEXT_UNKNOWN` until the complete native
mutation inventory is qualified. Gap rows clear masks, availability and
coverage. Configuration-unavailable gaps also clear configuration/classifier/
policy/build/content versions. Delivery loss requires a cleared gap and fresh
baseline; a missing predecessor never becomes continuous time. Capacity refusal
and sequence exhaustion preserve uncertainty and never recycle sequence keys.
A UTC regression stays marked even if a later UTC sample recovers.

## Storage and independent review

The C++ payload occupies 408 bytes; its portable encoding is exactly 368 bytes
with 76 typed fields. The full telemetry record remains 488 bytes. Canonical
fields are defined in `src/telemetry/telemetry_control_fields.inc`; the independent
Python codec and validator are in `scripts/telemetry/control_contract.py`.

Migration `0067_telemetry_typed_control` adds nullable family-13 columns and a
unique producer/operation-sequence index to the existing ingestion table.
Insert and update validation triggers enforce eight rules without exceeding
MariaDB's table-definition metadata limit. Their exact bodies are verified;
runtime metadata fingerprints include trigger definitions, timing, event, order
and SQL mode. Guarded reruns preserve an existing definition so drift fails
verification. Migrations 1–66 are unchanged. `CREATE TRIGGER IF NOT EXISTS`
requires MySQL 8.0.29 or later; qualification targets MySQL 8.0.46 and MariaDB
10.11.14. See the [MySQL statement documentation](https://dev.mysql.com/doc/refman/8.0/en/create-trigger.html).

The private writer preserves the admitted receipt and all 76 values, rejects a
second receipt for an existing logical key, compares immutable configuration
evidence and keeps every inactive family field NULL. Zero is present data, not
an absent field. The bootstrap compatibility reader needs visibility of trigger
metadata; the dedicated telemetry writer/report roles gain no DDL authority.

Independent incident schema 6 includes families 1–13, with mask 16382. A verified
post-fix reference must match an actually committed receipt, producer, kind,
occurrence and environment/season scope. Schemas 1–5 stay sealed. The template
CLI still defaults to schema 1; select schema 6 explicitly for control review.
This inventory does not activate definition-7 control reports.

New outage journals use `DMSTLJ05`; existing versions 1–4 remain read-only
compatible inputs. Kind-13 detail uses ordinary bounded queue capacity, and
cannot consume the protected lifecycle reserve. Control-only loss does not
alter kind-11 contribution coverage.

## Focused local checks

```sh
python3 tests/async/test_telemetry_battle_contribution_contract.py
python3 tests/async/test_telemetry_battle_contributions.py --sanitize
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_repository.py
python3 tests/async/test_telemetry_outage.py
python3 tests/async/test_telemetry_runtime_integration.py
python3 scripts/telemetry/incident.py --template --registry-schema-version 6
```

Using only the explicit disposable loopback SQL fixture described in
[BATTLES.md](BATTLES.md), run:

```sh
python3 tests/async/test_telemetry_control_storage.py --sql-fixture
python3 tests/async/test_telemetry_repository.py --sql-fixture
python3 tests/async/test_telemetry_battle_runtime_sql.py --sql-fixture
```

The storage check exercises the full immutable chain, exact fields, all 455
payload columns, independent reviewed loss, role isolation, logical uniqueness,
reruns and schema/trigger drift/restoration. Native SQL qualification compares
actual gameplay/runtime/queue/worker/private-writer values with committed rows.
Fixtures do not qualify a running personal server, authenticated login,
gameplay/save readback or measured game-loop/save performance. Those remain part
of the final personal-local gate.

Both full-chain MariaDB 10.11.14/MySQL 8.0.46 storage, private repository and
native runtime/queue/worker/SQL journeys pass. Native readback preserves all 76
fields for the 56-resolution fixture and explicitly partial state coverage.
Existing battle and build publications, private roles, bounded reports,
rollback, lost acknowledgements, corrected reviews and old immutable generations
remain qualified. The 164-test focused Python suite, fresh ASan/UBSan pure and
gameplay runs, lifecycle/outage/exhaustion, header contracts, formatting and the
maintained server build pass. TSan remains unsupported from its earlier probe.

The PR base has independently advanced its migrations. Integrating both sealed
histories and qualifying their common runtime schema remains required before
merge; the owned 67-step proof does not establish that merged schema.
