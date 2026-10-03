#include "flatfile/flatfile_accounting_bank_transaction.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_baseline.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/currency_flatfile_mutation_writer.h"
#include "flatfile/flatfile_store.h"
#include "economy/economic_currency_adapter.h"
#include "economy/economic_command_admission.h"
#include "economy/economic_gameplay_authority.h"
#include "player/player_snapshot_codec.h"
#include "persistence/critical_command_coordinator.h"
#include "world/epic_command.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <new>
#include <openssl/sha.h>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

class economic_gameplay_authority_test_access
{
    public:
	static economic_accounting_error
	install(const critical_operation_id &lineage, const critical_operation_id &epoch,
		const critical_operation_id &receipt,
		std::span<const economic_gameplay_wallet_mapping> wallets,
		std::span<const economic_gameplay_bank_mapping> banks)
	{
		return economic_gameplay_authority::install(lineage, epoch, receipt, wallets,
							    banks);
	}
	static void reset() { economic_gameplay_authority::reset_for_tests(); }
};

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto initialize_native =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto initialize_evidence =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto create = &flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto rename = &flatfile_accounting_authority_storage::rename_bank;
	static constexpr auto retire = &flatfile_accounting_authority_storage::retire_mapping;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;
	static constexpr auto stage = &flatfile_accounting_storage::stage;
	static constexpr auto verify_source_claim =
		&flatfile_accounting_storage::verify_source_claim;
	static constexpr auto commit = &flatfile_accounting_storage::commit;
	static constexpr auto baseline_initialize =
		&flatfile_accounting_baseline_storage::initialize;
	static constexpr auto baseline_stage = &flatfile_accounting_baseline_storage::stage;
	static constexpr auto native_stage = &currency_flatfile_mutation_writer::stage;
};
using access_type = flatfile_accounting_test_access;
using ops = std::vector<flatfile_authority_operation>;
using bytes = std::vector<uint8_t>;
using outcome = critical_apply_outcome;
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
critical_operation_id id(uint64_t n)
{
	critical_operation_id value = {};
	value.bytes[0] = 7;
	for (size_t i = 0; i < 8; ++i)
		value.bytes[i + 1] = static_cast<uint8_t>(n >> (i * 8));
	return value;
}
bytes read(const fs::path &path)
{
	bytes value;
	assert(flatfile_read(path.parent_path().string(), path.filename().string(), 8 * 1024 * 1024,
			     &value, nullptr) == flatfile_read_result::ok);
	return value;
}
void write(const fs::path &path, const bytes &value)
{
	assert(flatfile_atomic_write(path.parent_path().string(), path.filename().string(), value,
				     nullptr));
}
void write_pending_snapshot(const fs::path &path, uint64_t flags, uint64_t revision,
			    const std::vector<player_item_snapshot> &items = {},
			    const std::vector<player_pet_snapshot> &pets = {})
{
	player_snapshot snapshot = {};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 1;
	snapshot.revision = revision;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.encoded_size_bound = 8192;
	snapshot.items = items;
	snapshot.pets = pets;
	snapshot.status_integers = { { player_status_field::racewar, 1, 0, false },
				     { player_status_field::action_flags_3, 0, flags, true } };
	bytes payload;
	assert(player_snapshot_encode(snapshot, &payload) == player_snapshot_codec_result::ok);
	bytes file;
	const auto &magic = flatfile_player_snapshot_file::player_magic;
	file.insert(file.end(), magic.begin(), magic.end());
	const auto number = [&](uint64_t value, size_t width)
	{
		for (size_t index = 0; index < width; ++index)
			file.push_back(static_cast<uint8_t>(value >> (8 * index)));
	};
	number(flatfile_player_snapshot_file::player_file_version, 4);
	number(payload.size(), 4);
	number(snapshot.pid, 4);
	number(snapshot.revision, 8);
	number(snapshot.components, 8);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.data(), payload.size(), digest.data());
	file.insert(file.end(), digest.begin(), digest.end());
	file.insert(file.end(), payload.begin(), payload.end());
	fs::create_directories(path / "players");
	fs::permissions(path / "players", fs::perms::owner_all);
	write(path / "players/1.snapshot", file);
}
size_t bucket(uint16_t kind, uint64_t context, uint64_t pid, const std::string &name)
{
	bytes key;
	const auto add = [&](uint64_t n, size_t count)
	{
		for (size_t i = 0; i < count; ++i)
			key.push_back(static_cast<uint8_t>(n >> (8 * i)));
	};
	add(kind, 2);
	add(context, 8);
	add(kind, 2);
	if (kind == 1)
		add(pid, 8);
	else
		key.insert(key.end(), name.begin(), name.end());
	std::array<uint8_t, 32> digest;
	SHA256(key.data(), key.size(), digest.data());
	return digest[0];
}
flatfile_economic_control control(const std::string &root, const flatfile_authority_lock &lock)
{
	flatfile_economic_control value;
	assert(flatfile_economic_control_read(root, lock, &value, nullptr) == 0);
	return value;
}
void commit(const std::string &root, const flatfile_authority_lock &lock, ops &changes)
{
	assert(access_type::commit(root, lock, changes, nullptr) ==
	       flatfile_authority_transaction_result::ok);
	changes.clear();
}
void initialize_bucket(const std::string &root, const flatfile_authority_lock &lock, size_t index)
{
	ops changes;
	const auto result = access_type::initialize_native(root, lock, control(root, lock).revision,
							   index, id(90000), &changes, nullptr);
	assert(result == 0 || result == EALREADY);
	if (!result)
		commit(root, lock, changes);
}
void setup(const fs::path &path, bool activate = true, bool initialize_accounting = true)
{
	const auto root = path.string();
	for (const auto &dir :
	     { path, path / "players", path / "domains", path / "economic-evidence",
	       path / "identities", path / "identities/names" })
	{
		fs::create_directories(dir);
		fs::permissions(dir, fs::perms::owner_all);
	}
	int32_t pid;
	assert(flatfile_identity_allocate_pid(root, &pid, nullptr) ==
		       flatfile_identity_result::ok &&
	       pid == 1);
	assert(flatfile_identity_claim(root, 1, "Player", "ACCOUNT-ONE", nullptr) ==
	       flatfile_identity_result::ok);
	flatfile_identity_record identity;
	assert(flatfile_identity_lookup_pid(root, 1, &identity, nullptr) ==
	       flatfile_identity_result::ok);
	identity.racewar = 1;
	assert(flatfile_identity_sync_account(root, "account-one", { identity }, nullptr) ==
	       flatfile_identity_result::ok);
	flatfile_player_domain_record player;
	player.pid = 1;
	player.account_name = "account-one";
	player.racewar = 1;
	player.domains.wallet = { 100, 20, 3, 1 };
	player.domains.bank = { 100, 20, 3, 1 };
	player.domains.epics = 7;
	player.domains.frags = 9;
	player.domains.base_stats = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };
	player.domains.base_stat_revision = 1;
	player.recent_pvp_deaths = { 50, 40 };
	player.completed_epic_zones = { 3, 5 };
	assert(flatfile_player_domain_establish(root, player, nullptr) ==
	       flatfile_player_domain_result::ok);
	if (!initialize_accounting)
		return;
	flatfile_authority_lock lock;
	assert(lock.acquire(root, nullptr));
	ops changes;
	assert(access_type::bootstrap(root, lock, id(90001), id(90002), &changes, nullptr) == 0);
	commit(root, lock, changes);
	initialize_bucket(root, lock, bucket(1, 0, 1, {}));
	initialize_bucket(root, lock, bucket(2, 1, 0, "account-one"));
	flatfile_economic_mapping mapping;
	assert(access_type::create(root, lock, control(root, lock).revision,
				   economic_account_kind::wallet, 0, { 1, 1, {} }, id(90003),
				   &mapping, &changes, nullptr) == 0);
	assert(mapping.account.authority_id == 1);
	commit(root, lock, changes);
	assert(access_type::create(root, lock, control(root, lock).revision,
				   economic_account_kind::bank, 1, { 2, 0, "account-one" },
				   id(90004), &mapping, &changes, nullptr) == 0);
	assert(mapping.account.authority_id == 2);
	commit(root, lock, changes);
	flatfile_economic_epoch epoch;
	epoch.epoch = id(90005);
	epoch.ordinal = 1;
	epoch.creating_operation = id(90006);
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	assert(access_type::append_epoch(root, lock, control(root, lock).revision, epoch, &changes,
					 nullptr) == 0);
	commit(root, lock, changes);
	if (activate)
	{
		assert(access_type::select_epoch(root, lock, control(root, lock).revision, true,
						 id(90007), &changes, nullptr) == 0);
		commit(root, lock, changes);
	}
	assert(access_type::initialize_evidence(root, lock, control(root, lock).revision, 7,
						id(90008), &changes, nullptr) == 0);
	commit(root, lock, changes);
}
flatfile_player_domain_record state(const std::string &root)
{
	flatfile_player_domain_record value;
	assert(flatfile_player_domain_load(root, 1, "account-one", 1, &value, nullptr) ==
	       flatfile_player_domain_result::ok);
	return value;
}
economic_account_key wallet()
{
	return { id(90001), economic_account_kind::wallet, 1, 0 };
}
economic_account_key bank()
{
	return { id(90001), economic_account_kind::bank, 2, 1 };
}
critical_command command(const std::string &root, uint64_t n, int64_t amount = 1,
			 bool accounted = true, size_t denomination = 0)
{
	const auto before = state(root);
	currency_command_payload payload = {};
	payload.pid = 1;
	payload.racewar = 1;
	payload.reason = amount > 0 ? currency_reason_type::atm_withdraw :
				      currency_reason_type::atm_deposit;
	strcpy(payload.account_name.data(), "ACCOUNT-ONE");
	assert(denomination < 4);
	payload.wallet_delta.amount[denomination] = amount;
	payload.bank_delta.amount[denomination] = -amount;
	critical_command result;
	assert(currency_command_build(&result, id(n), payload, before.domains.wallet_revision,
				      before.domains.bank_revision, critical_source_site::command,
				      critical_deadline_class::interactive));
	result.accepted_at_usec = 12;
	assert(critical_command_normalize(&result));
	if (accounted)
	{
		assert(economic_bank_transfer_intent(result, id(90005), wallet(), bank(),
						     &result.accounting_intent) ==
		       economic_accounting_error::ok);
		result.schema_version = 2;
	}
	return result;
}
critical_command starter_command(const std::string &root)
{
	const auto before = state(root);
	critical_operation_id seed = {}, operation_id = {};
	memcpy(seed.bytes.data(), "CHAOSEED", 8);
	for (size_t index = 0; index < 8; ++index)
		seed.bytes[8 + index] = static_cast<uint8_t>(uint64_t{ 1 } >> (8 * index));
	assert(critical_operation_id_derive(seed, 0x43484250, 1, &operation_id));
	currency_command_payload payload = {};
	payload.pid = 1;
	payload.racewar = 1;
	payload.reason = currency_reason_type::chaos_starter_reward;
	payload.reason_id = 1;
	strcpy(payload.account_name.data(), "ACCOUNT-ONE");
	payload.bank_delta.amount[3] = 1000000;
	critical_command result;
	assert(currency_command_build(
		&result, operation_id, payload, before.domains.wallet_revision, UINT64_MAX,
		critical_source_site::login, critical_deadline_class::recovery));
	const std::array wallets = { economic_gameplay_wallet_mapping{ 1, wallet() } };
	const std::array banks = { economic_gameplay_bank_mapping{ "account-one", 1, bank() } };
	assert(economic_gameplay_authority_test_access::install(id(90001), id(90005), id(90006),
								wallets, banks) ==
	       economic_accounting_error::ok);
	assert(economic_gameplay_authority::prepare_currency(&result) ==
	       economic_accounting_error::ok);
	economic_gameplay_authority_test_access::reset();
	assert(result.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION);
	assert(result.expected_revisions[1].revision == UINT64_MAX);
	result.accepted_at_usec = 12;
	result.publication_required = true;
	assert(critical_command_envelope_valid(result) &&
	       economic_flatfile_command_admission_supported(result));
	return result;
}
void refreeze(critical_command &cmd, const critical_operation_id &epoch = id(90005))
{
	cmd.schema_version = 1;
	cmd.accounting_intent.clear();
	assert(economic_bank_transfer_intent(cmd, epoch, wallet(), bank(),
					     &cmd.accounting_intent) ==
	       economic_accounting_error::ok);
	cmd.schema_version = 2;
}
flatfile_accounting_record retained(const std::string &root, const critical_command &cmd)
{
	flatfile_authority_lock lock;
	assert(lock.acquire(root, nullptr));
	flatfile_accounting_record value;
	assert(flatfile_accounting_lookup(root, lock, cmd, &value, nullptr) ==
	       flatfile_accounting_status::ok);
	return value;
}
void same(const critical_apply_result &a, const critical_apply_result &b)
{
	assert(a.error_code == b.error_code && a.durable_revision == b.durable_revision &&
	       a.failure_stage == b.failure_stage && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload);
}
void basic(const std::string &root)
{
	auto legacy = command(root, 1, 1, false);
	assert(flatfile_player_domain_apply(root, legacy).outcome == outcome::applied);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		ops changes;
		assert(access_type::select_epoch(root, lock, control(root, lock).revision, true,
						 id(90007), &changes, nullptr) == 0);
		commit(root, lock, changes);
	}
	assert(flatfile_player_domain_apply(root, legacy).outcome == outcome::already_applied);
	auto clash = legacy;
	refreeze(clash);
	assert(flatfile_accounting_bank_transaction::apply(root, clash).error_code == EEXIST);
	assert(flatfile_identity_set_blocked(root, 1, true, nullptr) ==
	       flatfile_identity_result::ok);
	const auto before = state(root);
	const auto transfer = command(root, 2);
	const auto applied = flatfile_accounting_bank_transaction::apply(root, transfer);
	assert(applied.outcome == outcome::applied && !applied.error_code);
	auto replay = flatfile_accounting_bank_transaction::apply(root, transfer);
	assert(replay.outcome == outcome::already_applied);
	same(applied, replay);
	auto changed = transfer;
	++changed.accepted_at_usec;
	assert(flatfile_accounting_bank_transaction::apply(root, changed).error_code == EEXIST);
	const auto after = state(root);
	assert(after.domains.wallet[0] == before.domains.wallet[0] + 1 &&
	       after.domains.bank[0] + 1 == before.domains.bank[0]);
	assert(after.domains.epics == before.domains.epics &&
	       after.domains.frags == before.domains.frags &&
	       after.domains.base_stats == before.domains.base_stats &&
	       after.recent_pvp_deaths == before.recent_pvp_deaths);
	// Regenerate the same canonical plan with SQL revision policy.
	const auto record = retained(root, transfer);
	economic_frozen_intent intent;
	assert(economic_intent_decode(transfer.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	economic_currency_authority authority = { id(90005),	    wallet(),	      bank(),
						  transfer.keys[0], transfer.keys[1], {} };
	for (size_t i = 0; i < 4; ++i)
	{
		authority.state.wallet.amount[i] = before.domains.wallet[i];
		authority.state.bank.amount[i] = before.domains.bank[i];
	}
	authority.state.wallet_revision = before.domains.wallet_revision;
	authority.state.bank_revision = before.domains.bank_revision;
	std::optional<economic_prepared_currency> prepared;
	assert(economic_bank_transfer_prepare(transfer, intent, authority,
					      currency_revision_policy::sql_legacy,
					      &prepared) == economic_accounting_error::ok);
	bytes canonical;
	assert(economic_plan_encode(prepared->plan(), &canonical) ==
		       economic_accounting_error::ok &&
	       canonical == record.plan);
	auto rejected = command(root, 3, 10000);
	const auto rejection = flatfile_accounting_bank_transaction::apply(root, rejected);
	assert(rejection.outcome == outcome::terminal_failure && rejection.error_code == ENOSPC);
	same(rejection, flatfile_accounting_bank_transaction::apply(root, rejected));
	assert(retained(root, rejected).plan.empty());
	auto stale = command(root, 4);
	stale.expected_revisions[0].revision = 0;
	refreeze(stale);
	assert(flatfile_accounting_bank_transaction::apply(root, stale).error_code == ESTALE);
	// New accounting evidence must not append to the capped legacy receipt list.
	for (uint64_t n = 5; n < 519; ++n)
	{
		auto next = command(root, n, n % 2 ? 1 : -1);
		assert(flatfile_accounting_bank_transaction::apply(root, next).outcome ==
		       outcome::applied);
	}
	same(applied, flatfile_accounting_bank_transaction::apply(root, transfer));
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		std::optional<flatfile_legacy_domain_receipt> old;
		assert(flatfile_player_domain_legacy_receipt_locked(
			       root, lock, 1, legacy.operation_id, &old, nullptr) ==
			       flatfile_player_domain_result::ok &&
		       old);
		assert(flatfile_player_domain_legacy_receipt_locked(
			       root, lock, 1, transfer.operation_id, &old, nullptr) ==
			       flatfile_player_domain_result::ok &&
		       !old);
		ops changes;
		initialize_bucket(root, lock, bucket(2, 1, 0, "renamed"));
		flatfile_economic_mapping mapping;
		assert(flatfile_economic_mapping_read(root, lock, bank(), &mapping, nullptr) == 0);
		assert(access_type::rename(root, lock, control(root, lock).revision, bank(),
					   mapping.revision, "renamed", id(91000), &changes,
					   nullptr) == 0);
		commit(root, lock, changes);
		flatfile_economic_epoch later;
		later.epoch = id(91003);
		later.predecessor = id(90005);
		later.ordinal = 2;
		later.creating_operation = id(91004);
		later.transition_kind = 1;
		later.transition_digest[0] = 42;
		assert(access_type::append_epoch(root, lock, control(root, lock).revision, later,
						 &changes, nullptr) == 0);
		commit(root, lock, changes);
		assert(access_type::select_epoch(root, lock, control(root, lock).revision, true,
						 id(91005), &changes, nullptr) == 0);
		commit(root, lock, changes);
		assert(access_type::select_epoch(root, lock, control(root, lock).revision, false,
						 id(91001), &changes, nullptr) == 0);
		commit(root, lock, changes);
		for (const auto &key : { wallet(), bank() })
		{
			assert(flatfile_economic_mapping_read(root, lock, key, &mapping, nullptr) ==
			       0);
			assert(access_type::retire(root, lock, control(root, lock).revision, key,
						   mapping.revision, id(91002), &changes,
						   nullptr) == 0);
			commit(root, lock, changes);
		}
	}
	fs::remove(fs::path(root) / "domains/player-1.domain");
	fs::remove(fs::path(root) / "domains/bank-account-one-1.domain");
	same(applied, flatfile_accounting_bank_transaction::apply(root, transfer));
	same(rejection, flatfile_accounting_bank_transaction::apply(root, rejected));
	std::cout
		<< "bank root: success, exact replay beyond 512 later IDs, legacy preservation, rejection, SQL plan parity, rename/retirement/paused epoch passed\n";
}

