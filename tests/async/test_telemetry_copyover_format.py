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
    header = (ROOT / 'src/persistence/copyover.h').read_text()
    assert '#define COPYOVER_VERSION 16' in header
    recover = source[source.index('int copyover_recover('):]
    assert 'copyover_version_supported(header.version)' in recover
    assert 'version >= 12 && version <= COPYOVER_VERSION' in source
    assert 'header.version >= 15' in recover
    assert recover.index('read_telemetry_copyover_state(') < recover.index('restore_telemetry_copyover_sessions(')
    wire_types = source[source.index('constexpr char TELEMETRY_COPYOVER_MAGIC'):source.index('} // namespace', source.index('constexpr char TELEMETRY_COPYOVER_MAGIC'))]
    signatures = (
        'static bool copyover_descriptor_is_eligible(',
        'static bool write_telemetry_copyover_state(',
        'static bool read_telemetry_copyover_state(',
        'static void restore_telemetry_copyover_sessions(',
    )
    bodies = '\n'.join(function(source, s) for s in signatures)
    prelude = r'''
#include "telemetry/telemetry_runtime.h"
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
struct char_data { const char *name; };
struct descriptor_data {
    int descriptor;
    int connected;
    char_data *character;
    bool websocket;
    void *sslses;
    descriptor_data *next;
    std::uint64_t telemetry_connection_sequence = 0;
};
using P_desc = descriptor_data *;
using P_char = char_data *;
constexpr int CON_PLAYING = 0, LOG_STATUS = 1;
P_desc descriptor_list = nullptr;
#define GET_NAME(ch) ((ch)->name)
void logit(int, const char *, ...) {}
std::size_t strlcpy(char *dest, const char *src, std::size_t size) {
    if (size) std::snprintf(dest, size, "%s", src);
    return std::strlen(src);
}
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
    if (absent_player_name != nullptr && std::strcmp(ch->name, absent_player_name) == 0)
        result.outcome = telemetry_runtime_outcome::queue_full;
    result.handoff.session.id.session_seq = std::strcmp(ch->name, "alpha") == 0 ? 11 : 22;
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
constexpr std::uint32_t following_world_marker = 0xa1b2c3d4U;
char_data alpha{"alpha"}, beta{"beta"};
descriptor_data a{10, 0, &alpha, false, nullptr, nullptr};
descriptor_data b{11, 0, &beta, false, nullptr, nullptr};
descriptor_data ws{12, 0, &alpha, true, nullptr, nullptr};
descriptor_data tls{13, 0, &alpha, false, &alpha, nullptr};
descriptor_data menu{14, 1, &alpha, false, nullptr, nullptr};
void setup() {
    a.next = &b; b.next = &ws; ws.next = &tls; tls.next = &menu;
    descriptor_list = &a;
    observations.clear();
    a.telemetry_connection_sequence = b.telemetry_connection_sequence = 0;
    resume_full_before_admission = resume_full_after_admission = false;
    copy_outcome = telemetry_runtime_outcome::accepted;
    flush_outcome = telemetry_runtime_outcome::accepted;
    absent_player_name = nullptr;
    flush_calls = captures = 0;
}
FILE *saved() {
    FILE *fp = tmpfile(); assert(fp);
    assert(write_telemetry_copyover_state(fp, 2));
    assert(ftell(fp) == static_cast<long>(sizeof(telemetry_copyover_header) + 2 * sizeof(telemetry_copyover_entry)));
    assert(fwrite(&following_world_marker, sizeof(following_world_marker), 1, fp) == 1);
    rewind(fp);
    return fp;
}
void marker(FILE *fp) {
    std::uint32_t next = 0;
    assert(fread(&next, sizeof(next), 1, fp) == 1);
    assert(next == following_world_marker);
}
void mutate(FILE *fp, unsigned index, unsigned kind) {
    const long offset = sizeof(telemetry_copyover_header) + index * sizeof(telemetry_copyover_entry);
    assert(fseek(fp, offset, SEEK_SET) == 0);
    telemetry_copyover_entry entry{};
    assert(fread(&entry, sizeof(entry), 1, fp) == 1);
    if (kind == 0) entry.handoff_valid = 2;
    if (kind == 1) strlcpy(entry.player_name, "not-the-loaded-player", sizeof(entry.player_name));
    if (kind == 2) { entry.fd = a.descriptor; strlcpy(entry.player_name, "alpha", sizeof(entry.player_name)); }
    if (kind == 3) entry.handoff.session.pid = -1;
    if (kind == 4) entry.player_name[sizeof(entry.player_name)-1] = 'x';
    if (kind == 5) entry.reserved[1] = 1;
    if (kind == 6) entry.fd = -1;
    assert(fseek(fp, offset, SEEK_SET) == 0);
    assert(fwrite(&entry, sizeof(entry), 1, fp) == 1);
    rewind(fp);
}
void exactly_once(std::uint64_t seq_a, std::uint64_t seq_b) {
    assert(observations.size() == 2);
    assert(observations[0].descriptor == &a && observations[0].sequence == seq_a);
    assert(observations[1].descriptor == &b && observations[1].sequence == seq_b);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    setup();
    const char *scenario = argv[1];
    if (std::strcmp(scenario, "roundtrip") == 0) {
        FILE *fp = saved();
        std::vector<telemetry_copyover_entry> entries;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        assert(entries.size() == 2);
        assert(flush_calls == 1 && captures == 2);
        const auto expected = telemetry_runtime_game_handoff_copy(&alpha).handoff;
        assert(std::memcmp(&entries[0].handoff, &expected, sizeof(expected)) == 0);
        marker(fp); fclose(fp);
        descriptor_list = &a; b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(11, 22);
    } else if (std::strcmp(scenario, "mixed-handoff") == 0) {
        absent_player_name = "beta";
        FILE *fp = saved();
        std::vector<telemetry_copyover_entry> entries;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        assert(entries.size() == 2);
        assert(entries[0].fd == a.descriptor && std::strcmp(entries[0].player_name, "alpha") == 0);
        assert(entries[0].handoff_valid == 1);
        assert(entries[1].fd == b.descriptor && std::strcmp(entries[1].player_name, "beta") == 0);
        assert(entries[1].handoff_valid == 0);
        marker(fp); fclose(fp);
        b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(11, 0);
    } else if (std::strcmp(scenario, "resume-full-before") == 0 ||
               std::strcmp(scenario, "resume-full-after") == 0) {
        FILE *fp = saved();
        std::vector<telemetry_copyover_entry> entries;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        marker(fp); fclose(fp);
        resume_full_before_admission = std::strcmp(scenario, "resume-full-before") == 0;
        resume_full_after_admission = !resume_full_before_admission;
        b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(resume_full_before_admission ? 0 : 11, resume_full_before_admission ? 0 : 22);
    } else if (std::strcmp(scenario, "unavailable") == 0) {
        for (auto outcome : {telemetry_runtime_outcome::disabled,
             telemetry_runtime_outcome::flatfile_disabled,
             telemetry_runtime_outcome::not_initialized,
             telemetry_runtime_outcome::invalid,
             telemetry_runtime_outcome::queue_full}) {
            setup(); copy_outcome = outcome;
            FILE *fp = saved();
            std::vector<telemetry_copyover_entry> entries;
            assert(read_telemetry_copyover_state(fp, 2, &entries));
            assert(entries.size() == 2 && entries[0].handoff_valid == 0);
            marker(fp); fclose(fp);
            b.next = nullptr;
            restore_telemetry_copyover_sessions(&entries);
            exactly_once(0, 0);
        }
    } else if (std::strcmp(scenario, "write-memory-pressure") == 0) {
        fail_next_allocation = true;
        FILE *fp = saved();
        assert(!fail_next_allocation && captures == 0 && flush_calls == 0);
        std::vector<telemetry_copyover_entry> entries;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        assert(entries.size() == 2 && entries[0].handoff_valid == 0 && entries[1].handoff_valid == 0);
        marker(fp); fclose(fp);
        b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(0, 0);
    } else if (std::strcmp(scenario, "undurable") == 0) {
        flush_outcome = telemetry_runtime_outcome::queue_full;
        FILE *fp = saved();
        std::vector<telemetry_copyover_entry> entries;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        assert(entries.size() == 2 && entries[0].handoff_valid == 0 && entries[1].handoff_valid == 0);
        assert(flush_calls == 1);
        marker(fp); fclose(fp);
        b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(0, 0);
    } else if (std::strcmp(scenario, "legacy") == 0) {
        b.next = nullptr;
        restore_telemetry_copyover_sessions(nullptr);
        exactly_once(0, 0);
    } else if (std::strcmp(scenario, "damaged-entry") == 0) {
        for (unsigned kind : {0U, 4U, 5U, 6U}) {
            setup(); FILE *fp = saved(); mutate(fp, 0, kind);
            std::vector<telemetry_copyover_entry> entries;
            assert(read_telemetry_copyover_state(fp, 2, &entries));
            marker(fp); fclose(fp);
            b.next = nullptr;
            restore_telemetry_copyover_sessions(&entries);
            exactly_once(0, 22);
        }
    } else if (std::strcmp(scenario, "unmatched-entry") == 0 ||
               std::strcmp(scenario, "duplicate-entry") == 0 ||
               std::strcmp(scenario, "invalid-handoff") == 0) {
        FILE *fp = saved();
        const bool duplicate = std::strcmp(scenario, "duplicate-entry") == 0;
        const bool invalid = std::strcmp(scenario, "invalid-handoff") == 0;
        mutate(fp, duplicate ? 1 : 0, duplicate ? 2 : invalid ? 3 : 1);
        std::vector<telemetry_copyover_entry> entries;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        marker(fp); fclose(fp);
        b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(0, duplicate ? 0 : 22);
    } else if (std::strcmp(scenario, "memory-pressure") == 0) {
        FILE *fp = saved();
        std::vector<telemetry_copyover_entry> entries;
        fail_next_allocation = true;
        assert(read_telemetry_copyover_state(fp, 2, &entries));
        assert(!fail_next_allocation && entries.empty());
        marker(fp); fclose(fp);
        b.next = nullptr;
        restore_telemetry_copyover_sessions(&entries);
        exactly_once(0, 0);
    } else if (std::strcmp(scenario, "bounds-and-framing") == 0) {
        FILE *fp = tmpfile(); assert(fp);
        telemetry_copyover_header header{};
        std::memcpy(header.magic, TELEMETRY_COPYOVER_MAGIC, 4);
        header.version = TELEMETRY_COPYOVER_VERSION;
        header.count = 0x7fffffff;
        assert(fwrite(&header, sizeof(header), 1, fp) == 1); rewind(fp);
        std::vector<telemetry_copyover_entry> entries;
        assert(!read_telemetry_copyover_state(fp, 0x7fffffff, &entries));
        assert(entries.empty()); fclose(fp);
        fp = saved();
        assert(!read_telemetry_copyover_state(fp, 1, &entries)); fclose(fp);
        fp = tmpfile(); assert(fp);
        header.count = 2;
        assert(fwrite(&header, sizeof(header), 1, fp) == 1); rewind(fp);
        assert(!read_telemetry_copyover_state(fp, 2, &entries)); fclose(fp);
    } else { assert(false); }
    std::puts(scenario);
}
'''
    with tempfile.TemporaryDirectory(prefix='telemetry-copyover-wire-') as directory:
        cpp, binary = Path(directory) / 'wire.cc', Path(directory) / 'wire'
        cpp.write_text(prelude + wire_types + bodies + checks)
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        '-I', str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
        # Bound the malicious-count probe even before the fix, so red cannot OOM.
        import resource
        def limits():
            ceiling = 256 * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (ceiling, ceiling))
            resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        failed = []
        for scenario in ('roundtrip', 'unavailable', 'legacy', 'damaged-entry',
                         'mixed-handoff', 'unmatched-entry', 'duplicate-entry', 'invalid-handoff',
                         'memory-pressure', 'bounds-and-framing',
                         'resume-full-before', 'resume-full-after', 'undurable',
                         'write-memory-pressure'):
            result = subprocess.run([str(binary), scenario], text=True, capture_output=True,
                                    timeout=10, preexec_fn=limits)
            print(('PASS ' if result.returncode == 0 else 'FAIL ') + scenario)
            if result.returncode:
                print(result.stderr.strip())
                failed.append(scenario)
        assert not failed, failed
    print('telemetry actual copyover serialization and recovery seams passed')


if __name__ == '__main__':
    main()
