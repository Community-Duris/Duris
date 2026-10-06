# Plan 5 native provider repair: primary integration — 2026-10-06

The native command implementation now references the actual shop-trade recovery
manifest provider. Nine original Python native recipes omitted that production
source and could fail at link time. Eight owned successors from
`cb5134186f93a4472671a8996b9723eb88e19d91` are installed exactly. Primary repairs
`test_flatfile_accounting_store.SOURCES` with the same real provider once; the
original restore helper continues importing that shared list, without a second
copy. Every original recipe AST is recovered by removing only the added provider;
compiler flags, sanitizer controls, native inputs, cases and assertions remain.

## Current primary checks

Source base: `8795a0b086a7f581c6c4b8080bc83245646fbf5d`. The installed nine Python
files parse; the importing restore helper retains its exact original bytes.
The original `test_flatfile_accounting_store.py` runs unchanged in the existing
GCC13.3 tool image with the actual current production sources and ASan/UBSan.
It passes pending journals, allocation recovery, hardlink and child-plan guards,
85 commit/recovery write/sync/rename/remove failures and process-exit cases,
canonical records, private append, retained replay, bounded bundles and crash
recovery. Exit0,162.437s. Normal accounting validation and generated matrix check
also pass (exit0,10.453s and17.954s). Local full commands and log hashes remain
in `bin/tests/plan5-native-provider-primary-20261006/results.json`.

This repairs the missing providers. Other original native audit recipes retain
Plan 5's older-source qualification; the separate child expectation and census
capability defects are subsequent issues. No claim is made that all native
recipes or current full release qualification passed. R1–R8, full R6, producer
journeys, both-engine SQL integration and release qualification remain open.
Accounting stays inactive and the unrelated shop harness/report changes remain.
