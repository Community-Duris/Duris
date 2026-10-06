# Plan 5 retained native-mobile restore qualification — 2026-10-06

The original flatfile restore qualifier ignores retained
`domains/quest-mobile-native-<id>.qmn` images. The frozen reproducer places a
damaged image in an otherwise accepted private candidate. State preflight
returns success and the ordinary aggregate success report. This is a real
restore qualification omission, rather than a migration or producer defect.

The separate fix validates the entire protected filename namespace after the
existing authority-bundle recovery. Each image must be a bounded private regular
file with one link, decode as a canonical native value, and retain the exact
lifetime ID in its canonical filename. Both state preflight and final native
qualification execute the check. Historical v1 cash remains unknown; v2 cash
retains its exact denominations and revision. Retired images have empty stock,
and retired v2 cash must be zero. The checker never writes or materializes images.

The original complete restore-authority method passes the new controls and all
existing cases. The fresh maintained flatfile production build and original
managed backup/replay/service-boot method also pass. This closes the flatfile
value-validation omission. It does not authenticate native birth/source
authority or complete Plan 5, R6–R8, producer parity or release qualification.

## Branch, source and owned files

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch and sole publication destination: `codex/accounting-plan5`.
- Base: `4245289d7ad7742de7c47342c69d100852705310`.
- Refreshed primary: `f528a46b43e07444b97121a6e63cb9be414fb5d0`.
- Separately committed fix: `232aed48a104355677cc7ac3b0feb451475cdf67`.
- Native tree: `03a97173396f720857b1ad58a2ab69b7859a2387`.
- Migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61.
- The post-publication receipt binds the final report commit and remote tip.

The fix owns `scripts/qualify_flatfile_restore.cpp`, the existing native
`tests/async/flatfile_restore_authority_fixture.cpp`, its existing
`tests/async/test_flatfile_restore_economic_authority.py` driver, and the
`AUDIT_OPERATIONS.md` paragraph. This report and the appended remote follow-up
are separate evidence documentation. No shared contract, coordinator, producer,
source registry/matrix, migration or activation owner is changed. No shared
interface/schema request is needed; these owned files have no shared source pins.

The native image decoder is an existing pure value codec already present in
the qualifier's original link recipe. The fix introduces no provider, flag,
storage-reader, recovery call or duplicate decoder. The independent retained
economic reader remains separate and invokes no image mutation logic. Original
authority recovery runs only on the marked isolated candidate, as before.

The failing archive overlays the new fixture/driver on the base's unchanged
qualifier. The passing archive overlays exactly the four owned fix files.
All 3,122 tracked native, migration, script and test inputs match the committed
fix. The passing matrix adds three malformed namespace names beyond the failing
reproducer; its original damaged-image assertion is unchanged. The full native
and migration source remains byte-identical to primary. All seven preserved
older Plan 5 branch tips remain ancestors on the expected remote branch.

## Exact original native and managed proof

Both frozen candidates use this original command, with native build cache off:

```text
python3 -u -B tests/async/test_flatfile_restore_economic_authority.py
```

| Source | Original result | Elapsed seconds |
| --- | --- | --- |
| Base qualifier plus new regression | FAIL, exit 1: damaged image returns status 0 | 131.199299 |
| Exact committed successor | PASS, exit 0, zero skips | 167.391652 |

The failed observer returns zero only after verifying the expected original
exit 1 and exact false-success assertion. That original result remains failure
evidence. Original compiler flags, sanitizer flags, source providers, per-case
timeouts and the existing 20 positive/367 corrupt cases are preserved.

The passing original method reports:

- Four accepted v1/v2 live/retired image controls.
- 48 corrupt, noncanonical or unsafe image refusals, across 104 native calls in
  state preflight and final qualification. Inputs remain byte-identical.
- Reference/image magic, versions, reserved fields, lengths, identity, source
  shape, revisions, checksums, stock, cash and retired-state corruption checks.
- Canonical filename and lifetime equality, integer bounds, private permissions,
  symlink, hardlink, FIFO, directory and oversized-file refusals.
- The original 20 positive retained stores and 367 independent corruption refusals.
- 54 native semantic decodes, including the original 50 generic corruptions,
  and 1,058 native/independent metadata comparisons.
- Unchanged retained economic bytes and the original independent sanitizer reader.

The image controls use the original native reference and stock encoders with
explicit synthetic outer QMN framing. They represent private value-format
fixtures, not actual birth decisions, admitted sources or player journeys.
The production image decoder validates the complete resulting values. Neither
valid fixture hashes nor decoding grants source or activation authority.

