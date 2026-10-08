# Plan 5: next strict flatfile build finding before managed v3 restore

This independent qualification attempt is on branch `codex/accounting-plan5`,
worktree `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base `f8b8dec31d3f8d6d2e31647e58c69a5348c4644f` merges the primary header repair
`c78a97055920d5e38f068e8df30805ffaa22ebf6` after the separately committed price
audit slice `0bb64bd6482949559c8de5ae4075a6b389967b6b`. The native tree is
`c1dbd3e70f23548a23e9b7722046fc31f498e58c`; migrations remain
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The commit containing this report is the result handoff. Both input commits
and this lane are public; this lane does not push `experimental-accounting`.

## Observed build, without changing native source

An entirely fresh maintained build uses immutable image
`duris-plan5-origin-sql-tools:local`, previously pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Docker runs with `--network none`, a read-only worktree bind at `/workspace`
and a writable `bin` bind for explicit new outputs. No existing object,
binary or previous evidence path is overwritten. The exact command is:

```sh
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-lifecycle-v3-qualified/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-lifecycle-v3-qualified/server-flatfile
```

The maintained C++20 compiler flags retain `-Werror`, `-D__NO_MYSQL__`, the
existing warning set and hardening; no warning is disabled. The fresh command
returns **exit 2 after 335.406 seconds**. There are 476 compilation commands,
475 completed objects, one compiler error and no server binary. The sole error
is:

```text
economy/coin_physical_recovery.c:1225:56: error: comparison of integer expressions
of different signedness: 'const int32_t' and 'size_t' [-Werror=sign-compare]
    if (child.parent_index == index)
```

The corrected `flatfile_economic_runtime.c` and previously corrected
`collector_service.c` both compile under the maintained strict flags. Their
object SHA-256 values are respectively
`f0b652b1c0d33242cd7f39faf18a2cf6560824118cca1125edcf6d99d641ab6e` and
`ad13d4481e5eeaf862c731b10c90bcc7fcdd9783269860783184381d809c6f3f`.
This clears their compiler findings within this exact build, without claiming
their runtime publication or shutdown journeys have passed.

The native source fingerprint is unchanged by this attempt. All earlier
artifacts remain intact. The managed v3 integration test is **not invoked**:
there is no new server to select. Actual service boots in this attempt are zero;
no SQL runtime test, accounting activation or source capture is executed.

## Narrow primary-owned repair handoff

Owned native file: `src/economy/coin_physical_recovery.c`, line 1225, inside
`observe_and_project_flat`. This is shared Plan 2 recovery/publication logic;
Plan 5 does not edit or commit it.

Exact fields and invariants:

- `player_item_snapshot.parent_index`, declared in `player/player_snapshot.h`,
  is `int32_t`. `PLAYER_SNAPSHOT_NO_PARENT` is the existing signed sentinel -1.
- `flatfile_room_item_record.items` is a vector of these native item snapshots;
  its loop index is `size_t`. Native graph encoding/validation has the existing
  4,096-object bound. No type, wire, graph limit or sentinel change is needed.
- A retained active selected coin pile must occur exactly once in its owning
  room at the expected room revision, as a root with no equipment slot, with
  exact literal/custody bytes and no child graph. Any item whose nonnegative
  parent index equals the selected root's vector index must still refuse
  publication. A negative root sentinel must never be interpreted as a child.
- The existing catalog validation, duplicate-UID checks, current-cut fences,
  native/completion agreement and projection/ACK hold remain required.

A narrow candidate is:

```cpp
if (child.parent_index >= 0 &&
    static_cast<size_t>(child.parent_index) == index)
    return false;
