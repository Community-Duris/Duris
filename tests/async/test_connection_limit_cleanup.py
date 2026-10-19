#!/usr/bin/env python3
"""Execute the production capacity-refusal branch with counted TLS/socket ownership."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

descriptor = extract_function("comm.c", "int new_descriptor(int s, int conn_type)")
assert descriptor.index("ssl_new(") < descriptor.index("used_descs++")
start = descriptor.index("\tused_descs++;")
end = descriptor.index("\n\telse if (used_descs > max_descs)", start)
harness = r'''
#include <cassert>
#include <cstdio>
int used_descs, avail_descs = 4, freed, stopped, closed;
void gnutls_deinit(void *session) { assert(session); ++freed; }
int shutdown(int descriptor, int how) { assert(descriptor == 7 && how == 2); ++stopped; return 0; }
int close(int descriptor) { assert(descriptor == 7); ++closed; return 0; }
int admit(void *sslses) { const int desc = 7;
''' + descriptor[start:end] + r'''
    return 1;
}
int main() {
    int session;
    for (void *tls : {static_cast<void *>(nullptr), static_cast<void *>(&session)}) {
        used_descs = 3; freed = stopped = closed = 0;
        for (int i = 0; i < 1000; ++i) assert(!admit(tls));
        assert(used_descs == 3 && stopped == 1000 && closed == 1000);
        assert(freed == (tls ? 1000 : 0));
        used_descs = 0; freed = stopped = closed = 0;
        assert(admit(tls) && used_descs == 1);
        assert(!freed && !stopped && !closed);
    }
    puts("full-server TLS and plaintext refusal cleanup passed");
}
'''
harness = "#include <initializer_list>\n" + harness
with tempfile.TemporaryDirectory(prefix="connection-limit-") as tmp:
    test, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    test.write_text(harness)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", str(test),
                    "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
