# Restore operator provider repair: canonical handoff — 2026-10-07

The approved three-provider repair links successfully through the maintained
restore operator builder. Code commit
`1793deb80275f1fba2c6d9a73cebf278f13d54c6` changes only
`scripts/build_restore_qualifier.py`: its `SOURCES` adds
`lockpick_retirement_continuation native_quest_cost native_quest_coin_give`.
The original 59 nodes retain their order; all other AST, flags, options and
stubs are unchanged. The builder now has 62 native source nodes. No native
implementation, test fixture, assertion, manifest or shared authority changes.

The exact qualified candidate is published primary
`a53977fe21105b38f7396d2d08fd295f9822a66d` plus solely that actual committed
one-file patch. This is a compile-only handoff. Native runtime cases executed:
zero. Primary code adoption, final independent review, combined-candidate
qualification, activation and release completion are unclaimed here. The
continuing accounting Goal remains BLOCKED with its original finish line intact;
this finite repair does not resume, replace or complete it. R0–R14 remain closed.

## Approval, exact import and source authentication

Coordinator boundary approval is published at primary
`a38a0c9938bd564162c05501824ed3c8cf74faa4` in
`AUDITOR_PROVIDER_BOUNDARY_REVIEW_2026-10-07.md`. It permits only these three
source nodes and requires the original recipe policies and separate original
failure record. Its builder preimage was authenticated immediately before
composition. The earlier reservation remains in
`AUDITOR_PROVIDER_RECIPE_REVIEW_AND_RESERVATION_2026-10-07.md`, published by
`7d03a836613d4fd079c9c3cd5fb9c76aae034087`, the code commit's parent.

| Import pin | Exact value |
| --- | --- |
| Base primary commit | a53977fe21105b38f7396d2d08fd295f9822a66d |
| Base whole tree | 1dcc7f3c6d80048edee4356bd1db7fb33c7d7d30 |
| Code commit | 1793deb80275f1fba2c6d9a73cebf278f13d54c6 |
| Actual committed binary Git diff SHA256 | 729c5be4c0af7453744ceeaedd7b0095f55cbf1e6a2eecb9e5533291c1842f18 |
| Builder preimage Git blob | 45eec2f2a7dae94f6c3877db840147e11258ad30 |
| Builder preimage, 2501 bytes, SHA256 | 68be6e9b01dc6b5688971c5f29b074b8e16ce642463405e54e09e3a935178f86 |
| Builder postimage Git blob | 471ec432f9185d7b9668fced506060e60ebcc30c |
| Builder postimage, 2575 bytes, SHA256 | 39fb56b06201ec0dbb0ce3c6bfea0eac02cc506c12dd4615ac4b53066d4a1ddf |
| Composed whole tree | 0ead28564fdf61b09d59c5b4dad7b9407cc28ecd |
| Unchanged native tree | 833d3085815b396861ad18a77635412212381e4b |
| Unchanged migration tree | 7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2 |
| Complete composed archive SHA256 | be1f22b7252e814df7c9efcff1d8be762830d7f519f01abad4baee2212394e2c |

An isolated D: Git index reads the exact base tree, checks and applies the actual
committed patch, then writes the composed tree. Its sole difference from the
base is the builder. The actual Git diff differs in diff metadata from the
earlier proposed text patch; its postimage is byte-identical to the approved
proposal. The complete archive binds 6491 Git entries and 256631884 body bytes,
74 more than the base. Every literal Git body/blob, canonical 0644/0755 mode
and four symlink targets is authenticated against the tar. All 6487 extracted
regular bodies were reread before the build, after the build and at terminal
metadata authentication. No private native candidate or worktree WIP entered
this export.

The archive preserves Git modes and symlink targets. The D: NTFS build checkout
does not establish native Linux authority permission/stat qualification. Its
four links concern documentation/world help and are not compiler inputs.

## Actual maintained build and bounded dependency evidence

Evidence root:
`D:/Dev/Builds/Duris/domain-operator-provider-qualification-20261007/bin`.
The complete source export is `source/`; the actual executable and original
command receipts are in `operator-build/`. Temporary paths are explicitly
`/mnt/d/Dev/Temp`. Existing checkouts, build evidence and Docker volumes remain
in place; no environment repair, service change or Docker retry occurred.

The external observer imports the exact exported builder with a non-main run
name and invokes its original `build(fresh_destination)` once. It delegates
the original subprocess arguments and kwargs unchanged. The real command uses
the original operator C++ input and all 62 actual native units, in the export's
working directory:

```text
g++ -std=c++20 -ffunction-sections -fdata-sections -Wl,--gc-sections
    -Wall -Wextra -Wpedantic -Werror -D__NO_MYSQL__ -Isrc -Isrc/no_mysql
    scripts/qualify_flatfile_restore.cpp <62 original-order native units>
    -lcrypto -lz -pthread -o <fresh operator-build/qualify>
```

The full literal argv is retained in `operator-build/RESULT.json` and
`QUALIFICATION.json`; the abbreviated display above is not a replacement
command. This original recipe uses default optimization and no sanitizer
flags. Its subprocess has no internal timeout; the existing 900-second outer
policy is preserved, including source authentication.

| Actual observation | Result |
| --- | --- |
| Original g++ command | Exit 0, 192.460222426 seconds, stderr empty |
| Original builder invocation | 195.245378956 seconds |
| Outer host/controller | Exit 0, 250.485 seconds, within original 900-second policy |
| Actual ELF | 6710928 bytes; SHA256 7cd392038aef04eda56ff99c0d36859385ed2ca2da381bdb7f3a926a98634fa2 |
| Original build failures, timeouts or skipped steps in this repaired run | Zero |
| Native runtime cases | Zero |

