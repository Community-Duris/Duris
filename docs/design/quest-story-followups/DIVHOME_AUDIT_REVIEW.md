# Home of the Divine typed-dependency audit

Checkpoint 787 records a bounded static typed-reference review for active
Home of the Divine. The area files and native gameplay remain unchanged.

## Local inventory and references

Home of the Divine is an active full-area contributor with 122 rooms
(#40700–#40821), 62 mobiles (#40700–#40761), 83 objects (#40700–#40782), and
twelve QST givers. Its QST contains 32 Q/QA contracts. The ZON has 240
commands: 136 M, 30 G, 27 E, 22 D, 22 O, and three P. All 221 M/G/E/O/P
prototype fields, including P container fields, resolve to local prototypes;
the local M/O placement and D room fields also resolve locally.

The local QST has 66 typed item fields: 56 resolve locally, nine name active
external objects, and one reward (#31341 at row 84) has no current object
prototype. The missing Relazier reward remains the existing builder question;
no replacement is inferred. Its external item references are:

| Owner | Item | QST reference |
| --- | --- | --- |
| Storm Port Stronghold | #22625 | [R row 72](../../../areas/qst/divhome.qst#L72) |
| Cerebusp | #22020 | [R row 80](../../../areas/qst/divhome.qst#L80) |
| Airship Grave | #77539 | [G row 161](../../../areas/qst/divhome.qst#L161) |
| Heavens | #392 | [G row 184](../../../areas/qst/divhome.qst#L184) |
| Harpy Heights | #31105 | [G row 276](../../../areas/qst/divhome.qst#L276) |
| Surface | #500026 | [G row 281](../../../areas/qst/divhome.qst#L281) |
| Moria | #99061 | [G row 286](../../../areas/qst/divhome.qst#L286) |
| Jade | #76697 | [G row 395](../../../areas/qst/divhome.qst#L395) |
| Cerebusp | #22032 | [G row 417](../../../areas/qst/divhome.qst#L417) |

The local SHP contains one record, shop #40712, with seven products (#40705–
#40711), all local objects ([source](../../../areas/shp/divhome.shp#L1)). The
coverage register's prior zero-shop count missed the `#40712~` header; it now
records one shop.

The local story sidecar contains 139 typed VNUM fields: 27 `mob_vnum`, 70
`giver_vnum`, and 42 `item_vnums`. Its mobile and giver values resolve locally.
Seven item fields repeat imports #77539, #392, #31105, #500026, #99061,
#76697, and #22032. The remaining typed item fields resolve locally.

## Active external consumers

Nine active external ZON reset rows import Home of the Divine objects:

| Consumer | Imported prototype rows |
| --- | --- |
| Cerebusp | G #40774 ([row 261](../../../areas/zon/cerebusp.zon#L261)) |
| Desolate | G #40768 ([row 452](../../../areas/zon/desolate.zon#L452)) |
| Desolate Under Fire | G #40768 ([row 477](../../../areas/zon/desolateinv.zon#L477)) |
| Surface Mini | O #40743 ([row 60](../../../areas/zon/surfacemini.zon#L60)); G #40746 at rows 160, 162, 178, 180, and 181 ([first row](../../../areas/zon/surfacemini.zon#L160)) |

Six active external QST item fields also use local prototypes: Airship Grave
accepts #40778 and returns #40779 ([rows 182–183](../../../areas/qst/airshipgrave.qst#L182));
Cosmic accepts #40731 and #40733 ([rows 136 and 141](../../../areas/qst/cosmic.qst#L136));
Mira returns #40738 ([row 55](../../../areas/qst/mira.qst#L55)); and Stronghold
returns #40771 ([row 16](../../../areas/qst/spshold.qst#L16)).

Five structured references occur in two other active story sidecars. Airship
Grave identifies Emition #40712 as a mobile and giver and tracks item #40778
([contact](../../../areas/story/airshipgrave.story.json#L197),
[contract](../../../areas/story/airshipgrave.story.json#L865),
[item](../../../areas/story/airshipgrave.story.json#L877)). Cosmic tracks
items #40731 and #40733 ([#40731 step](../../../areas/story/cosmic.story.json#L646),
[#40733 step](../../../areas/story/cosmic.story.json#L683)).
All 231 other sidecars parsed without errors. No outside SHP product or
external QST giver header targets a local prototype. No external mobile or
object definition duplicates a local prototype VNUM, and no candidate-only
typed consumer was found.

## Compiled bindings and item travel

The source `chaos_eq_data.h` directly lists four local object prototypes in
equipment kits: mist viper venom sac #40730, griffon feather cloak #40747,
ring of celestial wonders #40768, and Dusk and Dawn #40781
([examples](../../../src/account/chaos_eq_data.h#L83),
[#40730](../../../src/account/chaos_eq_data.h#L110),
[#40768](../../../src/account/chaos_eq_data.h#L149),
[#40747](../../../src/account/chaos_eq_data.h#L185)). The compiled
[encrust table](../../../src/classes/epic_skills.c#L198) binds Snent #40760;
the [smith table](../../../src/economy/tradeskill.c#L207) binds Emition #40712.
No other exact C/C++ numeric reference to local room, mobile, or object VNUMs
was found.

Seven local type-25 ITEM_TELEPORT objects declare these destinations: #40720
→ #40773, #40721 → #40758, #40723 → #40781, #40724 → #40756, #40753 → #22053,
#40754 → #40780, and #40755 → #40700. Six targets are local rooms; #22053 is
the outbound Cerebusp room. Divine Home object #40753 is reset into room
#40787 ([reset](../../../areas/zon/divhome.zon#L117)). Cerebusp's active fiery
portal #22010 is reset into room #22053 and declares #40787 as its destination
([object](../../../areas/obj/cerebusp.obj#L142),
[reset](../../../areas/zon/cerebusp.zon#L179)). The pair is reciprocal at the
source level and does not add a WLD exit. No local type-29 ITEM_SWITCH or
external ITEM_SWITCH destination into Divine Home was found.

Keep active Home of the Divine **DO NOT MOVE**. Preserve its current imported
ingredients, named external reset consumers, item portal pair, and compiled
bindings. The existing twenty builder questions are unchanged. Static reset,
shop, quest, and portal declarations do not prove stock, admission, access, or
runtime behavior. No candidate dependency, WLD total, or disposition changed;
no native gameplay or area source was edited.
