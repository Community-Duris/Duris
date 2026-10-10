#include "flatfile/flatfile_accounting_native_mobile_birth_ordinary_transaction.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_baseline_history.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_initial.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_custody.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_physical.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_reference_history.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_ordinary_native_birth_receipt.h"
#include "flatfile/quest_mobile_native_flatfile.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <new>
#include <type_traits>
#include <utility>

namespace
{
struct failure
{
	unsigned int code;
};
void need(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code };
}
void checked(unsigned int code)
{
	if (code)
		throw failure{ code };
}
void checked(economic_accounting_error value)
{
	need(value == economic_accounting_error::ok,
	     value == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}
void checked(flatfile_accounting_status value)
{
	need(value == flatfile_accounting_status::ok,
	     value == flatfile_accounting_status::io_error  ? EIO :
	     value == flatfile_accounting_status::capacity  ? ENOSPC :
	     value == flatfile_accounting_status::not_found ? ENOENT :
	     value == flatfile_accounting_status::conflict ||
			     value == flatfile_accounting_status::already_exists ?
							      EEXIST :
							      EILSEQ);
}
void checked(flatfile_item_repository_result value)
{
	need(value == flatfile_item_repository_result::ok,
	     value == flatfile_item_repository_result::io_error	 ? EIO :
	     value == flatfile_item_repository_result::not_found ? ENOENT :
								   EILSEQ);
}
void checked(flatfile_item_accounting_status value)
{
	need(value == flatfile_item_accounting_status::ok,
	     value == flatfile_item_accounting_status::io_error	      ? EIO :
	     value == flatfile_item_accounting_status::capacity	      ? ENOSPC :
	     value == flatfile_item_accounting_status::already_exists ? EEXIST :
									EILSEQ);
}
struct original_values
{
	quest_mobile_native_image image;
	native_mobile_birth_recovery_context context;
	economic_frozen_intent intent;
};
original_values decode(const critical_native_recovery_envelope &original)
{
	need(native_mobile_birth_cash_role_recovery_valid(original), EINVAL);
	original_values value;
	checked(native_mobile_birth_cash_role_recovery_decode(original.command, original.attachment,
							      &value.context));
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	checked(native_mobile_birth_cash_role_command_decode(original.command, &value.image,
							     &recipes, &role));
	need(role.role == native_mobile_birth_cash_role::ordinary_wallet, ENOTSUP);
	need(value.image.cash.has_value() && value.image.cash->revision == 1, EINVAL);
	checked(economic_intent_decode(original.command.accounting_intent, &value.intent));
	checked(economic_intent_verify_binding(original.command, value.intent));
	need(value.intent.admission.metadata.source_event.has_value(), EINVAL);
	need(value.image.items.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS, E2BIG);
	return value;
}
std::vector<economic_accounting_item_reference> references(const critical_command &command,
							   const economic_accounting_plan &plan)
{
	std::vector<economic_accounting_item_reference> refs;
	refs.reserve(plan.item_events.size());
	for (const auto &event : plan.item_events)
	{
		need(event.event_index < UINT16_MAX, E2BIG);
		economic_accounting_item_reference ref{};
		ref.operation_id = command.operation_id;
		ref.line_index = event.event_index;
		ref.event_index = event.event_index;
		ref.child_index = event.child_index;
		ref.item_uid = event.uid;
		ref.before_revision = event.before.revision;
		ref.after_revision = event.after.revision;
		ref.legacy_operation_id = command.operation_id;
		ref.legacy_event_index = ref.line_index;
		need(economic_accounting_item_reference_validate(ref));
		refs.push_back(ref);
	}
	return refs;
}
critical_apply_result completion(const flatfile_accounting_record &record, bool replay)
{
	need(!record.result_code && record.failure_stage == critical_failure_stage::none &&
	     record.durable_revision == 1 &&
	     record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
	critical_apply_result result{ replay ? critical_apply_outcome::already_applied :
					       critical_apply_outcome::applied,
				      record.durable_revision, 0 };
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
	return result;
}
void verify_receipt(const critical_completion &receipt, const flatfile_accounting_record &record)
{
	need(receipt.disposition == critical_completion_disposition::execution &&
		     receipt.operation_id.bytes == record.command.operation_id.bytes &&
		     (receipt.outcome == critical_apply_outcome::applied ||
		      receipt.outcome == critical_apply_outcome::already_applied) &&
		     !receipt.error_code && receipt.failure_stage == critical_failure_stage::none &&
		     receipt.durable_revision == record.durable_revision &&
		     receipt.result_size == record.result.size() &&
		     std::equal(record.result.begin(), record.result.end(),
				receipt.result_payload.begin()) &&
		     std::all_of(receipt.result_payload.begin() + receipt.result_size,
				 receipt.result_payload.end(), [](uint8_t b) { return !b; }),
	     EEXIST);
}
void encode_image(const quest_mobile_native_image &image, std::vector<uint8_t> *output)
{
	const auto status = quest_mobile_native_image_encode(image, output);
	need(status == player_snapshot_codec_result::ok,
	     status == player_snapshot_codec_result::allocation_failure ? ENOMEM :
	     status == player_snapshot_codec_result::limit_exceeded	? E2BIG :
									  EBADMSG);
}
}

struct flatfile_accounting_native_mobile_birth_ordinary_transaction::implementation
{
	std::string root;
	const flatfile_identity_lock *identity = nullptr;
	const flatfile_authority_lock *lock = nullptr;
	bool commit_called = false, current_verified = false;
	critical_native_recovery_envelope original;
	flatfile_accounting_record record;
	native_mobile_birth_cash_role_result ordinary_result{};
	std::vector<flatfile_authority_operation> operations;
	flatfile_authority_commit_outcome outcome =
		flatfile_authority_commit_outcome::not_published;
};
flatfile_accounting_native_mobile_birth_ordinary_transaction::
	flatfile_accounting_native_mobile_birth_ordinary_transaction(
		std::unique_ptr<implementation> value)
	: state_(std::move(value))
{
}
flatfile_accounting_native_mobile_birth_ordinary_transaction::
	~flatfile_accounting_native_mobile_birth_ordinary_transaction() = default;

unsigned int flatfile_accounting_native_mobile_birth_ordinary_transaction::verify_record_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	flatfile_accounting_record *output) noexcept
{
	try
	{
		need(output && !root.empty() && identity.matches(root) && lock.matches(root),
		     EINVAL);
		const auto value = decode(original);
		flatfile_accounting_record record;
		flatfile_ordinary_native_birth_economic_history_counts history;
		checked(flatfile_ordinary_native_birth_receipt_storage::
				verify_retained_history_current_locked(
					root, lock, original.command.operation_id, &record,
					&history, nullptr));
		need(critical_command_equal(record.command, original.command), EEXIST);
		const auto proposal = completion(record, true);
		(void)proposal;
		if (value.context.receipt_present)
			verify_receipt(value.context.receipt, record);
		need(identity.matches(root) && lock.matches(root), EINVAL);
		static_assert(std::is_nothrow_move_assignable_v<flatfile_accounting_record>);
		*output = std::move(record);
		return 0;
	}
	catch (const failure &e)
	{
		return e.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
}

critical_apply_result
flatfile_accounting_native_mobile_birth_ordinary_transaction::verify_retained_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original) noexcept
{
	flatfile_accounting_record record;
	const auto error = verify_record_locked(root, identity, lock, original, &record);
	if (error)
		return { critical_apply_outcome::retryable_failure, 0, error };
	try
	{
		return completion(record, true);
	}
	catch (...)
	{
		return { critical_apply_outcome::retryable_failure, 0, EILSEQ };
	}
}

