# Complete frozen native literal allocation admission - 2026-10-09

The private frozen-literal provider and real raw allocator now admit decoded copies,
spellbook replacements, raw strings and description nodes before allocation. Full
original metadata and exact concatenated duplicate-key semantics are preserved by
an allocation-free comparison; original metadata functions stay unchanged. Actual
MEMCHK headers and old/new string overlap are counted. The original object pool and
configured affect pool use genuine nonfatal descriptor/list/page reservation and
acquisition, without RNG, UID issuance, callbacks or native enrollment.

The allocation-free pool observer measures CURRENT descriptors, registration nodes,
headers and mapped pages. Successful raw heap output excludes pooled slots/pages.
Callers retain the initial pool allowance once and refresh on every return; a
capacity reserve can persist after refusal. Raw/native output transfers only after
complete success and cleanup follows original private ownership.

Independent RAW/final source review passed: full original C prefixes, three exact
header insertion inverses, hashes, tokens and preprocessing. Evidence:
tmp/native-literal-allocation-bounded-owner-20261009 and
 tmp/native-literal-allocation-integrated-20261009. The isolated registry retains
931 policies/399 authenticated source pins and zero new/unmapped sites.

Cold DB restoration/adoption/enrollment and full four-route/root integration remain
open. Whole object/affect pools must pair with genuine pool-excluding census for all
covered flat warm/cold stages before aggregate selection, avoiding duplicate slots.
Original successful native world ownership transfer remains the body-release cut;
no extra perpetual world-inventory gate is introduced. Native tests stay deferred.
Accounting inactive/CLOSED, coverage incomplete, release BLOCKED, primary goal ACTIVE.
