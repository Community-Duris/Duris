# Character deletion follow-up (#200)

PR #204 already fixed false success and transactional cleanup. This follow-up
tests that merged implementation through real Telnet sessions and MariaDB.

Build the MariaDB server, then run in a disposable loopback database server:

```sh
TEST_DB_HOST=127.0.0.1 TEST_DB_USER=<test-user> TEST_DB_PASSWORD=<test-password> \
  python3 tests/async/run_mysql_deletion_journey.py --server /absolute/path/dms
```

The runner creates a uniquely named schema, applies the normal bootstrap and
migration runner, and creates a synthetic account/character through Telnet. It
saves and quits before attempting deletion. A database trigger first refuses the
account mapping update, then a separate trigger refuses the final player-row
deletion after earlier cleanup has executed. Both attempts must report failure,
retain the active mapping and exact inventory count, and allow the same player
to reconnect, play and save. With the trigger removed, retry must report success
once, remove the player and active mapping, and keep the account usable. After
process restart, the deletion selector and SQL authority must still be empty.
The schema is removed by the runner even on failure; no checkout `.env` is read.

The complete local journey passed all three cases and restart. The existing
account deletion runtime regression and terminal-save safety checks also passed.
Both local backend development builds passed in this bug sweep. The SQL journey
was repeated against a separately rebuilt, unchanged master `767e66e2b` binary;
deletion and account runtime match this test-only PR exactly.

## Historical operator follow-up

The historical September 5 disposable character has not been identified through
protected operator evidence. This synthetic journey cannot establish whether
that character still exists or whether authorized cleanup occurred. Do not delete
by an assumed name, publish identity details, or treat this test as that cleanup.
That historical housekeeping is separate from the software defect and does not
block this test-only PR or the bug's closure. The original software failure is
already fixed by merged PR #204 and the full disposable SQL/Telnet proof is
complete. No historical cleanup is claimed.
