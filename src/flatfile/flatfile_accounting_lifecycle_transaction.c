#include "flatfile/flatfile_accounting_lifecycle_transaction.h"
#include "flatfile/flatfile_accounting_store.h"
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

economic_digest hash(std::span<const uint8_t> data)
{
	economic_digest value = {};
	SHA256(data.data(), data.size(), value.data());
	return value;
}

economic_digest
compute_coverage_digest(const std::vector<flatfile_accounting_lifecycle_wallet_source> &wallets,
			const std::vector<flatfile_accounting_lifecycle_bank_source> &banks)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'F', 'L', 'A', 'T', 'F', 'I', 'L',
				   'E', '-', 'C', 'O', 'V', 'E', 'R', 'A', 'G', 'E', '-', 'V', '1' };
	for (const auto &w : wallets)
	{
		for (size_t i = 0; i < 4; ++i)
			data.push_back(static_cast<uint8_t>(w.pid >> (i * 8)));
		for (size_t i = 0; i < 4; ++i)
		{
			uint64_t val = static_cast<uint64_t>(w.balance[i]);
			for (size_t j = 0; j < 8; ++j)
				data.push_back(static_cast<uint8_t>(val >> (j * 8)));
		}
	}
	for (const auto &b : banks)
	{
		data.push_back(b.racewar);
		data.insert(data.end(), b.name.begin(), b.name.end());
		data.push_back(0);
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
		if (error && error->empty())
			*error = "lifecycle transaction failure code=" + std::to_string(f.code);
		return f.code;
	}
	catch (const std::bad_alloc &)
	{
		if (error && error->empty())
			*error = "out of memory";
		return ENOMEM;
	}
	catch (...)
	{
		if (error && error->empty())
			*error = "unexpected lifecycle exception";
		return EIO;
	}
}

} // namespace

unsigned int flatfile_accounting_lifecycle_transaction::install(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_lifecycle_request &request,
	const flatfile_accounting_lifecycle_native_sources &sources,
	const economic_account_key &opening_account, flatfile_accounting_lifecycle_receipt *receipt,
	std::string *error) noexcept
{
	return guarded(
		[&]
		{
			need(!root.empty(), EINVAL);
			need(nonzero(request.operation_id) && nonzero(request.lineage) &&
				     nonzero(request.epoch),
			     EINVAL);
			need(request.accepted_at_usec != 0, EINVAL);
			need(economic_account_key_valid(opening_account), EINVAL);
			need(opening_account.kind == economic_account_kind::opening, EINVAL);
			need(opening_account.lineage.bytes == request.lineage.bytes, EINVAL);

			// Epoch activation waits for receipt and never_activated / virgin_state proof.
			need(request.virgin_state_proven, EPERM);

			flatfile_economic_control control;
			unsigned int control_code =
				flatfile_economic_control_read(root, lock, &control, error);
			need(!control_code, control_code);

			// Never-activated invariant: active_epoch must not already be selected.
			need(!nonzero(control.active_epoch), EALREADY);

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
						flatfile_accounting_authority_storage::create_mapping(
							root, lock, control.revision,
							economic_account_kind::wallet, 0, loc,
							request.operation_id, &created, &ops,
							error);
					need(!create_code, create_code);
					++control.revision;
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
						flatfile_accounting_authority_storage::create_mapping(
							root, lock, control.revision,
							economic_account_kind::bank, b.racewar, loc,
							request.operation_id, &created, &ops,
							error);
					need(!create_code, create_code);
					++control.revision;
					mappings.push_back(created);
				}
				else
				{
					need(false, code);
				}
			}

			// Initialize baseline storage namespace for this epoch.
			flatfile_accounting_status init_status =
				flatfile_accounting_baseline_storage::initialize(
					root, lock, request.lineage, request.epoch, opening_account,
					request.operation_id, &ops, error);
			if (init_status != flatfile_accounting_status::ok &&
			    init_status != flatfile_accounting_status::already_exists)
			{
				need(false, EIO);
			}

			// Prepare and stage the baseline batch.
			economic_baseline_batch batch;
			batch.lineage = request.lineage;
			batch.epoch = request.epoch;
			batch.preparation_id = request.operation_id;
			batch.actor_id = request.actor_id;
			batch.batch_index = 0;
			batch.opening_account = opening_account;
			batch.coverage_digest = coverage;

			for (size_t i = 0; i < mappings.size(); ++i)
			{
				economic_baseline_holding holding;
				holding.account = mappings[i].account;
				holding.native_revision = mappings[i].revision;
				if (i < sources.wallets.size())
				{
					holding.balance = sources.wallets[i].balance;
				}
				else
				{
					size_t bank_idx = i - sources.wallets.size();
					holding.balance = sources.banks[bank_idx].balance;
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
			flatfile_accounting_status stage_status =
				flatfile_accounting_baseline_storage::stage(
					root, lock, baseline_cmd, *prepared, &ops, error);
			if (stage_status != flatfile_accounting_status::ok &&
			    stage_status != flatfile_accounting_status::already_exists)
			{
				need(false, EIO);
			}

			// Append epoch to authority if not present.
			flatfile_economic_epoch epoch_info;
			unsigned int epoch_read_code = flatfile_economic_epoch_read(
				root, lock, request.lineage, request.epoch, &epoch_info, nullptr);
			if (epoch_read_code == ENODATA)
			{
				flatfile_economic_epoch next_epoch{
					request.epoch,		 {}, request.operation_id,
					control.epoch_count + 1, 1,  coverage
				};
				unsigned int append_code =
					flatfile_accounting_authority_storage::append_epoch(
						root, lock, control.revision, next_epoch, &ops,
						error);
				need(!append_code, append_code);
				++control.revision;
			}

			// Receipt-bearing, never-activated-gated epoch selection.
			// Selection follows baseline receipts and external virgin-state proof.
			unsigned int select_code =
				flatfile_accounting_authority_storage::select_epoch(
					root, lock, control.revision, true, request.operation_id,
					&ops, error);
			need(!select_code, select_code);

			// Commit authority and accounting operations atomically.
			flatfile_authority_transaction_result commit_res =
				flatfile_accounting_storage::commit(root, lock, ops, error);
			need(commit_res == flatfile_authority_transaction_result::ok, EIO);

			// Build and emit retained lifecycle receipt.
			if (receipt)
			{
				receipt->operation_id = request.operation_id;
				receipt->lineage = request.lineage;
				receipt->epoch = request.epoch;
				receipt->baseline_operation_id = baseline_cmd.operation_id;
				receipt->coverage_digest = coverage;
				receipt->baseline_revision = 1;
				receipt->mappings = std::move(mappings);
			}
		},
		error);
}
