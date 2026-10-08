# Independent compound action fix integrated — 2026-10-06

Five owned blobs from Plan5 commit
`f2a9280946b57e79181d2b22b173e4f63be22e08` are imported after exact parent
compatibility checks. The independent SQL exporter now preserves explicit
creation/destruction reasons and classifies compound actions from their retained
system-source revision1 or destruction endpoint. All four read-only ownership
projections use the rule and retain domain reason and authority.

Primary's actual UID-scope, canonical audit and audit-origin suites pass on the
imported source. UID-scope exercises ten endpoint/reason combinations through
all four projections and committed-origin inference. Exact results/log hashes
are retained in `bin/tests/plan5-compound-action-primary-integration-20261006`.
AST parsing passes for all three imported Python files. Optional native fixture
skips remain skips; this local check does not replace peer native qualification.

The peer's [qualified report](PLAN5_COMPOUND_ITEM_ACTION_QUALIFICATION_2026-10-06.md)
retains its actual native observer, both-engine SQL/restore results and exact
bf7a92a7 native source scope. Those results do not qualify primary's new51de
native producer/recovery source. Primary's installed source and original native
milestone are unchanged by this import. No source registry, manifest, schema,
coordinator, activation or production data changes occur in this audit fix.

Writer source/matrix refresh, native producer/publication/restart journeys and
combined release qualification remain open. All accounting safety/inactive
behavior and unrelated WIP remain. No full Plan5, R7/R8 or release completion is
claimed; the independent reader cannot create native authority from inference.
