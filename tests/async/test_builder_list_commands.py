#!/usr/bin/env python3
"""Execute production builder handlers and ANSI helpers with inert indexed fixtures.

No server/database is started. ASan/UBSan cover output and table boundaries;
measured comparisons/probes reject hidden world scans. Registration is checked
separately; gameplay dispatch/paging is covered by the local runtime journey.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, HARNESS_STUBS, extract_function

PRELUDE = r"""
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/utility.h"
#include "core/profile.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
P_room world = nullptr;
P_index mob_index = nullptr, obj_index = nullptr;
zone_data *zone_table = nullptr;
int top_of_world = -1, top_of_mobt = -1, top_of_objt = -1, top_of_zone_table = -1;
const char *sector_types[NUM_SECT_TYPES] = {"Inside"};
static char_data entity_sentinel{};
static obj_data object_sentinel{};
P_char character_list = &entity_sentinel;
P_obj object_list = &object_sentinel;
static string output;
static int comparisons, probes;
void send_to_char(const char *text, P_char) { output += text; }
// Any loader call is a test failure. Other gameplay endpoints remain unlinked.
P_char read_mobile(int, int) { abort(); }
P_obj read_object(int, int) { abort(); }
void extract_char(P_char) { abort(); }
void extract_obj(P_obj) { abort(); }
template<class It, class Compare>
It measured_lower_bound(It first, It last, int value, Compare less) {
    return std::lower_bound(first, last, value, [&](const auto &a, int b) {
        ++comparisons; return less(a,b);
    });
}
#define fopen(...) (abort(), static_cast<FILE *>(nullptr))
#define fread(...) (abort(), size_t(0))
#define fseek(...) (abort(), 0)
"""

DRIVER = r"""
using Handler = void (*)(P_char, char *, int);
static Handler handlers[] = {do_rlist, do_mlist, do_olist};
static const char *commands[] = {"rlist", "mlist", "olist"};
static const char *plurals[] = {"rooms", "mobiles", "objects"};
static vector<room_data> rooms;
static vector<index_data> mobiles, objects;
static zone_data zones[3];
static reset_com cross_zone_resets[2];
static char_data staff;
static string name = "&+GCold\r\nname&n";

