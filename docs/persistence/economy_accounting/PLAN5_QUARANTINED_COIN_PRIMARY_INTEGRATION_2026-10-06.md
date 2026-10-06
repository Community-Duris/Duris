# Quarantined coin reader: primary integration — 2026-10-06

The exporter already captured quarantined coin piles, but the independent reader refused their existing state before producing findings. It now accepts that state and reports quarantined_coin_pile, while retaining those coins outside active holdings and live-pile totals. Unknown states and negative denominations still refuse; missing payload remains unknown. No correction or authority mutation is added.

Five owned source/test/report blobs are imported exactly from `eadeec0e799829e782f44560c58c60f4dcc66f73` onto primary `92ca96795ccb0da64b56e1f3a367b496f0178a58`. Existing primary preimages equal the issue parent, and all committed code/test/schema inputs match the peer's qualified composition. Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97` and migration tree `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d` remain exact. The complete central manifest, existing class discovery, required native baseline registration, original deadlines, flags and markers are unchanged. The two unrelated SHOP harness edits and untracked restore report retain their exact bytes and are excluded from staging.

Primary ran the new actual CLI regression, Python AST and normal accounting/matrix/current61 runtime metadata checks:

- `tests/async/test_reconcile_economy_accounting.py`: exit 0, 3.063 seconds; log SHA256 `5d436558ca52ad38a9d6afead82a62ed9de0654ab6953670ba681cbbca7926c3`.
- `scripts/validate_economy_accounting.py`: exit 0, 8.438 seconds; log SHA256 `43c93c673bfe11706e400dabe86b1290556cc11622ce68496dedb6c745771a50`.
- `scripts/generate_economy_writer_coverage.py`: exit 0, 15.547 seconds; log SHA256 `65f28430bff41eab2ff97fcba3d7e1e2dd8b3cb942e67a451364a3e4bf11eded`.
- `scripts/validate_runtime_compatibility.py`: exit 0, 0.937 seconds; log SHA256 `79c955597d2c3c42dd4551c27855b4f76b5e2b7a178280dcc8f6c233e619118f`.

[Peer qualification](PLAN5_QUARANTINED_COIN_QUALIFICATION_2026-10-06.md): Peer reports 203 Linux methods, 120 Windows reconciliation methods, strict native SQL/client-free quarantine observers with ASan/UBSan, and the original complete baseline recipe on both canonical SQL engines passing with zero selected skips. Four added SELECT-role captures and their CLI observations preserve authority. These are attributed component/managed-restore results; protected peer logs were not inspected here.

No primary native, SQL, gameplay, restore or sanitizer rerun was performed for this Python fix. No real writer journey, complete native authority capture, private birth candidate or full release gate is established by these component results.

Exact import/check receipt SHA256 `6ba01504370aae45970e207b50db2eb6a188e9f7f68c66d7560a4e01e4b4efb1`; raw primary logs and pins remain in private `tmp/plan5-quarantined-coin-primary-20261006*`. Separate issue commit and normal push are performed only after exact staged-scope verification. Plans 1–4 implementation/qualification and full Plan 5/R8 remain open; coverage is incomplete and release blocked. Inactive accounting, safety gates and the declined inactive spell path remain.
