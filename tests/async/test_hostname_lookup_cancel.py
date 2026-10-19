#!/usr/bin/env python3
"""A reverse-DNS lookup that answers after its connection closed writes nothing.

Each connection's lookup runs on its own thread and writes lib/etc/hosts/<descriptor>.<address>
through a temporary file. close_socket() removed the descriptor's files, but a lookup still
running then wrote its file afterwards, and it stayed. forget_hostname() now marks the
descriptor's running lookups cancelled under the lookups' mutex, and a lookup publishes only
under that mutex and only when not cancelled. The close also removes a temporary file that a
copyover cut short. Runs the real comm.c code under ThreadSanitizer, with getnameinfo() held
until the test lets it answer.
"""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function, source

comm = source("comm.c").read_text()
remove = extract_function("comm.c", "static void remove_hostname_files(const char *prefix)")
lookups = comm[comm.index("struct hostname_lookup_request\n{"):
               comm.index("\nint new_descriptor(")]

HARNESS = r'''
#include <arpa/inet.h>
#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <mutex>
#include <netdb.h>
#include <pthread.h>
#include <set>
#include <string>
#include <strings.h>
#include <unistd.h>

static std::mutex dns_mutex;
static std::condition_variable dns_wake;
static bool dns_may_answer = false;

static int held_getnameinfo(const struct sockaddr *, socklen_t, char *host, socklen_t size,
                            char *, socklen_t, int)
{
    std::unique_lock<std::mutex> lock(dns_mutex);
    dns_wake.wait(lock, [] { return dns_may_answer; });
    dns_may_answer = false;
    snprintf(host, size, "client.example");
    return 0;
}
#define getnameinfo held_getnameinfo
''' + remove + "\n" + lookups + r'''
#undef getnameinfo

static void answer()
{
    std::lock_guard<std::mutex> lock(dns_mutex);
    dns_may_answer = true;
    dns_wake.notify_all();
}

static void wait_for_lookups()
{
    for (;;)
    {
        pthread_mutex_lock(&hostname_lookup_mutex);
        const int running = hostname_lookup_workers;
        pthread_mutex_unlock(&hostname_lookup_mutex);
        if (!running)
            return;
        usleep(1000);
    }
}

static std::set<std::string> files()
{
    std::set<std::string> names;
    DIR *directory = opendir("lib/etc/hosts");
    while (const struct dirent *entry = readdir(directory))
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
            names.insert(entry->d_name);
    closedir(directory);
    return names;
}

static void touch(const char *name)
{
    FILE *file = fopen((std::string("lib/etc/hosts/") + name).c_str(), "w");
    fputs("left.behind.example\n", file);
    fclose(file);
}

int main()
{
    // The connection closes while its lookup is still running: nothing is written.
    resolve_descriptor_hostname_async("192.0.2.7", 7);
    forget_hostname(7);
    answer();
    wait_for_lookups();
    assert(files().empty());

    // A lookup that answers first publishes its file, and the close removes it with a
    // temporary file a copyover cut short; another descriptor's file stays.
    resolve_descriptor_hostname_async("192.0.2.7", 7);
    answer();
    wait_for_lookups();
    assert(files() == std::set<std::string>{"7.192.0.2.7"});
    touch(".7.192.0.2.7.42.tmp");
    touch("17.192.0.2.9");
    forget_hostname(7);
    assert(files() == std::set<std::string>{"17.192.0.2.9"});
    puts("a lookup writes no file for a closed descriptor");
}
'''

with tempfile.TemporaryDirectory(prefix="hostname-lookup-") as tmp:
    test, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    (Path(tmp) / "lib/etc/hosts").mkdir(parents=True)
    test.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror", "-fsanitize=thread",
                    "-fno-pie", "-no-pie",
                    str(test), "-o", str(binary), "-pthread"], check=True)
    subprocess.run([str(binary)], check=True, cwd=tmp, timeout=30)
