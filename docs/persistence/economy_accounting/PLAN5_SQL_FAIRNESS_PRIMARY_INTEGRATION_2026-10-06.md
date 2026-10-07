# SQL audit scheduling refusal integration — 2026-10-06

The maintained reader now rotates a budget-refused namespace without advancing
its coverage cursor. Refusal counts remain sticky through successful sibling
pages. Transport, schema, source and cleanup errors retain their refusal behavior
and do not save a rotation. Valid v1/v2 progress upgrades preserve existing
namespace coverage; new scheduling history starts explicitly at zero.

This integrates peer21f5896f6 over maintained3b7a465af. The original protected
inline progress I/O remains; this SQL slice does not depend on the new flatfile
progress helper. All previously retained test functions remain, with explicit
rotation, legacy-upgrade and cleanup-refusal cases registered in the central
mandatory entry and the native configurations.

## Qualification

The unchanged original full suite passed67 methods with zero errors/skips in
663.284 seconds on the pinned Linux image. The exact mandatory manifest command
passed18 tests, including all17 required selectors. Both original sanitizer
probes compiled and ran with no native cache reuse; generated probe bytes match
the original source/mobile transformations.

Private MariaDB10.11.14 and MySQL8.0.46 at canonical sequence62 passed the
composite, maximum-baseline, original-plan/projection and resumed-page cases.
Across both engines there were88 original-plan observations; each engine passed
108 saved-alias controls and399 invalid-position controls. Each retained three budget-refused
namespaces across six CLI resumptions, while application inventories stayed
unchanged. All3132 selected source files and4534 exported native artifact
members were authenticated. Owned daemons, container and volume were removed
after export and their absence verified.

The first attempt preserved66 passes and one missing-registry transport error.
Its successor added only the original raw HEAD registry and writer-matrix
dependencies; no source, assertion, flag or deadline was changed to pass it.

Protected local receipts:

| Receipt | SHA256 |
| --- | --- |
| Source transport | `68a9108d7049c89f3e415901d056f04787ad02cdfb46f9940dc45feb5a85d178` |
| PRIMARY-QUALIFICATION | `6dc367a0a2c880f523ab6706988b1dedf5c8c8234293223f87811f0a79eac7e6` |
| Native export | `de6108615804ea3a3834afb793506492a41d8552a146d2552510bc987c2b1394` |
| Owned cleanup | `f598ee696e3479d6c67f2748b9beec1a2a7fc830f4efa1dcb0af536e5c5b155e` |

The private packet is
`tmp/plan5-sql-fairness-native-dependency-successor-primary-20261006/`.
The reproduction environment variable is `DURIS_PLAN5_CANONICAL_ARTIFACTS`;
the peer report's earlier CANONICAL6972 typo is corrected in peer7bec84ae9.

## Remaining work

This is reader/native-codec/private-SQL integration evidence. It does not qualify
the separate private753-provider producers, actual player journeys, full native
holdings, activation, all of R7/R8 or release. Coverage and release flags remain
false; accounting and the declined inactive spell path retain their behavior.
