# Session Specification

**Session ID**: `phase03-session09-authenticated-personal-data-export`
**Phase**: 03 - Load Path, Schema, and Retention
**Base Commit**: `3ea6921618d97a00ca6d0c5bf82a5d8732f0b949`
**Created**: 2026-08-27

## Objective

Implement a reauthenticated, account-scoped, bounded export workflow that derives
coverage from the lifecycle manifest, excludes credentials and unrelated subjects,
builds a deterministic verified bundle, and releases it only through protected local
delivery with short-lived artifacts and minimal audit metadata.

## Architecture

- Stable request IDs bind authenticated account scope without persisting raw passwords.
- Additive request/batch/audit schema and a bounded worker/state machine with consistent
  snapshot identity, cancellation, retries, rate limits, and exact manifest coverage.
- One lifecycle-manifest export mapping records include/exclude/shared/redacted behavior;
  unknown or unmapped stores fail the request rather than silently disappearing.
- Deterministic versioned JSON bundle has per-section counts/checksums, a field guide,
  explicit exclusions, and whole-package verification before publication.
- Local-only spool uses no-follow 0700 directories, 0600 atomic files, owner-bound
  retrieval tokens, TTL cleanup, and redacted diagnostics. Remote release is unavailable
  unless an existing authenticated TLS connection owns the same account request.

## Success Criteria

- [ ] Account-menu creation/retrieval requires password reauthentication and exact
      descriptor/account ownership with bounded failures and rate limiting.
- [ ] Every lifecycle store has one export disposition; credentials, secrets, raw
      security controls, and unrelated shared-subject fields are excluded.
- [ ] Collection and package state are bounded, consistent, resumable/cancellable, and
      incapable of publishing partial success.
- [ ] Deterministic counts/checksums verify before one-account protected release.
- [ ] Spool permissions, no-follow handling, TTL expiry, cancellation cleanup, and Git
      exclusion are enforced and tested.
- [ ] Focused cross-account, shared-record, secret, mutation, duplicate, failure, and
      large-export regressions pass with formatting, build, full review, and validation.

## Safety

No real account export, credential read, email/web delivery, production connection, or
erasure. Dynamic tests use synthetic accounts and isolated temporary spools/databases.

## Next Steps

Run `implement`.
