"""Exercise the production cache plus the real informational command bodies."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/cmd/actinf.c').read_text()
def extract(signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]
bodies = '\n'.join(extract(signature) for signature in [
    'static void show_information_page(', 'void do_credits(', 'void do_faq(', 'void do_wizlist('])
assert 'get_mud_info' not in bodies and 'affect_timer' not in bodies and 'CharWait' not in bodies
sql_source = (ROOT / 'src/sql/sql.c').read_text()
assert 'SELECT content FROM mud_info' in sql_source  # unrelated freshness-sensitive reads remain direct
nanny_source = (ROOT / 'src/account/nanny.c').read_text()
assert 'get_mud_info("lock")' in nanny_source
assert 'information_cache_get' not in nanny_source  # creation authorization stays fresh
prefix = r'''
#include <string>
#include <cassert>
using namespace std;
constexpr bool TRUE=true;
struct descriptor { string text; };
struct character { descriptor *desc; };
using P_char=character *;
string cached="first";
bool ready=true;
const string *information_cache_get(const string &) { return ready ? &cached : nullptr; }
void send_to_char(const char *s,P_char ch) { ch->desc->text=s; }
void page_string(descriptor *d,char *s,int) { d->text=s; }
'''
main = r'''
int main() {
 descriptor d; character ch{&d};
 do_credits(&ch,nullptr,0); assert(d.text=="first");
 cached="second"; assert(d.text=="first");
 do_faq(&ch,nullptr,0); assert(d.text=="second");
 do_wizlist(&ch,nullptr,0); assert(d.text=="second");
 ready=false; do_credits(&ch,nullptr,0); assert(d.text.find("temporarily unavailable")!=string::npos);
}
'''
(ROOT / 'bin').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=ROOT / 'bin') as temp:
    temp = Path(temp)
    handler = temp / 'handlers.cpp'
    handler.write_text(prefix+bodies+main)
    for name, sources, flags in [
        ('refresh', ['tests/async/refresh_cache_harness.cpp'], []),
        ('information', ['tests/async/information_cache_harness.cpp','src/cmd/information_cache.c',
                         'src/flatfile/flatfile_help_catalog.c'], ['-D__NO_MYSQL__']),
        ('handlers',[str(handler)], [])]:
        binary = temp / name
        subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-pthread','-Isrc',
                        *flags,*sources,'-o',str(binary)],cwd=ROOT,check=True)
        fixture = temp / 'fixture'
        fixture.mkdir(exist_ok=True)
        subprocess.run([str(binary),str(fixture)],cwd=ROOT,check=True,timeout=30)
print('information command navigation passed')
