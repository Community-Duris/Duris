# Plan 5 MySQL temporary-filesystem repair qualified — 2026-10-11

Removing the added Windows `/tmp` bind restores MySQL startup under the exact
commissioning profile. The paired fresh runs preserve the image,512MiB/CPU1,
default entrypoint/options, direct D: datadir and original TCP readiness command.
Failed control01 stops at the binlog IOCACHE resize ERROR14 during time-zone
import; repaired02 completes initialization and returns actual version8.0.46.
No MySQL logging/time-zone/SQL-mode option was disabled or relaxed. This qualifies
the startup transport repair, not schema65, MariaDB, restore or release.

Owned branch/remote `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`. Slice base
`31f1f2c100b7bcca71f9f9ca1e39a1f02376e741`; result commit bound by delivery. Source association `a60f5dcce0c6496e687a0806ee7d0c559579e4fb`;
no maintained production/migration/interface file changed. This report and the
additive remote follow-up are the only maintained edits.

Exact command on Windows CPython3.12.10:

```text
python.exe -B D:/Dev/Temp/accounting-plan5-ACC02-20261011/commission_mysql_02.py
  --grant D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-02-COMMISSION-02-resource-grant.json
```

HelperSHA256 `72dd8a8492e3ea80b59824868311a07b59747738bd114d84166d402fffa4f002`;
grantSHA256 `65ebd13e6ffba4d8f93276abd031ccd3c25ed3f1521b786d7710aafed8199f4f`.
Session82480 terminalexit0;31 readiness attempts,42.515s including safe shutdown.
Actual `SELECT VERSION()` returned `8.0.46` over loopbackTCP:3306 with no stderr.
CID `70a4c34ab8ff1c7b8a5c89b476b42a3f48b145979911b78dbb5c389bbb71615d`, image `sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b`. Exact CID/image/owner/job
verified before stopping; active state reported OOMKilledfalse, restartno and
only the direct D: `/var/lib/mysql` mount. The image's original Linux `/tmp`
remains on Docker's registered D: virtual disk. Owned stop and foreground exit0;
final owned inventory exit0/empty. No migrations or test schemas ran.

Evidence `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-COMMISSION-02`: actual result,
daemon stdout/stderr, command/helper diff, normalized paired command comparison,
grant/lock and terminal output. ResultSHA256 `de5ec9b0f19a07a481503a10dfde3ee49088ef540679163c25a8fada76742cfb`.
Sealed manifestSHA256 `38d3c6092f1d8c3a07ad141f1155f23f12c2b7a04d294117bff8259986209203` covers17 files/46422
bytes and binds failed01 seal `3f5da42d3f9297ac13fc455f9acf2e69ded3cc45db3899c23d2d14cfb979e643`.
The comparison mechanically verifies that only the `/tmp` mount pair disappears
after normalizing fresh owner/job/path identities. Failed01 data/tmp/logs and
successful02 scratch are preserved separately; generated keys/data are outside
immutable result members and are not published in Git.
The first comparison helper accidentally included the fresh container name in
its readiness comparison and refused before sealing/release. Its source and
preparation-error record are preserved; exact client arguments pass after that
identity field is normalized. No native result was changed.

After sealing and independent inventory confirmation, only the exact matching
COMMISSION-02 lock was released. No owned container/lease remains; seven unrelated
services are preserved. Next full unchanged matrix uses a fresh identicala60
source/export/build/evidence and its own grant. Its external transport will
restore original Linux/tmp and remove the extra detached-DB `--rm` so stopped
daemon logs survive until the original guardedfinally captures/removes them.
All migration/verification/startup predicates remain unchanged; schema65 constants
may change only in the private export after actual measured proof.

Schema65 two-engine histories/default and explicit startup/tamper, native restore,
retention/erasure, authority/recovery/load and final release remain OPEN. Goal
ACTIVE, accounting inactive/admission CLOSED. Primary owns shared schema/header,
registration and integration. Report/follow-up are additive curator input only;
no notebook import or release acknowledgment is claimed.
