#!/usr/bin/env python3
"""Production failure path queues each transport and resumes persistence."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

source = 'copyover.c'
code = r'''
#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include <cassert>
#include <cstdlib>
#include <string>
#include <vector>
P_desc descriptor_list;
static int resumed = 0;
static std::vector<txt_q *> queued;
void critical_command_coordinator_resume() { ++resumed; }
void critical_outbox_resume() { ++resumed; }
void player_save_pipeline_resume() { ++resumed; }
void write_to_q(const char *text, txt_q *queue, int wrap) {
    assert(std::string(text) == "failed" && wrap == 1);
    queued.push_back(queue);
}
static void raw_write_to_fd(int, const char *) { std::abort(); }
''' + extract_function(source, 'const char *copyover_state_file()') + '\n' + extract_function(source, 'static void notify_copyover_failure(') + r'''
int main() {
    unsetenv("COPYOVER_STATE_FILE");
    assert(std::string(copyover_state_file()) == "copyover.dat");
    setenv("COPYOVER_STATE_FILE", "/var/lib/duris/copyover.dat", 1);
    assert(std::string(copyover_state_file()) == "/var/lib/duris/copyover.dat");
    setenv("COPYOVER_STATE_FILE", "", 1);
    assert(std::string(copyover_state_file()) == "copyover.dat");
    descriptor_data desc[5] = {};
    char_data player = {};
    for (int i=0; i<5; ++i) {
        desc[i].descriptor = 10 + i;
        desc[i].connected = i == 4 ? CON_MAIN_MENU : CON_PLAYING;
        desc[i].character = &player;
        desc[i].next = i == 4 ? nullptr : &desc[i+1];
    }
    desc[1].out_compress = 2;
    desc[2].sslses = reinterpret_cast<gnutls_session_t>(1);
    desc[3].websocket = 1;
    descriptor_list = desc;
    notify_copyover_failure("failed");
    assert(resumed == 3 && queued.size() == 4);
    for (int i=0; i<4; ++i) assert(queued[i] == &desc[i].output);
    puts("copyover state path and queued Telnet/MCCP/TLS/WebSocket failure notices passed");
}
'''
with tempfile.TemporaryDirectory(prefix='copyover-failure-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(code)
    subprocess.run(['g++', '-std=c++20', '-g', '-fsanitize=address,undefined', '-Isrc',
                    str(path / 'test.cpp'), '-o', str(path / 'test')], cwd=ROOT, check=True)
    subprocess.run([str(path / 'test')], check=True)
