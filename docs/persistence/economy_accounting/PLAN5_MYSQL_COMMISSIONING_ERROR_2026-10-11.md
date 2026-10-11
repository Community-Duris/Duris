# Plan 5 foreground MySQL commissioning diagnosis — 2026-10-11

The bounded foreground reproduction captured the exact startup error that was
missing from the first schema65 attempt. MySQL's default initialization reached
its temporary Unix-socket server (port0), then failed during time-zone import:
`Unable to resize binlog IOCACHE auxiliary file`, followed by
`ERROR 14 (HY000) at line 141392: Can't change size of file (OS errno 2 - No such
file or directory)`. Logs identify mysqld8.0.46. No TCP readiness/version query
succeeded, and no accounting schemas, migrations or qualification ran.

Owned local/remote branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`. This evidence-only
slice starts at `774d4422ff85aef8a0f6aaa1b9ae740b227a442d`; published result is bound by the delivery receipt.
Candidate association `a60f5dcce0c6496e687a0806ee7d0c559579e4fb`; no candidate code or contract was changed.
Only this report and the additive remote curator follow-up are maintained edits.

Exact host command:

```text
python.exe -B D:/Dev/Temp/accounting-plan5-ACC02-20261011/commission_mysql.py
  --grant D:/Dev/Temp/accounting-acceleration-bootstrap-20261011/ACC-02-COMMISSION-01-resource-grant.json
```

Windows CPython3.12.10, helperSHA256
`88637c74faeec6b53056cf2f7e70ce56dd7736eed84bcde56fb00008b3ce647e`,
grantSHA256 `67327e01ffb8e45ad1b8bba0b095673731985d756da26cba1d4793fc6ae50aa9`.
Session22689 terminalexit1, actual elapsed31.609s (150s bound),26 TCP readiness
attempts. Container `8d421be82de5c040f901942a8ef1d08f1babe88f05104127eca14bb63c91093d`, name `plan5-acc02-commission-01`,
pinned image `sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b`,512MiB/CPU1/networknone/restartno/foreground--rm.
Fresh direct D: datadir `/var/lib/mysql` and temporary `/tmp`; unchanged
`docker-entrypoint.sh`/`mysqld` command and default options. Readiness uses the
maintained helper's `mysql --protocol=tcp -h127.0.0.1 -P3306 -uroot -N -B --raw
-e SELECT VERSION();`. Password forwarded through environment names and redacted
from captured logs; no credential/daemon keys published.

Evidence `D:/Dev/Tests/Duris/accounting-plan5/ACC-02-COMMISSION-01` includes daemon
stdout/stderr, exact result/command/identity, grant/lock snapshots, host terminal
output, independent final inventory and helper sources. StderrSHA256
`2f74682bb6104f4ae672711c4ad0b5fabbc1e30d9697866e6ee16d720da9a9bf`, resultSHA256 `e29eea10319dabd4d86901a15b11987ad6d7a6eea2f6021d9b0e2eacc595c750`.
Sealed manifestSHA256 `3f5da42d3f9297ac13fc455f9acf2e69ded3cc45db3899c23d2d14cfb979e643` covers12 files/26013
bytes; generated daemon data/tmp remain retained outside immutable members.

The failed attempt established the error, not its causal filesystem. Next approved
comparison removes only the added Windows `/tmp` bind, retaining the original
image's Linux `/tmp` on Docker's registered D: virtual disk. Keep the same pinned
image,512MiB/CPU1, default server options, direct D: datadir and readiness command
on fresh data. No binlog/time-zone disabling, memory/sql-mode change or test
weakening. This native commissioning comparison needs its separate exclusive
grant; it is not a schema65 retry or release qualification.

Owned inventory exit0/empty was independently verified after terminal. The exact
matching job lock was released after sealing; no owned container/heavy lease
remains. Seven unrelated services and all failed data/logs are preserved. Full
goal ACTIVE; schema65/restore/retention/erasure/authority/recovery/load/release gates
OPEN, accounting inactive/admission CLOSED. Primary retains all shared contracts;
this report/follow-up are additive curator input without notebook import claim.
