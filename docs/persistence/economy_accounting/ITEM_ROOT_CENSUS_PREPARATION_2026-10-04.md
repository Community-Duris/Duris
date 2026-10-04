# Runtime item root census preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. The cold ordinary-drop
publisher needs complete runtime root enumeration: per-UID lookup misses extra
entries, and whole-owner snapshots hide foreign-owner conflicts claiming the root.

`item_ownership_runtime_snapshot_root(root_uid, limit, out)` copies every matching
root-field row, including foreign/malformed owner, state and parent topology.
It sorts by UID and neither validates nor repairs authority. Under existing
serialized game-thread ownership, two-pass count/reserve bounds allocation by
actual matches/caller limit and the existing262144-entry registry cap. Other roots
do not consume the limit. Exact limit succeeds; overflow/OOM/zero root/zero limit
refuse and empty nonnull output. An absent root succeeds empty; that alone proves
neither root existence nor native SQL/custody/publication authority. No mutex or
mutation contract changes.

Changed only `src/item/item_ownership_runtime.c/.h`. Independent architect source
review found no blocker. Source SHA-256:
`e71a743fcc741a7235b0a928a4a10ff1f439c613b5a8071f5766fc025bb7a594`;
header `d610a2449b8a854c6714559ddde65a9bdb534265fff048fa28f8feb32a8777f0`.
Private `tmp/item-root-census-prepared-v1/` retains53-file actual-runtime BEFORE/
AFTER closures. Manifest `5e6cbe441cda2313a455706634daa073ec1929380e6c80462ca90793d304b4f0`;
fixture `af15cb618c336d73a47dc851b90d7d4d23486d845a8d0620aab1c5eb4a999922`.
Observed candidate base80692d52b was dirty only in the two consumed source files.

Ten prepared groups cover empty/full forest, extra UID, other roots, mixed states/
owners, malformed/rootless graph, limits, allocation failures and reused output.
No test/compiler/AST/native/SQL/service run occurred. Missing BEFORE API is not
semantic RED. Only source formatting/diff hygiene/raw-byte closure checks ran.
Existing writer census, full cold publication and all release gates remain pending.
