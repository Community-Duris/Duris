# Implementation Notes

**Session ID**: `phase00-session10-security-policy-and-dependency-baseline`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 16 / 16 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Replaced the policy template with supported-version, private reporting, response,
  disclosure, safe-testing, and non-assurance guidance.
- Enabled weekly GitHub Actions Dependabot updates and pinned all workflow dependencies
  to immutable commits.
- Added deterministic direct native-package inventory and SPDX 2.3 generation from the
  maintained `equivs` manifest.
- Added a minimal direct-package root so Trivy receives Ubuntu package semantics while
  retaining SPDX as the portable SBOM.
- Added local repository-specific policy/config/source checks and CI CodeQL plus Trivy
  analysis with report retention and explicit enforcement.
- Documented ownership, exception requirements, scope gaps, and the measured baseline.
- Added focused contracts for policy, automation, reproducibility, package URLs,
  ignored outputs, and scanner-root filesystem safety.

## Verification Evidence

- `make security-check`: PASS.
- Deterministic inventory, SPDX, and scanner-root comparison: PASS.
- Trivy 0.70.0 direct-package baseline: one unfixed MEDIUM advisory; no fixed
  HIGH/CRITICAL finding.
- `actionlint` 1.7.10: PASS for both workflows.
- `./scripts/format.sh --check`: PASS.
- `make test-all`: PASS; 177/177 Python regressions plus signal-handler checks.
- `git diff --check`: PASS.

## Review Repair

Review removed recursive scanner-root deletion, guarded managed paths and symlinks,
encoded Debian epochs in package URLs, removed nonexistent Dependabot labels, and
pinned the build runner image.

## Scope Notes

- Generated inventory, SPDX, rootfs, reports, scanner databases, and tools remain under
  ignored `bin/security/`.
- CodeQL status is unknown until GitHub runs the workflow; transitive, deployment-only,
  firmware, external-service, and private-report coverage remains `UNKNOWN`.
- No configured credential, private advisory, player/account data, or production
  system was accessed or changed.
