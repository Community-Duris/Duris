# R8 ordinary enhancement cascade handoff — 2026-10-07

Implementation `7f0d11b4def2ed53b69b7fbbeeb9b102540688db` plus the narrow
compiler initialization fix `9f5119fc3b6d0f345f639da9e7d5630fbc9f76a5` is
published and qualified at declared component, maintained-build and current
import/module scope. Coordinator approved the exact three-file reservation at
accounting `f5216feff44a4620715dafcbec60829cc9e14a9a` in
[R8_ORDINARY_SEARCH_REVIEW_2026-10-07.md](https://github.com/Community-Duris/Duris/blob/f5216feff44a4620715dafcbec60829cc9e14a9a/docs/persistence/economy_accounting/domain-separation/R8_ORDINARY_SEARCH_REVIEW_2026-10-07.md).
Final declared-scope review is pending. This worker bundle R8 does not complete
original R8 or the continuing project. Actual continuing Goal remains ACTIVE/no
budget, createdAt1791384853, with the full Plans1-5/applicable original R1-R8
integrated implementation, supported gameplay/persistence/recovery, resolved
required blockers and owner completion disposition finish line preserved.

## Connected search control and native observation points

`src/economy/enhancement_original_search.h` owns the complete ordered cascade:
exact-only step0, two directions on later steps, candidate arithmetic and bounds,
failed-whole-step accounting and first-success stopping. The local synchronous
provider in `src/item/enhance.c` supplies fresh configuration at each original
condition/probe and retains the actual linked hash walk, live next entries,
current value/wear, fresh source VNUM, native read and original robj carrier.
The header stores no object pointer, catalogue, callback or ownership capability.

Strict >maxsearch remains, including an initial step for zero/negative budgets
when max_roll permits it. All-invalid probes still consume a whole-step budget;
negative roll permits none. Failed reads can change subsequent direction, roll,
cap, links or source VNUM. Captured wear and maxsearch remain captured. Duplicate
candidate probes after direction changes remain ordered and are not deduplicated.
Native int64 arithmetic retains the original native-int-sum input range.

The exact three implementation files are the new header, enhance.c and existing
`tests/async/test_ordinary_enhancement_payment.py`. Preconditions, fees/quote
assignment, RNG/gains/messages, object lifecycle, debit, effects, cleanup, active
refusal, receipt/publication/ACK and recovery retain native owners. The later
`cost = 0` declaration fixes GCC's strict maintained-build warning across the new
provider call; the quote guard and true fee assignments remain unchanged.

## Original/extracted executable controls

Complete original and extracted ordinary producers PASS the same existing R3/R6
numeric, affect, payment, pouch and refusal assertions plus **23 search scenarios**.
The actual header and producer are used with unchanged C++20 -Wall/-Wextra/-Werror,
-O1/-g, ASan/UBSan/no-recover, no-PIE and30-second execution limit. Exact ordered
hash probes/native reads and wallet, debit, publication and retirement outcomes
are asserted. The original source is the pre-R8 source at5a299035f; final generated
original/extracted CPP and final rerun logs are retained.

Cases include exact once; both directions; strict0/1/-1 and all-invalid budgets;
low/INT_MAX/cap+roll bounds; collision, wear and same-VNUM skips; unreadable
continuation and first success; failed-read config/next-link/source-VNUM changes;
captured wear/budget after mutation; negative roll; and refused payment cleaning
the selected output. Coordinator independently executed final original/extracted
controls and matched all three source hashes at9f. This is controlled component
proof, not a genuine native birth/publication or gameplay journey.

Adjacent material21, all-stat, stat/config, superior/essence payment, pool and
all four active paid refusal checks PASS. The module boundary contract FAILS the
same untouched world/db.c reset-count assertion in both original and extracted
runs; both failures are retained. It is not relabeled as an R8 regression or PASS.

## Maintained builds and retained initial compiler failure

The initial SQL build at7f exited2 under the unchanged strict -Og/-Werror flags:
`cost may be used uninitialized`. Fail-fast stopped before the flat build.
`build-sql-initial.log`, `build-RESULT-initial.json` and the initial terminal record
preserve that failure. After only the declaration initialization at9f, final
maintained SQL and flat builds PASS, controller exit0. Each recompiles one actual
enhance.c provider and fully links the existing **740-object** owned graph.
Actual ELF files and terminal logs are retained and authenticated:

| Backend | Final ELF SHA256 |
|---|---|
| SQL | `7a7fa9d729fa6d5587cc79bcbe45a1401c999aed302ffe41e3c59bea08d6e36f` |
| Flat | `bb916fc01ad149c6c595558c4ca954df3cca7f52056048239755bda213b9a9da` |

Final original/extracted ordinary controls were rerun after initialization and
PASS. Formatting and committed-delta whitespace checks PASS. No flags, assertions
or timeout were weakened. Earlier R7 ELF records remain separately retained.

## Current import and actual dependency qualification

Final range5a..9f is checked against immutable primary
`f5216feff44a4620715dafcbec60829cc9e14a9a`, which includes the coin-owner fixture
and approved R8 boundary. Production-only delta applies to **bare primary**;
complete test package applies on **R3 plus R6 alone**, with no R4/R5/R7 prerequisite.
No primary checkout or adoption is changed. Coordinator independently checked the
same production-only patch and bare tree. Exact final patch hashes:

| Patch | SHA256 |
|---|---|
| Complete three-file range | `7b4e09e6937bff2f15901d92300d2d1117c3c1dbb7b3ff05316a3e499fe7330a` |
| Production-only range | `50844d917e467b9d36208dd8970b9cf145d2cb5a49059019bbcd82d98a849506` |

Three immutable exports were executed independently. Current+R0-R8 and minimal
current+R3/R6/R8 each PASS the actual ordinary runner and SQL/flat full enhancement
module checks. Bare current+R8 production PASS SQL/flat full module checks only;
its absent optional test prerequisites are not silently supplied.

| Export | Git tree | Files | Archive SHA256 |
|---|---|---:|---|
| Current+R0-R8 | `f3761cbd20293ee5d47f39c657e8a2cb1baccfd9` | 6451 | `35512702f32372010c0479be8c82a1d3de8a497b5290ac6992ebacb46d6a3eba` |
| Current+R3/R6/R8 | `af9bedf84bbff08ba62efa6a4ea119a404790bd0` | 6444 | `ad2f2e1f734a50eea7103502d01abad6fe5e037a71015911e7bf41a5844bdbef` |
| Bare current+R8 production | `f181a0d2fc212aa00933559ef6a85e4755fe732b` | 6442 | `ba805de68f147edc949c1d40c6e64a6430f355f20bcc27d61dad503dcd6ba259` |

Verified Docker image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`
uses read-only exports, writable isolated bin volumes and no network. No DB,
player service or game server is started. These are component/module checks;
no current754-object full build or native gameplay/persistence/recovery is claimed.
Later primary8e4d77c945 is coordinator plan documentation, not a new source proof.

## Exact source and proof pins

| File | SHA256 |
|---|---|
| src/item/enhance.c | `ec5de67a5f4e6dadcab09d5a4e998828e466baae2ac22015629562ff08c7ff8e` |
| src/economy/enhancement_original_search.h | `9630b33e1fcbdd813a76341bd458bb6cc297ee49b9f70758e3a5430918c4b805` |
| tests/async/test_ordinary_enhancement_payment.py | `2e215e0676f533ac0d169a924b956438e2840582654ed62f750c93096ed90937` |

Ignored `bin/tests/domain-r8-20261007/proof-index.json` authenticates **46 proof
files**, including actual final ELF binaries, generated CPP, old/new component
logs, initial failed/final successful builds, source pins, build-graph counts,
current/prefix/bare results and container inspection. Export archives and private
Git indexes are separately pinned above and excluded from the46 count. Earlier
11-file original feasibility proof remains in domain-r8-feasibility-20261007.
No proof artifacts, credentials, logs or binaries are committed.

The full continuing finish line remains open. No adoption, supported native
outcome, activation, deployment or integrated completion is inferred from this
handoff. Final declared-scope review and the evolving nine-family assessment
continue after publication.
