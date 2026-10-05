# Retained SHOP v8 submission correction — 2026-10-05

The integrated build_accounted_command always calls shop_trade_command_build_recovery,
which produces payload version 8. submit_accounted required payload version 7,
so every newly built recovery command returned invalid before pipeline submission.
This is a source-proven producer/consumer contradiction, not a native test result.

The owned submission guard now accepts exactly native v7 or recovery v8. It still
requires successful payload decode, the original operation, original player,
completed and sealed native checkpoint, and held player checkpoint. Accounted
v6 lacks the native destination weight and remains rejected here; using the
broader is_accounted predicate would incorrectly admit it. Default v5 remains
on the original inactive path. Cold image overloads retain their v8-only guards.

Independent persistence review checked other maintained SHOP version comparisons,
accounting/coordinator/pipeline/repository consumers, sidecar history selectors
and runtime matching, finding no second analogous version refusal. Primary raw
source comparison proves the sole production change is this condition, and
changed-line clang-format18 is at a fixed point. Current source pins, writer
matrix and plan heads are refreshed without granting route qualification.

No compilation, tests, native/SQL/services, gameplay or recovery execution ran.
Major-plan testing remains deferred as requested. Central accounted SHOP
admission and activation remain closed. Producer driving, coherent schema0057,
cold publication/ACK, flat parity and original R1–R8 qualification remain open.
