"""Shared C/C++ source parsers for the contract tests.

A source-contract test asks "does the CODE do X", so its parsers decide what
the pins are worth.  These lived in two copies, and the copies drifted: the
prototype guard in `function_bodies()` below was added to one of them after a
pin was found examining an unrelated function's body, while the other kept
taking the first brace it saw.  One definition, imported by both, is what
stops that happening again.

Usage:

    from _source_contract import strip_comments, function_bodies

    body = function_bodies(text, r"\\bvoid\\s+do_thing\\s*\\(")[0]
"""

from __future__ import annotations

import re


def strip_comments(text: str) -> str:
    """Blank out /* */ and // comments, keeping every newline so that line
    numbers and statement anchors survive.

    Every parser here strips first, so a pin on code can never be satisfied by
    prose describing it -- a comment mentioning a call is not a call.
    """

    def blank(match: re.Match) -> str:
        """The matched comment with every character but newline replaced by a
        space, so offsets and line numbers survive the strip."""
        return re.sub(r"[^\n]", " ", match.group(0))

    text = re.sub(r"/\*.*?\*/", blank, text, flags=re.S)
    return re.sub(r"//[^\n]*", blank, text)


def function_bodies(text: str, signature: str) -> list:
    """Every brace-matched body of a function DEFINITION whose head matches the
    `signature` regex (which must end at, or before, the opening brace).
    Returns [] when there is no definition.

    A PROTOTYPE is not a definition.  An earlier draft took the next '{'
    anywhere after the match, so `bool move_guildhall(Guildhall *, int);` at
    the top of guildhall_cmds.c handed back the body of the next unrelated
    function -- and a pin asking "does move_guildhall notify the realm" could
    then be satisfied by a stranger, or pass while the real definition stopped
    notifying.  Only a parameter list and trailing qualifiers may stand between
    the head and the body, so a ';' or a '}' in that gap means the match was a
    declaration and this brace belongs to something else.
    """
    code = strip_comments(text)
    bodies = []
    for match in re.finditer(signature, code):
        start = code.find("{", match.end())
        if start < 0:
            continue
        gap = code[match.end() : start]
        if ";" in gap or "}" in gap:
            continue
        depth = 0
        for index in range(start, len(code)):
            if code[index] == "{":
                depth += 1
            elif code[index] == "}":
                depth -= 1
                if depth == 0:
                    bodies.append(code[start : index + 1])
                    break
    return bodies


def function_body(text: str, signature: str):
    """The body of the FIRST definition matching `signature`, or None.

    A thin front for function_bodies(), so a caller that wants one body gets
    the same prototype-skipping rule rather than a second parser.
    """
    bodies = function_bodies(text, signature)
    return bodies[0] if bodies else None


def block_start(code: str, pos: int) -> int:
    """Index just past the '{' that opens the innermost block containing
    `pos`, or 0 when `pos` is not inside one.  `code` must already be
    comment-stripped."""
    depth = 0
    for index in range(pos - 1, -1, -1):
        if code[index] == "}":
            depth += 1
        elif code[index] == "{":
            if depth == 0:
                return index + 1
            depth -= 1
    return 0


def top_level_definitions(text: str) -> list:
    """(name, start, end) for every top-level brace block in a translation
    unit, comments stripped, where `name` is the first identifier called in
    the block's head (a function's own name) or None for a block that is not
    a function -- a struct or an array initialiser.  Used to answer "which
    function is this call site in?" without hard-coding line numbers."""
    code = strip_comments(text)
    defs = []
    depth = 0
    head_start = 0
    open_at = -1
    for index, char in enumerate(code):
        if depth == 0 and char in ";}":
            head_start = index + 1
        if char == "{":
            if depth == 0:
                open_at = index
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0 and open_at >= 0:
                head = code[head_start:open_at]
                names = re.findall(r"(\w+)\s*\(", head)
                defs.append((names[0] if names else None, open_at, index + 1))
                head_start = index + 1
                open_at = -1
    return defs


def enclosing_definition(defs: list, pos: int):
    """The (name, start, end) triple from top_level_definitions() containing
    `pos`, or None when the position sits outside every top-level block."""
    for name, start, end in defs:
        if start <= pos < end:
            return (name, start, end)
    return None
