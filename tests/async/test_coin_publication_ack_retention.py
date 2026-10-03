#!/usr/bin/env python3
"""Production coin owner publication ordering with an irreversible ACK boundary.

Native component proof: actual transaction owner, authority, and wire codecs;
controlled coordinator and physical publisher. No database, journal I/O, Redis,
or game service is used. Successful simulated ACK erases its held operation.
"""

from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import signal
import subprocess
import tempfile
import time

from _paths import ROOT, rel


HARNESS = r'''

#include "core/utils.h"
#include "economy/account_bank_balances.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "player/player_snapshot_codec.h"
#include "sql/sql_player.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

P_desc descriptor_list = nullptr;
P_char character_list = nullptr;
static P_char online = nullptr, other = nullptr;
static critical_command submitted;
static bool held = false, ack_available = true, physical_available = false;
static int submissions = 0, ack_attempts = 0, acks = 0, physical_attempts = 0;
static int physical_successes = 0, notifications = 0, failures = 0;
static bool notification_before_ack = false;
static bool throw_physical = false, throw_notification = false, fail_notification = false;
static int bank_calls = 0, throw_bank_call = 0;
static constexpr uint64_t pile_uid = 99001;

class economic_gameplay_authority_test_access
{
    public:
	static void install()
	{
		critical_operation_id lineage = {}, epoch = {}, receipt = {};
		lineage.bytes[0] = 1;
		epoch.bytes[0] = 2;
		receipt.bytes[0] = 3;
		const std::array wallets = {
			economic_gameplay_wallet_mapping{
				42, { lineage, economic_account_kind::wallet, 42, 0 } },
			economic_gameplay_wallet_mapping{
				45, { lineage, economic_account_kind::wallet, 45, 0 } }
		};
		const std::array banks = { economic_gameplay_bank_mapping{
			"retention_account", 1, { lineage, economic_account_kind::bank, 52, 1 } } };
		assert(economic_gameplay_authority::install(lineage, epoch, receipt, wallets,
							    banks) ==
		       economic_accounting_error::ok);
	}
};

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}
const char *get_account_name_safe(P_char)
{
	return "retention_account";
}
P_char find_player_by_pid(int pid)
{
	if (online && GET_PID(online) == pid)
		return online;
	return other && GET_PID(other) == pid ? other : nullptr;
}
int IS_MORPH(P_char ch)
{
	return ch && IS_NPC(ch) && ch->only.npc && ch->only.npc->orig_char;
}
bool critical_command_coordinator_is_fenced(const critical_entity_key &key,
					    critical_operation_id *id)
{
	if (!held)
		return false;
	for (const auto &candidate : submitted.keys)
		if (candidate.type == key.type && candidate.id == key.id)
		{
			if (id)
				*id = submitted.operation_id;
			return true;
		}
	return false;
}
critical_submit_result critical_command_coordinator_submit(critical_command)
{
	abort(); // Only publication-held schema-2 roots are admitted by this fixture.
}
critical_submit_result critical_command_coordinator_submit_for_publication(critical_command command)
{
	assert(!held);
	command.publication_required = true;
	command.accepted_at_usec = 1;
	submitted = std::move(command);
	held = true;
	++submissions;
	return critical_submit_result::accepted;
}
bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &id)
{
	++ack_attempts;
	if (!held || id.bytes != submitted.operation_id.bytes || !ack_available)
		return false;
	held = false; // Successful ACK retires the operation: subsequent ACK must fail.
	++acks;
	return true;
}
bool critical_command_coordinator_get_completed(const critical_operation_id &,
						critical_completion *)
{
	return false;
}
void gmcp_char_vitals(P_char) {}
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}
void publish_account_bank_balances_revision(const char *, int, const AccountBankBalances *,
					    uint64_t)
{
	if (++bank_calls == throw_bank_call)
		throw std::bad_alloc();
}

static bool physical(P_char,
#ifdef DURIS_COIN_SPLIT_PUBLICATION_TEST
		     const critical_operation_id &id,
#endif
		     const coin_transfer_payload &, const coin_transfer_result &, const uint8_t *,
		     size_t)
{
#ifdef DURIS_COIN_SPLIT_PUBLICATION_TEST
	assert(id.bytes == submitted.operation_id.bytes);
#endif
	++physical_attempts;
	if (throw_physical)
		throw std::runtime_error("controlled physical failure");
	if (!physical_available)
		return false;
	++physical_successes;
	return true;
}
static bool notify(P_char, bool committed, const coin_transfer_payload &,
		   const coin_transfer_result &, unsigned int error, const uint8_t *, size_t)
{
	assert(committed && !error);
	++notifications;
	if (!acks || held)
		notification_before_ack = true;
	if (throw_notification)
		throw std::runtime_error("controlled notification failure");
	return !fail_notification;
}
#ifndef DURIS_COIN_SPLIT_PUBLICATION_TEST
static bool legacy_composite(P_char actor, bool committed, const coin_transfer_payload &payload,
			     const coin_transfer_result &result, unsigned int error,
			     const uint8_t *context, size_t size)
{
	if (error == EOWNERDEAD)
		return false;
	assert(committed && !error);
	if (!physical(actor, payload, result, context, size))
		return false;
	return notify(actor, committed, payload, result, error, context, size);
}
#endif
static void check(bool condition, const char *name)
{
	printf("%s %s\n", condition ? "PASS" : "FAIL", name);
	if (!condition)
		++failures;
}

static coin_transfer_payload transfer(P_char actor, bool wallet_only)
{
	coin_transfer_payload payload = {};
	assert(currency_transaction_coin_wallet(actor, -1, &payload.source));
	if (wallet_only)
	{
		assert(currency_transaction_coin_wallet(other, 1, &payload.destination));
		return payload;
	}
	auto &endpoint = payload.destination;
	endpoint.after = { 1, 0, 0, 0 };
	item_transfer_payload pile = {};
	pile.from_owner = { item_owner_type::system, 0, 0 };
	pile.to_owner = { item_owner_type::room, 77, 0 };
	pile.reason = item_transfer_reason::creation;
	pile.selected_item_uid = pile_uid;
	pile.target_root_item_uid = pile_uid;
	pile.item_count = 1;
	pile.items[0] = { pile_uid, pile_uid,
			  0,	    ITEM_TRANSFER_ABSENT_REVISION,
			  402013,   item_custody_state::absent };
	player_item_snapshot snapshot = {};
	snapshot.object_uid = pile_uid;
	snapshot.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	snapshot.vnum = 402013;
	snapshot.type = ITEM_MONEY;
	snapshot.values[0] = 1;
	std::vector<uint8_t> blob;
	assert(player_item_snapshot_list_encode({ snapshot }, &blob) ==
	       player_snapshot_codec_result::ok);
	pile.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), pile.item_blob.begin());
	critical_operation_id id = {};
	assert(critical_operation_id_generate(&id));
	assert(item_transfer_command_build(&endpoint.change, id, pile,
					   critical_source_site::command,
					   critical_deadline_class::interactive));
	return payload;
}

static critical_completion receipt(const coin_transfer_payload &payload)
{
	coin_transfer_result result = {};
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	for (size_t i = 0; i < 2; ++i)
	{
		if (endpoints[i]->change.type == critical_command_type::account_bank)
		{
			for (size_t d = 0; d < 4; ++d)
				result.wallets[i].wallet.amount[d] = endpoints[i]->after[d];
			result.wallets[i].wallet_revision =
				endpoints[i]->change.expected_revisions[0].revision + 1;
			result.wallets[i].bank_revision =
				endpoints[i]->change.expected_revisions[1].revision + 1;
		}
		else
			result.piles[i] = { pile_uid, 1, 1, 1, 1, 0 };
	}
	std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> bytes = {};
	assert(coin_transfer_command_encode_result(payload, result, &bytes));
	critical_completion completed = {};
	completed.operation_id = submitted.operation_id;
	completed.outcome = critical_apply_outcome::applied;
	completed.result_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), completed.result_payload.begin());
	return completed;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string scenario = argv[1];
	pc_only_data player = {}, recipient_player = {};
	player.pid = 42;
	recipient_player.pid = 45;
	player.wallet_revision = player.bank_revision = 1;
	recipient_player.wallet_revision = recipient_player.bank_revision = 1;
	char_data actor = {}, recipient = {};
	actor.only.pc = &player;
	recipient.only.pc = &recipient_player;
	actor.player.racewar = recipient.player.racewar = 1;
	GET_COPPER(&actor) = 5;
	online = &actor;
	other = &recipient;
	economic_gameplay_authority_test_access::install();
	const bool partial = scenario == "partial_projection_retry" ||
			     scenario == "partial_projection_conflict";
	const bool wallet_only = scenario == "wallet_only_replay" || partial;
	const bool replay = scenario == "missing_publisher" || scenario == "absent_actor" ||
			    scenario == "wallet_only_replay";
	const auto payload = transfer(&actor, wallet_only);
	if (scenario == "missing_admission")
	{
		const bool admitted =
			currency_transaction_submit_coin(&actor, payload, nullptr, nullptr, 0);
		check(!admitted, "schema2 item admission needs physical capability");
		check(!held && !submissions && !currency_transaction_health_copy().pending,
		      "missing capability refusal has no coordinator or owner mutation");
		check(GET_COPPER(&actor) == 5 && player.wallet_revision == 1,
		      "missing capability refusal preserves live wallet");
		return failures ? 1 : 0;
	}
	if (replay)
	{
		critical_operation_id id = {};
		assert(critical_operation_id_generate(&id));
		assert(coin_transfer_command_build(&submitted, id, payload,
						   critical_source_site::command,
						   critical_deadline_class::interactive));
		assert(economic_gameplay_authority::prepare_coin_transfer(&submitted) ==
		       economic_accounting_error::ok);
		submitted.publication_required = true;
		submitted.accepted_at_usec = 1;
		assert(critical_command_envelope_valid(submitted));
		held = true;
		assert(currency_transaction_restore_replayed_command(submitted));
		if (scenario == "absent_actor")
			online = nullptr;
	}
	else
	{
#ifdef DURIS_COIN_SPLIT_PUBLICATION_TEST
		assert(currency_transaction_submit_coin(&actor, payload,
							wallet_only ? notify : nullptr, nullptr, 0,
							{ physical, notify }));
#else
		assert(currency_transaction_submit_coin(
			&actor, payload, wallet_only ? notify : legacy_composite, nullptr, 0));
#endif
		assert(submissions == 1);
	}
	const auto original_id = submitted.operation_id;
	const auto completed = receipt(payload);
	const bool changed = scenario == "changed_after_physical" ||
			     scenario == "changed_after_wallet";
	if (scenario == "ack_retry" || scenario == "changed_after_physical")
	{
		physical_available = true;
		ack_available = false;
	}
	if (scenario == "notification_exception" || scenario == "notification_false")
	{
		physical_available = true;
		throw_notification = scenario == "notification_exception";
		fail_notification = scenario == "notification_false";
	}
	if (scenario == "physical_exception")
		throw_physical = true;
	if (partial)
		throw_bank_call = scenario == "partial_projection_retry" ? 2 : 1;
	bool projection_escaped = false;
	try
	{
		currency_transaction_handle_completions(&completed, 1);
	}
	catch (const std::bad_alloc &)
	{
		projection_escaped = true;
	}
	if (partial)
	{
		check(!projection_escaped, "partial native projection exception retained by owner");
		check(GET_COPPER(&actor) == 4 && player.wallet_revision == 2,
		      "partial exception follows actual source wallet assignment");
		check(held && !ack_attempts && !acks && !notifications &&
			      currency_transaction_health_copy().pending == 1,
		      "partial projection retains original obligation before ACK");
		check(currency_transaction_player_busy(&actor) &&
			      currency_transaction_player_busy(&recipient),
		      "partial projection retains both wallet lifecycle owners");
		if (scenario == "partial_projection_retry")
		{
			currency_transaction_handle_completions(nullptr, 0);
			check(!held && acks == 1 && notifications == 1 &&
				      !currency_transaction_health_copy().pending,
			      "same exact partial receipt retries to ACK");
			check(GET_COPPER(&actor) == 4 && GET_COPPER(&recipient) == 1 &&
				      player.wallet_revision == 2 &&
				      recipient_player.wallet_revision == 2,
			      "partial retry publishes exact wallets without double debit");
		}
		else
		{
			check(GET_COPPER(&recipient) == 0 && recipient_player.wallet_revision == 1,
			      "first-endpoint fault precedes destination assignment");
			auto conflict = completed;
			conflict.durable_revision = completed.durable_revision + 1;
			currency_transaction_handle_completions(&conflict, 1);
			currency_transaction_handle_completions(nullptr, 0);
			check(held && !acks && !notifications &&
				      currency_transaction_health_copy().publication_blocked == 1,
			      "changed receipt cannot replace partial native effect");
			check(GET_COPPER(&recipient) == 0 &&
				      recipient_player.wallet_revision == 1 && bank_calls == 1,
			      "changed partial receipt cannot continue native projection");
		}
	}
	else if (changed)
	{
		check(held && !acks && !notifications,
		      "changed-receipt setup retains original obligation");
		const int before = physical_attempts;
		auto conflict = completed;
		conflict.durable_revision = completed.durable_revision + 1;
		physical_available = true;
		ack_available = true;
		currency_transaction_handle_completions(&conflict, 1);
		currency_transaction_handle_completions(nullptr, 0);
		check(held && !acks && !notifications && physical_attempts == before,
		      "changed authoritative receipt cannot overwrite published wallet or physical obligation");
		check(currency_transaction_health_copy().pending == 1 &&
			      currency_transaction_health_copy().publication_blocked == 1 &&
			      currency_transaction_player_busy(&actor) &&
			      currency_transaction_coin_item_busy(pile_uid),
		      "changed receipt retains original entry and lifecycle fences");
		check(GET_COPPER(&actor) == 4 && player.wallet_revision == 2,
		      "changed receipt cannot reapply wallet");
	}
	else if (scenario == "notification_exception" || scenario == "notification_false")
	{
		check(acks == 1 && !held && physical_successes == 1 && notifications == 1 &&
			      !notification_before_ack &&
			      !currency_transaction_health_copy().pending,
		      "post-ACK notification failure cannot resurrect economic owner");
		currency_transaction_handle_completions(nullptr, 0);
		check(physical_attempts == 1 && notifications == 1,
		      "post-ACK failure has no economic retry");
	}
	else if (wallet_only)
	{
		check(acks == 1 && !held && !currency_transaction_health_copy().pending,
		      "wallet-only replay can ACK without item publisher");
		check(GET_COPPER(&actor) == 4 && GET_COPPER(&recipient) == 1,
		      "wallet-only exact balances");
	}
	else if (replay)
	{
		check(!ack_attempts && !acks && held, "missing physical capability cannot ACK");
		check(currency_transaction_health_copy().pending == 1,
		      "replay original entry retained");
		check(currency_transaction_coin_item_busy(pile_uid), "replay pile remains busy");
		check(currency_transaction_player_busy(&actor),
		      "replay actor remains busy after wallet publication");
		currency_transaction_handle_completions(nullptr, 0);
		check(held && !ack_attempts && !physical_attempts && !notifications,
		      "replay pulse cannot invent publisher");
	}
	else if (scenario == "ack_retry")
	{
		check(physical_successes == 1 && physical_attempts == 1,
		      "physical publication precedes failed ACK");
		check(held && !acks && !notifications,
		      "failed ACK retains hold and defers notification");
		check(currency_transaction_player_busy(&actor), "failed ACK actor remains busy");
		auto duplicate = completed;
		duplicate.outcome = critical_apply_outcome::already_applied;
		duplicate.attempt = completed.attempt + 1;
		currency_transaction_handle_completions(&duplicate, 1);
		check(physical_successes == 1 && physical_attempts == 1,
		      "ACK retry cannot repeat physical publication");
		ack_available = true;
		currency_transaction_handle_completions(nullptr, 0);
		check(acks == 1 && !held && notifications == 1 && !notification_before_ack,
		      "notification follows successful ACK");
		check(!currency_transaction_health_copy().pending,
		      "ACK retry retires domain entry");
	}
	else
	{
		check(physical_attempts == 1 && !physical_successes,
		      "failed physical publication attempted");
		check(!ack_attempts && !acks && held, "failed physical publication cannot ACK");
		check(currency_transaction_player_busy(&actor),
		      "physical retry actor remains busy");
		check(currency_transaction_coin_item_busy(pile_uid),
		      "physical retry pile remains busy");
		if (scenario == "exhaustion" || scenario == "physical_exception")
		{
			for (unsigned int i = 0; i < CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS + 2;
			     ++i)
				currency_transaction_handle_completions(nullptr, 0);
			check(physical_attempts == CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS,
			      "physical attempts bounded");
			check(held && !acks && currency_transaction_health_copy().pending == 1,
			      "exhaustion retains coordinator receipt and domain entry");
			check(currency_transaction_player_busy(&actor) &&
				      currency_transaction_coin_item_busy(pile_uid),
			      "exhaustion retains lifecycle and pile fences");
			check(!notifications &&
				      !currency_transaction_health_copy().publication_abandoned,
			      "exhaustion cannot finalize or abandon committed publication");
		}
		else
		{
			physical_available = true;
			currency_transaction_handle_completions(nullptr, 0);
			check(physical_attempts == 2 && physical_successes == 1,
			      "same operation later physically publishes");
			check(acks == 1 && !held && notifications == 1 && !notification_before_ack,
			      "physical retry ACK then notification exactly once");
			check(!currency_transaction_health_copy().pending,
			      "physical retry retires domain entry");
		}
	}
	check(submitted.operation_id.bytes == original_id.bytes,
	      "original operation identity unchanged");
	const int before_physical = physical_attempts, before_notification = notifications,
		  before_acks = acks;
	currency_transaction_handle_completions(&completed, 1);
	check(physical_attempts == before_physical && notifications == before_notification &&
		      acks == before_acks,
	      "duplicate completion has no repeated successful publication");
	if (!held)
		check(!critical_command_coordinator_acknowledge_publication(original_id),
		      "second successful ACK impossible");
	printf("OBSERVE attempts=%d physical=%d notifications=%d ACK_attempts=%d ACK=%d held=%d pending=%llu failures=%d\n",
	       physical_attempts, physical_successes, notifications, ack_attempts, acks, held,
	       static_cast<unsigned long long>(currency_transaction_health_copy().pending),
	       failures);
	return failures ? 1 : 0;
}
'''

