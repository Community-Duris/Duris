"""Static safety contracts for poll registration and live connection capacity.

The native network-readiness harness separately exercises a real descriptor
above FD_SETSIZE. These checks guard production admission/registration without
starting a listener or connecting to a game.
"""
import re

from _paths import extract_function, source
from _source_contract import strip_comments
from contract_text import contains, find


def poll_registration_safe(comm: str, turn: str) -> bool:
    code = re.sub(r'"(?:[^"\\]|\\.)*"', '""', strip_comments(comm))
    forbidden = r"\b(?:FD_SET|FD_CLR|FD_ISSET|FD_ZERO|select|pselect)\s*\("
    return (
        re.search(forbidden, code) is None
        and contains(turn, "std::vector<pollfd> sockets")
        and contains(turn, "poll(sockets.data(), sockets.size(), timeout_ms)")
        and contains(turn, "point->network_close_pending || !interests ? -1 : point->descriptor")
        and contains(turn, "clients.push_back(point)")
        and 0 <= find(turn, "clients[index]->network_revents = sockets[index + 4].revents;")
        < find(turn, 'drain_new_connections(ctx.telnet_listener, 0, "Telnet")')
    )


def admission_safe(admission: str) -> bool:
    code = strip_comments(admission)
    guard = re.search(r"if\s*\(used_descs\s*>=\s*avail_descs\)\s*\{([^{}]*)\}", code)
    if guard is None:
        return False
    rejected = guard.group(1)
    return (
        0 <= find(code, "desc = new_connection(s)") < guard.start()
        and 0 <= find(rejected, "shutdown(desc, 2);")
        < find(rejected, "close(desc);") < find(rejected, "return 0;")
        and guard.end() < find(code, "ssl_new(desc)") < find(code, "used_descs++;")
        < find(code, "mm_get(dead_desc_pool)")
    )


comm = source("net/comm.c").read_text(encoding="latin-1")
turn = extract_function("net/comm.c", "static bool service_network_turn(")
admission = extract_function("net/comm.c", "int new_descriptor(")
assert poll_registration_safe(comm, turn), "dynamic poll registration and readiness before admission"
assert admission_safe(admission), "live capacity rejects and closes before TLS/count/allocation"

for label, changed_comm, changed_turn in (
    ("fd-set regression", comm + "\nFD_SET(10000, &set);", turn),
    ("select regression", comm + "\nselect(10001, &set, 0, 0, 0);", turn),
    ("missing dynamic poll", comm, turn.replace("poll(sockets.data(), sockets.size(), timeout_ms)", "0")),
    ("missing readiness assignment", comm, turn.replace("sockets[index + 4].revents", "0")),
):
    assert not poll_registration_safe(changed_comm, changed_turn), label
for label, changed in (
    ("numeric capacity", admission.replace("used_descs >= avail_descs", "desc >= FD_SETSIZE")),
    ("missing close", admission.replace("close(desc);", "", 1)),
    ("missing shutdown", admission.replace("shutdown(desc, 2);", "", 1)),
    ("missing rejection", admission.replace("return 0;", "", 1)),
):
    assert not admission_safe(changed), label
print("descriptor poll/admission contracts passed; 8 negative mutations rejected")
