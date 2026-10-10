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

namespace
{
constexpr size_t ordinary_current_library_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t ordinary_current_library_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t ordinary_current_library_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t ordinary_current_library_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t ordinary_current_library_vector_frames =
	ordinary_current_library_allocator_frames + ordinary_current_library_copy_frames +
	ordinary_current_library_relocate_frames + ordinary_current_library_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t ordinary_current_library_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + ordinary_current_library_allocator_frames;
constexpr size_t ordinary_current_library_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + ordinary_current_library_vector_frames;
constexpr size_t ordinary_current_library_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t ordinary_current_library_string_frames =
	ordinary_current_library_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	ordinary_current_library_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);
constexpr size_t ordinary_current_library_string_move_frames =
	2 * sizeof(void *) + sizeof(char) + sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	12 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(bool) + 2 * sizeof(void *) +
	sizeof(char) + ordinary_current_library_string_frames;
// Full canonical command encoder/profile/append source scopes, beside genuine
// reserve/insert/push_back/copy/allocator/move carriers above. Working-byte
// helper counts its actual private output vector and fresh wire request.
constexpr size_t ordinary_current_encoder_source_frames =
	ordinary_current_library_vector_frames + ordinary_current_library_move_frames +
	// encode: command/output/result arguments, wire_bytes/status, pad/key/revision;
	// bounded/profile: actual working/status/outer and callback/context carriers.
	14 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(unsigned int) +
	2 * sizeof(critical_command_codec_result) +
	// append_le<uint64_t>, largest genuine transported value + index + byte.
	sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t) +
	// envelope_encoded_size's size/result/lambda/width/count and actual key-limit
	// validity references/scalars; original full envelope check remains separate.
	6 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(bool);
// Original MBR4 decode and its canonical encode are pure allocation-free.
// All actual candidate/canonical objects/fields/clocks and by-value span are
// admitted prospectively; full original decoder still performs every check.
constexpr size_t ordinary_current_result_source_frames =
	sizeof(native_mobile_birth_cash_role_result) +
	2 * sizeof(std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES>) +
	sizeof(std::span<const uint8_t>) + 11 * sizeof(uint64_t) + 5 * sizeof(size_t) +
	12 * sizeof(void *) + 3 * sizeof(bool) +
	// get/put actual input/output/value/size/index and uint64_t transport;
	// digest/born-cash predicates and shared-participant validation values.
	4 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(uint64_t) + sizeof(int64_t) +
	sizeof(uint8_t) + ordinary_current_library_copy_frames;

