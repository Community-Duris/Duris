# Coin put failure after auction listing

Tracked in [issue #305](https://github.com/Community-Duris/Duris/issues/305).

Investigated against master `0a1129385` on September 13, 2026. Listing an auction
item can make a continuously connected player's next `put all.coins bag` fail
with `The coin transfer did not commit; nothing changed.` Repeated attempts fail;
a full character logout and login makes the same transfer work again. The target
bag can contain an ordinary item and no coins. Linkdead is not required.

The reporter subsequently confirmed that the affected player had used the auction
and had not gone linkdead. Auction use, the exact rejection message, the bag's
contents, and recovery after renting and relogging strongly connect this incident
to the reproduced ownership-revision regression. The deterministic reproduction
uses auction listing; the report confirms auction use without specifying the
individual auction action.

Auction settlement increments both the source and destination ownership revisions
in durable storage. The completion publisher in `src/economy/auction_transaction.c`
previously hydrated only the destination owner through the transferred item.
On a listing, that updated the auction owner while leaving the seller's runtime
owner revision behind. `prepare_coin_pile` in `src/cmd/actobj.c` then used that
stale seller revision for the bag transfer, which durable validation rejected.
Full login reloaded the current revision and cleared the failure.

The missing source publication dates to commit
[`119b10bf7fdb35f01cb33bb92bdbbfdfabc96de3`](https://github.com/Community-Duris/Duris/commit/119b10bf7fdb35f01cb33bb92bdbbfdfabc96de3),
`feat: cut over auction settlement`, dated August 27, 2026. Both the SQL and
flat-file auction repositories return the updated player and auction revisions;
the affected completion publisher is shared by both backends.

The current `prepare_coin_pile` path was introduced by
[`bda0978f5b0cd68f4560fbc988a946417f364984`](https://github.com/Community-Duris/Duris/commit/bda0978f5b0cd68f4560fbc988a946417f364984),
`Preserve coin custody and recover rejected player deaths (#158)`, dated
September 6, 2026. Earlier coin puts also used custody transfers. The history
above identifies the missing publication and the current failing path; the
first affected release has not been established by a historical gameplay bisect.

The fix publishes the source revision as well as the destination revision after
a committed item transfer, before invoking the auction completion callback. It
preserves a newer source revision if one has already been published. This also
updates the auction source revision when an item is claimed. Rejected auction
transactions do not publish ownership changes.

To run the focused publication regression, including rejected settlement,
listing, claim, and preservation of a newer source revision:

```sh
python3 tests/async/test_auction_ownership_publication.py
```

The test compiles the actual completion publisher with the real ownership runtime
and codecs under AddressSanitizer and UndefinedBehaviorSanitizer. It fails against
the original publisher because the seller remains at revision 4 instead of 5,
and passes with the fix. It runs in the code-quality workflow without requiring
a MySQL client.

The live reproduction uses an isolated flat-file server and temporary synthetic
account, character, world, and authority data. It puts food in a bag, acquires ten
platinum, lists a separate item, then immediately puts the remaining coins in the
bag. It checks repeated transfers and the exact balance after a full logout and
login. It does not read the checkout's `.env`.

```sh
python3 tests/async/test_flatfile_auction_coin_put_journey.py
# Or reuse a flat-file binary:
python3 tests/async/test_flatfile_auction_coin_put_journey.py --server /absolute/path/to/dms_new
# To verify the original failure and reload workaround with an unpatched binary:
python3 tests/async/test_flatfile_auction_coin_put_journey.py --server /absolute/path/to/original/dms_new --expect-regression
```

Validation performed: original connected-session failure and full-reload recovery;
fixed connected-session transfer and repeated/reloaded balance checks; focused
sanitizer regression; existing auction transactional-cutover and coin-command
contract tests; MariaDB and flat-file server builds; source formatting check.
Live gameplay was exercised with the flat-file backend. A live SQL-backed server
was not exercised. No production state was changed.
