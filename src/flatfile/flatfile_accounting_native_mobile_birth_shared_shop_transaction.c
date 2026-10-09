#include "flatfile/flatfile_accounting_native_mobile_birth_shared_shop_transaction.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include <algorithm>
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
void checked(flatfile_shopkeeper_result value)
{
	need(value == flatfile_shopkeeper_result::ok,
	     value == flatfile_shopkeeper_result::io_error  ? EIO :
	     value == flatfile_shopkeeper_result::not_found ? ENOENT :
							      EILSEQ);
}
void checked(flatfile_item_accounting_status value)
{
	need(value == flatfile_item_accounting_status::ok,
	     value == flatfile_item_accounting_status::io_error ? EIO :
	     value == flatfile_item_accounting_status::capacity ? ENOSPC :
								  EILSEQ);
}
struct original_values
{
	quest_mobile_native_image image;
	native_mobile_birth_cash_role_recipe role;
	native_mobile_birth_shared_shop_recovery_context context;
	flatfile_shopkeeper_record checkpoint;
	economic_frozen_intent intent;
};
original_values decode(const critical_native_recovery_envelope &original)
{
	need(native_mobile_birth_shared_shop_recovery_valid(original), EINVAL);
	original_values value;
	checked(native_mobile_birth_shared_shop_recovery_decode(
		original.command, original.attachment, &value.context));
	std::vector<native_mobile_birth_item_recipe> recipes;
	checked(native_mobile_birth_cash_role_command_decode(original.command, &value.image,
							     &recipes, &value.role));
	need(value.role.role == native_mobile_birth_cash_role::shared_shopkeeper &&
		     value.image.cash,
	     EINVAL);
	need(flatfile_shopkeeper_initial_checkpoint_decode(value.context.original_checkpoint,
							   &value.checkpoint));
	checked(economic_intent_decode(original.command.accounting_intent, &value.intent));
	checked(economic_intent_verify_binding(original.command, value.intent));
	need(value.intent.admission.metadata.source_event.has_value(), EINVAL);
	return value;
}
void historical_authority(const std::string &root, const flatfile_authority_lock &lock,
			  const critical_command &command, const original_values &value)
{
	flatfile_economic_control control;
	checked(flatfile_economic_control_read(root, lock, &control, nullptr));
	const auto &metadata = value.intent.admission.metadata;
	const size_t bucket = command.operation_id.bytes[0];
	need(control.lineage.bytes == metadata.lineage.bytes &&
	     (control.evidence_initialized[bucket / 8] & (1U << (bucket % 8))));
	flatfile_economic_epoch epoch;
	checked(flatfile_economic_epoch_read(root, lock, metadata.lineage, metadata.epoch, &epoch,
					     nullptr));
}
void current_authority(const std::string &root, const flatfile_authority_lock &lock,
		       const original_values &value)
{
	flatfile_economic_authority_snapshot authority;
	const auto &metadata = value.intent.admission.metadata;
	// Shared birth has no wallet/treasury/coin posting or new mapping. Empty
	// requests retain the original actual installed lineage/active epoch gate.
	checked(economic_flatfile_lock_authority(
		root, lock, metadata.lineage, metadata.epoch,
		std::span<const flatfile_economic_mapping_request>{}, &authority, nullptr));
}
void initial_policy(const native_mobile_birth_shared_shop_participant &p, bool stock)
{
	need(!p.shop_before_present && !p.shop_revision_before && p.shop_after_present &&
	     p.shop_revision_after == 1);
	if (stock)
		need(p.owner_after_present && p.owner_revision_before != UINT64_MAX &&
		     p.owner_revision_after == p.owner_revision_before + 1);
	else
		need(p.owner_after_present == p.owner_before_present &&
		     p.owner_revision_after == p.owner_revision_before);
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
	     record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
	critical_apply_result result{ replay ? critical_apply_outcome::already_applied :
					       critical_apply_outcome::applied,
				      record.durable_revision, 0 };
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
	return result;
}
bool successful(const critical_completion &receipt) noexcept
{
	return receipt.outcome == critical_apply_outcome::applied ||
	       receipt.outcome == critical_apply_outcome::already_applied;
}
bool receipt_core_equal(const critical_completion &a, const critical_completion &b) noexcept
{
	return successful(a) && successful(b) && a.operation_id.bytes == b.operation_id.bytes &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.failure_stage == b.failure_stage && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload &&
	       a.disposition == critical_completion_disposition::execution &&
	       b.disposition == critical_completion_disposition::execution;
}
}

