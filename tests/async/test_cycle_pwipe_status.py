from pathlib import Path

text = (Path(__file__).resolve().parents[2] / "cycle_mud.sh").read_text()
start = text.index('if [ $RESULT == 55 ]')
end = text.index('fi\n\nif [ -f /usr/bin/sendemail', start)
block = text[start:end]
assert '[ ! -x "./Players/wipers/wipe_it_all" ]' in block
assert 'if ! ./Players/wipers/wipe_it_all' in block
assert 'echo "Wiped!"' in block
print('filesystem pwipe fail-closed checks passed')