struct ordinary_current_refusal
{
	unsigned int code;
};
size_t ordinary_current_add(size_t left, size_t right)
{
	if (right > SIZE_MAX - left)
		throw ordinary_current_refusal{ ENOBUFS };
	return left + right;
}
struct ordinary_current_reservation
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	bool refused = false;
	static bool callback(size_t bytes, void *opaque) noexcept
	{
		auto &self = *static_cast<ordinary_current_reservation *>(opaque);
		if (self.refused || !self.reserve || !self.reserve(bytes, self.context))
		{
			self.refused = true;
			return false;
		}
		return true;
	}
};
struct ordinary_current_values
{
	original_values value;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	std::span<const uint8_t> attachment, intent_wire;
	size_t context_heap = 0, image_heap = 0, recipe_heap = 0;
};
size_t ordinary_current_heap(const ordinary_current_values &work)
{
	return ordinary_current_add(
		work.context_heap,
		ordinary_current_add(
			work.image_heap,
			ordinary_current_add(work.recipe_heap,
					     work.value.intent.admission.facts.capacity())));
}
struct ordinary_current_record_workspace
{
	ordinary_current_values values;
	flatfile_accounting_record record;
	flatfile_ordinary_native_birth_economic_history_counts history;
	std::vector<uint8_t> left_command, right_command;
	critical_apply_result proposal{ critical_apply_outcome::already_applied, 0, 0 };
	ordinary_current_reservation reservation;
	size_t record_heap = 0, index = 0, command_request = 0;
	int storage_errno = 0;
	flatfile_accounting_status receipt_status = flatfile_accounting_status::invalid;
	critical_command_codec_result command_status = critical_command_codec_result::invalid;
};
size_t ordinary_current_heap(const ordinary_current_record_workspace &work)
{
	return ordinary_current_add(
		ordinary_current_heap(work.values),
		ordinary_current_add(work.record_heap,
				     ordinary_current_add(work.left_command.capacity(),
							  work.right_command.capacity())));
}
struct ordinary_current_projection_workspace
{
	flatfile_accounting_record record;
	ordinary_current_values values;
	native_mobile_birth_cash_role_result result;
	economic_account_key wallet;
	flatfile_ordinary_native_birth_projection current;
	std::vector<uint8_t> actual_image, original_image;
	flatfile_native_mobile_birth_ordinary_current_custody custody;
	flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence physical;
	ordinary_current_reservation reservation;
	size_t record_heap = 0, wallet_heap = 0, custody_heap = 0, retained = 0, index = 0;
	unsigned int status = 0;
	int storage_errno = 0;
	player_snapshot_codec_result image_status = player_snapshot_codec_result::invalid_value;
	flatfile_item_repository_result custody_status = flatfile_item_repository_result::invalid;
};
size_t ordinary_current_heap(const ordinary_current_projection_workspace &work)
{
	return ordinary_current_add(
		ordinary_current_heap(work.values),
		ordinary_current_add(
			work.record_heap,
			ordinary_current_add(
				work.wallet_heap,
				ordinary_current_add(
					work.actual_image.capacity(),
					ordinary_current_add(work.original_image.capacity(),
							     work.custody_heap)))));
}
// Genuine source carriers shared by the actual bounded wrappers/callbacks and
// same-root borrowed-lock predicates. These count declared source storage;
// emitted stack, allocator/runtime internals remain qualification obligations.
constexpr size_t ordinary_current_call_frames =
	// Wrapper root/identity/authority/envelope/receipt/output/callback/context,
	// outer/scalar-output arguments; nested helper reference/results and catches.
	12 * sizeof(void *) + sizeof(flatfile_scratch_reserve_fn) + 6 * sizeof(size_t) +
	3 * sizeof(unsigned int) + sizeof(ordinary_current_refusal) + sizeof(failure) +
	2 * sizeof(economic_accounting_error) + sizeof(bool) + sizeof(int) +
	// callback(bytes,opaque), actual self reference and original callback result.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// matches(root), each lock/root reference and returned comparison result.
	4 * sizeof(void *) + 2 * sizeof(bool);
// Vectors move through their genuine libstdc++13 operator=/_M_move_assign,
// _M_swap_data and destroy/deallocate call closure. The source data __tmp is
// three actual pointers; the owned vector temporary is a real vector object.
constexpr size_t ordinary_current_move_frames =
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) + sizeof(void *) +
	// allocator deallocate(this,p,n), traits allocator/p/n, forwarding operands.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::allocator<uint8_t>);
