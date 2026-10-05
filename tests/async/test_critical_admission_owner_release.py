#!/usr/bin/env python3
"""Actual journal/coordinator admission failures retain or release domain owners.

Native persistence-component test with private disposable journal directories.
No database, Redis, live game, or production accounting activation. World leaves
and the execution worker are controlled; journal/coordinator/domain code is real.
"""

from pathlib import Path
import argparse
import ast
import hashlib
import json
import os
import shlex
import signal
import subprocess
import tempfile
import time

from _paths import ROOT, rel


def fixture_literal(name):
    tree = ast.parse((ROOT / "tests/async" / name).read_text(encoding="utf-8"))
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(
            isinstance(target, ast.Name) and target.id == "HARNESS" for target in node.targets
        ):
            return ast.literal_eval(node.value)
    raise AssertionError("maintained native fixture HARNESS missing")


FAULTS = r'''

#include <atomic>
#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
static std::atomic<int> write_fault{ 0 }, sync_faults{ 0 };
static std::atomic<bool> sync_blocked{ false };
static std::atomic<unsigned int> apply_calls{ 0 };
static int failures = 0;
extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_fsync(int);
extern "C" ssize_t __wrap_write(int fd, const void *data, size_t size)
{
	const int flags = fcntl(fd, F_GETFL);
	if (flags >= 0 && (flags & O_APPEND))
	{
		const int fault = write_fault.exchange(0);
		if (fault)
		{
			if (fault == 2)
			{
				assert(size > 1);
				assert(__real_write(fd, data, size / 2) > 0);
			}
			errno = ENOSPC;
			return -1;
		}
	}
	return __real_write(fd, data, size);
}
extern "C" int __wrap_fsync(int fd)
{
	struct stat status = {};
	if (fstat(fd, &status) == 0 && S_ISREG(status.st_mode))
	{
		if (sync_blocked.load())
		{
			errno = EIO;
			return -1;
		}
		int count = sync_faults.load();
		while (count && !sync_faults.compare_exchange_weak(count, count - 1))
		{
		}
		if (count)
		{
			errno = EIO;
			return -1;
		}
	}
	return __real_fsync(fd);
}
static void check(bool condition, const char *label)
{
	printf("%s %s\n", condition ? "PASS" : "FAIL", label);
	if (!condition)
		++failures;
}
template <typename Predicate> static void wait_for(Predicate ready)
{
	const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!ready())
	{
		assert(std::chrono::steady_clock::now() < until);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}
static critical_completion native_completion()
{
	critical_completion completion = {};
	wait_for([&] { return critical_command_coordinator_pulse(&completion, 1) == 1; });
	return completion;
}
static size_t replay_count()
{
	size_t count = 0;
	assert(critical_command_journal_replay(
		       [](critical_command, void *context)
		       {
			       ++*static_cast<size_t *>(context);
			       return true;
		       },
		       &count) == critical_command_journal_result::ok);
	return count;
}
static void arm_fault(const std::string &scenario)
{
	if (scenario.find("partial") != std::string::npos)
		write_fault = 2;
	else if (scenario.find("sync_failure") != std::string::npos)
		sync_faults = 1;
	else if (scenario.find("uncertain") != std::string::npos)
		sync_blocked = true;
	else if (scenario.find("reject") == std::string::npos &&
		 scenario.find("conflict") == std::string::npos &&
		 scenario.find("quota") == std::string::npos &&
		 scenario.find("applied") == std::string::npos)
		write_fault = 1;
}
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
static critical_completion malformed(critical_completion value, const std::string &scenario)
{
	value.disposition = critical_completion_disposition::never_admitted;
	if (scenario.find("unknown") != std::string::npos)
		value.disposition = static_cast<critical_completion_disposition>(255);
	else if (scenario.find("outcome") != std::string::npos)
		value.outcome = critical_apply_outcome::applied;
	else if (scenario.find("error") != std::string::npos)
		value.error_code = 0;
	else if (scenario.find("revision") != std::string::npos)
		value.durable_revision = 1;
	else if (scenario.find("started") != std::string::npos)
		value.started_at_usec = 1;
	else if (scenario.find("stage") != std::string::npos)
		value.failure_stage = critical_failure_stage::coin_source_wallet_revision;
	else if (scenario.find("size") != std::string::npos)
		value.result_size = 1;
	else if (scenario.find("tail") != std::string::npos)
		value.result_payload.back() = 1;
	else
		assert(false);
	assert(!critical_completion_disposition_valid(value));
	return value;
}
static critical_completion conflicting(critical_completion value)
{
	value.disposition = critical_completion_disposition::never_admitted;
	value.outcome = critical_apply_outcome::terminal_failure;
	value.error_code = EIO;
	value.durable_revision = 0;
	value.started_at_usec = 0;
	value.failure_stage = critical_failure_stage::none;
	value.result_size = 0;
	value.result_payload = {};
	assert(critical_completion_disposition_valid(value));
	return value;
}
#endif
'''

