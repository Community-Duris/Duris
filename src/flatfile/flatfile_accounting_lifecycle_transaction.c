#include "flatfile/flatfile_accounting_lifecycle_transaction.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_accounting_staging_view.h"
#include "flatfile/flatfile_store.h"
#include "economy/currency_command.h"
#include "economy/economic_accounting_plan.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <map>
#include <openssl/sha.h>
#include <set>
#include <type_traits>

namespace
{
struct failure
{
	unsigned int code;
};

void need(bool condition, unsigned int error = EILSEQ)
{
	if (!condition)
		throw failure{ error };
}

bool nonzero(const critical_operation_id &id)
{
	return !critical_operation_id_is_zero(id);
}

std::string canonical_identity_account(const std::string &account)
{
	std::string canonical = account;
	for (char &character : canonical)
		if (character >= 'A' && character <= 'Z')
			character = static_cast<char>(character - 'A' + 'a');
	return canonical;
}

economic_digest hash(std::span<const uint8_t> data)
{
	economic_digest value = {};
	SHA256(data.data(), data.size(), value.data());
	return value;
}

void append_u32(std::vector<uint8_t> *data, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		data->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u64(std::vector<uint8_t> *data, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		data->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

economic_digest holding_source_digest(uint8_t kind, uint64_t id, uint8_t context,
				      const std::string &name, uint64_t revision,
				      const economic_coin_vector &balance)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'H', 'O', 'L',
				   'D', 'I', 'N', 'G', '-', 'V', '1', kind };
	append_u64(&data, id);
	data.push_back(context);
	append_u32(&data, static_cast<uint32_t>(name.size()));
	data.insert(data.end(), name.begin(), name.end());
	append_u64(&data, revision);
	for (int64_t amount : balance)
		append_u64(&data, static_cast<uint64_t>(amount));
	return hash(data);
}

unsigned int capture_sources(const std::string &root, const flatfile_identity_lock &identity_lock,
			     const flatfile_authority_lock &authority_lock,
			     flatfile_accounting_lifecycle_native_sources *sources,
			     std::string *error)
{
	need(!root.empty() && identity_lock.matches(root) && authority_lock.matches(root) &&
		     sources,
	     EINVAL);
	std::vector<flatfile_identity_record> identities;
	const auto identity_result = flatfile_identity_list_all_locked(
		root, identity_lock, authority_lock, &identities, error);
	need(identity_result == flatfile_identity_result::ok,
	     identity_result == flatfile_identity_result::io_error ? EIO : EILSEQ);
	flatfile_player_domain_native_sources native;
	const auto native_result = flatfile_player_domain_capture_native_sources_locked(
		root, authority_lock, &native, error);
	need(native_result == flatfile_player_domain_result::ok,
	     native_result == flatfile_player_domain_result::io_error ? EIO : EILSEQ);
	need(native.wallets.size() + native.banks.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS, ENOSPC);
	std::map<int32_t, flatfile_identity_record> identities_by_pid;
	for (const auto &identity : identities)
	{
		need(identity.pid > 0 && identity.racewar >= 0, EILSEQ);
		need(identities_by_pid.emplace(identity.pid, identity).second, EILSEQ);
	}
	std::set<int32_t> wallet_pids;
	std::set<std::pair<std::string, uint8_t>> bank_keys;
	for (const auto &bank : native.banks)
		bank_keys.insert({ bank.account_name, static_cast<uint8_t>(bank.racewar) });
	flatfile_accounting_lifecycle_native_sources candidate;
	candidate.wallets.reserve(native.wallets.size());
	candidate.banks.reserve(native.banks.size());
	for (const auto &wallet : native.wallets)
	{
		auto identity = identities_by_pid.find(wallet.pid);
		const std::string identity_account =
			identity == identities_by_pid.end() ?
				std::string{} :
				canonical_identity_account(identity->second.account);
		need(identity != identities_by_pid.end() &&
			     identity_account == wallet.account_name &&
			     identity->second.racewar == wallet.racewar,
		     EILSEQ);
		wallet_pids.insert(wallet.pid);
		flatfile_accounting_lifecycle_wallet_source source;
		source.pid = static_cast<uint32_t>(wallet.pid);
		source.account_name = wallet.account_name;
		source.racewar = static_cast<uint8_t>(wallet.racewar);
		source.native_revision = wallet.revision;
		for (size_t index = 0; index < source.balance.size(); ++index)
		{
			need(wallet.balance[index] <= static_cast<uint64_t>(INT64_MAX), EOVERFLOW);
			source.balance[index] = static_cast<int64_t>(wallet.balance[index]);
		}
		source.source_digest =
			holding_source_digest(1, source.pid, source.racewar, source.account_name,
					      source.native_revision, source.balance);
		candidate.wallets.push_back(std::move(source));
	}
	for (const auto &identity : identities)
		if (identity.active)
		{
			need(wallet_pids.count(identity.pid) == 1, EILSEQ);
			need(bank_keys.count({ canonical_identity_account(identity.account),
					       static_cast<uint8_t>(identity.racewar) }) == 1,
			     EILSEQ);
		}
	for (const auto &bank : native.banks)
	{
		need(bank.racewar >= 0, EILSEQ);
		flatfile_accounting_lifecycle_bank_source source;
		source.name = bank.account_name;
		source.racewar = static_cast<uint8_t>(bank.racewar);
		source.native_revision = bank.revision;
		for (size_t index = 0; index < source.balance.size(); ++index)
		{
			need(bank.balance[index] <= static_cast<uint64_t>(INT64_MAX), EOVERFLOW);
			source.balance[index] = static_cast<int64_t>(bank.balance[index]);
		}
		source.source_digest = holding_source_digest(
			2, 0, source.racewar, source.name, source.native_revision, source.balance);
		candidate.banks.push_back(std::move(source));
	}
	*sources = std::move(candidate);
	return 0;
}

economic_digest
compute_coverage_digest(const std::vector<flatfile_accounting_lifecycle_wallet_source> &wallets,
			const std::vector<flatfile_accounting_lifecycle_bank_source> &banks)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'F', 'L', 'A', 'T', 'F', 'I', 'L',
				   'E', '-', 'C', 'O', 'V', 'E', 'R', 'A', 'G', 'E', '-', 'V', '1' };
	append_u64(&data, wallets.size());
	append_u64(&data, banks.size());
	for (const auto &w : wallets)
	{
		data.push_back('W');
		for (size_t i = 0; i < 4; ++i)
			data.push_back(static_cast<uint8_t>(w.pid >> (i * 8)));
		data.push_back(w.racewar);
		append_u32(&data, static_cast<uint32_t>(w.account_name.size()));
		data.insert(data.end(), w.account_name.begin(), w.account_name.end());
		append_u64(&data, w.native_revision);
		for (size_t i = 0; i < 4; ++i)
		{
			uint64_t val = static_cast<uint64_t>(w.balance[i]);
			for (size_t j = 0; j < 8; ++j)
				data.push_back(static_cast<uint8_t>(val >> (j * 8)));
		}
	}
	for (const auto &b : banks)
	{
		data.push_back('B');
		data.push_back(b.racewar);
		append_u32(&data, static_cast<uint32_t>(b.name.size()));
		data.insert(data.end(), b.name.begin(), b.name.end());
		append_u64(&data, b.native_revision);
		for (size_t i = 0; i < 4; ++i)
		{
			uint64_t val = static_cast<uint64_t>(b.balance[i]);
			for (size_t j = 0; j < 8; ++j)
				data.push_back(static_cast<uint8_t>(val >> (j * 8)));
		}
	}
	return hash(data);
}