template <typename Workspace> struct ordinary_current_budget
{
	size_t outer, fixed;
	Workspace &work;
	size_t live() const
	{
		return ordinary_current_add(ordinary_current_add(outer, fixed),
					    ordinary_current_heap(work));
	}
	void admit(size_t extra = 0) const
	{
		if (!ordinary_current_reservation::callback(ordinary_current_add(live(), extra),
							    &work.reservation))
			throw ordinary_current_refusal{ ENOBUFS };
	}
	void codec(economic_accounting_error status, unsigned int malformed = EILSEQ) const
	{
		if (status == economic_accounting_error::ok)
			return;
		if (work.reservation.refused || status == economic_accounting_error::capacity)
			throw ordinary_current_refusal{ ENOBUFS };
		if (status == economic_accounting_error::unresolved)
			throw ordinary_current_refusal{ ENOTSUP };
		throw ordinary_current_refusal{ malformed };
	}
};
template <typename Workspace>
void ordinary_current_decode(const critical_native_recovery_envelope &original,
			     ordinary_current_values &values,
			     ordinary_current_budget<Workspace> &budget)
{
	// Original validity pass, original full context decode, original full command
	// decode and original complete intent/binding validation all remain separate.
	// Keeping recipes as actual workspace members retains their genuine capacity
	// across later calls; no vanished transient storage is represented as output.
	budget.admit();
	budget.codec(native_mobile_birth_cash_role_recovery_validate_bounded(
			     original, ordinary_current_reservation::callback,
			     &budget.work.reservation, budget.live()),
		     EINVAL);
	values.attachment = original.attachment;
	budget.codec(native_mobile_birth_cash_role_recovery_decode_status_bounded(
		original.command, values.attachment, &values.value.context,
		ordinary_current_reservation::callback, &budget.work.reservation, budget.live(),
		&values.context_heap));
	budget.admit();
	budget.codec(native_mobile_birth_cash_role_command_decode_bounded(
		original.command, &values.value.image, &values.recipes, &values.role,
		ordinary_current_reservation::callback, &budget.work.reservation, budget.live(),
		&values.image_heap, &values.recipe_heap));
	need(values.role.role == native_mobile_birth_cash_role::ordinary_wallet, ENOTSUP);
	need(values.value.image.cash.has_value() && values.value.image.cash->revision == 1, EINVAL);
	values.intent_wire = original.command.accounting_intent;
	budget.codec(economic_intent_decode_bounded(values.intent_wire, &values.value.intent,
						    ordinary_current_reservation::callback,
						    &budget.work.reservation, budget.live()));
	budget.admit();
	budget.codec(economic_intent_verify_binding_bounded(
		original.command, values.value.intent, ordinary_current_reservation::callback,
		&budget.work.reservation, budget.live()));
	need(values.value.intent.admission.metadata.source_event.has_value(), EINVAL);
	need(values.value.image.items.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS, E2BIG);
}
// Identical complete receipt comparison expressed as fixed direct byte loops.
// Its actual index belongs to the prospective workspace, and no algorithm
// recursion or temporary owner substitutes for the original full tail check.
void ordinary_current_receipt(const critical_completion &receipt,
			      const flatfile_accounting_record &record, size_t &index)
{
	need(receipt.disposition == critical_completion_disposition::execution &&
		     receipt.operation_id.bytes == record.command.operation_id.bytes &&
		     (receipt.outcome == critical_apply_outcome::applied ||
		      receipt.outcome == critical_apply_outcome::already_applied) &&
		     !receipt.error_code && receipt.failure_stage == critical_failure_stage::none &&
		     receipt.durable_revision == record.durable_revision &&
		     receipt.result_size == record.result.size(),
	     EEXIST);
	for (index = 0; index < record.result.size(); ++index)
		need(record.result[index] == receipt.result_payload[index], EEXIST);
	for (index = receipt.result_size; index < receipt.result_payload.size(); ++index)
		need(!receipt.result_payload[index], EEXIST);
}
} // namespace

unsigned int
flatfile_accounting_native_mobile_birth_ordinary_transaction::verify_record_locked_bounded(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	flatfile_accounting_record *output, flatfile_scratch_reserve_fn reserve, void *context,
	size_t outer_live, size_t *retained_record_heap) noexcept
{
	if (!output || root.empty() || !identity.matches(root) || !lock.matches(root))
		return EINVAL;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||          \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)original;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_record_heap;
	return ENOTSUP;