SOURCES = (
    "currency_transaction.c", "currency_command.c", "critical_command.c",
    "economic_currency_adapter.c", "economic_accounting_intent.c",
    "economic_gameplay_authority.c", "economic_command_admission.c",
    "economic_accounting_plan.c", "economic_accounting_types.c",
    "coin_transfer_command.c", "item_transfer_command.c", "craft_pouch_mutation.c",
    "chaos_pouch_ledger.c", "coin_transfer_accounting.c", "item_transfer_accounting.c",
    "player_snapshot_codec.c",
)
SCENARIOS = ("physical_retry", "ack_retry", "missing_publisher", "absent_actor",
             "exhaustion", "wallet_only_replay", "missing_admission", "physical_exception",
             "notification_exception", "notification_false", "changed_after_physical", "changed_after_wallet")
SCENARIOS += ("partial_projection_retry", "partial_projection_conflict")


def bounded(command, timeout, **kwargs):
    if kwargs.pop("capture_output", False):
        kwargs["stdout"] = subprocess.PIPE
        kwargs["stderr"] = subprocess.PIPE
    process = subprocess.Popen(command, start_new_session=True, **kwargs)
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except BaseException:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
        raise
    return subprocess.CompletedProcess(command, process.returncode, stdout, stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--split-publication", action="store_true", default=True)
    parser.add_argument("--legacy-before-fix", dest="split_publication", action="store_false")
    parser.add_argument("--flatfile", action="store_true")
    parser.add_argument("--compile-only", type=Path)
    parser.add_argument("--run-binary", type=Path)
    parser.add_argument("--binary-sha256")
    args = parser.parse_args()
    if args.compile_only and args.run_binary:
        parser.error("select one supplied-binary mode")
    if args.run_binary and not args.binary_sha256:
        parser.error("supplied binary requires its exact SHA256")
    with tempfile.TemporaryDirectory(prefix="coin-publication-ack-") as directory:
        source = Path(directory) / "owner.cpp"
        source.write_text(HARNESS, encoding="utf-8")
        binary = args.run_binary or args.compile_only or Path(directory) / "owner"
        if not args.run_binary:
            if args.compile_only and (binary.exists() or binary.with_suffix(".json").exists()):
                raise RuntimeError("unique artifact output already exists")
            cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
            libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
            command = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                       "-g", "-O1", "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
                       "-pthread", "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
                       *(["-DDURIS_COIN_SPLIT_PUBLICATION_TEST"] if args.split_publication else []),
                       *(["-D__NO_MYSQL__", "-Isrc/no_mysql"] if args.flatfile else []),
                       "-Isrc", *cflags, str(source), *[rel(name) for name in SOURCES],
                       "-Wl,--gc-sections", "-lcrypto", *libs, "-o", str(binary)]
            pins = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in sorted((ROOT / "src").rglob("*"))
                    if path.is_file() and path.suffix in (".c", ".h", ".hpp", ".cpp")}
            started = time.monotonic()
            result = bounded(command, 300, cwd=ROOT)
            if result.returncode:
                if binary.exists():
                    binary.unlink()
                raise subprocess.CalledProcessError(result.returncode, command)
            if args.compile_only:
                metadata = {"binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                            "compile_seconds": time.monotonic() - started, "compile_budget_seconds": 300,
                            "source_inputs": pins, "harness_sha256": hashlib.sha256(HARNESS.encode()).hexdigest(),
                            "linked_sources": list(SOURCES), "flags": command[1:command.index(str(source))],
                            "scope": "native component; controlled coordinator/world; no external services"}
                binary.with_suffix(".json").write_text(json.dumps(metadata, sort_keys=True, indent=2) + "\n")
                print(json.dumps({k: metadata[k] for k in ("binary_sha256", "compile_seconds", "compile_budget_seconds")}))
                return
        if args.run_binary and hashlib.sha256(binary.read_bytes()).hexdigest() != args.binary_sha256:
            raise RuntimeError("supplied binary hash mismatch")
        failed = []
        for scenario in SCENARIOS:
            result = bounded([str(binary), scenario], 30, capture_output=True, text=True)
            print(f"{scenario}: {'PASS' if result.returncode == 0 else 'FAIL'}\n{result.stdout}{result.stderr}", flush=True)
            if result.returncode:
                failed.append(scenario)
        if failed:
            raise AssertionError("Coin ACK publication contract failed: " + ", ".join(failed))


if __name__ == "__main__":
    main()
