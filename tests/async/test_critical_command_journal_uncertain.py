#!/usr/bin/env python3
"""Verify coordinator retention of the original fence on uncertain journal append."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel


HARNESS = r'''
#include "persistence/critical_command_coordinator.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unistd.h>

static int close_fault = 0;
static int apply_calls = 0;

extern "C" int __real_close(int);
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

static critical_command make_command()
{
    critical_command command = {};
    command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
    assert(critical_operation_id_generate(&command.operation_id));
    command.type = critical_command_type::test;
    command.payload_version = 1;
    command.source_site = critical_source_site::command;
    command.deadline_class = critical_deadline_class::interactive;
    command.accepted_at_usec = 1700000000000000ULL;
    command.keys = {{critical_entity_type::player, 9}};
    command.payload = {9};
    assert(critical_command_normalize(&command));
    return command;
}

static critical_apply_result apply(const critical_command &, void *)
{
    ++apply_calls;
    return {critical_apply_outcome::applied, 1, 0};
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    std::filesystem::remove_all(argv[1]);
    assert(critical_command_coordinator_init(argv[1], apply, nullptr, 1));
    const critical_command command = make_command();
    close_fault = 2; // append close + rollback close: rollback outcome is uncertain
    assert(critical_command_coordinator_submit(command) ==
           critical_submit_result::journal_uncertain);
    const auto health = critical_command_coordinator_health_copy();
    assert(health.blocked == 1 && health.fenced_keys == 1 && health.ambiguous == 1);
    critical_operation_id fenced = {};
    assert(critical_command_coordinator_is_fenced(
        {critical_entity_type::player, 9}, &fenced));
    assert(critical_operation_id_equal(fenced, command.operation_id));
    close_fault = 0;
    assert(critical_command_coordinator_recover_uncertain());
    assert(critical_command_coordinator_drain(3000));
    const auto recovered = critical_command_coordinator_health_copy();
    assert(recovered.blocked == 0 && recovered.fenced_keys == 0);
    assert(apply_calls == 1);
    critical_command_coordinator_shutdown();
}
'''


with tempfile.TemporaryDirectory(prefix="duris-coordinator-uncertain-") as temporary:
    root = Path(temporary)
    source = root / "coordinator_uncertain.cpp"
    binary = root / "coordinator_uncertain"
    source.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread", "-Isrc",
            str(source), rel("critical_command.c"), rel("critical_command_journal.c"),
            rel("critical_command_coordinator.c"), "-lz", "-lcrypto",
            "-Wl,--wrap=close", "-o", str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(root / "journal")], check=True, timeout=20)

for relative in (
    "src/world/zone_touch_transaction.c",
    "src/world/epic_transaction.c",
    "src/economy/shop_trade_transaction.c",
    "src/economy/auction_transaction.c",
    "src/economy/boon_shop_transaction.c",
    "src/economy/boon_reward_transaction.c",
    "src/economy/currency_transaction.c",
    "src/guild/artifact_guild_transaction.c",
    "src/combat/combat_outcome_transaction.c",
    "src/persistence/corpse_lifecycle_transaction.c",
    "src/item/item_transfer_synthetic.c",
    "src/account/session_audit_transaction.c",
):
    assert "critical_submit_result_keeps_operation" in (ROOT / relative).read_text()

print("critical coordinator uncertain-journal fence checks passed")
