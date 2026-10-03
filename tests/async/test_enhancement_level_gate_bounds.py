#!/usr/bin/env python3
"""Exercise both production enhancement level gates at configured integer bounds."""
from _paths import ROOT, SRC, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

source = (SRC / 'enhance.c').read_text(encoding='utf-8')
helper = (extract_function('enhance.c', 'static int enhance_maximum_item_value(')
          if 'static int enhance_maximum_item_value(' in source else '')
gates = {}
for name, condition in (
        ('ordinary', 'if (sval > '),
        ('superior', 'if (itemvalue(source) > ')):
    start = source.index(condition)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    gates[name] = source[start:end]

HARNESS = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <string>
int level, value, admitted, messages;
int enhance_level_gate_multiplier=3;
std::string message;
#define GET_LEVEL(ch) level
constexpr size_t MAX_STRING_LENGTH=512;
int itemvalue(void*) { return value; }
void send_to_char(const char* text, void*) { ++messages; message=text; }
'''
FUNCTION = r'''
void gate_@NAME@() {
 void* ch=nullptr;
 [[maybe_unused]] void* source=nullptr;
 [[maybe_unused]] int sval=value;
 [[maybe_unused]] char buf[512], rest[512];
 @GATE@
 ++admitted;
}
'''
MAIN = r'''
int main() {
 for (auto gate : {gate_ordinary, gate_superior}) {
  level=50; enhance_level_gate_multiplier=INT_MAX; value=INT_MAX;
  admitted=messages=0; gate();
  assert(admitted==1 && messages==0);
  enhance_level_gate_multiplier=3;
  value=150; admitted=messages=0; gate();
  assert(admitted==1 && messages==0);
  value=151; admitted=messages=0; gate();
  assert(admitted==0 && messages==1);
  assert(message.find("up to ival 150.")!=std::string::npos);
  level=1; enhance_level_gate_multiplier=INT_MAX;
  value=INT_MAX; admitted=messages=0; gate();
  assert(admitted==1 && messages==0);
  for (int invalid : {0,-1,INT_MIN}) {
   level=invalid; enhance_level_gate_multiplier=INT_MAX; value=1;
   admitted=messages=0; gate(); assert(admitted==0 && messages==1);
   level=50; enhance_level_gate_multiplier=invalid;
   admitted=messages=0; gate(); assert(admitted==0 && messages==1);
  }
 }
}
'''
with tempfile.TemporaryDirectory(prefix='duris-enhance-level-bounds-') as temporary:
    cpp = Path(temporary) / 'gate.cpp'
    binary = Path(temporary) / 'gate'
    cpp.write_text(HARNESS + helper + '\n'.join(
        FUNCTION.replace('@NAME@', name).replace('@GATE@', gate)
        for name, gate in gates.items()) + MAIN)
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra',
                    '-Werror', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-pie', '-no-pie',
                    str(cpp), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print('Ordinary/superior enhancement: wide configured level gates, ordinary boundaries and invalid-limit refusal passed')
