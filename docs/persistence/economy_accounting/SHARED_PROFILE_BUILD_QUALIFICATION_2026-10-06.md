# Shared producer build and first full-tree component qualification

Date: 2026-10-06. Maintained parent: `79df6775b92c50d9a423397e477b5e24b9e3c59d`.
The tested producer source is a separate private candidate, not this maintained
parent. No production service, player connection or activation is involved.

The combined source contains the shared startup owner, native auction custody,
publication/replay integration and a narrow cold-SHOP object-pool reservation.
Two SQL-only helper definitions failed the original strict flatfile build as
unused functions. Their conditional guards preserve the real SQL bodies and
all other source bytes. Both failed predecessors remain preserved.

Frozen source archive SHA256:
`4f8332ce8e7b1eab670ee59ef38e40dade3608d489124381300913860eb20658`.
Source manifest SHA256:
`259325cb2211214d461991694d6ead67edbb2bdf22843120bf776e41ce936933`.
Build handoff SHA256:
`85bcf5e3c469f83c53f12b1baffc249d72b436b6826317d606b46ee90c45a046`.

Both original production Make builds now pass with their original strict flags,
macros and full 753-provider links. SQL takes 22.91 seconds, compiling the changed
pipeline and reusing 752 authenticated objects. Flatfile takes 186.02 seconds,
compiling 227 units and reusing 526 authenticated objects. Reuse is bound to
source, headers, dependency files, compiler argv and toolchain. All nine original
contracts pass. Primary independently authenticates all 23 result artifacts,
6367 source members and both complete 1508-member object/dependency caches.

Protected build evidence:
`bin/tests/native-auction-profile-guards-major-production-primary-20261006/`.
Primary receipt: `PRIMARY-BUILD-AUTHENTICATION.json` in that directory.
This establishes production compilation of the private candidate. It does not
establish runtime, cold recovery, current maintained-head qualification or
complete producer coverage.

The first full-tree SQL component attempt authenticates both caches, compiles
the strict original driver in 5.868 seconds and links the complete original graph
in 14.649 seconds. MySQL migrations through 0063 and runtime compatibility pass.
It then refuses at the original initial player-row/sidecar projection assertion,
before any auction case. MariaDB does not start under the original fail-fast
policy. The returned participant errno was not logged; no exact first-return
cause is claimed.

Source review establishes a missing required fixture prerequisite: the original
projection participant reads its result through the native runtime reader, which
requires a genuinely parsed/sealed recovery object-template catalog. This
component fixture never initializes that catalog for its fictional vnums
501/502/503. A faithful successor must use the original parser/catalog setup,
retain the real writer and every assertion, and observe the actual refusal code.
No production-source fix, template stub, SQL backfill or weakened readback is
justified by this failure.

Protected first-failure evidence:
`tmp/auction-native-profile-guards-full-tree-launch-primary-20261006/`.
Terminal seal SHA256:
`07164be78470fd6af998fec4537886949fce085b86fc1726d92e5648a204981f`.
The owned MySQL daemon exits zero and the container is removed. Owned evidence
volumes are retained. The component's physical-gameplay and production-route
qualification remain zero. Allocator/cold checks proceed independently; no
whole Plan, R1–R8, activation or release gate is promoted.

## Genuine allocator and cold recovery follow-up

Both original actual-mm units pass against the same candidate's complete753
provider links: SQL0.177 seconds, flatfile0.122 seconds. They exercise existing
free capacity, configured empty-chunk reservation, acquire/release/refill,
geometry/statistics overflow and mmap refusal with unchanged state. Primary
checks every provider/dependency hash, exact original link substitution, actual
ELF/logs and all26 exported artifact members. Unit receipt SHA256:
`44e2e9fbfe365116cecbfc61f39adf4c4079e75ce6b55fdaca13fdf992762090`.
Protected unit evidence:
`bin/tests/native-shop-pool-profile-guards-regression-primary-20261006/unit-1/`.

The original same-ELF MySQL warm publication passes in0.975 seconds. Cold exits2
in1.480 seconds at `original save execution/admission finish refused`. The
unchanged driver's control flow proves recovery returned ready and its following
regular-authority/recovery-off/retained-admission-guard assertion passed. The
combined `player_save_pipeline_start() && finish_boot_admission()` condition
does not identify which call returned false; later native cash/stock/journal
checks were not reached. This is not completed cold qualification.

All47 sealed artifacts and the454236160-byte archive are authenticated. Its507
regular members retain recorded native sizes/modes. Handoff SHA256:
`4735533ebb9faaeaad958b00dacd1d5112ca0f8d5f574e3991a9ee9c4a3e7209`.
Protected cold evidence:
`bin/tests/native-full-cold-shop-pool-profile-guards-primary-20261006/combined-1/`.
Both owned containers and all three volumes have verified cleanup absence.
MariaDB and origin-INSERT journal fault cases remain unrun under the original
fail-fast policy. Exact refusal diagnosis continues; neither this observation
nor the passing allocator controls complete a Plan or activation/release gate.
