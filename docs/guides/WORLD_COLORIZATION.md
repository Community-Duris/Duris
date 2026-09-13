# World output colors

`toggle color` exposes title, inspect, exits, auras, occupants, roomitems,
inventory, social and weather alongside the separate long-prose `room` setting.
All retain original presentation with no configured profile or personal choice.
A personal color is a static foreground for eligible text. `default` and reset
restore inherited presentation without removing authored ANSI.

| Choice | Delivery boundary | Protected information |
| --- | --- | --- |
| title / room.title | Visible room name in look | Existing name colors; map output is separate |
| inspect / room.inspect | Character, exit and room/object extra descriptions | Authored styles, layout and existing visibility gates |
| exits / room.exits | Obvious-exits heading and unstyled list text | Closed/locked/secret/walled markers and destination names retain their existing accents |
| auras / room.auras | Room-aura messages after detection checks | Soothing, evil, magic, narrow, silence and light accents come from their existing branches |
| occupants / room.occupants | The room's character-list helper | Sense-life, blindness, invisibility and altitude checks remain before display |
| roomitems / room.items | Room contents in look | Item visibility, grouping, authored names and known item flags |
| inventory / items.list | Inventory heading and carried-object list | Independent from room items; magic detection and other flags retain their existing gates |
| social | Successful and unsuccessful social templates after command dispatch | Existing recipients, visibility, actor/entity roles and authored styles |
| weather | Weather-sector broadcasts | Existing sector, plane, outdoor, precipitation, darkness, altitude, blindness and awake checks |

The shared object and character helpers take an explicit OutputContext. Their
original overloads retain Preserve, so unrelated callers do not acquire a profile.
A caller-supplied Preserve is an absolute veto. Notes keep their old authored
presentation. Maps and tracks retain their existing output paths. Legacy object
inspection/equipment callers that do not pass a context remain unchanged.

Dense fields (title, exits, auras, occupants, roomitems, inventory) force Static
and suppress configured word dictionaries and animated recipes. This prevents a
word such as `chest` from suggesting hidden interaction or rarity. Existing visible
state markers remain authoritative and retain their authored colors. Inspection
prose can use a configured dictionary; weather can use optional configured effects,
and global motion-off demotes those effects to their stable frame.

Authored names and partial-word gradients retain every existing attribute. World
prose and list sends also apply a conservative layout veto: tabs, three aligned
spaces, three consecutive drawing characters, or Unicode box/block drawing retain
the entire original message. This can intentionally preserve an indented prose
passage too. It is only a presentation rule and never changes visibility or game
state. Raw terminal controls and oversized decoration keep the renderer's original
fallback. Extra descriptions still enter the same automatic command pager, which
stores frozen display bytes.

Try `toggle color title bright cyan`, `toggle color inventory yellow`, and
`toggle color roomitems bright green`. Preview each with `toggle color preview
<choice>` and restore with `toggle color reset <choice>`. Preview samples retain
semantic authored accents rather than promising to repaint them. Fully authored
weather/social passages may show no visible difference under a base-color choice.

Validation lives in `tests/async/test_word_output_integration.py`: real output,
queue and pager functions plus real exit, item-list, item-detail, aura and weather
functions run with controlled world/visibility boundaries. Fixtures cover all new
channels, override/reset, caller Preserve, protected gradients/layout, closed and
hidden exits, unseen items, magic detection, room/inventory independence, aura
detection, indoor weather and blindness. `test_color_command.py` exercises every
exposed color and actual rendered previews. The final live-client walkthrough and
release evidence are tracked by #289.
