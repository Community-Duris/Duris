# Genuine diagnostic variadic-format admission - 2026-10-09

Scheduler diagnostics require real variadic formatting admission. The owning bounded
helper admits the complete named workspace before construction/va_copy and exact
prefix+vsnprintf body+suffix+NUL malloc request before allocation. Both va_list copies
are correctly ended; checked overflow, original two-pass formatting/copy behavior
and exact strong pointer/retained-byte output transfer remain. Caller retains args,
old outputs and transferred malloc until freed. Original debug/logit/format/send bodies
are byte-exact; libc internals remain the existing policy, without a new exclusion.

Independent RAW/final source review, normalized original-prefix/header inverse and
tokens/preprocessor PASS. All931 policies/397 sourcepins authenticate, including two
new genuine utility provider pins; zero new/unmapped sites. Evidence:
tmp/diagnostic-format-bounded-owner-20261009 and tmp/diagnostic-format-integrated-20261009.
Native builds/tests stay deferred to major-plan readiness. Diagnostic output/queue/
pager/multi-recipient/logging/inspector/scheduler/service-step/root joins remain open;
this private leaf does not qualify the full call chain. No new gate/bypass/refusalstub.
Full warm/cold/checkpoint/ACK/root and existing fault/mixed workload qualification
remain unfinished. Original Plan1 acceptance scoped; Plans2-4/full5/R1-R8/release
incomplete. Plan5 ownership, protected WIP, inactive accounting/admission CLOSED,
coverage incomplete/release BLOCKED and primary ACTIVE goal persist.
