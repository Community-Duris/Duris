# Empty SHP Candidate Audit Review: `cript`

Reviewed October 11, 2026, Checkpoint 465. This is a static file, history, manifest, and shop-loader-path review of the unregistered `cript` candidate. It does not establish why the file exists, whether external editor state refers to it, or permission to delete or move it.

## Source scope and history

The only current candidate file is [`areas/shp/cript.shp`](../../../areas/shp/cript.shp). It is zero bytes and contains no shop record, shop VNUM, keeper, room, product, or message. There is no same-stem WLD, MOB, OBJ, ZON, QST, or story sidecar; the coverage row has no title or raw area annotation. The candidate is absent from the full [area manifest](../../../areas/AREA), the [quest manifest](../../../areas/qst/AREA), and current `RANDOM_AREA`.

Followed Git history reaches the initial 2005 revision. The path moves through the old Players tree in 2006 and into `areas/shp` in 2009; same-stem maintenance appears in 2013. The current tracked blob remains empty. These path records do not provide an area title, narrative, shop identity, or replacement decision.

## Static loader path and dependency scope

In this checkout, `areas/world.shp` is absent. The non-mini [`open_shop_stream`](../../../src/economy/shop.c#L3267) path first tries that aggregate file and, if it cannot open it, globs `areas/shp/*.shp`. That glob can include `cript.shp` even though `cript` is not in the full area manifest. Since the candidate file has zero bytes, concatenating it contributes no shop record or typed VNUM. This is a code-path observation, not evidence that the shop system was run or that the file has an operational role.

Exact filename searches found no reference to `cript.shp` in the current area manifests, area sources, or C++ source. With no SHP records and no WLD/MOB/OBJ/ZON files, there are no candidate room, mobile, object, exit, reset, shopkeeper, or product identities to map. Dynamic editor state, external tools, and persisted data are not established by these repository files.

## Retention

Keep the zero-byte file RETAIN IN PLACE until its owner confirms whether it is a deliberate placeholder, a legacy path marker, or an accidentally empty shop file, and whether any editor or external process tracks it. The static review provides no basis to remove it.
