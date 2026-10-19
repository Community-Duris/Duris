#!/usr/bin/env python3
"""Execute the production website handler with transport and coordinator doubles."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/net/ws_handlers.c").read_text()
handler = source[source.index("void ws_cmd_durisweb_auction_bid("):
                 source.index("void ws_cmd_durisweb_auction_remove(")]
preamble = r'''
#include "economy/auction_command.h"
#include "persistence/critical_command_coordinator.h"
#include <cjson/cJSON.h>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <string>
struct descriptor_data { bool durisweb_verified = false; };
constexpr int WS_CLOSE_POLICY_VIOLATION = 1008;
bool active = true, ready = true, authorized = true, completed = false;
bool closed = false;
unsigned submissions = 0;
critical_submit_result admission = critical_submit_result::accepted;
critical_completion completion{};
std::string response;
void websocket_close(descriptor_data *, int, const char *) { closed = true; }
void websocket_send_text(descriptor_data *, const char *text) { response = text; }
bool economic_state_active() { return active; }
bool operation_log_ready() { return ready; }
bool auction_website_build_bid(const char *, uint32_t, uint32_t, int64_t,
                              const char *, critical_command *) { return authorized; }
critical_submit_result auction_transaction_submit_website(const critical_command &) {
    ++submissions; return admission;
}
bool critical_command_coordinator_get_completed(const critical_operation_id &, critical_completion *out) {
    *out = completion; return completed;
}
'''
main = r'''
void check(descriptor_data *d, const char *text, const char *status, bool success) {
    cJSON *data = cJSON_Parse(text); assert(data);
    ws_cmd_durisweb_auction_bid(d, data); cJSON_Delete(data);
    cJSON *reply = cJSON_Parse(response.c_str()); assert(reply);
    assert(!strcmp(cJSON_GetObjectItem(reply, "status")->valuestring, status));
    assert(bool(cJSON_IsTrue(cJSON_GetObjectItem(reply, "success"))) == success);
    cJSON_Delete(reply);
}
int main() {
    descriptor_data d;
    ws_cmd_durisweb_auction_bid(&d, nullptr);
    assert(closed && submissions == 0 && response.empty());
    d.durisweb_verified = true;
    const char *valid = R"({"requestId":"stable","accountName":"bidder","characterId":2,"auctionId":7,"amountCopper":1000})";
    check(&d, R"({"requestId":"stable","accountName":"bidder","characterId":2,"auctionId":7,"amountCopper":1.5})", "rejected", false);
    check(&d, R"({"requestId":"stable","accountName":"bidder","characterId":2,"auctionId":7,"amountCopper":4294967296})", "rejected", false);
    assert(submissions == 0);
    active = false; check(&d, valid, "unavailable", false); active = true;
    ready = false; check(&d, valid, "unavailable", false); ready = true;
    authorized = false; check(&d, valid, "rejected", false); authorized = true;
    assert(submissions == 0);
    check(&d, valid, "pending", false);
    admission = critical_submit_result::identity_conflict;
    check(&d, valid, "rejected", false);
    admission = critical_submit_result::overloaded;
    check(&d, valid, "unavailable", false);
    admission = critical_submit_result::attached; completed = true;
    completion.outcome = critical_apply_outcome::terminal_failure;
    completion.error_code = ENOSPC;
    check(&d, valid, "rejected", false);
    auction_command_result result{};
    result.action = auction_action::bid; result.auction_id = 7; result.final_price = 1000;
    result.auction_revision = UINT64_C(9007199254740993); result.wallet_revision = 5;
    std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> bytes;
    assert(auction_command_encode_result(result, &bytes));
    std::copy(bytes.begin(), bytes.end(), completion.result_payload.begin());
    completion.result_size = bytes.size(); completion.error_code = 0;
    completion.outcome = critical_apply_outcome::applied;
    check(&d, valid, "committed", true);
    assert(response.find("9007199254740993") != std::string::npos);
    completion.outcome = critical_apply_outcome::already_applied;
    check(&d, valid, "committed", true);
}
'''
with tempfile.TemporaryDirectory(prefix="auction-web-") as directory:
    path = Path(directory)
    (path / "handler.cpp").write_text(preamble + handler + main)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-Isrc", str(path / "handler.cpp"), "src/economy/auction_command.c",
                    "src/economy/currency_command.c", "src/item/item_transfer_command.c",
                    "src/persistence/critical_command.c", "src/player/player_snapshot_codec.c",
                    "-lcjson", "-lcrypto", "-o", str(path / "handler")], cwd=ROOT, check=True)
    subprocess.run([str(path / "handler")], check=True)
