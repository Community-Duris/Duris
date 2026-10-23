# NAH builder questions

These source-backed prompts are for the map, guildhall, mobile, object,
reset, shop, loader, tooling, and persisted-data owners. They record
unresolved decisions and recommended resolutions; they do not imply live
travel or reset testing.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| NAH-01 | Is “Nihilists Anarchists Horde Guildhall” the intended canonical title for this package? | Preserve the exact ZON title and record aliases only after the owner confirms them. |
| NAH-02 | How should all 13 Nah room records coexist with active Underworld3 rooms at the same VNUMs but with different titles and descriptions? | Preserve both source maps and resolve room ownership before any load or renumber proposal. |
| NAH-03 | What is the relationship among Nah, unregistered cabal_hall, and shadow_hall at overlapping room VNUMs? | Keep the three map identities separate until their owners document provenance and intended ownership. |
| NAH-04 | Is the #301 zone header and #30100–#30112 room band intentional? | Keep the range unchanged; the room band is consistent, while ownership conflicts with active Underworld3. |
| NAH-05 | Are the two up exits to Duris3 Ocean rooms #156123 and #179220 intended as Nah entrances or just source-map links? | Preserve both one-way source records and confirm their intended endpoints and return routes with the map owner. |
| NAH-06 | Are the 10-room and 3-room WLD components intentionally separated? | Preserve the authored graph and ask whether the portal objects are the intended connection. |
| NAH-07 | Should portal #30100 lead to #30111 and portal #30101 lead to #30103 when their descriptions promise the Icy Tundra and realms of Light? | Keep the raw value fields and descriptions; confirm the intended destination map and current VNUM owner. |
| NAH-08 | Is reset mode 0 intended to accompany 27 authored O/M/G reset commands? | Preserve the commands and ask the builder to resolve reset-mode intent before any runtime claim or edit. |
| NAH-09 | Are the duplicate high-priestess M reset rows at rooms #30100 and #30110 intentional? | Keep all four source rows until the owner confirms whether they are duplicate placement attempts or separate design. |
| NAH-10 | Are Kallisti, the high priestess, Kaberf, and Eris the intended mobiles for this guildhall? | Preserve their current prototypes and the competing source-only mobile VNUMs until ownership is confirmed. |
| NAH-11 | Are Player Castles objects #11061, #11013, and #11111 intentionally reset in these rooms? | Preserve the typed imports and confirm the room placements with both area owners. |
| NAH-12 | Is #6089 intentionally part of Eris’s 11-object shop stock when it has no current object prototype? | Preserve the stock reference while the builder locates its intended definition or resolves its removal separately. |
| NAH-13 | Which source identity should the shop and ZON G inventory use for #53, #108, and #109 when other inactive OBJ files reuse them? | Keep the VNUMs and ask the item owner to identify the intended object source. |
| NAH-14 | Is Eris #30103 intended as a shopkeeper in room #30102 when both prototypes are absent from the active full-area set? | Keep the candidate records and confirm the expected mobile, room, and normal loader source. |
| NAH-15 | Should nah.shp ever be included by the all-SHP fallback when normal AREA aggregation excludes it? | Treat fallback behavior as conditional code evidence; ask the loader and area owners to settle the intended file set. |
| NAH-16 | Is compiled inn behavior at #30104 meant for active Underworld3’s river room rather than Nah’s Treasure Vault? | Preserve the active code and ask the code/area owners to confirm the numeric binding before any map decision. |
| NAH-17 | Is Nah intentionally absent from areas/AREA and areas/qst/AREA? | Confirm deployment intent before editing either manifest. |
| NAH-18 | What editor project or exported map owns this five-file package? | Identify editor, tooling, and persisted map copies before proposing a move or archive. |
| NAH-19 | Do the followed 2009/2012/2013 same-stem histories capture the package’s full provenance? | Record the tracked history only and request owner evidence for anything outside Git. |
| NAH-20 | Should Nah remain source-only while VNUM, loader, reset, and ownership questions remain unresolved? | Retain all five files in place; make any later activation, move, or archive a separate owner decision. |

See the [NAH source audit](NAH_AUDIT_REVIEW.md) for the graph,
prototype, reset, shop, and history evidence behind these questions.