"""Code-whitespace-tolerant matching for source-contract tests.

The C/C++ tree is clang-formatted, so spacing inside a statement belongs to
the formatter, not to the contract a test is pinning.  A test that asserts
`"obj->type==ITEM_CONTAINER" in source` is really asserting "this comparison
is here", and it should keep passing when the formatter writes it as
`obj->type == ITEM_CONTAINER` or wraps it across two lines.

Every helper here ignores whitespace outside C/C++ strings and comments.
Whitespace inside a string or comment remains contractual. `find`/`index`/`end`
report offsets into the ORIGINAL text, so slicing a region out of a file still
works:

    start = index(src, "static void do_thing(")
    body  = src[start:index(src, "\\n}", start)]

Use these for code-shaped patterns.  Keep plain `in` checks where spacing is
the point (the indentation of a generated file, a string the game sends to a
player).
"""

import re
from functools import lru_cache

__all__ = ["squeeze", "contains", "count", "missing", "find", "index", "end", "before",
           "section", "split_at"]


def squeeze(text):
	"""Drop every run of whitespace, so wrapping and alignment stop mattering."""
	return re.sub(r"\s+", "", text)


_NORMAL = "\x01"
_QUOTED = "\x02"
_COMMENT = "\x03"
_RAW_PREFIXES = ('u8R"', 'uR"', 'UR"', 'LR"', 'R"')


def _significant_chars(text):
	"""Yield (lexical-state marker, character, original offset).

	Normal code whitespace is omitted. Quoted and commented text is emitted in
	full with a distinct marker, so a bare text needle cannot accidentally make
	a whitespace-insensitive match inside a player-visible string.
	"""
	i = 0
	while i < len(text):
		if text[i].isspace():
			i += 1
			continue

		if text.startswith("//", i):
			end_pos = text.find("\n", i + 2)
			if end_pos < 0:
				end_pos = len(text)
			for pos in range(i, end_pos):
				yield _COMMENT, text[pos], pos
			i = end_pos
			continue

		if text.startswith("/*", i):
			close = text.find("*/", i + 2)
			end_pos = len(text) if close < 0 else close + 2
			for pos in range(i, end_pos):
				yield _COMMENT, text[pos], pos
			i = end_pos
			continue

		raw_prefix = next((prefix for prefix in _RAW_PREFIXES if text.startswith(prefix, i)), None)
		if raw_prefix:
			quote = i + len(raw_prefix) - 1
			opening = text.find("(", quote + 1)
			if opening >= 0:
				delimiter = text[quote + 1:opening]
				closing = ")" + delimiter + '"'
				close = text.find(closing, opening + 1)
				if close >= 0:
					end_pos = close + len(closing)
					for pos in range(i, end_pos):
						yield _QUOTED, text[pos], pos
					i = end_pos
					continue

		if text[i] in "\"'":
			quote = text[i]
			end_pos = i + 1
			while end_pos < len(text):
				if text[end_pos] == "\\":
					end_pos += 2
				elif text[end_pos] == quote:
					end_pos += 1
					break
				else:
					end_pos += 1
			for pos in range(i, min(end_pos, len(text))):
				yield _QUOTED, text[pos], pos
			i = end_pos
			continue

		yield _NORMAL, text[i], i
		i += 1


@lru_cache(maxsize=64)
def _canonical(text):
	"""Encode text without normal code whitespace and with lexical-state markers."""
	return "".join(marker + char for marker, char, _ in _significant_chars(text))


def _canonical_count(haystack, needle):
	"""Count aligned, non-overlapping canonical matches."""
	canonical_needle = _canonical(needle)
	if not canonical_needle:
		return 0
	canonical_haystack = _canonical(haystack)
	count_matches = 0
	start = 0
	while True:
		pos = canonical_haystack.find(canonical_needle, start)
		if pos < 0:
			return count_matches
		if pos % 2 == 0:
			count_matches += 1
			start = pos + len(canonical_needle)
		else:
			start = pos + 1


