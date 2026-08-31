from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "mobpatrol.c").read_text()
assert "PatrolData *huntData = (PatrolData *)data;" in source
assert "huntData ? sizeof(PatrolData) : 0" in source
assert "sizeof(PatrolData));" in source
assert "sizeof(struct hunt_data)" not in source
print("patrol event payload-size checks passed")