void clone(const fs::path &seed, const fs::path &target)
{
	fs::copy(seed, target, fs::copy_options::recursive);
}

// Requirement CURRENCY-REPLAY: generated histories use the production writer;
// the oracle only adds/subtracts integer denomination balances and revisions.
std::map<std::string, bytes> durable_files(const fs::path &root)
{
	std::map<std::string, bytes> result;
	for (const auto &directory : { "domains", "economic-evidence" })
		for (const auto &entry : fs::recursive_directory_iterator(root / directory))
			if (entry.is_regular_file())
				result.emplace(entry.path().lexically_relative(root).string(),
					       read(entry.path()));
	return result;
}
void generated_histories(const fs::path &seed, const fs::path &base)
{
	for (uint32_t seed_value : { 0xc011u, 0xbab1u, 0x5eedu })
	{
		const auto path = base / ("generated-" + std::to_string(seed_value));
		clone(seed, path);
		const auto root = path.string();
		const auto initial = state(root);
		auto model_wallet = initial.domains.wallet;
		auto model_bank = initial.domains.bank;
		auto wallet_revision = initial.domains.wallet_revision;
		auto bank_revision = initial.domains.bank_revision;
		uint32_t generator = seed_value;
		std::vector<std::pair<critical_command, critical_apply_result>> history;
		bool recovered_interruption = false;
		for (unsigned step = 0; step < 24; ++step)
		{
			generator = generator * 1664525u + 1013904223u;
			const size_t denomination = (generator >> 16) % 4;
			const bool withdraw = generator & 1;
			const auto available = withdraw ? model_bank[denomination] :
							  model_wallet[denomination];
			const bool reject = step % 7 == 6;
			const int64_t magnitude = reject ? available + 1 :
							   std::min<int64_t>(available, 2);
			const int64_t amount = withdraw ? magnitude : -magnitude;
			// Zero transfers are not a useful generated operation.
			if (!amount)
				continue;
			std::cout << "CURRENCY-REPLAY seed=" << seed_value << " step=" << step
				  << " denomination=" << denomination << " amount=" << amount
				  << " reject=" << reject << std::endl;
			const auto cmd = command(root, 100000 + step, amount, true, denomination);
			critical_apply_result applied;
			if (step == 11)
			{
				// Crash with only the durable authority journal published. A new
				// process must recover the four after-images exactly once.
				const auto child = fork();
				assert(child >= 0);
				if (!child)
				{
					setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL",
					       "1", 1);
					const auto interrupted =
						flatfile_accounting_bank_transaction::apply(root,
											    cmd);
					_exit(interrupted.outcome == outcome::ambiguous_commit ?
						      77 :
						      78);
				}
				int status;
				assert(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
				       WEXITSTATUS(status) == 77);
				applied = flatfile_accounting_bank_transaction::apply(root, cmd);
				assert(applied.outcome == outcome::already_applied);
				recovered_interruption = true;
			}
			else
				applied = flatfile_accounting_bank_transaction::apply(root, cmd);
			if (reject)
			{
				assert(applied.outcome == outcome::terminal_failure &&
				       applied.error_code == ENOSPC);
				assert(retained(root, cmd).plan.empty());
			}
			else
			{
				assert(applied.outcome == outcome::applied || step == 11);
				model_wallet[denomination] += amount;
				model_bank[denomination] -= amount;
				++wallet_revision;
				++bank_revision;
			}
			history.emplace_back(cmd, applied);
			const auto actual = state(root);
			assert(actual.domains.wallet == model_wallet &&
			       actual.domains.bank == model_bank &&
			       actual.domains.wallet_revision == wallet_revision &&
			       actual.domains.bank_revision == bank_revision);
			for (size_t index = 0; index < 4; ++index)
				assert(model_wallet[index] + model_bank[index] ==
				       initial.domains.wallet[index] + initial.domains.bank[index]);
			const auto before_replay = durable_files(path);
			const auto &old = history[(generator >> 8) % history.size()];
			const auto replay =
				flatfile_accounting_bank_transaction::apply(root, old.first);
			same(old.second, replay);
			assert(replay.outcome == (old.second.error_code ?
							  outcome::terminal_failure :
							  outcome::already_applied));
			if (step % 5 == 0)
			{
				const auto child = fork();
				assert(child >= 0);
				if (!child)
				{
					same(applied, flatfile_accounting_bank_transaction::apply(
							      root, cmd));
					_exit(0);
				}
				int status;
				assert(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
				       WEXITSTATUS(status) == 0);
			}
			assert(durable_files(path) == before_replay);
		}
		assert(history.size() >= 16 && recovered_interruption);
	}
	std::cout
		<< "PASS: CURRENCY-REPLAY generated transfers, refusals, old replay and process restart\n";
}
void initialize_starter_bucket(const std::string &root, const critical_command &cmd)
{
	flatfile_authority_lock lock;
	assert(lock.acquire(root, nullptr));
	ops changes;
	const auto result = access_type::initialize_evidence(root, lock,
							     control(root, lock).revision,
							     cmd.operation_id.bytes[0], id(92000),
							     &changes, nullptr);
	assert(result == 0 || result == EALREADY);
	if (!result)
		commit(root, lock, changes);
}
void starter_grant(const fs::path &seed, const fs::path &base)
{
	const auto path = base / "starter";
	clone(seed, path);
	const auto root = path.string();
	const auto cmd = starter_command(root);
	initialize_starter_bucket(root, cmd);
	assert(economic_command_admission_supported(cmd) &&
	       economic_flatfile_command_admission_supported(cmd));
	const auto before = state(root);
	const auto missing_snapshot = flatfile_accounting_bank_transaction::apply(root, cmd);
	assert(missing_snapshot.outcome == outcome::retryable_failure &&
	       missing_snapshot.error_code == ENOENT);
	write_pending_snapshot(path, 0, 1);
	const auto missing_flag = flatfile_accounting_bank_transaction::apply(root, cmd);
	assert(missing_flag.outcome == outcome::retryable_failure &&
	       missing_flag.error_code == EACCES);
	assert(state(root).domains.bank == before.domains.bank &&
	       state(root).domains.bank_revision == before.domains.bank_revision);
	write_pending_snapshot(path, CURRENCY_CHAOS_STARTER_BANK_PENDING_FLAG, 2);
	const auto applied = flatfile_accounting_bank_transaction::apply(root, cmd);
	assert(applied.outcome == outcome::applied && !applied.error_code);
	const auto after = state(root);
	assert(after.domains.wallet == before.domains.wallet &&
	       after.domains.wallet_revision == before.domains.wallet_revision + 1 &&
	       after.domains.bank[3] == before.domains.bank[3] + 1000000 &&
	       after.domains.bank_revision == before.domains.bank_revision + 1);
	const auto record = retained(root, cmd);
	economic_accounting_plan plan;
	assert(economic_plan_decode(record.plan, &plan) == economic_accounting_error::ok &&
	       plan.accounts.size() == 3 && plan.postings.size() == 2);
	bool wallet_effect = false, bank_effect = false, issuance_effect = false;
	for (const auto &account : plan.accounts)
	{
		if (economic_account_key_equal(account.key, wallet()))
		{
			wallet_effect = true;
			assert(account.before_revision == before.domains.wallet_revision &&
			       account.after_revision == after.domains.wallet_revision);
			for (size_t part = 0; part < 4; ++part)
				assert(account.before[part] ==
					       static_cast<int64_t>(before.domains.wallet[part]) &&
				       account.after[part] == account.before[part]);
		}
		else if (economic_account_key_equal(account.key, bank()))
		{
			bank_effect = true;
			assert(account.before_revision == before.domains.bank_revision &&
			       account.after_revision == after.domains.bank_revision);
			for (size_t part = 0; part < 4; ++part)
				assert(account.before[part] ==
					       static_cast<int64_t>(before.domains.bank[part]) &&
				       account.after[part] ==
					       static_cast<int64_t>(after.domains.bank[part]));
		}
		else
		{
			issuance_effect = true;
			assert(account.key.kind == economic_account_kind::issuance &&
			       account.before == economic_coin_vector{} &&
			       account.after == economic_coin_vector{});
		}
	}
	assert(wallet_effect && bank_effect && issuance_effect);
	int64_t copper_sum = 0;
	bool bank_posting = false, issuance_posting = false;
	for (const auto &posting : plan.postings)
	{
		copper_sum += posting.copper;
		const auto kind = plan.accounts[posting.account_index].key.kind;
		if (kind == economic_account_kind::bank)
			bank_posting = posting.delta == economic_coin_vector{ 0, 0, 0, 1000000 } &&
				       posting.copper == 1000000000;
		else if (kind == economic_account_kind::issuance)
			issuance_posting = posting.delta ==
						   economic_coin_vector{ 0, 0, 0, -1000000 } &&
					   posting.copper == -1000000000;
	}
	assert(bank_posting && issuance_posting && copper_sum == 0);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		assert(access_type::verify_source_claim(root, lock, record, nullptr) ==
		       flatfile_accounting_status::ok);
	}
	write_pending_snapshot(path, 0, 3);
	const auto replay = flatfile_accounting_bank_transaction::apply(root, cmd);
	assert(replay.outcome == outcome::already_applied);
	same(applied, replay);
	auto changed_id = cmd;
	changed_id.operation_id = id(92001);
	changed_id.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	changed_id.accounting_intent.clear();
	changed_id.publication_required = false;
	std::vector<uint8_t> changed_intent;
	assert(economic_chaos_starter_bank_intent(changed_id, id(90005), wallet(), bank(),
						  &changed_intent) ==
	       economic_accounting_error::unauthorized);
	const auto tampered = base / "starter-tampered";
	clone(path, tampered);
	bool removed_claim = false;
	for (const auto &entry : fs::directory_iterator(tampered / "economic-evidence"))
		if (entry.path().filename().string().starts_with("source-claim-"))
		{
			assert(!removed_claim && fs::remove(entry.path()));
			removed_claim = true;
		}
	assert(removed_claim);
	const auto damaged = flatfile_accounting_bank_transaction::apply(tampered.string(), cmd);
	assert(damaged.outcome == outcome::retryable_failure && damaged.error_code);
	assert(state(tampered.string()).domains.bank[3] == after.domains.bank[3]);
	const auto rebase = base / "starter-rebase";
	clone(seed, rebase);
	const auto rebase_root = rebase.string();
	const auto stale_grant = starter_command(rebase_root);
	initialize_starter_bucket(rebase_root, stale_grant);
	write_pending_snapshot(rebase, CURRENCY_CHAOS_STARTER_BANK_PENDING_FLAG, 1);
	assert(flatfile_accounting_bank_transaction::apply(rebase_root, command(rebase_root, 2))
		       .outcome == outcome::applied);
	const auto rebased = flatfile_accounting_bank_transaction::apply(rebase_root, stale_grant);
	assert(rebased.outcome == outcome::applied &&
	       state(rebase_root).domains.wallet[0] == before.domains.wallet[0] + 1 &&
	       state(rebase_root).domains.bank[3] == before.domains.bank[3] + 1000000);
	const auto crash = base / "starter-crash";
	clone(seed, crash);
	const auto crash_root = crash.string();
	const auto crash_command = starter_command(crash_root);
	initialize_starter_bucket(crash_root, crash_command);
	write_pending_snapshot(crash, CURRENCY_CHAOS_STARTER_BANK_PENDING_FLAG, 1);
	const auto child = fork();
	assert(child >= 0);
	if (!child)
	{
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION", "5", 1);
		const auto result =
			flatfile_accounting_bank_transaction::apply(crash_root, crash_command);
		_exit(result.outcome == outcome::ambiguous_commit ? 77 : 78);
	}
	int status;
	assert(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
	       WEXITSTATUS(status) == 77);
	const auto recovered =
		flatfile_accounting_bank_transaction::apply(crash_root, crash_command);
	assert(recovered.outcome == outcome::already_applied &&
	       state(crash_root).domains.bank[3] == before.domains.bank[3] + 1000000);
	const auto recovered_record = retained(crash_root, crash_command);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(crash_root, nullptr));
		assert(access_type::verify_source_claim(crash_root, lock, recovered_record,
							nullptr) == flatfile_accounting_status::ok);
	}
	std::cout
		<< "bank root: sourced starter grant, missing witness, retained claim, tamper and crash recovery passed\n";
}
void crashes(const fs::path &seed, const fs::path &base)
{
	for (size_t boundary = 0; boundary <= 4; ++boundary)
	{
		const auto path = base / ("crash-" + std::to_string(boundary));
		clone(seed, path);
		const auto root = path.string();
		const auto cmd = command(root, 2);
		const auto player_path = path / "domains/player-1.domain";
		const auto bank_path = path / "domains/bank-account-one-1.domain";
		const auto index_path = path / "economic-evidence/bucket-07.eai";
		const auto original_player = read(player_path), original_bank = read(bank_path),
			   original_index = read(index_path);
		const auto child = fork();
		assert(child >= 0);
		if (!child)
		{
			if (boundary == 0)
				setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL", "1",
				       1);
			else
				setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION",
				       std::to_string(boundary).c_str(), 1);
			const auto result = flatfile_accounting_bank_transaction::apply(root, cmd);
			_exit(result.outcome == outcome::ambiguous_commit ? 77 : 78);
		}
		int status;
		assert(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
		       WEXITSTATUS(status) == 77);
		const auto journal = path / "domains/.critical-authority-transaction";
		assert(fs::exists(journal));
		if (boundary == 0)
		{
			// Check raw files before any recovery-capable read: only the journal
			// may have been published at this boundary.
			assert(!read(journal).empty() && read(player_path) == original_player &&
			       read(bank_path) == original_bank &&
			       read(index_path) == original_index &&
			       !fs::exists(path / "economic-evidence/bucket-07-0.eas"));
		}
		const auto replay = flatfile_accounting_bank_transaction::apply(root, cmd);
		assert(replay.outcome == outcome::already_applied);
		const auto after = state(root);
		assert(after.domains.wallet[0] == 101 && after.domains.bank[0] == 99);
		assert(!fs::exists(journal));
		const auto receipt = retained(root, cmd);
		bytes expected, actual;
		assert(critical_command_encode(cmd, &expected) ==
		       critical_command_codec_result::ok);
		assert(critical_command_encode(receipt.command, &actual) ==
		       critical_command_codec_result::ok);
		assert(actual == expected && replay.failure_stage == critical_failure_stage::none);
		same(replay, flatfile_accounting_bank_transaction::apply(root, cmd));
	}
	std::cout
		<< "bank root: journal-only and all four domain/evidence publication interruptions recover exactly once\n";
}
void allocations(const fs::path &seed, const fs::path &base)
{
	size_t failures = 0, ambiguous = 0;
	bool complete = false;
	for (size_t nth = 1; nth < 4096; ++nth)
	{
		const auto path = base / ("allocation-" + std::to_string(nth));
		clone(seed, path);
		const auto root = path.string();
		const auto cmd = command(root, 2);
		allocation_seen = 0;
		allocation_target = nth;
		const auto result = flatfile_accounting_bank_transaction::apply(root, cmd);
		allocation_target = 0;
		if (allocation_seen < nth)
		{
			assert(result.outcome == outcome::applied);
			complete = true;
			fs::remove_all(path);
			break;
		}
		assert(result.outcome == outcome::retryable_failure ||
		       result.outcome == outcome::ambiguous_commit);
		++failures;
		if (result.outcome == outcome::ambiguous_commit)
			++ambiguous;
		const auto retry = flatfile_accounting_bank_transaction::apply(root, cmd);
		assert(retry.outcome == outcome::applied ||
		       retry.outcome == outcome::already_applied);
		const auto after = state(root);
		assert(after.domains.wallet[0] == 101 && after.domains.bank[0] == 99);
		same(retry, flatfile_accounting_bank_transaction::apply(root, cmd));
		fs::remove_all(path);
	}
	assert(complete && failures && ambiguous);
	std::cout << "bank root: " << failures << " allocation failures recover exactly once ("
		  << ambiguous << " during/after commit)\n";
}
void forged(const fs::path &seed, const fs::path &base)
{
	const auto source = base / "valid";
	clone(seed, source);
	const auto valid_command = command(source.string(), 2);
	assert(flatfile_accounting_bank_transaction::apply(source.string(), valid_command).outcome ==
	       outcome::applied);
	const auto original = retained(source.string(), valid_command);
	const auto rejected_command = command(source.string(), 3, 10000);
	const auto rejected_result =
		flatfile_accounting_bank_transaction::apply(source.string(), rejected_command);
	assert(rejected_result.outcome == outcome::terminal_failure &&
	       rejected_result.error_code == ENOSPC);
	const auto legitimate_rejection = retained(source.string(), rejected_command);
	assert(legitimate_rejection.failure_stage == critical_failure_stage::none);
	same(rejected_result,
	     flatfile_accounting_bank_transaction::apply(source.string(), rejected_command));
	for (int variant = 0; variant < 5; ++variant)
	{
		const auto path = base / ("forged-" + std::to_string(variant));
		clone(seed, path);
		const auto root = path.string();
		auto record = original;
		if (variant == 4)
		{
			// All receipt fields except the stage come from a verified bank
			// rejection. A valid enum from another domain is still not a bank stage.
			record = legitimate_rejection;
			record.failure_stage = critical_failure_stage::coin_source_wallet_revision;
			assert(critical_failure_stage_valid(record.failure_stage));
		}
		else if (variant == 0)
		{
			currency_command_result result;
			assert(currency_command_decode_result(record.result.data(),
							      record.result.size(), &result));
			++result.wallet.amount[0];
			std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> encoded;
			assert(currency_command_encode_result(result, &encoded));
			record.result.assign(encoded.begin(), encoded.end());
		}
		else if (variant == 1)
		{
			economic_accounting_plan plan;
			assert(economic_plan_decode(record.plan, &plan) ==
			       economic_accounting_error::ok);
			plan.postings.push_back({ 2, 0, 0, { 1, 0, 0, 0 }, 1 });
			plan.postings.push_back({ 3, 0, 0, { -1, 0, 0, 0 }, -1 });
			assert(economic_plan_normalize(&plan) == economic_accounting_error::ok);
			assert(economic_plan_encode(plan, &record.plan) ==
			       economic_accounting_error::ok);
		}
		else
		{
			record.plan.clear();
			record.result_code = variant == 2 ? EACCES : ESTALE;
			if (variant == 3)
				refreeze(record.command, id(99999));
		}
		{
			flatfile_authority_lock lock;
			assert(lock.acquire(root, nullptr));
			ops changes;
			assert(access_type::stage(root, lock, record, &changes, nullptr) ==
			       flatfile_accounting_status::ok);
			commit(root, lock, changes);
		}
		const auto result =
			flatfile_accounting_bank_transaction::apply(root, record.command);
		assert(result.outcome == outcome::retryable_failure && result.error_code);
		assert(state(root).domains.wallet[0] == 100);
	}
	std::cout
		<< "bank root: forged results, extra cancelling legs, arbitrary rejection, nonexistent epoch and forged valid failure stage refused\n";
}
void legacy_capacity(const fs::path &seed, const fs::path &path)
{
	clone(seed, path);
	const auto root = path.string();
	critical_command first;
	for (uint64_t n = 0; n < 512; ++n)
	{
		critical_command cmd;
		epic_command_payload payload = { 1, 1, epic_reason_type::quest_award, 0, 1 };
		assert(epic_command_build(&cmd, id(10000 + n), payload, UINT64_MAX,
					  critical_source_site::command,
					  critical_deadline_class::interactive));
		cmd.accepted_at_usec = 1;
		assert(critical_command_normalize(&cmd));
		if (!n)
			first = cmd;
		assert(flatfile_player_domain_apply(root, cmd).outcome == outcome::applied);
	}
	const auto old = flatfile_player_domain_apply(root, first);
	assert(old.outcome == outcome::already_applied);
	auto full = first;
	full.operation_id = id(11000);
	assert(flatfile_player_domain_apply(root, full).error_code == ENOSPC);
	const auto cmd = command(root, 2);
	assert(flatfile_accounting_bank_transaction::apply(root, cmd).outcome == outcome::applied);
	same(old, flatfile_player_domain_apply(root, first));
	assert(state(root).domains.epics == 519);
	std::cout
		<< "bank root: 512 legacy receipts survive typed transfer without consuming a legacy slot\n";
}
void contention(const fs::path &seed, const fs::path &path)
{
	clone(seed, path);
	const auto root = path.string();
	int32_t pid;
	assert(flatfile_identity_allocate_pid(root, &pid, nullptr) ==
		       flatfile_identity_result::ok &&
	       pid == 2);
	assert(flatfile_identity_claim(root, pid, "Second", "account-one", nullptr) ==
	       flatfile_identity_result::ok);
	std::vector<flatfile_identity_record> identities;
	assert(flatfile_identity_list_account(root, "account-one", &identities, nullptr) ==
	       flatfile_identity_result::ok);
	for (auto &entry : identities)
		entry.racewar = 1;
	assert(flatfile_identity_sync_account(root, "account-one", identities, nullptr) ==
	       flatfile_identity_result::ok);
	auto player = state(root);
	player.pid = 2;
	const auto bank_revision = player.domains.bank_revision;
	player.domains.bank_revision = 0;
	assert(flatfile_player_domain_establish(root, player, nullptr) ==
	       flatfile_player_domain_result::ok);
	player.domains.bank_revision = bank_revision;
	economic_account_key second;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, nullptr));
		initialize_bucket(root, lock, bucket(1, 0, 2, {}));
		ops changes;
		flatfile_economic_mapping mapping;
		assert(access_type::create(root, lock, control(root, lock).revision,
					   economic_account_kind::wallet, 0, { 1, 2, {} },
					   id(90020), &mapping, &changes, nullptr) == 0);
		second = mapping.account;
		commit(root, lock, changes);
	}
	critical_command commands[2] = { command(root, 2), command(root, 3) };
	currency_command_payload payload;
	assert(currency_command_decode_payload(commands[1], &payload));
	payload.pid = 2;
	assert(currency_command_build(&commands[1], id(3), payload, player.domains.wallet_revision,
				      player.domains.bank_revision, critical_source_site::command,
				      critical_deadline_class::interactive));
	commands[1].accepted_at_usec = 12;
	assert(critical_command_normalize(&commands[1]));
	assert(economic_bank_transfer_intent(commands[1], id(90005), second, bank(),
					     &commands[1].accounting_intent) ==
	       economic_accounting_error::ok);
	commands[1].schema_version = 2;
	pid_t children[2];
	for (size_t i = 0; i < 2; ++i)
	{
		children[i] = fork();
		assert(children[i] >= 0);
		if (!children[i])
		{
			const auto result =
				flatfile_accounting_bank_transaction::apply(root, commands[i]);
			_exit(result.outcome == outcome::applied ? 10 :
			      result.outcome == outcome::terminal_failure &&
					      result.error_code == ESTALE ?
								   11 :
								   12);
		}
	}
	size_t committed = 0, rejected = 0;
	for (auto child : children)
	{
		int status;
		assert(waitpid(child, &status, 0) == child && WIFEXITED(status));
		committed += WEXITSTATUS(status) == 10;
		rejected += WEXITSTATUS(status) == 11;
	}
	assert(committed == 1 && rejected == 1);
	flatfile_player_domain_record two;
	assert(flatfile_player_domain_load(root, 2, "account-one", 1, &two, nullptr) ==
	       flatfile_player_domain_result::ok);
	assert(state(root).domains.wallet[0] + two.domains.wallet[0] == 201 &&
	       two.domains.bank[0] == 99);
	for (const auto &cmd : commands)
	{
		const auto replay = flatfile_accounting_bank_transaction::apply(root, cmd);
		assert(replay.outcome == outcome::already_applied ||
		       (replay.outcome == outcome::terminal_failure &&
			replay.error_code == ESTALE));
	}
	std::cout
		<< "bank root: concurrent players sharing a bank serialize with one exact stale rejection\n";
}
struct coordinator_bank_context
{
	std::string root;
};

