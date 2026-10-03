#!/usr/bin/env python3
"""Execute production recovery menu/callback bodies with controlled descriptor/queue I/O.

This proves callback and selector boundaries, not socket authentication or live death release.
The real request validator and operation-ID codec are compiled; no DB responses are claimed.
"""
from pathlib import Path
import os
import shlex
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


account = Path(os.environ.get('PLAYER_DEATH_RECOVERY_ACCOUNT_SOURCE', ROOT / 'src/account/account.c')).read_text()
repository = (ROOT / 'src/player/player_load_repository.c').read_text()
signatures = (
    'bool account_descriptor_is_live(',
    'struct acct_chars *account_recovery_character_by_name(',
    'struct acct_chars *account_recovery_character_by_identity(',
    'void account_death_recovery_prompt(',
    'void account_death_recovery_clear(',
    'bool account_death_recovery_submit(',
    'struct acct_chars *account_death_recovery_selection(',
    'bool parse_death_recovery_revision(',
    'void account_death_recovery_input(',
    'void account_recovery_gate_refused(',
    'void account_death_recovery_query_complete(',
)
bodies = function(repository, 'bool player_load_request_valid(') + '\n' + '\n'.join(function(account, s) for s in signatures)
prelude = r'''
#include "player/player_load_pipeline.h"
#include "persistence/persistence_observability.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <strings.h>
#include <utility>
#include <vector>
constexpr int CON_ACCT_SELECT_CHAR=1, CON_DISPLAY_ACCT_MENU=2, CON_PLAYING=3;
constexpr int MAX_CHARS_PER_ACCOUNT=10;
constexpr unsigned char PLAYER_LOAD_MODE_NONE=0, PLAYER_LOAD_MODE_ACCOUNT_DEATH_RECOVERY=3;
struct acct_chars { char *charname; int pid; long last; acct_chars *next=nullptr; };
struct Account { char *acct_name; acct_chars *acct_character_list; };
struct Descriptor {
    Account *account=nullptr; Descriptor *next=nullptr; char *selected_char_name=nullptr;
    uint64_t player_load_request_id=0; int player_load_pid=0;
    unsigned char player_load_mode=PLAYER_LOAD_MODE_ACCOUNT_DEATH_RECOVERY;
    int state=CON_ACCT_SELECT_CHAR; void *character=nullptr; std::string output;
};
using P_desc=Descriptor*;
P_desc descriptor_list=nullptr;
#define STATE(d) ((d)->state)
#define SEND_TO_Q(text,d) ((d)->output.append(text))
std::vector<player_load_request> submitted;
std::vector<uint64_t> cancelled;
unsigned stale=0, menus=0, lists=0;
bool accept_submissions=true;
uint64_t next_id=10;
uint64_t persistence_observability_now_usec() { return 1000000; }
uint64_t player_load_pipeline_next_request_id() { return ++next_id; }
player_load_submit_outcome player_load_pipeline_submit(player_load_request request) {
    if (!accept_submissions) return player_load_submit_outcome::unavailable;
    submitted.push_back(std::move(request)); return player_load_submit_outcome::accepted;
}
bool player_load_pipeline_cancel(uint64_t id) { cancelled.push_back(id); return true; }
void player_load_pipeline_note_stale() { ++stale; }
char *str_dup(const char *text) { return strdup(text); }
void str_free(char *text) { free(text); }
void display_account_menu(P_desc d, void*) { ++menus; d->output += "ACCOUNT MENU\n"; }
void display_character_list(P_desc d, void*) {
    ++lists; d->output += "CHARACTER LIST\n";
    d->state = d->account && d->account->acct_character_list ? CON_ACCT_SELECT_CHAR : CON_DISPLAY_ACCT_MENU;
}
void require(bool ok,const char *message) {
    if(!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
'''
checks = r'''
struct Fixture {
    char owner[16]="Owner", outsider[16]="Outsider", probe[16]="Probe", other[16]="Other";
    acct_chars second{other,999,20}, first{probe,101,10,&second};
    Account account{owner,&first}, foreign{outsider,&first};
    Descriptor descriptor;
    Fixture() { descriptor.account=&account; descriptor_list=&descriptor; submitted.clear(); cancelled.clear(); accept_submissions=true; }
    ~Fixture() { if(descriptor.selected_char_name) str_free(descriptor.selected_char_name); descriptor_list=nullptr; }
    void input(const std::string &text) {
        std::vector<char> buffer(text.begin(),text.end()); buffer.push_back(0);
        account_death_recovery_input(&descriptor,buffer.data());
        require(descriptor.character==nullptr,"recovery input materialized a character");
    }
    void select() { input("Probe"); require(submitted.size()==1,"own name did not submit"); }
    player_load_result result(bool detail=false) {
        const auto &request=submitted.back(); player_load_result r;
        r.request_id=request.request_id; r.pid=request.pid; r.request_account_name=request.account_name;
        r.request_player_name=request.player_name; r.outcome=player_load_outcome::applied;
        r.death_recovery_query.kind=detail ? player_death_recovery_query_kind::detail : player_death_recovery_query_kind::list;
        r.death_recovery_query.outcome=player_death_recovery_query_outcome::read;
        player_death_conflict_case item{}; item.operation_id.bytes[0]=0xad;
        item.save_revision=123; item.source_revision=122; item.corpse_item_uid=501;
        r.death_recovery_query.cases.push_back(item); r.death_recovery_query.detail_identity=item;
        r.death_recovery_query.detail_summary="PRIVATE_CASE_MARKER";
        return r;
    }
};
int main() {
    {
        Fixture f;
        f.input("101"); require(submitted.empty(),"raw PID accepted as an account selection");
        f.input("Stranger"); require(submitted.empty(),"foreign character selection accepted");
        f.input("1"); require(submitted.size()==1 && submitted.back().pid==999,"numeric selection not mapped through sorted own list");
        const auto r=submitted.back();
        require(r.account_name=="Owner" && r.player_name=="Other" && !r.include_items && !r.include_pets,"unsafe worker request");
        f.input("L"); require(submitted.size()==1,"pending request overwritten");
        auto completed=f.result(); f.descriptor.output.clear();
        account_death_recovery_query_complete(&f.descriptor,completed);
        require(f.descriptor.output.find("Case ad")!=std::string::npos && f.descriptor.player_load_request_id==0,"matching list completion not rendered");
        f.input("C"); require(!f.descriptor.selected_char_name && f.descriptor.player_load_pid==0,"character switch retained selected identity");
        f.input("Probe"); require(submitted.back().pid==101,"named selection used wrong trusted PID");
        f.descriptor.output.clear(); account_death_recovery_query_complete(&f.descriptor,f.result());
        f.input("L 123"); require(submitted.back().death_recovery_query.after_revision==123,"page cursor not preserved");
        account_death_recovery_query_complete(&f.descriptor,f.result());
        const auto count=submitted.size();
        for(const char *bad:{"L -1","L 18446744073709551616","L 1 trailing","D bad","D 00000000000000000000000000000000"}) f.input(bad);
        require(submitted.size()==count,"malformed cursor/case ID submitted");
        f.input("D ad000000000000000000000000000000");
        require(submitted.size()==count+1 && submitted.back().death_recovery_query.kind==player_death_recovery_query_kind::detail && submitted.back().death_recovery_query.operation_id.bytes[0]==0xad,"exact case ID not preserved");
        auto detail=f.result(true); detail.death_recovery_query.detail_summary="visible\n\x1b[31m%s$n&r\xff";
        f.descriptor.output.clear(); account_death_recovery_query_complete(&f.descriptor,detail);
        require(f.descriptor.output.find("Detail for retained case ad")!=std::string::npos,"detail heading missing");
        for(const char *bad:{"\x1b","%","$","&","\xff"}) require(f.descriptor.output.find(bad)==std::string::npos,"callback control/format injection rendered");
    }
    for(int fault=0;fault<8;++fault) {
        Fixture f; f.select(); auto result=f.result(true); f.descriptor.output.clear();
        const auto old_stale=stale;
        switch(fault) {
        case 0: ++result.request_id; break;
        case 1: ++result.pid; break;
        case 2: result.request_account_name="Outsider"; break;
        case 3: result.request_player_name="Other"; break;
        case 4: f.descriptor.account=&f.foreign; break;
        case 5: f.first.pid=202; break;
        case 6: f.descriptor.state=CON_PLAYING; break;
        case 7: descriptor_list=nullptr; break;
        }
        account_death_recovery_query_complete(&f.descriptor,result);
        require(stale==old_stale+1 && f.descriptor.output.find("PRIVATE_CASE_MARKER")==std::string::npos,"stale identity rendered a recovery result");
        if(fault==0) require(f.descriptor.player_load_request_id==submitted.back().request_id && f.descriptor.selected_char_name,"old completion retired a newer request");
        if(fault>=6) require(f.descriptor.output.empty(),"disconnected/non-menu callback wrote output");
        require(f.descriptor.character==nullptr,"callback materialized character state");
    }
    {
        Fixture f; f.select(); auto result=f.result(true); auto *closed=new Descriptor;
        delete closed; account_death_recovery_query_complete(closed,result);
        require(f.descriptor.player_load_request_id==submitted.back().request_id,"freed descriptor callback affected live descriptor");
        f.descriptor.output.clear(); f.input("0"); const auto output=f.descriptor.output;
        require(f.descriptor.state==CON_DISPLAY_ACCT_MENU && !f.descriptor.selected_char_name && cancelled.size()==1,"cancel did not return to authenticated menu");
        account_death_recovery_query_complete(&f.descriptor,result);
        require(f.descriptor.output==output,"cancelled callback wrote to later menu");
    }
    for(int fault=0;fault<4;++fault) {
        Fixture f; f.select(); auto result=f.result(true); f.descriptor.output.clear();
        if(fault==0) { result.outcome=player_load_outcome::component_failure; result.death_recovery_query.outcome=player_death_recovery_query_outcome::failed; }
        if(fault==1) { result.outcome=player_load_outcome::timed_out; result.request_account_name.clear(); result.request_player_name.clear(); }
        if(fault==2) { result.death_recovery_query.detail_summary.assign(PLAYER_DEATH_RECOVERY_SUMMARY_MAX+1,'X'); }
        if(fault==3) { result.death_recovery_query.kind=player_death_recovery_query_kind::list; result.death_recovery_query.cases.resize(PLAYER_DEATH_CONFLICT_LIST_LIMIT+1); }
        account_death_recovery_query_complete(&f.descriptor,result);
        require(f.descriptor.player_load_request_id==0 && f.descriptor.output.find("PRIVATE_CASE_MARKER")==std::string::npos && f.descriptor.output.find(std::string(100,'X'))==std::string::npos && f.descriptor.output.find("Case ")==std::string::npos,"failed/unbounded completion rendered data or wedged request");
    }
    for(auto gate:{player_load_recovery_gate::retained_conflict,player_load_recovery_gate::unavailable}) {
        Fixture f; f.select(); account_recovery_gate_refused(&f.descriptor,gate);
        require(f.descriptor.state==CON_DISPLAY_ACCT_MENU && f.descriptor.player_load_mode==PLAYER_LOAD_MODE_NONE && !f.descriptor.selected_char_name && !f.descriptor.player_load_request_id && f.descriptor.character==nullptr && f.descriptor.output.find("option 9")!=std::string::npos,"cold-load refusal did not leave read-only authenticated recovery access");
    }
    {
        Fixture f; accept_submissions=false; f.input("Probe");
        require(submitted.empty() && !f.descriptor.player_load_request_id && f.descriptor.character==nullptr,"unavailable worker created pending or live state");
    }
    std::cout << "PASS: production menu selectors/submit/callback/gate bodies; trusted PID; stable case/cursor; stale/reused/disconnected/freed descriptors; cancel; bounded sanitized output; no character materialization\n";
}
'''
with tempfile.TemporaryDirectory(prefix='death-recovery-account-') as directory:
    directory = Path(directory)
    source = directory / 'account.cpp'
    source.write_text(prelude + '\n' + bodies + '\n' + checks)
    flags = shlex.split(subprocess.check_output(['mysql_config', '--cflags'], text=True))
    libs = shlex.split(subprocess.check_output(['mysql_config', '--libs'], text=True))
    binary = directory / 'account'
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-use-after-free', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-ffunction-sections', '-fdata-sections', '-Isrc', *flags, str(source),
                    'src/player/player_death_recovery_query.c', 'src/persistence/critical_command.c',
                    '-Wl,--gc-sections', *libs, '-lcrypto', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
