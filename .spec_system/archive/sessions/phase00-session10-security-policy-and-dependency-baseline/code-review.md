# Code Review: Security Policy and Dependency Baseline

**Reviewed**: 2026-08-27
**Base commit**: `d5d1d8f2`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 10 diff: disclosure policy, Dependabot, workflow supply
chain, direct-package resolution, SPDX generation, scanner input, source contracts,
artifact handling, failure policy, documentation, and tests.

## Findings

### Critical / High

None.

### Medium - resolved

1. Scanner-root regeneration initially removed the entire requested directory. It now
   permits only the four managed files and their directories, rejects symlinks or
   unmanaged content, and never recursively deletes caller-owned paths.
2. Debian package epochs initially appeared as raw colons in package URLs. Names,
   versions, and architectures are now percent-encoded and covered by regression tests.

### Low - resolved

1. Dependabot named repository labels that do not exist. The configuration now relies
   on GitHub defaults instead of an operationally brittle custom-label assumption.
2. The legacy build workflow retained a floating runner image. Both workflows now pin
   Ubuntu 24.04 while every third-party action remains pinned by full commit SHA.

## Behavioral Review

- Private reporting points to the enabled repository advisory flow and provides a
  usable public fallback without soliciting sensitive details.
- The native inventory preserves every declared expression and explicitly records
  unresolved alternatives instead of silently omitting them.
- SPDX and inventory content are deterministic for a source commit and installed
  package set; generated data and scanner databases stay ignored.
- Trivy must recognize a supported target, preserves its JSON report on failure, and
  fails only for fixed HIGH/CRITICAL findings under the documented baseline policy.
- CodeQL builds the warning-as-error C++ target, and all workflow actions are immutable.

## Verification

- `make security-check`: PASS.
- `actionlint .github/workflows/build.yml .github/workflows/security.yml`: PASS.
- `./scripts/format.sh --check` and `git diff --check`: PASS.
- Full suite: PASS, 177/177 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation and Phase 00
audit.
