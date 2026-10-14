#!/usr/bin/env python3
"""Compare actual original output-queue bodies; controlled allocator faults only.

This exercises value/resource preparation. It cannot qualify descriptor lifetime,
SQL/source/native publication, the private recipient transfer or ACK.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, extract_function

original = extract_function("comm.c", "void write_to_q(")
prepared = (ROOT / "src/net/consumable_prepared_output_queue.c").read_text()

HARNESS = r'''
#include "core/structs.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/consumable_prepared_output_queue.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

// The same controlled allocator boundary as session_queues_runtime_harness.
// Full original queue bodies run; no game actor/current/permission is simulated.
static size_t allocation_count, allocation_cut = SIZE_MAX, live_allocations;
void *__try_malloc(size_t bytes, const char *, const char *, int) noexcept {
    if (allocation_count++ == allocation_cut) return nullptr;
    void *p = std::calloc(1,bytes); if(p) ++live_allocations; return p;
}
void __free(void *p, const char *, int) { if(p) {assert(live_allocations); --live_allocations; std::free(p);} }
#undef CREATE
#undef RECREATE
#undef FREE
#define CREATE(result,type,count,tag) do { result=(type *)__try_malloc(sizeof(type)*(count),tag,__FILE__,__LINE__); assert(result); } while(false)
#define RECREATE(result,type,count) do { void *replacement=__try_malloc(sizeof(type)*(count),"M",__FILE__,__LINE__); assert(replacement); std::memcpy(replacement,result,std::min(std::strlen((char *)result)+1,sizeof(type)*(count))); __free(result,__FILE__,__LINE__); result=(type *)replacement; } while(false)
#define FREE(p) do {__free(p,__FILE__,__LINE__); p=nullptr;} while(false)
void logit(const char *, const char *, ...) {assert(false && "invalid original fixture queue");}
'''

DRIVER = r'''
struct queue_image {
    txt_q header; std::vector<const txt_block *> nodes; std::vector<std::string> text;
};
queue_image image(const txt_q &q) {
    queue_image out{q,{},{}};
    for(const txt_block *p=q.head;p;p=p->next){out.nodes.push_back(p);out.text.emplace_back(p->text);}
    return out;
}
bool unchanged(const txt_q &q, const queue_image &before) {
    auto after=image(q);
    return q.head==before.header.head && q.tail==before.header.tail && q.bytes==before.header.bytes &&
      q.entries==before.header.entries && q.overflowed==before.header.overflowed &&
      q.overflow_reported==before.header.overflow_reported && after.nodes==before.nodes && after.text==before.text;
}
void clear(txt_q &q) {while(q.head){auto *p=q.head;q.head=p->next;__free(p->text,__FILE__,__LINE__);__free(p,__FILE__,__LINE__);}q={};}
void check(std::vector<std::string> seed, std::vector<std::string> messages, bool overflow=false) {
    txt_q actual{}, reference{};
    allocation_cut=SIZE_MAX;
    for(const auto &s:seed){write_to_q(s.c_str(),&actual,1);write_to_q(s.c_str(),&reference,1);}
    actual.overflowed=reference.overflowed=overflow;
    auto before=image(actual);
    std::vector<std::string_view> views;
    for(const auto &s:messages) views.emplace_back(s);
    for(const auto &s:messages) write_to_q(s.c_str(),&reference,1);
    auto expected=image(reference).text;
    allocation_count=0;
    {
      auto candidate=consumable_prepared_output_queue::prepare(actual,views);assert(candidate);
      std::vector<std::string> bytes;
      assert(candidate->copy_proposed_bytes(&bytes));assert(bytes==expected);assert(unchanged(actual,before));
      consumable_proposed_output_queue_state proposed;
      assert(candidate->proposed_state(&proposed));
      assert(proposed.bytes==reference.bytes && proposed.entries==reference.entries && proposed.overflowed==reference.overflowed);
      size_t storage=0;assert(candidate->current_storage_bytes(&storage)&&storage);
    }
    const size_t successful_allocations=allocation_count;
    const size_t surviving=live_allocations;
    for(size_t cut=0;cut<successful_allocations;++cut){
      allocation_count=0;allocation_cut=cut;
      auto candidate=consumable_prepared_output_queue::prepare(actual,views);
      assert(!candidate);assert(live_allocations==surviving);assert(unchanged(actual,before));
    }
    allocation_cut=SIZE_MAX;
    clear(actual);clear(reference);assert(live_allocations==0);
}
int main(){
    check({}, {"First\r\n","Second\r\n","Third\r\n"});
    check({"Existing tail: "}, {"quaff."," energy.","\r\n"});
    check({std::string(MAX_INPUT_LENGTH-2,'a')}, {"b","c","d"});
    check({std::string(MAX_INPUT_LENGTH+2,'a')}, {"First","Second"});
    check({}, {std::string(MAX_STRING_LENGTH-1,'a'),"next"});
    check({"preserve"}, {std::string(MAX_STRING_LENGTH,'x'),"not admitted"});
    check({"already overflowed"}, {"unchanged"}, true);
    check({}, {std::string("embedded\0ignored",16),"tail"});
    check({"base"}, {"",""});
    std::puts("Original output bytes and pre-effect allocation-fault refusal passed; recipient/current/private transfer/native/ACK remain unqualified.");
}
'''

build = Path(os.environ.get("BIN_ROOT", str(ROOT / "bin"))) / "tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="consumable-output-", dir=build) as directory:
    tmp = Path(directory)
    source = tmp / "original_output.cpp"
    source.write_text(HARNESS + "\n" + original + "\n" + prepared + "\n" + DRIVER)
    binary = tmp / "harness"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-D__NO_MYSQL__", "-I", str(ROOT / "src"), "-I", str(ROOT / "src/no_mysql"),
        str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
