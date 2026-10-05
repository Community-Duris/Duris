# NPC capture declaration repair — 2026-10-05

The existing `GET_RNUM` macro invokes `panic_corruption_int`. NPC capture included
structs/utils but omitted its existing declaration from `core/prototypes.h`.
Plan5 established the same compiler refusal with both maintained SQL and flatfile
warning profiles; copied-source probes adding exactly this include compiled both.
See [the exact peer report](PLAN5_NATIVE_COMPONENT_BUILD_AND_CONTRACT_HANDOFF_2026-10-05.md),
imported byte-for-byte from180b0b58e. Its attempted source is cd89d4b02; its
native capture input exactly matches the pre-repair published source871b2473.

This repair includes the existing header, preserving GET_RNUM and its corruption
guard. No duplicate prototype, raw-field replacement, schema or behavior change.
Current raw source pins and generated definition anchors are refreshed. Source
delta, whitespace and census are checked; no local compiler/tests/native/SQL
execution ran under the requested major-plan testing cadence. The peer probe is
not a repaired maintained build or current combined-candidate qualification.
Both maintained builds and applicable native runtime/recovery checks remain
required at the major-plan batch. Three peer coverage-contract failures are
separately queued for source reconciliation. Coverage is incomplete; release
remains blocked and accounting inactive.
