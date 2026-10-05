# Shop native ordering and physical row-ID boundary — 2026-10-05

The shared world witness now exports `shop_trade_world_expected_player_order`.
Its exact reviewed C8a6a2841/Hb74ee118 supplies the previously private native
insertion-order conversion. The original owner delegates to it rather than
maintaining another implementation. The body is unchanged apart from name and
formatting; the new public interface is pure and grants no SQL/custody/ACK power.

Native obj_to_char/obj_to_obj inserts before the first sibling with the same
actual R_num; otherwise it inserts at the head. The helper observes the full
physical target chain, including NORENT anchors omitted from the player save,
and skips the selected node during resumed placement. Remaining saved siblings
must retain original order. Equipment prefix is preserved; the complete ordered
forest is rebuilt in DFS order with reindexed parents under the existing
4096-item/depth32/4MiB bounds. Outputs remain unchanged on refusal. Caller must
already own fresh complete world census and original player/keeper exclusion.

Private native cut proof02556592 adds another necessary boundary: SQL row IDs
are uint64, while obj_data::db_item_id is signed int. Every whole-player, keeper
and selected physical row ID must be1..INT_MAX, parent IDs0..INT_MAX, before a
future callback/assignment/output. Empty maps remain valid. The current private
ownerbb3d6e46 preserves that reviewed guard and calls the shared ordering helper.

Independent source review accepted the exact extraction/delegation and guards;
formatter fixed point, whitespace, source pins and source census pass. No native
compiler, gameplay, SQL or recovery test ran. The original successful native
publication/actorless recovery driver remains incomplete and has not been wired
to either helper. Guarded ACK, original transaction cleanup and complete
current projection qualification remain required. Inactive behavior is unchanged.

Parallel Plan3 native SQL participant is integrated0723e10a5 without a caller.
Additive0059 SQL8ca3e813/verifier21cc7ea9 is independently source accepted but
remains private with0057/0058; no historical migration entries or engine metadata
fingerprints are relabeled. The coherent chain and both-engine upgrade/retry
qualification remain at major-plan readiness. Flat NPC participant preparation
is separate; no backend parity or full native lifetime completion is claimed.