unsigned int flatfile_accounting_native_mobile_birth_ordinary_transaction::read_current_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	const critical_completion &receipt,
	flatfile_ordinary_native_birth_projection *output) noexcept
{
	try
	{
		need(output && identity.matches(root) && lock.matches(root), EINVAL);
		flatfile_accounting_record record;
		checked(verify_record_locked(root, identity, lock, original, &record));
		verify_receipt(receipt, record);
		const auto value = decode(original);
		native_mobile_birth_cash_role_result result;
		need(native_mobile_birth_cash_role_result_decode(record.result, &result) &&
		     result.role == native_mobile_birth_cash_role::ordinary_wallet);
		const economic_account_key wallet{ value.intent.admission.metadata.lineage,
						   economic_account_kind::wallet,
						   result.wallet_mapping_id,
						   ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		flatfile_ordinary_native_birth_projection current;
		checked(flatfile_native_mobile_wallet_storage::observe_current_locked(
			root, lock, value.intent.admission.metadata.epoch, wallet,
			value.image.reference, &current.wallet));
		need(!current.wallet.mapping.revision &&
			     current.wallet.mapping.last_operation.bytes ==
				     original.command.operation_id.bytes &&
			     current.wallet.mapping.locator.name.empty(),
		     ESTALE);
		std::vector<uint8_t> actual_image, original_image;
		encode_image(current.wallet.native, &actual_image);
		encode_image(value.image, &original_image);
		need(actual_image == original_image, ESTALE);
		flatfile_native_mobile_birth_ordinary_current_custody custody;
		checked(flatfile_native_mobile_birth_ordinary_custody_storage::read_locked(
			root, lock, original, record, &custody, nullptr));
		need(custody.owner_revision == 1 && custody.rows.size() == value.image.items.size(),
		     ESTALE);
		current.owner_revision = custody.owner_revision;
		current.custody = std::move(custody.rows);
		// Original SQL verify_current repeats physical_absence after custody.
		// Observe actual covered physical catalogs at this SAME identity->
		// authority cut, including recovery/reconciliation, before success.
		// Native .qmn and item_ownership are separate authority namespaces;
		// the genuine helper does not treat our own committed rows as conflict.
		flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence physical;
		checked(flatfile_native_mobile_birth_ordinary_physical_storage::
				verify_catalog_namespaces_locked(root, identity, lock, original,
								 &physical, nullptr));
		need(identity.matches(root) && lock.matches(root), EINVAL);
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_ordinary_native_birth_projection>);
		*output = std::move(current);
		return 0;
	}
	catch (const failure &e)
	{
		return e.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
}

unsigned int flatfile_accounting_native_mobile_birth_ordinary_transaction::prepare_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	std::unique_ptr<flatfile_accounting_native_mobile_birth_ordinary_transaction>
		*output) noexcept
{
	try
	{
		need(output && !root.empty() && identity.matches(root) && lock.matches(root) &&
			     native_mobile_birth_cash_role_recovery_initial(original),
		     EINVAL);
		const auto value = decode(original);
		flatfile_accounting_record existing;
		flatfile_ordinary_native_birth_economic_history_counts existing_history;
		const auto found = flatfile_ordinary_native_birth_receipt_storage::
			verify_retained_history_current_locked(root, lock,
							       original.command.operation_id,
							       &existing, &existing_history,
							       nullptr);
		if (found == flatfile_accounting_status::ok)
		{
			checked(verify_record_locked(root, identity, lock, original, &existing));
			return EALREADY;
		}
		need(found == flatfile_accounting_status::not_found,
		     found == flatfile_accounting_status::io_error ? EIO : EILSEQ);
		flatfile_ordinary_native_birth_economic_history_counts history;
		checked(flatfile_ordinary_native_birth_receipt_storage::
				verify_initial_history_absence_locked(root, lock, original,
								      &history, nullptr));
		flatfile_native_mobile_birth_ordinary_reference_absence reference_history;
		checked(flatfile_native_mobile_birth_ordinary_reference_history_storage::
				verify_initial_absence_locked(root, lock, original,
							      &reference_history, nullptr));
		flatfile_native_mobile_birth_ordinary_reference_quarantine_absence quarantine;
		checked(flatfile_native_mobile_birth_ordinary_reference_history_storage::
				verify_initial_quarantine_absence_locked(root, lock, original,
									 &quarantine, nullptr));
		// Applicable flat domains: actual retained player/pet/world/locker/SHOP,
		// auction and collector. Siege runtime is retired; restitution SQL-only.
		// Actual live-world/source/liveness proof stays with the sole root owner.
		flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence physical;
		checked(flatfile_native_mobile_birth_ordinary_physical_storage::
				verify_catalog_namespaces_locked(root, identity, lock, original,
								 &physical, nullptr));
		flatfile_native_mobile_birth_ordinary_retained_metadata metadata;
		checked(flatfile_native_mobile_birth_ordinary_baseline_history_storage::
				read_metadata_locked(root, lock, &metadata, nullptr));
		need(metadata.control.lineage.bytes == history.lineage.bytes &&
			     metadata.control.revision == history.lineage_revision &&
			     metadata.control.epochs_digest == history.epochs_digest,
		     ESTALE);
		flatfile_native_mobile_birth_ordinary_initial_stage mapping;
		checked(flatfile_native_mobile_birth_ordinary_initial_storage::prepare_locked(
			root, lock, original, metadata.control.revision, &mapping, nullptr));
		need(mapping.operations.size() == 3 &&
			     mapping.lineage_revision_before == metadata.control.revision &&
			     mapping.lineage_revision_before != UINT64_MAX &&
			     mapping.lineage_revision_after == mapping.lineage_revision_before + 1,
		     ESTALE);
		flatfile_native_mobile_birth_ordinary_custody_stage custody;
		checked(flatfile_native_mobile_birth_ordinary_custody_storage::prepare_locked(
			root, lock, original, mapping.mapping, &custody, nullptr));
		need(custody.owner_revision_after == 1 &&
		     custody.catalog_revision_before != UINT64_MAX &&
		     custody.catalog_revision_after == custody.catalog_revision_before + 1 &&
		     custody.plan.children.empty());
		quest_mobile_native_flatfile_row native;
		checked(quest_mobile_native_flatfile_read_locked(
			root, lock, value.image.reference.mobile_instance_id, &native));
		need(!native.present, EEXIST);
		flatfile_authority_operation native_operation;
		checked(quest_mobile_native_flatfile_prepare_locked(
			root, lock, original.command.operation_id, native, value.image,
			&native_operation));
		auto state = std::make_unique<implementation>();
		state->root = root;
		state->identity = &identity;
		state->lock = &lock;
		state->original = original;
		auto &record = state->record;
		record.command = original.command;
		checked(economic_plan_encode(custody.plan, &record.plan));
		checked(native_mobile_birth_cash_role_result_build(
			original.command, mapping.mapping.account, custody.plan,
			&state->ordinary_result));
		std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> bytes{};
		need(native_mobile_birth_cash_role_result_encode(state->ordinary_result, &bytes));
		record.result.assign(bytes.begin(), bytes.end());
		record.durable_revision = 1;
		state->operations = std::move(mapping.operations);
		state->operations.push_back(std::move(custody.operation));
		state->operations.push_back(std::move(native_operation));
		const auto refs = references(original.command, custody.plan);
		checked(flatfile_native_mobile_birth_ordinary_reference_stage_storage::stage_locked(
			root, lock, original.command.operation_id, refs, &state->operations,
			nullptr));
		checked(flatfile_accounting_storage::stage_ordinary_locked(
			root, lock, record, &state->operations, nullptr));
		checked(flatfile_accounting_storage::stage_source_claim(
			root, lock, record, &state->operations, nullptr));
		need(identity.matches(root) && lock.matches(root), EINVAL);
		auto prepared =
			std::unique_ptr<flatfile_accounting_native_mobile_birth_ordinary_transaction>(
				new flatfile_accounting_native_mobile_birth_ordinary_transaction(
					std::move(state)));
		*output = std::move(prepared);
		return 0;
	}
	catch (const failure &e)
	{
		return e.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
}

critical_apply_result flatfile_accounting_native_mobile_birth_ordinary_transaction::commit_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock) noexcept
{
	critical_apply_result proposed{ critical_apply_outcome::retryable_failure, 0, EINVAL };
	try
	{
		need(state_ != nullptr, EINVAL);
		state_->current_verified = false;
		proposed = completion(state_->record, false);
		need(!state_->commit_called && state_->root == root &&
			     state_->identity == &identity && state_->lock == &lock &&
			     identity.matches(root) && lock.matches(root),
		     EINVAL);
		if (state_->outcome == flatfile_authority_commit_outcome::not_published)
		{
			// Clear stack-lock identity BEFORE the first actual commit cut so
			// every success/exception/refusal leaves no cross-callback pointer.
			state_->commit_called = true;
			state_->identity = nullptr;
			state_->lock = nullptr;
			const auto committed = flatfile_accounting_storage::commit_with_outcome(
				root, lock, state_->operations, nullptr, &state_->outcome);
			need(committed == flatfile_authority_transaction_result::ok, EIO);
		}
		// Once publication is possible, the exact original proposal is retained.
		// Actual successful commit completed the journal before passive receipt proof.
		// A missing/corrupt proof stays ambiguous; never reapply those images.
		flatfile_accounting_record retained;
		checked(verify_record_locked(root, identity, lock, state_->original, &retained));
		need(retained.plan == state_->record.plan &&
		     retained.result == state_->record.result &&
		     retained.durable_revision == state_->record.durable_revision);
		critical_completion receipt{};
		receipt.operation_id = state_->original.command.operation_id;
		receipt.outcome = proposed.outcome;
		receipt.durable_revision = proposed.durable_revision;
		receipt.result_size = proposed.result_size;
		receipt.result_payload = proposed.result_payload;
		flatfile_ordinary_native_birth_projection current;
		checked(read_current_locked(root, identity, lock, state_->original, receipt,
					    &current));
		state_->current_verified = true;
		return proposed;
	}
	catch (const failure &e)
	{
		if (state_ && state_->outcome != flatfile_authority_commit_outcome::not_published)
		{
			proposed.outcome = critical_apply_outcome::ambiguous_commit;
			return proposed;
		}
		return { critical_apply_outcome::retryable_failure, 0, e.code };
	}
	catch (const std::bad_alloc &)
	{
		if (state_ && state_->outcome != flatfile_authority_commit_outcome::not_published)
		{
			proposed.outcome = critical_apply_outcome::ambiguous_commit;
			return proposed;
		}
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	}
	catch (...)
	{
		if (state_ && state_->outcome != flatfile_authority_commit_outcome::not_published)
		{
			proposed.outcome = critical_apply_outcome::ambiguous_commit;
			return proposed;
		}
		return { critical_apply_outcome::retryable_failure, 0, EFAULT };
	}
}

