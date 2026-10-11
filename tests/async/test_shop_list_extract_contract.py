#!/usr/bin/env python3
from _paths import SRC


source = (SRC / "economy" / "shop.c").read_text()

listing = source[source.index("void shopping_list(") :]
listing = listing[: listing.index("\n}\n")]
loop = listing[listing.index("for (obj1 = keeper->carrying;") :]

assert loop.startswith("for (obj1 = keeper->carrying; obj1; obj1 = next_obj)")
assert loop.index("next_obj = obj1->next_content;") < loop.index("extract_obj(obj1, TRUE);")
assert "obj1 = obj1->next_content" not in listing

print("[PASS] the shop listing reads the next item before it extracts invalid stock")
