# Exact accounting source pins across checkouts — 2026-10-05

The independent Plan5 [combined build/recovery report](PLAN5_COMBINED_BUILD_RESTORE_AND_PIN_HANDOFF_2026-10-05.md)
established one remaining provenance-contract failure on frozen55a314496.
The repaired placement/unload/status contracts passed; the five existing source
pin values differed from the peer's exact source bytes. Primary refreshed peer
3e1c9c865 and inspected the actual2a3c05f3 checkout before changing anything.

## Established cause and complete source repair

All five mismatches are checkout line endings. Their indexed Git source blobs
contain LF; this Windows checkout contained CRLF. The previous pins correctly
hashed those Windows bytes but could not match a Linux checkout. For all58
current pinned components, LF-normalized original checkout bytes exactly equal
their Git preimage. No source-content correction or weakened hash assertion is
needed.

The five affected files are economic_command_admission.h, both
flatfile_accounting_coin_transaction files, and both inert_item_stage files.
Their current checkout bytes are normalized to the existing LF Git blobs. The
five registry hashes and their generated matrix copies now use those exact
bytes. `.gitattributes` explicitly requires `text eol=lf` for the58 existing
pin paths, preventing the same failure after another checkout. Existing
attribute rules are preserved; unrelated paths receive no new policy.

The strict raw-byte provenance contract remains unchanged. The registry and
matrix retain the same integrated/unqualified status, historical source/base
fields, scope, ownership, routes, evidence and incomplete coverage. Independent
source review proves both JSON files differ only in the five hash values.

## Evidence and limits

`tmp/cross-platform-source-pins-20261005.json` records all58 Git preimages,
original raw hashes, exact CRLF counts and repaired LF hashes. Its SHA-256 is
`02a9596d4b67a24f66b769cec704b5e0e79a7e75bd48e2ece3e5b1a131a0fdf8`.
Independent source review confirms every repaired raw hash, every effective Git
LF attribute, exact Git source bytes and unchanged metadata outside five pins.
The existing coverage generator ran successfully:887 rows,879 function anchors,
2843 occurrences,2784 unique sites,0 unmapped; coverage=false/release=BLOCKED.
These are source/metadata checks, not executed writer qualification. The strict
71-contract rerun remains at the major-plan batch requested by the user.

The peer report is imported as the exact e1fe6218a Git blob. Its two full
maintained builds and three managed recovery tests passed with zero skips on
its frozen55a314496/canonical0056 inputs. These prove that frozen candidate;
they do not qualify the subsequent active-custody helper, private shop/NPC
changes, real player journeys, coherent0057–0059 or complete R1–R8 release.
No new local compile, tests, SQL, gameplay, recovery, production operation,
activation or declined inactive-spell change occurs in this repair.
