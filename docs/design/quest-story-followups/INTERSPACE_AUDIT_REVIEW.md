# Interspace audit: graph, boundary, and typed references

This static supplement accompanies the [Interspace environmental dossier](../zone-stories/INTERSPACE.md). It reconciles the local WLD graph, active and inactive WLD boundaries, typed area references, normalized title, and tracked source history. No native area or gameplay source was changed.

## Static connectivity and boundary

The active `interspa` contributor has 100 rooms (#99900–#99999) and 601 authored exits. All targets resolve: 600 remain in Interspace and one leaves the area. The local weak graph is one component, and its directed graph is one strongly connected component of 100 rooms. From #99900, all 100 rooms are forward-reachable and all 100 can return by local WLD exits.

The sole WLD boundary is room #99999 southeast (`D9`) to Astral Main #19801. The exact record is [cross-area exit-register row 1189](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1189), and the [pair summary row 202](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L202) records one outbound exit with no reverse pair. No Astral Main WLD exit returns to #99999.

I checked all 351 active WLD contributors and all 95 non-active WLD source files for targets in #99900–#99999 and for room-number overlap. Neither set contains an inbound WLD edge or overlapping room definition. The outbound Astral edge is therefore the only checked-in WLD boundary; non-WLD travel references are classified separately below. This is source topology, not proof of access, movement, door state, or safe return. See the [Interspace WLD](../../../areas/wld/interspa.wld#L2971) and [Astral Main room #19801](../../../areas/wld/astral_main.wld#L4004).

The Astral exit stores raw state `4` at [WLD line 3003](../../../areas/wld/interspa.wld#L3003). In [`setup_dir`](../../../src/world/db.c#L1545), the loader masks state to its low two bits before deciding whether the exit is a door. The result is zero, so this raw value does not set `EX_ISDOOR` during WLD setup. The local ZON contains no `D` reset command. This is source-level loader interpretation, not a live movement test.

## Room flags and loader boundary

All 100 rooms use sector value 22 (`SECT_ETHEREAL`). Ninety-nine rooms use flag value `1241808960`; #99999 uses `1241808896`, a difference of 64. Under the room-flag definitions, bit 6 is `ROOM_UNDERWATER`; the shared flags also include `ROOM_JAIL`, `ROOM_HEAL`, `ROOM_GUILD`, `ROOM_NO_PSI`, and `ROOM_ATRIUM` ([definitions](../../../src/core/defines.h#L396)). Thus #99999 is the only room on the Astral boundary without the source underwater bit. These records do not establish how the repeated flags should affect access or gameplay.

## Typed dependencies and tracked history

`interspa` is registered in [`areas/AREA`](../../../areas/AREA#L364) and in the QST roster ([`areas/qst/AREA`](../../../areas/qst/AREA#L322)), but there is no local `.qst`, `.shp`, or story sidecar. `areas/obj/interspa.obj` is empty. The only local mobile prototype is #99900, the vaporous mist.

The local reset file contains two mobile (`M`) commands, at [lines 10–11](../../../areas/zon/interspa.zon#L10): each names mobile VNUM #2, with destination rooms #99900 and #99937. `areas/mob/limbo.mob` defines #2 as a black Durian dragon ([definition](../../../areas/mob/limbo.mob#L12)); the Interspace mobile file instead defines #99900 as the swirling mist. The local reset commands do not reference that mist. A role-based scan of all 442 ZON files found no foreign reset into Interspace rooms #99900–#99999 or its local mobile #99900. The same scan found no exact outside reset reference to an Interspace object because its local OBJ file defines none.

The all-area QST, SHP, and structured story-sidecar scans found no exact foreign consumer of an Interspace local prototype. The external object-type-25 scan found one distinct destination declaration into the room range: active Ravenloft 2 object #59310 stores `value[0] = 99999` ([object](../../../areas/obj/ravenloft2.obj#L5152)). Ravenloft 2's QST awards #59310 in the Jander Strahd finale ([QST](../../../areas/qst/ravenloft2.qst#L2431); structured story contract [lines 1962 and 2006](../../../areas/story/ravenloft2.story.json#L1962)). `ITEM_TELEPORT` is type 25 ([definition](../../../src/core/defines.h#L140)), and the item travel handler requires `value[1]` to equal the activation command before it uses `value[0]` as the destination ([handler](../../../src/magic/spell_travel.c#L949)). This object's `value[1]` is also 99999. The checked-in sources establish a typed destination and reward reference; they do not establish a matching command, actual activation, or return path.

No external MOB or OBJ prototype duplicates a local Interspace mobile or object VNUM. Ravenloft 2 room #59310 and OBJ prototype #59310 are separate type namespaces; the object number does not claim the room destination. A direct scan of active C/H sources found no active compiled reference to the Interspace room range. These bounded source findings do not account for persisted player-held objects or runtime-generated state.

Followed `interspa.wld` history includes the 2013-11-13 source edit `71d731c5b` and the 2009 `areas/` tree move `642da5400`. The MOB history also includes `bb0d59425` from 2012; ZON history includes `f18992b8f` from 2015 reset-field maintenance. The empty OBJ file's followed history reaches the 2009 move, the 2006 directory addition, and the 2005 initial revision. The normalized title “Interspace” is unique among the 442 current ZON titles. No alternate-stem predecessor, title rename, or explicit whole-area replacement was identified in the reviewed history.

The separately registered `ethereal_main` contributor is titled “The Ethereal Plane” and owns rooms #12401–#12550 ([roster](../../../areas/AREA#L70), [WLD source](../../../areas/wld/ethereal_main.wld#L1)). Astral Main owns the distinct #19701–#19825 range. Those separate titles and source ranges, together with Interspace's single outbound edge to Astral Main, establish a thematic connection but no duplicate room ownership or whole-area replacement. See the [Ethereal Plane dossier](../zone-stories/THE_ETHEREAL_PLANE.md) and [Astral Main dossier](../zone-stories/ASTRAL_MAIN.md).

## Disposition

Keep active `interspa` **DO NOT MOVE**. It is a current full-area contributor with a strongly connected local grid, one one-way Astral WLD edge, two source reset records importing Limbo mobile #2, and an external Ravenloft 2 type-25 destination into #99999. No source-only WLD approach, room overlap, or attic candidate is warranted. Preserve the active references and open builder questions without inferring a usable entrance or safe return.