CURRENCY_HEAD = r'''

#include "core/utils.h"
#include "economy/account_bank_balances.h"
#include "economy/currency_transaction.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/economic_command_admission.h"
#include "player/player_snapshot_codec.h"
#include "persistence/critical_command_journal.h"
#include "sql/sql_player.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
P_desc descriptor_list = nullptr;
P_char character_list = nullptr;
static P_char online = nullptr, other = nullptr;
static constexpr uint64_t pile_uid = 99001;
static int notifications = 0, projections = 0;
static unsigned int notified_error = 0;
static bool force_rejection = false;
static bool force_applied = false;
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
void gmcp_char_vitals(P_char)
{
	++projections;
}
void send_to_char(const char *, P_char) {}
void logit(const char *, const char *, ...) {}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
}
void publish_account_bank_balances_revision(const char *, int, const AccountBankBalances *,
					    uint64_t)
{
	++projections;
}
static bool coin_notify(P_char actor, bool committed, const coin_transfer_payload &,
			const coin_transfer_result &, unsigned int error, const uint8_t *, size_t)
{
	assert(actor && committed == force_applied);
	++notifications;
	notified_error = error;
	return true;
}
static void bank_notify(P_char actor, bool committed, const currency_command_result &,
			unsigned int error, const uint8_t *, size_t)
{
	assert(actor && committed == force_applied);
	++notifications;
	notified_error = error;
}
static critical_apply_result execute(const critical_command &command, void *)
{
	++apply_calls;
	assert(critical_command_envelope_valid(command));
	assert(economic_command_admission_supported(command));
	if (!force_applied)
	{
		assert(force_rejection);
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	}
	critical_apply_result applied = { critical_apply_outcome::applied, 2, 0 };
	if (command.type == critical_command_type::coin_transfer)
	{
		coin_transfer_payload payload = {};
		assert(coin_transfer_command_decode_payload(command, &payload));
		coin_transfer_result result = {};
		const coin_transfer_endpoint *endpoints[] = { &payload.source,
							      &payload.destination };
		for (size_t index = 0; index < 2; ++index)
		{
			for (size_t denomination = 0; denomination < 4; ++denomination)
				result.wallets[index].wallet.amount[denomination] =
					endpoints[index]->after[denomination];
			result.wallets[index].wallet_revision =
				result.wallets[index].bank_revision = 2;
		}
		std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> bytes = {};
		assert(coin_transfer_command_encode_result(payload, result, &bytes));
		applied.result_size = bytes.size();
		std::copy(bytes.begin(), bytes.end(), applied.result_payload.begin());
	}
	else
	{
		currency_command_result result = {};
		result.wallet.amount[0] = 4;
		result.bank.amount[0] = 1;
		result.wallet_revision = result.bank_revision = 2;
		std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> bytes = {};
		assert(currency_command_encode_result(result, &bytes));
		applied.result_size = bytes.size();
		std::copy(bytes.begin(), bytes.end(), applied.result_payload.begin());
	}
	return applied;
}
'''