bool flatfile_accounting_native_mobile_birth_ordinary_transaction::publication_possible()
	const noexcept
{
	return state_ && state_->outcome != flatfile_authority_commit_outcome::not_published;
}

bool flatfile_accounting_native_mobile_birth_ordinary_transaction::retained_bytes(
	size_t *output) const noexcept
{
	if (!output || !state_)
		return false;
	size_t total = 0;
	const auto add = [&](size_t bytes) noexcept
	{
		if (bytes > SIZE_MAX - total)
			return false;
		total += bytes;
		return true;
	};
	const auto array = [&](size_t capacity, size_t element) noexcept
	{ return capacity <= SIZE_MAX / element && add(capacity * element); };
	const auto string = [&](const std::string &value) noexcept
	{ return add(value.capacity()) && add(1); };
	const auto command = [&](const critical_command &value) noexcept
	{
		return array(value.keys.capacity(), sizeof(critical_entity_key)) &&
		       array(value.expected_revisions.capacity(),
			     sizeof(critical_expected_revision)) &&
		       add(value.payload.capacity()) && add(value.accounting_intent.capacity());
	};
	// sizeof(state) includes envelope/command/record/string/vector objects and
	// fixed outcome/identity/latches exactly once. Vector storage additionally
	// owns capacity operation objects, including unused slots. Live operations
	// own filename and byte heaps; unused vector slots own no live nested heap.
	if (!add(sizeof(*this)) || !add(sizeof(implementation)) || !string(state_->root) ||
	    !command(state_->original.command) || !add(state_->original.attachment.capacity()) ||
	    !command(state_->record.command) || !add(state_->record.plan.capacity()) ||
	    !add(state_->record.result.capacity()) ||
	    !array(state_->operations.capacity(), sizeof(flatfile_authority_operation)))
		return false;
	for (const auto &operation : state_->operations)
		if (!string(operation.filename) || !add(operation.bytes.capacity()))
			return false;
	*output = total;
	return true;
}

