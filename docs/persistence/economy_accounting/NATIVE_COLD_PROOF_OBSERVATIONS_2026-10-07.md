# Native cold recovery proof observations — 2026-10-07

The pending-journal cold case still times out. A numeric-only diagnostic
establishes that at least one invocation completes the original SQL proof and
rollback boundary. It does not identify the later failed reconstruction or
publication check. No functional repair or release gate is claimed.

The private source changes only observations in `recover_cold` in
`src/world/quest_mobile_native_birth.c`. It preserves the actual helper call,
short-circuit comparisons, exception codes, transaction cleanup, lease reuse
and original decisions. Output contains only numeric stage/error codes, once
per stage per process. Missing output establishes nothing; logging can affect
timing. The source is not installed in the maintained branch.

Source archive SHA256:
`764509ffea486fda933834ddde967520894236a50834f1664fc75be24f19436a`.
Manifest SHA256:
`ee49bffeeec48c9d05ae37ff13dc925ab2f1cf40cf8a16174879d9a2eb3776b3`.
All 6,367 original members/modes authenticate; one source file changes.

Both original production Make builds pass with all 753 providers, one freshly
compiled provider, 752 dependency-authenticated reused objects and fresh full
links per profile. All nine original contracts pass. Build handoff SHA256:
`611b8a917467eadd223f5ef2a641a208319ed7fbdcd9359b7bb896333d95c638`.
These build results do not establish runtime recovery.

The original genuine driver and its callbacks, flags, schema-63 preparation,
case order and 55/60/70-second budgets remain. Fresh compilation/linking creates
the actual new driver ELF:
`16134c2cd775bb1a75391f3a5bc0a867f0433c52f9d2a3d79725a6bcb9d0f31e`.
Its genuine warm/fault capture binds the new executable; earlier ELF/journal
evidence is not relabeled or rewritten.

| Original case | Actual result |
| --- | --- |
| MySQL warm publication / full cold restoration | Both exit 0; cold 1.279 seconds |
| MariaDB warm publication / full cold restoration | Both exit 0; cold 1.380 seconds |
| MySQL origin-INSERT fault | Expected and actual exit 17; 1.281 seconds |
| MySQL pending-journal full cold | Exit 124 at unchanged deadline; 60.053 seconds |
| MariaDB fault cases | Unrun under original fail-fast order |

Actual pending-cold stderr contains `stage=9400 code=0` and
`stage=9411 code=0`: the helper returned zero and its original complete
SQL-proof/rollback boundary passed. No numeric refusal was captured. The
subsequent recovery/reconstruction boundary remains to be observed; no SQL
wait, lock, generation or constructor defect is inferred from these lines.

Primary independently authenticates all 124 evidence files, all 1,779 regular
native archive members/modes, the actual ELF, original case outcomes, numeric
stderr and owned cleanup. Private evidence lives under
`bin/tests/native-full-cold-proof-diagnostic-primary-20261007/combined-1/`.
`NATIVE-HANDOFF.json` SHA256 is
`f7dbd30d7f01a817b50932cd486696b775e9b381e0ec55b26275f67af2fc8fb3`;
`FINAL-EVIDENCE-PINS.json` SHA256 is
`adb2a03373b39c5694cdc675c76652e0ee91d49eb5f20d91582cfc3d7ebf8b45`.
All exact owned containers/volumes are absent. No production data, accounting
activation, deployment or live game interaction occurs.

The separate unchanged-ELF GDB and SELECT-only observations remain preserved.
They established the sampled SQL stack and transaction turnover, respectively;
neither identified the actual failed recovery predicate. Complete recovery,
maintained producer integration, physical gameplay and R1–R8 remain unfinished.
