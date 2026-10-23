# The Reliquary Nexus of the Corsairs: comprehensive zone-story mapping

This dossier maps the story expressed by active source stem nexus_island. It records declared source behavior and does not certify live gameplay. The [repository-wide Nexus Island audit supplement](../quest-story-followups/NEXUS_ISLAND_AUDIT_REVIEW.md) covers the directed map, fall hazards, inactive-map references, typed dependencies, title/history, and builder questions.

## Source inventory and present delivery

Active nexus_island owns 100 WLD rooms, #130300–#130399, in zone 1303; 20 mobile prototypes, #130300–#130319; one object prototype, dagger #130302; and one ZON reset source. There is no local QST, SHP, or story-sidecar file. The shared [QST area index](../../../areas/qst/AREA#L329) still lists nexus_island. No native contract, shop, or sidecar-defined outcome is present in the local files.

The normalized ZON title is “The Reliquary Nexus of the Corsairs.” Its reset source uses mode 0 and declares 54 mobile placements, two follower commands, four door resets, and six dagger equipment placements; it has no O object-placement command. These declarations do not prove a reset schedule, current inventory, follower behavior, or live door state.

## Island arrival, castaways, and corsairs

One reciprocal Surface boundary brings travelers to pristine beach room #130301: Nexus Island south to Surface #519153, with Surface north returning to #130301 ([outbound record](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1295), [return record](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1355)). The surrounding beach and jungle rooms describe a large pond, shaded paths, treehouses and rowhouses glimpsed through vegetation, and routes repeatedly blocked by rock walls or thick growth.

The central dispute is explicit but unresolved. Castaway text says the corsairs believe the castaways stole treasure meant for a king on the next tide, and that they raid the castaway camp for it. The castaways describe a settled community defending its home. Their Chief is marked by battle tattoos and protects the tribe. Bella the Bloodthirsty stalks for non-corsairs, carries a sword, and has special hatred for the castaways. The source describes raiding, combat, prisoner restraints, and competing claims; it does not establish which side is right or a player action that settles the dispute.

The hunting cat lives with and fights beside the castaways. The reset source gives it a follower command toward the Chief. A trained dog is reset with a follower command toward Bella. Those source relationships do not prove a player can recruit or command either animal. Toothy fish inhabit the island’s pools and are described as able to strip flesh from large creatures. Giant birds protect nests and attack intruders. Their descriptions support a living island with danger and allies, not an accepted rescue, supply, or conservation outcome.

## Crater ascent, tunnel, and the Nexus

From the jungle, the map rises along thin, crumbling hillside paths. Their descriptions warn that travelers may tumble into jungle below and that nesting birds attack when disturbed. Thirty-nine WLD rooms carry fall-chance markers, ranging from 30 to 100; three rooms have 100 markers. The WLD file does not declare a fall destination or the resulting damage, rescue, or return.

The crater rim overlooks the corsair and castaway settlements and describes magical walls protecting both from attack. A large pond in the jungle resembles a blue sapphire from above. Inside the crater, dry dust may choke travelers; dried blood and remains mark past battles. A paired wooden slat door separates room #130383 from the tunnel at #130384. Its extra description says magic resists attempts to open it until the correct method is used, while the reset declares the reciprocal doors closed.

Tunnel residents include beetlemen and giant lemurs. The lemur descriptions say creatures normally timid have been enlarged and twisted by forces active in the crater. Deeper in, an echoing open room (#130398) has a floor carving and dried blood. A reciprocal closed/secret trapdoor connects it to room #130399, titled The Nexus. That room says the stone at its center emits intense, nearly visible power. The source suggests a relationship between the stone, crater forces, and changed creatures, but contains no local script, quest, or saved outcome proving that mechanism.

Two additional down exits from hillside rooms #130342 and #130348 target -1 and are rejected by the loader. The prose describes crumbling paths above jungle, but the intended destinations or whether those exits were placeholders remain builder questions. There are no WLD current markers.

## Object and reset behavior

The dagger #130302 is the only local object. Six ZON E commands equip it in both hand slots on three giant lemurs at rooms #130360, #130361, and #130363. The item has no local O reset and no QST hand-in or reward binding. Confirm whether the dual daggers are part of the intended crater threat or merely equipment before assigning them story significance.

Four D resets close two reciprocal passages: the crater/tunnel door (#130383/#130384) and the closed/secret trapdoor into The Nexus (#130398/#130399). The reset file does not declare how players learn the “correct method” for the wooden barrier. The area header uses reset mode 0; actual opening, resetting, and return timing remain unqualified.

## Story boundaries and future design

The current source supports a linked environmental story: two island communities clash over a king-bound treasure; a jungle shelters castaways and animals; a hazardous climb reaches a blood-marked crater; and a central stone is associated with unnatural power and altered wildlife. It does not deliver a native quest or resolve the conflict. No contract, treasure identity, accepted allegiance, stone interaction, survivor result, or durable island-state change can be inferred from the descriptions alone.

The title “The Reliquary Nexus of the Corsairs” is unique among all 442 current .zon files. Followed WLD/MOB/OBJ/ZON history stays on this stem; no alternate-stem predecessor or explicit replacement was identified. Keep active nexus_island **DO NOT MOVE**. The inactive Duris3 references documented in the [audit supplement](../quest-story-followups/NEXUS_ISLAND_AUDIT_REVIEW.md#inactive-wld-sweep-and-duris3-source-only-approaches) are not active approaches or replacement evidence.