static void fixture(int matches, int prefix = 1, int suffix = 1) {
    rooms.assign(prefix + matches + suffix, {});
    mobiles.assign(rooms.size(), {}); objects.assign(rooms.size(), {});
    memset(zones, 0, sizeof(zones)); memset(&staff, 0, sizeof(staff));
    zones[0].number=17; zones[0].top=99999;
    zones[1].number=83; zones[1].top=199999;
    zones[2].number=145; zones[2].top=INT_MAX;
    for(int z=0;z<3;++z) zones[z].real_bottom=zones[z].real_top=-1;
    for(size_t i=0;i<rooms.size();++i) {
        const int zone = i < size_t(prefix) ? 0 : i < size_t(prefix+matches) ? 1 : 2;
        const int vnum = zone==0 ? int(i) : zone==1 ? 100000+int(i)-prefix : 200000+int(i)-prefix-matches;
        rooms[i].number=vnum; rooms[i].zone=zone; rooms[i].name=name.data();
        mobiles[i].virtual_number=objects[i].virtual_number=vnum;
        mobiles[i].desc2=objects[i].desc2=name.data();
        mobiles[i].number=objects[i].number=7;
        mobiles[i].limit=objects[i].limit=11;
        if(zones[zone].real_bottom<0) zones[zone].real_bottom=int(i);
        zones[zone].real_top=int(i);
    }
    world=rooms.data(); mob_index=mobiles.data(); obj_index=objects.data();
    top_of_world=top_of_mobt=top_of_objt=int(rooms.size())-1;
    zone_table=zones; top_of_zone_table=2;
    // Another zone resets a prototype from zone 83: ownership must not follow it.
    cross_zone_resets[0].command='M'; cross_zone_resets[0].arg1=prefix;
    cross_zone_resets[1].command='S'; zones[2].cmd=cross_zone_resets;
    staff.in_room=prefix;
    SET_BIT(staff.specials.act, PLR_PAGING_ON);
}
static string run(int kind, const string &argument) {
    auto before_rooms=rooms; auto before_mobiles=mobiles; auto before_objects=objects;
    const auto before_staff=staff;
    const auto before_entity=entity_sentinel;
    const auto before_object=object_sentinel;
    output.clear(); comparisons=probes=0;
    string input=argument;
    handlers[kind](&staff,input.data(),0);
    assert(comparisons<=18 && probes<=502);
    assert(output.size()<15*MAX_STRING_LENGTH); // Existing command pager capacity.
    assert(character_list==&entity_sentinel && object_list==&object_sentinel);
    assert(memcmp(&staff,&before_staff,sizeof(staff))==0);
    assert(memcmp(&entity_sentinel,&before_entity,sizeof(entity_sentinel))==0);
    assert(memcmp(&object_sentinel,&before_object,sizeof(object_sentinel))==0);
    assert(memcmp(rooms.data(),before_rooms.data(),rooms.size()*sizeof(room_data))==0);
    assert(memcmp(mobiles.data(),before_mobiles.data(),mobiles.size()*sizeof(index_data))==0);
    assert(memcmp(objects.data(),before_objects.data(),objects.size()*sizeof(index_data))==0);
    return output;
}
static int row_count(const string &text) {
    istringstream lines(text); int rows=0;
    for(string line;getline(lines,line);) if(line.size()>1 && line[0]=='|') {
        const auto end=line.find('|',1); const auto first=line.substr(1,end-1);
        if(first.find_first_not_of(" -0123456789")==string::npos && first.find_first_of("0123456789")!=string::npos) {
            ++rows;
            assert(strip_ansi(line.c_str()).size()<=80);
        }
    }
    return rows;
}
static void shown(int kind,const string &arg,int count,bool truncated=false) {
    const string text=run(kind,arg);
    assert(row_count(text)==count);
    assert((text.find("More matches exist")!=string::npos)==truncated);
    const string expected=count ? "Shown: "+to_string(count)+" "+plurals[kind]+"." : "No matching "+string(plurals[kind])+".";
    assert(text.find(expected)!=string::npos);
    if(kind && count) assert(text.find("|          7|         11|")!=string::npos);
}
int main() {
    for(int kind=0;kind<3;++kind) {
        for(int matches : {0,1,199,499,500,501,700}) {
            fixture(matches);
            // Empty current zones need a standing room in another zone; explicit
            // selection still tests the empty requested zone.
            shown(kind,"zone 83",min(matches,500),matches>500);
            if(matches) {
                shown(kind,"",min(matches,500),matches>500);
                shown(kind,"  \t ",min(matches,500),matches>500);
                assert(output.find("zone 83")!=string::npos);
                if(kind==0) assert(output.find("|    83|")!=string::npos);
            }
            if(matches>500) {
                const string next=string(commands[kind])+" zone 83 100500 2147483647";
                assert(output.find(next)!=string::npos);
                shown(kind,"zone 83 100500 2147483647",matches-500);
            }
        }
        fixture(700,20000,20000);
        shown(kind,"zone 83",500,true);
        assert(probes==501); // No suffix traversal or hidden count pass.
        shown(kind,"100000 100699",500,true);
        assert(output.find(string(commands[kind])+" 100500 100699")!=string::npos);
        shown(kind,"100500 100699",200);
        fixture(3);
        shown(kind,"100000",1);
        shown(kind,"100002 100000",3);
        shown(kind,"zone 83 100000",1);
        shown(kind,"zone 83 0 100001",2);
        shown(kind,"zone 83 0",0);
        shown(kind,"100003 199999",0);
        shown(kind,"zone 17",1);
        shown(kind,"zone 145",1);
        shown(kind,"0 2147483647",5);
        rooms.back().number=INT_MAX; mobiles.back().virtual_number=objects.back().virtual_number=INT_MAX;
        shown(kind,"2147483647",1);
        shown(kind,"zone 145 2147483647",1);
        staff.in_room=NOWHERE;
        shown(kind,"100000",1);
        shown(kind,"zone 83",3);
        assert(run(kind,"").find("no valid current zone")!=string::npos);
        staff.in_room=top_of_world+1;
        assert(run(kind,"").find("no valid current zone")!=string::npos);
        staff.in_room=0; rooms[0].zone=USHRT_MAX;
        assert(run(kind,"").find("no valid current zone")!=string::npos);
        if(kind==0) shown(kind,"0",1); // Explicit searches can show a room with no valid zone.
        const vector<string> bad={"-", "-1", "+1", "1x", "2147483648", "99999999999999999999999999", "1 -2", "1 2 3", "zone", "zone -1", "zone 1", "zone 0", "zone 83 x", "zone 83 1 2 3", "help x", "? x"};
        for(const auto &arg:bad) {
            const auto text=run(kind,arg);
            assert(text.find("Usage: "+string(commands[kind]))!=string::npos && row_count(text)==0);
        }
        REMOVE_BIT(staff.specials.act,PLR_PAGING_ON);
        assert(run(kind,"?").find("Usage:")!=string::npos);
        assert(run(kind,"help").find("Usage:")!=string::npos);
        assert(run(kind,"1").find(string(commands[kind])+": use toggle paging")!=string::npos);
        SET_BIT(staff.specials.act,ACT_ISNPC);
        assert(run(kind,"?").empty() && run(kind,"0 100000").empty());
        fixture(1);
        world=nullptr; top_of_world=-1;
        shown(kind,"0 0",kind?1:0);
        top_of_mobt=top_of_objt=-1;
        shown(kind,"0 2147483647",0);
        mob_index=obj_index=nullptr;
        shown(kind,"0 2147483647",0);
        top_of_zone_table=-1;
        assert(run(kind,"zone 83").find("unknown public zone")!=string::npos);
        zone_table=nullptr;
        assert(run(kind,"zone 83").find("unknown public zone")!=string::npos);
        assert(run(kind,"").find("no valid current zone")!=string::npos);
    }
    fixture(2);
    rooms[2].number=rooms[1].number; // Existing adjacent room VNUM overlap remains listable.
    shown(0,"100000",2);
    // Follow the actual continuation advice: every duplicate must appear once,
    // including groups larger than a page and the largest representable VNUM.
    for(int matches : {501,1201}) for(int duplicate_start : {0,499,500}) {
        fixture(matches,0,0);
        vector<string> names(matches);
        for(int i=0;i<matches;++i) {
            names[i]="room-"+to_string(i)+"!";
            rooms[i].name=names[i].data();
            if(i>=duplicate_start) rooms[i].number=INT_MAX;
        }
        for(const string &scope : {string(""),string("zone 83 ")}) {
            string argument=scope+"0 2147483647";
            int consumed=0;
            for(int page=0;consumed<matches;++page) {
                assert(page<4);
                const string text=run(0,argument);
                const int count=min(500,matches-consumed);
                assert(row_count(text)==count);
                for(int i=0;i<matches;++i)
                    assert((text.find(names[i])!=string::npos)==
                           (i>=consumed && i<consumed+count));
                consumed+=count;
                const string marker="Continue: rlist ";
                const auto next=text.find(marker);
                if(consumed==matches) assert(next==string::npos);
                else {
                    assert(next!=string::npos);
                    const auto start=next+marker.size();
                    argument=text.substr(start,text.find("\r\n",start)-start);
                }
            }
        }
    }
    fixture(2,0,0);
    rooms[1].number=rooms[0].number;
    shown(0,"100000 offset 1",1);
    shown(0,"zone 83 100000 offset 1",1);
    shown(0,"100000 offset 2",0);
    for(const char *arg : {"100000 offset -1","100000 offset x",
                           "100000 offset 2147483648","offset 1",
                           "zone 83 offset 1","100000 offset 1 junk"}) {
        const auto text=run(0,arg);
        assert(text.find("Usage: rlist")!=string::npos && row_count(text)==0);
    }
    for(const char *arg : {"100000 offset 3","100000 offset 2147483647",
                           "100001 offset 1","2147483647 offset 1"}) {
        const auto text=run(0,arg);
        assert(text.find("offset exceeds")!=string::npos && row_count(text)==0);
    }
    for(int kind : {1,2})
        assert(run(kind,"100000 offset 1").find("Usage:")!=string::npos);
    // The same VNUM may straddle two owning zones. Global continuation counts
    // the whole group; zone-specific offsets use only that zone's room bounds.
    fixture(501,0,1);
    for(size_t i=0;i<rooms.size();++i) rooms[i].number=51300+int(i);
    rooms.back().number=rooms[500].number;
    const auto first_page=run(0,"51301 2147483647");
    assert(row_count(first_page)==500);
    assert(first_page.find("rlist 51800 2147483647 offset 1")!=string::npos);
    shown(0,"51800 2147483647 offset 1",1);
    shown(0,"zone 83 51800",1);
    shown(0,"zone 145 51800",1);
    fixture(501);
    rooms[1].sector_type=255; rooms[2].sector_type=1;
    assert(run(0,"100000 100001").find("Unknown")!=string::npos);
    // Raw cap boundaries can fall in the middle of ANSI sequences.
    for(const string &edge : {string(159,'x')+"&+R",string(158,'x')+"&=RW",string(160,'&'),string(""),string("&+Rred\nrow\rrow&n")}) {
        for(auto &r:rooms) r.name=const_cast<char *>(edge.c_str());
        for(auto &r:mobiles) r.desc2=const_cast<char *>(edge.c_str());
        for(auto &r:objects) r.desc2=const_cast<char *>(edge.c_str());
        for(int kind=0;kind<3;++kind) {
            shown(kind,"zone 83",500,true);
            assert(count(output.begin(),output.end(),'\n')==504);
            if(edge.empty()) assert(output.find("(unnamed)")!=string::npos);
        }
    }
    puts("builder list handlers: syntax, scope, limits, traversal, inertness and output OK");
#ifndef __SANITIZE_ADDRESS__
    fixture(700,20000,20000);
    for(int kind=0;kind<3;++kind) {
        profile_timer timer{}; profile_timer_reset(&timer);
        uint64_t fastest=UINT64_MAX,slowest=0;
        for(int repeat=0;repeat<50;++repeat) {
            output.clear(); char argument[]="zone 83";
            profile_timer_start(&timer);
            handlers[kind](&staff,argument,0);
            profile_timer_end(&timer);
            assert(timer.last_valid);
            fastest=min(fastest,timer.last_us); slowest=max(slowest,timer.last_us);
        }
        printf("%s optimized fixture handler (captured output): calls=%llu min/avg/max=%llu/%llu/%llu us\n",
               commands[kind],(unsigned long long)timer.calls,(unsigned long long)fastest,
               (unsigned long long)(timer.total_inside_us/timer.calls),(unsigned long long)slowest);
    }
#endif
}
"""


def main():
    source = (ROOT / "src/cmd/actinf.c").read_text()
    start = source.index("namespace\n{\nconstexpr int BUILDER_LIST_LIMIT")
    commands = source[start:]
    # Instrument only comparisons and loop conditions, without replacing behavior.
    assert commands.count("for (; at != end &&") == 2
    commands = commands.replace("std::lower_bound(", "measured_lower_bound(")
    commands = commands.replace("for (; at != end &&", "for (; at != end && (++probes, true) &&")
    interp = (ROOT / "src/cmd/interp.c").read_text()
    for command in ("rlist", "mlist", "olist"):
        registration = next(line for line in interp.splitlines() if f"CMD_{command.upper()}," in line and "IMMORTAL" in line)
        assert f"do_{command}" in registration
    parts = [PRELUDE,
             extract_function("core/utility.c", "string strip_ansi(const char *str)"),
             extract_function("core/utility.c", "string pad_ansi(const char *str, int length, bool trim_to_length)"),
             extract_function("interp.c", "bool is_abbrev(const char *arg1, const char *arg2)"),
             extract_function("db.c", "int real_zone(const int virt)"), commands, DRIVER]
    with tempfile.TemporaryDirectory(prefix="builder-lists-") as temporary:
        test, binary = Path(temporary) / "test.cpp", Path(temporary) / "test"
        test.write_text("\n".join(parts))
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc", str(test), str(HARNESS_STUBS),
                        "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)
        # Separate optimized timing from sanitizer qualification. The production
        # profile timer surrounds only the real handler, not fixture snapshots.
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O2",
                        "-Isrc", str(test), str(HARNESS_STUBS), "-o", str(binary)],
                       cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