The original fresh maintained build command is:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/evidence/build/bin OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server
```

Result: exit 0 in 401.765251 seconds within 600 seconds; 738 fresh objects and
738 dependency records, zero reused objects, warnings and errors. Server SHA256:
`52eb56adfb380d577d013d5b66fd401fb423c869bca612c657b023b5c50e0d96`.
Objects, dependencies, server and full original Make log are retained. The
native bytes are unchanged; this fresh build supplements the changed tool's
own original native compilation and does not build that tool through Make.

Original managed method, with `PYTHONPATH=tests/async` and
`DURIS_RUN_BACKUP_INTEGRATION=1`:

```text
python3 -u -B -m unittest -v test_persistence_backup_integration.PersistenceRecoveryIntegration.test_flatfile_real_pending_replay_account_player_domain_load_and_boot
```

Result: one method, exit 0, zero skips, 117.793680 seconds. It performs real
managed capture, pending authority/player/critical journal replay, account and
player loading, retained locker receipt checks, isolated native service boot,
drained journal checks and repeated idempotent verification. Live source,
generation and journal inventories remain unchanged. This existing journey
checks the surrounding workflow; it contains no actual native-mobile birth.

The method uses the fresh flatfile server above. Its unused SQL prerequisite
binary is the earlier matching native-tree build, SHA256
`9fe1e158d286076438ce9a8b1d85528e2f60215c3e57e46a22f0fd10c73691d0`.
All 738 original dependency records and 1,285 local dependency inputs match the
current archive. No SQL runtime, fresh SQL build or both-engine database result
is claimed by this flatfile method. SQL native-mobile image validation remains
separate work.

The original `scripts/format.sh --check --file <qualifier> --file <fixture>`
passes both complete touched C++ files with clang-format 14. Its committed LF
script runs from an ignored minimal view to avoid Windows Git-directory/CRLF
shell transport issues; it reads the actual owned C++ files and the same
repository configuration. `git diff --check` passes. No formatter diagnostic
or unrelated source change is waived.

## Source and retained evidence

Protected root: `D:\CodexEvidence\accounting-plan5\bin`.

- `native-mobile-restore-red-01-20261006`: original failed command/log/result,
  source archive, per-file transport, original execution and preparation helpers.
  Archive SHA256:
  `8f577ff06f225083d8529d249c0fe0bd17f7d6b252e0ec7fde3c91978da2b01a`.
- `native-mobile-restore-green-01-20261006`: complete passing original method,
  source/log hashes and formatter evidence. Archive SHA256:
  `c9bf34ed715b0931c60cf606f52e4203fe09b162ae5b29d013794061dd723f61`.
- `native-mobile-maintained-flatfile-01-20261006`: same archive, original fresh
  Make command/log/results, all objects/dependencies and maintained server.
- `native-mobile-managed-flatfile-01-20261006`: same source archive, 101 Git-bound
  public mini-world/runtime assets, original selected method/log/result, exact
  execution/preparation/launch helpers and reused SQL dependency proof.
- `native-mobile-restore-final-seal-01-20261006/evidence.json`: all 3,122 code input
  hashes, 1,518 artifact hashes, original results and terminal container states.
  SHA256: `25fcf9dac424dfab310fe98681d2c10df1be69affbe8ddce96bc21e1327c05cf`.
- Post-publication receipt: `tmp/plan5/native-mobile-restore-delivery.json`.
  It binds the tested fix, report/result/remote commits, seal and prior receipt,
  clean worktree, unchanged qualified inputs and preserved earlier tips.

The original native method prints temporary binary hashes before removing its
private build directory: qualifier
`d4dada8d8033f9e424590ed7076930477638585e4e8c76876672028b7081325d`,
fixture `78a9f8b006f09e2eccb45c23f59b421d4fcb4b1bbc27003ba1a8cef59c629af0`,
and independent reader
`99f254ddd40cfbf96d213c98cfaf65c587f6ba6055108e985cb302be0d04253b`.
Those are original observed hashes, not retained binary files. The maintained
server/build files are retained. Total sealed qualification artifacts occupy
1,376,970,210 bytes; that is evidence storage, not a gameplay growth measurement.

All execution uses tools image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, two CPUs, 4 GiB memory and separate 2 GiB RAM workspaces/tmp
limits. No live checkout, environment credentials or player data is mounted.
Only the original managed method receives namespace capability for its private
mount and isolated service process. All four containers exit zero at observer
level without OOM; the original reproducer remains exit 1. Source hashes remain
unchanged. Executed helpers are retained; all temporary fixture/authority data
stays private and is removed by the original recipes.

## Remaining work and curator packet

No skipped test or unresolved blocker remains for this flatfile omission.
Current SQL `quest_mobile_native` stores `mobile_instance_id`, `mobile_revision`,
`stock_revision`, `lifetime_state` and `canonical_image`; the independent SQL
restore/audit readers currently do not read that table. Its value/projection
validation is the next separate owned reader obligation, rather than a result
of this flatfile test. No shared schema change is requested here.

Complete native opening/physical forests, escrow/claims and mapping recovery,
retained partial claim consumption, authentic activation verification, actual
writer/player/publication/ACK/lost-reply/cold journeys, full backend/lifecycle
parity, typed active erasure and current retention, and mixed release-host
size/latency/storage/checkpoint/reconciliation budgets remain required.
Inventory and component fixtures cannot close those gates.

Primary's locally maintained notebook remains nonblocking. This owned report,
remote follow-up and sealed receipt provide its curator packet. Accounting
stays inactive; wallet-root exclusions and the declined inactive spell path
remain. No production data, audit findings or economic authority is corrected;
no activation, deployment, PR merge or independent primary-branch push occurs.
