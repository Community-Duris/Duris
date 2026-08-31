#!/usr/bin/env python3
"""Source contract for preserving complete status and epic log messages."""

from _paths import SRC
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "utility.c").read_text(encoding="utf-8")


def function_body(name: str) -> str:
	match = re.search(rf"void {name}\([^{{]+\)\n\{{(.*?)(?=\nvoid \w+\()", SOURCE, re.S)
	assert match, f"could not find {name} body"
	return match.group(1)


for function_name in ("statuslog", "epiclog"):
	body = function_body(function_name)
	assert "lbuf[strlen(lbuf) - 2]" not in body
	assert "lbuf[len - 1] == '\\n' || lbuf[len - 1] == '\\r'" in body
	assert "lbuf[len - 1] = '\\0'" in body

print("log message source contract passed")
