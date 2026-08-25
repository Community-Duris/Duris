#!/usr/bin/env python3
"""Direct contracts for code-whitespace-tolerant source matching."""

from contract_text import contains, count, end, find, index

source = '''
if (ready)
{
\tsend_to_char("Keep  literal spacing", ch);
\t// Keep  comment spacing
\treturn value + 1;
}
'''

# Formatting whitespace is ignored, including line wrapping.
code = 'if(ready){send_to_char("Keep  literal spacing",ch);'
assert contains(source, code)
assert find(source, "return value+1;") == source.index("return value + 1;")
assert index(source, "send_to_char(") == source.index("send_to_char(")
assert end(source, "return value+1;") == source.index("return value + 1;") + len("return value + 1;")
assert count("x = 1;\nx=1;", "x=1;") == 2

# Whitespace inside strings and comments remains exact. Bare text can make an
# exact match there, but cannot fall back to code-whitespace normalization.
assert contains(source, "Keep  literal spacing")
assert not contains(source, "Keep literal spacing")
assert contains(source, "// Keep  comment spacing")
assert not contains(source, "// Keep comment spacing")
assert count(source + source, "Keep  literal spacing") == 2
assert count(source, "Keep literal spacing") == 0

print("contract text matching contracts OK")