template <typename F> unsigned int guarded(F &&action, std::string *error) noexcept
{
	try
	{
		action();
		return 0;
	}
	catch (const failure &f)
	{
		try
		{
			if (error && error->empty())
				*error = "lifecycle transaction failure code=" +
					 std::to_string(f.code);
		}
		catch (const std::bad_alloc &)
		{
		}
		return f.code;
	}
	catch (const std::bad_alloc &)
	{
		try
		{
			if (error && error->empty())
				*error = "out of memory";
		}
		catch (const std::bad_alloc &)
		{
		}
		return ENOMEM;
	}
	catch (...)
	{
		try
		{
			if (error && error->empty())
				*error = "unexpected lifecycle exception";
		}
		catch (const std::bad_alloc &)
		{
		}
		return EIO;
	}
}

} // namespace

unsigned int flatfile_accounting_lifecycle_transaction::capture_native_sources_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	flatfile_accounting_lifecycle_native_sources *sources, std::string *error) noexcept
{
	return guarded(
		[&]
		{
			need(sources != nullptr, EINVAL);
			flatfile_accounting_lifecycle_native_sources candidate;
			const auto code = capture_sources(root, identity_lock, authority_lock,
							  &candidate, error);
			need(!code, code);
			if (sources)
				*sources = std::move(candidate);
		},
		error);
}

