# Actual story SQL query-capacity component — 2026-10-08

**PASS: full MySQL `qry_at` provider and complete story repository component**, O1
control and Og ASan/UBSan. With revision7 and identity-escaped safe ASCII, the
complete UPSERT has296 bytes of overhead:65,239 state bytes produce65,535 query
bytes and one exact controlled trace call;65,240 produce65,536 query bytes and
are refused before trace. The repository's16MiB preguard is a different boundary.
This characterizes the current writer; no capacity fix or persistence policy changes.

## Publication, source and import

| Input | Exact pin |
|---|---|
| Prep base | `cb8e0ffeb7506fac293c7638d5744a7d0cb53c37` |
| Executable code, pushed | `1e8e977a50d3e1cad9312af0b7cfbfa643bcf444` |
| Frozen public candidate | `c4f695b1aa90e978b1e977b702fc9fc6aaef6342` |
| Whole-TU feasibility candidate | `5f5a8bdfd0306a6c65857cf936fd6b332832c90b` |
| Final fresh-fetch candidate | `e6221a016ba8af26451831f1f4ec07ea264cf115`, docs only |
| All three complete `src` trees | `833d3085815b396861ad18a77635412212381e4b` |
| All three migrations trees | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Compiled source archive | 26,910,720 bytes; SHA256 `470f03e0c22e3af112e187e4f082674d99604640cc83ba49280ef77f62d70ec3` |

Code adds exactly `tests/async/quest_accounting_prep/story_history_sql_capacity.cpp`
and `tests/async/quest_accounting_prep/test_story_history_sql_capacity.py`.
This documentation companion adds only this handoff. Import the code commit
alone; documentation is separate. There is no dependency on an earlier prep
helper. The actual standalone patch applies to the frozen source export; imported,
committed and executed test bytes match. Source bodies remain exact. This is
import/identity proof, not another inferred runtime execution.

| Test input | Git blob | SHA256 |
|---|---|---|
| C++9,462 bytes | `b216897b48eb115a6db990c4b7bd8f68a9a2ae8e` | `d5b56022ca7733cb2efe2cb6177edf95488003e7f79866348ea70e9f434c44a6` |
| Python15,949 bytes | `224a90ef23a20ce98f765a77ec666cfcb602e866` | `c6b0d69108c220b1a63233ca6a95e613a32413e3ec0c594339e0f15ed447dcee` |

The immutable whole source export supplies these **complete translation units**:

- `src/sql/zone_story_quest_state_repository.c`: actual save at96-143, preguard,
  escape-result ownership, UPSERT, query-site macro and result/error mapping.
- `src/sql/sql.c`: actual MySQL `qry_at` at3818-3850, DB guard, actual
  `MAX_STRING_LENGTH` stack buffer, `vsnprintf`/return-length check and trace call.
  The `__NO_MYSQL__` stub at409 is not compiled.
- `src/core/utility.c`: actual `logit` at793 and its complete local helpers.
  Reached non-SQL logging/time/formatting/filesystem providers are genuine.

Actual config is `src/core/config.h:141`, buffer65,536. Actual `sql.h:144` and
`persistence_observability.h:14` propagate repository file/function/line123.
The real production escape owner, `sql_player.c:1045`, accepts a C string and
uses `strlen`; it is **source verified, not compiled/executed** in this component.
All these source/header bodies are pinned by the whole archive; compiler inputs
and dependencies have separate exact hashes. Repository SHA256 is
`cd84c2dbec51ee1fde58607fb962944fdf84b5163cc333100503abd1ef38ff3c`;
complete `sql.c` SHA256 is `fcb648fea1a64750d097556c4254e338a749c9727cb92e989298598650bf7ee0`.

## SQL double and final call proof

Only the permitted SQL boundary is controlled: the real provider's `DB` global
points to an opaque zeroed `MYSQL` token; the harness supplies malloc-owned
`sql_escape_string` output and a strong `sql_trace_exec_at` double. Escaping accepts
only safe uppercase ASCII, either identity or explicitly simulated2x expansion;
refusal/no-DB controls return null. The actual repository frees that allocation.
Trace records/checks the full query, byte length, label, drain flags and original
query site, then returns the selected control result. These are not MySQL escaping,
execution, receipts or durability. No client library or database is linked/contacted.

