# Projection contract default-argument parser repair — 2026-10-06

The complete-startup candidate passed eight original source contracts, then the
wallet-root projection check failed before evaluating its publication invariant.
The parser took `expected = {}` in the function parameters as the function body.
The helper still names the real `publish_projection`; this was a parser failure,
not a rename or an accounting rejection.

The repaired parser skips balanced parameter parentheses before locating the
body. Two regressions cover the empty default initializer and a nested default
call followed by the compare-exchange body. The original assertions remain;
the scope version must precede both the existing store and, when present, the
compare-exchange publication. Production implementation and admission policy
are unchanged by this issue's commit.

## Primary evidence

- Original private complete-startup archive: `8aa61ad0552e95323bac3ac9e6fccd1291d55b74e2870a340fc3002e6f793ec4`;
  manifest: `6199ae438464dcf4492f8da4f36977b715c3cab63b7f64cee995dff3407186ee`.
  The original nine-contract batch retains eight passes and this one error.
- The exact failing method was reproduced using its frozen test and source;
  the repaired method passes against the same frozen implementation. All seven
  methods pass against the maintained checkout, with zero skips, using
  `python -B tests/async/test_economic_sql_wallet_root_projection_contract.py -v`.
- Evidence: `tmp/native-complete-boot-parser-primary-20261006/PARSER-REPAIR-PROOF.json`,
  SHA-256 `f8380141c4e7efb6e812c8051ba1539869ba3506a91745d92d7d2f9aeda5c49e`.
  Original failure and repaired-method/full-contract logs are preserved with
  hashes in that receipt.
- The new private successor changes only this test among all 6,361 authenticated
  members and modes. Archive: `3945310cde086154b206a11d3f32e384a8d16793df1a8e2d4dcfd767d2349609`;
  manifest: `110158516cce289d635dafdcc8b2efd2d293147b8701320f0bd04b832b40eb6f`.
  Every production source byte remains identical to the failed predecessor.

Full startup-source compilation, all nine successor contracts and genuine cold
world/journal journeys remain qualification work. This narrow test repair does
not complete a Plan, R1–R8, writer coverage, activation or release gate.
