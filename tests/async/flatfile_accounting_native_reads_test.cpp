#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_store.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <new>
#include <openssl/sha.h>
#include <unistd.h>

namespace fs = std::filesystem;
size_t allocation_target = 0, allocation_seen = 0;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__real__Znam(size_t);
extern "C" void *__wrap__Znwm(size_t count)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znwm(count);
}
extern "C" void *__wrap__Znam(size_t count)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znam(count);
}
using domain_result = flatfile_player_domain_result;
using identity_result = flatfile_identity_result;
critical_command command(uint8_t operation, uint64_t wallet_revision, uint64_t bank_revision)
{
	critical_operation_id id = {};
	id.bytes[0] = operation;
	currency_command_payload payload = {};
	payload.pid = 1;
	payload.racewar = 1;
	payload.reason = currency_reason_type::atm_withdraw;
	strcpy(payload.account_name.data(), "account-one");
	payload.wallet_delta.amount[0] = 1;
	payload.bank_delta.amount[0] = -1;
	critical_command cmd;
	assert(currency_command_build(&cmd, id, payload, wallet_revision, bank_revision,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	cmd.accepted_at_usec = 1;
	assert(critical_command_normalize(&cmd));
	return cmd;
}
std::vector<uint8_t> read(const fs::path &directory, const std::string &name)
{
	std::vector<uint8_t> bytes;
	assert(flatfile_read(directory.string(), name, 64 * 1024 * 1024, &bytes, nullptr) ==
	       flatfile_read_result::ok);
	return bytes;
}
flatfile_player_domain_record sentinel()
{
	flatfile_player_domain_record record;
	record.pid = -7;
	record.account_name = "unchanged";
	record.recent_pvp_deaths = { 123 };
	return record;
}
bool unchanged(const flatfile_player_domain_record &record)
{
	return record.pid == -7 && record.account_name == "unchanged" &&
	       record.recent_pvp_deaths == std::vector<int64_t>{ 123 };
}
int main(int argc, char **argv)
{
	assert(argc == 2);
	// Deadlock is a test failure rather than a permanently hung runner.
	alarm(60);
	const std::string root = argv[1];
	const fs::path domains = fs::path(root) / "domains";
	const fs::path names = fs::path(root) / "identities/names";
	for (const auto &path : { fs::path(root), domains, fs::path(root) / "identities", names })
	{
		fs::create_directories(path);
		fs::permissions(path, fs::perms::owner_all);
	}
	int32_t pid = 0;
	assert(flatfile_identity_allocate_pid(root, &pid, nullptr) == identity_result::ok &&
	       pid == 1);
	assert(flatfile_identity_claim(root, pid, "Player", "account-one", nullptr) ==
	       identity_result::ok);
	flatfile_player_domain_record baseline;
	baseline.pid = 1;
	baseline.account_name = "account-one";
	baseline.racewar = 1;
	baseline.domains.wallet = { 4, 3, 2, 1 };
	baseline.domains.bank = { 10, 9, 8, 7 };
	baseline.recent_pvp_deaths = { 100, 90 };
	baseline.completed_epic_zones = { 5, 10 };
	assert(flatfile_player_domain_establish(root, baseline, nullptr) == domain_result::ok);
	flatfile_player_domain_record state;
	assert(flatfile_player_domain_load(root, 1, "account-one", 1, &state, nullptr) ==
	       domain_result::ok);
	critical_command last;
	for (bool legacy : { true, false })
	{
		last = command(legacy ? 1 : 2, state.domains.wallet_revision,
			       state.domains.bank_revision);
		if (legacy)
			setenv("DURIS_FLATFILE_TEST_LEGACY_TRANSACTION", "1", 1);
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_BANK", "1", 1);
		const auto result = flatfile_player_domain_apply(root, last);
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_BANK");
		unsetenv("DURIS_FLATFILE_TEST_LEGACY_TRANSACTION");
		assert(result.outcome == critical_apply_outcome::retryable_failure);
		const auto journal =
			domains / (legacy ? ".currency-transaction" : ".player-domain-transaction");
		assert(fs::exists(journal));
		const auto journal_bytes = read(domains, journal.filename().string());
		flatfile_identity_lock identity;
		flatfile_authority_lock authority, unlocked;
		assert(identity.acquire(root, nullptr) && authority.acquire(root, nullptr));
		auto output = sentinel();
		assert(flatfile_player_domain_load_locked(root, unlocked, 1, "account-one", 1,
							  &output,
							  nullptr) == domain_result::invalid);
		assert(unchanged(output));
		assert(flatfile_player_domain_recover_locked(root + "-other", authority, nullptr) ==
		       domain_result::invalid);
		assert(read(domains, journal.filename().string()) == journal_bytes);
		std::optional<flatfile_legacy_domain_receipt> receipt;
		// Reading a receipt must recover the complete legacy transaction first.
		assert(flatfile_player_domain_legacy_receipt_locked(root, authority, 1,
								    last.operation_id, &receipt,
								    nullptr) == domain_result::ok);
		assert(receipt && !receipt->result_code && !fs::exists(journal));
		std::vector<uint8_t> encoded;
		assert(critical_command_encode(last, &encoded) ==
		       critical_command_codec_result::ok);
		std::array<uint8_t, 32> digest;
		SHA256(encoded.data(), encoded.size(), digest.data());
		assert(receipt->command_digest == digest);
		auto changed = last;
		++changed.accepted_at_usec;
		assert(critical_command_encode(changed, &encoded) ==
		       critical_command_codec_result::ok);
		SHA256(encoded.data(), encoded.size(), digest.data());
		assert(receipt->command_digest != digest);
		currency_command_result decoded;
		assert(currency_command_decode_result(receipt->result.data(), receipt->result_size,
						      &decoded));
		assert(flatfile_player_domain_load_locked(root, authority, 1, "ACCOUNT-ONE", 1,
							  &state, nullptr) == domain_result::ok);
		assert(state.domains.wallet[0] == static_cast<uint64_t>(decoded.wallet.amount[0]) &&
		       state.domains.bank[0] == static_cast<uint64_t>(decoded.bank.amount[0]));
		assert(state.domains.wallet_revision == decoded.wallet_revision &&
		       state.domains.bank_revision == decoded.bank_revision);
		assert(state.recent_pvp_deaths == baseline.recent_pvp_deaths &&
		       state.completed_epic_zones == baseline.completed_epic_zones);
		assert(flatfile_player_domain_load_locked(root, authority, 1, "wrong-account", 1,
							  &output,
							  nullptr) == domain_result::conflict &&
		       unchanged(output));
		auto absent = last.operation_id;
		absent.bytes[0] = 99;
		assert(flatfile_player_domain_legacy_receipt_locked(root, authority, 1, absent,
								    &receipt,
								    nullptr) == domain_result::ok &&
		       !receipt);
		receipt.emplace();
		receipt->result_code = 123;
		assert(flatfile_player_domain_legacy_receipt_locked(root, authority, 999, absent,
								    &receipt, nullptr) ==
			       domain_result::not_found &&
		       receipt->result_code == 123);
		// Receipt reads do not depend on the current shared bank's existence.
		const auto bank = domains / "bank-account-one-1.domain";
		fs::rename(bank, domains / "bank-hidden");
		assert(flatfile_player_domain_legacy_receipt_locked(root, authority, 1,
								    last.operation_id, &receipt,
								    nullptr) == domain_result::ok &&
		       receipt);
		assert(flatfile_player_domain_load_locked(root, authority, 1, "account-one", 1,
							  &output,
							  nullptr) == domain_result::not_found &&
		       unchanged(output));
		fs::rename(domains / "bank-hidden", bank);
	}
	{
		flatfile_identity_lock identity, unlocked_identity;
		flatfile_authority_lock authority, unlocked_authority;
		assert(identity.acquire(root, nullptr) && authority.acquire(root, nullptr));
		flatfile_authority_operation removal;
		assert(flatfile_identity_prepare_remove(root, identity, authority, 1, "Player",
							&removal, nullptr) == identity_result::ok);
		flatfile_authority_operation first;
		first.filename = "borrowed-read-marker";
		first.bytes = { 1 };
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
		assert(flatfile_authority_transaction_commit_operations(
			       root, authority, { first, removal }, nullptr) ==
		       flatfile_authority_transaction_result::io_error);
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
		const auto journal = read(domains, ".critical-authority-transaction");
		flatfile_identity_record out;
		out.pid = -7;
		out.name = "unchanged";
		assert(flatfile_identity_lookup_pid_locked(root, unlocked_identity, authority, 1,
							   &out,
							   nullptr) == identity_result::invalid);
		assert(flatfile_identity_lookup_pid_locked(root, identity, unlocked_authority, 1,
							   &out,
							   nullptr) == identity_result::invalid);
		assert(flatfile_identity_lookup_pid_locked(root + "-other", identity, authority, 1,
							   &out,
							   nullptr) == identity_result::invalid);
		assert(out.pid == -7 && out.name == "unchanged" &&
		       read(domains, ".critical-authority-transaction") == journal);
		assert(flatfile_identity_lookup_pid_locked(root, identity, authority, 1, &out,
							   nullptr) == identity_result::ok);
		assert(out.pid == 1 && !out.active && out.blocked &&
		       !fs::exists(domains / ".critical-authority-transaction"));
		assert(flatfile_player_domain_recover_locked(root, authority, nullptr) ==
		       domain_result::ok);
		size_t failures[3] = {};
		for (size_t which = 0; which < 3; ++which)
		{
			bool complete = false;
			for (size_t nth = 1; nth < 256; ++nth)
			{
				auto domain_out = sentinel();
				flatfile_identity_record identity_out;
				identity_out.pid = -7;
				identity_out.name = "unchanged";
				std::optional<flatfile_legacy_domain_receipt> receipt;
				receipt.emplace();
				receipt->result_code = 123;
				allocation_seen = 0;
				allocation_target = nth;
				unsigned status;
				if (which == 0)
					status = static_cast<unsigned>(
						flatfile_identity_lookup_pid_locked(
							root, identity, authority, 1, &identity_out,
							nullptr));
				else if (which == 1)
					status = static_cast<unsigned>(
						flatfile_player_domain_load_locked(
							root, authority, 1, "account-one", 1,
							&domain_out, nullptr));
				else
					status = static_cast<unsigned>(
						flatfile_player_domain_legacy_receipt_locked(
							root, authority, 1, last.operation_id,
							&receipt, nullptr));
				allocation_target = 0;
				if (allocation_seen < nth)
				{
					assert(status == 0);
					complete = true;
					break;
				}
				assert(status ==
				       static_cast<unsigned>(
					       which == 0 ? static_cast<unsigned>(
								    identity_result::io_error) :
							    static_cast<unsigned>(
								    domain_result::io_error)));
				if (which == 0)
					assert(identity_out.pid == -7 &&
					       identity_out.name == "unchanged");
				else if (which == 1)
					assert(unchanged(domain_out));
				else
					assert(receipt && receipt->result_code == 123);
				++failures[which];
			}
			assert(complete && failures[which]);
		}
		std::cout << "borrowed reads: allocation failures " << failures[0] << "/"
			  << failures[1] << "/" << failures[2] << " preserve outputs\n";
		auto bytes = read(domains, "player-1.domain");
		bytes.back() ^= 1;
		assert(flatfile_atomic_write(domains.string(), "player-1.domain", bytes, nullptr));
		auto domain_out = sentinel();
		std::optional<flatfile_legacy_domain_receipt> receipt;
		receipt.emplace();
		receipt->result_code = 123;
		assert(flatfile_player_domain_legacy_receipt_locked(
			       root, authority, 1, last.operation_id, &receipt, nullptr) ==
			       domain_result::invalid &&
		       receipt->result_code == 123);
		assert(flatfile_player_domain_load_locked(root, authority, 1, "account-one", 1,
							  &domain_out,
							  nullptr) == domain_result::invalid &&
		       unchanged(domain_out));
		bytes = read(names, "catalog.identity");
		bytes.back() ^= 1;
		assert(flatfile_atomic_write(names.string(), "catalog.identity", bytes, nullptr));
		out.pid = -7;
		assert(flatfile_identity_lookup_pid_locked(root, identity, authority, 1, &out,
							   nullptr) == identity_result::invalid &&
		       out.pid == -7);
		// A malformed pending journal must block all reads, even when files exist.
		assert(flatfile_atomic_write(domains.string(), ".critical-authority-transaction",
					     { 1, 2, 3 }, nullptr));
		assert(flatfile_player_domain_recover_locked(root, authority, nullptr) ==
		       domain_result::invalid);
	}
	std::cout
		<< "flatfile borrowed reads: lock ownership, all journal recovery, retained identity and exact legacy receipts passed\n";
}
