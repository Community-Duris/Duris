# Reader transport primary integration — 2026-10-11

The reader rejects nonregular input before reading, bounds bytes after reading,
rejects duplicate JSON fields, and catches actual parser recursion refusal before
any operator view. The measured test-only fixture uses16384 depth and requires a
real parser RecursionError before its unchanged42 CLI checks; production has no
new depth policy. These changes preserve the other44 script callables and all
149 original test methods. Four methods and minimal existing-row registrations
are added. The separate origin-output-limit fix is excluded.

Actual Linux evidence: original149-method suite146PASS/3optional unqualified;
new six-case regression on the original reader has four failures and two passing
controls; corrected153-method suite150PASS/3optional unqualified; all6 component
cases including real FIFO and all19 existing-row selectors PASS without skips.
Only the full matrix's registration listing executes; full matrix NOT_RUN.
Optional native SQL and two large audit-budget classes remain unqualified.
Windows scoped execution retains an honest FIFO skip:149 original methods146PASS/3optional skips;153 candidate methods149PASS/4skips; component5PASS/1FIFOskip. Windows registration listing only; no Windows registered-selector run. The first failed sandbox TEMP attempt remains preserved. Windows FIFO skip is never PASS.

Tested base `2206f446cd8bb1481664046dcddf7103a4912e83`; published Origin parent `9d288f175fbedaec7f439805a17e32a2fe8ddbd7`; tested manifest `3162aa42d048a0b2eb88d9533a6435174c3dfae0be238660dd9c2bac683e05da`; publication-control manifest `78996409de4887303a7a6dfffdd490d87e42e12699950f4b3042ad1f3b8f1fee`; component freeze `e640dd85b37216743753cd5e64bea92216f6623d6da84580b841addc8b7d57f4`;
Linux result `87a8a81863cccf2c5b4eea8b26bd585bab0655aaac91adbc4b21bdd814bf55c9`; Docker image `sha256:4994cc50a09a4acd40462ff412f3c3df21fc7c23a3f298f7fa725f0e2a350fb3`. Evidence remains at
`tmp/plan5-reader-measured-parser-root-20261011/linux-results/`. The482 source pins retain479 unowned pins; only the
three registered reader candidates change. The fourth owned path is the existing
regression row. All931 writer policies/2911 census entries/2853 mapped sites stay
unchanged; coverage incomplete, release BLOCKED.

This is an offline reader/unit milestone, not full Plan5, protocol or release
qualification. Current2206 combined native build remains PENDING here; older
native evidence is not promoted to this revision. SQL durability, restoration,
active gameplay/authority, full host and remaining release gates stay OPEN.
Accounting activation and admission CLOSED remain unchanged.
