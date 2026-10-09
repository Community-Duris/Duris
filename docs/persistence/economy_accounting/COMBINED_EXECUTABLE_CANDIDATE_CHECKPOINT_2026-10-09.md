# Combined executable candidate checkpoint - 2026-10-09

Source: `ec632155cca4a781a3014eba9a71815011f50ddd`. Both fresh production profiles compile and link; the existing
disposable inactive flatfile boot preflight passes. This is a development
candidate checkpoint. Original Plans2-4, full Plan5, R1-R8 and release acceptance
remain incomplete; admission and production accounting remain closed.

## Exact results

| Profile/check | Result | Binary SHA-256 |
| --- | --- | --- |
| MariaDB production compile/link | PASS | `1f6ef03d605b56273b781f9bd61a4e8cd71abac1e3f31ba0f71b0f48becac034` |
| Client-free flatfile production compile/link | PASS | `5f3337ff1abaf2e2a87521ae2a511f09802e7adac70cba20fab5e9b8b2086b3f` |
| Existing flatfile inactive boot preflight | PASS | Same flatfile binary |
| SQL disposable service-load smoke | UNAVAILABLE | Docker WSL integration unavailable; image availability unknown |

Compiler: GCC13.3.0 with the original strict production warnings and hardening.
Each profile has separate freshly exported source, objects and output. Archive
SHA-256: `5d519dc4dd6320c8ad58d092a3c27db141381eeb8da9b3d7c026681a5ebf13ce`.
No unrelated WIP, runtime/private data or private candidate packet was exported.

Both full compiles initially reached the linker, where the private path workaround
also changed an already valid `/usr/lib` token. Those failures are preserved.
The corrected retry changes only complete `/lib/x86_64-linux-gnu/` path tokens in
private copies of the two installed GNU linker scripts. Every referenced genuine
ELF/archive is checked, including `libc_nonshared.a`; system files are unchanged.
All 3390 archived regular source files are hash-verified for each
profile before reusing its freshly compiled objects. Neither retry recompiles a
source file. The successful binaries are copied into retained Windows evidence
before the WSL session exits and their hashes are verified again here.

The smoke executes the candidate's exact tracked
`tests/async/test_flatfile_boot_preflight.py` (SHA-256
`cd454f0ed34d5e8f066a175ad7c7a64ebd65e543e25b6aa719a2a4abb90e0638`). Its three healthy game-loop boots verify HTTP readiness,
SIGTERM exit0/normal termination, initialized missing/stale UID authority refusal,
exact authority restoration, controlled missing-world exit, and exact0700
disposable authority topology. It performs no accepting accounting/player journey
and supplies no crash/recovery, SQL schema or gameplay qualification.

## Reproduction and handoff

Export `src`, `Makefile`, `migrations`, `scripts`, `tests`, `lib`, and `areas_mini`
from the source commit into two separate clean build directories. For each:

```sh
make -C SOURCE/src -k -j2 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production \
  OBJDIR=SQL/objects DMS_BINARY=SQL/dms
make -C SOURCE/src -k -j2 PERSISTENCE_BACKEND=flatfile BUILD_PROFILE=production \
  OBJDIR=FLAT/objects DMS_BINARY=FLAT/dms
python3 -B SOURCE/tests/async/test_flatfile_boot_preflight.py --server FLAT/dms
```

Use absolute SOURCE/output paths. This WSL installation additionally requires
`EXTRA_LDFLAGS=-L/absolute/owned/linker-aliases-corrected` with the private genuine
linker-script correction described above. This is an environment workaround,
not a source or production-policy change. Actual commands, compiler/library
identities, timings and retained binary locations are in the JSON evidence.

SQL operator handoff still requires a genuinely prepared disposable schema65
restore candidate and its existing private socket-only service-load owner.
Plan5 retains backup/restore and independent qualification ownership; no shared
service or fabricated restore marker was used. Major-plan broad tests remain
batched. Genuine source/factory admission, full prospective32MiB overlap,
publication/terminal/ACK and stopped legacy enrollment remain required. Current
registry coverage/census are false; a working inactive binary does not complete
accounting. The selected/unselected source scope is retained in
[the combined source handoff](COMBINED_ACCOUNTING_CANDIDATE_HANDOFF_2026-10-09.md).

Evidence directory:
`bin/tests/combined-accounting-candidate-primary-20261009/build-successor-ec632155c/native/`.
Local evidence and binaries are retained, not committed as large Git artifacts.

| Evidence file | SHA-256 |
| --- | --- |
| `BUILD-RESULTS.json` | `bb03753972ab3d1750cc518c6abbeb8a615b52caa97bf584c3ccee30efab5bed` |
| `LINK-RETRY-RESULTS.json` | `d5849b478342aad63c02f220e36898ce27ae60892087f53bcb86f7169c4f2569` |
| `FLATFILE-SMOKE.json` | `128a6c7bee7c402255f783b36a50c9600d7a8efde4c1e01df34130744ceb25b6` |
| `mariadb-build.log` | `c0d7c62283b7afc0ac76ffc17bab07b48f86093ed9a2b59499e0b978ba2115cd` |
| `flatfile-build.log` | `c8b7d8cb0ba4654d6ff2bcb934bd9506c9b9878b0821ef1c973e0d84e0278a25` |
| `mariadb-link-retry.log` | `00fd7fd68625119a547b49e13b893742b9af25acfaaf7114417890baf2389b29` |
| `flatfile-link-retry.log` | `c2a8cb7754e719aaf2214ea6f6734689af62d3b4eddbf877abe621a9c96d5861` |
| `flatfile-smoke.log` | `d0c518234e49e57855d211a12da29264c76a1348966babbc502baf7e3e2f162d` |
