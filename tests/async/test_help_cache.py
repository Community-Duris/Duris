"""Run the production cache state machine and actual help lookup/render bodies."""
from pathlib import Path
import ast
import json
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/cmd/wikihelp.c').read_text()

def function(signature, start=0):
    begin = source.index(signature, start)
    opening = source.index('{', begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]

sql_branch = source.index('#else')
lookup = function('string wiki_help(string str)', sql_branch)
render = function('string wiki_help_single(string str)', sql_branch)
assert 'qry(' not in lookup + render and 'mysql_' not in lookup + render
actinf = (ROOT / 'src/cmd/actinf.c').read_text()
help_body = actinf.split('void do_help(', 1)[1].split('void do_wizhelp', 1)[0]
assert 'CharWait(' not in help_body and 'affect_timer(' not in help_body

prefix = r'''
#include "cmd/help_cache.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace std;
constexpr int WIKIHELP_RESULTS_LIMIT=100, MAX_INPUT_LENGTH=1024;
constexpr int WIKI_CLASS=1, WIKI_RACE=2, WIKI_SPEC=3;
const char *LOG_HELP="help";
void logit(const char *, const char *, ...) {}
int ansi_strlen(const char *s) { return strlen(s); }
void one_argument(const char *s, char *out) { auto n=strcspn(s," "); memcpy(out,s,n); out[n]=0; }
help_catalog pages;
bool ready=false;
const help_catalog *help_cache_get() { return ready ? &pages : nullptr; }
bool help_title_equal(const string &a,const string &b) { return strcasecmp(a.c_str(),b.c_str())==0; }
bool help_title_matches(const string &a,const string &b) {
 return search(a.begin(),a.end(),b.begin(),b.end(),[](unsigned char x,unsigned char y){return tolower(x)==tolower(y);}) != a.end(); }
string wiki_help_single(string);
string dewikify(string s) { return s; }
string trim(const string &s,const char *chars) {
 auto begin=s.find_first_not_of(chars); return begin==string::npos ? "" : s.substr(begin,s.find_last_not_of(chars)-begin+1); }
int dynamic_version=1;
'''
for name in ['wiki_classes', 'wiki_racial_stats', 'wiki_specs', 'wiki_multiclass', 'wiki_pcraces']:
    prefix += f'string {name}(string) {{ return "dynamic"+to_string(dynamic_version); }}\n'
for name in ['wiki_innates', 'wiki_races', 'wiki_skills', 'wiki_spells']:
    prefix += f'string {name}(string,int) {{ return "dynamic"+to_string(dynamic_version); }}\n'
main = r'''
void add(string title,string text,string category="1") {
 pages.push_back({{title,text,category,"2026-09-10","Editor"}}); }
int main() {
 assert(wiki_help("help").find("unavailable")!=string::npos);
 ready=true; add("help","welcome"); add("Fire","burn","9"); add("Fire shield","protect");
 add("Alias","Redirect: Fire shield"); add("Cycle","Redirect: Cycle");
 assert(wiki_help("").find("welcome")!=string::npos);
 assert(wiki_help("fIrE").find("burn")!=string::npos);
 assert(wiki_help("Fire").find("also matched")!=string::npos);
 assert(wiki_help("shield").find("protect")!=string::npos);
 assert(wiki_help("missing").find("no help topics")!=string::npos);
 assert(wiki_help("Alias").find("protect")!=string::npos);
 assert(wiki_help("Cycle").find("redirect limit")!=string::npos);
 assert(wiki_help("Fire").find("Editor")!=string::npos);
 assert(wiki_help("Fire").find("dynamic1")!=string::npos);
 dynamic_version=2;
 assert(wiki_help("Fire").find("dynamic2")!=string::npos);
 for(int i=0;i<150;++i) add("bulk"+to_string(i),"text");
 auto result=wiki_help("bulk");
 assert(result.find("bulk100\n")!=string::npos);
 assert(result.find("bulk101\n")==string::npos);
 for(int i=0;i<10000;++i) assert(wiki_help("Fire").find("burn")!=string::npos);
 // Catalog entries are parsed by the maintained SQL importer's actual parser.
 pages.clear();
 __GAMEPLAY_HELP__
 const auto quick_step = wiki_help("quick step");
 for (const char *query : {"quickstep", "QuIcKsTeP"}) {
   assert(wiki_help(query) == quick_step);
   assert(quick_step.find("powerful blow") != string::npos);
 }
 for (const char *query : {"seadog", "SeAdOg"}) {
   const auto seadog = wiki_help(query);
   for (const char *meaning : {"passive seafaring", "owner with Seadog", "anywhere aboard",
       "person operating the panel", "bonus will not persist", "10% more proceeds",
       "Humans and Orcs", "level 1", "Swashbucklers", "level 30"})
     assert(seadog.find(meaning) != string::npos);
 }
 cout << "help matching, redirects, dynamic content, metadata, repeated reads and gameplay topics passed\n";
}
'''
# Execute only the importer's pure parsing functions: no SQL or live environment.
importer = (ROOT / 'scripts/import_help_to_prod.sh').read_text()
block = re.findall(r"python3 <<'PYTHON_SCRIPT'\n(.*?)\nPYTHON_SCRIPT", importer, re.S)[1]
definitions = [node for node in ast.parse(block).body if isinstance(node, ast.FunctionDef)
               and node.name in ('help_index_entries', 'parse_help_index')]
assert len(definitions) == 2
parser = {'re': re}
exec(compile(ast.Module(body=definitions, type_ignores=[]), '<help importer>', 'exec'), parser)
entries = parser['parse_help_index'](ROOT / 'lib/information/help_index')
topics = {'QUICK STEP', 'QUICKSTEP', 'SEADOG'}
selected = [(title, text) for title, text in entries if title.upper() in topics]
assert {title.upper() for title, _ in selected} == topics, 'gameplay help topics missing'
main = main.replace('__GAMEPLAY_HELP__', '\n'.join(
    f'add({json.dumps(title)}, {json.dumps(text)});' for title, text in selected))

out = Path(os.environ.get('BIN_ROOT', ROOT / 'bin')) / 'tests'
out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(dir=out) as temp:
    temp = Path(temp)
    replacement = temp / 'replacement.cpp'
    replacement.write_text('#include <string>\n#include <cassert>\nusing namespace std;\n' +
                           function('string str_replace(') + r'''
int main() {
 string text; for(int i=0;i<20000;++i) text += "[[x]]";
 auto converted=str_replace(text,"[[","&+c");
 assert(converted.size()==120000);
 assert(str_replace("aaaa","a","aa")=="aaaaaaaa");
 assert(str_replace("abc","", "x")=="abc");
}
''')
    subprocess.run(['g++','-std=c++20',str(replacement),'-o',str(temp/'replacement')],check=True)
    subprocess.run([str(temp/'replacement')],check=True,timeout=30)
    for name, files in [('refresh', [ROOT / 'tests/async/refresh_cache_harness.cpp']),
                        ('help', [temp / 'help.cpp'])]:
        if name == 'help':
            files[0].write_text(prefix + lookup + '\n' + render + main)
        binary = temp / name
        subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-pthread',
                        '-Isrc', *map(str,files), '-o',str(binary)],cwd=ROOT,check=True)
        subprocess.run([str(binary)],cwd=ROOT,check=True,timeout=30)
