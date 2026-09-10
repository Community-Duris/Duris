#!/usr/bin/env python3
"""Execute production account-menu/delete bodies with injected persistence failures.

No live DB, account, or player data. The native harness preserves separate durable
and transaction state, checks post-commit publication, and drives numeric selection,
confirmation, cancel, refresh, rollback, retry, and uncertain commit responses.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 0
    for end in range(brace, len(source)):
        depth += (source[end] == '{') - (source[end] == '}')
        if depth == 0:
            return source[start:end + 1]
    raise AssertionError(signature)

account = (ROOT / 'src/account/account.c').read_text()
files = (ROOT / 'src/core/files.c').read_text()
guild = (ROOT / 'src/guild/assocs.c').read_text()
prototypes = (ROOT / 'src/core/prototypes.h').read_text()
# Menu loads must not instantiate inventory/pets before free_char().
load = function(account, 'P_char load_char_into_game(')
assert 'STATE(d) == CON_ACCT_DELETE_CHAR' in load
assert 'request.include_items = false;' in load
assert 'request.include_pets = false;' in load

enum_start = prototypes.index('enum class character_delete_result')
enum_end = prototypes.index('};', enum_start) + 2
bodies = '\n'.join([
    prototypes[enum_start:enum_end],
    'character_delete_result delete_character_result(P_char, bool = true);',
    function(guild, 'bool Guild::save_without_member('),
    function(guild, 'void Guild::forget_deleted_member('),
    function(account, 'void remove_char_from_list('),
    function(files, 'character_delete_result delete_character_result('),
    function(files, 'int deleteCharacter('),
    function(account, 'static void release_delete_character('),
    function(account, 'void account_delete_char('),
])
prelude = r'''
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <cctype>
#include <string>
#define TRUE 1
#define FALSE 0
#define USE_ACCOUNT
#define MAX_CHARS_PER_ACCOUNT 10
#define CON_DISPLAY_ACCT_MENU 1
#define CON_ACCT_DELETE_CHAR 2
#define PLAYER_LOAD_MODE_NONE 0
#define LOG_DEBUG 0
#define LOG_PLAYER 1
#define AVATAR 56
#define SEX_MALE 0
#define GET_NAME(ch) ((ch)->player.name)
#define GET_PID(ch) ((ch)->pid)
#define GET_LEVEL(ch) ((ch)->player.level)
#define GET_SEX(ch) ((ch)->sex)
#define GET_FRAGS(ch) ((ch)->frags)
#define GET_ASSOC(ch) ((ch)->assoc)
#define STATE(d) ((d)->state)
#define FREE(p) free(p)
#define SEND_TO_Q(text,d) ((d)->output += (text))
void checked_snprintf(char *out,size_t size,const char *format,...) {
    va_list args; va_start(args,format); vsnprintf(out,size,format,args); va_end(args);
}
struct member { char name[32]; member *next = nullptr; };
using P_member = member*;
struct Character;
using P_char = Character*;
class Guild {
public:
    member *members = nullptr;
    unsigned member_count = 0;
    struct { long frags=0, top_frags=0; char topfragger[32] = {}; } frags;
    bool save();
    bool save_without_member(P_char);
    void forget_deleted_member(P_char);
};
struct acct_chars { char *charname; int pid; int last; acct_chars *next; };
struct Account { acct_chars *acct_character_list=nullptr; int num_chars=0; };
using P_acct = Account*;
struct Descriptor { Character *character=nullptr; Account *account=nullptr; int player_load_mode=7;
    int state=CON_ACCT_DELETE_CHAR, term_type=7; char host[8]="fixture"; std::string output; };
using P_desc = Descriptor*;
struct Character { struct { char *name; int level; } player; int pid=1, sex=0, frags=7;
    Guild *assoc=nullptr; Descriptor *desc=nullptr; };
static int mode=0, fail_stage=0, stage=0, frees=0, menus=0, refreshes=0, writes=0,
    audits=0, runtime_ships=0, loads=0, backend_calls=0;
static bool in_tx=false, durable_active=true, txn_active=true, rollback_ok=true,
    commit_ok=true, refresh_ok=true, commit_landed=false;
static int durable_cleanup=0, txn_cleanup=0;
static Guild fixture_guild;
constexpr int PERSISTENCE_MODE_FLATFILE_PRIMARY=1;
int persistence_mode_get() { return mode; }
const char *persistence_mode_flatfile_root() { return "fixture"; }
enum class flatfile_character_delete_result { ok, already_deleted, io_error };
flatfile_character_delete_result flatfile_character_delete(const std::string&, int, const std::string&, std::string*) {
    ++backend_calls;
    if (fail_stage) return flatfile_character_delete_result::io_error;
    durable_active=false; return flatfile_character_delete_result::ok;
}
void logit(int kind, const char*,...) { if(kind==LOG_PLAYER) { assert(!durable_active && !in_tx); ++audits; } }
void statuslog(int, const char *fmt,...) { if(strstr(fmt,"deleted")) { assert(!durable_active && !in_tx); ++audits; } }
void persistence_alert(int,const char*,const char*,const char*,const char*,const char*,const char*,...) {}
bool sql_in_transaction() { return in_tx; }
bool sql_begin_transaction() { ++backend_calls; assert(!in_tx); in_tx=true; stage=0; txn_active=durable_active; txn_cleanup=durable_cleanup; return true; }
bool cleanup() { assert(in_tx); if(++stage==fail_stage) return false; ++txn_cleanup; return true; }
bool sql_soft_delete_character(int) { if(!cleanup())return false; txn_active=false; return true; }
bool remove_all_artifacts_sql(P_char) { return cleanup(); }
bool remove_all_locker_access(P_char) { return cleanup(); }
bool Guild::save() { assert(members==nullptr && member_count==1 && frags.frags==0); return cleanup(); }
bool sql_delete_locker(int,int) { return cleanup(); }
bool sql_delete_ship(const char*) { return cleanup(); }
bool sql_delete_player(int, bool forget) { assert(!forget); return cleanup(); }
void player_revision_forget(int) { assert(!durable_active && !in_tx); }
bool sql_commit() { assert(in_tx); if(commit_ok || commit_landed) {durable_active=txn_active; durable_cleanup=txn_cleanup;} if(!commit_ok)return false; in_tx=false; return true; }
bool sql_rollback() { assert(in_tx); in_tx=false; return rollback_ok; }
void delete_ship_runtime(const char*) { assert(!durable_active && !in_tx); ++runtime_ships; }
int write_account(P_acct) { ++writes; return 1; }
int read_account(P_acct a) {
    assert(!in_tx); ++refreshes;
    if(!refresh_ok) return -1;
    // Model a backend refresh hiding a committed tombstone after a lost COMMIT reply.
    if(!durable_active) {
        auto link=&a->acct_character_list;
        while(*link) {
            if((*link)->pid==1) {
                auto old=*link; *link=old->next; free(old->charname);free(old);--a->num_chars;
            } else link=&(*link)->next;
        }
    }
    return 1;
}
void free_char(P_char ch) { assert(!ch->desc); assert(!ch->assoc || fixture_guild.members); ++frees; free(ch->player.name); delete ch; }
void display_account_menu(P_desc d, char*) { assert(!d->character); assert(STATE(d)==CON_DISPLAY_ACCT_MENU); ++menus; }
void display_delete_character_list(P_desc) {}
size_t strlcpy(char *out,const char *s,size_t n) { snprintf(out,n,"%s",s); return strlen(s); }
P_char load_char_into_game(acct_chars *c,P_desc d) { ++loads; auto ch=new Character; ch->player={strdup(c->charname),10}; ch->desc=d; ch->assoc=&fixture_guild; return ch; }
void remove_char_from_list(P_acct,char*,bool=true);
'''
main = r'''
static void input(P_desc d,const char *s) { account_delete_char(d,const_cast<char*>(s)); }
static void reset(Account &a,Descriptor &d) {
    mode=fail_stage=stage=frees=menus=refreshes=writes=audits=runtime_ships=loads=backend_calls=0;
    in_tx=false; durable_active=txn_active=rollback_ok=commit_ok=refresh_ok=true;
    commit_landed=false; durable_cleanup=txn_cleanup=0;
    a.acct_character_list=(acct_chars*)calloc(1,sizeof(acct_chars));
    *a.acct_character_list={strdup("Fixture"),1,1,nullptr}; a.num_chars=1;
    d=Descriptor{}; d.account=&a;
    fixture_guild.members=new member; strcpy(fixture_guild.members->name,"Fixture");
    fixture_guild.member_count=1; fixture_guild.frags.frags=7;
    fixture_guild.frags.top_frags=7; strcpy(fixture_guild.frags.topfragger,"Fixture");
}
static void released(Descriptor &d) { assert(!d.character && frees==loads); assert(d.player_load_mode==0); assert(d.term_type==7); assert(menus>0); }
static void dispose(Account &a) {
    while(a.acct_character_list) {auto old=a.acct_character_list;a.acct_character_list=old->next;free(old->charname);free(old);}
    delete fixture_guild.members; fixture_guild.members=nullptr;
}
int main() {
    Account a; Descriptor d;
    // Every SQL stage: soft-delete refusal through a late player-data cleanup failure.
    for(int failure=1;failure<=7;++failure) {
        reset(a,d); fail_stage=failure; input(&d,"1"); input(&d,"yes"); released(d);
        assert(a.num_chars==1 && durable_active && durable_cleanup==0 && !audits && !runtime_ships && !writes);
        assert(fixture_guild.member_count==1 && fixture_guild.frags.frags==7);
        assert(d.output.find("did not complete")!=std::string::npos);
        assert(d.output.find("successfully")==std::string::npos && refreshes==1);
        fail_stage=0; input(&d,"1"); input(&d,"yes"); released(d);
        assert(!durable_active && durable_cleanup==7 && audits==2 && runtime_ships==1);
        assert(!a.acct_character_list && a.num_chars==0 && !writes && refreshes==2);
        assert(!fixture_guild.members && fixture_guild.member_count==0 && fixture_guild.frags.frags==0);
        int calls=backend_calls; input(&d,"yes"); assert(backend_calls==calls && audits==2);
        dispose(a);
    }
    reset(a,d); mode=1; fail_stage=1; input(&d,"1"); input(&d,"yes"); released(d);
    assert(durable_active && !audits && a.num_chars==1);
    fail_stage=0; input(&d,"1"); input(&d,"yes"); released(d);
    assert(!durable_active && audits==2 && !a.num_chars && !writes); dispose(a);
    for(const char *cancel:{"no","0","back"}) {
        reset(a,d); input(&d,"1"); input(&d,cancel); released(d);
        assert(!backend_calls && !audits && durable_active); dispose(a);
    }
    reset(a,d); input(&d,"1"); input(&d,"1"); assert(frees==1 && loads==2); input(&d,"no"); released(d); dispose(a);
    for(bool landed:{false,true}) {
        reset(a,d); commit_ok=false; commit_landed=landed; input(&d,"1"); input(&d,"yes"); released(d);
        assert(!audits && !runtime_ships && !writes && refreshes==1);
        assert(a.num_chars==(landed ? 0 : 1));
        assert(d.output.find("Some cleanup may have completed")!=std::string::npos); dispose(a);
    }
    reset(a,d); fail_stage=7; rollback_ok=false; input(&d,"1"); input(&d,"yes"); released(d);
    assert(!audits && d.output.find("could not be confirmed")!=std::string::npos); dispose(a);
    reset(a,d); refresh_ok=false; input(&d,"1"); input(&d,"yes"); released(d);
    assert(d.output.find("could not be refreshed")!=std::string::npos); dispose(a);
    for(bool at_head:{false,true}) {
        reset(a,d);
        auto other=(acct_chars*)calloc(1,sizeof(acct_chars));
        *other={strdup("Other"),2,0,nullptr};
        if(at_head) a.acct_character_list->next=other;
        else {other->next=a.acct_character_list;a.acct_character_list=other;}
        a.num_chars=2; input(&d,"1"); input(&d,"yes"); released(d);
        assert(a.num_chars==1 && a.acct_character_list==other && !other->next && !writes);
        dispose(a);
    }
    puts("PASS: account deletion runtime failure, release, publication and retry scenarios");
}
'''
build_root = ROOT / 'bin/tests'
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='account-delete-', dir=build_root) as directory:
    cpp = Path(directory) / 'runtime.cpp'
    cpp.write_text(prelude + bodies + main)
    exe = Path(directory) / 'runtime'
    subprocess.run(['g++', '-std=c++20', '-g', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
