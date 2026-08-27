# Task Checklist

**Session ID**: `phase00-session09-private-chest-password-hardening`
**Total Tasks**: 16
**Created**: 2026-08-27

---

## Inventory And Design

- [x] T001 Confirm Session 09 selection and clean base `b0297c07` without reading configured secrets or data.
- [x] T002 Inventory chest create, reset, open, legacy SQL, shared bcrypt, and all schema definitions.
- [x] T003 Select bcrypt cost 12, 72-byte input cap, constant-time legacy verification, and conditional upgrade.

## Hashing And Persistence

- [x] T004 Extract account bcrypt functions into a reentrant shared password-hash module.
- [x] T005 Add strict bcrypt recognition and constant-time legacy SHA-256 verification.
- [x] T006 Hash new private-chest passwords before SQL and reject overlong input.
- [x] T007 Add a checked chest password set/remove API and route the owner command through it.
- [x] T008 Fetch stored hashes and verify bcrypt or explicit no-password state in process.
- [x] T009 Upgrade successfully verified legacy hashes with a conditional fresh-salt update.
- [x] T010 Preserve valid legacy access on upgrade failure while keeping diagnostics categorical.

## Compatibility And Tests

- [x] T011 Confirm every authoritative schema definition supports the 60-character versioned bcrypt value.
- [x] T012 Add standalone runtime tests for salts, cost, correct/incorrect inputs, legacy recognition, and bounds.
- [x] T013 Add source contracts proving plaintext chest passwords and SQL `SHA2()` are absent from the lifecycle.
- [x] T014 Run nearest locker, SQL, hashing, and log-hygiene regressions.

## Completion

- [x] T015 Run formatting, warning-as-error build, shell/whitespace checks, and the full suite.
- [x] T016 Complete code review, repair findings, validate, update project records/version, commit, and publish.

## Completion Checklist

- [x] All 16 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Review and validation complete
