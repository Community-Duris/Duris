#!/usr/bin/env python3
"""Source contracts for the local/live help content importer."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "scripts" / "import_help_to_prod.sh").read_text()
live_blocks = re.findall(
    r"python3 <<'PYTHON_SCRIPT'\n(.*?)\nPYTHON_SCRIPT", source, re.DOTALL
)

assert len(live_blocks) == 2
assert source.count(r"entries = content.split('\n#\n')") == 2
assert source.count(r"entries = content.split('\n#0\n')") == 2
for block in live_blocks:
    assert r"split('\\n" not in block
    assert r"join(content_lines)" in block
    assert "if error_count:\n    raise SystemExit(1)" in block

assert "selected database" in source
assert 'MYSQL_SOCKET="${DB_SOCKET:-}"' in source
assert source.count('--protocol=socket --socket="$MYSQL_SOCKET"') == 2
assert source.count('f"--socket={MYSQL_SOCKET}"') == 2
assert "~535" not in source
for missing_source in ("helpguild1", "helpguild2", '["info"]'):
    assert missing_source not in source
print("help import live parser contracts passed")