critical_apply_result apply_coordinator_bank(const critical_command &cmd, void *opaque)
{
	const auto &context = *static_cast<coordinator_bank_context *>(opaque);
	return flatfile_accounting_bank_transaction::apply(context.root, cmd);
}

void coordinated_bank(const fs::path &root, const fs::path &journal)
{
	setup(root);
	fs::create_directories(journal);
	fs::permissions(journal, fs::perms::owner_all, fs::perm_options::replace);
	coordinator_bank_context context{ root.string() };
	auto cmd = command(context.root, 91001);
	cmd.publication_required = true;
	assert(economic_flatfile_command_admission_supported(cmd));
	const auto before = state(context.root);
	const auto start = [&]
	{
		assert(critical_command_coordinator_init(
			journal.c_str(), apply_coordinator_bank, &context, 1, nullptr, nullptr,
			economic_flatfile_command_admission_supported));
	};
	const auto completed = [&]
	{
		critical_completion result = {};
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
		while (critical_command_coordinator_pulse(&result, 1) != 1)
		{
			assert(std::chrono::steady_clock::now() < deadline);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		assert(critical_operation_id_equal(result.operation_id, cmd.operation_id) &&
		       !result.error_code);
		return result;
	};
	start();
	assert(critical_command_coordinator_submit_for_publication(cmd) ==
	       critical_submit_result::awaiting_durability);
	const auto first = completed();
	assert(first.outcome == outcome::applied);
	auto conflict = cmd;
	++conflict.accepted_at_usec;
	assert(critical_command_coordinator_submit_for_publication(conflict) ==
	       critical_submit_result::identity_conflict);
	const auto after = state(context.root);
	assert(after.domains.wallet[0] == before.domains.wallet[0] + 1 &&
	       after.domains.bank[0] + 1 == before.domains.bank[0]);
	assert(critical_command_journal_health_copy().checkpoints == 0 &&
	       critical_command_coordinator_health_copy().publication_pending == 1);
	assert(critical_command_coordinator_shutdown());
	start();
	const auto second = completed();
	assert(second.outcome == outcome::already_applied &&
	       second.durable_revision == first.durable_revision &&
	       second.result_payload == first.result_payload &&
	       state(context.root).domains.wallet == after.domains.wallet &&
	       state(context.root).domains.bank == after.domains.bank);
	assert(critical_command_coordinator_acknowledge_publication(cmd.operation_id));
	assert(critical_command_journal_health_copy().checkpoints == 1);
	assert(critical_command_coordinator_shutdown());
	start();
	assert(critical_command_coordinator_health_copy().publication_pending == 0);
	assert(critical_command_coordinator_shutdown());
}

int main(int argc, char **argv)
{
	assert(argc == 2 || (argc == 3 && std::string(argv[2]) == "--generated-only"));
	alarm(600);
	const fs::path base = argv[1];
	fs::create_directories(base);
	fs::permissions(base, fs::perms::owner_all);
	const auto seed = base / "seed";
	setup(seed);
	generated_histories(seed, base);
	if (argc == 3)
		return 0;
	starter_grant(seed, base);
	crashes(seed, base);
	legacy_capacity(seed, base / "legacy-full");
	contention(seed, base / "contended");
	forged(seed, base);
	allocations(seed, base);
	const auto journey = base / "basic";
	setup(journey, false);
	basic(journey.string());
	coordinated_bank(base / "coordinator-bank", base / "coordinator-bank-journal");
	std::cout << "flatfile typed bank root journeys passed\n";
}
