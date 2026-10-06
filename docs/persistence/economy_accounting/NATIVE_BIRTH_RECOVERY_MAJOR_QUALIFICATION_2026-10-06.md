# Native birth and quest recovery candidate qualification — 2026-10-06

The original production MariaDB and flatfile builds both pass on private source
`8c3cea806a83504244d0be47ff76efcbed1057b67f8c0b211fd88af0ce5f58fd`.
The two original affected native codec recipes pass on successor
`401ca7762bac1e6d8f00e41be3b3c512f12b86f04c9e542fb01dda61b4158224`,
including the expanded NQR3 context/pair controls. These are distinct candidates
and scopes. Neither result establishes a complete Plan, R1–R8 or release gate.

## Concrete failures and verified corrections

The first original cold-restoration build (`bf116bc6â€¦`) failed on impossible
negative checks of unsigned fields, unavailable translation-unit UID lookup and
a private procedure-recovery call. The loader repair preserves the actual
pointer/UID lookup and private owner capability, with no warning suppression.

The next original rebuild (`d2fe8ed1â€¦`) reached two concrete compiler failures:
new shop witness helpers expanded a legacy `.virtual` macro on a type that has
no VNUM member, and an unused SQL-only quest child helper was compiled in the
flatfile build under `-Werror`. The reviewed shop helpers now retain the actual
selected shop array slot, including slot zero; they do not invent a VNUM. The
quest declaration and definition now have the same SQL guard as their sole
caller. Original SQL statements, legacy shop parser and macro stay unchanged.

Both repaired production builds complete and link. Their predecessor failed
artifacts remain retained; no failed candidate was relabeled passing.

## Original production build evidence

Command policy remains `make -C /suite/src -j2 BUILD_PROFILE=production
PERSISTENCE_BACKEND=<backend>` with the original DMS and pfile targets. The
original production driver, flags, provider set, backend order and deadlines
are unchanged. The pinned Docker fixture uses two CPUs, 3 GiB, no network,
read-only source and an owned writable artifact volume. All 5,783 source inputs
remain exact before and after execution.

| Backend | Result | Elapsed seconds | Server SHA256 |
| --- | --- | ---: | --- |
| mariadb | PASS, exit 0 | 419.920 | `349bba0ed63f94d1e7f99df612d0e3da2047686aff722facaad50731696bab61` |
| flatfile | PASS, exit 0 | 404.301 | `79172ef9b28ae3e840c71c9a0d229a2a621561b09271631e1d6951e39c9296ec` |

Both stderr files are empty. The private attempt is
`bin/tests/native-birth-major-repairs-and-mechanics-archive-major-builds-20261006-085a6e74df83`.
Its results, source manifest, exact stdout/stderr and binaries remain retained.
All 2,962 owned regular artifacts exported with exact bytes, followed by verified
owned runner and source/artifact volume cleanup. No game/world boot, database
mutation, accounting activation or production operation occurred.

## Original affected native component evidence

Successor401ca composes the source-reviewed actual reducing-container shell
factory and original begin/probe/finish order, NBC3 decision/UID codec and new
NQR3 controls. Its published script/test content matches `d35dbaba2`, preserving
unchanged frozen checkout line endings. Original provider/driver bytes remain;
unrelated SHOP WIP is excluded. All 5,783 inputs remain exact before and after.

| Original recipe | Result | Elapsed seconds | Retained outer log SHA256 |
| --- | --- | ---: | --- |
| `tests/async/test_native_quest_recovery_context.py` | PASS, exit 0 | 57.709 | `f82d71c124e05cc5e4280682b131121b492aca56da04e2c5bb485bd6a4a0d3a9` |
| `tests/async/test_item_native_mobile_recovery_codec.py` | PASS, exit 0 | 55.051 | `69baf72e588ede94d7dd47833655bba289207196be09d49883d4b4a747881073` |

The context recipe links its original 15 actual production providers with C++20,
`-Wall -Wextra -Wpedantic -Werror`, ASan/UBSan, frame pointers, non-PIE and the
original strong-output allocation wrapper. Its six original groups remain;
NQR3 adds last-child fields, literal leaf/canonical round trips, actual pair
correlation and mismatched stage/phase/receipt/cursor/revision/budget controls.
Its original runtime deadline is 20 seconds. The item codec recipe keeps its
original 13 providers, sanitizer/warning flags and 30-second runtime deadline.
No compiler deadline or retry policy was changed.

The private attempt is
`bin/tests/native-recovery-controls-20261006-32943905df81`. Eight owned regular
artifacts exported exactly, and the owned runner and source/artifact volumes
were removed. The context fixture retains its compiler log, native output,
binary and source hashes; its native binary SHA256 is
`691b7d353bdf51f39a7fd63034a9ab28509895aea68ce26d7ff12e7421145a52`.
The original item wrapper removes its temporary binary after execution; its
actual wrapper output and exit remain retained. No binary-retention claim is
made for that original temporary artifact.

These component fixtures use constructed values with actual production codecs.
They do not link the original birth factory/world owner, SQL admission, full
coordinator/journal lifetime, publication ACK or crash/restart authority. The
401ca full production build has not run. Do not extend the 8c production build
result to its subsequent factory/caller/codec changes.

## Remaining implementation and acceptance

The explicit NBC2/NBC3 factory guards and actual alchemist reset-cut/selected
grant/cold-latch integration are independently source-reviewed and included in
51de below. They preserve NBC1 and artifact/template/procedure/reset-tail, clock
and captured constructor witnesses. Compatibility execution is complete at its
component scope. Actual producer/publication/restart journeys remain open.

