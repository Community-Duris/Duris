# Native baseline initialization marker preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. Independent persistence
review found no blocking source defect in the four assigned authority/baseline
files. No compiler/test/AST/native/SQL/service run or milestone push occurred.

Before, an empty baseline namespace authorized fresh initialization after complete
book deletion, and no authority-bound catalog marker made that loss discoverable.
The native owner now implements the [v2 interface](BASELINE_INITIALIZATION_MARKER_INTERFACE_V2.md):
a160-byte retained epoch row records unknown/never/initialized state, original
initialization ID and canonical opening key. `authority.eal` binds the entire
catalog digest. New append proves never initialized; v1 remains unknown. Selection,
deactivation and later epochs retain markers. Only the epoch decoder accepts v2.

Initialization stages head+16indexes+catalog+control into a temporary complete
19-image shared-journal bundle. Failed allocation/capacity/stale revision/duplicate
target leaves caller operations unchanged. Exact initialized retry verifies the
original ID/key, head and16indexes without appending writes; missing initialized
files refuse. Revision-zero heads bind initialization ID and empty indexes. Intact
unknown historical/retained retry evidence stays readable; new initialization and
new preparation for unknown histories refuse. Private storage permissions remain.

This is initialization history, not holdings/custody/source attestation or activation.
Retry does not verify every historical witness or namespace closure. Native
control_read does not scan all baseline namespaces: independent restore completion
still requires Plan5's v2 marker enumeration/cross-checks. Existing lifecycle
staged-state composition remains unresolved. V1-only readers and old17-image
fixture expectations must be updated through their owning interfaces at the
major-plan batch; no existing assertion was silently weakened. An explicit
independently verified pre-marker migration remains unimplemented. Overflow at
control revision UINT64_MAX fails closed through the existing invalid-status
mapping; richer capacity classification remains a diagnostic refinement.

Private owner `tmp/baseline-marker-prepared-v1/` preserves546 consumed inputs.
BEFORE manifest: `871daa0abc605b1c0734cf43214bc51d96b5fe512690246361dfa99cc9e8a5f5`.
AFTER: `4f3cfbc38c50b059a20c1d36a18255cef5637302987dff5c36b4ce7e886587cd`.
Prepared unexecuted cases cover complete loss/each17namespace file, exact empty/
populated retries, ID/key/coherent catalog-head substitutions, v1 unknown reads/
refusals, retention, maximum4096catalog, OOM unchanged-output and all19 journal
publication/recovery boundaries. A separate actual-BEFORE-compatible loss owner
avoids treating unavailable new API compilation as semantic RED.

Reviewed source pins:

- authority.c: `9dca31532378a2ccca81542614d6f1c3e9b2bb511ccd93b98969f25ae2b6c3a7`.
- authority.h: `3b2c92de31a0e1a3aab9e2137ffd750f5c5acb07e787e2ba9241877ad7040bfa`.
- baseline.c: `4546ba39310329440a7bcb0f2d4ff4cca7d07629b98a76d34a566ddd09a1baca`.
- baseline.h: `4c20bf06b0958a6f87a247caa8b89e752168da40355d8620cb0f9db96144a61d`.
