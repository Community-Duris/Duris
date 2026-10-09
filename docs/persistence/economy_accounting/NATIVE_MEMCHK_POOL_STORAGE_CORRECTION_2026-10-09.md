# Active MEMCHK event-pool storage correction - 2026-10-09

Independent diagnostic review found that MEMCHK=1's actual __malloc/getmem adds a
repository-owned ALLOCATION_HEADER to each CREATE request. The pool observer counted
its descriptor and pool-list payloads but omitted those two live headers. The actual
bounded census now adds both owning sizeof(ALLOCATION_HEADER) terms with checked
arithmetic under the exact same #ifdef MEMCHK as __malloc. Mmap pages require no such
header. Current/initial-once storage and configured prospective chunk reservation
inherit this correction without allocation or scheduler changes.

This closes the concrete omission in f0e981dec's finite pool review; its original
source evidence remains historical. Ordinary allocator/scheduler/inactive functions
remain exact. Corrected RAW/final source review, original bounded-addition inverse,
tokens/preprocessor PASS; all931 policies/397pins authenticate, zero new/unmapped
sites. Evidence: tmp/event-pool-memchk-correction-owner-20261009 and
 tmp/event-pool-memchk-correction-integrated-20261009. Native builds/tests stay in the
major-plan batch. Diagnostic queue's corresponding headers are separately worker-owned
and source-corrected under review; not part of this commit. Full scheduler/service/root/
warm-cold/checkpoint/ACK/retirement and existing budget/recovery/mixed qualification
remain open. No new gate or full completion claim. Original scoped Plan1 acceptance,
Plan5 independent ownership, protected WIP, inactive accounting/admission CLOSED,
coverage incomplete/release BLOCKED and primary ACTIVE goal persist.