unsigned int flatfile_accounting_lifecycle_transaction::install(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &lock, const flatfile_accounting_lifecycle_request &request,
	const economic_account_key &opening_account, flatfile_accounting_lifecycle_receipt *receipt,
	std::string *error) noexcept
{
	return guarded(
		[&]
		{
			need(!root.empty() && identity_lock.matches(root) && lock.matches(root),
			     EINVAL);
			need(nonzero(request.operation_id) && nonzero(request.lineage) &&
				     nonzero(request.epoch),
			     EINVAL);
			need(request.accepted_at_usec != 0, EINVAL);
			need(request.frozen_boundary_proven, EPERM);
			need(request.boundary_digest != economic_digest{}, EINVAL);
			need(economic_account_key_valid(opening_account), EINVAL);
			need(opening_account.kind == economic_account_kind::opening, EINVAL);
			need(opening_account.lineage.bytes == request.lineage.bytes, EINVAL);

			// Epoch activation waits for receipt and never_activated / virgin_state proof.
			need(request.virgin_state_proven, EPERM);

			flatfile_economic_control control;
			unsigned int control_code =
				flatfile_economic_control_read(root, lock, &control, error);
			need(!control_code, control_code);
			need(control.lineage.bytes == request.lineage.bytes, ESTALE);

			// Never-activated invariant: active_epoch must not already be selected.
			need(!nonzero(control.active_epoch), EALREADY);
			// The shared receipt bucket must already be initialized. This owner
			// does not manufacture that independent storage/lifecycle prerequisite.
			critical_operation_id baseline_operation = {};
			need(critical_operation_id_derive(request.operation_id,
							  ECONOMIC_BASELINE_OPERATION_DOMAIN, 0,
							  &baseline_operation),
			     EINVAL);
			const size_t receipt_bucket = baseline_operation.bytes[0];
			need(control.evidence_initialized[receipt_bucket / 8] &
				     (1u << (receipt_bucket % 8)),
			     ENODATA);
			const auto bucket_status = flatfile_accounting_check_bucket(
				root, lock, request.lineage, receipt_bucket, error);
			need(bucket_status == flatfile_accounting_status::ok,
			     bucket_status == flatfile_accounting_status::capacity ? ENOMEM :
			     bucket_status == flatfile_accounting_status::io_error ? EIO :
										     EILSEQ);
			flatfile_accounting_lifecycle_native_sources sources;
			const auto capture_code = capture_native_sources_locked(
				root, identity_lock, lock, &sources, error);
			need(!capture_code, capture_code);

			// Complete native capture covers wallets and shared banks.
			std::set<uint32_t> seen_wallets;
			for (const auto &w : sources.wallets)
			{
				need(w.pid != 0, EINVAL);
				need(seen_wallets.insert(w.pid).second, EINVAL);
			}

			std::set<std::pair<std::string, uint8_t>> seen_banks;
			for (const auto &b : sources.banks)
			{
				need(!b.name.empty(), EINVAL);
				need(seen_banks.insert({ b.name, b.racewar }).second, EINVAL);
			}

			// Bind coverage digest.
			economic_digest coverage =
				compute_coverage_digest(sources.wallets, sources.banks);
			if (request.coverage_digest != economic_digest{})
			{
				need(coverage == request.coverage_digest, EINVAL);
			}

			// Reconcile shared-bank lifetimes one-to-one. Missing or duplicate
			// shared-bank lifetimes must stop baseline staging.
			std::vector<flatfile_economic_mapping> mappings;
			mappings.reserve(sources.wallets.size() + sources.banks.size());

			std::vector<flatfile_authority_operation> ops;
			flatfile_accounting_staging_view view(root, lock, ops);

			// Reconcile wallets.
			for (const auto &w : sources.wallets)
			{
				flatfile_economic_locator loc{ 1, w.pid, "" };
				flatfile_economic_mapping mapping;
				unsigned int code = flatfile_economic_native_lookup(
					root, lock, economic_account_kind::wallet, 0, loc, &mapping,
					error);
				if (!code)
				{
					flatfile_economic_mapping verified;
					unsigned int read_code = flatfile_economic_mapping_read(
						root, lock, mapping.account, &verified, error);
					need(!read_code, read_code);
					need(!nonzero(verified.retiring_operation), ESTALE);
					mappings.push_back(verified);
				}
				else if (code == ENODATA)
				{
					flatfile_economic_mapping created;
					unsigned int create_code =
						flatfile_accounting_authority_storage::
							create_mapping_staged(
								root, lock, control.revision,
								economic_account_kind::wallet, 0,
								loc, request.operation_id, &created,
								&ops, error, &view);
					need(!create_code, create_code);
					const auto staged_control = view.control(&control, error);
					need(!staged_control, staged_control);
					mappings.push_back(created);
				}
				else
				{
					need(false, code);
				}
			}

			// Reconcile shared banks: one-to-one shared-bank lifetime reconciliation.
			for (const auto &b : sources.banks)
			{
				flatfile_economic_locator loc{ 2, 0, b.name };
				flatfile_economic_mapping mapping;
				unsigned int code = flatfile_economic_native_lookup(
					root, lock, economic_account_kind::bank, b.racewar, loc,
					&mapping, error);
				if (!code)
				{
					flatfile_economic_mapping verified;
					unsigned int read_code = flatfile_economic_mapping_read(
						root, lock, mapping.account, &verified, error);
					need(!read_code, read_code);
					need(!nonzero(verified.retiring_operation), ESTALE);
					mappings.push_back(verified);
				}
				else if (code == ENODATA)
				{
					flatfile_economic_mapping created;
					unsigned int create_code =
						flatfile_accounting_authority_storage::
							create_mapping_staged(
								root, lock, control.revision,
								economic_account_kind::bank,
								b.racewar, loc,
								request.operation_id, &created,
								&ops, error, &view);
					need(!create_code, create_code);
					const auto staged_control = view.control(&control, error);
					need(!staged_control, staged_control);
					mappings.push_back(created);
				}
				else
				{
					need(false, code);
				}
			}

			// Membership must precede marker initialization in the staged view.
			flatfile_economic_epoch epoch_info;
			const auto epoch_read_code =
				view.epoch(request.lineage, request.epoch, &epoch_info);
			if (epoch_read_code == ENODATA)
			{
				flatfile_economic_epoch next_epoch{
					request.epoch,
					control.last_epoch,
					request.operation_id,
					uint64_t(control.epoch_count) + 1,
					1,
					coverage
				};
				const auto append_code =
					flatfile_accounting_authority_storage::append_epoch_staged(
						root, lock, control.revision, next_epoch, &ops,
						error, &view);
				need(!append_code, append_code);
				const auto staged_control = view.control(&control, error);
				need(!staged_control, staged_control);
			}
			else
			{
				need(!epoch_read_code, epoch_read_code);
				// This installer may resume only its own matching transition.
				// General baseline initialization retains separate-ID semantics.
				need(epoch_info.creating_operation.bytes ==
						     request.operation_id.bytes &&
					     epoch_info.transition_kind == 1 &&
					     epoch_info.transition_digest == coverage,
				     ESTALE);
			}
			// Selection targets the requested epoch, never a different catalog tail.
			need(control.last_epoch.bytes == request.epoch.bytes, ESTALE);
			// Initialize baseline storage namespace for this epoch.
			flatfile_accounting_status init_status =
				flatfile_accounting_baseline_storage::initialize_staged(
					root, lock, request.lineage, request.epoch, opening_account,
					request.operation_id, &ops, error, &view);
			if (init_status != flatfile_accounting_status::ok &&
			    init_status != flatfile_accounting_status::already_exists)
			{
				need(false, EIO);
			}
			const auto initialized_control = view.control(&control, error);
			need(!initialized_control, initialized_control);

			// Prepare and stage the baseline batch.
			economic_baseline_batch batch;
			batch.lineage = request.lineage;
			batch.epoch = request.epoch;
			batch.preparation_id = request.operation_id;
			batch.actor_id = request.actor_id;
			batch.batch_index = 0;
			batch.opening_account = opening_account;
			batch.coverage_digest = coverage;
			batch.boundary_digest = request.boundary_digest;

			for (size_t i = 0; i < mappings.size(); ++i)
			{
				economic_baseline_holding holding;
				holding.account = mappings[i].account;
				if (i < sources.wallets.size())
				{
					holding.balance = sources.wallets[i].balance;
					holding.native_revision =
						sources.wallets[i].native_revision;
					holding.source_digest = sources.wallets[i].source_digest;
				}
				else
				{
					size_t bank_idx = i - sources.wallets.size();
					holding.balance = sources.banks[bank_idx].balance;
					holding.native_revision =
						sources.banks[bank_idx].native_revision;
					holding.source_digest =
						sources.banks[bank_idx].source_digest;
				}
				batch.holdings.push_back(holding);
			}

			std::optional<economic_prepared_baseline> prepared;
			economic_accounting_error prep_err =
				economic_baseline_prepare(batch, &prepared);
			need(prep_err == economic_accounting_error::ok && prepared.has_value(),
			     EINVAL);

			critical_command baseline_cmd;
			economic_accounting_error cmd_err = economic_baseline_command_build(
				*prepared, request.accepted_at_usec, &baseline_cmd);
			need(cmd_err == economic_accounting_error::ok, EINVAL);

			// Stage baseline reservations and witness.
			uint64_t verified_baseline_revision = 0;
			flatfile_accounting_status stage_status =
				flatfile_accounting_baseline_storage::stage_staged(
					root, lock, baseline_cmd, *prepared, &ops, error, &view,
					&verified_baseline_revision);
			if (stage_status != flatfile_accounting_status::ok &&
			    stage_status != flatfile_accounting_status::already_exists)
			{
				need(false, EIO);
			}
			need(verified_baseline_revision != 0, EILSEQ);

			// Receipt-bearing, never-activated-gated epoch selection.
			// Selection follows baseline receipts and external virgin-state proof.
			unsigned int select_code =
				flatfile_accounting_authority_storage::select_epoch_staged(
					root, lock, control.revision, true, request.operation_id,
					&ops, error, &view);
			need(!select_code, select_code);
			flatfile_accounting_lifecycle_receipt candidate_receipt;
			candidate_receipt.operation_id = request.operation_id;
			candidate_receipt.lineage = request.lineage;
			candidate_receipt.epoch = request.epoch;
			candidate_receipt.baseline_operation_id = baseline_cmd.operation_id;
			candidate_receipt.coverage_digest = coverage;
			candidate_receipt.boundary_digest = request.boundary_digest;
			candidate_receipt.baseline_revision = verified_baseline_revision;
			candidate_receipt.mappings = std::move(mappings);
			static_assert(std::is_nothrow_move_assignable_v<
				      flatfile_accounting_lifecycle_receipt>);

			// Commit authority and accounting operations atomically.
			flatfile_authority_transaction_result commit_res =
				flatfile_accounting_storage::commit(root, lock, ops, error);
			need(commit_res == flatfile_authority_transaction_result::ok, EIO);

			// Build and emit retained lifecycle receipt.
			if (receipt)
				*receipt = std::move(candidate_receipt);
		},
		error);
}
