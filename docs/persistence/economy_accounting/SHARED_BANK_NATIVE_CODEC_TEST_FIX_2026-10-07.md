# Shared-bank native codec regression recipe fix - 2026-10-07

The original shared-bank accounting regression failed at link after the native
item codec integration. Its standalone recipe omitted four genuine owners:
native quest cost, native quest coin give, SHOP recovery manifest and lockpick
retirement continuation. Link those production providers; preserve every existing
assertion, sanitizer/compiler flag and the original 30-second execution limit.
Keep the changed test script in canonical LF form across checkouts.

The corrected original suite passes with ASan/UBSan on both the current coin
candidate and, separately, the authentic published pre-coin source at
`84607c94cb26db4047b0ef9d599274f8c5ed8294`. The latter establishes that this fix
can be published independently of the unqualified coin recovery work.
Compiler dependency analysis identifies 78 actual local sources/headers; all
match the published baseline after explicitly labeled Git LF comparison. Raw
transport identity is recorded separately. The test AST changes only by the
four real provider additions. No production owner or test oracle changes.

The independent receipt is
`bin/tests/coin-maintained-native-build-primary-20261007/shared-baseline/SHARED-BANK-MILESTONE-HANDOFF.json`,
SHA256 `760f100c214cb16ad638f46829ef0744914865f065b696e706dd3612bb28acba`.
Its original suite, dependency capture and terminal logs are pinned together.
The original link failure and separate current-candidate result are preserved.

Reproduce with `python3 -B tests/async/test_coin_transfer_shared_bank_accounting.py`
using the supported native compiler and sanitizer environment. This is a focused
real-source regression. It does not establish SQL/gameplay/recovery completion,
flatfile domain parity, activation or full R1-R8 readiness.
