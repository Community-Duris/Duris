#!/usr/bin/env python3
"""Prove journal append rollback versus admission-uncertain classification."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel


HARNESS = r'''
#include "persistence/critical_command_journal.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

static int write_fault = 0;
static int fsync_fault = 0;
static int close_fault = 0;

extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_fsync(int);
extern "C" int __real_close(int);

extern "C" ssize_t __wrap_write(int fd, const void *data, size_t size)
{
    const int flags = fcntl(fd, F_GETFL);
    if (flags >= 0 && (flags & O_APPEND) && write_fault)
    {
        const int fault = write_fault;
        write_fault = 0;
        if (fault == 1)
        {
            errno = ENOSPC;
            return -1;
        }
        assert(fault == 2);
        assert(size > 1);
        const ssize_t partial = __real_write(fd, data, size / 2);
        assert(partial > 0);
        errno = EIO;
        return -1;
    }
    return __real_write(fd, data, size);
}

extern "C" int __wrap_fsync(int fd)
{
    if (fsync_fault)
    {
        --fsync_fault;
        errno = EIO;
        return -1;
    }
    return __real_fsync(fd);
}

extern "C" int __wrap_close(int fd)
{
    if (close_fault)
    {
        --close_fault;
        const int result = __real_close(fd);
        assert(result == 0);
        errno = EIO;
        return -1;
    }
    return __real_close(fd);
}

static critical_command make_command(uint8_t tag)
{
    critical_command command = {};
    command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
    assert(critical_operation_id_generate(&command.operation_id));
    command.type = critical_command_type::test;
    command.payload_version = 1;
    command.source_site = critical_source_site::command;
    command.deadline_class = critical_deadline_class::interactive;
    command.accepted_at_usec = 1700000000000000ULL + tag;
    command.keys = {{critical_entity_type::player, static_cast<uint64_t>(tag) + 1}};
    command.payload = {tag};
    assert(critical_command_normalize(&command));
    return command;
}

static bool collect(critical_command command, void *raw)
{
    static_cast<std::vector<critical_command> *>(raw)->push_back(std::move(command));
    return true;
}

static std::string journal_path(const char *directory)
{
    return std::string(directory) + "/critical-command.journal";
}

static void reset_journal(const char *directory)
{
    critical_command_journal_shutdown();
    critical_command_journal_reset_for_tests();
    std::filesystem::remove_all(directory);
    assert(std::filesystem::create_directory(directory));
    assert(chmod(directory, 0700) == 0);
    assert(critical_command_journal_init(directory));
}

static void assert_only_baseline(const char *directory)
{
    std::vector<critical_command> replayed;
    assert(critical_command_journal_replay(collect, &replayed) ==
           critical_command_journal_result::ok);
    assert(replayed.size() == 1 && replayed[0].payload[0] == 1);
    (void)directory;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *directory = argv[1];
    reset_journal(directory);
    const auto baseline = make_command(1);
    assert(critical_command_journal_append(baseline) == critical_command_journal_result::ok);
    const auto baseline_size = std::filesystem::file_size(journal_path(directory));

    write_fault = 1;
    assert(critical_command_journal_append(make_command(2)) ==
           critical_command_journal_result::io_failure);
    assert(std::filesystem::file_size(journal_path(directory)) == baseline_size);
    assert_only_baseline(directory);

    write_fault = 2;
    assert(critical_command_journal_append(make_command(3)) ==
           critical_command_journal_result::io_failure);
    assert(std::filesystem::file_size(journal_path(directory)) == baseline_size);
    assert_only_baseline(directory);

    fsync_fault = 1;
    assert(critical_command_journal_append(make_command(4)) ==
           critical_command_journal_result::io_failure);
    assert(std::filesystem::file_size(journal_path(directory)) == baseline_size);
    assert_only_baseline(directory);

    close_fault = 1;
    assert(critical_command_journal_append(make_command(5)) ==
           critical_command_journal_result::io_failure);
    assert(std::filesystem::file_size(journal_path(directory)) == baseline_size);
    assert_only_baseline(directory);

    close_fault = 2;
    assert(critical_command_journal_append(make_command(6)) ==
           critical_command_journal_result::append_uncertain);
    assert(std::filesystem::file_size(journal_path(directory)) == baseline_size);
    close_fault = 0;
    assert(critical_command_journal_append(make_command(7)) ==
           critical_command_journal_result::append_uncertain);
    assert(critical_command_journal_sync() == critical_command_journal_result::ok);
    assert_only_baseline(directory);
    assert(critical_command_journal_result_name(
               critical_command_journal_result::append_uncertain) ==
           std::string("append_uncertain"));
}
'''


with tempfile.TemporaryDirectory(prefix="duris-journal-faults-") as temporary:
    root = Path(temporary)
    source = root / "journal_faults.cpp"
    binary = root / "journal_faults"
    source.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread", "-Isrc",
            str(source), rel("critical_command.c"), rel("critical_command_journal.c"),
            "-lz", "-lcrypto", "-Wl,--wrap=write", "-Wl,--wrap=fsync",
            "-Wl,--wrap=close", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(root / "journal")], check=True, timeout=20)

print("critical command journal append fault checks passed")
