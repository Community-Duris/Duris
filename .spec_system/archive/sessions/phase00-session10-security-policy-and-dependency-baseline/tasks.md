# Task Checklist

**Session ID**: `phase00-session10-security-policy-and-dependency-baseline`
**Total Tasks**: 16
**Created**: 2026-08-27

---

## Inventory And Policy

- [x] T001 Confirm Session 10 selection and clean base `d5d1d8f2`.
- [x] T002 Inventory policy, package mechanisms, CI actions, ignored outputs, and available validators.
- [x] T003 Confirm repository private vulnerability reporting is enabled and select supported channels/expectations.
- [x] T004 Define direct-package SPDX, local contract, CodeQL, Trivy, ownership, and failure boundaries.

## Governance And Inventory

- [x] T005 Replace template `SECURITY.md` with actionable supported-version and disclosure guidance.
- [x] T006 Replace invalid Dependabot entry with weekly GitHub Actions updates.
- [x] T007 Implement deterministic direct dependency inventory and SPDX 2.3 generation under `bin/security/`.
- [x] T008 Add local source/config security checks and Make targets.
- [x] T009 Document commands, scope gaps, ownership, exceptions, and non-assurance language.

## CI And Tests

- [x] T010 Pin existing and new workflow actions to immutable commit SHAs.
- [x] T011 Add CodeQL C/C++ analysis and pinned Trivy scanning of the generated direct-package root.
- [x] T012 Preserve inventory, SBOM, and scan output as CI artifacts without committing them.
- [x] T013 Add focused policy, dependency, SPDX, workflow, and generated-output regressions.
- [x] T014 Run local baseline generation, source gate, actionlint, and focused tests.

## Completion

- [x] T015 Run build, formatting/whitespace, full suite, and record measured/unknown baseline results.
- [x] T016 Complete review, validation, phase audit, records/version, commit, and publication.

## Completion Checklist

- [x] All 16 tasks complete
- [x] No outstanding blocker or unresolved failure
- [x] Review and validation complete
