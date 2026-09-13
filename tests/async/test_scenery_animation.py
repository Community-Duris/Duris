#!/usr/bin/env python3
"""Production scenery recipes, full palette, and the long-description routing seam."""
import json
import re
import subprocess
from scenery_fixture import ROOT, CONFIG, scenery_harness, alternate_config

expected = {
    "foliage": "forest forests tree trees moss vine vines grass foliage jungle jungles fern ferns undergrowth shrub shrubs bush bushes",
    "water_flow": "water waters river rivers stream streams waterfall waterfalls",
    "wide_wave": "ocean sea lake",
    "fire": "fire fires flame flames lava magma embers",
    "ice": "ice icy frost frozen",
    "snow": "snow snowy",
    "blood": "blood bloody bleeding",
    "remains": "bone bones skull skulls skeleton skeletons",
    "magic": "magic magical arcane rune runes portal portals",
    "toxic": "poison poisonous venom venomous acid acidic",
}
config = json.loads(CONFIG.read_text())
assert config["dictionaries"]["scenery"] == {
    word: recipe for recipe, words in expected.items() for word in words.split()
}
assert config["channels"] == {"room.description": "scenery"}
with scenery_harness() as binary:
    subprocess.run([str(binary), str(CONFIG)], check=True, timeout=120)
    preview = subprocess.check_output([str(binary), str(CONFIG), "--preview"], timeout=120)
    assert preview == (ROOT / "docs/examples/scenery-transcript.txt").read_text().encode()
    alternate = binary.parent / "alternate.json"
    alternate.write_text(json.dumps(alternate_config()))
    alternate_preview = subprocess.check_output([str(binary), str(alternate), "--preview"], timeout=120)
    assert alternate_preview == (ROOT / "docs/examples/scenery-transcript-alternate.txt").read_text().encode()
    assert alternate_preview != preview
    # Count repeated exact matches, protect whole partly styled words, and do not
    # mistake possessives/substrings or existing foregrounds for eligible words.
    counts = subprocess.check_output([str(binary), str(CONFIG), "--audit"],
        input=b"water water forest\0&+rwater forest&n\0wa&+rte&nr forest\0water's underwater\0", timeout=120)
    assert counts == b"3\n0\n1\n0\n"

source = (ROOT / "src/cmd/actinf.c").read_text()
start = source.index("if (!(brief_mode || (keyword_no == 8) || (vis_mode == 3)))")
end = source.index("display_room_auras(ch, room_no);", start)
room_prose = source[start:end]
assert room_prose.count("OutputChannel::RoomDescription") == 1
assert "send_to_char(world[room_no].description,ch,profile.context)" in re.sub(r"\s+", "", room_prose)
assert "send_authored_output(world[room_no].name,ch,recipient_output_context(OutputChannel::RoomTitle))" in re.sub(r"\s+", "", source)
assert source.count("OutputChannel::RoomDescription") == 1
