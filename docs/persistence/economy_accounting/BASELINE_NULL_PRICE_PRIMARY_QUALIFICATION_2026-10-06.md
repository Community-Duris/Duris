# Baseline absent-price verification — 2026-10-06

The original baseline receipt verifier omitted normalized `realized_price_copper`. A corrupted row with a sale price could therefore pass the original baseline replay proof even though the canonical baseline has no sale price. The maintained owner now requires SQL NULL; the original corruption vector explicitly sets this field to1 and requires refusal. This repairs receipt verification without changing accounting activation or native balances.

## Exact maintained qualification

Base HEAD: ad2ebe4dbe803f429fa5195ad4671f94499f1f29. Exact maintained source SHA256: 87bf49198082ab22e706f50a3adb8c45ef29088947e3999ec1d9dcba1f981d9d. Original harness with one added corruption case SHA256: 399ee07b88b743dab3cdb0ad39796c41d593c12468c35ddfc63d72408f8457fa. No private composed baseline helpers or substitute headers were used for this qualification.

The complete original ASan/UBSan SQL baseline suite passes on fresh canonical0062 MySQL8.0.46 (88.343s) and MariaDB10.11.19 (47.797s). Both engines pass actual migration/runtime verification, the price-corruption case and original retention/replay,63 before-query/10 hidden-write-ACK faults,30 replay-query faults, all three concurrency modes,1153 apply/570 replay allocation faults, initialization/exhaustion and maximum batches. MariaDB's original same-ID1213 attempt-2 policy retried the same original command and passed. Original900-second executable deadlines and all sanitizer/assertion controls remain.

The original flatfile refusal executable passes. The actual baseline owner production objects pass both original Make profiles, MariaDB and flatfile. Full server builds remain at the coherent major integration boundary; these objects are not whole-server or gameplay qualification.

The initial original runner closure failed to link because the maintained item codec requires the real shop recovery manifest. The runner now includes `src/economy/shop_trade_recovery_manifest.c`, exactly the sole real provider added to the successful native compilation. Flags, wrappers, original providers and executable deadlines are unchanged. The initial link failure remains recorded. Runner syntax and exact provider-list equivalence are checked separately; the full native outcomes above used that identical effective provider list.

Evidence: bin/tests/baseline-null-price-primary-20261006/HANDOFF.md, qualification-results.json and FROZEN-RECEIPT.json. Receipt SHA256: 8073c7fd30210fd64c5234d9018f5714cb68e6719e0c4ae7070576326e452501; source archive c2c9af6f7519f270df0ca226d8e53662c328130557d5d884fb51f34939fc89df. All37 sealed artifact hashes are verified. Only owned no-network containers were used and removed. Binaries, logs and private fixtures remain uncommitted.

## Remaining original scope

This solves the normalized baseline-price omission. The broader auction/HRT/quest integrations, original claim-policy/PID proof, producer/ACK/cold journeys, writer registry/matrix, activation and R1–R8 release gates remain open. Coverage is incomplete and release stays BLOCKED. Inactive behavior and the declined inactive spell path remain unchanged.
