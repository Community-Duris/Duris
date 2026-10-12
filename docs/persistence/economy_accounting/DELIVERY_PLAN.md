# Accounting delivery crosswalk

Use the [completion plan](FINISH_ACCOUNTING_PLAN.md) for execution and
[R1–R8](REMAINING_REQUIREMENTS.md) for acceptance. The five domain plans retain
their detailed functional requirements; their old starting revisions and status
paragraphs are historical. Historical handoffs and reports are retained in the separate accounting context worktree; the completion plan governs current delivery.

| Domain plan | Completion responsibility | Contract |
| --- | --- | --- |
| [1. Authority/cutover](plans/01_AUTHORITY_AND_CUTOVER.md) | Shared coordination, atomic owners, baseline, publication, guarded activation implementation | R1, R6 |
| [2. Money/supply](plans/02_MONEY_AND_SUPPLY.md) | Holdings, transfers, piles, sourced issuance/expense, event dedupe | R2, R3 |
| [3. Item provenance](plans/03_ITEM_PROVENANCE.md) | UID supply, custody/topology, retirement and restoration identity | R4 |
| [4. Compound domains](plans/04_COMPOUND_DOMAINS.md) | Shop/Collector/auction/craft/death/resurrection composites | R5 |
| [5. Independent audit/release](plans/05_AUDIT_OPERATIONS_AND_RELEASE.md) | Independent reconciliation, operators, lifecycle, workload and final certification | R7, R8; independent coverage proof for R6 |

Issue [#490](https://github.com/Community-Duris/Duris/issues/490) is the technical
implementation/qualification tracker. Historical issue closures are supersession
records, not acceptance waivers. [#664](https://github.com/Community-Duris/Duris/issues/664)
retains separately authorized historical-case reconciliation. Preserve scoped
delivered fixes, including #551/PR #693. SQL is first; both SQL engines and flatfile
remain required for full completion. Technical completion does not authorize
production activation, deployment, migrations or restitution.
