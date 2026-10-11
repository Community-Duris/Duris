# Guarded combined build checkpoint — 2026-10-11

Fresh production builds and links for MariaDB and flatfile pass at exact published source `83e2e4392064667fc4f64780183d17b5acc431ef`. The existing inactive-flat preflight also passes its original three boot/UID-refusal cases. This establishes the required combined build/boot prerequisite before responsibility extraction; it does not qualify the full active accounting protocol.

Source freeze SHA256: `4472e3c18ce1f39d439a9854dc05235935fb5cd05ebf7911ee52573b76226156`. Result SHA256: `9d5e84c09e3b355e9364d0b478b7c95d36387f995fd791ced2bcb5f8a6f02cc6`. WSL owner directory: `/var/tmp/guarded-candidate-83e2e4392-root-20261011`. Compiler: GNU13.3/C++20. Both backend builds use the original production flags, distinct fresh object directories and jobs4. Private linker aliases resolve the existing real system libraries; there are no fabricated bodies, system edits or source patches. Tracked source was frozen from Git; protected/unrelated WIP is excluded.

| Check | Result | Elapsed seconds |
| --- | --- | ---: |
| MariaDB production compile/link | PASS | 538.403 |
| Flatfile production compile/link | PASS | 475.21 |
| Original inactive-flat preflight | PASS | 2.151 |
| SQL startup/service smoke | NOT_RUN | — |

Retained MariaDB binary SHA256: `3e6a2cf0a407fdd371e389ac33838b5163fa06745e1365eca271095d9cd974c0`. Retained flatfile binary SHA256: `35af6746fde05c619e9762050df5784cb712a5e0df85b0a27d52fc2d80352307`. The preflight script is unchanged, SHA256 `cd454f0ed34d5e8f066a175ad7c7a64ebd65e543e25b6aa719a2a4abb90e0638`. Original commands, exit codes, logs, binaries and resource checks remain at `tmp/guarded-candidate-83e2e4392-20261011/native/`; all cited logs/binaries were rehashed before recording this checkpoint.

The same live process was polled to terminal exit0; it was not restarted after observation yields. MariaDB log SHA256: `451f1d757aa3629883258a1af735d1a992f5e9b2375d35855144347fc5ac651e`. Flatfile log SHA256: `eebfce39fa526b2bd165308ed5380e41568a2abb71c4493cddda01c16f278288`. Inactive boot log SHA256: `8ca58a7d60ee77b883e8448977a061f28e0c2ef45da2bd9bd1cb787d40f4ba9f`. Source remains unchanged; accounting is inactive and admission CLOSED.

SQL startup needs its separate disposable schema65/service owner. Native resource/ABI/frame qualification, admitted player/writer journeys, actual MySQL/MariaDB persistence and recovery, backup/restore, retention/erasure/export policy, fault/restart/load and R1–R8 release gates remain open. Later provider implementations and architecture changes require their own affected builds and applicable executable evidence; this result does not transfer automatically to them.
