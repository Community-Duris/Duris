from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
actinf = (ROOT / "src" / "actinf.c").read_text(encoding="utf-8", errors="replace")
epic = (ROOT / "src" / "epic.c").read_text(encoding="utf-8", errors="replace")
new_events = (ROOT / "src" / "new_events.c").read_text(encoding="utf-8", errors="replace")
properties = (ROOT / "lib" / "duris.properties").read_text(encoding="utf-8", errors="replace")

help_body = re.search(r"void do_help\(.*?\n}\n\nvoid do_wizhelp", actinf, re.S)
assert help_body, "do_help body not found"
assert 'help.cooldown.secs' in help_body.group(0)
assert 'CharWait(' not in help_body.group(0), "help must not impose command lag"
assert 'help.lag.pulses' not in properties, "obsolete help lag property remains active"

assert 'if (type == EPIC_BOTTLE)' in epic
assert 'log_epic_gain_event("epic_bottle"' in epic
assert 'func == event_wait' in new_events
assert 'return ch != NULL && IS_PC(ch);' in new_events

print("reported latency contract passed")
