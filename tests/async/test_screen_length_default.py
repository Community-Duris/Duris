from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
structs = (ROOT / "src" / "structs.h").read_text()
actoth = (ROOT / "src" / "actoth.c").read_text()
nanny = (ROOT / "src" / "nanny.c").read_text()
sql_player = (ROOT / "src" / "sql_player.c").read_text()

assert "constexpr int DEFAULT_SCREEN_LENGTH = 40;" in structs

assert "ch->only.pc->screen_length = DEFAULT_SCREEN_LENGTH;" in nanny
assert "sql_row_int(row, col++, DEFAULT_SCREEN_LENGTH)" in sql_player
assert actoth.count("ch->only.pc->screen_length = DEFAULT_SCREEN_LENGTH;") == 1
assert 'snprintf(Gbuf3, MAX_STRING_LENGTH, "%d", DEFAULT_SCREEN_LENGTH);' in actoth
assert 'snprintf(Gbuf3, MAX_INPUT_LENGTH, "%3d", DEFAULT_SCREEN_LENGTH);' in actoth
assert "Screen length set to default %s lines." in actoth

for runtime_source in (actoth, nanny, sql_player):
    assert "screen_length = 24" not in runtime_source
    assert "default 24 lines" not in runtime_source

print("screen-length default contract passed")
