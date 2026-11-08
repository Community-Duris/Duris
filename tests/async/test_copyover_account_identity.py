#!/usr/bin/env python3
"""Execute copyover's authoritative account lookup with distinct identities."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

code = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
static acct_entry account;
static acct_chars member;
static bool allocation_ok=true, read_ok=true, same_name=true, owns=true;
static int reads, frees;
P_acct allocate_account() { return allocation_ok ? &account : nullptr; }
char *str_dup(const char *text) { return strdup(text); }
P_acct free_account(P_acct value) { ++frees; free(value->acct_name); value->acct_name=nullptr; return nullptr; }
int read_account(P_acct value) {
    ++reads; assert(!strcmp(value->acct_name,"Journeyacct"));
    if(!read_ok) return -1;
    if(!same_name) { free(value->acct_name); value->acct_name=strdup("Otheraccount"); }
    member.pid=owns ? 17 : 18; member.charname=const_cast<char *>("Taverek");
    value->acct_character_list=&member; return 1;
}
void logit(const char *, const char *, ...) {}
''' + extract_function('copyover.c', 'static P_acct copyover_load_account(') + r'''
int main() {
    assert(!copyover_load_account("",17,"Taverek") && !reads);
    assert(!copyover_load_account("Journeyacct",17,nullptr) && !reads);
    allocation_ok=false; assert(!copyover_load_account("Journeyacct",17,"Taverek")); allocation_ok=true;
    read_ok=false; assert(!copyover_load_account("Journeyacct",17,"Taverek") && frees==1); read_ok=true;
    same_name=false; assert(!copyover_load_account("Journeyacct",17,"Taverek") && frees==2); same_name=true;
    owns=false; assert(!copyover_load_account("Journeyacct",17,"Taverek") && frees==3); owns=true;
    assert(copyover_load_account("Journeyacct",17,"Taverek")==&account);
    free_account(&account);
}
'''
with tempfile.TemporaryDirectory(prefix='copyover-account-identity-') as directory:
    path = Path(directory)
    (path/'test.cpp').write_text(code)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc',
                    str(path/'test.cpp'), '-o', str(path/'test')], cwd=ROOT, check=True)
    subprocess.run([str(path/'test')], check=True)
print('copyover distinct account identity, allocation, read and membership refusal passed')