def _original_offset(text, token_index):
	"""Map a canonical logical-character index back into `text`."""
	for current, (_, _, original) in enumerate(_significant_chars(text)):
		if current == token_index:
			return original
	return len(text)


def _match(haystack, needle, start=0, stop=None):
	"""Return an original-text (start, end) pair, or (-1, -1)."""
	stop = len(haystack) if stop is None else stop
	segment = haystack[start:stop]
	canonical_needle = _canonical(needle)
	canonical_pos = -1
	if canonical_needle:
		canonical_segment = _canonical(segment)
		search_at = 0
		while True:
			candidate = canonical_segment.find(canonical_needle, search_at)
			if candidate < 0 or candidate % 2 == 0:
				canonical_pos = candidate
				break
			search_at = candidate + 1

	exact_pos = segment.find(needle)
	if canonical_pos < 0:
		if exact_pos < 0:
			return -1, -1
		return start + exact_pos, start + exact_pos + len(needle)

	canonical_start = _original_offset(segment, canonical_pos // 2)
	logical_end = (canonical_pos + len(canonical_needle)) // 2 - 1
	canonical_end = _original_offset(segment, logical_end) + 1
	if exact_pos >= 0 and exact_pos < canonical_start:
		return start + exact_pos, start + exact_pos + len(needle)
	return start + canonical_start, start + canonical_end


def contains(haystack, needle):
	"""True when `needle` appears, ignoring only normal code whitespace.

	Non-string containers (a list of parsed entries, a set of names) fall back
	to plain membership, so a test can use one helper for both.
	"""
	if not isinstance(haystack, str) or not isinstance(needle, str):
		return needle in haystack
	if not _canonical(needle):
		return True
	return _match(haystack, needle)[0] >= 0


def count(haystack, needle):
	"""Occurrences of `needle`, ignoring only normal code whitespace."""
	return max(haystack.count(needle), _canonical_count(haystack, needle))


def missing(haystack, needles):
	"""The subset of `needles` that `haystack` does not contain."""
	return [n for n in needles if not contains(haystack, n)]


def find(haystack, needle, start=0, stop=None):
	"""Offset of `needle` ignoring normal code whitespace, or -1.

	The offset is into `haystack` itself, so it can be used to slice.
	`start`/`stop` bound the search the way str.find does.
	"""
	return _match(haystack, needle, start, stop)[0]


def index(haystack, needle, start=0, stop=None):
	"""Like `find`, but raises ValueError when the pattern is absent."""
	pos = find(haystack, needle, start, stop)
	if pos < 0:
		raise ValueError(f"pattern not found: {needle!r}")
	return pos


def end(haystack, needle, start=0, stop=None):
	"""Offset just past `needle`, or -1 when it is absent."""
	return _match(haystack, needle, start, stop)[1]


def section(haystack, opening, closing, start=0):
	"""The text between `opening` and the next `closing`, both ignored-whitespace.

	Returns the slice starting at `opening` and ending at `closing`, which is
	how these tests carve a function or a table out of a source file.
	"""
	a = index(haystack, opening, start)
	b = index(haystack, closing, end(haystack, opening, start))
	return haystack[a:b]


def split_at(haystack, marker, maxsplit=1):
	"""Whitespace-tolerant str.split() on a single marker.

	Only the pieces matter to these tests, so the marker itself is dropped.
	"""
	pieces, pos = [], 0
	while maxsplit < 0 or len(pieces) <= maxsplit - 1:
		a = find(haystack, marker, pos)
		if a < 0:
			break
		pieces.append(haystack[pos:a])
		pos = end(haystack, marker, pos)
	pieces.append(haystack[pos:])
	return pieces


def before(haystack, first, second):
	"""True when both patterns are present and `first` comes before `second`."""
	a, b = find(haystack, first), find(haystack, second)
	return a >= 0 and b >= 0 and a < b
