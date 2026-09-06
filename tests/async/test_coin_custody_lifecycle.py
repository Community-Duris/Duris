#!/usr/bin/env python3
"""Physical coin puts retain or establish custody before live publication."""

from _paths import ROOT, SRC
from pathlib import Path
import subprocess
import tempfile


def body(source: str, signature: str, terminator: str) -> str:
    """Return source text from a function signature to the next declaration."""
    start = source.index(signature)
    return source[start:source.index(terminator, start)]


actobj = (SRC / "actobj.c").read_text(encoding="utf-8")
movement = (SRC / "item_movement_transaction.c").read_text(encoding="utf-8")
mysql = (ROOT / "tests/async/item_transfer_mysql_harness.cpp").read_text(encoding="utf-8")
flatfile = (ROOT / "tests/async/flatfile_item_repository_harness.cpp").read_text(encoding="utf-8")

publish = body(actobj, "static bool submit_coin_put(", "\nstruct coin_pickup_context")
completion = body(actobj, "bool coin_put_custody_completion(", "\nstatic bool submit_coin_put(")
prepare = body(actobj, "static bool prepare_coin_pile(", "\n// A committed result")
materialize = body(actobj, "static bool publish_coin_pile(", "\nbool coin_put_custody_completion(")

# Both the debit and final pile payload are submitted as one coin command.
assert "currency_transaction_coin_wallet" in publish
assert "prepare_coin_pile" in publish
assert "currency_transaction_submit_coin" in publish
assert "currency_transaction_submit_wallet_value" not in publish
assert "item_movement_transaction_submit" not in publish
assert "snapshot.values.begin()" in prepare
assert "player_item_snapshot_list_encode" in prepare
assert "candidate.after = after" in prepare
assert "extract_obj(money, FALSE)" in completion
assert "refund_committed_coin_debit" not in completion
assert completion.index("if (!committed)") < completion.index("publish_coin_pile")
assert "player_load_item_graph_materialize_detached" in materialize
assert "current.item_revision > result.max_item_revision" in materialize

# The shared item command serializes the pile before submission. Existing backend
# harnesses exercise direct custody creation under a container, which is the
# backend-neutral boundary used here; MariaDB snapshots may publish only after it.
assert "player_item_snapshot_list_encode" in movement
assert "nested_creation.target_parent_item_uid" in mysql
assert "nested_single_creation" in flatfile and "target_parent_item_uid" in flatfile
assert "item_transfer_repository_execute" in (
    SRC / "item_transfer_repository.c").read_text(encoding="utf-8")
assert "flatfile_item_transfer_materialization_prepare" in (
    SRC / "flatfile_item_repository.c").read_text(encoding="utf-8")

print("physical coin puts commit wallet and final pile custody together")

# Exercise the actual pile arithmetic, including amounts that overflow an int
# when summed across denominations. Transport and string ownership are fixtures.
handler = (SRC / "handler.c").read_text(encoding="utf-8")
add_coins = body(handler, "void add_coins(", "\nP_obj create_money(")
harness = r'''
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
struct extra_descr_data { char *description = nullptr; };
struct Object {
    int type = 1, value[8] = {}, weight = 0, str_mask = 0;
    char *description = nullptr, *short_description = nullptr;
    extra_descr_data *ex_description = nullptr;
    ~Object() { free(description); free(short_description); }
};
using P_obj = Object *;
constexpr int ITEM_MONEY = 1, LOG_EXIT = 0, STRUNG_DESC1 = 1, STRUNG_DESC2 = 2;
const char *coin_names[] = {"copper", "silver", "gold", "platinum"};
void logit(const char *, const char *, ...) {}
void logit(int, const char *, ...) {}
void str_free(char *s) { free(s); }
char *str_dup(const char *s) { return strdup(s); }
#define APPENDF(buf, ...) snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), __VA_ARGS__)
''' + add_coins + r'''
int main() {
    Object pile;
    extra_descr_data detail;
    pile.ex_description = &detail;
    add_coins(&pile, INT_MAX, INT_MAX, INT_MAX, INT_MAX);
    for (int i = 0; i < 4; ++i) assert(pile.value[i] == INT_MAX);
    assert(!strcmp(pile.short_description, "a mountain of coins"));
    assert(strstr(detail.description, "1/4 copper coins"));

    const char *before = pile.description;
    add_coins(&pile, 0, 0, 0, 1);
    for (int i = 0; i < 4; ++i) assert(pile.value[i] == INT_MAX);
    assert(pile.description == before);
    add_coins(&pile, -1, 0, 0, 0);
    assert(pile.value[0] == INT_MAX && pile.description == before);
    free(detail.description);

    Object ordinary;
    add_coins(&ordinary, 1, 2, 3, 4);
    add_coins(&ordinary, 2, 3, 4, 5);
    assert(ordinary.value[0] == 3 && ordinary.value[1] == 5 &&
           ordinary.value[2] == 7 && ordinary.value[3] == 9);
    puts("coin pile arithmetic preserves amounts and rejects overflow without mutation");
}
'''
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="coin-custody-", dir=build) as directory:
    source = Path(directory) / "coin_arithmetic.cpp"
    binary = Path(directory) / "coin_arithmetic"
    source.write_text(harness)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
        str(source), "-o", str(binary),
    ], check=True, cwd=ROOT, timeout=60)
    subprocess.run([str(binary)], check=True, cwd=ROOT, timeout=10)