critical_apply_result
flatfile_accounting_native_mobile_birth_ordinary_transaction::reconcile_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock) noexcept
{
	critical_apply_result proposed{ critical_apply_outcome::retryable_failure, 0, EINVAL };
	try
	{
		need(state_ != nullptr, EINVAL);
		state_->current_verified = false;
		proposed = completion(state_->record, true);
		need(publication_possible() && state_->root == root && identity.matches(root) &&
			     lock.matches(root),
		     EINVAL);
		// The genuine fresh recovered same-root lock protects actual original
		// retained proof and CURRENT cut. No old stack lock or proposal writes.
		flatfile_accounting_record retained;
		checked(verify_record_locked(root, identity, lock, state_->original, &retained));
		need(retained.plan == state_->record.plan &&
		     retained.result == state_->record.result &&
		     retained.durable_revision == state_->record.durable_revision);
		critical_completion receipt{};
		receipt.operation_id = state_->original.command.operation_id;
		receipt.outcome = proposed.outcome;
		receipt.durable_revision = proposed.durable_revision;
		receipt.result_size = proposed.result_size;
		receipt.result_payload = proposed.result_payload;
		flatfile_ordinary_native_birth_projection current;
		checked(read_current_locked(root, identity, lock, state_->original, receipt,
					    &current));
		state_->current_verified = true;
		return proposed;
	}
	catch (const failure &e)
	{
		if (publication_possible())
		{
			proposed.outcome = critical_apply_outcome::ambiguous_commit;
			return proposed;
		}
		return { critical_apply_outcome::retryable_failure, 0, e.code };
	}
	catch (const std::bad_alloc &)
	{
		if (publication_possible())
		{
			proposed.outcome = critical_apply_outcome::ambiguous_commit;
			return proposed;
		}
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	}
	catch (...)
	{
		if (publication_possible())
		{
			proposed.outcome = critical_apply_outcome::ambiguous_commit;
			return proposed;
		}
		return { critical_apply_outcome::retryable_failure, 0, EFAULT };
	}
}

const native_mobile_birth_cash_role_result *
flatfile_accounting_native_mobile_birth_ordinary_transaction::ordinary_wallet_result() const noexcept
{
	return state_ && state_->current_verified ? &state_->ordinary_result : nullptr;
}

unsigned int flatfile_native_mobile_birth_ordinary_publication_storage::read_current_locked(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	const critical_completion &receipt,
	flatfile_ordinary_native_birth_projection *output) noexcept
{
	return flatfile_accounting_native_mobile_birth_ordinary_transaction::read_current_locked(
		root, identity, lock, original, receipt, output);
}
