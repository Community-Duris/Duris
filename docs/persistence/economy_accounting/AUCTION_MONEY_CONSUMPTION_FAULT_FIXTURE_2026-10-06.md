# Auction money consumption fault regression — 2026-10-06

The original money-claim fault fixture installed a `BEFORE UPDATE` trigger on
the immutable source table. Canonical0062 records consumption by appending to
`economic_pending_claim_consumption`, so that trigger did not interrupt the
transaction. The preserved original run reached a successful claim where the
test expected a failure.

The fixture now injects failure into the actual consumption `INSERT`. It compares
the complete 22-table transaction snapshot after rollback, requires no successful
source claim, and verifies that the original source rows remain byte-identical.
The successful retry must append the two exact original source slots/amounts,
retain one receipt and source claim, and preserve the complete committed snapshot
after reconnect. All prior assertions remain.

The original native major batch passes on fresh MySQL8 and MariaDB10.11 with all
canonical0–62 migrations and their runtime verifier. Each original nine-case set
runs once with the additive source-claim controls; all18 cases, all seven strict
native profiles, the original flat unit, all18 SELECT-only source gates and both
independent canonical audits pass. Primary rechecks all175 sealed artifacts.
The money wrapper adds one private include and wraps four calls to the same real
owner; its exact raw inverse restores the complete standalone fixture. The
standalone test translation unit also passes its original strict compile profile.

This is a test-only solved issue. The qualified source archive is
`743d6aafa75d66d9ff64bf493f06dc638c11c3754949a0e8667a8417677132fe`,
based on `b687b199fd0c9185f87d793b3318f2d4b6d06ee2` with the separate nine-file
source-claim candidate and this fixture. That production candidate remains
uncommitted pending its fresh740-provider builds. The native batch does not
qualify the full maintained server, auction gameplay publication/ACK, cold world
recovery, complete activation, any whole Plan or release.

The adjacent JSON records exact source/fixture/evidence hashes and scope.
Private logs, binaries, snapshots and archives remain under
`bin/tests/auction-maintained-native-primary-20261006/combined-once-1`.
Both fixture-owned SQL engines were removed and absence verified. No production
data, live game, inactive accounting behavior or activation gate was changed.
