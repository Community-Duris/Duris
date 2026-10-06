# Shared player inspector baseline provider handoff

The original shared `build_player_inspector` recipe fails to link on published
native tree `cf8dc0761057de7087a347b8e8c21285968ef4f9`. This is a dependency
request for the primary; Plan 5 does not independently edit the shared helper.
No public API, accounting contract, migration or wire-format change is proposed.

## Exact source and finding

Plan 5 branch/worktree are `codex/accounting-plan5` and
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Base is `445b4caa9f755ae7283cc6a0de1aec5ac67ed22a`, including primary
`9817f58b57a4a4179c4db5506fce3ccfe166c12b`, canonical 0062 migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.

The actual unmodified helper is invoked by `build_inspector` in the frozen
retaining observer. Its original cache-disabled C++20/Werror link fails:

```
flatfile_accounting_staging_view::validate(...):
undefined reference to economic_baseline_decode(...)
collect2: error: ld returned 1 exit status
```

The selected reference attempt incorrectly used the common player inspector
for deletion retention, whose original preparation requires the distinct
deletion inspector. It stops before any retention journey and is unqualified.
That caller mistake does not change the actual shared helper's unchanged link
command or its missing-symbol finding. The retention reference proceeds through
the original `test_flatfile_character_delete.py` preparation, without editing
or bypassing the shared helper. Its existing source list already declares the
real baseline adapter/codec pair for the same authority provider.

Frozen source archive SHA256:
`6c5247e11b759ab50308de15a4dcd933416eec3017696fe34b601f0898bf8f5b`.
Protected evidence is
`D:/CodexEvidence/accounting-plan5/bin/flat-source-loss-retention-red-01-20261006/`:
`native-commands.json`, `checks.log`, `process.json`, full public source archive
and transport. Process exit is 1, duration 58.981189s; its original native
compiler timeout remains 600s. Log SHA256 is
`7a5fa91dd0c318eab66ddd436b8b48d7c7195dc0095df829269c4644be742f7f`.
No source or authority is modified by this failed build.

## Narrow requested field and invariant

Review `tests/async/_flatfile_player_fixture.py:SOURCES` for the omitted
`src/economy/economic_baseline_adapter.c` and
`src/economy/economic_baseline_codec.c` providers required by
`src/flatfile/flatfile_accounting_authority.c`.

The original provider order, flags, native test switches, linker GC/wrappers,
timeout, cache fingerprint, assertions and runtime cases must remain. Each
needed real provider must occur exactly once. This request adds no stub,
injected header, linker suppression or duplicate helper. The original deletion
inspector source list supplies the existing dependency pattern; its distinct
harness is not a replacement for the common player inspector's consumers.

Exact SHA256 inputs are:

| Input | SHA256 |
| --- | --- |
| Shared player fixture | `4c1fae30bf30339b4f2798835cd5486546e4d917d2e151a2d94d6a48339b3a54` |
| Original deletion preparation | `214f158599e693f8c2f9acda358ffe6dbf7d1b236e728f2703e990dee83b8a15` |
| Flatfile authority | `887e40389a376bfc8ae635f9ea38b9fb2a24e4c7189d421fded745ad9644f00c` |
| Baseline adapter | `ca70e4f8b486ec23a7b575cb55b333f523d6bb1b336ff57b3e4210850f9ccea0` |
| Baseline codec | `54608da6a32243fcfb079037c17d6d83e5ce32d3291edaa9d935f89435100be7` |

## Consumers and required proof

Direct consumers are `test_flatfile_player_repository.py`, the original
`test_flatfile_combat_journey.py` wrapper and `test_mob_gold_dial_runtime.py`.
First repeat their actual shared native build with cache disabled and original
flags. Then run the original repository method and the real normal/reset-coin
combat journeys against the primary's combined flatfile candidate; preserve
their cases and deadlines. The gold-dial runtime consumer needs its original
selected qualification as well. A link pass or the separate deletion inspector
does not qualify those player journeys.

The shared provider repair and its combined qualification remain primary-owned.
This handoff is not a completed repair or a release gate waiver. The independent
flatfile retained-source-loss work can continue using its original deletion
preparation. Accounting stays inactive; no production data, audit finding,
producer behavior, activation or deployment is changed. This owned handoff is
part of the shared notebook curator packet on the same remote Plan 5 branch.
