#!/usr/bin/env python3
"""Positive actual creator friendship; negative unrelated TU/API/symbol controls.

Root owns compiler execution. Uses the actual transport access class and whole
transport compilation for symbol/link checks, without weakening its visibility.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import ROOT, extract_function

transport = (ROOT / "src/net/transport.c").read_text()
header = (ROOT / "src/net/consumable_descriptor_lifetime.h").read_text()
old = "consumable_descriptor_transport_creator_hook"
assert old not in transport and old not in header
access = extract_function("transport.c", "class transport_descriptor_creator_access final") + ";"
assert "friend bool consumable_transport_receiver::consume_world_frame(" in " ".join(access.split())
assert "private:" in access and "public:" not in access
assert "static constexpr auto install = &consumable_descriptor_lifetime_hooks::created;" in access
assert "transport_descriptor_creator_access() = delete;" in access

PRELUDE = r'''
#include "net/consumable_descriptor_lifetime.h"
#include "net/transport_protocol.h"
namespace { namespace descriptor_creator_receiver { bool consume_world_frame(const duris_transport::message &); } }
namespace consumable_transport_receiver = descriptor_creator_receiver;
namespace duris_consumable_detail {
'''
POSITIVE = r'''
}
namespace {
namespace descriptor_creator_receiver {
bool consume_world_frame(const duris_transport::message &) {
    duris_consumable_detail::transport_descriptor_creator_access::install(nullptr,nullptr,
        duris_consumable_detail::transport_descriptor_creator_access::source);
    return true;
}
}
}
int main(){return descriptor_creator_receiver::consume_world_frame(duris_transport::message{}) ? 0 : 1;}
'''
UNRELATED = r'''
}
namespace { namespace descriptor_creator_receiver { bool consume_world_frame(const duris_transport::message &){return false;} } }
void unrelated(descriptor_data *d,mm_ds *pool) {
    duris_consumable_detail::transport_descriptor_creator_access::install(d,pool,
        duris_consumable_detail::transport_descriptor_creator_access::source);
}
int main(){return descriptor_creator_receiver::consume_world_frame(duris_transport::message{}) ? 0 : 1;}
'''
OPAQUE = r'''
#include "net/consumable_descriptor_lifetime.h"
void unrelated(descriptor_data *d,mm_ds *pool) {
    duris_consumable_detail::transport_descriptor_creator_access::install(d,pool,0);
}
'''
EXTERN = r'''
struct descriptor_data;
struct mm_ds;
extern void consumable_descriptor_transport_creator_hook(descriptor_data *,mm_ds *) noexcept;
int main(){consumable_descriptor_transport_creator_hook(nullptr,nullptr);}
'''

scratch = Path(os.environ.get("DURIS_TEST_SCRATCH", str(ROOT / "bin/tests")))
scratch.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="consumable-creator-boundary-", dir=scratch) as directory:
    temp = Path(directory)
    compiler = os.environ.get("CXX", "g++")
    flags = [compiler, "-std=c++20", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-D__NO_MYSQL__", "-DDURIS_PERSISTENT_TRANSPORT", "-ffunction-sections", "-fdata-sections",
             "-I" + str(ROOT / "src"), "-pthread"]
    def compile_case(name, body):
        source = temp / (name + ".cpp")
        source.write_text(body)
        return subprocess.run(flags + ["-c", str(source), "-o", str(temp / (name + ".o"))],
                              capture_output=True, text=True, check=False)
    positive = compile_case("actual_friend", PRELUDE + access + POSITIVE)
    assert positive.returncode == 0, positive.stderr
    unrelated = compile_case("unrelated", PRELUDE + access + UNRELATED)
    assert unrelated.returncode != 0 and "private" in unrelated.stderr and "install" in unrelated.stderr, unrelated.stderr
    opaque = compile_case("opaque_unrelated", OPAQUE)
    assert opaque.returncode != 0 and "incomplete" in opaque.stderr, opaque.stderr
    whole_object = temp / "actual_transport.o"
    subprocess.run(flags + ["-c", "-x", "c++", str(ROOT / "src/net/transport.c"), "-o", str(whole_object)], check=True)
    symbols = subprocess.check_output([os.environ.get("NM", "nm"), "-g", "--defined-only", str(whole_object)], text=True)
    assert old not in symbols, "Removed raw registration bridge still exported"
    external = compile_case("ordinary_extern", EXTERN)
    assert external.returncode == 0, external.stderr
    linked = subprocess.run([compiler, str(temp / "ordinary_extern.o"), str(whole_object),
                             "-Wl,--gc-sections", "-pthread", "-o", str(temp / "unrelated_link")],
                            capture_output=True, text=True, check=False)
    assert linked.returncode != 0 and old in linked.stderr and "undefined reference" in linked.stderr, linked.stderr
print("actual descriptor creator friendship/private unrelated-TU/symbol boundary controls passed")
