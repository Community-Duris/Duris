#!/usr/bin/env python3
"""Direct contracts for code-whitespace-tolerant source matching."""

from contract_text import code_text, contains, count, end, find, index
from _source_contract import block_start, function_bodies, strip_comments, top_level_definitions
from _paths import extract_function
from pathlib import Path
import tempfile
from unittest.mock import patch

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

# Removing the guard while leaving its spelling in a comment/log must fail.
guard = "if (!ready) return;"
for decoy in ('// ' + guard, '/* ' + guard + ' */', 'log("' + guard + '");',
              'log(R"tag(' + guard + ')tag");'):
    assert not contains(decoy, guard), decoy
    assert count(decoy, guard) == 0, decoy
    assert find(decoy, guard) == -1, decoy
    assert end(decoy, guard) == -1, decoy
    assert find(decoy, guard, decoy.index('if')) == -1, decoy
    try:
        index(decoy, guard)
    except ValueError:
        pass
    else:
        raise AssertionError("a non-code guard was accepted")
    assert contains(decoy + '\n' + guard, guard)
    assert count(decoy + '\n' + guard, guard) == 1
assert contains('log("status=ready");', 'status=ready', literal=True)
assert find('before; target(); after;', 'target();', -17) == 8
assert find('before; target(); after;', 'target();', 0, 12) == -1
assert find('abc', '', 4) == -1
assert find('abc', '', -1) == 2
assert not contains('returnvalue;', 'return value;')
assert contains('return\n\tvalue;', 'return value;')

# Comment markers and braces inside literals are data. Offsets survive masking.
tricky = r'''void target();
void before() { emit("https://example/{"); }
void target() {
    emit("escaped \\\" } /* not a comment */");
    emit(R"tag( } // still a literal { )tag");
    char brace = '}';
    unsigned value = 0xFF'AB + 1'000'000;
    // } protect();
    /* { protect(); } */
    protect();
}
void after() { other(); }
'''
mask = code_text(tricky)
assert len(mask) == len(tricky)
assert mask.count('\n') == tricky.count('\n')
assert "0xFF'AB + 1'000'000" in mask
assert 'https://example' in strip_comments(tricky)
bodies = function_bodies(tricky, r'void\s+target\s*\(')
assert len(bodies) == 1 and contains(bodies[0], 'protect();')
assert not contains(bodies[0], 'other();')
assert [name for name, _, _ in top_level_definitions(tricky)] == ['before', 'target', 'after']
guard_pos = tricky.index('    protect();')
assert block_start(strip_comments(tricky), guard_pos) == tricky.index('{', tricky.index('void target() {')) + 1
continued_comment = '// disabled \\\nprotect();\nreal();'
assert not contains(code_text(continued_comment), 'protect();')
assert contains(code_text(continued_comment), 'real();')
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'probe.c'
    path.write_text(tricky)
    with patch('_paths.source', return_value=path):
        extracted = extract_function('probe.c', 'void target(')
    assert extracted.startswith('void target()') and extracted.endswith('\n}')
    assert contains(extracted, 'protect();') and not contains(extracted, 'other();')

print("contract text matching contracts OK")
