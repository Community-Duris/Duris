#!/usr/bin/env python3
"""Production superior caps must fit the persisted signed-byte modifier."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

HARNESS = r'''
#include <cassert>
#include <climits>
#include <cmath>
#include <initializer_list>
double enhance_stat_cap_multiplier=1.5;
@FUNCTION@
int main() {
 assert(enhance_stat_cap(10)==15 && enhance_stat_cap(1)==1);
 assert(enhance_stat_cap(0)==0 && enhance_stat_cap(-10)==0);
 assert(enhance_stat_cap(100)==SCHAR_MAX);
 assert(enhance_stat_cap(INT_MAX)==SCHAR_MAX);
 enhance_stat_cap_multiplier=1e300;
 assert(enhance_stat_cap(INT_MAX)==SCHAR_MAX);
 for(double multiplier : {static_cast<double>(INFINITY), static_cast<double>(NAN), -1.0, 0.0}) {
  enhance_stat_cap_multiplier=multiplier;
  assert(enhance_stat_cap(10)==0);
 }
 enhance_stat_cap_multiplier=0.5;
 assert(enhance_stat_cap(3)==1);
}
'''
with tempfile.TemporaryDirectory(prefix='duris-superior-cap-') as temporary:
    cpp=Path(temporary)/'cap.cpp'; binary=Path(temporary)/'cap'
    cpp.write_text(HARNESS.replace('@FUNCTION@',extract_function('enhance.c','static int enhance_stat_cap(')))
    subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                    '-O1','-g','-fsanitize=address,undefined,float-cast-overflow',
                    '-fno-sanitize-recover=all','-fno-pie','-no-pie',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
print('superior caps: signed-byte limits, invalid multiplier refusal, ordinary tiers passed')
