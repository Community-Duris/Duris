# Plan 5: independent durable custody catalog decoding - 2026-10-07

The owned flatfile audit previously had no independent DUROWN custody catalog
decoder or operator command. The new reader validates that durable format
without calling native repository decoding, recovery or mutation functions.
`--economic-custody-catalog-audit /absolute/private/state-root` reports bounded
aggregate counts after a protected, unchanged read. This establishes the
catalog-reading prerequisite for full independent holdings reconciliation;
it does not complete that reconciliation or release qualification.

Branch/worktree: `codex/accounting-plan5`,
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `cffe05a0352a3d482fe9a2b48bf4861db834252c`. The result is the containing
commit, with its exact SHA and verified remote SHA in the external delivery
packet. All seven previous branch tips remain ancestors. No other worktree
or shared coordinator, producer, contract, migration, registry/matrix,
activation or shared test-runner file is changed.

Owned files are `scripts/qualify_flatfile_native_custody.h`,
`scripts/qualify_flatfile_restore.cpp`,
`tests/async/flatfile_custody_audit_fixture.cpp`,
`tests/async/test_flatfile_custody_audit.py`, AUDIT_OPERATIONS.md, this report
and the additive PLAN5_REMOTE_FOLLOWUP_2026-10-06.md. No shared schema or
interface change is requested. For the primary's test coordination, the new
test takes an explicit `--native-source` path to the exact integrated checkout
supporting quest continuation v6. Its real native dependencies are the item
repository, authority transaction, store, snapshot codec, item command,
critical command and economic source event providers. Integration and shared
runner registration remain with the primary.

Exact tested owned code tree `e53db810789b55e8936f8666e4b078aea1df5bc3`,
archive SHA256 `34b7813b0e46ecb863b0b5d8edfcfbddb61ac13c4afcfccd1b79ea59ffeaa0ff`.
Owned native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. Publication adds
only this report, operator guidance and remote follow-up prose to that frozen
tested tree. The older owned native base is not the combined candidate.

The refreshed primary oracle is `973bb6c0e423acf105b2bea4680dcea2df4417b6`,
tree `566e353f9c6d87ceaa816e16125777828af491bb`, native tree
`833d3085815b396861ad18a77635412212381e4b`, migration tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, archive SHA256
`6f0c87a856b8ed229f8fbbf4a16076d4860f2c652dd7acb7e5d9c06bfe2eb379`.
The refresh from the prior oracle changes documentation only; native and
migration trees are identical. Its documented host limitations do not block
this host's disposable Docker work.

The reader checks the exact envelope, checksum, version/revision, count bounds,
sorted unique owner identities and UIDs, native owner constraints, item state,
equipment placement, owner existence and inline coin framing. Inline literals
retain their complete borrowed bytes for later field agreement; every borrowed
span requires the caller's encoded buffer to remain alive and immutable. The
one-item snapshot uses the native 128 KiB bound, 4,096-byte strings and shared
8,192-row budget. Catalog decoding preserves native acceptance of zero owner
or item revision and signed coin values; it does not grant money admission.

Retained operations validate identity uniqueness, result framing, creation
fields, quest acknowledgement/legacy state and XP bit/revision framing.
Quest continuation versions 1 through 6 validate complete nested reward,
recipient and award structure. Version 6 also checks its typed source event,
mobile lifetime, distinct triggering acceptance identity and fixed original
item result. These are structural checks: the original accepted command,
source authority, native quest execution and fee acceptance are not
authenticated by this catalog-only command. Opaque coin-result bytes retain
the native catalog's framing semantics; their monetary validity is separate.

The command uses the existing authority shared read lock, protected private
regular-file reads and aggregate byte/file/time budget. It requires an existing
lock when a catalog exists, checks pending journals twice and verifies the
authority boundary at completion. It creates no lock and performs no recovery.
An absent catalog returns `catalog_present=false` and
`custody_catalog_decoded=false`. Corruption, unsafe paths, pending journals and
an unavailable required lock refuse with no stdout and the existing fixed
`native_restore_qualification_failed` diagnostic. Every successful report leaves
`native_holdings_compared`, `owner_literals_compared`, `item_history_verified`,
`full_R7_qualified` and `release_qualified` false.

The exact final execution is `python3 -u -B /evidence/observer.py` in pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The observer calls the owned fixture builder with `/primary` and the unchanged
maintained `build_restore_qualifier.build` with `/workspace`. It then runs the
complete catalog and boundary functions. Every compiler/operator command is
retained in `retained-tests/custody-catalog/commands.json`. The standalone
reproduction command is `python3 -u -B tests/async/test_flatfile_custody_audit.py
--native-source /absolute/exact/integrated-checkout`; its help and required-source
CLI controls were separately executed against the final source.