# The durable coin command must contain the intended pile amount. In particular,
# accepting the old amount here reproduces the existing merge loss window.
command_harness = r'''
#include "economy/coin_transfer_command.h"
#include "economy/currency_command.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "core/structs.h"
#include "world/vnum.obj.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>

critical_operation_id operation() {
    critical_operation_id id = {};
    assert(critical_operation_id_generate(&id));
    return id;
}
coin_transfer_endpoint wallet(uint32_t pid, std::array<int32_t,4> before,
                              std::array<int32_t,4> after) {
    coin_transfer_endpoint endpoint;
    endpoint.before = before;
    endpoint.after = after;
    currency_command_payload payload = {};
    payload.pid = pid;
    payload.racewar = 1;
    payload.reason = currency_reason_type::coin_transfer;
    strcpy(payload.account_name.data(), "coin_harness");
    for (size_t i = 0; i < 4; ++i)
        payload.wallet_delta.amount[i] = int64_t(after[i]) - before[i];
    assert(currency_command_build(&endpoint.change, operation(), payload, 4, 5,
                                  critical_source_site::command,
                                  critical_deadline_class::interactive));
    return endpoint;
}
coin_transfer_endpoint pile(std::array<int32_t,4> before,
                            std::array<int32_t,4> after, bool stale_snapshot = false) {
    coin_transfer_endpoint endpoint;
    endpoint.before = before;
    endpoint.after = after;
    const bool created = before == std::array<int32_t,4>{};
    const bool consumed = after == std::array<int32_t,4>{};
    item_transfer_payload transfer = {};
    transfer.from_owner = {created ? item_owner_type::system : item_owner_type::player, created ? 0u : 1u, 0};
    transfer.to_owner = {consumed ? item_owner_type::destruction : item_owner_type::player, consumed ? 0u : 1u, 0};
    transfer.reason = created ? item_transfer_reason::creation : consumed ?
        item_transfer_reason::destruction : item_transfer_reason::player_put;
    transfer.expected_from_revision = 7;
    transfer.expected_to_revision = 7;
    transfer.selected_item_uid = 100;
    transfer.target_root_item_uid = consumed ? 100 : 90;
    transfer.target_parent_item_uid = consumed ? 0 : 90;
    transfer.expected_target_parent_revision = consumed ? 0 : 2;
    transfer.item_count = 1;
    transfer.items[0] = {100, created ? 100u : 90u, created ? 0u : 90u,
                        created ? ITEM_TRANSFER_ABSENT_REVISION : 3,
                        VOBJ_COINS, created ? item_custody_state::absent : item_custody_state::active};
    player_item_snapshot snapshot = {};
    snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
    snapshot.equipment_slot = -1;
    snapshot.object_uid = 100;
    snapshot.vnum = VOBJ_COINS;
    snapshot.type = ITEM_MONEY;
    const auto &amount = consumed || stale_snapshot ? before : after;
    std::copy(amount.begin(), amount.end(), snapshot.values.begin());
    std::vector<uint8_t> bytes;
    assert(player_item_snapshot_list_encode({snapshot}, &bytes) == player_snapshot_codec_result::ok);
    transfer.item_blob_size = bytes.size();
    std::copy(bytes.begin(), bytes.end(), transfer.item_blob.begin());
    assert(item_transfer_command_build(&endpoint.change, operation(), transfer,
                                     critical_source_site::command,
                                     critical_deadline_class::interactive));
    return endpoint;
}
bool build(const coin_transfer_payload &payload, critical_command *command) {
    return coin_transfer_command_build(command, operation(), payload,
                                       critical_source_site::command,
                                       critical_deadline_class::interactive);
}
void roundtrip(const coin_transfer_payload &payload) {
    critical_command command;
    assert(build(payload, &command));
    command.accepted_at_usec = 12345;
    assert(critical_command_valid(command));
    std::vector<uint8_t> bytes;
    assert(critical_command_encode(command, &bytes) == critical_command_codec_result::ok);
    critical_command recovered;
    assert(critical_command_decode(bytes.data(), bytes.size(), &recovered) == critical_command_codec_result::ok);
    coin_transfer_payload decoded;
    assert(coin_transfer_command_decode_payload(recovered, &decoded));
    assert(decoded.source.before == payload.source.before && decoded.source.after == payload.source.after);
    assert(decoded.destination.before == payload.destination.before && decoded.destination.after == payload.destination.after);
    assert(!critical_operation_id_equal(decoded.source.change.operation_id, decoded.destination.change.operation_id));
    assert(!critical_operation_id_equal(decoded.source.change.operation_id, command.operation_id));
    for (size_t size = 0; size < command.payload.size(); ++size) {
        critical_command truncated = command;
        truncated.payload.resize(size);
        assert(!coin_transfer_command_decode_payload(truncated, &decoded));
    }
    recovered.payload.push_back(0);
    assert(!coin_transfer_command_decode_payload(recovered, &decoded));
    recovered = command;
    recovered.keys.pop_back();
    assert(!coin_transfer_command_decode_payload(recovered, &decoded));
    recovered = command;
    recovered.operation_id = operation();
    assert(!coin_transfer_command_decode_payload(recovered, &decoded));
}
int main() {
    const std::array<int32_t,4> full = {INT_MAX,INT_MAX,INT_MAX,INT_MAX}, zero = {};
    roundtrip({wallet(1, full, zero), pile(zero, full)});
    roundtrip({wallet(1, {1,2,3,4}, zero), pile({5,6,7,8}, {6,8,10,12})});
    roundtrip({pile({1,2,3,4}, zero), wallet(1, zero, {1,2,3,4})});
    roundtrip({pile({1,2,3,4}, {1,2,3,2}), wallet(1, zero, {0,0,0,2})});
    roundtrip({wallet(1, {0,0,0,1}, {0,0,9,0}), wallet(2, zero, {0,0,1,0})});
    critical_command rejected;
    assert(!build({wallet(1, {1,2,3,4}, zero), pile({5,6,7,8}, {6,8,10,12}, true)}, &rejected));
    assert(!build({wallet(1, {1,2,3,4}, zero), pile(zero, {1,2,3,5})}, &rejected));
    assert(!build({wallet(1, {0,0,0,1}, zero), wallet(1, zero, {0,0,0,1})}, &rejected));
    assert(!build({wallet(1, {-1,0,0,1}, zero), pile(zero, {0,0,0,1})}, &rejected));
    puts("coin command preserves full amounts, post-merge payloads, remainders, and replay identity");
}
'''
with tempfile.TemporaryDirectory(prefix="coin-command-", dir=build) as directory:
    source = Path(directory) / "coin_command.cpp"
    binary = Path(directory) / "coin_command"
    source.write_text(command_harness)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-Isrc",
        str(source), "src/economy/coin_transfer_command.c", "src/economy/currency_command.c",
        "src/item/item_transfer_command.c", "src/player/player_snapshot_codec.c",
        "src/persistence/critical_command.c", "-lcrypto", "-o", str(binary),
    ], check=True, cwd=ROOT, timeout=60)
    subprocess.run([str(binary)], check=True, cwd=ROOT, timeout=30)
