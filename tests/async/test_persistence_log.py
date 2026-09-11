#!/usr/bin/env python3
"""Exercise the real reporter and worker against disposable files and injected I/O faults."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
utility = (ROOT / 'src/core/utility.c').read_text()
header = (ROOT / 'src/core/prototypes.h').read_text()
severity = re.search(r'enum class persistence_severity\s*\{.*?\};', header, re.S).group()
reporter = utility[utility.index('void persistence_log_poll()'):
                   utility.index('unsigned long long persistence_next_item_uid(')]
harness = r'''
#include "persistence/persistence_log.h"
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#define AVATAR 57
#define checked_snprintf snprintf
std::vector<std::string> broadcasts;
void wizlog(int level, const char *format, ...) {
    assert(level == AVATAR);
    char text[4096]; va_list args; va_start(args, format);
    vsnprintf(text, sizeof(text), format, args); va_end(args);
    broadcasts.emplace_back(text);
}
std::atomic<bool> block_write{false}, entered{false};
std::atomic<int> write_fault{0};
std::atomic<bool> close_fault{false};
extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_close(int);
extern "C" ssize_t __wrap_write(int fd, const void *data, size_t size) {
    assert(fcntl(fd, F_GETFD) & FD_CLOEXEC);
    assert(fcntl(fd, F_GETFL) & O_APPEND);
    if (block_write.load()) {
        entered = true;
        while (block_write.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const int fault = write_fault.exchange(0);
    if (fault == 1) { errno = ENOSPC; return -1; }
    if (fault == 2) return __real_write(fd, data, size / 2);
    return __real_write(fd, data, size);
}
extern "C" int __wrap_close(int fd) {
    const int result = __real_close(fd);
    if (close_fault.exchange(false)) { errno = EIO; return -1; }
    return result;
}
''' + severity + '\n' + reporter + r'''
using clock_type = std::chrono::steady_clock;
void wait_entered() {
    const auto deadline = clock_type::now() + std::chrono::seconds(3);
    while (!entered.load() && clock_type::now() < deadline) std::this_thread::yield();
    assert(entered.load());
}
void submit(const char *text) {
    const auto deadline = clock_type::now() + std::chrono::seconds(3);
    while (!persistence_log_submit(text)) {
        assert(clock_type::now() < deadline);
        std::this_thread::yield();
    }
}
std::string read(const std::string &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
size_t occurrences(const std::string &text, const std::string &needle) {
    size_t count = 0, pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) { ++count; pos += needle.size(); }
    return count;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string root = argv[1], file = root + "/nested/file", wiz = root + "/nested/wiz";
    assert(!persistence_log_submit("before-start"));
    assert(persistence_log_snapshot().rejected == 1);
    assert(persistence_log_start(file.c_str(), wiz.c_str()));
    assert(!persistence_log_submit(nullptr));
    assert(!persistence_log_submit(""));
    const std::string oversized(PERSISTENCE_LOG_RECORD_BYTES, 'x');
    assert(!persistence_log_submit(oversized.c_str()));
    for (auto value : {persistence_severity::ok, persistence_severity::info, persistence_severity::alert}) {
        persistence_report(value, AVATAR, "player_save", "private", "private", "private",
                           "death_recovery", "retry=%d", 1);
        assert(persistence_log_drain(3000));
    }
    assert(broadcasts.size() == 1);
    assert(read(file) == read(wiz));
    assert(occurrences(read(file), "PERSISTENCE:") == 3);
    assert(read(file).find("private") == std::string::npos);
    assert(read(file).find("outcome=ok detail=retry=1") != std::string::npos);
    assert(read(file).find("outcome=info") != std::string::npos);
    assert(read(file).find("outcome=alert") != std::string::npos);

    // Rotate while an append is already open: it belongs to the old inode once,
    // and the following record goes to the newly created file.
    block_write = true; entered = false;
    submit("rotation-in-flight"); wait_entered();
    std::filesystem::rename(file, file + ".1");
    block_write = false;
    assert(persistence_log_drain(3000));
    submit("rotation-after"); assert(persistence_log_drain(3000));
    assert(occurrences(read(file + ".1"), "rotation-in-flight") == 1);
    assert(read(file).find("rotation-in-flight") == std::string::npos);
    assert(occurrences(read(file), "rotation-after") == 1);
    assert(occurrences(read(wiz), "rotation-in-flight") == 1);

    // Open failure in one sink must not suppress the other sink or recurse.
    auto before = persistence_log_snapshot();
    std::filesystem::remove(file); std::filesystem::create_directory(file);
    submit("open-failure"); assert(persistence_log_drain(3000));
    assert(persistence_log_snapshot().file_failures == before.file_failures + 1);
    assert(persistence_log_snapshot().wiz_failures == before.wiz_failures);
    assert(occurrences(read(wiz), "open-failure") == 1);
    std::filesystem::remove(file);
    assert(mkfifo(file.c_str(), 0600) == 0);
    submit("fifo-failure"); assert(persistence_log_drain(3000));
    assert(persistence_log_snapshot().file_failures == before.file_failures + 2);
    std::filesystem::remove(file);

    // Full disk, short write, and close failure: count once, do not retry an
    // ambiguous append, and still attempt the second sink exactly once.
    for (int fault : {1, 2, 3}) {
        before = persistence_log_snapshot();
        if (fault == 3) close_fault = true; else write_fault = fault;
        submit("write-failure"); assert(persistence_log_drain(3000));
        assert(persistence_log_snapshot().file_failures == before.file_failures + 1);
        assert(persistence_log_snapshot().wiz_failures == before.wiz_failures);
    }
    assert(occurrences(read(wiz), "write-failure") == 3);
    assert(occurrences(read(file), "write-failure") == 1); // only the close failure fully wrote

    // Symmetric failure accounting for the wizard file.
    before = persistence_log_snapshot();
    std::filesystem::rename(wiz, wiz + ".1"); std::filesystem::create_directory(wiz);
    submit("wiz-open-failure"); assert(persistence_log_drain(3000));
    assert(persistence_log_snapshot().wiz_failures == before.wiz_failures + 1);
    assert(persistence_log_snapshot().file_failures == before.file_failures);
    assert(occurrences(read(file), "wiz-open-failure") == 1);
    std::filesystem::remove(wiz);

    // Delivery notices are on the game thread, rate limited, and not recursive.
    before = persistence_log_snapshot(); broadcasts.clear();
    persistence_log_poll(); persistence_log_poll();
    assert(broadcasts.size() == 1);
    assert(broadcasts[0].find("action=log_delivery") != std::string::npos);
    assert(persistence_log_snapshot().accepted == before.accepted);

    block_write = true; entered = false;
    submit("blocked-writer"); wait_entered();
    for (size_t n = 0; n < PERSISTENCE_LOG_CAPACITY; ++n) assert(persistence_log_submit("queued"));
    assert(!persistence_log_submit("overflow"));
    broadcasts.clear(); before = persistence_log_snapshot();
    auto begin = clock_type::now();
    for (int n = 0; n < 20000; ++n)
        persistence_report(persistence_severity::info, AVATAR, "player_save", "private", "private",
                           "private", "death_recovery_wait", "retry=%d", n);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock_type::now() - begin).count();
    assert(elapsed < 1000); // catches waiting for the blocked sink, not a microbenchmark SLA
    assert(broadcasts.empty());
    assert(persistence_log_snapshot().accepted == before.accepted);
    assert(persistence_log_snapshot().rejected == before.rejected + 20000);
    persistence_alert(AVATAR, "player_save", "private", "private", "private", "terminal_save_failed", "retry=%d", 1);
    assert(broadcasts.size() == 1);
    assert(broadcasts[0].find("terminal_save_failed") != std::string::npos);
    begin = clock_type::now(); assert(!persistence_log_drain(25));
    assert(clock_type::now() - begin < std::chrono::milliseconds(500));
    block_write = false; assert(persistence_log_drain(3000));
    auto after = persistence_log_snapshot();
    assert(after.accepted == after.completed);
    submit("after-drain"); assert(persistence_log_drain(3000));
    assert(occurrences(read(wiz), "after-drain") == 1);
    std::cout << "[PASS] real reporter/worker routing, rotation, open/write/close faults, bounded queue, "
              << "alert delivery and drain timeout; 20000 blocked-sink reports in " << elapsed << " ms\n";
}
'''
with tempfile.TemporaryDirectory(prefix='persistence-log-') as temp:
    source, binary = Path(temp) / 'harness.cpp', Path(temp) / 'harness'
    source.write_text(harness)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-pthread',
                    '-I', str(ROOT / 'src'), str(source),
                    str(ROOT / 'src/persistence/persistence_log.c'),
                    '-Wl,--wrap=write', '-Wl,--wrap=close', '-o', str(binary)], check=True)
    subprocess.run([str(binary), temp], check=True, timeout=20)