#else
	try
	{
		const size_t fixed =
			sizeof(ordinary_current_record_workspace) +
			sizeof(ordinary_current_budget<ordinary_current_record_workspace>) +
			ordinary_current_call_frames + ordinary_current_move_frames +
			ordinary_current_library_string_move_frames +
			ordinary_current_library_vector_constructor_frames +
			ordinary_current_encoder_source_frames +
			critical_command_valid_frame_bytes();
		if (!reserve || !reserve(ordinary_current_add(outer_live, fixed), context))
			return ENOBUFS;
		ordinary_current_record_workspace work;
		work.reservation = { reserve, context, false };
		ordinary_current_budget<ordinary_current_record_workspace> budget{ outer_live,
										   fixed, work };
		ordinary_current_decode(original, work.values, budget);
		errno = 0;
		work.receipt_status = flatfile_ordinary_native_birth_receipt_storage::
			verify_retained_history_current_locked_bounded(
				root, lock, original.command.operation_id, &work.record,
				&work.history, ordinary_current_reservation::callback,
				&work.reservation, budget.live(), &work.record_heap);
		work.storage_errno = errno;
		if (work.reservation.refused)
			return ENOBUFS;
		if (work.receipt_status == flatfile_accounting_status::io_error &&
		    work.storage_errno == ENOTSUP)
			return ENOTSUP;
		checked(work.receipt_status);
		budget.admit();
		// Original critical_command_equal compares BOTH complete canonical encodes.
		// Its old failure remains EEXIST unless actual reservation refusal latched.
		work.command_status = critical_command_encoder_working_bytes(work.record.command,
									     &work.command_request);
		need(work.command_status == critical_command_codec_result::ok, EEXIST);
		budget.admit(work.command_request);
		work.command_status = critical_command_encode_bounded(
			work.record.command, &work.left_command,
			ordinary_current_reservation::callback, &work.reservation, budget.live());
		if (work.reservation.refused)
			return ENOBUFS;
		need(work.command_status == critical_command_codec_result::ok, EEXIST);
		budget.admit();
		work.command_status = critical_command_encoder_working_bytes(original.command,
									     &work.command_request);
		need(work.command_status == critical_command_codec_result::ok, EEXIST);
		budget.admit(work.command_request);
		work.command_status = critical_command_encode_bounded(
			original.command, &work.right_command,
			ordinary_current_reservation::callback, &work.reservation, budget.live());
		if (work.reservation.refused)
			return ENOBUFS;
		need(work.command_status == critical_command_codec_result::ok, EEXIST);
		need(work.left_command == work.right_command, EEXIST);
		// Original completion(record,true), including exact revision/result length.
		// Its full DTO is a real admitted workspace member and tail stays zero.
		need(!work.record.result_code &&
		     work.record.failure_stage == critical_failure_stage::none &&
		     work.record.durable_revision == 1 &&
		     work.record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
		work.proposal.durable_revision = work.record.durable_revision;
		work.proposal.result_size = work.record.result.size();
		for (work.index = 0; work.index < work.record.result.size(); ++work.index)
			work.proposal.result_payload[work.index] = work.record.result[work.index];
		if (work.values.value.context.receipt_present)
			ordinary_current_receipt(work.values.value.context.receipt, work.record,
						 work.index);
		need(identity.matches(root) && lock.matches(root), EINVAL);
		budget.admit();
		static_assert(std::is_nothrow_move_assignable_v<flatfile_accounting_record>);
		*output = std::move(work.record);
		if (retained_record_heap)
			*retained_record_heap = work.record_heap;
		return 0;
	}
	catch (const ordinary_current_refusal &value)
	{
		return value.code;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
#endif
}

unsigned int
flatfile_accounting_native_mobile_birth_ordinary_transaction::read_current_locked_bounded(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	const critical_completion &receipt, flatfile_ordinary_native_birth_projection *output,
	flatfile_scratch_reserve_fn reserve, void *context, size_t outer_live,
	size_t *retained_projection_heap) noexcept
{
	if (!output || !identity.matches(root) || !lock.matches(root))
		return EINVAL;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||          \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)original;
	(void)receipt;
	(void)reserve;
	(void)context;
	(void)outer_live;
	(void)retained_projection_heap;
	return ENOTSUP;
#else
	try
	{
		const size_t fixed =
			sizeof(ordinary_current_projection_workspace) +
			sizeof(ordinary_current_budget<ordinary_current_projection_workspace>) +
			ordinary_current_call_frames + ordinary_current_move_frames +
			ordinary_current_library_string_move_frames +
			ordinary_current_library_vector_constructor_frames +
			ordinary_current_result_source_frames;
		if (!reserve || !reserve(ordinary_current_add(outer_live, fixed), context))
			return ENOBUFS;
		ordinary_current_projection_workspace work;
		work.reservation = { reserve, context, false };
		ordinary_current_budget<ordinary_current_projection_workspace> budget{ outer_live,
										       fixed,
										       work };
		work.status = verify_record_locked_bounded(root, identity, lock, original,
							   &work.record,
							   ordinary_current_reservation::callback,
							   &work.reservation, budget.live(),
							   &work.record_heap);
		checked(work.status);
		budget.admit();
		ordinary_current_receipt(receipt, work.record, work.index);
		ordinary_current_decode(original, work.values, budget);
		need(native_mobile_birth_cash_role_result_decode(work.record.result,
								 &work.result) &&
		     work.result.role == native_mobile_birth_cash_role::ordinary_wallet);
		work.wallet = { work.values.value.intent.admission.metadata.lineage,
				economic_account_kind::wallet, work.result.wallet_mapping_id,
				ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
		work.status = flatfile_native_mobile_wallet_storage::observe_current_locked_bounded(
			root, lock, work.values.value.intent.admission.metadata.epoch, work.wallet,
			work.values.value.image.reference, &work.current.wallet,
			ordinary_current_reservation::callback, &work.reservation, budget.live(),
			&work.wallet_heap);
		if (work.reservation.refused)
			return ENOBUFS;
		checked(work.status);
		need(!work.current.wallet.mapping.revision &&
			     work.current.wallet.mapping.last_operation.bytes ==
				     original.command.operation_id.bytes &&
			     work.current.wallet.mapping.locator.name.empty(),
		     ESTALE);
		budget.admit();
		work.image_status = quest_mobile_native_image_encode_bounded(
			work.current.wallet.native, &work.actual_image,
			ordinary_current_reservation::callback, &work.reservation, budget.live());
		if (work.reservation.refused)
			return ENOBUFS;
		need(work.image_status == player_snapshot_codec_result::ok,
		     work.image_status == player_snapshot_codec_result::allocation_failure ?
			     ENOMEM :
		     work.image_status == player_snapshot_codec_result::limit_exceeded ? E2BIG :
											 EBADMSG);
		budget.admit();
		work.image_status = quest_mobile_native_image_encode_bounded(
			work.values.value.image, &work.original_image,
			ordinary_current_reservation::callback, &work.reservation, budget.live());
		if (work.reservation.refused)
			return ENOBUFS;
		need(work.image_status == player_snapshot_codec_result::ok,
		     work.image_status == player_snapshot_codec_result::allocation_failure ?
			     ENOMEM :
		     work.image_status == player_snapshot_codec_result::limit_exceeded ? E2BIG :
											 EBADMSG);
		need(work.actual_image == work.original_image, ESTALE);
		budget.admit();
		errno = 0;
		work.custody_status =
			flatfile_native_mobile_birth_ordinary_custody_storage::read_locked_bounded(
				root, lock, original, work.record, &work.custody,
				ordinary_current_reservation::callback, &work.reservation,
				budget.live(), &work.custody_heap);
		work.storage_errno = errno;
		if (work.reservation.refused)
			return ENOBUFS;
		// This genuine storage provider explicitly reports resource/policy errno.
		// Inspect it only on its io_error result after clearing prior errno.
		if (work.custody_status == flatfile_item_repository_result::io_error &&
		    (work.storage_errno == ENOMEM || work.storage_errno == ENOBUFS ||
		     work.storage_errno == ENOTSUP))
			return work.storage_errno;
		checked(work.custody_status);
		need(work.custody.owner_revision == 1 &&
			     work.custody.rows.size() == work.values.value.image.items.size(),
		     ESTALE);
		budget.admit(ordinary_current_move_frames);
		work.current.owner_revision = work.custody.owner_revision;
		work.current.custody = std::move(work.custody.rows);
		// The one actual transferred custody allocation remains counted by the
		// same scalar after relocation; no copied/vanished heap is counted twice.
		work.status = flatfile_native_mobile_birth_ordinary_physical_storage::
			verify_catalog_namespaces_locked_bounded(
				root, identity, lock, original, &work.physical,
				ordinary_current_reservation::callback, &work.reservation,
				budget.live());
		if (work.reservation.refused)
			return ENOBUFS;
		checked(work.status);
		need(identity.matches(root) && lock.matches(root), EINVAL);
		work.retained = ordinary_current_add(work.wallet_heap, work.custody_heap);
		budget.admit(ordinary_current_move_frames);
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_ordinary_native_birth_projection>);
		*output = std::move(work.current);
		if (retained_projection_heap)
			*retained_projection_heap = work.retained;
		return 0;
	}
	catch (const ordinary_current_refusal &value)
	{
		return value.code;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
#endif
}

unsigned int flatfile_native_mobile_birth_ordinary_publication_storage::read_current_locked_bounded(
	const std::string &root, const flatfile_identity_lock &identity,
	const flatfile_authority_lock &lock, const critical_native_recovery_envelope &original,
	const critical_completion &receipt, flatfile_ordinary_native_birth_projection *output,
	flatfile_scratch_reserve_fn reserve, void *context, size_t outer_live,
	size_t *retained_projection_heap) noexcept
{
	// This genuine private birth-owner facade retains its own actual argument/
	// return carriers throughout the complete sibling proof. It acquires nothing
	// and changes no runtime selection, recovery, writer, publication or ACK gate.
	constexpr size_t frames = 9 * sizeof(void *) + sizeof(flatfile_scratch_reserve_fn) +
				  2 * sizeof(size_t) + sizeof(unsigned int) + sizeof(bool);
	if (frames > SIZE_MAX - outer_live || !reserve || !reserve(outer_live + frames, context))
		return ENOBUFS;
	return flatfile_accounting_native_mobile_birth_ordinary_transaction::
		read_current_locked_bounded(root, identity, lock, original, receipt, output,
					    reserve, context, outer_live + frames,
					    retained_projection_heap);
}