```

This avoids signed/unsigned comparison without narrowing the vector index.
It preserves every comparison for valid nonnegative parent indices and makes
the negative sentinel exclusion explicit. There is no schema, public interface,
new observer, counter or mutation-policy request.

The consumer is `coin_physical_recovery_publish`, which calls this flatfile
projection path before a publication can be accepted/released. Primary tests
must retain child rejection, exact root/custody matching, foreign-room/revision
and duplicate refusal, and ACK ordering; a compiler repair alone does not prove
those behaviors. Use the existing prepared physical publication/ACK regressions
at their owner milestone and an actual flatfile retained-pile cold replay for
callback-free recovery. Include root sentinel -1, a matching child parent,
a nonmatching valid parent and malformed graph refusal. Do not substitute the
prepared component seams for the actual cold-replay acceptance journey.

Diagnostic only: a fresh copied translation unit under `bin` changes exactly
this one condition. The probe reconstructs the original failed compiler command
with `shlex`, retains all warnings, uses `-fsyntax-only`, and puts dependency
output under the fresh artifact directory. It returns **exit 0**, with no
diagnostic output and original source bytes unchanged. Original source SHA-256
is `a64e8f47c814aeb34c11ea3e6051f2527b3d2b80da5963a9a0175b149d30b8ae`;
diagnostic copy SHA-256 is
`0670e71865ebe3543ec18c4142653ed0ea6499cd93cc3bb36f06407f951e8397`.
The exact compiler argument vector is in
`coin-physical-recovery-diagnostic.json`. This is syntax evidence for the
proposed primary repair, not a qualified production build or runtime fix.

## Independent checks and preserved evidence

On the refreshed exact base:

| Command | Result |
| --- | --- |
| `python3 scripts/validate_economy_accounting.py` | Pass, 14.526 s, 14 fixtures / 886 routes / 2,843 occurrences; contract validity only. |
| `python3 scripts/generate_economy_writer_coverage.py --check` | Pass, 14.570 s; zero unmapped current sites; census/coverage remain false and release BLOCKED. |
| `python3 scripts/validate_economy_accounting.py --release` | Expected exit 1, 0.362 s: writer has no executable evidence. |
| Maintained flatfile `make` above | Exit 2; exact sign-comparison error; server absent. |
| Copied native diagnostic, original strict compiler arguments | Pass, exit 0; original source unchanged; no runtime proof. |
| `git diff --check` | Pass. |

No unrelated passing database/fixture suite is repeated for this one shared
header refresh. Those earlier exact-source reports retain their own scope.
The next build must use the primary's real repair and fresh evidence paths.
Once a maintained server exists, the prepared managed test command is:

```sh
PYTHONPATH=/workspace/tests/async \
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_SERVER=<new-maintained-flatfile-server> \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=<fresh-directory-under-bin> \
python3 -u -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

That test uses the real backup/restore managers and two actual isolated service
boots. Its source requires exact native WAL and pending-transaction replay,
drained journals, one million UID reservation and sealed witness advancement
per boot, retained inactive receipt/generation preservation, retention pruning,
manifest damage refusal and checksummed pre-capture loss/corruption refusal.
It models known-native inactive history and does not run lifecycle install or
source capture. This attempt does not claim any of those unexecuted outcomes.

Evidence resides under `bin/tests/plan5-lifecycle-v3-qualified/`: the complete
build log, command/result/timing record, 475 objects and dependencies, contract
logs/records, preservation result, diagnostic source/log/command/dependencies
and copied probe/recorder sources. The directory name was allocated for the
planned qualification; its recorded build result is failure. The frozen Git
entries are `tmp/plan5/lifecycle-v3-qualified-base-tree.txt`. Final manifest
`tmp/plan5/lifecycle-v3-qualified-evidence.json` verifies all 1,254 native and
236 migration inputs and all 25,556 protected prior artifacts unchanged. It
records 1,011 fresh evidence files. Manifest SHA-256 is
`7574f957244bfd15b3805938b3205f0d9165bb18b3608c6ccf54b23ac1a3379a`.

## Remaining gates and notebook handoff

The primary owns the signed-parent-index compiler repair and all related shared
recovery/publication decisions. Actual managed v3 restore/cold boots remain
unqualified until the maintained build succeeds. Full SQL and flatfile release
qualification, real source capture/install, writer coverage and all applicable
R1–R8 acceptance gates remain open. The primary's confirmed original baseline
admission-time field still requires its native transaction and additive schema
implementation, followed by independent full command verification/damage and
cold-restore evidence on both engines. No historical timestamp is fabricated.

The primary maintains the shared notebook locally through its curator workflow,
per the user's clarification. This exact report supplies the curator handoff;
notebook access is not a blocker. The independently qualified price slice stays
separate and available at `0bb64bd6482949559c8de5ae4075a6b389967b6b`.
Accounting remains inactive; wallet-root item exclusions and the declined
inactive spell-path behavior are preserved. No production data, deployment,
PR merge, audit correction or activation is performed.
