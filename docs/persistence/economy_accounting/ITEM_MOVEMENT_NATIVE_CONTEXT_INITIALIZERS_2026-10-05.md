# Ordinary movement native-context initializer repair — 2026-10-05

[The exact Plan5 build report](PLAN5_A015_MAINTAINED_BUILD_HANDOFF_2026-10-05.md)
is imported unchanged from 72fee509ca517c4ab422f53e49bf145493d99d11. On its
frozen a015-based candidate, fresh SQL and flat maintained builds both stopped
at three missing-field-initializers diagnostics for item_transfer_payload's
trailing native_mobile member. Those failed builds remain historical evidence;
they are not successful current executable qualification.

The primary-owned submit_movement, item_movement_transaction_submit_batch and
item_movement_transaction_submit_craft aggregates now explicitly initialize
.native_mobile = {} immediately after continuation, in declared member order.
The existing item_native_mobile_context defaults preserve present=false, the
reference's existing default initializer, zero action and zero final giver.
No wire/schema/producer authority, public interface, warning flag, active or
inactive route, wallet exclusion or declined inactive spell behavior changes.

The exact three source edits pass forward/inverse and changed-line clang18 fixed
point. Registry locations move only through identical source lines; all writer
classifications remain, all current source pins match, and static validator and
generated matrix results are recorded in the milestone receipt. No local native
compiler, build, gameplay, persistence, recovery, SQL or service execution ran.

Per the user's batching instruction, re-run both original maintained build
commands and movement/batch/craft context plus inactive-path checks with the
original major-plan candidate. This source repair closes the identified omitted
initializers, not that execution gate. Existing source-contract successes in the
peer report remain exact frozen evidence; source inventory is not runtime
coverage. R1–R8, release and activation remain BLOCKED.
