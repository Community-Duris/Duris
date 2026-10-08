# Valid whole-history SQL runtime rollback component — 2026-10-08

**PASS at component scope:** real story runtime, production binding, feature,
tracking, catalog, SQL repository, complete MySQL `sql.c` formatter, persistence
mode and logger. O1 and Og ASan/UBSan generate the same valid whole-service
history. Forty completions fit; the next completion crosses the actual formatted
query buffer. The runtime reports the repository write error and restores the
byte-identical prior state on both identical attempts. Fitting persists before
and after those refusals reach the exact controlled trace. The writer capacity
limitation remains; this delivery changes no production policy or implementation.

## Publication and import

| Input | Exact pin |
|---|---|
| Prep base | `2eb2fedd683ba83130bf4beddea419e6a153ef71` |
| Executable code, pushed | `2b17bc59b21a4201760e2f3717afb2ee5f635053` |
| Frozen public candidate, compiled | `e6221a016ba8af26451831f1f4ec07ea264cf115` |
| Final fetched primary candidate | `d9d98b6c1b30b2ff8f0a901db5f1c35ecf27c0b1` |
| Both complete `src` trees | `833d3085815b396861ad18a77635412212381e4b` |
| Both migrations trees | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Compiled whole-source archive | 26,910,720 bytes; SHA256 `85e4d0cd4960c7b46d69dcab0b1d64beab65bca733741ca0260de22f4bca1b9a` |

The final primary successor changes documentation only relative to the compiled
candidate; no source or migration difference. Code adds exactly these two files:

- `tests/async/quest_accounting_prep/story_history_sql_runtime_rollback.cpp`
- `tests/async/quest_accounting_prep/test_story_history_sql_runtime_rollback.py`

This documentation companion adds only this dated handoff. No production,
shared-driver, canonical handoff, finish-plan, migration or registry edits.
Import the code commit alone; it does not depend on another prep helper. The
standalone binary patch applies to a fresh frozen-source export. Imported,
committed and executed inputs are byte-identical, and all source bodies remain
exact. That import check is not a second runtime execution. The exact final
documentation commit and remote head are recorded in local `FINAL.json` below
and the publication response; the handoff does not embed its own commit SHA.

| Test input | Git blob | SHA256 |
|---|---|---|
| C++ 15,981 bytes | `670aa41f26035aade0c0100d06ba60374ea2087d` | `fe848fc6cd5310ab07c04c2e9ceaf2cc34f55875f5a3a5bde5d8d90bcda3419c` |
| Python 17,025 bytes | `4eff8e599bb919cd355c40893b092a4fd5aaf7cd` | `7cb9ff870fedd86931fb23de11b485e3ef46a78a418f6b97a72f049e8802739b` |

## Valid history, actual owners and exact observations

The actual production bootstrap binds one minimal in-memory Q: giver17, zone1,
ITEM24402 input and ITEM24403 reward, no fee or disappearance. This is a
component fixture, not a production-area claim, full-boot/reset binding or native
NPC lifetime. Its actual generated definition is:

```text
zone-story:qst:17:676976653d493a32343430323b726563656976653d493a32343430333b6469736170706561723d30
```

Actual `service::record_completion` builds each candidate document; there is no
padded state string or injected serialized transaction. Typed events use keys
`history-0` through `history-40`, short component name labels `Component0` through
`Component40`, season101, revision1, zone1, room101, timestamps1791400000+i,
direct PID701+2i and ordered recipients `[701+2i,702+2i]`. These PIDs and names
are modeled component identities, not native account/player/source identities.
Level20, racewar1, known party size2, strongest level25, duration0 and success
produce real telemetry. Daily mode is disabled. The real parser/serializer
round-trips both documents byte-exactly.

| Actual generated document | Fitting | Next completion |
|---|---:|---:|
| Completions | 40 | 41 |
| Raw serialized bytes | 63,707 | 65,303 |
| LF bytes | 201 | 206 |
| Quotes / backslashes | 0 / 0 | 0 / 0 |
| Controlled escaped bytes | 63,908 | 65,509 |
| Full UPSERT bytes, including296-byte overhead | 64,204 | 65,805 |
| T / N / C / E rows | 40 / 40 / 80 / 40 | 41 / 41 / 82 / 41 |

