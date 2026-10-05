# Publication compile-recipe source path repair

The existing coin physical-publication and shop retained-publication scripts
listed `src/item/chaos_pouch_ledger.c`, which does not exist. The maintained
translation unit is `src/combat/chaos_pouch_ledger.c`. Both recipes now select
that existing source; no case, assertion, compiler flag or time budget changes.

The nonexistent before-path and existing after-path were checked directly, and
both modified Python files parse. This solves the compile-input selection
defect; it does not establish that the native binaries compile or cases pass.
Compilation, native execution and broader major-plan qualification remain
deferred until the major plan is ready, per the user's testing instruction.
The separate central-inventory omissions are handled by the registration slice.
