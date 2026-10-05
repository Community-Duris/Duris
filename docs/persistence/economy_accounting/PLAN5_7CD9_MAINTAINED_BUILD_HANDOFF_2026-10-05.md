# Plan 5 refreshed maintained builds and SHOP shadowing handoff

The primary's three ordinary-movement initializer fixes close their original
compile errors. A fresh flatfile maintained server builds and links with no
warnings. The fresh SQL build proceeds farther and exposes a separate
primary-owned SHOP lambda shadowing error. Release qualification remains open.

## Source and delivery

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `5284d155bdbc92010ddc923ce6d6ced5b4b33600`, a preserving merge of
  refreshed primary `7cd9b42ee09b080c0b438eec83741c2828610447` into published
  Plan 5 `9745ec30d141dac846753c71bcf36444ec0c3b16`.
- Tested native tree: `ddfebad0ee6e2bdc6a0854fbe98f0e5cf9860e56`.
- Tested migration tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, through
  canonical0056. Neither earlier0055 evidence nor private successors qualify
  this source.
- This report is the sole owned file in this slice. The separate independent
  versioned-baseline reader work began after the contract checks; it did not
  change the native or migration inputs of either build.
- Exact evidence: `tmp/plan5/7cd9-build-evidence.json`; result commit and
  verified remote are in `tmp/plan5/7cd9-build-delivery.json`.

No producer, coordinator, accounting contract, registry/matrix, schema or
activation file was independently edited. All previously completed slices and
old branches remain preserved in the requested branch's ancestry.

## Maintained executable checks

Immutable Linux image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Network disabled, read-only checkout, writable ignored `bin/`, separate empty
object directories and isolated `BIN_ROOT`. No cached objects, backend stamps,
warning suppressions or reduced source lists were used.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  BIN_ROOT=/workspace/bin/tests/p5-7cd9-build-20261005/bin-sql \
  OBJDIR=/workspace/bin/tests/p5-7cd9-build-20261005/objects-sql \
  DMS_BINARY=/workspace/bin/tests/p5-7cd9-build-20261005/server-sql
make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=/workspace/bin/tests/p5-7cd9-build-20261005/bin-flatfile \
  OBJDIR=/workspace/bin/tests/p5-7cd9-build-20261005/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/p5-7cd9-build-20261005/server-flatfile
```

| Policy | Exit | Seconds | Compile commands | Objects | Errors | Linked server |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| SQL | 2 | 280.324 | 412 | 411 | 1 | No |
| Flatfile | 0 | 547.436 | 726 | 726 | 0 | Yes |

Both builds retain the full maintained hardening and warning profile, including
`-Werror=shadow=compatible-local` and `-Werror=missing-field-initializers`.
Neither emits separate `warning:` diagnostics. Every native/migration input
hash remains unchanged over each command. Full commands, compile-source lists,
input maps and logs are under `bin/tests/p5-7cd9-build-20261005/`.

The flatfile binary is170749320 bytes, SHA256
`319beb398d186ed80f24ec6c894e79ff60ac3296e9a6f78e6952c53aea8ff610`.
This is build evidence, not a boot, gameplay or backup/restore attestation.
The failed SQL objects remain preserved and unlinked.

## Narrow primary-owned fix request

The original SQL source in
`src/persistence/shop_item_runtime_payload.c:1630` declares
`std::vector<player_item_snapshot> literal` inside `capture_literal_root`, while
the enclosing `attempt` lambda captures the outer vector declared at1561 and
uses it for the completed checkpoint path at1582. GCC12 rejects the inner
declaration as shadowing a compatible captured local.

Rename only the inner tree vector and all its uses within
`capture_literal_root`, for example `tree_literal`. Preserve the outer vector,
capture behavior, original payload/literal policy, parent/slot checks, byte
budget, output assignment, transaction/session checks and refusal semantics.
There is no interface, schema, wire-format, ownership or activation request.
The primary owns this repair. Do not weaken the warning profile to qualify it.

Re-run the original maintained SQL command on the integrated repair and the
relevant literal/checkpoint native tests; revalidate both maintained policies
for the published combined candidate. Existing inactive behavior, wallet-root
item exclusions and declined inactive spell-path change must remain.

## Contracts, release gate and curator handoff

```sh
PYTHONPATH=tests/async python3 -u -B -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
python3 -u -B scripts/validate_economy_accounting.py
python3 -u -B scripts/generate_economy_writer_coverage.py --check
python3 -u -B scripts/validate_economy_accounting.py --release
```

All71 original methods pass, with zero skips:11.958 seconds in unittest,
12.198 seconds including process startup. Ordinary validator and generated
matrix check exit0; release exits1 with `writer has no executable evidence`.
The complete input map and unchanged-source receipt were sealed before the
pending independent baseline reader edits. These are inventory/source checks;
they do not establish producer capture or release completion.

No database, production data, activation, service or gameplay boot was involved.
SQL executable and combined-server journeys remain gated by the reproduced
shared build error. EAB2 native/schema integration, original writer coverage,
both-engine fresh/populated upgrades, backup/restore/retention, real gameplay
and workload/fault evidence, and full R1–R8 remain open. There are no selected
test skips or notebook blockers in this slice.

This is a curator handoff for the primary's locally maintained notebook. Import
the completed report from `codex/accounting-plan5`, retain the exact failed and
successful build evidence, and register results only for their recorded source
and scope. Central registration and combined release publication stay primary
owned; accounting remains inactive.
