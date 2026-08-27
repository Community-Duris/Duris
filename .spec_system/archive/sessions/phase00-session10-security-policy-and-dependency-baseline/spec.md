# Session Specification

**Session ID**: `phase00-session10-security-policy-and-dependency-baseline`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `d5d1d8f2`
**Work Window**: Actionable vulnerability reporting, native dependency inventory/SPDX, local security contracts, immutable CI scanning, and baseline triage policy.

---

## 1. Session Overview

`SECURITY.md` is an untouched template, Dependabot has an invalid empty ecosystem, the build action uses a moving tag, and no SBOM or security scan exists. Duris has no language package lockfile; its authoritative dependency mechanism is the Debian/Ubuntu `equivs` manifest plus GitHub Actions. Repository private vulnerability reporting is enabled.

## 2. Objectives

1. Publish a supported-version and private-reporting process the project can actually operate.
2. Replace invalid Dependabot configuration with the applicable GitHub Actions ecosystem.
3. Generate deterministic installed-resolution inventory and SPDX 2.3 artifacts under ignored `bin/security/`.
4. Add a dependency/source security gate with immutable CI action references and explicit triage/failure rules.
5. Record results and coverage gaps without treating scan completion as proof of safety.

## 3. Scope

### In Scope

- `SECURITY.md`, dependency/security documentation, Dependabot, build/security workflows, SBOM tooling, Make targets, and focused regressions.
- Direct native/system packages declared by the maintained `equivs` manifest and GitHub Actions dependencies.
- CodeQL C/C++, Trivy scanning of a minimal direct-package root, portable SPDX output, and repository-specific local source/config contracts.

### Outside This Work Window

- Legal/privacy conclusions, Phase 03 data-rights implementation, package-manager conversion, generated artifact commits, private report contents, or claims about unknown vulnerabilities.

## 4. Technical Approach

Parse the `equivs` `Depends` field, deterministically select each installed alternative with `dpkg-query`, and emit sorted inventory plus SPDX JSON to `bin/security/`. Keep unresolved declared expressions explicit. Add a stdlib-only local security checker for policy placeholders, credential defaults, insecure chest SQL, private-key additions, Dependabot validity, and immutable action references. Pin checkout, CodeQL, Trivy, and artifact-upload actions to commit SHAs while retaining version comments. CI builds the dependency package, generates the SBOM and minimal direct-package root, runs the local gate, analyzes C/C++ with CodeQL, scans the root with a pinned Trivy version, and preserves reports even on failure. The root is necessary because Trivy does not infer Debian/Ubuntu vulnerability semantics from this direct-only SPDX input.

## 5. Deliverables

| File | Change |
|------|--------|
| `SECURITY.md` | Supported version, private reporting, response/disclosure, and scope process |
| `.github/dependabot.yml`, `.github/workflows/*.yml` | Valid Actions updates and immutable build/security automation |
| `scripts/generate_security_sbom.py` | Deterministic direct dependency inventory and SPDX 2.3 generation |
| `scripts/security_source_check.py`, `Makefile` | Reproducible local configuration/source gate and targets |
| `docs/SECURITY_BASELINE.md` | Coverage, commands, ownership, failure policy, and measured baseline |
| `tests/async/test_security_dependency_baseline.py` | Policy, generator, output, workflow, and no-placeholder contracts |

## 6. Success Criteria

- [x] Private reporting uses the enabled GitHub advisory path with concrete response expectations.
- [x] Dependabot contains only valid applicable ecosystems.
- [x] Inventory and SPDX output are stable, valid, explicit about unresolved packages, and ignored by Git.
- [x] Local security contracts and immutable CI CodeQL/Trivy checks have documented ownership/failure behavior.
- [x] Baseline reporting separates measured findings, incomplete coverage, and unknown status.
- [x] Actionlint, focused tests, formatting/build, and full suite pass.

## 7. Risks And Resolutions

- **False assurance**: label unrun and unsupported scopes `UNKNOWN`; a successful tool invocation is not a clean bill of health.
- **Distribution drift**: preserve declared expressions and record resolved package versions per environment instead of committing runner-specific versions.
- **Supply-chain drift**: pin every `uses:` dependency by full commit SHA and let valid Actions Dependabot propose reviewed updates.
- **Scanner noise**: fail Trivy only on fixed HIGH/CRITICAL findings; CodeQL findings remain visible in code scanning and require maintainer triage.

## Next Steps

Session complete. Continue with Phase 01 Session 01 after the Phase 00 audit.
