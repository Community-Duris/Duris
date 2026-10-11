#!/usr/bin/env python3
"""Exercise full signed RNG intervals and unmatched random-equipment spell ids."""
from pathlib import Path
import subprocess
import tempfile
from _paths import extract_function

HARNESS = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define STRUNG_DESC2 1
#define FREE(p) { free(p); (p)=nullptr; }
#include <cstdarg>
int checked_snprintf(char *out, size_t size, const char *fmt, ...) {
 va_list args; va_start(args,fmt); int n=vsnprintf(out,size,fmt,args); va_end(args); return n;
}
struct obj_data { int value[6]{}; char *short_description; int str_mask{}; };
using P_obj = obj_data *;
struct { int spell; const char *name; } spells_data[61] = {{10, "test"}};
char *str_dup(const char *s) { return strdup(s); }
static uint64_t draw;
uint64_t rnd64() { return draw; }
''' + extract_function("core/random.c", "int number(int from, int to)") + "\n" + extract_function(
    "item/randomeq.c", "bool identify_random(P_obj obj)") + r'''
int main() {
 for (uint64_t sample : {uint64_t(0), uint64_t(123), UINT64_MAX}) {
  draw=sample;
  const int ascending=number(INT_MIN,INT_MAX);
  const int descending=number(INT_MAX,INT_MIN);
  assert(int64_t(ascending)==int64_t(INT_MIN)+int64_t(sample%UINT64_C(4294967296)));
  assert(int64_t(descending)==int64_t(INT_MAX)-int64_t(sample%UINT64_C(4294967296)));
  assert(number(7,7)==7);
 }
 obj_data obj{};
 obj.short_description=const_cast<char *>("ring"); obj.value[5]=INT_MAX;
 assert(!identify_random(&obj));
 spells_data[60]={20,"last"}; obj.value[5]=20;
 assert(identify_random(&obj)); assert(!strcmp(obj.short_description,"ring of last&n"));
 free(obj.short_description);
}
'''
HARNESS = "#include <initializer_list>\n" + HARNESS
with tempfile.TemporaryDirectory(prefix="random-boundaries-") as build:
    cpp = Path(build) / "harness.cpp"
    binary = Path(build) / "harness"
    cpp.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                    "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("signed RNG interval endpoints and random equipment lookup boundaries passed")
