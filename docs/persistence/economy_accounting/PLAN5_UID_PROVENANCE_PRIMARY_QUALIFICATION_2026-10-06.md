# Exact UID provenance: primary integration

The provenance view treated a stored float or Boolean as the requested integer
UID because Python equality accepts those aliases. The new regression fails
against primary `a5a1f4b196496d50a6f46afecfec03aba3e66190`. The view now requires
the stored UID to have the exact integer representation before including it.

Only the completed source hunk and its unchanged 33-line regression from Plan5
`68c6938d9f0ddd2e308f31cea62e9df97746194d` are imported. Other pending Plan5
allocation-reader changes are separate. No production C++, schema, activation,
gameplay, or inactive spell behavior changes.

## Primary verification

Private evidence is `tmp/plan5-uid-provenance-primary-20261006/` and
`tmp/plan5-audit-integration-primary-20261006/pure-evidence/`.

- Original exact-UID regression: expected exit1 against the old reader.
- Maintained source: three original/new provenance methods pass in5.937s,
  zero skips. The regression exercises eight stored representations at limits
  0/1/100, preserves the snapshot/file bytes and global refusal, and compares
  actual CLI output to the view. Retained and unattributed histories remain.
- The complete incoming pure classes also pass:42 origin/export methods and
  131 reconciliation methods, zero selected skips. That combined private
  candidate contains additional allocation changes; its outcome is scoped to
  those frozen inputs and does not claim they have been integrated.
- Owned diff check passes. The two unrelated dirty shop harnesses and untracked
  session report are excluded and preserved.

The maintained receipt records a corrected test-selection error: an initial
invocation named a nonexistent third method; the first two methods passed.
The authentic three-method invocation above is terminal PASS. No test was
removed or weakened.

Maintained source SHA256:
`42b6442a80c9e5899fdfb178c8313d81f0a122f41ad178203c06bc7beb854c1c`.
Maintained test SHA256:
`19768d3c06f7164062b317aac048b77d5bb55b96fdb7071bed2bf065a2e2f73f`.
CLI log SHA256:
`6e83452544397d19a0300ebeaa0cc4487ba6546c4ade883a36c31438a83d7b6a`.

This closes the UID view representation issue. Partial-consumption snapshots,
their SQL batch-boundary repair, real producer/publication/recovery journeys,
full Plans/R1–R8 and release qualification remain open. Shared native full-build
failures and their private successor repairs remain recorded separately.