To select this allowed boundary without extracting/modifying the formatter,
`objcopy --weaken-symbol=_Z17sql_trace_exec_at22persistence_query_sitePKcS1_mbb`
operates on a **copied** full `sql.o`; original and controlled objects are retained.
Per-section authentication covers604/O1 and6,814/Og sections, including repeated
COMDAT `.group` entries by index. Every executable, relocation, data and debug
section is byte-identical. The symbol table differs only in the intended function's
`st_info`, GLOBAL/FUNC to WEAK/FUNC. Objcopy also re-encodes `.shstrtab`; section
names, indices and semantic metadata remain exact. This is not a claim of whole
object byte equality. Section GC removes uncalled SQL/server owners.

| Profile | Original object SHA256 | Controlled object SHA256 |
|---|---|---|
| O1,3,276,808 bytes each | `b33ff9b5728219695f07ae57438f78f244db5603472961c42817698e313d464d` | `b70c20547beb6282abcf3b50b02f6bb0d5cb4554360723c3246a6208b75ca830` |
| Og,6,093,752 bytes each | `392c37670a3400ff8822e6d2bd8b0eeeec046c8612fc0408c38ee9f9a49ac1c0` | `c9fdccfd66996d9a608fa469fa5a888e6ea1e08964e502f7b7c234f6142adc1b` |

Actual `qry_at` machine code is429/O1 and677/Og bytes, unchanged within each
original/controlled pair. Final ELF disassembly proves its call reaches the
strong controlled trace: O1 call7ba2→4974; Og call41b7ee→40ca8a. Final symbol and
relocation/disassembly dumps, link maps and undefined-symbol lists are retained.
No live `mysql_` dependency remains. Max-fit exact trace body/count and forced
trace-refusal propagation separately demonstrate the reached provider at runtime.
Six private integrity controls modify formatter code, its relocation or another
symbol binding, once/profile: **all rejected before execution**. They are proof
sensitivity checks, not additional SQL/native behavioral passes.

## Executed observations

Independent expected UPSERTs use literal string concatenation; they do not call
`qry`, `snprintf` or reimplement its formatter. Full trace bodies/lengths/counts,
actual repository results/errors, real logger output and counters are retained.
Each profile passes the same12 observations:

| Case | Actual result |
|---|---|
| Revision7 largest ASCII fit | State65,239; complete query65,535; one exact trace; `ok`. |
| First complete-query overflow | State65,240; query65,536; zero trace; `io_error`, `zone-story SQL state write failed`; actual formatter logs overflow. |
| Forced trace refusal at max fit | Same exact65,535-byte trace once; actual repository returns the write error. |
| Wider revision spelling | Same65,239 state bytes with4294967295 revision produce65,544 complete-query bytes; zero trace/write error. |
| Controlled2x-growth fit/overflow | Raw32,619→query65,534/one trace/ok; raw32,620→query65,536/zero trace/write error. These are simulated escape-output sizes. |
| Repository no DB | Escape double returns null; zero trace; `zone-story SQL state could not be escaped`. |
| Direct actual formatter no DB | `qry("SELECT 1")` returns false; zero trace; actual logger emits initialization error. |
| Controlled escape refusal | Ready opaque token, null escape output; zero trace/escape error. |
| At16MiB preguard |16,777,216 state bytes pass guard and escaping; expected complete query16,777,512; actual formatter refuses/zero trace/write error. |
| Above16MiB preguard |16,777,217 state bytes; `invalid`, `zone-story SQL state is oversized`; zero escape/trace. |
| Controlled embedded NUL | Three input bytes `A\0B`; C-string double observes only one, produces one;297-byte trace/ok. No binary-safe MySQL behavior is established. |

For no-escape/preguard refusals, empty-escape expected-query artifacts are only
hypothetical oracles; no query reaches trace. Decisive evidence is the actual
guard/escape/trace counts and result/error. No-overflow success means only the
explicit trace double returned true. Raw-state fitting size is not universal:
revision spelling and escaped-byte growth consume the same complete-query budget.
The repository's size guard uses `>`16MiB; no live column-capacity test was run.

## Commands, binaries and retained proof

Executed from the existing Windows-managed prep worktree:

