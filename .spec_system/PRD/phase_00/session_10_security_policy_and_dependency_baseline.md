# Session 10: Security Policy and Dependency Baseline

**Session ID**: `phase00-session10-security-policy-and-dependency-baseline`
**Status**: Not Started
**Work Window**: One repository security-governance boundary covering vulnerability
reporting, dependency inventory, reproducible SBOM output, and CI security checks.

---

## Objective

Replace placeholder security and dependency automation with an actionable,
reproducible baseline that maintainers can run locally and in CI without making an
unsupported claim that the dependency set is vulnerability-free.

---

## Scope

### In Scope (MVP)

- Replace the template `SECURITY.md` with supported-version, private-reporting,
  response, disclosure, and scope guidance grounded in channels the repository can
  actually support.
- Remove or repair the invalid empty Dependabot ecosystem entry according to the
  package manifests and dependency mechanisms that exist in the repository.
- Produce a deterministic native/system dependency inventory and SBOM artifact under
  `bin/` or another ignored generated-output location.
- Add reproducible dependency and source security checks with documented triage
  ownership and failure policy.
- Pin third-party CI actions immutably where practical and validate workflow syntax.
- Record the baseline result without claiming unknown vulnerabilities are absent.

### Out of Scope

- Legal conclusions about GDPR applicability or a complete privacy program.
- Phase 03 retention, access/export, erasure, or backup-propagation implementation.
- Committing generated build artifacts, credentials, scan databases, or private reports.

---

## Prerequisites

- [ ] Sessions 08 and 09 have validated the runtime and stored-secret findings that the
      policy and baseline must describe accurately.
- [ ] Security tools can write generated artifacts only to ignored local paths.

---

## Deliverables

1. Actionable repository security policy in `SECURITY.md` with no template placeholders.
2. Valid dependency-update configuration or a documented removal when no supported
   ecosystem applies.
3. Reproducible dependency inventory, SBOM, and source-security commands integrated
   into local tooling and `.github/workflows/` as appropriate.
4. Focused configuration and workflow regressions under `tests/async/`.

---

## Success Criteria

- [ ] `SECURITY.md` gives a concrete private reporting path and response expectations
      supported by the repository.
- [ ] Dependabot configuration is valid or absent for an explicitly documented reason.
- [ ] Dependency inventory, SBOM generation, and security checks are reproducible and
      place generated artifacts only in ignored paths.
- [ ] CI actions and security checks have explicit versions, scope, and triage behavior.
- [ ] The recorded baseline distinguishes findings, unknown coverage, and clean results
      without unsupported assurance.
- [ ] Focused regressions and the relevant repository validation commands pass.
