# R13 complete shop item acceptance handoff — 2026-10-07

Implementation `835e70e1d9c35d56c2254ca65cbc306c7e60fc81` is published and
qualified at original/extracted actual-caller, maintained build and independent
bare-current import/module scope. The amended exact three-file reservation at
`dfd78bea24632ba94951bdc3645ccc814cf5f6b1` was approved at accounting
`7c8b665b735955ab060e76a6fddbf1903063ddb8` in
[R13_SHOP_ITEM_ACCEPTANCE_REVIEW_2026-10-07.md](https://github.com/Community-Duris/Duris/blob/7c8b665b735955ab060e76a6fddbf1903063ddb8/docs/persistence/economy_accounting/domain-separation/R13_SHOP_ITEM_ACCEPTANCE_REVIEW_2026-10-07.md).
Final committed code/import/artifact and canonical delivery review follow this
publication. R11 and R12 remain closed at their declared scopes; this optional
package requires no R0-R12/prep ancestry.

The continuing Goal remains ACTIVE/no budget through original primary Plans1-5
and applicable original R1-R8 integrated completion, supported gameplay,
persistence and recovery, resolved required blockers, a published owner completion
disposition consistent with evidence, and no outstanding selected work/review/
handoff. Primary adoption, current754/native journeys, the full finish, deployment
and activation are not established by this extraction.

## Complete policy and native ownership

`src/economy/shop_item_acceptance.h` owns the complete original trade_with
classifier. It preserves native int counter/result, char repairing, cost<1 refusal,
native NOSELL/TRANSIENT short circuit, sentinel/type search, current repeated field
and configured-type reads, empty wand/staff rejection and armor-to-worn acceptance.
Nonzero repairing, including negative char, bypasses NOSELL; TRANSIENT still
refuses. Negative charges retain the original behavior. First sentinel and first
matching entry retain their precedence. Reverse worn-to-armor acceptance, new
bounds/null guards, normalization and a new malformed-input policy are not added.

An exact matching non-dead type calls the keyword evaluator once at the original
point. Both true and false return OBJECT_OK in the original source. This behavior
and the callback are preserved, including native diagnostics. Query mutation of
cost/type/charges/config does not trigger revalidation after the query. Empty
wand/staff and armor-to-worn branches skip the evaluator exactly as before.

The unchanged public `int trade_with(P_obj, int, char)` wrapper constructs one
local synchronous borrowed provider. Its methods use actual native int cost,
unsigned-long IS_OBJ_STAT flag tests, ::byte item type, int value[2], current
SHOP_BUYTYPE and the whole real
`evaluate_expression(item, SHOP_BUYWORD(shop_nr, index))` int callback. It does
not capture an early config/keyword snapshot. The classifier stores no native
pointer, provider, callback, config or authority and introduces no allocation,
suspension or service lifetime.

The complete real get_selling_obj selector remains byte-identical: native visible
lookup, classifier call, pointer result and refusal/dead-message switch remain
native. All six parser/stack/operator helpers, operator strings, extra_bits
metadata and every other native body remain unchanged. Admission, original
lifetime/custody, payment/effects, physical publication, completion, ACK,
persistence and recovery retain their native owners. This transient classifier
result does not grant any of those capabilities.

Exactly three maintained files change: the new header, one include and complete
trade_with replacement in shop.c, and the first two-component direct runner.
Restoring the original wrapper and removing its include recovers the complete
owned preimage byte for byte. Both frozen PRELUDE/DRIVER pairs are byte-exact.
The inverse observation mapping preserves native rule tokens/order; redundant
item-pointer parentheses are the only normalization. Disabled historical comments
are omitted from the new header. Existing runners and every public signature,
schema, metadata/parser body and native authority boundary remain unchanged.

## Original and extracted executable evidence

The [amended reservation](R13_SHOP_ITEM_ACCEPTANCE_RESERVATION.md) records two
separate original feasibility components. Controlled-query38 retains its immutable
19-file index `1a3e84d8d3f78052513e1edc8117d5173b6ac24599bd6562f0131cff327d512d`.
The real-keyword54 supplement retains its separate27-file index
`8fa8c12ecd12f1c900394508f71b830aa6c825aa3ef24722745ce399111c6a50`.
Both profiles and actual original compiler closures were independently reviewed
before approval. The supplement's first private HUM/NOSHOW metadata expectation
failure remains retained; final original evidence uses the actual NOSHOW row with
the same int2 expectation. No production parser repair is inferred.

The maintained runner preserves BOTH complete frozen components. Controlled38
executes the actual classifier and selector with a controlled keyword endpoint.
Real54 executes all eight native functions: push, topp, pop, evaluate_operation,
find_oper_num, evaluate_expression, trade_with and get_selling_obj. It extracts the
actual operator_str declaration and complete canonical extra_bits table. Only
name lookup, visible lookup, log capture and formatter endpoints are controlled;
the real evaluator is neither replaced nor wrapped.

The private paired original run substitutes only complete original native shop
functions from the retained owned preimage. Current metadata/parser source and
both fixtures remain exact; the new header is unused by the original classifier.
Original and extracted actual maintained main each execute38 controlled and54
real cases under baseline and -Og. Each variant has EIGHT compile/runtime exits0,
with C++20/Wall/Wextra/Werror/debug/ASan/UBSan/frame-pointer/sections/GC and original
compile120/runtime30 bounds. Only the supplement adds -Og. No assertion, native
body or individual compile/runtime deadline is relaxed.

Controlled cases cover cost/repair/flag/type/charges/query results, repeated
config search, first-match/sentinel, one-way armor compatibility, query mutation,
selector pointer/message modes, lookup mutation and early refusal. Real cases
cover fourteen expressions against evaluator, classifier and selector with ordered
traces, native false/diagnostic results, NOT/AND/OR/parenthesis/stack behavior,
actual GLOW/NOSHOW low-bit int results0/1/2, dead/armor skips and query mutation.
The legacy `sword)` true result with an illegal-expression diagnostic is retained.
Bounded supported parser paths are qualified; genuine world names/visibility,
native object lifetime/custody, malformed config/null-item UB, oversized tokens/
stacks and high-bit shifts are outside this evidence.

Eight owned adjacent checks pass: complete R12 sale quote, purchase usability,
multi-buy, ship listing, issue552 local shop, secondary keeper binding, command
codec and shop runtime. The live-route contract fails identically on original
and extracted inputs: `object->loc.carrying == keeper` is missing. Both logs remain;
the separately owned quest-prep repair is not borrowed. Changed-line formatting,
whole NEW-header formatting and git diff --check pass. No R13 compiler/runtime
failure required source or fixture repair.

## Maintained builds and exact independent imports

Both strict `make -C src -j2` recipes pass at835: mariadb11.440s and flatfile9.635s,
each with the original900-second bound and distinct output. Actual logs contain
one shop.c compile and740 unique linked objects per profile. Retained/copied native
ELFs authenticate below. These are owned740 builds including earlier owned work;
they do not establish current754 composition or genuine native backend journeys.

Exact full three-file and production two-file patches from dfd to835 independently
pass cached check and actual apply onto freshly fetched primary7c8. All6454 full
and6453 production tracked blob bodies authenticate against their actual export
Git trees. The patches exclude prior R10/R12 work. Consequently owned and bare
whole shop.c hashes intentionally differ; new header/runner and generated extracted
CPP for BOTH components are identical between owned and bare-full inputs.

Separate full/production containers bind complete exports read-only at /workspace,
with isolated writable bin, network none and image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`
(GCC13.3). Both complete shop modules pass SQL and flat syntax qualification.
Actual dependency manifests contain462 SQL and456 flat inputs per export.
Every input is reread/authenticated inside its container; all87 actual export
source/header inputs per profile also authenticate against exported bodies.

The full export executes the actual maintained runner:38 controlled and54 real
cases under BOTH profiles, all eight subprocess exits0, plus five adjacent runtime,
multi-buy, ship-list, issue552 and secondary-keeper checks. All pass. Production
intentionally omits the runner and proves both full-module type profiles. All
component/build/bare supervisors and children are terminal; retained idle
containers are not active jobs.

## Exact final-review artifacts

Private evidence is `bin/tests/domain-r13-20261007`. Its flat-map91-file
SHA256/byte-length index includes committed source copies, both components'
original/extracted/bare CPP/binaries/results/logs, paired old failure, strict
build logs/ELFs/graphs, import results, both bare proof sets, actual compiler input
manifests and container records. Index SHA256:
`261572b3ada7f74dcb729659c0efd313494fb27ac8bdb541caba8aa4210fc5a0`.
Archives, temporary Git indices and export directories are authenticated separately
by archive/tree/all-body pins. One private sealing attempt failed because Windows
Path.is_absolute misclassified Linux /usr paths after remote authentication; the
controller uses PurePosixPath for that filter. That failed attempt remains recorded.
No native source, fixture, flags, assertions or deadlines changed.

| Artifact | SHA256 |
| --- | --- |
| Owned src/economy/shop.c | 8a5aeb634aa5b635761f438698b4191aad06ab9ff299571b8777de986d34aed1 |
| Bare full/production src/economy/shop.c | 3bfa5ae74a560a4b4c9204d862e22f9b29a2c79bad17628a4359a3ebdea29c98 |
| src/economy/shop_item_acceptance.h | cf97ce200bfe84d7a247fd3a368680e40e60e37c5ec393d2a026e8e4ffbffcf9 |
| tests/async/test_shop_item_acceptance.py | 6681d514e0fd89aee3a3d454d52f366ea8ff223669c76911fa90aa200329db82 |
| Complete owned preimage | bdfe9e5786bd6a1441fbd3b0c5dfedc0e0b7b97bef84ea3dd7225e8871dc87fc |
| Paired original controlled CPP | 668a1c3a81846d73a9f2a5e80b2d22dbca7014876ee5d7e0e5b1c1f2ce4712df |
| Paired original real CPP | bba7ceb8d2d88bddd35fa04768ad0160092d170fda61f3aee596a75c7270e62e |
| Owned/bare-full extracted controlled CPP | 713cfbe38e39749e02bc5863fe6e42f4567b574d658c69be97b63a1850db024d |
| Owned/bare-full extracted real CPP | c91e3c7937e121f7be1256b88315026710951192aba5e1fd2c0e81c89fcdcf7b |
| SQL ELF,207419480 bytes | e3be0239eb26b5470dfc5cea2cfe95156ad31a2c5bc04630afeb5ba5fba227d4 |
| Flat ELF,178067856 bytes | 775dac41d3943409351850c26fa562a9bcc4771f756549c7d1e07f19cd07aa4b |
| Full patch | c88c8728556528372eb9898fbee88cb77f0c80a754d6e82d5a6a7b49bfc5f87f |
| Full archive | c814a90f54b1b9a2351e464d325f264f194f0979c96e388a322667afd0faa672 |
| Production patch | d7737b9d9601e998d889eb11630a2d0604619c1f234452841f3ebafd6e1ce670 |
| Production archive | 57fa8c2d6560cfaad29fdd4b1b6fef9e2aace2fcd2c791d781fd95ac9450e18f |

Full tree `357afca2ed3278fb4e9cc6cd9bfdbc59131dabe5` authenticates6454 bodies;
production tree `fd09da8e2ff794a5438443403cb5c44bbdd610b3` authenticates6453.
Source-terminal, integrity, component, adjacent, build-graph and import RESULT
files retain actual source/commands/exits and the supported limits. Final
independent committed code/import/artifact and canonical review remain pending
this delivery. The post-R12 owner dependency disposition remains the native
integration baseline; this completed policy does not discharge those owners or
the continuing primary finish line.
