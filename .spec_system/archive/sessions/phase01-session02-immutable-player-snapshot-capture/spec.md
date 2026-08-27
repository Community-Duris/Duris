# Session Specification

**Session ID**: `phase01-session02-immutable-player-snapshot-capture`
**Phase**: 01 - Replace Forked Full Saves
**Status**: In Progress
**Created**: 2026-08-27
**Base Commit**: `7d3b7996`
**Work Window**: Bounded immutable value capture for every checkpoint component, with no live-pointer escape or gameplay mutation.

---

## 1. Session Overview

The legacy save path mutates equipment/affects around SQL and several persistence
helpers traverse live character/object graphs directly. Session 01 established exact
revision/component identity; this session seals selected values into a worker-safe DTO
without changing the active save route.

## 2. Objectives

1. Define pointer-free typed snapshot metadata and rows for status, replacement sets, affects, items, pets, shapes, and trophies.
2. Capture nested equipment/inventory/container state without unequip, extraction, reordering, or affect mutation.
3. Enforce byte, row, object, depth, and string limits with classified failures.
4. Build transactionally into a temporary snapshot so failure never publishes partial payload.
5. Prove source equivalence and mutation isolation while leaving active saves untouched.

## 3. Scope

### In Scope

- Snapshot value definitions, capture builder, deterministic cleanup, size accounting,
  cycle/depth detection, and synthetic/runtime/source contracts.
- All Session 01 checkpoint groups, including empty replacement sets and explicit
  no-pet semantics for non-crash save intent.

### Outside This Work Window

- SQL apply, worker queues, acknowledgements, journals, mutation marking, or save cutover.

## 4. Technical Approach

Keep the DTO header independent of `structs.h`: it contains only fixed-width scalars,
arrays, strings, vectors, and local row indices. A separate capture adapter accepts a
live `P_char` only on the game thread and recursively copies values into a temporary
snapshot. Parent/container relationships use row indices, never object addresses.
Allocation errors are retryable; invalid identity, graph cycles, excessive nesting,
malformed prototypes, and configured bounds return explicit classes. Output is moved
to the caller only after all requested components pass.

## 5. Success Criteria

- [ ] DTO definitions contain no live pointers or engine object types.
- [ ] Capture performs no unequip/affect/extract/list mutation.
- [ ] Metadata contains schema, PID, revision, mask, save intent, and room identity.
- [ ] Every selected component produces bounded value rows or an explicit empty set.
- [ ] Failures publish no partial output and never consume revision-state dirtiness.
- [ ] Focused tests, formatting, warning-as-error build, and full regressions pass.

## Next Steps

Implement the pointer-free DTO, capture adapter, bounds, and contracts.