```powershell
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' --exec env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B tests/async/quest_accounting_prep/test_story_history_sql_capacity.py --candidate c4f695b1aa90e978b1e977b702fc9fc6aaef6342 --git-dir '/mnt/c/Users/alexa/OneDrive/Documents/ChatGPT/NewDuris Max/.git/worktrees/NewDuris-Max8'
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' --exec bash scripts/format.sh --check --file tests/async/quest_accounting_prep/story_history_sql_capacity.cpp
wsl.exe -d Ubuntu-22.04 --exec env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B /mnt/d/Dev/Temp/quest-story-sql-capacity-publication-20261008/verify.py
python -B D:\Dev\Temp\quest-story-sql-capacity-publication-20261008\publish.py
git diff --check
git push origin codex/accounting-quest-prep
```

Ordinary Linux checkouts omit `--git-dir`; it maps existing Windows worktree
metadata without configuration changes. Runner takes explicit evidence/bin
parents and otherwise creates unique task folders on D:. Compile/run streams,
launch failures, nonzero exits and timeouts remain recorded. Whole deadline600s,
180s per translation-unit compile,90s link,30s runtime; timeout kills its process
group. No failed attempt is hidden by an automatic retry.

Flags: C++20/`-g -Wall -Wextra -Wpedantic -Werror`, maintained test defines,
actual `mysql_config --cflags`, function/data sections; O1 control and
`-Og -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all -fno-pie -no-pie`.
Link uses GC and pthread, **no MySQL client library**. ASan leak checking/halt
and UBSan halt/stack traces enabled. Actual compiler Ubuntu g++11.4.0;
mysql_config/headers version8.0.46. All8 final translation-unit compiles,2 copied
symbol operations,2 links and2 runtime executions exit0, no sanitizer findings.
Runtime0.373932s/O1 and0.473181s/Og; exact query/observation bytes agree across
profiles. Real time-stamped logs are separately retained, not compared as equal.

| Final ELF | Bytes | SHA256 |
|---|---:|---|
| O1 | 2,876,216 | `327dc889926a08a722c25e90840ab8ee014ba2ff819add8269866523e231e19f` |
| Og ASan/UBSan | 5,034,600 | `6e04eeafcd76056f1d04b2f1a1a76354fa12ca432c78092eed3530a161abff58` |

Evidence `D:\Dev\Temp\quest-story-sql-capacity-t497ar24`; build objects/depfiles/
maps/ELFs `D:\Dev\Builds\NewDuris\quest-story-sql-capacity\quest-story-sql-capacity-t497ar24\bin`;
verification/import proof `D:\Dev\Temp\quest-story-sql-capacity-publication-20261008`.
All1,334 source archive bodies/modes/Git blobs,560 compiler dependencies,
tools/toolchain and dependent libraries, final ELF libraries, build artifacts and
1,634 original worker artifacts authenticate. Standalone patch26,558 bytes,
SHA256 `0f9cff06c05cf8bfbd418d3553f651da2f436ec4e9d86ad3ddf3f947e6ec2201`.

Retain `quest-story-sql-capacity-feasibility-20261008`: actual complete strict
SQL TU compile and full-provider/genuine-logger link/runtime probe all pass.
Retain `quest-story-sql-capacity-l3ma4_4l`: all4 O1 inputs compile and objcopy
passes, then the initial verifier refuses repeated COMDAT `.group` names before
link/runtime. Its manifest/streams/input copies remain. The corrected final
verifier authenticates every section by index rather than dropping duplicates.

## Remaining authority boundary

The bounded optional component is reviewable; it proves the real formatting
channel can refuse a blob far below the repository preguard under the disclosed
SQL double. The shared owner must decide any write-path/capacity/error-handling
change and qualify actual escaping, connection/transaction/schema semantics,
whole valid story-history persistence and economic recovery integration.
This fixture's ASCII strings are capacity inputs, not native history activity.
No NPC/source/reset/full-boot binding, custody, reward, XP save, ACK, retirement,
SQL service, migration, journal or native/crash journey is executed or fabricated.
No server build/boot, production/shared source, existing test/helper/driver,
schema/registry/capture/decoder/Plan5/finish-plan/canonical HANDOFF changed.
Prior runtime reentry delivery remains closed. The actual continuing native Goal
remains **BLOCKED and unfinished**; this component does not resume/complete it or
add a primary adoption wait. Integrated primary final qualification stays separate.
