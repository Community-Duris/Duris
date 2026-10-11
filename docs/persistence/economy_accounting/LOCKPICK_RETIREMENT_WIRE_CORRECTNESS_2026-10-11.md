# Lockpick retirement wire correction - 2026-10-11

A valid lockpick-retirement DTO has target_root=0 because there is no destination tree. Two existing encoders replaced zero with selected UID on their non-multi-root fallback, producing wire rejected by the unchanged strict decoder. The correction retains zero only for lockpick_retirement; other continuation bytes and malformed nonzero-root rejection are preserved. This is the explicit wire correctness behavior change.

The first local commit selects the formatted existing C prefix `c15587e4f60d05567941dbfd762fcf00b4f47f9cd4f6b983e410245e16f24236`, read back as the exact Source9 prefix. The third corrected expression belongs to the later additive complete encoder. Final combined Source9 evidence does not imply a separately linked intermediate prefix-only tree.

Actual wire-corrected Source9 freeze `f9db6520...11be0` completed production make all on both backends: SQL668.989328049s and flat699.296125517s. Native RESULTS SHA256 is `0f93e453c77e5682a059a53f6bfccf98894205df029e938bf40a65b1f5d2713c`; host dispatch `a51626887c681567cd5b276dccaf52e43e92c952f93fa482aed640441c8a8c36` also records terminal exit0. Ordinary focused, legacy ASan and deliberate unsupported-profile cohorts passed. These are combined eight-file compilation/link and value-fixture results, not services or NPC gameplay.

Separate standalone v3 RESULTS `23ded2c67f85145cfa4fef092dbfb4ca5944d7ce6b24364f34f21f71660cff6c` records terminal PASS108.621747686s: SQL-header and flat each15 groups/2653 refusal points/2653 reservations, required lockpick and malformed-binding markers, and preserved source identity. This is the actual runner `f5f7c546...5c367` receipt; the Source9 bundle's original Python `8feb` NOT_RUN field remains historical and unchanged. Registration can record108.622s with the unchanged5400s deadline.

Metadata freeze `c9dc6facc11a6c454586516e668a044f1907cf5f6a41aedd2ac1dfc5aae54b70` records generator/validator/check PASS and all10 paths restored:494 pins,931 policies,2911 occurrences,2853 sites,0 unmapped. Only the real builder census/site/definition2736 to2739 coordinates change. The capture contains all8 Source hashes; each local commit must refresh its current subset rather than import all8 prematurely.

Own Source1 plus writers/matrix, this report and tracking3:7 paths. Refresh1 existing pin, preserve493 of494; no new registration. Separate local commits are pushed only as the qualified combined final Source, after Root's actual current checks.

The held-retirement extraction is already adopted and normally published at `97ded8571375f3fdc6168487a13120f65caedbec`; its full protocol qualification remains open. Full protocol and R1-R8, architecture, gameplay, actual database durability,32MiB whole-host peak,cold restore/shutdown and release remain open. Accounting remains inactive/CLOSED; the declined spell path is preserved; coverage is incomplete and release BLOCKED.

Integration parent: `97ded8571375f3fdc6168487a13120f65caedbec`.