The real buffer is65,536 bytes including the terminating NUL. Both documents
are below the repository16MiB preguard, and even the refused raw document is
below65,536 bytes. LF escaping plus the actual SQL envelope crosses the real
formatter limit. Forty/forty-one describes this fixture, not a universal
completion count or a chosen retention policy.

The executed sequence, identical in both profiles:

1. Real mode configuration selects `mariadb-primary`; real `sql_season_epoch()`
   remains0 and the actual runtime uses the configured season101 fallback.
2. Real runtime bootstrap calls real repository load. Controlled query/row/length
   responses provide version1/revision1 and the fitting generated state. Actual
   repository parsing, row ownership/free and runtime deserialization reconstruct
   the exact document. One query, row, lengths read and result free; no live result.
3. Actual runtime persist reaches real repository save and real `qry_at`, then
   controlled trace1 with the exact fitting complete query.
4. Actual `record_authoritative_completion` for `history-40` constructs the
   same oversized document. Real `vsnprintf` length rejection prevents any trace
   call; real repository returns `zone-story SQL state write failed`. Actual
   runtime deserialization restores the entire prior document byte-for-byte.
5. The identical event/key is retried. It fails with the same error and same
   attempted bytes, without a trace or controlled saved-image change. No ghost
   T, recipient credit C, recipient metadata N or completion telemetry E survives.
   Actual summaries for both new recipients remain completed0/name `Absent`.
6. A final fitting runtime persist reaches exact trace2. Final memory and the
   controlled saved image equal the original fitting bytes.

Final counters: reads1, rows1, lengths1, frees1, escapes4, traces2. The genuine
logger records exactly two `MySQL error: Query too long or formatting error`
messages per profile. Independent decoding checks every transaction field,
direct/ordered recipients, names, credit masks13/direct and4/peer, and every
telemetry field. Failed attempted bytes contain the additional facts; both
restored images, both failed saved images and final memory/saved images equal
the fitting document exactly.

## Genuine providers and explicit SQL boundaries

The runner compiles the complete TUs listed in `SOURCES`, not extracted function
bodies: story runtime/production/feature/tracking/catalog, SQL story repository,
full MySQL `sql.c`, `utility.c`, persistence mode, and the three retained flat
providers needed for link closure. Flat providers are not executed in SQL mode.
Actual runtime owners are `zone_story_quest_runtime.c:43,67,137,151,186,252`;
actual feature serialization/deserialization is at1376/1445. Actual repository
load query is at36 and save query at123; actual MySQL formatter at3818 and
config buffer at `core/config.h:141`. Whole source bodies, headers and transitive
compiler dependencies are authenticated separately.

Only explicit SQL boundaries are controlled: opaque `DB` token, `db_query_at`,
`mysql_fetch_row`/`mysql_fetch_lengths`/`mysql_free_result`, malloc-owned
`sql_escape_string`, and `sql_trace_exec_at`. Load validates exact SELECT and
file/function/line36. Trace validates file/function/line123, `qry/direct`, full
query bytes/length and drain flags. A successful trace copies the attempted
document into a **controlled image**, not a database receipt. Escape is an
explicit printable-ASCII model: LF becomes `\n`, quote `\'`, backslash `\\`;
other control/high bytes are refused. Actual generated data uses only LF and
printable ASCII, with zero quotes/backslashes. This does not execute MySQL
escaping, charset handling, a client connection, SQL statements or transactions.

`objcopy` weakens exactly these two symbols in a retained copy of full `sql.o`:

```text
_Z11db_query_at22persistence_query_sitePKcz
_Z17sql_trace_exec_at22persistence_query_sitePKcS1_mbb
```

The executable supplies strong SQL doubles. Each original GLOBAL/FUNC to
WEAK/FUNC change is authenticated independently. Across604/O1 and6,814/Og
sections, all code, relocations, data and debug bytes remain exact; only the two
symbol binding bytes and objcopy's section-name-table representation differ.
`qry_at` remains the genuine provider:429/O1 and677/Og code bytes, unchanged
SHA256s `964631f00a663e5071aed020b4bc207db59caf328d66d06186b0aa6533c5140e`
and `3709a45fa8fb51604eff5e4c4ae93b6f59cdb0c5581ab1db4106cafe96126b98`.
Final ELF disassembly independently verifies real repository load calls the
strong query boundary and real formatter calls the strong trace boundary.
No unresolved `mysql_` symbols or MySQL client library remain. Six private
code/relocation/unrelated-binding mutations are rejected before execution.

