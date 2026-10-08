# All-absent ordinary-drop enrollment preparation — 2026-10-04

Status: **implemented source subset, UNQUALIFIED**. Testing is deferred until
major-plan readiness. No compiler, AST, native, SQL, service, gameplay or recovery
checks ran. Source review and formatting do not establish a working route.

The existing SQL publisher uses the normal materializer and refuses an already
present expected UID. Its cleanup can extract objects; normal construction issues
UIDs and invokes conversion/procedure/event machinery. The discard-only inert
stage previously offered no enrollment owner. Neither path supplies the required
actor-independent, exact-ID, side-effect-free reconstruction.

`ordinary_drop_recovery_publish` now reacquires the original native SQL authority
internally. It accepts only the original command and sealed receipt, never an
earlier unlocked absent observation. It preserves the observer's current
epoch/season/custody/literal locks, historical exact receipt/outbox proof and
complete global/room/container/character/runtime census. An exact existing graph
returns unchanged; partial/extra/duplicate/misplaced graphs refuse.

For the entirely absent subset it resolves only trusted `find_object_template`
cache entries. Complete parent-container/native depth, prototype and literal
eligibility precede pool acquisition. The factored `inert_item_stage_eligibility`
performs no allocation or pool access; the existing factory retains its semantics.
Original canonical literals supply slot0, ordered children, retained UID and exact
weights/text. No container weight recalculation or shared prototype string change
occurs. Every stage privately owns its allocation, including while children are
linked; refusal frees only detached memory, without extraction or callbacks.

The private, nonescaping module owner prepares explicit runtime entries from
current SQL identities and paired VNUMs, aggregated signed index-count deltas and
bounded saturating room lighting. It rechecks exact SQL session, complete absence,
cache identity/fresh eligibility, room/index identities and counter capacity.
`item_ownership_runtime_hydrate_many_atomic` is the final fallible step and restores
entry/owner values on allocation refusal. Success immediately performs only
nonthrowing global links, prechecked counts, root room placement, light assignment
and stage disarming. It invokes no normal object constructor, UID issuance,
conversion, procedure, event, activity, placement or extraction callback.

`published` requires confirmed original-session rollback/idle cleanup. If cleanup
becomes uncertain after native enrollment, the graph remains complete, the lease
retires, and the result remains unavailable/retry-required. The next attempt must
prove that existing graph. No publication result grants critical ACK authority or
releases the original save hold.

## Scope that still blocks ordinary-route qualification

Current boot caching covers starter/alchemist prototypes. Other cache misses stay
held; a trusted lifecycle-bound catalog covering every required retained command
is still needed. Runtime parser fallback is prohibited. Teleport/ship/boat types
remain unsupported because their activity indexes may allocate or bind external
state. Existing procedure/artifact/transient/corpse/money/trap/timer/dynamic-affect
restrictions remain. These exclusions do not qualify all ordinary drops. Production
admission must eventually refuse unsupported routes before economic mutation or
support their complete bookkeeping. No day-one product requirement is removed.

Production dispatcher/startup, complete affected-PID save mutation census,
coordinator-enforced ACK reservation, durable checkpoint, exact hold release and
journal replay revisit remain unconnected. Full active and inactive gameplay,
both SQL engines, flatfile parity, faults and complete cold recovery are required
on one combined candidate. There is no production activation or data modification.

## Frozen source preparation

Ten selected BEFORE inputs at source base `862f00381` are preserved in
`tmp/ordinary-drop-enrollment-before-v1.local/manifest.json`, SHA256
`e0a752625ce6d41ca0269cf2744661414984abf5fdba94bb538f8f888f61eaf8`.
AFTER manifest `tmp/ordinary-drop-enrollment-prepared-v1/manifest.json`, SHA256
`2f7642b7dea2095e634c48a370161f6d3cdb247782afb966cca8d8b546f1baa8`,
records the same ten selected paths. Neither inventory is a compiler closure.
Its21 declarative case groups are non-executable and unobserved; cases file SHA256
`6a2dfa48d59d72fb9d433eb16a9c1ed05ac2955e4220665ac668a42d83cb3793`.
Missing BEFORE API or compile failure is unsupported, never semantic RED.

| Source | Final raw SHA256 |
| --- | --- |
| `ordinary_drop_recovery.c` | `b13d6a06e2b691616e46f083365d6e8bd77efc2fd22d947ab49b6133b2d8473e` |
| `ordinary_drop_recovery.h` | `8f42a58690cdbc59e6e2358927740e45a780c26225d4103ae30bb90c129c6ee7` |
| `inert_item_stage.c` | `4738b25f9b1443037d62357de1d08366aecdd1317df84b61f95f67363e1c39c2` |
| `inert_item_stage.h` | `08cd423c4b0b33db6c3bfe34de89bd4ef345121515742caf0f580c0f17c46f29` |

Independent full source review found no remaining blocker for this explicit
cache-only/activity-excluding subset. It corrected the native load-identity versus
runtime-entry type mismatch and required complete nesting eligibility. No native
execution followed; major-plan and release acceptance remain open.
