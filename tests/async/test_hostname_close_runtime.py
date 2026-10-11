#!/usr/bin/env python3
"""An old reverse-DNS worker cannot publish after a descriptor closes and is reused."""
from pathlib import Path
import subprocess
import tempfile
from _paths import source, extract_function

comm = source("net/comm.c").read_text()
helpers = comm[comm.index("static void remove_hostname_files("):comm.index("int new_descriptor(")]
assert "forget_hostname(d->descriptor);" in extract_function("net/comm.c", "void close_socket(")
accepted = extract_function("net/comm.c", "int new_descriptor(")
assert accepted.index("STATE(newd) = conn_type == 1") < accepted.index("descriptor_list = newd;")
assert "STATE(newd) = CON_FLUSH;" in extract_function("net/comm.c", "static void greet(P_desc newd)\n")

HARNESS = r'''
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <dirent.h>
#include <unistd.h>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
static std::mutex gate;
static std::condition_variable ready;
static bool blocked=false, release_old=false;
static std::atomic<int> calls{0}, wakes{0};
void network_wakeup_notify() { ++wakes; }
int getnameinfo(const sockaddr *, socklen_t, char *host, socklen_t size,
                char *, socklen_t, int) {
 const int call=++calls;
 if(call==1) {
  std::unique_lock<std::mutex> lock(gate); blocked=true; ready.notify_all();
  ready.wait(lock, [] { return release_old; });
 }
 snprintf(host,size,"%s",call==1 ? "old.example" : "new.example"); return 0;
}
''' + helpers + r'''
static hostname_lookup_request *request(int fd) {
 auto *r=static_cast<hostname_lookup_request *>(calloc(1,sizeof(hostname_lookup_request)));
 strcpy(r->address,"127.0.0.1"); r->descriptor=fd;
 pthread_mutex_lock(&hostname_lookup_mutex);
 r->next=hostname_lookups; hostname_lookups=r; ++hostname_lookup_workers;
 pthread_mutex_unlock(&hostname_lookup_mutex); return r;
}
int main() {
 std::filesystem::create_directories("lib/etc/hosts");
 std::thread old(hostname_lookup_worker,request(42));
 { std::unique_lock<std::mutex> lock(gate); ready.wait(lock, [] { return blocked; }); }
 forget_hostname(42);
 std::thread current(hostname_lookup_worker,request(42)); current.join();
 { std::lock_guard<std::mutex> lock(gate); release_old=true; } ready.notify_all(); old.join();
 std::ifstream file("lib/etc/hosts/42.127.0.0.1"); std::string value; std::getline(file,value);
 assert(value=="new.example" && wakes==1 && hostname_lookup_workers==0 && !hostname_lookups);
 forget_hostname(42);
 assert(std::filesystem::is_empty("lib/etc/hosts"));
}
'''
with tempfile.TemporaryDirectory(prefix="hostname-close-") as directory:
    cpp = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    cpp.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                    "-pthread", "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], cwd=directory, check=True, timeout=20)
print("descriptor reuse retains only the current hostname and wakes the network loop")
