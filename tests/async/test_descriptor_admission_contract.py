"""Admission counts live clients; poll supports large descriptor numbers.

The native network-readiness fixture separately executes real high-FD sockets.
This contract protects the production admission boundary used by that loop.
"""

from _paths import extract_function
from contract_text import contains, index

admission = extract_function("net/comm.c", "int new_descriptor(")
network = extract_function("net/comm.c", "static bool service_network_turn(")

assert contains(admission, "if (used_descs >= avail_descs)")
assert index(admission, "if (used_descs >= avail_descs)") < index(admission, "ssl_new(desc)")
assert index(admission, "ssl_new(desc)") < index(admission, "used_descs++;")
assert not contains(admission, "FD_SETSIZE")
assert contains(network, "poll(sockets.data(), sockets.size(), timeout_ms)")
assert not contains(network, "FD_SET(")
print("live-connection admission and poll descriptor contracts passed")