struct flatfile_accounting_native_mobile_birth_shared_shop_transaction::implementation
{
	std::string root;
	// Address identity is only valid until first commit on the preparing callback.
	const flatfile_authority_lock *lock = nullptr;
	bool commit_called = false;
	critical_native_recovery_envelope original;
	flatfile_accounting_record record;
	std::vector<flatfile_authority_operation> operations;
	flatfile_authority_commit_outcome outcome =
		flatfile_authority_commit_outcome::not_published;
};
flatfile_accounting_native_mobile_birth_shared_shop_transaction::
	flatfile_accounting_native_mobile_birth_shared_shop_transaction(
		std::unique_ptr<implementation> value)
	: state_(std::move(value))
{
}
flatfile_accounting_native_mobile_birth_shared_shop_transaction::
	~flatfile_accounting_native_mobile_birth_shared_shop_transaction() = default;

unsigned int flatfile_accounting_native_mobile_birth_shared_shop_transaction::verify_record_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_accounting_record *output) noexcept
{
	try
	{
		need(output && !root.empty() && lock.matches(root), EINVAL);
		const auto value = decode(original);
		flatfile_accounting_record record;
		checked(flatfile_accounting_lookup(root, lock, original.command, &record, nullptr));
		historical_authority(root, lock, original.command, value);
		need(critical_command_equal(record.command, original.command) &&
		     !record.result_code && record.failure_stage == critical_failure_stage::none &&
		     record.result.size() == NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES);
		native_mobile_birth_cash_role_result result;
		need(native_mobile_birth_cash_role_result_decode(record.result, &result) &&
		     result.role == native_mobile_birth_cash_role::shared_shopkeeper);
		initial_policy(result.shared, !value.image.items.empty());
		economic_accounting_plan expected;
		checked(native_mobile_birth_cash_role_accounting_compile(original.command,
									 result.shared, &expected));
		need(expected.accounts.empty() && expected.postings.empty() &&
		     expected.children.empty());
		std::vector<uint8_t> bytes;
		checked(economic_plan_encode(expected, &bytes));
		need(bytes == record.plan &&
		     native_mobile_birth_cash_role_result_matches(original.command, result.shared,
								  expected, result));
		std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> encoded{};
		need(native_mobile_birth_cash_role_result_encode(result, &encoded) &&
		     std::equal(encoded.begin(), encoded.end(), record.result.begin()) &&
		     record.durable_revision ==
			     std::max(uint64_t{ 1 }, result.shared.owner_revision_after));
		const auto refs = references(original.command, expected);
		checked(flatfile_item_accounting_reference_verify_operation(
			root, original.command.operation_id, refs, nullptr));
		checked(flatfile_accounting_storage::verify_source_claim(root, lock, record,
									 nullptr));
		if (value.context.progress.receipt_present)
		{
			// Passive actual stored receipt counterpart; no coordinator delivery
			// or terminal-progress authority. Reuse original economic core policy.
			const auto stored = completion(record, true);
			critical_completion core{};
			core.operation_id = record.command.operation_id;
			core.outcome = stored.outcome;
			core.durable_revision = stored.durable_revision;
			core.error_code = stored.error_code;
			core.failure_stage = stored.failure_stage;
			core.result_size = stored.result_size;
			core.result_payload = stored.result_payload;
			need(receipt_core_equal(value.context.progress.receipt, core));
		}
		need(lock.matches(root), EINVAL);
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
flatfile_accounting_native_mobile_birth_shared_shop_transaction::verify_retained_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original) noexcept
{
	flatfile_accounting_record record;
	const auto error = verify_record_locked(root, lock, original, &record);
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

unsigned int flatfile_accounting_native_mobile_birth_shared_shop_transaction::read_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, const critical_completion &receipt,
	flatfile_shared_native_birth_projection *output) noexcept
{
	try
	{
		need(output && lock.matches(root), EINVAL);
		flatfile_accounting_record record;
		checked(verify_record_locked(root, lock, original, &record));
		const auto expected_receipt = completion(record, true);
		need(receipt.disposition == critical_completion_disposition::execution &&
			     critical_operation_id_equal(receipt.operation_id,
							 original.command.operation_id) &&
			     (receipt.outcome == critical_apply_outcome::applied ||
			      receipt.outcome == critical_apply_outcome::already_applied) &&
			     receipt.error_code == 0 &&
			     receipt.failure_stage == critical_failure_stage::none &&
			     receipt.durable_revision == expected_receipt.durable_revision &&
			     receipt.result_size == expected_receipt.result_size &&
			     std::equal(record.result.begin(), record.result.end(),
					receipt.result_payload.begin()) &&
			     std::all_of(receipt.result_payload.begin() + receipt.result_size,
					 receipt.result_payload.end(),
					 [](uint8_t b) { return !b; }),
		     EEXIST);
		const auto value = decode(original);
		current_authority(root, lock, value);
		native_mobile_birth_cash_role_result result;
		need(native_mobile_birth_cash_role_result_decode(record.result, &result));
		flatfile_shared_native_birth_projection current;
		quest_mobile_native_flatfile_row native;
		checked(quest_mobile_native_flatfile_read_locked(
			root, lock, value.image.reference.mobile_instance_id, &native));
		need(native.present, ESTALE);
		{
			std::vector<uint8_t> actual_image, original_image;
			need(quest_mobile_native_image_encode(native.image, &actual_image) ==
					     player_snapshot_codec_result::ok &&
				     quest_mobile_native_image_encode(value.image,
								      &original_image) ==
					     player_snapshot_codec_result::ok &&
				     actual_image == original_image,
			     ESTALE);
		}
		{
			std::vector<flatfile_shopkeeper_record> keepers;
			checked(flatfile_shopkeeper_list_locked(root, lock, &keepers, nullptr));
			const auto selected =
				std::find_if(keepers.begin(), keepers.end(), [&](const auto &k)
					     { return k.shop_id == value.checkpoint.shop_id; });
			need(selected != keepers.end(), ESTALE);
			std::vector<uint8_t> actual_checkpoint;
			need(flatfile_shopkeeper_initial_checkpoint_encode(*selected,
									   &actual_checkpoint) &&
				     actual_checkpoint == value.context.original_checkpoint,
			     ESTALE);
			current.keeper = std::move(*selected);
		}
		flatfile_shared_shop_current_custody custody;
		checked(flatfile_shared_shop_current_custody_storage::read_locked(
			root, lock, original, record.result, record.plan, &custody, nullptr));
		current.owner_present = custody.owner_present;
		current.owner_revision = custody.owner_revision;
		current.custody = std::move(custody.rows);
		need(lock.matches(root), EINVAL);
		current.native = std::move(native.image);
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_shared_native_birth_projection>);
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

unsigned int flatfile_accounting_native_mobile_birth_shared_shop_transaction::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	std::unique_ptr<flatfile_accounting_native_mobile_birth_shared_shop_transaction>
		*output) noexcept
{
	try
	{
		need(output && !root.empty() && lock.matches(root) &&
			     native_mobile_birth_shared_shop_recovery_initial(original),
		     EINVAL);
		const auto value = decode(original);
		flatfile_accounting_record existing;
		const auto found = flatfile_accounting_lookup(root, lock, original.command,
							      &existing, nullptr);
		if (found == flatfile_accounting_status::ok)
		{
			checked(verify_record_locked(root, lock, original, &existing));
			return EALREADY;
		}
		need(found == flatfile_accounting_status::not_found,
		     found == flatfile_accounting_status::io_error ? EIO : EILSEQ);
		historical_authority(root, lock, original.command, value);
		current_authority(root, lock, value);
		auto state = std::make_unique<implementation>();
		state->root = root;
		state->lock = &lock;
		state->original = original;
		flatfile_shopkeeper_initial_catalog_stage keeper;
		checked(flatfile_shopkeeper_initial_catalog_storage::prepare_locked(
			root, lock, value.context.original_checkpoint, &keeper, nullptr));
		flatfile_shared_shop_initial_custody_stage custody;
		checked(flatfile_shared_shop_initial_custody_storage::prepare_locked(
			root, lock, original, &custody, nullptr));
		initial_policy(custody.participant, !value.image.items.empty());
		need(custody.plan.accounts.empty() && custody.plan.postings.empty() &&
		     custody.plan.children.empty());
		quest_mobile_native_flatfile_row native;
		checked(quest_mobile_native_flatfile_read_locked(
			root, lock, value.image.reference.mobile_instance_id, &native));
		need(!native.present, EEXIST);
		flatfile_authority_operation native_operation;
		checked(quest_mobile_native_flatfile_prepare_locked(
			root, lock, original.command.operation_id, native, value.image,
			&native_operation));
		auto &record = state->record;
		record.command = original.command;
		checked(economic_plan_encode(custody.plan, &record.plan));
		native_mobile_birth_cash_role_result result;
		checked(native_mobile_birth_cash_role_result_build(
			original.command, custody.participant, custody.plan, &result));
		std::array<uint8_t, NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES> bytes{};
		need(native_mobile_birth_cash_role_result_encode(result, &bytes));
		record.result.assign(bytes.begin(), bytes.end());
		record.durable_revision =
			std::max(uint64_t{ 1 }, custody.participant.owner_revision_after);
		state->operations.push_back(std::move(keeper.operation));
		if (custody.has_operation)
			state->operations.push_back(std::move(custody.operation));
		state->operations.push_back(std::move(native_operation));
		const auto refs = references(original.command, custody.plan);
		checked(flatfile_item_accounting_reference_stage(
			root, lock, original.command.operation_id, refs, &state->operations,
			nullptr));
		checked(flatfile_accounting_storage::stage(root, lock, record, &state->operations,
							   nullptr));
		checked(flatfile_accounting_storage::stage_source_claim(
			root, lock, record, &state->operations, nullptr));
		need(lock.matches(root), EINVAL);
		auto prepared = std::unique_ptr<
			flatfile_accounting_native_mobile_birth_shared_shop_transaction>(
			new flatfile_accounting_native_mobile_birth_shared_shop_transaction(
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

critical_apply_result
flatfile_accounting_native_mobile_birth_shared_shop_transaction::commit_locked(
	const std::string &root, const flatfile_authority_lock &lock) noexcept
{
	critical_apply_result proposed{ critical_apply_outcome::retryable_failure, 0, EINVAL };
	try
	{
		need(state_ != nullptr, EINVAL);
		proposed = completion(state_->record, false);
		need(!state_->commit_called && state_->root == root && state_->lock == &lock &&
			     lock.matches(root),
		     EINVAL);
		if (state_->outcome == flatfile_authority_commit_outcome::not_published)
		{
			// Clear stack-lock identity BEFORE the first actual commit cut so
			// every success/exception/refusal leaves no cross-callback pointer.
			state_->commit_called = true;
			state_->lock = nullptr;
			const auto committed = flatfile_accounting_storage::commit_with_outcome(
				root, lock, state_->operations, nullptr, &state_->outcome);
			need(committed == flatfile_authority_transaction_result::ok, EIO);
		}
		// Once publication is possible, the exact original proposal is retained.
		// Original lookup recovers the authority journal before receipt proof.
		// A missing/corrupt proof stays ambiguous; never reapply those images.
		flatfile_accounting_record retained;
		checked(verify_record_locked(root, lock, state_->original, &retained));
		need(retained.plan == state_->record.plan &&
		     retained.result == state_->record.result &&
		     retained.durable_revision == state_->record.durable_revision);
		critical_completion receipt{};
		receipt.operation_id = state_->original.command.operation_id;
		receipt.outcome = proposed.outcome;
		receipt.durable_revision = proposed.durable_revision;
		receipt.result_size = proposed.result_size;
		receipt.result_payload = proposed.result_payload;
		flatfile_shared_native_birth_projection current;
		checked(read_current_locked(root, lock, state_->original, receipt, &current));
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

bool flatfile_accounting_native_mobile_birth_shared_shop_transaction::publication_possible()
	const noexcept
{
	return state_ && state_->outcome != flatfile_authority_commit_outcome::not_published;
}

bool flatfile_accounting_native_mobile_birth_shared_shop_transaction::retained_bytes(
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
flatfile_accounting_native_mobile_birth_shared_shop_transaction::reconcile_locked(
	const std::string &root, const flatfile_authority_lock &lock) noexcept
{
	critical_apply_result proposed{ critical_apply_outcome::retryable_failure, 0, EINVAL };
	try
	{
		need(state_ != nullptr, EINVAL);
		proposed = completion(state_->record, true);
		need(publication_possible() && state_->root == root && lock.matches(root), EINVAL);
		// The genuine fresh recovered same-root lock protects actual original
		// retained proof and CURRENT cut. No old stack lock or proposal writes.
		flatfile_accounting_record retained;
		checked(verify_record_locked(root, lock, state_->original, &retained));
		need(retained.plan == state_->record.plan &&
		     retained.result == state_->record.result &&
		     retained.durable_revision == state_->record.durable_revision);
		critical_completion receipt{};
		receipt.operation_id = state_->original.command.operation_id;
		receipt.outcome = proposed.outcome;
		receipt.durable_revision = proposed.durable_revision;
		receipt.result_size = proposed.result_size;
		receipt.result_payload = proposed.result_payload;
		flatfile_shared_native_birth_projection current;
		checked(read_current_locked(root, lock, state_->original, receipt, &current));
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

unsigned int flatfile_shared_native_birth_publication_storage::read_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, const critical_completion &receipt,
	flatfile_shared_native_birth_publication_projection *output) noexcept
{
	try
	{
		need(output && lock.matches(root), EINVAL);
		flatfile_shared_native_birth_projection current;
		checked(flatfile_accounting_native_mobile_birth_shared_shop_transaction::
				read_current_locked(root, lock, original, receipt, &current));
		flatfile_shared_native_birth_publication_projection candidate;
		candidate.owner_present = current.owner_present;
		candidate.owner_revision = current.owner_revision;
		candidate.custody.reserve(current.custody.size());
		const item_owner_identity keeper{ item_owner_type::shopkeeper,
						  item_shopkeeper_owner_id(current.keeper.shop_id),
						  0 };
		need(current.native.items.size() == current.custody.size() &&
			     (current.custody.empty() || current.owner_present),
		     ESTALE);
		for (const auto &entry : current.custody)
		{
			need(item_owner_identity_equal(entry.owner, keeper) &&
				     entry.state == item_custody_state::active,
			     ESTALE);
			candidate.custody.push_back({ entry.item_uid, entry.root_item_uid,
						      entry.parent_item_uid, entry.owner,
						      entry.item_revision, current.owner_revision,
						      entry.vnum, entry.state });
		}
		need(lock.matches(root), EINVAL);
		candidate.native = std::move(current.native);
		candidate.keeper = std::move(current.keeper);
		static_assert(std::is_nothrow_move_assignable_v<
			      flatfile_shared_native_birth_publication_projection>);
		*output = std::move(candidate);
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