Remaining original work includes authentic constructor/service and callback
ordering, actual player/NPC/item producer journeys, durable source/receipt and
publication/ACK proof, restart/fault behavior, applicable compound domains and
flatfile parity. Complete writer capture/matrix, guarded activation-owner and
Plan5 release qualification remain required. Inventory or these passing codec
components are not full accounting completion. Maintain inactive behavior,
safety gates and the declined inactive spell-path boundary. The reviewed native source is now installed as an inactive review candidate;
source installation does not establish these remaining acceptance gates.

## Reviewed candidate installed and both production backends pass

Private source51de combines the accepted actual constructor/container ordering,
typed quest recovery, NBC3 alchemist decision/UID retention and cold latch with
the earlier original birth/quest command, journal, SQL and publication owners.
Independent review found no further installation blocker in the inspected
boundaries. The inactive reset branch remains exact; no declined inactive
spell-path change is installed. All twelve new production C providers appear
in the original production Makefile.

The installed source is exactly the candidate's 89 changed/new `src/` paths.
Six original test companions are installed. Two missing original native test
drivers are registered in the current complete regression manifest, preserving
every existing row, providers, cases and runtime limits. Complete inventory
validation passes with 920 tests; inventory registration is not qualification.
The two unrelated SHOP harness edits and the independent Plan5 report remain
untouched. No migration or activation policy is changed by this installation.

Source manifest: `51de87fb263df80e1aba5265f92600d6a6dfa438bc19934a5217142640285b8f`.
All 5,783 inputs remain exact; original build drivers, compiler/link policy,
two-backend sequence and deadlines are unchanged.

| Backend | Result | Elapsed seconds | Server SHA256 |
| --- | --- | ---: | --- |
| mariadb | PASS, exit 0 | 455.540 | `f3727b0eda8018b548218a770f9e36de3ffb7750b2d0f9c0621784fe4ac2232f` |
| flatfile | PASS, exit 0 | 397.609 | `67a81adbe7ccb881ab34d9bffb1476877a17a8eaa6e93c16c50d5a236bc31691` |

The retained attempt is
`bin/tests/native-birth-alchemist-archive-major-builds-20261006-24ad6d6f1892`.
Both stderr files are empty. All 2,967 owned regular artifacts exported exactly.
The wrapper then stopped at source-volume cleanup because two completed owned
component containers still referenced it. After authoritative terminal-state
checks, those containers were removed normally, then the source volume was
removed. Separate cleanup receipts preserve that interruption; it does not
change either terminal build result. Owned runner/artifact volume also removed.

## Original NBC compatibility component

Frozen test packet `ff2c5f2a5029dd0cd8061263f439d73eceac7608990bd430bde66ff5c9c8f42e`
links actual historical and successor production codecs, command builders and
intent providers. Independently reviewed native receipt:
`fe75ce1b1c33f2c27fad0aafb0d3e13f40bb25385752c511b115a4a707010caf`.
Historical two groups and successor seven groups pass with strict warnings,
ASan/UBSan and the original 20-second runtime limit. Native execution takes
0.171/0.373 seconds after 76.572/81.145-second compilation. All 1,302 files per
executed stage remain exact, including the same actual harness.

Six legacy encoding lengths/hashes match; independent literal NBC1/NBC2 goldens
are asserted in both runs. Controls exercise NBC3 decision/UID rules, canonical
grant correlation including moved/nested vial placement, malformed/refrozen
commands and strong-output allocation refusal. Every wrapper truncation refuses
with its old intent; selected refrozen corruptions separately reach deep
validation. C/OpenSSL malloc or process exhaustion are not injected. These are
synthetic values with actual providers, not actual grant/factory or cold-ACK proof.

## Shared journal/coordinator regression evidence

On actual51de the original journal fault and coordinator recipes pass in
6.007/17.363 seconds. The original uncertain-journal recipe failed at link before
running: its three-source prefix lacked newly referenced production codecs and
out-of-scope world capabilities. Original failure and the first corrective link
diagnostic are retained. The corrected private recipe adds nineteen real pure
production providers and exact unavailable domain methods that abort if called.
No success authority, codec replacement, section-GC workaround, changed case,
flag or runtime timeout is supplied. The original embedded cases are byte-exact
after removing only the added linkage declarations/definitions.

Original uncertain-journal cases then pass, exit0, 26.690 seconds including
compilation, original20s runtime. Receipt:
`5b092d0cf3cd14bc4dada4ee1edfc1cd26afc15533d14da061184d969f9cb583`.
This proves the original close/unknown-admission fence; it does not qualify
native world capture, restored player hold consumption or SQL/publication ACK.
The fixture linkage fix is a separate owned issue commit after source installation.

## Scope and next acceptance work

Installed source connectivity and passing production builds do not establish
complete writer coverage or a finished Plan. Writer census/matrix source anchors
must reflect installed owners without promoting backend evidence. Authentic
alchemist/reset and player/NPC/item journeys, original native SQL authority,
receipt/publication/ACK and crash/restart behavior remain the next integration
work, alongside applicable compound writers and flatfile parity. Plan5 remains
independently owned and must meet the same tested candidate for release.

Canonical Plans and R1–R8 do not require exact seed-for-seed global RNG stream
position equivalence with inactive execution. Keep captured constructor substream
and clock witnesses and retain once-selected staged effects; no extra global-RNG
qualification gate is added. All inactive behavior, safety gates and full release
requirements stay in force.
