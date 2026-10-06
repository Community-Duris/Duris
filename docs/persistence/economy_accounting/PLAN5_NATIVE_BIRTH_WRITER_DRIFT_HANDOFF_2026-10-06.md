# Plan 5 shared writer metadata handoff — 2026-10-06

The published combined native-birth candidate has an established source census
and matrix failure. Native qualification continues independently; this handoff
does not change shared writer contracts or treat source inventory as release
evidence. Primary owns the review and repair below.

- Plan 5 branch: remote `codex/accounting-plan5`.
- Exact tested source checkpoint: `450b60fe08b89523095106ca0880cc1558e5eacd`.
- Imported primary: `2c4e17f363ecff0f0d7eb3451ddb229abdd63d9a`.
- Native tree: `244559a07b8d046839c4fb9ff174685351d8f386`.
- Migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61.
- Owned files: this report and protected observation helpers only. Shared
  registry, matrix, coordinator, producers and activation remain untouched.

## Established observations

```text
python -B scripts/validate_economy_accounting.py
  exit 1: economic writer census drift; review new/changed sites
python -B scripts/generate_economy_writer_coverage.py --check
  exit 1: writer coverage matrix is stale; regenerate it
python -B scripts/validate_runtime_compatibility.py
  exit 0: current runtime schema metadata, migration 0061, 228 tables
python -B scripts/validate_economy_accounting.py --release
  exit 1: writer has no executable evidence
```

The actual scanner's `(path, family, excerpt)` multiset changes from
2871 registered occurrences to
2876: 10 added
and 5 removed signatures. These are
lexical candidates, not a count of new real writers or complete semantic
coverage. Exact occurrences/excerpts and before/after source hashes are retained
in protected `shared-writer-drift.json`.

| Source | Added signature occurrences | Removed signature occurrences |
| --- | ---: | ---: |
| `src/item/item_movement_transaction.c` | 3 | 0 |
| `src/world/db.c` | 7 | 5 |

## Narrow primary-owned update

1. Review the real reachability and authority ownership of the changed sites.
   Update `writers.json.census` to the actual reviewed source multiset and the
   affected `writers[*].sites` to reviewed `(path,line,family)` identities.
   Reuse the accepted native birth/quest source and preparation handoffs rather
   than repeating the zone survey. Do not infer semantic completeness from
   lexical equality or discard previously reviewed routes.
2. Refresh `writer_coverage_matrix.json.candidate_worktree_evidence.source_pins`
   for the changed published inputs below and add new owner inputs where the
   primary's reviewed composition requires them. Its base/source attribution
   must identify the actual reviewed candidate.
3. Regenerate the matrix with the existing generator after that review. The
   consumers are `validate_inventory`, `generate_economy_writer_coverage.build`
   and the existing writer-coverage contract suite. Derived `source_state`,
   `lexical_census` counters/mapping identities, `routes[*].source` definition
   lines and repository attribution must agree with the reviewed candidate.
4. Preserve existing source/custody/receipt and inactive invariants, route IDs,
   status and backend evidence. Do not promote `coverage_complete`, executable
   evidence or release status from a pin refresh. Real source and backend
   qualification remain required before any route changes status.

Required checks after the primary update:

```text
python3 tests/async/test_economy_writer_coverage_contract.py
python3 tests/async/test_audit_accounting_invariants.py
python3 scripts/validate_economy_accounting.py
python3 scripts/generate_economy_writer_coverage.py --check
python3 scripts/validate_runtime_compatibility.py
python3 scripts/validate_economy_accounting.py --release
```

The normal/source checks must pass; the final release result remains incomplete
until original R1–R8 evidence is present. This request adds no schema/interface
field, storage format, activation gate or new test framework.

## Changed existing source pins