Actual `nm` output defines all six previously missing symbols, and none appears
as undefined: `lockpick_retirement_payload_valid`,
`native_quest_cost_projection_encode`, `native_quest_cost_projection_decode`,
`native_quest_coin_give_project`, `native_quest_coin_give_encode` and
`native_quest_coin_give_decode`. Readelf header/program/dynamic observations,
defined/undefined symbols and linked-library observations are retained.

After that one maintained build, 63 dependency-only `g++ -M` probes use the
original flags, includes, source inputs and working directory, one input per
probe. All exit 0. They identify 677 unique compiler input paths: 231 tracked
Git inputs and 446 system inputs. Each body and resolved path is pinned; tracked
inputs also bind their Git blob and mode. The real GCC 11.4.0 Ubuntu 22.04
compiler/toolchain observations bind 13 tool and library bodies, and seven
requested/resolved runtime-library pairs are recorded. All 677 compiler inputs,
13 tool/library bodies and seven runtime pairs were reopened unchanged in WSL.
Metadata host exits 0 after 554.641 seconds, stderr empty. These probes and
metadata reads do not rerun the original link or execute a native case.

All build and metadata jobs are terminal. Host audit independently checks the
complete Git/tar/body composition, exact actual patch, old-node ordering,
unchanged other AST, actual original argv, bounds, ELF and dependency receipts.
No new source or build retry was needed.

## Original records preserved separately

The original investigation packet remains immutable at
`D:/Dev/Builds/Duris/domain-auditor-provider-review-20261007/bin`:
6524 indexed files, 570932023 bytes; index SHA256
`f66151d8f2ad991a820ad061e757a6925a73081b1d4a91a2ed19abaf549f9cc6`.
Its original operator link remains exit 1 with nine surviving references to six
symbols from these three real omitted providers, 204.836412 seconds for the
command and 259.360 seconds for its host. Original stderr SHA256:
`6716497520fdf1d0cfdd1ad714f19896dd364ece1bd11e759badbce7adfb90f7`.
Original operator result SHA256:
`c804f6c4cff752905d7915e8ebd57e9a0565a5ff427245273b9957dc8d59804f`.

The original custody fixture's unchanged GC recipe already links: exit 0,
387.414840 seconds for its command, 439.750 seconds for its host, empty stderr;
actual ELF 50807880 bytes, SHA256
`a3eaa419177630506714d0154826d6394f4ac8e94b162dc45ea53e0accc8a05f`.
Custody's result remains sufficient for that compile boundary. No custody source
expansion or repeated passing build occurred. The new packet's
`original-packet-reference.json` reopens the original index, operator result,
stderr and custody ELF pins; it does not relabel or copy the old ELF as new
proof.

## Sealed packet and remaining owner boundary

The new flat index binds 6789 files and 540794158 bytes, excluding itself.
Its 6790 regular files include all complete source bodies, archive, actual
patch/index, executed private helpers, original build/metadata logs, dependency
receipts, executable, audits and original-record reference. No packet member
will be changed after sealing. Repository publication contains only the code
repair and this handoff, not generated artifacts or private helpers.

| New packet member | SHA256 |
| --- | --- |
| artifact-index.json | ebd28ddc6e4e8fda8e04c52a91f8eeb8098a59418c46e4982c0946b8c1a3a145 |
| source-manifest.json | 80a4a0c2cc520e8376e37e7e6aef55ad89bb65600e4282195a699171a194171c |
| operator-build/RESULT.json | 707c7258fcd6c72dd8b815524d9a4305e23d24771105460b7d991606993e2191 |
| operator-build/host-RESULT.json | b8cd66f26da0f98f3ce044334c3c627de4e0b532af07d8df07d9a043ce771cb1 |
| operator-build/QUALIFICATION.json | f434d91345b236360a49a672f2fc363e8beb2ec9d4776debf1c6ed230841b699 |
| operator-build/compiler-dependencies.json | 1179c7337304bbccc4cf1859920f6c0b5657cfc26dd7cceb9d700c20ea30a0e6 |
| operator-build/runtime-libraries.json | e1a1ddd519ee56b487ce9804a35762c947a023fdadeaff7ef5d6138fdd8dac16 |
| metadata-host-RESULT.json | 78835f7c307ac56e52b9fc00d048bfb6bdf7069fe7b75aa93305551e6260df06 |
| toolchain-info.json | 3a469b79fbaaa15b9f3973061299b671a3b16f26038864c1f2f388688a2866b5 |
| input-reread-RESULT.json | b872644c2872932b21ecbd899c20296aaa39a42173d4e839dbcc20153cfb4e33 |
| AUDIT.json | 8ef9f3b40e6abe73e663f67e9c54a67ab7072d91eeb093954c32d711aaedec17 |
| original-packet-reference.json | 2f1b4a52069a6545a8c5b652d21e8624a07c416fe095a624ef08c51412ec82eb |

This closes the selected three-provider recipe implementation and its original
compile-only qualification. Primary may review and import the exact one-file
code commit independently of this document. Independent final review and
primary adoption remain external dispositions; no acknowledgement-only commit
or additional suite is required here.

The original dedicated custody differential, native-domain, auction, authority,
baseline/lifecycle, backup/restore and major-plan runtime suites remain outside
this finite task. Native lifecycle V2 installation/consumption, mobile/live/reset
census, all writer coverage, gameplay, persistence/recovery, private combined
candidates and the owner completion gates remain unproved here. No full Make,
server, database, migration, deployment or activation occurred. Compile success
does not satisfy the continuing Goal's full implementation/integrated
qualification and owner-completion requirements.