CURRENCY_MAIN = r'''

int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string scenario = argv[1];
	assert(!std::filesystem::exists(argv[2]));
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
	assert(critical_command_coordinator_init(argv[2], execute, nullptr, 1, nullptr, nullptr,
						 economic_command_admission_supported));
	if (scenario.find("quota") != std::string::npos)
	{
		critical_command_journal_shutdown();
		assert(critical_command_journal_init(argv[2], 1));
	}
	const bool bank = scenario.starts_with("bank_");
	force_rejection = scenario.find("reject") != std::string::npos ||
			  scenario.find("conflict") != std::string::npos;
	force_applied = scenario.find("applied") != std::string::npos;
	arm_fault(scenario);
	if (bank)
		assert(currency_transaction_submit(
			&actor, { { -1, 0, 0, 0 } }, { { 1, 0, 0, 0 } },
			currency_reason_type::atm_deposit, 0, critical_source_site::command,
			critical_deadline_class::interactive, bank_notify, nullptr, 0));
	else
	{
		const auto payload = transfer(&actor, true);
		assert(currency_transaction_submit_coin(&actor, payload, coin_notify, nullptr, 0));
	}
	critical_operation_id original = {};
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::player, 42 },
						      &original));
	if (scenario.find("uncertain") != std::string::npos)
	{
		wait_for(
			[&]
			{
				return critical_command_coordinator_durability(original) ==
				       critical_command_durability::uncertain;
			});
		currency_transaction_handle_completions(nullptr, 0);
		critical_operation_id retained = {};
		check(critical_command_coordinator_is_fenced({ critical_entity_type::player, 42 },
							     &retained) &&
			      retained.bytes == original.bytes,
		      "uncertain append retains original coordinator fence");
		check(currency_transaction_health_copy().pending == 1 &&
			      currency_transaction_player_busy(&actor),
		      "uncertain append retains domain owner");
		check(!apply_calls && !notifications && !projections,
		      "uncertain append cannot execute notify or project");
		force_rejection = true;
		sync_blocked = false;
		assert(critical_command_coordinator_recover_uncertain());
		const auto recovered = native_completion();
		check(recovered.operation_id.bytes == original.bytes && apply_calls == 1 &&
			      replay_count() == 1,
		      "uncertain append recovers execution under original operation ID");
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
		assert(recovered.disposition == critical_completion_disposition::execution);
#endif
		currency_transaction_handle_completions(&recovered, 1);
		check(!currency_transaction_health_copy().pending && notifications == 1 &&
			      replay_count() == 0 &&
			      !critical_command_coordinator_is_fenced(
				      { critical_entity_type::player, 42 }, nullptr),
		      "uncertain recovery releases only after original execution ACK");
	}
	else
	{
		if (!force_rejection && !force_applied)
		{
			wait_for(
				[&]
				{
					return critical_command_coordinator_durability(original) ==
					       critical_command_durability::failed;
				});
			assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
			check(critical_command_coordinator_is_fenced(
				      { critical_entity_type::player, 42 }, nullptr),
			      "zero-capacity pulse retains non-admission delivery and fence");
		}
		const auto completion = native_completion();
		assert(completion.operation_id.bytes == original.bytes);
		assert(completion.outcome == (force_applied ?
						      critical_apply_outcome::applied :
						      critical_apply_outcome::terminal_failure));
		if (force_rejection || force_applied)
		{
			assert(completion.started_at_usec && apply_calls == 1);
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
			assert(completion.disposition ==
			       critical_completion_disposition::execution);
#endif
			sync_blocked = true;
			currency_transaction_handle_completions(&completion, 1);
			check(currency_transaction_health_copy().pending == 1 && !notifications,
			      "durable rejection ACK failure retains domain owner without notification");
			check(critical_command_coordinator_is_fenced(
				      { critical_entity_type::player, 42 }, nullptr) &&
				      replay_count() == 1,
			      "durable rejection ACK failure retains real journal and fence");
			sync_blocked = false;
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
			if (scenario.find("conflict") != std::string::npos)
			{
				const auto conflict = scenario.find("unknown") !=
								      std::string::npos ?
							      malformed(completion, "unknown") :
							      conflicting(completion);
				if (scenario.find("batch") != std::string::npos)
				{
					const critical_completion batch[] = { conflict,
									      completion };
					currency_transaction_handle_completions(batch, 2);
				}
				else
					currency_transaction_handle_completions(&conflict, 1);
				currency_transaction_player_ready(&actor);
				currency_transaction_handle_completions(nullptr, 0);
				check(!notifications &&
					      currency_transaction_health_copy().pending == 1 &&
					      critical_command_coordinator_is_fenced(
						      { critical_entity_type::player, 42 },
						      nullptr) &&
					      replay_count() == 1,
				      "conflicting disposition cannot use repaired ACK or reconnect to release execution");
				currency_transaction_handle_completions(&completion, 1);
			}
			else
#endif
				currency_transaction_handle_completions(nullptr, 0);
			check(notifications == 1 &&
				      notified_error == (force_applied ? 0 : EINVAL) &&
				      !currency_transaction_health_copy().pending,
			      "real durable rejection completes once after journal checkpoint repair");
			check(!critical_command_coordinator_is_fenced(
				      { critical_entity_type::player, 42 }, nullptr) &&
				      replay_count() == 0,
			      "repaired execution ACK removes original real journal record and fence");
		}
		else
		{
			assert(!completion.started_at_usec && !apply_calls &&
			       !completion.result_size);
			assert(!critical_command_coordinator_is_fenced(
				{ critical_entity_type::player, 42 }, nullptr));
			assert(replay_count() == 0 &&
			       critical_command_journal_health_copy().records == 0);
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
			assert(completion.disposition ==
			       critical_completion_disposition::never_admitted);
			assert(critical_completion_disposition_valid(completion));
			if (scenario.find("malformed") != std::string::npos)
			{
				const auto invalid = malformed(completion, scenario);
				currency_transaction_handle_completions(&invalid, 1);
				currency_transaction_player_ready(&actor);
				currency_transaction_handle_completions(nullptr, 0);
				check(currency_transaction_health_copy().pending == 1 &&
					      !notifications && !projections,
				      "malformed never-admitted disposition cannot discharge owner");
			}
#endif
			currency_transaction_handle_completions(&completion, 1);
			check(!currency_transaction_health_copy().pending &&
				      !currency_transaction_player_busy(&actor),
			      "definite never-admission releases domain owner without impossible ACK");
			check(notifications == 1 && notified_error == completion.error_code,
			      "definite never-admission notifies exact rejection once");
		}
		currency_transaction_handle_completions(&completion, 1);
		currency_transaction_handle_completions(nullptr, 0);
		check(notifications == 1, "duplicate native failure cannot notify twice");
		if (force_applied)
			check(GET_COPPER(&actor) == 4 && GET_COPPER(&recipient) == (bank ? 0 : 1) &&
				      player.wallet_revision == 2,
			      "applied root publishes exact balances after repaired real ACK");
		else
			check(!projections && GET_COPPER(&actor) == 5 &&
				      GET_COPPER(&recipient) == 0 && player.wallet_revision == 1 &&
				      player.bank_revision == 1,
			      "failure does not publish or mutate money");
	}
	check(critical_command_coordinator_shutdown(), "private coordinator shutdown completes");
	printf("OBSERVE worker=%u callbacks=%d projections=%d pending=%llu failures=%d\n",
	       apply_calls.load(), notifications, projections,
	       static_cast<unsigned long long>(currency_transaction_health_copy().pending),
	       failures);
	currency_transaction_reset_for_tests();
	return failures ? 1 : 0;
}
'''


