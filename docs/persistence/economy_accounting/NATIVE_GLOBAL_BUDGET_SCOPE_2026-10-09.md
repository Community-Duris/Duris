# Native global preparation scope contract - 2026-10-09

The shared native preparation aggregate now supports a private, actual game-thread
guard. The real room owner can install one static, allocation-free CURRENT-storage
observer. Outside the guard, shared admission includes current native globals once,
including after a command releases its local charge. Inside it, the actual root
scratch owns that same storage once. Begin/end perform only scalar transitions and
refuse reentry, mismatched ownership, a changed observer or the wrong thread.

This closes the shared-contract gap; the full room pulse must still install its
genuine observer and rebase scratch on every return. The observer must not acquire
coordinator/journal locks. Root clears scratch before ending its guard, with no
allocation or callback between those scalar transitions. The NULL default preserves
existing unregistered ordinary/SQL/inactive behavior. No source or ACK authority is
created. Production accounting and admission remain inactive/CLOSED.

Independent RAW and formatted source review passed, with exact inverses and preserved
tokens/preprocessing. Existing capacity/reset insertions are explicitly recorded;
whole original C-prefix preservation is not claimed. The isolated registry verifies
931 existing policies, 399 authenticated source pins and zero new/unmapped sites.
Evidence: tmp/native-global-budget-scope-owner-20261009 and
 tmp/native-global-budget-scope-integrated-20261009.

Native builds/tests remain deferred to major-plan readiness. The four warm/cold
routes, complete aggregate observer/pulse join, placement/service integration and
runtime/recovery qualification remain unfinished. Coverage remains incomplete and
release BLOCKED; Plans 2-4/full 5/R1-R8 are not complete. The primary goal stays ACTIVE.