GCC 13.3.0 compiles the private native oracle as C++20 with
-Wall/-Wextra/-Wpedantic/-Werror, AddressSanitizer/UndefinedBehaviorSanitizer,
no PIE and the seven actual native providers. The maintained operator retains
its original strict C++20 build recipe and providers; that builder does not
enable sanitizers. Both builds pass. Compiler dependency probes bind 329
tracked inputs: 114 from the actual primary and 215 from the owned tree. Every
body matches the frozen transport and immutable Git blob; original archive
mode 0664 and all four tracked links remain exact through the terminal guard.

All 133 catalog observations pass: 50 native-accepted catalogs and 83 native
refusals agree with independent decoding. Accepted item UID, root, parent,
owner, revision, vnum, state, equipment and complete inline payload bytes match
the actual native catalog records. Cases include versions 1-8, all 12 owner
kinds, historical/default fields, full-width identities and XP masks,
quest versions 1-6, fee-tail corruption, envelope errors, ordering/duplicates,
equipment, creation, quest/XP controls and coin literal limits. Each accepted
case also refuses an exhausted aggregate byte budget. These modeled native
component fixtures do not prove original gameplay or fee-acceptance journeys.

All 13 operator boundary controls pass: healthy/absent/uninitialized catalogs,
missing/held locks, three pending journal types, public file/root, symlink,
dangling symlink and hardlink. Authority file bodies, modes, kinds and link
counts remain unchanged before/after every operator call. Unsafe inputs refuse
without partial JSON. Private native fixture initialization occurs only in
fresh disposable roots. SQL is not applicable to this flatfile catalog feature;
neither SQL engine nor a canonical database migration is claimed for this slice.

`make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile
BIN_ROOT=/workspace/bin/tests/money-history-build/bin
OBJDIR=/workspace/bin/tests/money-history-build/objects
DMS_BINARY=/workspace/bin/tests/money-history-build/server` passes using the
previous validated 740 objects after exact verification of all 1,291 native
dependencies and Make inputs. Objects, dependency files and server are byte
identical; this is an incremental server check, not a fresh native build.
Server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.
Make's frozen source precedes only the non-native test CLI entrypoint addition;
all native inputs and touched C++ bodies match the final tested tree.
`python3 -u -B scripts/validate_economy_accounting.py` passes; its `--release`
mode returns the expected refusal for missing writer execution evidence.
Changed-file formatter checks pass for all three C++/header files, and staged
whitespace checks pass. No skips occur in the final owned qualification.

The first native fixture link fails before any catalog case because two real
providers were missing. That attempt is retained with exit 1 and its exact
sources/command. The repaired seven-provider recipe passes. A complete second
run passes before the explicit CLI addition; the final third run recompiles
and passes the final source. No fake provider, reduced assertion or timeout
relaxation is used. All four Docker processes are terminal and not OOM-killed.

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`. The immutable seal is
`custody-catalog-seal-01-20261007/evidence.json`, SHA256
`39701e1ee1351ad4bf22fbedbaff411e2f6b151acedd8f175c47e09be66e0e71`,
binding 1,729 artifact entries and 1,627,700,639 bytes across all attempts,
build/format/CLI controls, complete input catalogs, binaries, Linux boundary
receipts, compiler probes, source transports and terminal process inspections.
Symlink target metadata is hashed without following it. Copied NTFS artifacts
do not establish original Linux link counts or modes; the original before/after
Linux receipts establish those controls.

The prior validated build is additionally bound by
`custody-catalog-build-binding-01-20261007/binding.json`, SHA256
`e2c48a8ea35baa6d163585e88276b21415974e080ce617a63160f01a9864ab3c`,
covering 1,491 original artifact/source entries and 1,294,533,318 bytes,
including all 740 objects and 740 dependency files. Final run
`custody-catalog-green-03-20261007` exits 0 in 126.273363 seconds including
artifact retention, log SHA256
`d1a35da31219e6dbbd8fc1fe982ae0e66293bb4cd7699963213e82a6c6cb5405`.
Publication transport and final remote delivery are separate additive packets.

Remaining gates include complete native holdings and owner-literal comparison,
UID/custody/history/treasury reconciliation, current combined canonical SQL
qualification on both engines, original producer/gameplay journeys, all R1-R8,
and release-host budget, latency, growth, backup/restore and retention evidence.
Catalog counts, modeled fixtures and isolated passing checks do not complete
Plan 5 or release. There is no blocker to continuing the next owned slice.
Inactive behavior, wallet-root item exclusions and the declined inactive spell
change remain intact. No activation, production change, correction, deployment
or merge occurs. The primary's locally maintained notebook remains nonblocking;
this report and the remote follow-up provide the curator-ready update without
claiming notebook application, acknowledgement or cross-chat notification.
