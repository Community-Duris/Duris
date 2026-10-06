# Player-give source classification — 2026-10-05

Plan2 Work2 source review found `submit_coin_give` already connects active
player giving to the typed coin owner. The registry and matrix described it as
legacy-only. This correction records the existing exact-denomination wallet root
and original-player wallet for morph recipients. Inactive schema1 behavior,
unverified backends, empty executable evidence and activation blocking remain.

Only `coin.player_give`, its generated interpretation, the source-connected
count (15 to16), and the generator's exact source pin change. No native source,
schema, producer or gameplay change. References: `src/cmd/actobj.c::submit_coin_give`,
`src/economy/currency_transaction.c::currency_transaction_submit_coin` and
`src/economy/economic_gameplay_authority.c::prepare_coin_transfer`.
The same review found split already connected; native qualification stays open.

The first original55-method coverage run found the changed generator's stale
hash. Updating it and regenerating the matrix resolved that failure. All55
original methods then passed with zero skips; matrix `--check`, normal accounting
contracts, runtime compatibility and whitespace checks passed. Census stays2871
occurrences/2812 unique sites/zero unmapped sites;899 routes,766 runtime routes,
85 schema1 paths and two unified evidence routes remain. Coverage is incomplete;
release is `BLOCKED`.

These checks qualify metadata only. Actual MySQL, MariaDB and flatfile peer/split
command-to-native gameplay, morph/endpoints, interrupted splits, held publication,
lost replies and restart remain original Plan2 acceptance. No native test/build
ran for this issue; major-plan runtime testing stays deferred. No Plan or R1–R8
requirement is marked complete.
