#!/usr/bin/env python3
"""Exercise the actual salvage command's configured roll arithmetic."""
from _paths import ROOT, extract_function
import ast
import os
from pathlib import Path
import re
import subprocess
import tempfile

fixture=ROOT/'tests/async/test_salvage_prototype_preflight.py'
harness=next(ast.literal_eval(node.value) for node in ast.parse(fixture.read_text()).body
             if isinstance(node,ast.Assign) and any(isinstance(target,ast.Name) and target.id=='harness' for target in node.targets))
harness=harness[:harness.index('int main(){')]
harness=harness.replace('#include <cassert>','#include <cassert>\n#include <climits>\n#include <cmath>\n#include <limits>')
harness=harness.replace('#define GET_C_LUK(ch) 1','#define GET_C_LUK(ch) fixture_luck')
harness=harness.replace('int skill=1000,','double luck_multiplier=1.0,chance_multiplier=1.0;\nint fixture_luck=1,tool_multiplier=2,tool_divisor=2,rare_roll=500000;\nint skill=100,')
harness=harness.replace('int number(int low,int){return low;}','int number(int low,int high){return high==1000000 ? rare_roll : low;}')
harness=harness.replace('return 2;}\nint crafting_scientific_tools_recipe_player_multiplier(){return 2;}','return tool_divisor;}\nint crafting_scientific_tools_recipe_player_multiplier(){return tool_multiplier;}')
harness=harness.replace('double crafting_salvage_essence_luck_multiplier(){return 1;}','double crafting_salvage_essence_luck_multiplier(){return luck_multiplier;}')
harness=harness.replace('double crafting_salvage_essence_chance_multiplier(){return 1;}','double crafting_salvage_essence_chance_multiplier(){return chance_multiplier;}')
harness+=r'''
void attempt(bool refused,bool rare){
 reset();missing=0;quality=20;eligible=true;
 object original;character actor{&original};char argument[]="fixture";
 do_salvage(&actor,argument,0);
 if(refused){assert(!original.retired && !tools_consumed && !grants && !reads && !notches);}
 else {assert(original.retired && tools_consumed==1 && grants==3+rare && reads==3+rare && notches==1);}
}
int main(){
 // Finite settings can exceed integer conversion ranges without changing the
 // mathematically certain outcome or overflowing the tool-assisted score.
 luck_multiplier=1e308;chance_multiplier=1e308;tool_multiplier=INT_MAX;attempt(false,true);
 fixture_luck=INT_MAX;attempt(false,true);
 luck_multiplier=1;chance_multiplier=1;fixture_luck=1;attempt(false,false);
 tool_multiplier=2;fixture_luck=100;
 luck_multiplier=0.805;attempt(false,false);
 luck_multiplier=0.815;chance_multiplier=0.499999;attempt(false,false);
 chance_multiplier=0.5;attempt(false,true);
 chance_multiplier=0;attempt(false,false);
 const double invalid[]={-1.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
 for(double value:invalid){luck_multiplier=value;chance_multiplier=1;attempt(true,false);}
 luck_multiplier=0;attempt(true,false);
 luck_multiplier=1;
 for(double value:invalid){chance_multiplier=value;attempt(true,false);}
 chance_multiplier=1;
 tool_divisor=0;attempt(true,false);tool_divisor=2;
 tool_multiplier=0;attempt(true,false);
}
'''
command=extract_function('salvage.c','void do_salvage(')
materials=sorted(set(re.findall(r'case (MAT_[A-Z_]+):',command)))
harness=harness.replace('@MATERIALS@','enum { '+','.join(materials)+' };').replace('@COMMAND@',command)
with tempfile.TemporaryDirectory(prefix='duris-salvage-rolls-') as temporary:
 source=Path(temporary)/'salvage.cpp';binary=Path(temporary)/'salvage'
 source.write_text(harness)
 subprocess.run([os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror',
                 '-Wno-unused-parameter','-fsanitize=address,undefined,float-cast-overflow',
                 '-fno-sanitize-recover=all','-fno-pie','-no-pie','-g',str(source),'-o',str(binary)],cwd=ROOT,check=True)
 subprocess.run([str(binary)],check=True,timeout=30)
print('PASS: native salvage handles huge finite multipliers, preserves fractional roll boundaries, and refuses invalid settings before rewards or retirement')