| Existing pinned input | Matrix SHA256 | Actual published candidate SHA256 |
| --- | --- | --- |
| `src/Makefile` | `2ae8f1f3e8cba75233c75a5680e1ca423abf5e4cf5b4e1f1cf56b1b7f5a1d00e` | `5251188e6181fc237d79f29959a458453f830122665494a40726ff383fe440c0` |
| `src/economy/economic_command_admission.c` | `1cdb4fe567f3aa4271360b36757733720848a65f9ce0beaa011d123d63bb3d9b` | `431baf195b7e8249da5f0a28b0b68d794deea385361ccf0b1cbb46c74eecc675` |
| `src/economy/economic_gameplay_authority.c` | `3ecf407a223f960291ea3e142b23128e2eefade0344f1264e4d5c2a5e57729be` | `2e89e38f2df237209f80d3b5c4171d4f50ffb884067adb7b5de57ca000d00d59` |
| `src/economy/economic_gameplay_authority.h` | `9163499b28c38fa61099b8b66bbd09a42d533bf139c5c2256d51ce0a2cca080d` | `0696358bfce39551ee5ef8636da2871af1d1b04ccf8d4f7543b231c8106bd1c6` |
| `src/economy/item_transfer_accounting.c` | `9193cd90f797917ab0c150744d7e219347ac00711cd65c49c7eb0a1c20558476` | `cb58d2254133d397e56c5030c7d368d5cce9b963069b1636d65123478a3effbb` |
| `src/economy/shop.c` | `bb872e42d47c482fe8c68fc3cbdbe4aa784e1232a6a220ad3603cb18ddd42618` | `de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393` |
| `src/economy/shop.h` | `1e393704298fb48afe21a90b871b3396ea23b4fe1e8a7dca95518b81145f54ed` | `8fcceb4db03ab4f0db88489ae9aa0b9e8edb8bfc53de092fb03be11460a5a610` |
| `src/economy/shop_trade_recovery_manifest.c` | `20ed4076709855dc27cce60fea1c5f4f09c674bb0d8ce27ca29e2feb57cd47b7` | `1a1543b2f8e26aa665eea052e0e6184dcd7e02a2b3da1680450737fc231139c3` |
| `src/economy/shop_trade_recovery_manifest.h` | `9174ca1fac0372a0f36037ac2150f84355da9d3acff74fdbf71e0192c8809db9` | `215424403244168829006518cc3e858764ed0dd55501ee541211854ad0ccbed1` |
| `src/item/item_ownership_runtime.c` | `a0a58d74b75fdbc78d4987a83804bd23ffbcf7bea96fefb06b78ea8b13ad5a25` | `b4e87dff4aafe2f360d927efc28700362d11015d7181b763c8a3b4c33bb7c388` |
| `src/item/item_ownership_runtime.h` | `19ba1140abdb70406ebd4f351f230715c6b6b15405c44688f315e76e13d2cafb` | `8fbc775fa4e6919834e56c1e1059b7abba1842753c59eaadbb42342d3c57871a` |
| `src/item/item_transfer_command.c` | `7dacb22ff89cb55d5fe12869f83589ac71e1bb61e1eec50e9f44c8ebf620059c` | `874e2edc890a377cf7afd970102387211acc1740750836de7e968f4b12ebead6` |
| `src/item/item_transfer_command.h` | `49c9553ff958d4f7a8fedb4d3ad328009d3a63a42f0a5929abaa08bd5d6937eb` | `c6805ab7681946d3de48fd3dd092edee6c9f92153353874ac324ca060b038a22` |
| `src/item/item_transfer_repository.c` | `b4966c9da23d0362ac2879f7c38e61a510522c109f0c63d9ea5e305b52cd34dc` | `738bc71bb6418badf09a234adb1e08998b37461875c9c0cf25b49f1efa7eb5f6` |
| `src/item/item_transfer_repository.h` | `0bb5240e80811f9f147982c76051ab3d6a290dae504ce1a359c8f2f7f5c9533b` | `7f3e29d005d6268a5183345595c0060ac281e6b48d89bf9f6d70026d464fc8bb` |
| `src/mob/studioproclib.c` | `5d09f2068c452c13a2cb35f9803891072d925760edf64e762f01e105e77907a6` | `299f91e89032d32ae01c492550010bf28d7f84077f4f62e6a5cf417f75bf1d4b` |
| `src/mob/studioproclib.h` | `f1f8cbc675cd5494ba0dd19a603a60bd60cb75440f6cb83fcf6824c854fe43de` | `515a80db161b60b3d22bb3dc89d66aaa3349709ad4247c825a68639a4f4bb184` |
| `src/net/comm.c` | `f8573e2d73fa4ffbe9f0e87db3628a890d92fe1afa7074ca52ad94b7d8ccb8c0` | `2c884cfa5ec1644a33f40252af816cd6eccd10dff8dccbd32715eaedc4814303` |
| `src/persistence/critical_command.c` | `c7aa6deedf0e5656ad77f7875c404c48893b801c2ad1e27c9cbf149ba9032f73` | `4efd70c530be71a563f6ff2e492d2522c172b79a6cb5ca986c057d00c8014dc3` |
| `src/persistence/critical_command.h` | `744813f6c2cc280d5fd95e9b7e92e3811bb9dfa814c2ce1c8a1c13245ff095ab` | `e72698c5c84efc0e52b9fb8de13b30c7f1c67b353b2aba2e60dfb4dcff50931f` |
| `src/persistence/critical_command_coordinator.c` | `205ffe71ff0e0cd03a1048ba4266848767804c114ced552d7b81a4925a24f1e2` | `8ae0e75ebb0ddff1025dbcafda7519f66ce8b8c47b33ed31c8ee89f67403d150` |
| `src/persistence/critical_command_coordinator.h` | `8c891ade85820fd1f894dc751a52d4fd43ed5fc7a168438580e904c5b48d2b54` | `5a47d9ae897bebf3a0749cd62b6f11a1138e3995ae26c8de476ee01a6471420e` |
| `src/persistence/critical_command_repository.c` | `e5f48e33d289cc77ff223a821dad6e3db5593eb81ee120e8135bc294f7e9b504` | `a6d928f987701159f8b3a447a6c45dfb4125510cc584d1ae0060a949eede1f71` |
| `src/persistence/critical_command_repository.h` | `2f5938b071d513bef52446003539b0a5e3eff19ace29d0db58dd3b36fada5f50` | `aff79ae5fe9ed2bbcb11b80a2064da5c2f140da2276f6f9a311048ab7a990176` |
| `src/player/inert_item_stage.h` | `78ca10e63675aff19fb55aab1e4cab8b84323f3db53c716de6a3a4ab5d67f54a` | `03d9517feef1abaae2b05f45054a229e391e4a88f82fb158f2ca0aae4b726ee6` |
| `src/player/player_load_items.c` | `fabb268d55f6fb59c0f42541512cdbfb5cfdcc96ee848e2b06e2ac9c6e923800` | `33793deff7b1a4ae6a2f3518c0058357490b555766fca2681ce48ed3b3d5b410` |
| `src/player/player_save_pipeline.c` | `67bacb24b2edcf27a4496eb07610b6e557e11877fad7f6d160657cae6883f663` | `ef991d14358d58b062c13565a6872e087a16c42963ccc30b46bb17509a55e5bd` |
| `src/player/player_save_pipeline.h` | `22124b688c036d0525dde81a63243004e722e39ca3ecac017455f73e41df24a7` | `99795973f8efd14b97bc362eb0177ae46fba5a7b336895c215c5ad8637ef8354` |
| `src/specs/specs.library.c` | `dfc79b4af11b4688fa69b547e6e82c4b214c8de3ff02db6acae06ef87ae8eeec` | `5bc33e9cabe77028942a93129567a2f4041900d2f952d5bae35790815341009e` |
| `src/world/db.c` | `6dc1651425126fe0e63835312eb2bd0e71d9b48f988e41b18b073b391384fea8` | `47db8a88e7b5abc97b698a29b0224492a6311a077a2f9bb4b3b6c49aa49b6d40` |
| `src/world/db.h` | `5897c9aa58084208b407643913b24490322e77975461a69c9fb306274afc51ff` | `dfa14023b207673de9c7583dc9411bfdfef569941f874d7aa3775303c1e650a2` |
| `src/world/handler.c` | `be9a24aee571aeafabebdb747ee3e9e6b2fdb47874a529ec9b19c34459ebe71b` | `3f046194166dbbfd949f9ec0199e72f6155283edbdac69a7fd186da21333ce07` |
| `src/world/handler.h` | `3650f740dc709fc11b746cb78b59ac5d105498881adbb7239d3184edef6bb9af` | `69cf721db5b448bd67b76074a5af2cb338d46ef3425dc8cae7bb18bc7803f124` |
| `src/world/object_template.h` | `4b96c4846af69f2aebb3af813f2e6e106206e871088574fc066866747c319b1c` | `2ea82718045886d4005941ad4e374324e3f3d0023846fe26480236e0becbf4ff` |
| `src/world/quest_mobile_native.c` | `7d381e3bb44d0ed7ff77ee4887bf0238586ce19cd659a3e7cc45a4cad8a712ac` | `7e65a49da35448d5c2e0131851f2cf2b0c169bc1516c2bf0ba94bb0abb8698f2` |
| `src/world/quest_mobile_native.h` | `1b4dd5989a82d4c27a7cea03180f8d17c8e9e2599495fa15112d734e5b994035` | `b4d6f9c0f5d8d8dea74cd1519e1b0f29fe36fe8797993f20a8c4316e831d490d` |
| `src/world/quest_mobile_native_binding.c` | `0be18cca59ecb29345ac67d48b8503973c27bb1e182456e8a46582b2560d2306` | `1b04901860ca7395000a8cb9a66d02de3177d4c7daa46338775133c6b1c6df37` |
| `src/world/quest_mobile_native_binding.h` | `a8826e1127bfaca3d3afb0a4557b2a0698d59a606f6166762f4e48ef362b58c9` | `9f5836b9f2b12af192fd70e58a5b15729106080d7820e6ab05b65edf0e527162` |

## Evidence and remaining work

Evidence root: `D:\CodexEvidence\accounting-plan5\bin\native-birth-combined-01-20261006`.
`windows-metadata.json` records all four exact commands, terminal exits, times
and log hashes. `shared-writer-drift.json` SHA256:
`4f2f327205a2702ea7557a4d7bd4d8aebe4d7e5141e489d0b3e3dd1901d69975`.

The source checkpoint is published and the original fresh native build/audit
process is active separately. No passing build, database, restore or retention
result is claimed by this metadata handoff. Full independent capture, actual
producer/receipt and player recovery journeys, backend parity, active erasure
and mixed release-host budgets remain open. Accounting stays inactive, wallet
root exclusions and the declined inactive spell path remain. No production
mutation, audit auto-correction, deployment or PR merge occurred.

Primary's local notebook remains nonblocking. This owned report and sealed
source/evidence receipts form the curator packet.
