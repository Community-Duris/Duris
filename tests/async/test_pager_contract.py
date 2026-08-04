from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src" / "modify.c").read_text()
start = source.index("void show_string(")
end = source.index("\n//--------------------------------------------------------------------", start)
show_string = source[start:end]

assert "d->showstr_vector[d->showstr_page + 1]" in show_string
assert "memcpy(buffer" in show_string
assert "buffer[page_length] = '\\0';" in show_string
assert "strlcpy(buffer, d->showstr_vector[d->showstr_page], sizeof buffer)" not in show_string

print("pager bounded-copy contract passed")
