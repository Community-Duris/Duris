# Recovery observation registry preparation — 2026-10-04

Status: **source classification, UNQUALIFIED**. No generator, AST, compiler,
native, SQL, gameplay, service or recovery checks ran. Test execution remains
deferred until major-plan readiness.

The semantic registry and generator's nonwriter table now explicitly classify
three opt-in read-only recovery interfaces on source candidate
`488414dc1b3e41689ecba444aadef6fd8657d689`:

- Transaction-scoped original ordinary-drop receipt verification (`16e77e111`).
- Non-hydrating runtime owner-cache observation (`8068950d3`).
- Actor-independent existing ordinary-drop graph observation (`488414dc1`).

These interfaces neither mutate native holdings nor create, enroll, retire or
destroy items. They do not grant a publication ACK capability. The graph and
receipt SQL seams refuse flatfile; the runtime cache read is backend neutral.
All backend evidence remains unverified. No production dispatcher consumes the
new graph owner yet.

Existing registry row values, escaped historic Unicode, generated matrix and
historic census/source anchors are preserved. `pending_source_integration`
records the newer candidate and pending routes separately. Current-source
semantic/site census, executable same-root tests and matrix reanchoring remain
required. Lexical inventory and these classifications do not establish R6 or
full accounting completion. Later all-absent enrollment must receive its own
writer/projection classification; it cannot inherit this observer's status.
