# Retained claim allocations: primary integration and qualification

Restore and canonical SQL audit ignored retained claim origins and consumption
rows. The two original regressions fail against the previous reader because
unattached allocations are accepted. This integration imports the completed
Plan5 fix from `bfbc513e8b959a03613da5b3f366575c36398eb1`, with its final pagination
and canonical0062 labels at `1b4ab18728e10b302d72ecff38650451a4fac53c`.

The independent SELECT-only readers now bind original positive lots and
whole/partial debits to the decoded original account effects. They reject
orphaned, missing, extra, mixed, overdrawn and mismapped allocations, preserve
fully consumed original credits, and check declared version1 opening slots.
Historical NULL origins retain unknown coverage. Original PID/policy source
authentication and current native balances remain separate producer contracts.

## Current primary evidence

Base: `26d7b66b86e1a38d09430386be257a065fceacd5`. Six code/test files exactly match
the peer's final bytes. Only the15-line claim section is imported into
`AUDIT_OPERATIONS.md`; unrelated peer documentation is not substituted.
Unrelated physical/publication harnesses and the untracked session report retain
their original hashes. Sealed migrations and production source remain unchanged.

Private artifact: `bin/tests/plan5-retained-claim-primary-20261006/`.
Its authenticated3,126-input archive SHA256 is
`8c003ac57b8ab1de9a10f2a74637202f0cfab9ded544cd23f77b189958afdc7b`.
The source-only offline tools image is
`sha256:4994cc50a09a4acd40462ff412f3c3df21fc7c23a3f298f7fa725f0e2a350fb3`.
The native log SHA256 is
`93da42a6840c3440f4bfeb9a91a53ff9d48b7277c1144cdc4c13f0cc546bb997`.

- Original two failure controls: exit1, both `RuntimeError not raised`.
- All33 pure reader methods: exit0,1.119s, zero skips.
- Original full native/SQL restore method: exit0,666.296s; source observer
 667.716s, all frozen input hashes unchanged. Both fresh canonical0062 engines
 identify MariaDB10.11.14 and MySQL8.0.46.
- Original32 coin cases,3,026 decoder decisions, both native codec profiles,
 four native claim controls and identical original output bytes pass.
- Each engine passes105 restore/audit cuts,89 refusals,55 full-entry cuts,
 259-root pagination and21 allocation cases. The258-source/257-consumption
 second-page corruption is refused by both original readers. SELECT-only
 audits preserve the private application-table contents.
- Original providers, ASan/UBSan flags, child deadlines and the1,800-second
 original method deadline remain; native cache is disabled and zero objects
 are reused. Changed C++ lines pass clang-format18 and the owned diff check.

No new production source is compiled in this issue. It closes the retained
allocation-reader omission, not a whole Plan or release gate. Source capture,
the new original-policy/PID handoff, current balance reconciliation, complete
producer/publication/cold-recovery journeys and full release remain unfinished.
