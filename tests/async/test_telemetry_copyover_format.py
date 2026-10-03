#!/usr/bin/env python3
"""Exercise the actual #265 copyover wire helpers with tmpfile round trips.

The runtime API is an observation seam here; its real implementation has its
own lifecycle test. This is not a full server exec/recovery journey.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    match = re.search(r'\s+'.join(re.escape(word) for word in signature.split()), source)
    assert match, signature
    start = match.start()
    cursor = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == '{') - (source[cursor] == '}')
        cursor += 1
    return source[start:cursor]


def main():
    source = (ROOT / 'src/persistence/copyover.c').read_text()
    recover = source[source.index('int copyover_recover('):]
    assert recover.index('copyover_codec_read(') < recover.index('restore_telemetry_copyover_sessions(')
    bodies = '\n'.join(function(source, signature) for signature in (
        'static bool copyover_descriptor_is_eligible(',
        'static bool write_telemetry_copyover_state(',
        'static void restore_telemetry_copyover_sessions(',
    ))
    prelude = r'''
#include "persistence/copyover_codec.h"
#include "net/transport.h"
#include "core/utils.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <new>
#include <cstdlib>
#include <sys/select.h>
bool fail_next_allocation = false;
void *operator new(std::size_t size) {
    if (fail_next_allocation) { fail_next_allocation = false; throw std::bad_alloc(); }
    if (void *p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
P_desc descriptor_list = nullptr;
void logit(const char *, const char *, ...) {}
telemetry_runtime_outcome copy_outcome = telemetry_runtime_outcome::accepted;
telemetry_runtime_outcome flush_outcome = telemetry_runtime_outcome::accepted;
const char *absent_player_name = nullptr;
unsigned flush_calls = 0, captures = 0;
bool telemetry_runtime_now(telemetry_monotonic_usec *m, telemetry_utc_usec *u) noexcept {
    *m = 1000; *u = 2000; return true;
}
telemetry_runtime_outcome telemetry_runtime_flush_for_copyover(telemetry_monotonic_usec) {
    ++flush_calls; assert(captures == 2); return flush_outcome;
}
telemetry_handoff_result telemetry_runtime_game_handoff_copy(char_data *ch) {
    telemetry_handoff_result result{};
    ++captures;
    result.outcome = copy_outcome;
    if (absent_player_name != nullptr && std::strcmp(GET_NAME(ch), absent_player_name) == 0)
        result.outcome = telemetry_runtime_outcome::queue_full;
    result.handoff.session.id.session_seq = std::strcmp(GET_NAME(ch), "alpha") == 0 ? 11 : 22;
    result.handoff.session.pid = 42;
    return result;
}
struct observation { P_desc descriptor; std::uint64_t sequence; };
std::vector<observation> observations;
bool resume_full_before_admission = false;
bool resume_full_after_admission = false;
telemetry_capture_result telemetry_runtime_game_session_resume(
    char_data *, descriptor_data *d, const telemetry_session_handoff *handoff) {
    if (handoff && handoff->session.pid == -1) {
        telemetry_capture_result result{};
        result.outcome = telemetry_runtime_outcome::invalid;
        return result;
    }
    if (handoff && resume_full_before_admission) {
        telemetry_capture_result result{};
        result.outcome = telemetry_runtime_outcome::queue_full;
        return result;
    }
    observations.push_back({d, handoff ? handoff->session.id.session_seq : 0});
    if (handoff && resume_full_after_admission) {
        d->telemetry_connection_sequence = 1;
        telemetry_capture_result result{};
        result.outcome = telemetry_runtime_outcome::queue_full;
        return result;
    }
    telemetry_capture_result result{};
    result.outcome = telemetry_runtime_outcome::accepted;
    return result;
}
'''
    checks = r'''
char_data alpha{}, beta{};
descriptor_data a{}, b{}, ws{}, tls{}, menu{};
void setup() {
    alpha.player.name = const_cast<char *>("alpha");
    beta.player.name = const_cast<char *>("beta");
    a = {}; b = {}; ws = {}; tls = {}; menu = {};
    a.descriptor = 10; b.descriptor = 11; ws.descriptor = 12;
    tls.descriptor = 13; menu.descriptor = 14;
    a.connected = b.connected = ws.connected = tls.connected = CON_PLAYING;
    menu.connected = CON_MAIN_MENU;
    a.character = ws.character = tls.character = menu.character = &alpha;
    b.character = &beta; ws.websocket = true;
    tls.sslses = reinterpret_cast<gnutls_session_t>(&alpha);
    a.next = &b; b.next = &ws; ws.next = &tls; tls.next = &menu;
    descriptor_list = &a; observations.clear();
    resume_full_before_admission = resume_full_after_admission = false;
    copy_outcome = flush_outcome = telemetry_runtime_outcome::accepted;
    absent_player_name = nullptr; flush_calls = captures = 0;
}
std::vector<telemetry_copyover_entry> saved(bool allocation_failure = false) {
    FILE *file = tmpfile(); assert(file);
    copyover_header header{}; header.num_descriptors = 2; header.num_rooms = 1;
    const int listeners[3] = {-1,-1,-1};
    assert(copyover_codec_begin(file, header, listeners));
    for (P_desc d : {&a, &b}) {
        copyover_desc entry{}; entry.fd = d->descriptor;
        strlcpy(entry.player_name, GET_NAME(d->character), sizeof(entry.player_name));
        assert(copyover_codec_write(file, entry));
    }
    fail_next_allocation = allocation_failure;
    assert(write_telemetry_copyover_state(file, 2));
    copyover_room door{200,9,123};
    assert(copyover_codec_write(file, door));
    assert(copyover_codec_finish(file, nullptr));
    copyover_decoded_state state;
    assert(copyover_codec_read(file, &state, nullptr));
    assert(state.doors.size() == 1 && state.doors[0].state == 123 && fgetc(file) == EOF);
    fclose(file); return state.telemetry;
}
void exactly_once(std::uint64_t seq_a, std::uint64_t seq_b) {
    assert(observations.size() == 2);
    assert(observations[0].descriptor == &a && observations[0].sequence == seq_a);
    assert(observations[1].descriptor == &b && observations[1].sequence == seq_b);
}
int main() {
    setup(); auto entries = saved();
    assert(entries.size() == 2 && entries[0].handoff.session.id.session_seq == 11);
    assert(flush_calls == 1 && captures == 2);
    b.next = nullptr; restore_telemetry_copyover_sessions(&entries); exactly_once(11,22);
    setup(); absent_player_name = "beta"; entries = saved();
    assert(entries[0].handoff_valid && !entries[1].handoff_valid);
    b.next = nullptr; restore_telemetry_copyover_sessions(&entries); exactly_once(11,0);
    for (auto outcome : {telemetry_runtime_outcome::disabled,
         telemetry_runtime_outcome::flatfile_disabled, telemetry_runtime_outcome::not_initialized,
         telemetry_runtime_outcome::invalid, telemetry_runtime_outcome::queue_full}) {
        setup(); copy_outcome = outcome; entries = saved();
        assert(entries.size() == 2 && !entries[0].handoff_valid);
        b.next = nullptr; restore_telemetry_copyover_sessions(&entries); exactly_once(0,0);
    }
    setup(); entries = saved(true);
    assert(!fail_next_allocation && !captures && !flush_calls);
    b.next = nullptr; restore_telemetry_copyover_sessions(&entries); exactly_once(0,0);
    setup(); flush_outcome = telemetry_runtime_outcome::queue_full; entries = saved();
    assert(flush_calls == 1 && !entries[0].handoff_valid && !entries[1].handoff_valid);
    b.next = nullptr; restore_telemetry_copyover_sessions(&entries); exactly_once(0,0);
    setup(); b.next = nullptr; restore_telemetry_copyover_sessions(nullptr); exactly_once(0,0);
    for (unsigned kind = 0; kind < 3; ++kind) {
        setup(); entries = saved();
        if (kind == 0) strlcpy(entries[0].player_name, "unmatched", sizeof(entries[0].player_name));
        if (kind == 1) { entries[1].fd = 10; strlcpy(entries[1].player_name, "alpha", sizeof(entries[1].player_name)); }
        if (kind == 2) entries[0].handoff.session.pid = -1;
        b.next = nullptr; restore_telemetry_copyover_sessions(&entries);
        exactly_once(0, kind == 1 ? 0 : 22);
    }
    for (bool after : {false,true}) {
        setup(); entries = saved();
        resume_full_before_admission = !after; resume_full_after_admission = after;
        b.next = nullptr; restore_telemetry_copyover_sessions(&entries);
        exactly_once(after ? 11 : 0, after ? 22 : 0);
    }
    setup(); FILE *file = tmpfile(); assert(file);
    assert(!write_telemetry_copyover_state(file, -1));
    assert(!write_telemetry_copyover_state(file, FD_SETSIZE + 1));
    assert(!write_telemetry_copyover_state(file, 1)); fclose(file);
    puts("telemetry portable round trip, durability/allocation absence, identity matching and exactly-once resume passed");
}
'''
    with tempfile.TemporaryDirectory(prefix='telemetry-copyover-wire-') as directory:
        cpp, binary = Path(directory) / 'wire.cc', Path(directory) / 'wire'
        cpp.write_text(prelude + bodies + checks)
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc',
                        '-ffunction-sections', '-fdata-sections', str(cpp),
                        'src/persistence/copyover_codec.c', 'src/world/world_recovery_codec.c',
                        'src/world/generated_npc_state.c', 'src/player/pet_restore_state.c',
                        'src/item/item_transfer_command.c', '-Wl,--gc-sections', '-lbsd',
                        '-o', str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
