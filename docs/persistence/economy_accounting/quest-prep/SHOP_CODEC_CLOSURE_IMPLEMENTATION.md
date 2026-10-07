# Maintained shop codec provider closure — 2026-10-07

**Implemented, executed and pushed; final coordinator review pending.** Code
commit0b0075724457511f6a7c6599489958cf45cd828a, base
b33f39d037b886b3f299b8073274423c2c8cdc34. The code commit authors exactly one
maintained file, tests/async/test_shop_trade_command.py, with three additions and
zero deletions. Approval is published at primary
e6c1fa35fb41674c2a0ce425354e61a629c0e879 in
SHOP_CODEC_CLOSURE_REVIEW_2026-10-07.md. The earlier investigation/reservation is
historical evidence, now followed by this separately approved implementation.

## Correction and independently applicable import

The original complete codec runner omitted actual providers referenced by the
current item-transfer codec. Add only rel("lockpick_retirement_continuation.c"),
rel("native_quest_cost.c"), rel("native_quest_coin_give.c"), immediately after
original critical_command.c and before original -lcrypto. All ten existing source
inputs, complete HARNESS/all46 assertions, semantic fixtures, compiler/link flags,
subprocess controls and temporary cleanup remain byte-identical after removing
those three added lines. No production/header/manifest/registry/schema/authority,
R10 shop extraction or Plan5 file is authored.

| Pin | Original | Repaired |
|---|---|---|
| Test Git blob | 51449801c538c412be86671bc4789dbdb81b6602 | 0a46981af92ec8942d40554736b6fde5eeec192a |
| Test SHA256 | faf2d31b66cd77ab7678d6b4e69ddd96440b9c90f7d1468e2c8d2fc56cce9ca1 | 368401f4c651fc3553f1f1336a9e090ed946235f04f57d55547480faf106c2d3 |
| Canonical bytes | 11167 | 11307 |

The actual standalone code diff applies with git apply --cached --check to a
private index loaded from **bare current primarye6c1fa35fb41674c2a0ce425354e61a629c0e879**.
After applying, git write-tree returns51aaf7b87c77a375c264372f2bf7e7ce0116d62f.
The difference from primary is ONLY the repaired test, with its mode unchanged.
No prep ancestry, optional historical QP02/QP07 or architecture R0–R10 package is
required. Primary may fetch and cherry-pick0b0075724457511f6a7c6599489958cf45cd828a
independently. Import code only; documentation checkpoints are separate.

Actual package SHA256377e69502b9a123f969f2d744eaf0ceb39f1103929b85d1df669347c5683e84f.
Bare-primary archive4c236451a558c324dcf6a2d433bf722306e80ff3f6a7458e9a4232f56c299589;
bare-primary+repair archivec22945772f9a4e1a65442dda1e46dda48deca80ba9545ebb61160279327b4c76.
Unpacked source authenticates all6446 canonical file/link entries, their bodies,
sizes and modes before and after execution; only the test differs between archives.

## Actual maintained driver execution and current header authentication

Networknone, 2CPU/3GiB owned container quest-prep-shop-repair-e6c, unchanged QA image
sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b,
Ubuntu GCC13.3.0. Immutable imported tree unpacked as /candidate; private evidence
bind /evidence. No DB/player/game/server start or operation.

Actual proof command:

```text
docker exec quest-prep-shop-repair-e6c python3 -B /evidence/run_verified.py
```

It executes the maintained entry point directly, cwd/candidate:

```text
python3 -B tests/async/test_shop_trade_command.py
```

**PASS**, exit0 in7.847s, output
`[PASS] bounded recoverable shop-trade command codec`.
The complete actual compile/link exits0 in7.721s, followed by the original
check=True executable invocation and original temporary cleanup. No accepting
definition, source extraction or reduced harness is substituted.

For artifact retention only, an ignored PATH compiler observer copies the original
generated harness and successful ELF before temporary cleanup. It delegates to
/usr/bin/g++ with EVERY requested argument unchanged, forwards original compiler
stdout/stderr and exit code, and records the actual invocation. It changes no
flag, source, define or link library; the maintained Python controls remain intact.
Original flags are `-std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc`, link-lcrypto.
The original recipe supplies no explicit timeout; none is inserted. No NDEBUG,
sanitizer, optimizer or linker-GC alteration. Real compiler SHA256
1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769.

Generated complete HARNESS retains SHA256
f054d84ec7e76103001404bab2dc723cac42fdf34cdccd30dd0bc12b5c1ddf0a and46 assertions.
Actual current ELF SHA256
dffd788d473c457f93ceee34082bfd3880c718586f1e3d52eb83b5569369e6d2.
The separate compiler -M dependency observation authenticates385 distinct inputs,
including56 actual current repository headers against canonical imported bodies.
All13 actual compiler-source hashes and these56 header hashes also agree with
the earlier e6 investigation; new primary flat-coin claims/proof-reader interfaces
do not change this codec closure. System-header hashes are retained separately.
This current import/execution proof does not relabel the earlier private e6 ELF.

Post-run integrity command:

```text
docker exec quest-prep-shop-repair-e6c python3 -B /evidence/verify_integrity.py
```

PASS: exact original script restored by removing only the140-byte three-line
insertion; original harness/assertions/controls intact; current13 sources/56 headers
authenticated. git diff --check PASS; no unchanged broad production build is needed
for this approved test-only repair. The owned container is stopped normally.

## Retained evidence and scope

Ignored host root:
C:/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max/bin/tests/shop-codec-repair-e6c-20261007.

| Private artifact | SHA256 |
|---|---|
| RESULT.json (actual commands, package/export, inputs and terminal) | d39d265421aee47937c44a379a671b42e24e5ab51f9100bdc47259308f07c625 |
| INTEGRITY.json | a2245aeec66bf18d77d5eb5c24e0906edc566052a73207a07342453a08587c47 |
| compiler-observation.json | 25b9f3adc01c55ab480d0e72f2bceb6f46372d13f6bd789a6d36654e5839f6dc |
| current-dependencies.txt | 3d2efbb30116e71e900c32a3f0d4e19a71789a48d593893e0e8b54a37b1e758c |
| maintained-stdout.txt | 387fcf9a960261fac22728a2bba1774e5f9343f59be74a214d22bb26e4ad1422 |
| run_verified.py | b53ad4a09b90e78b1891766f916a82d42028f2e3374bf483cb458cdaa90c785f |
| verify_integrity.py | c9aecfaf9837c7304338d321a18d18a79b3ccd6726ebe3199983505028fd3e37 |

The original e6 FAIL/linker stderr and separate private closure PASS remain sealed
under shop-codec-closure-e6e-20261007; their exact pins remain in
SHOP_CODEC_CLOSURE_INVESTIGATION.md. They are not overwritten by this maintained
success. Raw logs, source archives, compiler observer, code packages and ELFs stay
ignored and uncommitted. This document and HANDOFF are the authored owned evidence.

This closes the bounded original command-codec fixture dependency, not an R10
production defect or the separate live-route source-contract failure. It proves
codec/fence/version/result assertions, not actual SHOP/quest/backend/source/birth/
custody/publication/ACK gameplay. Coordinator final review and primary adoption
remain separate. Actual continuing Goal still reports **BLOCKED**, not resumed or
complete; genuine lifecycle/reset-born source, native capture/ACK boundaries,
delayed settlement and durable held-charge/refund/paired-retirement prerequisites
remain unfinished. Full Plans1–5/applicable R1–R8 finish line is unchanged.