def currency_harness():
    old = fixture_literal("test_coin_publication_ack_retention.py")
    authority = old[old.index("class economic_gameplay_authority_test_access"):old.index("[[noreturn]]")]
    builder = old[old.index("static coin_transfer_payload transfer("):old.index("static critical_completion receipt(")]
    return CURRENCY_HEAD[:CURRENCY_HEAD.index("static critical_apply_result execute(")] + authority + FAULTS + \
        CURRENCY_HEAD[CURRENCY_HEAD.index("static critical_apply_result execute("):] + builder + CURRENCY_MAIN


ITEM_MAIN = r'''

static unsigned int notified_error = 0;
static bool force_rejection = false;
static critical_apply_result admission_worker(const critical_command &command, void *)
{
	++apply_calls;
	assert(critical_command_envelope_valid(command));
	item_transfer_payload decoded = {};
	assert(item_transfer_command_decode_payload(command, &decoded));
	assert(force_rejection);
	return { critical_apply_outcome::terminal_failure, 0, EINVAL };
}
static void rejection_callback(P_char actor, bool committed, const item_transfer_result &,
			       unsigned int error, const uint8_t *, size_t)
{
	assert(actor && !committed);
	++completion_calls;
	notified_error = error;
}
static bool rejection_publication(const critical_operation_id &, P_char actor, bool committed,
				  const item_transfer_result &, unsigned int error, const uint8_t *,
				  size_t)
{
	assert(actor && !committed);
	++publication_attempts;
	notified_error = error;
	return true;
}
int main(int argc, char **argv)
{
	assert(argc == 3 && !std::filesystem::exists(argv[2]));
	fixture_check_runtime_identity_retirement();
	const std::string scenario = argv[1];
	const bool craft = scenario.starts_with("craft_");
	index_data indexes[1] = {};
	indexes[0].virtual_number = 42;
	obj_index = indexes;
	pc_only_data player = {};
	player.pid = 1001;
	char_data actor = {};
	actor.only.pc = &player;
	actor.runtime_id = 7001;
	fixture_character_registration identity(&actor);
	character_list = &actor;
	obj_data object = {}, output = {};
	object.obj_uid = 5001;
	object.R_num = 0;
	object.loc_p = LOC_CARRIED;
	object.loc.carrying = &actor;
	output.obj_uid = 5030;
	output.R_num = 0;
	output.loc_p = LOC_NOWHERE;
	output.next = &object;
	object_list = craft ? &output : &object;
	runtime_entry = { 5001, 5001, 0,  { item_owner_type::player, 1001, 0 },
			  1,	1,    42, item_custody_state::active };
	assert(!economic_gameplay_authority::active());
	assert(critical_command_coordinator_init(argv[2], admission_worker, nullptr, 1, nullptr,
						 nullptr,
						 item_transfer_accounting_command_supported));
	if (scenario.find("quota") != std::string::npos)
	{
		critical_command_journal_shutdown();
		assert(critical_command_journal_init(argv[2], 1));
	}
	force_rejection = scenario.find("reject") != std::string::npos ||
			  scenario.find("conflict") != std::string::npos;
	arm_fault(scenario);
	item_movement_reject reject = item_movement_reject::none;
	if (craft)
	{
		P_obj inputs[] = { &object }, outputs[] = { &output };
		assert(item_movement_transaction_submit_craft(&actor, inputs, 1, outputs, 1, 551,
							      rejection_callback, nullptr, 0,
							      &reject));
	}
	else
		assert(item_movement_transaction_submit(
			&actor, &object, nullptr, runtime_entry.owner,
			{ item_owner_type::player, 2002, 0 }, item_transfer_reason::player_give,
			1001, rejection_callback, nullptr, 0, nullptr, &reject,
			rejection_publication));
	critical_operation_id original = {};
	assert(critical_command_coordinator_is_fenced({ critical_entity_type::item, 5001 },
						      &original));
	if (scenario.find("uncertain") != std::string::npos)
	{
		wait_for(
			[&]
			{
				return critical_command_coordinator_durability(original) ==
				       critical_command_durability::uncertain;
			});
		item_movement_transaction_handle_completions(nullptr, 0);
		critical_operation_id retained = {};
		check(critical_command_coordinator_is_fenced({ critical_entity_type::item, 5001 },
							     &retained) &&
			      retained.bytes == original.bytes,
		      "uncertain append retains original item fence");
		check(item_movement_transaction_health_copy().pending == 1 &&
			      item_movement_transaction_player_busy(&actor),
		      "uncertain append retains actual item owner");
		check(!apply_calls && !completion_calls && !publication_attempts && !extractions,
		      "uncertain append cannot execute publish notify or discard craft output");
		force_rejection = true;
		sync_blocked = false;
		assert(critical_command_coordinator_recover_uncertain());
		const auto recovered = native_completion();
		check(recovered.operation_id.bytes == original.bytes && apply_calls == 1 &&
			      replay_count() == 1,
		      "uncertain item admission recovers execution with exact original ID");
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
		assert(recovered.disposition == critical_completion_disposition::execution);
#endif
		item_movement_transaction_handle_completions(&recovered, 1);
		check(!item_movement_transaction_health_copy().pending && replay_count() == 0 &&
			      !critical_command_coordinator_is_fenced(
				      { critical_entity_type::item, 5001 }, nullptr),
		      "uncertain item recovery releases only after actual execution ACK");
	}
	else
	{
		if (!force_rejection)
		{
			wait_for(
				[&]
				{
					return critical_command_coordinator_durability(original) ==
					       critical_command_durability::failed;
				});
			assert(critical_command_coordinator_pulse(nullptr, 0) == 0);
			check(critical_command_coordinator_is_fenced(
				      { critical_entity_type::item, 5001 }, nullptr),
			      "zero-capacity pulse retains actual item admission failure until delivery");
		}
		const auto completion = native_completion();
		assert(completion.operation_id.bytes == original.bytes &&
		       completion.outcome == critical_apply_outcome::terminal_failure);
		if (force_rejection)
		{
			assert(apply_calls == 1 && completion.started_at_usec);
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
			assert(completion.disposition ==
			       critical_completion_disposition::execution);
#endif
			sync_blocked = true;
			item_movement_transaction_handle_completions(&completion, 1);
			check(item_movement_transaction_health_copy().pending == 1 &&
				      critical_command_coordinator_is_fenced(
					      { critical_entity_type::item, 5001 }, nullptr) &&
				      replay_count() == 1,
			      "durable rejection ACK failure retains real item journal fence and owner");
			const int callbacks_before_repair = completion_calls;
			const int discarded_before_repair = extractions;
			sync_blocked = false;
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
			if (scenario.find("conflict") != std::string::npos)
			{
				const auto conflict = scenario.find("unknown") !=
								      std::string::npos ?
							      malformed(completion, "unknown") :
							      conflicting(completion);
				if (scenario.find("batch") != std::string::npos)
				{
					const critical_completion batch[] = { conflict,
									      completion };
					item_movement_transaction_handle_completions(batch, 2);
				}
				else
					item_movement_transaction_handle_completions(&conflict, 1);
				item_movement_transaction_player_ready(&actor);
				item_movement_transaction_handle_completions(nullptr, 0);
				check(item_movement_transaction_health_copy().pending == 1 &&
					      critical_command_coordinator_is_fenced(
						      { critical_entity_type::item, 5001 },
						      nullptr) &&
					      replay_count() == 1 &&
					      completion_calls == callbacks_before_repair &&
					      extractions == discarded_before_repair,
				      "conflicting disposition staged before retries cannot consume repaired real ACK");
				item_movement_transaction_handle_completions(&completion, 1);
			}
			else
#endif
				item_movement_transaction_handle_completions(nullptr, 0);
			check(!item_movement_transaction_health_copy().pending &&
				      replay_count() == 0 &&
				      !critical_command_coordinator_is_fenced(
					      { critical_entity_type::item, 5001 }, nullptr),
			      "execution rejection releases only after repaired real journal checkpoint");
			check(completion_calls == 1 && extractions == discarded_before_repair,
			      "durable rejection repair completes once without repeated physical craft disposal");
			(void)callbacks_before_repair;
		}
		else
		{
			assert(!apply_calls && !completion.started_at_usec &&
			       !completion.result_size);
			assert(!critical_command_coordinator_is_fenced(
				{ critical_entity_type::item, 5001 }, nullptr));
			assert(replay_count() == 0 &&
			       critical_command_journal_health_copy().records == 0);
#ifdef DURIS_CRITICAL_ADMISSION_DISPOSITION_TEST
			assert(completion.disposition ==
			       critical_completion_disposition::never_admitted);
			assert(critical_completion_disposition_valid(completion));
			if (scenario.find("malformed") != std::string::npos)
			{
				const auto invalid = malformed(completion, scenario);
				item_movement_transaction_handle_completions(&invalid, 1);
				item_movement_transaction_player_ready(&actor);
				item_movement_transaction_handle_completions(nullptr, 0);
				check(item_movement_transaction_health_copy().pending == 1 &&
					      !completion_calls && !publication_attempts &&
					      !extractions,
				      "malformed disposition cannot publish notify or discard retained craft output");
			}
#endif
			item_movement_transaction_handle_completions(&completion, 1);
			check(!item_movement_transaction_health_copy().pending &&
				      !item_movement_transaction_player_busy(&actor),
			      "definite never-admission releases retained item owner without impossible ACK");
			check(completion_calls == 1 && notified_error == completion.error_code,
			      "definite never-admission notifies exact failure once");
			check(extractions == (craft ? 1 : 0) &&
				      (craft ? output.obj_uid == 0 : object.obj_uid == 5001),
			      "never-admitted craft disposes only staged output and preserves live input");
		}
		const int settled_callbacks = completion_calls;
		const int settled_extractions = extractions;
		item_movement_transaction_handle_completions(&completion, 1);
		item_movement_transaction_handle_completions(nullptr, 0);
		check(completion_calls == settled_callbacks && extractions == settled_extractions,
		      "duplicate receipt cannot repeat settled notification or physical disposal");
	}
	check(object.obj_uid == 5001 && OBJ_CARRIED_BY(&object, &actor) && !restores &&
		      !recipe_publications && !recipe_acknowledgements,
	      "rejected admission never moves input or acknowledges uncommitted recipe progression");
	check(critical_command_coordinator_shutdown(), "private coordinator shutdown completes");
	printf("OBSERVE worker=%u callbacks=%d publications=%d discarded=%d pending=%llu failures=%d\n",
	       apply_calls.load(), completion_calls, publication_attempts, extractions,
	       static_cast<unsigned long long>(item_movement_transaction_health_copy().pending),
	       failures);
	item_movement_transaction_reset_for_tests();
	return failures ? 1 : 0;
}
'''


