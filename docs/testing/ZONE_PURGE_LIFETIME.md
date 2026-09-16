# Zone purge lifetime regression (#229)

`zone_purge` used to save `next_in_room` before extracting an NPC. Production
`die_follower` can recursively extract the next NPC (a summoned follower), so
that pointer can refer to an already extracted character or freed storage.
The purge now records runtime identities, resolves each identity again in its
original room, and checks NPC/morph eligibility immediately before extraction.

## Local validation

Run `python3 tests/async/test_zone_purge_lifetime.py`. The test compiles the
production `zone_purge` and `die_follower` bodies under ASan/UBSan. Its controlled
extraction adapter exercises both retained and immediately freed allocations,
recursive follower teardown, a moved NPC, reused storage, and surviving PCs and
morphs. It also checks the existing object-purge policy. The original iterator
fails the double-extraction assertion; the candidate passes all four cases.
Combat-reference cleanup in this test belongs to the extraction adapter, so it
does not establish the correctness of production `stop_fighting` by itself.

Run `python3 tests/async/run_zone_purge_journey.py /absolute/path/dms` for
the real flatfile Telnet journey. It creates two disposable accounts, promotes
only their offline synthetic snapshots, and verifies 15 cycles of a mortal
fighting Raoul, an immortal purging the opponent, the mortal's combat reference
clearing, a full zone reset, and another usable player command. Both players
save and reconnect after a process restart. The local run passed all 15 cycles.

Local development builds passed for flatfile and MariaDB with warnings treated
as errors. The formatter check passed for `src/world/new_events.c`.

## Incident attribution and readiness

The original report was an intermittent full-world crash in room 402003 after
purging an apprentice and running `zresetfull`. No original core/ASan trace has
attributed that crash to this confirmed iterator defect. The synthetic live
journey uses the small test world and the destructive follower scenario uses
controlled extraction. A full-world sanitizer journey reproducing the original
sequence, with a real summoned master/follower chain, remains necessary before
claiming that all of #229 is resolved. That attribution does not block reviewing
this independently reproduced and tested iterator fix. The PR is ready; #229
remains open until the historical crash is attributed or its investigation is
otherwise concluded. No production player state was used for these tests.
