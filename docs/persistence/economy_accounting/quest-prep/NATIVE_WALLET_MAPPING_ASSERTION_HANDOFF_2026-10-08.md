# Optional live native-wallet captured agreement — 2026-10-08

Added an explicit, optional join consuming the accepted mapping/head capture
inputs for paid QP02, pre-D original A and live replacement B. It compares a
selected live native image, its original birth row links, an explicitly observed
encoded wallet key, the selected mapping lifetime and the current lineage head.
It returns **captured agreement only**, with owner authentication and world/
publication proof still required. This delivery is modeled unit evidence, not
SQL, native journey, owner, journal or publication qualification. The continuing
native Goal remains BLOCKED.

## Exact bundle and candidate pins

| Pin | Exact value |
| --- | --- |
| Preserved prep parent / accepted mapping capture handoff | `9905d55da42a12b916e0f2362de441b569365d66` |
| Optional helper and focused tests | `1945e9152b460d759669b55c3485f7643ccc64c5` |
| Fetched public primary / source verification | `fb641bb56ae719758756d4bd1a575df48fbe5ef7` |
| Previous public primary, same source/migrations | `20510d07da21759396bbe8775150f1d9aafc4233` |
| Public source / migrations trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| New helper Git blob | `db1c18a23b5e9b81a90f40322683b88b03c86ea3` |
| New focused test Git blob | `d5c271ce1f90434862f1b2903489a0be159395fa` |

The code commit adds only:

- `tests/async/quest_accounting_prep/native_wallet_mapping_checks.py`
- `tests/async/quest_accounting_prep/test_native_wallet_mapping_checks.py`

The next commit adds only this handoff. Its exact SHA is the containing commit,
available through `git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/NATIVE_WALLET_MAPPING_ASSERTION_HANDOFF_2026-10-08.md`
and reported with remote publication. Import only compatible owned bundles.
No accepted capture/cost/temporal/pair/default oracle, production source, shared
driver, schema, registry, finish plan, canonical HANDOFF or Plan 5 file changed.
No primary adoption is inferred. Earlier live/component evidence and failed fee
attempts remain intact. Private primary source/NBC4/NMB4 candidates are unavailable.

## Source trace and existing-helper overlap

The following exact public blobs were read and retained. Each was also compared
byte-for-byte between `20510d07da21759396bbe8775150f1d9aafc4233` and
`fb641bb56ae719758756d4bd1a575df48fbe5ef7`:

| Maintained source | Git blob | Predicate used or boundary preserved |
| --- | --- | --- |
| `migrations/economy_accounting.sql` | `a1d65bc8dc54d4426b6a8e3f7687636fd75fd7db` | Full mapping lifetime and lineage-head columns; unsigned IDs/revisions; nullable active epoch and retirement. No current-balance table is introduced. |
| `src/persistence/quest_mobile_published_world_sql.c` | `a42ec60cba96cfb2a7ddb355295a7bb10c714f67` | `observe_mapping`: exact mapping ID/lineage, wallet kind1, context12, backend1, locator7, native/active IDs, creator, NULL retirement and revision0. |
| `src/persistence/economic_sql_native_mobile_birth_transaction.h` | `5c147355456f7cb38f6989cfc7c917c79933f937` | Locator7; LIVE-only wallet lifetime; historical birth epoch is distinct from current cash/epoch. Real same-session owner borrow is external. |
| `src/persistence/economic_sql_native_mobile_birth_transaction.c` | `9e7d0f0eab19279a8f2917a0b05ba34b17d214d0` | `mapping`/`create_mapping` and `economic_sql_native_mobile_birth_lock_wallet_lifetimes`: exact original creator/account identity, active native lifetime and zero mapping revision; progressed current cash/stock permitted. Authentication, locks and published origin checks are not reproduced. |
| `src/economy/native_mobile_birth_accounting.h` | `ef2e8cf1c2805a9fcc695d8a5dabf0e16211ab2c` | Wallet authority is the original SQL mapping lifetime, not the mobile UID; context is the native custody type. |
| `src/item/item_transfer_command.h` | `f1d8a67d565133eeb45b50135dd82af82f50c439` | Native custody type/context12. |

The generic reader-test mapping builder defaults to locator12. That fixture is
not native policy. The new modeled rows explicitly use the maintained locator7.
No accepted fixture or capture helper was changed to conceal that distinction.

Existing paid/temporal helpers already check item/cash/root/source transitions
and explicitly leave mapping authentication external. Neither consumes selected
mapping/head rows. The new helper fills that literal consistency gap without
copying their transition assertions or the production authority resolver.

It reuses maintained `reconcile_economy_accounting.account_key`,
`quest_cut_checks.bind`, `rows`, `row`, `index`, `mobile`, and the accepted
reader's `watched_mapping_ids` bounds. `mobile` calls the complete maintained
`economic_restore_evidence.decode_native_mobile` grammar and refuses unknown
historical cash. No copied origin codec, wire decoder, SQL/session/lock resolver
or receipt validator was added.

## Explicit optional contract

```python
from native_wallet_mapping_checks import assert_live_native_wallet

agreement = assert_live_native_wallet(
    actual_captured_cut,
    original_instance=observed_selected_native_instance,
    native_wallet=observed_encoded_wallet_key,
)
```

No existing assertion opts in automatically. `original_instance` identifies the
original lifetime of the selected actor; it may select replacement B when B is
live. The helper requires native-shaped cut metadata and the optional reader's
explicit unauthenticated mapping-observation marker. Existing metadata pin
grammar is reused; it cannot authenticate an asserted binary/source pin.

