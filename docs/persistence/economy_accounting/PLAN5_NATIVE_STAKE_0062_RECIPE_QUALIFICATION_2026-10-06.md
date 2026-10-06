# Plan 5 native stake 0062 recipe qualification — 2026-10-06

The maintained full native stake method applied canonical0062, then asserted
that its migration head was0061. That stale guard stopped the original method
before SQL reader qualification. Separate owned fix
`53c46b132eb3074d5cd60e1c7a503ff498be10d9` changes the exact head guard and printed
head to0062. The original full method now passes both fresh engines, with all
assertions, compiler profiles, sanitizer flags and deadlines preserved.

Branch remains remote `codex/accounting-plan5`, in
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`. Exact base is
`09b7ceac7925fd9e144302219c0709cafbc7aa07`; result is
`53c46b132eb3074d5cd60e1c7a503ff498be10d9`. Sole owned file is
`tests/async/test_reconcile_economy_accounting.py`; its commit contains only
the two recipe-line corrections. The exact independently qualified overlay
was staged while preserving the separate snapshot fix and operator guide.
No shared interface or implementation change is requested.

The original command is:

```text
python3 -u -B -m unittest -v test_reconcile_economy_accounting.NativeStakeSQLTests
```

| Source | Result / observer seconds | Original log SHA256 |
| --- | --- | --- |
| Exact base, original method and reader | Exit1 /66.634588, one failure at the stale head guard, zero skips. Native controls pass first; SQL cuts do not run. | `ef05341c8f54df1393757a241bfe47e367476e36c0321dbfa3e9954b8e5dc7c9` |
| Base plus exactly the committed two-line correction | Exit0 /93.594576, original full method, zero skips. | `9e058561676a3faf39208bde28df24fa32bb14740f30c7c15bfff720188682c7` |

Connected engines identify MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 and
MySQL8.0.46-0ubuntu0.22.04.4. The unchanged native method applies the complete
immutable history and requires head
`62\t0062_economic_pending_claim_consumption`; it prints `through=0062` for
each engine. Both SQL and flatfile native controls agree on104 source grammar
cases,1,107 source-policy decisions, six original-link cases and four price
roots across two epochs. The encoded fixture hash remains
`1ed10a94436e14271b6bc07d8f17ebf364b10dcd770e1da1c6738df1d02c7281`,
and both binary hashes remain
`30318b724aae1dd53aac7f28738637d3a5d5a1f8dc71c49da6c650f8c442c7eb`.

Across the two engines the method passes90 read-only stake captures, eight
price captures,24 price CLI cases at limits0/1/100, selected count/result/index
representation refusals, index-density refusals, source-kind and original-self
refusals, and16 price projection scope cases. Both SELECT-only users reject
UPDATE. Seven captured source tables stay unchanged during reads and return
to the original fixture state after deliberate corruption/repair. Accounting
stays inactive. Native encoding/decoding and modeled SQL cuts do not establish
mutation or gameplay journeys.

Both source transports use offline image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
two CPUs/4 GiB, private workspace/tmp mounts, no network and no `.env`.
`PYTHONPATH=/workspace/tests/async`, `PYTHONDONTWRITEBYTECODE=1`,
`DURIS_REGRESSION_BUILD_CACHE=off` and
`DURIS_RUN_STAKE_SQL_INTEGRATION=1` are retained in the executed helpers.
Other inherited gates dispatch no additional method. Original outer deadline
is2,400 seconds. Both observers finish normally without OOM and preserve their
source bytes; the red command's expected exit1 is recorded separately from its
observer's exit0.

Protected evidence under `D:/CodexEvidence/accounting-plan5/bin/`:

- `native-stake-0062-recipe-red-01-20261006`, raw source archive SHA256
  `56c8b5413b5a8af749bad3910203f9343606d463888b89fbffee6fe058dde3fd`;
- `native-stake-0062-recipe-green-01-20261006`, archive SHA256
  `3e0e83df6d898587697717a96afdd669f3efe65c871174804534dadba4623b9d`;
- `native-stake-0062-recipe-final-seal-01-20261006/evidence.json`, SHA256
  `8b02f50226729abd99b0d2db1961124e58d6f1b732519848a5d10bf9e8e5f7e7`.

The seal binds3,125 committed code inputs to the green source,92 retained
artifacts/298,427,131 bytes, exact original commands/results, source modes,
staging preservation, terminal states and native output. Native tree
`f0ae5c63273e94035552a75a1b70596d5021e54d` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are unchanged. No maintained C++
input changes and no fresh production build result is asserted for this
test-recipe issue. The primary's newer
`26d7b66b86e1a38d09430386be257a065fceacd5` baseline repair is outside that native
tree and needs its combined candidate qualification.

This owned report, protected seal and expected remote branch are the primary's
local notebook curator packet. [The separate snapshot slice](PLAN5_PARTIAL_CLAIM_SNAPSHOT_QUALIFICATION_2026-10-06.md)
repeats this original method on the composed snapshot-reader source.
Authenticated opening-policy/original-PID reference, complete native/live-world
capture and activation verifier, actual producers/replay/lost reply/cold
recovery, typed erasure, full managed backup/restore/retention and release-host
budgets remain gates. No activation, production operation or release completion
is claimed.
