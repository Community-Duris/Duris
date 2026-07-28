from pathlib import Path

text = (Path(__file__).resolve().parents[2] / "cycle_mud.sh").read_text()
root = Path(__file__).resolve().parents[2]
start = text.index('if [ $RESULT == 55 ]')
end = text.index('fi\n\nif [ -f /usr/bin/sendemail', start)
block = text[start:end]
assert '[ ! -x "./Players/wipers/wipe_it_all" ]' in block
assert 'if ! ./Players/wipers/wipe_it_all' in block
assert 'echo "Wiped!"' in block

wipe = root / "Players" / "wipers" / "wipe_it_all"
assert wipe.is_file(), "Players/wipers/wipe_it_all must exist in tree"
assert wipe.stat().st_mode & 0o111, "wipe_it_all must be executable"
body = wipe.read_text()
assert "set -euo pipefail" in body
assert "Players/" in body
assert "Ships" in body
assert "FLUSHALL" not in body
assert "rm -rf -- /" not in body and "rm -rf /" not in body
print('filesystem pwipe fail-closed checks passed')
