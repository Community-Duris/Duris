# Independent lifecycle V2 reader: primary integration - 2026-10-07

The immutable flatfile lifecycle reader now accepts the published DURELR V2
contract and verifies retained room-pile holdings alongside wallet/bank mappings.
It preserves historical V1 framing and existing command, plan, receipt and common
root proof. It never recaptures current piles or installs/activates accounting.

Before import, primary inspected the maintained frame implementation: only V1
was allowed for DURELR, while the lifecycle reader required zero item rows and a
wallet/bank-only EAB witness. This is direct source evidence of the missing V2
reader requirement, not a locally executed native red fixture.

Primary imports exactly four reviewed code blobs and the authored report from
Plan5 commit `018cb09f2c5850641d18ed8bcaf2c0c9b0befa16`:

| Path | Git blob |
| --- | --- |
| `scripts/qualify_flatfile_economic_authority.h` | `593272f3ab668e970c0f561deed60cb3ba11a232` |
| `scripts/qualify_flatfile_economic_lifecycle.h` | `8f82d0dc30f3e1cf55ef4e0f37484fe18dcef3e3` |
| `tests/async/flatfile_lifecycle_v2_fixture.cpp` | `d139ffed657514fd5d9f33e476f1f322ec319c90` |
| `tests/async/test_flatfile_lifecycle_v2.py` | `90ee6a829bc5a2e0d0e3e64c7404b3e79b10c070` |
| `docs/persistence/economy_accounting/PLAN5_NATIVE_LIFECYCLE_V2_2026-10-07.md` | `822510f6d2390f04a886dd9e09789a1d9098d50f` |

Independent source review found the component dependency closure complete on
primary: existing baseline item/forest APIs suffice. The authority header also
retains older additive pending-player-domain and mapping accessors carried by
that exact peer blob; older full-operator dependencies are not imported here.
The peer later reached `a5dac7db92e4e251145f0eb7f2a030f015d8a246`; its locker
and shopkeeper slices need their complete older audit-provider closure and are
separate unfinished integrations. No peer branch merge/cherry-pick occurred.

## Actual primary checks

All three changed C++ files pass repository changed-line formatting, and the
new fixture passes strict C++20 syntax (`-Wall -Wextra -Wpedantic -Werror`).
Python driver AST parsing passes; it is not a runtime case result.
Normal `scripts/validate_economy_accounting.py` passes, exit0, 5.547 seconds.

The original new component command ran once with its original native sources,
ASan/UBSan, no PIE, GC and crypto/pthread flags, original600-second build and
45-second case budgets. It failed at link, exit1, 64.094 seconds:
`/lib/x86_64-linux-gnu/libm.so.6` and `libmvec.so.1` are missing from the WSL
link closure. No runtime coverage/envelope/EAB case ran. No flags, source list,
provider stub, budget or environment was repaired, and no failed full-server
link was retried. The known missing-library blocker is preserved, not waived.

Exact argv, terminal stdout/stderr hashes, elapsed times and source-check receipts
are in `bin/tests/plan5-lifecycle-v2-integration-primary-20261007/RESULT.json`.
Pinned preimages and direct source failure witness are in
`tmp/plan5-lifecycle-v2-integration-primary-20261007/PREIMPORT.json`.
The [peer report](PLAN5_NATIVE_LIFECYCLE_V2_2026-10-07.md) describes its65
coverage cases, five modeled frames, historical suites and native EAB roundtrips.
Its external executable receipts are unavailable locally and are not authenticated
primary results or evidence for this narrower mixed dependency candidate.

No `src/` or migrations change. Native tree
`833d3085815b396861ad18a77635412212381e4b` and migration tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` remain unchanged. Protected SHOP
harness WIP and the unrelated untracked report remain byte-identical. The private
production source archive excludes these independent scripts/tests; it does not
qualify this whole checkout. Original native V2 encode/decode/install retry/fault
fixtures, current combined builds, complete census, activation and release remain
open. No original suite is replaced, no gate or926 writer policy is changed,
and inactive accounting/declined spell behavior remain intact.