## Commands, evidence and execution limits

Executed from the existing prep worktree in Ubuntu-22.04 WSL:

```sh
bash scripts/format.sh --file tests/async/quest_accounting_prep/story_history_sql_runtime_rollback.cpp
bash scripts/format.sh --check --file tests/async/quest_accounting_prep/story_history_sql_runtime_rollback.cpp
TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B \
  tests/async/quest_accounting_prep/test_story_history_sql_runtime_rollback.py \
  --candidate e6221a016ba8af26451831f1f4ec07ea264cf115 \
  --git-dir '/mnt/c/Users/alexa/OneDrive/Documents/ChatGPT/NewDuris Max/.git/worktrees/NewDuris-Max8'
python3 -B /mnt/d/Dev/Temp/quest-story-sql-runtime-rollback-publication-20261008/verify.py
```

Windows publication checks: `python -B D:\Dev\Temp\quest-story-sql-runtime-rollback-publication-20261008\publish.py`
and `final.py`; `git diff --cached --check`, `git diff --check`,
`git fetch origin experimental-accounting`, exact source/migration tree comparison
and push only to `origin/codex/accounting-quest-prep`. Runner proof records every
actual compiler/link/tool/runtime argv, cwd, overrides, return code, timeout and
stdout/stderr. Strict C++20 `-Wall -Wextra -Wpedantic -Werror`; O1 control and
Og ASan/UBSan with no sanitizer recovery, leak checking and frame pointers.
Whole deadline600s, compile180s, link90s, runtime30s; no timeout or compiler,
component or sanitizer failure. g++11.4.0, MySQL headers/config8.0.46. Both
runtime executions completed under0.3s. No server build or gameplay batch.

| Executed ELF | Bytes | SHA256 |
|---|---:|---|
| O1 | 9,502,432 | `8740a66b72000ff4e81b0b87b28cc5251eff63b29a5560a417614986f7852262` |
| Og ASan/UBSan | 18,110,848 | `b62086cc14811625944f3771293a787006f2efcdbbd1b8dd53222e94792c7714` |

- Evidence: `D:\Dev\Temp\quest-story-sql-runtime-rollback-1cydqwqw`.
- Objects, dependency files, link maps and ELFs:
  `D:\Dev\Builds\NewDuris\quest-story-sql-runtime-rollback\quest-story-sql-runtime-rollback-1cydqwqw\bin`.
- Independent verification/import/publication:
  `D:\Dev\Temp\quest-story-sql-runtime-rollback-publication-20261008`.
  `verification.json` authenticates1,334 source bodies/modes,584 dependency paths,
  1,694 artifacts, both original/controlled objects, final calls, ELF/build hashes,
  compiler tools/transitive libraries, every actual row and query/rollback byte.
  `publication.json`, `FINAL.json` and `SEALED.json` pin import, commits/remote
  and retained publication files. No generated evidence or binary is committed.

One private publication-verifier launch occurred after both component PASS
lines but before the runner finished `ARTIFACTS.json`; it failed immediately
with `FileNotFoundError`. `verification-launch-too-early.json` preserves this
sequencing failure. After actual runner exit0, verification passed. This did not
rerun or alter either component. The initial ELF-reader NUL-token typo was fixed
before the only component invocation; all executed/committed input bytes match.

## Remaining shared-owner boundaries

This proves the current runtime restores valid whole-service memory after an
actual SQL formatting refusal. It does not fix history capacity, prove successful
database commit, cold database recovery, lost reply, economic effect rollback,
refund, reward ACK, original parent/child retirement, native birth/source custody
or crash durability. Controlled saved-image stability is limited to refusal
before trace. Whole story history remains distinct from economic authority.

Primary still owns the history writer/capacity policy and any integration with
economic completion and acknowledgement. This fixture demonstrates a legitimate
history can hit the writer before its MEDIUMTEXT guard; the primary must select
the shared production remedy and qualify it on the integrated candidate. The
maintained real SQL/native quest journeys and original broader finish criteria
remain separate. This bounded delivery is complete and importable; the actual
continuing native Goal remains **blocked/unfinished**, not completed by this PASS.
