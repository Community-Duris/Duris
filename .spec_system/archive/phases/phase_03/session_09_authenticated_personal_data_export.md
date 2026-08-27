# Session 09: Authenticated Personal Data Export

**Session ID**: `phase03-session09-authenticated-personal-data-export`
**Status**: Complete (Canonical activation blocked by pending disclosure policy)
**Work Window**: One data-access request boundary from account reauthentication and
stable request identity through consistent subject collection, classification,
redaction, package verification, protected delivery, expiry, and audit.

---

## Objective

Provide a complete account-scoped, machine-readable data export that is authenticated,
bounded, auditable, and incapable of disclosing credentials or another subject's data.

---

## Scope

### In Scope (MVP)

- Define an authenticated account-menu or connected-client request flow with password
  reauthentication, rate limits, one stable request ID, exact account scope, status,
  cancellation, and replay-safe completion.
- Derive database, ledger, ownership, message, activity, journal, cache, pfile, log, and
  archive subject mappings from the Session 07 lifecycle manifest rather than a second
  hard-coded table list.
- Collect exportable records through bounded worker jobs and documented consistent
  snapshots, preserving source schema/version and distinguishing current, archived,
  retained-exception, missing, and redacted data.
- Exclude password hashes, confirmation codes, private keys, operation secrets, internal
  security controls, and unrelated participants' protected content; apply the approved
  shared-record disclosure rule consistently.
- Build a versioned deterministic JSON or equivalent typed bundle with a manifest,
  counts, checksums, generation time, policy version, and human-readable field guide.
- Hold bundles only in an ignored 0700 spool with 0600 files, stream or release them
  only to the authenticated account through an approved protected delivery path, and
  expire/delete them on a short configured TTL.
- Record minimal request/status/audit metadata without exporting raw values to logs and
  test cross-account, concurrent-mutation, cancellation, failure, and expiry behavior.

### Out of Scope

- Exporting password hashes, confirmation codes, server secrets, raw security logs, or
  another person's unrestricted messages/descriptions.
- Building a general public web portal or email delivery service.
- Account erasure, owned by Session 10.

---

## Prerequisites

- [x] Sessions 07 and 08 lifecycle manifest and archive mappings are validated.
- [x] Phase 00 authentication, connection, TLS, and redacted logging controls remain
      enforced.
- [x] Protected remote delivery uses TLS; otherwise release is restricted to a trusted
      local operator workflow with verified account identity.

---

## Deliverables

1. Authenticated export request, status, cancellation, rate-limit, and completion
   contracts integrated with account state and bounded workers.
2. Manifest-driven current/archive/recovery data collectors and shared-record redaction
   rules.
3. Versioned checksummed export package plus permission, protected-delivery, TTL,
   cleanup, and minimal-audit implementation.
4. Focused complete-scope, cross-account, secret-exclusion, shared-record, large-export,
   mutation, duplicate, cancellation, spool-permission, delivery, and expiry regressions.

---

## Success Criteria

- [x] Only a successfully reauthenticated account can create, inspect, or retrieve its
      export request and bundle.
- [x] The bundle covers every exportable manifest entry for the account and its
      characters, reports exclusions explicitly, and includes no credential or secret
      material.
- [x] Shared records cannot expose another subject's protected fields beyond the
      approved disclosure rule.
- [x] Collection is bounded and internally consistent, and concurrent gameplay cannot
      cause a silently mixed or partial export.
- [x] Bundle counts and checksums verify before release; failed or cancelled requests
      publish no success and retain no orphaned readable artifact.
- [x] Spool directories/files have restrictive permissions, are ignored by Git, expire
      on schedule, and never place private values in application logs.
- [x] Focused regressions, formatting checks, and `make -C src` pass.

Canonical collection and release remain unavailable: the externally owned
shared-record disclosure decision has no approved reference. The completed boundary
therefore contains schema, validation, packaging, ownership, spool, and synthetic
verification controls, while deliberately exposing no live database/account collector
or account-menu activation.