def item_harness():
    old = fixture_literal("test_publication_retention_runtime.py")
    leaves = old[:old.index("critical_apply_result apply_transfer(")]
    leaves = leaves[:leaves.index("bool economic_gameplay_authority::active()")] + \
        leaves[leaves.index("bool collector_death_enrollment_attach("):]
    leaves = leaves.replace("static bool accounting_active = false;", "")
    leaves = leaves.replace("static critical_apply_outcome forced_outcome = critical_apply_outcome::applied;", "")
    return leaves + '\n#include "persistence/critical_command_journal.h"\n#include <string>\n#include <cerrno>\n' + FAULTS + ITEM_MAIN


CURRENCY_SOURCES = (
    "currency_transaction.c", "currency_command.c", "critical_command.c",
    "economic_currency_adapter.c", "economic_accounting_intent.c", "economic_gameplay_authority.c",
    "economic_command_admission.c", "economic_accounting_plan.c", "economic_source_event.c", "economic_accounting_types.c",
    "coin_transfer_command.c", "item_transfer_command.c", "quest_mobile_native_reference.c", "craft_pouch_mutation.c", "chaos_pouch_ledger.c",
    "coin_transfer_accounting.c", "item_transfer_accounting.c", "player_snapshot_codec.c",
    "critical_command_journal.c", "critical_command_coordinator.c",
)
ITEM_SOURCES = (
    "account/character_identity.c", "item/item_movement_transaction.c", "item/item_transfer_command.c", "world/quest_mobile_native_reference.c",
    "craft_pouch_mutation.c", "chaos_pouch_ledger.c", "chaos_pouch_publication.c", "player_snapshot_codec.c",
    "item_transfer_accounting.c", "economic_accounting_types.c", "economic_accounting_plan.c", "economic_source_event.c",
    "economic_accounting_intent.c", "economic_gameplay_authority.c", "critical_command.c",
    "economic_command_admission.c", "economic_currency_adapter.c", "currency_command.c",
    "coin_transfer_command.c", "coin_transfer_accounting.c",
    "persistence/critical_command_journal.c", "persistence/critical_command_coordinator.c",
)


