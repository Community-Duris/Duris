#!/usr/bin/env python3
"""Measure bounded item-action work with the production scheduler and runtime.

This is an explicit benchmark, not a timing assertion in CI. Spell payloads and
world lookup use the foundation fixture; results are not whole-server capacity.
"""
import ast
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
def literal(path,name):
    return next(ast.literal_eval(node.value) for node in ast.parse(path.read_text()).body
        if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id==name for t in node.targets))
def function(text,signature):
    start=text.index(signature); end=text.index('{',start)+1; depth=1
    while depth: depth+=(text[end]=='{')-(text[end]=='}'); end+=1
    return text[start:end]

platform=literal(ROOT/'tests/async/test_nevent_scheduler_runtime.py','HARNESS').split('struct record_payload\n',1)[0]
platform=platform.replace('DEFINE_LABEL_CALLBACK(event_item_action_active)','')
fixture=literal(ROOT/'tests/async/test_item_actions_runtime.py','HARNESS')
fixture=fixture.replace('int main() {','void foundation_main() {').replace('// INSERT_PRODUCTION_ABORT',
    function((ROOT/'src/net/sparser.c').read_text(),'void do_abort(P_char ch,'))
BENCH=r'''
#include <chrono>
static uint64_t bench_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
int main() {
    nevent_bind_game_thread(); ne_dead_event_pool=&test_pool;
    fake_clock_ns=1000000000ULL; ne_events();
    for(int observed:{0,1}) for(int count:{1,128,1024,4096}) {
        scene s;
        properties["itemActions.telemetry.enabled"]=observed; update_item_action_properties();
        std::vector<std::pair<P_char,P_obj>> population{{s.actor,s.source}};
        for(int i=1;i<count;++i) {
            auto actor=new char_data{}; auto object=new obj_data{};
            actor->only.pc=&s.actor_pc; actor->runtime_id=1000+i;
            actor->in_room=0; SET_POS(actor,STAT_NORMAL+POS_STANDING);
            actor->next=character_list; character_list=actor;
            object->obj_uid=10000+i; object->loc_p=LOC_WORN; object->loc.wearing=actor;
            actor->equipment[WIELD]=object;
            object->next=object_list; object_list=object;
            population.emplace_back(actor,object);
        }
        uint64_t start=bench_ns(), admission_max=0;
        for(const auto &[actor,object]:population) {
            uint64_t before=bench_ns();
            assert(start_item_action(1,actor,s.target,object)==item_action_start::scheduled);
            admission_max=std::max(admission_max,bench_ns()-before);
        }
        uint64_t admission=bench_ns()-start;
        assert(item_actions_pending()==static_cast<size_t>(count));
        uint64_t tick_max=0; start=bench_ns();
        for(int i=0;i<12;++i) {
            uint64_t before=bench_ns(); advance(1);
            tick_max=std::max(tick_max,bench_ns()-before);
        }
        uint64_t release=bench_ns()-start;
        assert(!item_actions_pending() && s.report.effects==count);
        auto metrics=item_actions_telemetry_snapshot();
        printf("{\"telemetry\":%d,\"pending\":%d,\"admission_total_us\":%llu,\"admission_max_us\":%llu,"
            "\"release_total_us\":%llu,\"tick_max_us\":%llu,\"callback_max_us\":%llu}\n",
            observed,count,(unsigned long long)(admission/1000),(unsigned long long)(admission_max/1000),
            (unsigned long long)(release/1000),(unsigned long long)(tick_max/1000),
            (unsigned long long)metrics.callback_max_us);
    }
    // The exact Studio expiry sweep at its configured 65,536-entry bound.
    for(bool expired:{false,true}) {
        std::map<std::pair<uint64_t,uint32_t>,uint64_t> cooldowns;
        for(uint64_t i=0;i<65536;++i) cooldowns[{i,1}]=expired?1:100;
        const uint64_t now=2, start=bench_ns();
        // INSERT_COOLDOWN_SWEEP
        printf("{\"studio_cooldowns\":65536,\"expired\":%d,\"sweep_us\":%llu,\"remaining\":%zu}\n",
            expired,(unsigned long long)((bench_ns()-start)/1000),cooldowns.size());
    }
}
'''
studio=(ROOT/'src/item/studio_abilities.c').read_text()
start=studio.index('for (auto it = cooldowns.begin();')
end=studio.index('\n\titem_action_definition action;',start)
bench=BENCH.replace('// INSERT_COOLDOWN_SWEEP',studio[start:end])
with tempfile.TemporaryDirectory(prefix='item-action-benchmark-') as tmp:
    source=Path(tmp)/'benchmark.cpp'; binary=Path(tmp)/'benchmark'
    source.write_text(platform+fixture+bench)
    subprocess.run(['g++','-std=c++20','-O2','-ffunction-sections','-fdata-sections','-pthread',
        '-I'+str(ROOT/'src'),str(source),str(ROOT/'src/persistence/latency_trace.c'),
        '-Wl,--gc-sections','-o',str(binary)],check=True)
    env=dict(os.environ,DURIS_NEVENT_ANALYTICS='0',DURIS_NEVENT_BUDGET_USEC='0',
        DURIS_NEVENT_MAX_CALLBACKS='0',DURIS_NEVENT_PLAYER_PRIORITY='1')
    rows=subprocess.check_output([str(binary)],text=True,env=env)
    print(json.dumps({'compiler':'g++ -O2, no sanitizers','cases':[json.loads(row) for row in rows.splitlines()]},indent=2))