It validates bounded/sorted/distinct watched and missing IDs, exact correspondence
between watched/missing/returned mapping IDs, selected native observation and
duplicate row identities. Mapping IDs use the schema's positive unsigned64 range,
including UINT64_MAX; native IDs retain their distinct reserved-sentinel limit.
Numeric equality of mapping ID and native UID is allowed and conveys no proof.

Only the selected mapping's lifetime is joined: wallet/key/cut lineage, kind,
context, backend, locator, native/active IDs, original birth creator, NULL retiring
operation and integer revision0 must agree. The current head must match the cut
lineage/current epoch, have an unsigned revision and have its captured epoch row.
A missing/NULL/different head refuses this live join. Head revision0 or UINT64_MAX
is not invented into a refusal; mapping revision is independently fixed at0 by
the maintained live predicate.

The selected current native image must pass maintained full grammar/header
binding, be LIVE and have known cash. Mapping creator, image birth/source,
captured original birth operation lineage/source and opaque origin row ID/birth
link must agree. The captured birth epoch must be a valid operation identity;
an older birth epoch remains valid and is returned separately from the current
head. Current mobile/stock/cash clocks are not equated with mapping revision or
original birth clocks. Origin payloads and receipts remain opaque; this helper
does not validate their bytes, statuses, financial evidence or authenticity.

Unrelated foreign/retired mapping rows and explicitly missing unrelated IDs stay
literal observations. They are not filtered, repaired or promoted into a complete
lifetime census. A selected retired mapping/native refuses; no terminal/D
contract is inferred. The helper never mutates the input or changes proof labels.

Returned values include the observed mapping/native IDs, lineage/current epoch/
head revision, historical birth epoch/birth operation, current cash/revision and:

- `scope="captured live native-wallet agreement only"`
- `owner_authenticated=false`, `world_publication_proven=false`
- `retirement_scope="unsupported"`
- Exact external requirements: original-owner birth command/retained receipt/
  carrier/lineage/mapping authentication; actual same-cut reconnect-disabled
  IN_TRANS owner borrow/lifecycle exclusion/mapping-native locks/current cash;
  complete native world/stock/custody and physical publication/hold/ACK proof.

## Narrow qualification and evidence

Windows Python3.12.10, D: TEMP/TMP and no bytecode. Exact focused command:

```powershell
Set-Location 'C:\Users\alexa\.codex\worktrees\accounting-quest-prep\NewDuris Max'
$env:TEMP='D:\Dev\Temp'
$env:TMP='D:\Dev\Temp'
$env:PYTHONDONTWRITEBYTECODE='1'
python -B tests/async/quest_accounting_prep/test_native_wallet_mapping_checks.py -v
python -B D:\Dev\Temp\quest-native-wallet-join-20261008\qualify.py
python -B D:\Dev\Temp\quest-native-wallet-join-20261008\seal.py 1945e9152b460d759669b55c3485f7643ccc64c5
git diff --cached --check
```

**14/14 focused tests PASS**, exit0, final retained unittest run0.065s; external
controller deadline60s. Staged whitespace checks passed. No DB/client connection,
server, C++ build, migration, native journey or unchanged broad suite ran.

Tests reuse the accepted paid-QP02/temporal-QP03 builders. All three paid contracts
at distinct/equal numeric mapping IDs demonstrate before/after additive joins;
the existing paid result remains equal before and after adding observations.
Pre-D A and live B join independently, with A's retired row retained literally
in B's cut; the existing temporal result also remains equal. These are modeled
oracle demonstrations, not actual gameplay or SQL results.

Independent controls cover explicit key grammar/kind/context/lineage/ID;
native selection types/bounds; mapping presence/duplicates/metadata; all11
selected mapping fields; bool/float/string/NULL/overflow values; absent/NULL/
different current head and epoch; older/current birth epoch; source and origin
row links; missing/duplicate/malformed/native-header disagreement; valid v1
unknown-cash refusal; nonlive image refusal; no mutation on success or rejection.
A coherent forged key/mapping/head/epoch packet agrees while returning all
authentication/publication flags false and preserving opaque fake carrier/
receipt strings. Agreement cannot establish original-owner evidence.

Evidence: `D:\Dev\Temp\quest-native-wallet-join-20261008`. The receipt records
six exact public blobs, thirteen byte-for-byte unchanged helper/builder/decoder
dependencies, command/runtime scope and before/after source hashes. The seal
proves committed bodies equal tested bodies. `artifact-index.json` hashes12
payload files (index additional), including source excerpts, stdout/stderr,
modeled input/result, qualification/seal scripts and receipt.

| Artifact | SHA-256 |
| --- | --- |
| Committed/executed helper | `b20efe1cc2339c514baea28113b07cfb1f9e53ec2df13e0fb3e89326c7588ecf` |
| Committed/executed focused test | `1b9b012691bfb8b10cbda4509d75dd39db2cdca6d8f528629220688866611589` |
| `receipt.json` | `0e6cc81b23216427d9ac4e3486f63e5f053b50d42db101bd3c5cf63130735698` |
| `artifact-index.json` | `06dbc831247f4104838a06bf05a6aee7af85047e8883214739df079e3af268cf` |

## Remaining integration boundary

This optional join is reviewable without a new production hook. The accepted
capture reader must be present; the helper requires its nonempty observed mapping
inputs. It supplies no authenticated same-cut borrow, funding/held setup,
physical publication, ACK, D/retirement/cold execution or private source access.
These remain concrete integration dependencies. Replacing them with mutually
consistent rows would invalidate the acceptance claim.

The bounded preparation is complete at this handoff. Dependent native execution
remains blocked. Preserve the agreed major-batch gameplay cadence; final native
qualification must use the actual integrated primary candidate. Do not resume or
complete the continuing native Goal based on this unit result.