def scenarios(family, typed):
    roots = ("coin", "bank") if family == "currency" else ("item", "craft")
    result = [f"{root}_{fault}" for root in roots for fault in
              ("enospc", "partial", "sync_failure", "quota", "uncertain", "reject_ack_retry")]
    if family == "currency":
        result.extend(f"{root}_applied_ack_retry" for root in roots)
    if typed:
        result.extend(f"{root}_malformed_{field}" for root in roots for field in
                      ("unknown", "outcome", "error", "revision", "started", "stage", "size", "tail"))
        result.extend(f"{root}_{fault}" for root in roots for fault in
                      ("conflict_ack_pending", "conflict_unknown_ack_pending", "conflict_batch_ack_pending"))
    return result


def bounded(command, timeout, **kwargs):
    if kwargs.pop("capture_output", False):
        kwargs["stdout"] = subprocess.PIPE
        kwargs["stderr"] = subprocess.PIPE
    process = subprocess.Popen(command, start_new_session=True, **kwargs)
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except BaseException:
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.communicate()
        raise
    return subprocess.CompletedProcess(command, process.returncode, stdout, stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", choices=("currency", "item", "both"), default="both")
    parser.add_argument("--legacy-before-fix", action="store_true")
    parser.add_argument("--flatfile", action="store_true")
    parser.add_argument("--compile-only", type=Path)
    parser.add_argument("--run-binary", type=Path)
    parser.add_argument("--binary-sha256")
    args = parser.parse_args()
    if args.compile_only and args.run_binary:
        parser.error("select one artifact mode")
    if (args.compile_only or args.run_binary) and args.family == "both":
        parser.error("supplied artifact mode needs one explicit family")
    if args.run_binary and not args.binary_sha256:
        parser.error("supplied binary requires SHA256")
    families = ("currency", "item") if args.family == "both" else (args.family,)
    failed = []
    with tempfile.TemporaryDirectory(prefix="duris-admission-owner-") as directory:
        private = Path(directory)
        for family in families:
            harness = currency_harness() if family == "currency" else item_harness()
            source = private / f"{family}.cpp"
            source.write_text(harness, encoding="utf-8")
            binary = args.run_binary or args.compile_only or private / family
            if not args.run_binary:
                if args.compile_only and (binary.exists() or binary.with_suffix(".json").exists()):
                    raise RuntimeError("unique artifact target already exists")
                sources = CURRENCY_SOURCES if family == "currency" else ITEM_SOURCES
                cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
                libraries = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
                command = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-g", "-Og",
                           "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                           "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                           "-ffunction-sections", "-fdata-sections",
                           *(["-D__NO_MYSQL__", "-Isrc/no_mysql"] if args.flatfile else []),
                           "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
                           *([] if args.legacy_before_fix else ["-DDURIS_CRITICAL_ADMISSION_DISPOSITION_TEST"]),
                           "-Isrc", "-Itests/async", *cflags, str(source),
                           *[rel(name) for name in sources], "-Wl,--gc-sections", "-lz", "-lcrypto",
                           "-Wl,--wrap=write", "-Wl,--wrap=fsync", *libraries, "-o", str(binary)]
                pins = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                        for path in sorted((ROOT / "src").rglob("*"))
                        if path.is_file() and path.suffix in (".c", ".h", ".hpp", ".cpp")}
                for name in ("test_coin_publication_ack_retention.py", "test_publication_retention_runtime.py",
                             "character_identity_test_fixture.h", "test_critical_admission_owner_release.py"):
                    path = ROOT / "tests/async" / name
                    pins[str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
                started = time.monotonic()
                try:
                    result = bounded(command, 300, cwd=ROOT)
                    if result.returncode:
                        raise subprocess.CalledProcessError(result.returncode, command)
                except BaseException:
                    if binary.exists():
                        binary.unlink()
                    raise
                if args.compile_only:
                    metadata = {"family": family, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                                "compile_seconds": time.monotonic() - started, "compile_budget_seconds": 300,
                                "harness_sha256": hashlib.sha256(harness.encode()).hexdigest(),
                                "source_inputs": pins, "linked_sources": list(sources),
                                "flags": command[1:command.index(str(source))], "typed_disposition": not args.legacy_before_fix,
                                "backend": "flatfile" if args.flatfile else "sql-header",
                                "scope": "actual coordinator+journal/domain owners, controlled worker/world, private journal only"}
                    binary.with_suffix(".json").write_text(json.dumps(metadata, sort_keys=True, indent=2) + "\n")
                    print(json.dumps({key: metadata[key] for key in ("family", "binary_sha256", "compile_seconds")}))
                    return
            if args.run_binary and hashlib.sha256(binary.read_bytes()).hexdigest() != args.binary_sha256:
                raise RuntimeError("supplied binary hash mismatch")
            for scenario in scenarios(family, not args.legacy_before_fix):
                journal = private / (family + "-" + scenario)
                result = bounded([str(binary), scenario, str(journal)], 30, text=True, capture_output=True)
                print(f"{family}/{scenario}: {'PASS' if result.returncode == 0 else 'FAIL'}\n{result.stdout}{result.stderr}", flush=True)
                if result.returncode:
                    failed.append(f"{family}/{scenario}")
        if failed:
            raise AssertionError("Admission owner regression failed: " + ", ".join(failed))


if __name__ == "__main__":
    main()
