#!/usr/bin/env python3
"""Source contracts for the local/live help content importer."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "scripts" / "import_help_to_prod.sh").read_text()
live_blocks = re.findall(
    r"python3 <<'PYTHON_SCRIPT'\n(.*?)\nPYTHON_SCRIPT", source, re.DOTALL
)
count_blocks = re.findall(
    r"python3 << PYTHON_SCRIPT[^\n]*\n(.*?)\nPYTHON_SCRIPT", source, re.DOTALL
)

# Three quoted live blocks: the SECTION 1.5 collision report, the SECTION 2
# import pass and the SECTION 3 import pass. The two UNQUOTED blocks are the
# SECTION 2 and SECTION 3 counting passes, which write no database rows.
assert len(live_blocks) == 3
assert len(count_blocks) == 2

# The help_index parsing must agree with the flat build's parse_help_index
# (src/flatfile/flatfile_help_catalog.c): a line whose only content is '#'
# ends an entry, whatever whitespace surrounds it. The collision report's
# index_titles() and both SECTION 2 passes share that rule through the
# help_index_entries() helper; the exact-string split it replaced merged the
# entries across a '# ' delimiter line and lost the second entry's title.
assert source.count("entries = help_index_entries(content)") == 2
assert "def help_index_entries(content)" in live_blocks[0]
assert source.count("content.split('\\n#0\\n')") == 2
assert source.count("(src/flatfile/flatfile_help_catalog.c, parse_help_index)") == 3

# The SECTION 1.5 report only reads files and prints; it must not touch the
# database and must not change what is imported.
report_block, import_block_2, import_block_3 = live_blocks
assert "error_count" not in report_block
assert "mysql" not in report_block
assert "def index_titles" in report_block
assert "def parsed_titles" in report_block
assert "IMPORT_RESERVED_TITLES" in report_block

# No block that parses help_index may go back to the exact-string split the
# helper replaced: a delimiter line written '# ' or '#  ' merged the entries
# across it and the second entry's title was lost. Both quote spellings the
# legacy code used are pinned; splitting on the parsed file's '#0' marker or
# on a plain newline stays legal.
for block in (report_block, import_block_2, import_block_3, *count_blocks):
    assert "content.split('\\n#\\n')" not in block
    assert 'content.split("\\n#\\n")' not in block
for block in (import_block_2, import_block_3):
    assert "join(content_lines)" in block
    assert "if error_count:\n    raise SystemExit(1)" in block

assert "selected database" in source
assert 'MYSQL_SOCKET="${DB_SOCKET:-}"' in source
assert source.count('--protocol=socket --socket="$MYSQL_SOCKET"') == 2
assert source.count('f"--socket={MYSQL_SOCKET}"') == 2
assert "~535" not in source
for missing_source in ("helpguild1", "helpguild2", '["info"]'):
    assert missing_source not in source
print("help import live parser contracts passed")
