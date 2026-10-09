#include "flatfile/flatfile_accounting_zone_reset_item_transaction.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_season_state.h"
#include "flatfile/flatfile_store.h"
#include "economy/zone_reset_item_accounting.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include <algorithm>
#include <array>
#include <cerrno>
#include <new>
#include <type_traits>
#include <unordered_set>
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
void checked(economic_accounting_error error)
{
	need(error == economic_accounting_error::ok,
	     error == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}
void checked(flatfile_accounting_status s)
{
	need(s == flatfile_accounting_status::ok,
	     s == flatfile_accounting_status::io_error	? EIO :
	     s == flatfile_accounting_status::capacity	? ENOSPC :
	     s == flatfile_accounting_status::not_found ? ENOENT :
	     s == flatfile_accounting_status::conflict ||
			     s == flatfile_accounting_status::already_exists ?
							  EEXIST :
							  EILSEQ);
}
void checked(flatfile_item_repository_result s)
{
	need(s == flatfile_item_repository_result::ok,
	     s == flatfile_item_repository_result::io_error  ? EIO :
	     s == flatfile_item_repository_result::not_found ? ENOENT :
							       EILSEQ);
}
void checked(flatfile_world_item_result s)
{
	need(s == flatfile_world_item_result::ok,
	     s == flatfile_world_item_result::io_error	? EIO :
	     s == flatfile_world_item_result::not_found ? ENOENT :
	     s == flatfile_world_item_result::conflict	? ESTALE :
							  EILSEQ);
}
void checked(flatfile_item_accounting_status s)
{
	need(s == flatfile_item_accounting_status::ok,
	     s == flatfile_item_accounting_status::io_error ? EIO :
	     s == flatfile_item_accounting_status::capacity ? ENOSPC :
							      EILSEQ);
}
struct original_values
{
	zone_reset_item_image image;
	zone_reset_item_recovery_context context;
	economic_frozen_intent intent;
};
original_values decode(const critical_native_recovery_envelope &original)
{
	need(zone_reset_item_recovery_valid(original), EINVAL);
	original_values value;
	checked(zone_reset_item_command_decode(original.command, &value.image));
	checked(zone_reset_item_recovery_decode(original.command, original.attachment,
						&value.context));
	checked(economic_intent_decode(original.command.accounting_intent, &value.intent));
	checked(economic_intent_verify_binding(original.command, value.intent));
	need(value.intent.admission.metadata.source_event && value.image.placement &&
		     !value.image.placement->fall_selected,
	     ENOTSUP);
	for (const auto &item : value.image.items)
		need(item.type >= ITEM_LOWEST && item.type <= ITEM_LAST &&
			     item.type != ITEM_CORPSE && !(item.extra_flags & ITEM_ARTIFACT),
		     ENOTSUP);
	return value;
}
item_transfer_result expected_result(const zone_reset_item_image &image)
{
	need(!image.items.empty() && image.items.size() <= UINT16_MAX &&
		     image.expected_room_revision != UINT64_MAX,
	     EINVAL);
	item_transfer_result result{};
	result.root_item_uid = image.items.front().object_uid;
	result.item_count = static_cast<uint16_t>(image.items.size());
	result.to_owner_revision = image.expected_room_revision + 1;
	result.max_item_revision = 1;
	return result;
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
#ifndef __NO_MYSQL__
	(void)root;
	(void)lock;
	(void)value;
	need(false, ENOTSUP);
#else
	const char *configured = persistence_mode_flatfile_root();
	need(persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
		     !persistence_mode_requires_mysql() && configured && root == configured &&
		     lock.matches(root),
	     EACCES);
	flatfile_economic_authority_snapshot authority;
	const auto &metadata = value.intent.admission.metadata;
	checked(economic_flatfile_lock_authority(
		root, lock, metadata.lineage, metadata.epoch,
		std::span<const flatfile_economic_mapping_request>{}, &authority, nullptr));
	flatfile_season_state season;
	const auto read = flatfile_season_state_read_locked(root, lock, &season);
	need(read == flatfile_season_state_result::ok,
	     read == flatfile_season_state_result::io_error  ? EIO :
	     read == flatfile_season_state_result::not_found ? ENODATA :
							       ESTALE);
	need(season.status == flatfile_season_status::active &&
		     season.epoch == value.image.season_epoch,
	     ESTALE);
#endif
}
std::vector<economic_accounting_item_reference> references(const critical_command &command,
							   const economic_accounting_plan &plan)
{
	std::vector<economic_accounting_item_reference> result;
	result.reserve(plan.item_events.size());
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
		result.push_back(ref);
	}
	return result;
}
critical_apply_result completion(const flatfile_accounting_record &record, bool replay)
{
	need(!record.result_code && record.failure_stage == critical_failure_stage::none &&
	     record.result.size() == ITEM_TRANSFER_RESULT_BYTES && record.durable_revision);
	critical_apply_result result{ replay ? critical_apply_outcome::already_applied :
					       critical_apply_outcome::applied,
				      record.durable_revision, 0 };
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
	return result;
}
critical_completion passive_core(const flatfile_accounting_record &record)
{
	const auto result = completion(record, true);
	critical_completion core{};
	core.operation_id = record.command.operation_id;
	core.outcome = result.outcome;
	core.durable_revision = result.durable_revision;
	core.error_code = result.error_code;
	core.failure_stage = result.failure_stage;
	core.result_size = result.result_size;
	core.result_payload = result.result_payload;
	return core;
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
std::string origin_filename(uint64_t uid)
{
	need(uid && uid != UINT64_MAX, EINVAL);
	return "room_reset_birth_" + std::to_string(uid) + ".zro";
}
unsigned int read_origin(const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
			 zone_reset_item_retained_origin *output)
{
	need(output && lock.matches(root), EINVAL);
	std::vector<uint8_t> bytes;
	const auto read = flatfile_read(root + "/domains", origin_filename(uid),
					ZONE_RESET_ITEM_ORIGIN_MAX_BYTES, &bytes, nullptr);
	if (read != flatfile_read_result::ok)
		return read == flatfile_read_result::not_found ? ENOENT :
		       read == flatfile_read_result::io_error  ? EIO :
								 EILSEQ;
	zone_reset_item_retained_origin observed;
	checked(zone_reset_item_origin_decode(bytes, &observed));
	zone_reset_item_image image;
	checked(zone_reset_item_command_decode(observed.original, &image));
	need(image.items.front().object_uid == uid);
	need(lock.matches(root), EINVAL);
	observed.present = true;
	*output = std::move(observed);
	return 0;
}
void verify_origin(const std::string &root, const flatfile_authority_lock &lock,
		   const zone_reset_item_image &image, const flatfile_accounting_record &record)
{
	zone_reset_item_retained_origin stored;
	checked(read_origin(root, lock, image.items.front().object_uid, &stored));
	need(stored.present && critical_command_equal(stored.original, record.command) &&
	     std::equal(stored.result.begin(), stored.result.end(), record.result.begin()));
}
void verify_plan(const critical_command &command, const original_values &value,
		 economic_accounting_plan *plan)
{
	checked(zone_reset_item_accounting_compile(command, plan));
	need(plan->children.empty() && plan->item_events.size() == value.image.items.size());
	size_t piles = 0, issuance = 0;
	for (const auto &effect : plan->accounts)
	{
		if (effect.key.kind == economic_account_kind::issuance)
		{
			++issuance;
			need(effect.key.authority_id == 1 && !effect.key.context_id &&
			     effect.key.lineage.bytes ==
				     value.intent.admission.metadata.lineage.bytes &&
			     effect.before == economic_coin_vector{} &&
			     effect.after == economic_coin_vector{} && !effect.before_revision &&
			     !effect.after_revision);
			continue;
		}
		need(effect.key.kind == economic_account_kind::pile && !effect.key.context_id);
		const auto coin = std::find_if(value.image.coins.begin(), value.image.coins.end(),
					       [&](const auto &c)
					       { return c.item_uid == effect.key.authority_id; });
		need(coin != value.image.coins.end() &&
		     effect.key.lineage.bytes == value.intent.admission.metadata.lineage.bytes &&
		     effect.before == economic_coin_vector{} &&
		     effect.after == coin->denominations && effect.before_revision == 0 &&
		     effect.after_revision == 1);
		++piles;
	}
	need(piles == value.image.coins.size() && issuance <= 1);
}
void verify_piles(const std::string &root, const flatfile_authority_lock &lock,
		  const critical_command &command, const original_values &value,
		  const economic_accounting_plan &plan)
{
	for (const auto &effect : plan.accounts)
	{
		if (effect.key.kind != economic_account_kind::pile)
			continue;
		flatfile_accounting_pile_state head;
		checked(flatfile_accounting_pile_state_read(root, lock, effect.key.authority_id,
							    &head, nullptr));
		need(economic_account_key_equal(head.account, effect.key) &&
			     head.epoch.bytes == value.intent.admission.metadata.epoch.bytes &&
			     head.operation_id.bytes == command.operation_id.bytes &&
			     !head.retired && head.item_revision == 1 &&
			     head.balance == effect.after,
		     ESTALE);
	}
}
} // namespace

struct flatfile_accounting_zone_reset_item_transaction::implementation
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
flatfile_accounting_zone_reset_item_transaction::flatfile_accounting_zone_reset_item_transaction(
	std::unique_ptr<implementation> value)
	: state_(std::move(value))
{
}
flatfile_accounting_zone_reset_item_transaction::~flatfile_accounting_zone_reset_item_transaction() =
	default;

unsigned int flatfile_accounting_zone_reset_item_transaction::verify_record_locked(
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
		need(critical_command_equal(record.command, original.command));
		const auto core = passive_core(record);
		economic_accounting_plan plan;
		verify_plan(original.command, value, &plan);
		std::vector<uint8_t> encoded_plan;
		checked(economic_plan_encode(plan, &encoded_plan));
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> result{};
		need(item_transfer_command_encode_result(expected_result(value.image), &result) &&
		     encoded_plan == record.plan &&
		     std::equal(result.begin(), result.end(), record.result.begin()) &&
		     record.durable_revision == value.image.expected_room_revision + 1);
		checked(flatfile_accounting_storage::verify_source_claim(root, lock, record,
									 nullptr));
		const auto refs = references(original.command, plan);
		checked(flatfile_item_accounting_reference_verify_operation(
			root, original.command.operation_id, refs, nullptr));
		verify_origin(root, lock, value.image, record);
		if (value.context.receipt_present)
			need(receipt_core_equal(value.context.receipt, core));
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
critical_apply_result flatfile_accounting_zone_reset_item_transaction::verify_retained_locked(
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
unsigned int flatfile_accounting_zone_reset_item_transaction::read_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, const critical_completion &receipt,
	flatfile_zone_reset_item_projection *output) noexcept
{
	try
	{
		need(output && lock.matches(root), EINVAL);
		flatfile_accounting_record record;
		checked(verify_record_locked(root, lock, original, &record));
		need(receipt_core_equal(receipt, passive_core(record)), EEXIST);
		const auto value = decode(original);
		current_authority(root, lock, value);
		economic_accounting_plan plan;
		verify_plan(original.command, value, &plan);
		std::vector<flatfile_corpse_record> corpses;
		std::vector<flatfile_room_item_record> rooms;
		std::vector<flatfile_saved_world_item_record> saved;
		checked(flatfile_world_item_recovery_list_all_locked(root, lock, &corpses, &rooms,
								     &saved, nullptr));
		const auto selected =
			std::find_if(rooms.begin(), rooms.end(), [&](const auto &room)
				     { return room.room_vnum == value.image.room_vnum; });
		need(selected != rooms.end() &&
			     selected->revision == value.image.expected_room_revision + 1,
		     ESTALE);
		flatfile_zone_reset_item_projection current;
		checked(flatfile_room_reset_current_custody_storage::read_locked(
			root, lock, original, record.result, record.plan, *selected,
			&current.custody));
		need(current.custody.size() == value.image.items.size(), ESTALE);
		verify_piles(root, lock, original.command, value, plan);
		need(lock.matches(root), EINVAL);
		current.room = std::move(*selected);
		static_assert(
			std::is_nothrow_move_assignable_v<flatfile_zone_reset_item_projection>);
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
unsigned int flatfile_accounting_zone_reset_item_transaction::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	std::unique_ptr<flatfile_accounting_zone_reset_item_transaction> *output) noexcept
{
	try
	{
		need(output && !root.empty() && lock.matches(root) &&
			     zone_reset_item_recovery_initial(original),
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
		flatfile_initial_room_reset_world_stage world;
		checked(flatfile_initial_room_reset_world_storage::prepare_locked(
			root, lock, original, &world));
		flatfile_initial_room_reset_custody_stage custody;
		checked(flatfile_initial_room_reset_custody_storage::prepare_locked(
			root, lock, original, world, &custody));
		need(critical_command_equal(world.original_command, original.command) &&
		     world.room_revision_before == value.image.expected_room_revision &&
		     world.room_revision_after == value.image.expected_room_revision + 1 &&
		     custody.owner_revision_before == world.room_revision_before &&
		     custody.owner_revision_after == world.room_revision_after);
		economic_accounting_plan plan;
		verify_plan(original.command, value, &plan);
		auto &record = state->record;
		record.command = original.command;
		checked(economic_plan_encode(plan, &record.plan));
		std::vector<uint8_t> actual_plan;
		checked(economic_plan_encode(custody.plan, &actual_plan));
		need(actual_plan == record.plan);
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> result{}, actual_result{};
		need(item_transfer_command_encode_result(expected_result(value.image), &result) &&
		     item_transfer_command_encode_result(custody.result, &actual_result) &&
		     result == actual_result);
		record.result.assign(result.begin(), result.end());
		record.durable_revision = world.room_revision_after;
		zone_reset_item_retained_origin existing_origin;
		const auto origin_status = read_origin(
			root, lock, value.image.items.front().object_uid, &existing_origin);
		need(origin_status == ENOENT, origin_status ? origin_status : EEXIST);
		flatfile_authority_operation origin;
		origin.store = flatfile_authority_store::domains;
		origin.kind = flatfile_authority_operation_kind::write;
		origin.filename = origin_filename(value.image.items.front().object_uid);
		checked(zone_reset_item_origin_encode(original.command, record.result,
						      &origin.bytes));
		state->operations.push_back(std::move(world.operation));
		state->operations.push_back(std::move(custody.operation));
		state->operations.push_back(std::move(origin));
		for (const auto &effect : plan.accounts)
			if (effect.key.kind == economic_account_kind::pile)
				checked(flatfile_accounting_pile_state_stage(
					root, lock, effect, value.intent.admission.metadata.epoch,
					original.command.operation_id, false, &state->operations,
					nullptr));
		const auto refs = references(original.command, plan);
		checked(flatfile_item_accounting_reference_stage(
			root, lock, original.command.operation_id, refs, &state->operations,
			nullptr));
		checked(flatfile_accounting_storage::stage(root, lock, record, &state->operations,
							   nullptr));
		checked(flatfile_accounting_storage::stage_source_claim(
			root, lock, record, &state->operations, nullptr));
		need(lock.matches(root), EINVAL);
		auto prepared = std::unique_ptr<flatfile_accounting_zone_reset_item_transaction>(
			new flatfile_accounting_zone_reset_item_transaction(std::move(state)));
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

critical_apply_result flatfile_accounting_zone_reset_item_transaction::commit_locked(
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
		flatfile_zone_reset_item_projection current;
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

bool flatfile_accounting_zone_reset_item_transaction::publication_possible() const noexcept
{
	return state_ && state_->outcome != flatfile_authority_commit_outcome::not_published;
}

bool flatfile_accounting_zone_reset_item_transaction::retained_bytes(size_t *output) const noexcept
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

critical_apply_result flatfile_accounting_zone_reset_item_transaction::reconcile_locked(
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
		flatfile_zone_reset_item_projection current;
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

unsigned int flatfile_accounting_zone_reset_item_transaction::read_origin_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	zone_reset_item_retained_origin *output) noexcept
{
	try
	{
		return read_origin(root, lock, uid, output);
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
unsigned int flatfile_accounting_zone_reset_item_transaction::observe_initial_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original) noexcept
{
	try
	{
		need(!root.empty() && lock.matches(root) &&
			     zone_reset_item_recovery_initial(original),
		     EINVAL);
		const auto value = decode(original);
		historical_authority(root, lock, original.command, value);
		current_authority(root, lock, value);
		flatfile_accounting_record existing;
		const auto receipt = flatfile_accounting_lookup(root, lock, original.command,
								&existing, nullptr);
		need(receipt == flatfile_accounting_status::not_found,
		     receipt == flatfile_accounting_status::ok	     ? EEXIST :
		     receipt == flatfile_accounting_status::io_error ? EIO :
								       EILSEQ);
		flatfile_initial_room_reset_world_stage world;
		checked(flatfile_initial_room_reset_world_storage::prepare_locked(
			root, lock, original, &world));
		flatfile_initial_room_reset_custody_stage custody;
		checked(flatfile_initial_room_reset_custody_storage::prepare_locked(
			root, lock, original, world, &custody));
		need(critical_command_equal(world.original_command, original.command) &&
		     world.room_revision_before == value.image.expected_room_revision &&
		     world.room_revision_after == value.image.expected_room_revision + 1 &&
		     custody.owner_revision_before == world.room_revision_before &&
		     custody.owner_revision_after == world.room_revision_after);
		economic_accounting_plan expected;
		verify_plan(original.command, value, &expected);
		std::vector<uint8_t> expected_plan, actual_plan;
		checked(economic_plan_encode(expected, &expected_plan));
		checked(economic_plan_encode(custody.plan, &actual_plan));
		need(expected_plan == actual_plan);
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> expected_typed{}, actual_typed{};
		need(item_transfer_command_encode_result(expected_result(value.image),
							 &expected_typed) &&
		     item_transfer_command_encode_result(custody.result, &actual_typed) &&
		     expected_typed == actual_typed);
		zone_reset_item_retained_origin origin;
		const auto prior_origin =
			read_origin(root, lock, value.image.items.front().object_uid, &origin);
		need(prior_origin == ENOENT, prior_origin ? prior_origin : EEXIST);
		for (const auto &effect : expected.accounts)
			if (effect.key.kind == economic_account_kind::pile)
			{
				flatfile_accounting_pile_state pile;
				const auto prior = flatfile_accounting_pile_state_read(
					root, lock, effect.key.authority_id, &pile, nullptr);
				need(prior == flatfile_accounting_status::not_found,
				     prior == flatfile_accounting_status::ok	   ? EEXIST :
				     prior == flatfile_accounting_status::io_error ? EIO :
										     EILSEQ);
			}
		need(lock.matches(root), EINVAL);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
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
unsigned int flatfile_zone_reset_item_publication_storage::observe_initial_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original) noexcept
{
	return flatfile_accounting_zone_reset_item_transaction::observe_initial_locked(root, lock,
										       original);
}
unsigned int flatfile_zone_reset_item_publication_storage::read_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, const critical_completion &receipt,
	flatfile_zone_reset_item_projection *output) noexcept
{
	return flatfile_accounting_zone_reset_item_transaction::read_current_locked(
		root, lock, original, receipt, output);
}
unsigned int flatfile_zone_reset_item_publication_storage::read_origin_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	zone_reset_item_retained_origin *output) noexcept
{
	return flatfile_accounting_zone_reset_item_transaction::read_origin_locked(root, lock, uid,
										   output);
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool observation_storage_add(size_t &total, size_t request) noexcept
{
	if (request > SIZE_MAX - total)
		return false;
	total += request;
	return true;
}
bool observation_storage_rows(size_t &total, size_t count, size_t width) noexcept
{
	return (!width || count <= SIZE_MAX / width) &&
	       observation_storage_add(total, count * width);
}
bool observation_image_heap(const zone_reset_item_image &image, size_t *output) noexcept
{
	if (!output)
		return false;
	size_t bytes = 0;
	if (!observation_storage_rows(bytes, image.items.capacity(),
				      sizeof(player_item_snapshot)) ||
	    !observation_storage_rows(bytes, image.recipes.capacity(),
				      sizeof(native_mobile_birth_item_recipe)) ||
	    !observation_storage_rows(bytes, image.coins.capacity(),
				      sizeof(zone_reset_coin_output)))
		return false;
	const auto text = [&](const std::string &value) noexcept
	{
		return value.capacity() <= 15 ||
		       (value.capacity() != SIZE_MAX &&
			observation_storage_add(bytes, value.capacity() + 1));
	};
	for (const auto &item : image.items)
	{
		if (!text(item.name) || !text(item.short_description) || !text(item.description) ||
		    !text(item.action_description) ||
		    !observation_storage_rows(bytes, item.dynamic_affects.capacity(),
					      sizeof(player_item_dynamic_affect_snapshot)) ||
		    !observation_storage_rows(bytes, item.extra_descriptions.capacity(),
					      sizeof(player_item_extra_description_snapshot)))
			return false;
		for (const auto &extra : item.extra_descriptions)
			if (!text(extra.keyword) || !text(extra.description) ||
			    !observation_storage_rows(bytes, extra.spell_ids.capacity(),
						      sizeof(int32_t)))
				return false;
	}
	for (const auto &recipe : image.recipes)
		if (!observation_storage_rows(bytes, recipe.libraries.capacity(),
					      sizeof(native_mobile_birth_library_recipe)))
			return false;
	*output = bytes;
	return true;
}

// This wrapper owns a distinct profile through the unchanged original encoder.
// Old output storage and input plan capacities stay in caller outer_live.
void observation_plan_encode_bounded(const economic_accounting_plan &plan,
				     std::vector<uint8_t> *output,
				     flatfile_scratch_reserve_fn reserve, void *context,
				     size_t outer_live)
{
	need(output && reserve, EINVAL);
	size_t base = outer_live, scan_peak = 0;
	need(observation_storage_add(base, sizeof(economic_accounting_plan_allocation_profile)),
	     ENOBUFS);
	scan_peak = base;
	need(observation_storage_add(scan_peak,
				     economic_plan_allocation_preflight_working_bytes()) &&
		     reserve(scan_peak, context),
	     ENOBUFS);
	economic_accounting_plan_allocation_profile profile;
	checked(economic_plan_allocation_preflight(plan, &profile));
	need(profile.storage_policy_supported, ENOTSUP);
	need(observation_storage_add(base, profile.encode_working_bytes) && reserve(base, context),
	     ENOBUFS);
	checked(economic_plan_encode(plan, output));
}

void observation_command_equal_bounded(const critical_command &left, const critical_command &right,
				       flatfile_scratch_reserve_fn reserve, void *context,
				       size_t outer_live)
{
	need(reserve, EINVAL);
	size_t base = outer_live;
	need(observation_storage_add(base, 2 * sizeof(std::vector<uint8_t>)) &&
		     reserve(base, context),
	     ENOBUFS);
	std::vector<uint8_t> a, b;
	size_t working = 0, peak = base;
	need(critical_command_encoder_working_bytes(left, &working) ==
	     critical_command_codec_result::ok);
	need(observation_storage_add(peak, working), ENOBUFS);
	need(critical_command_encode_bounded(left, &a, reserve, context, base) ==
	     critical_command_codec_result::ok);
	need(observation_storage_add(base, a.capacity()), ENOBUFS);
	need(critical_command_encoder_working_bytes(right, &working) ==
	     critical_command_codec_result::ok);
	peak = base;
	need(observation_storage_add(peak, working), ENOBUFS);
	need(critical_command_encode_bounded(right, &b, reserve, context, base) ==
	     critical_command_codec_result::ok);
	need(a == b);
}

// Byte-identical origin filename and domain path constructed without hidden
// to_string/operator+ reallocations. Both fresh strings and the decimal buffer
// are prospective caller-retained helpers, not an origin existence proof.
void observation_origin_paths_bounded(const std::string &root, uint64_t uid,
				      std::string *directory_out, std::string *filename_out,
				      flatfile_scratch_reserve_fn reserve, void *context,
				      size_t outer_live)
{
	need(uid && uid != UINT64_MAX && directory_out && filename_out && reserve, EINVAL);
	size_t digits = 1;
	for (uint64_t remaining = uid; remaining >= 10; remaining /= 10)
		++digits;
	size_t directory_size = root.size(), filename_size = sizeof("room_reset_birth_") - 1;
	need(observation_storage_add(directory_size, sizeof("/domains") - 1) &&
		     observation_storage_add(filename_size, digits) &&
		     observation_storage_add(filename_size, sizeof(".zro") - 1),
	     ENOBUFS);
	size_t live = outer_live;
	need(observation_storage_add(live,
				     2 * sizeof(std::string) + sizeof(std::array<char, 20>)) &&
		     (directory_size <= 15 ||
		      (directory_size != SIZE_MAX &&
		       observation_storage_add(live, directory_size + 1))) &&
		     (filename_size <= 15 || (filename_size != SIZE_MAX &&
					      observation_storage_add(live, filename_size + 1))) &&
		     reserve(live, context),
	     ENOBUFS);
	std::array<char, 20> decimal{};
	uint64_t remaining = uid;
	for (size_t index = digits; index; --index)
	{
		decimal[index - 1] = static_cast<char>('0' + remaining % 10);
		remaining /= 10;
	}
	std::string directory(directory_size, '\0'), filename(filename_size, '\0');
	std::copy(root.begin(), root.end(), directory.begin());
	std::copy_n("/domains", sizeof("/domains") - 1, directory.begin() + root.size());
	std::copy_n("room_reset_birth_", sizeof("room_reset_birth_") - 1, filename.begin());
	std::copy_n(decimal.begin(), digits, filename.begin() + sizeof("room_reset_birth_") - 1);
	std::copy_n(".zro", sizeof(".zro") - 1,
		    filename.begin() + sizeof("room_reset_birth_") - 1 + digits);
	directory_out->swap(directory);
	filename_out->swap(filename);
}

void observation_verify_plan_bounded(const critical_command &command, const original_values &value,
				     economic_accounting_plan *plan,
				     flatfile_scratch_reserve_fn reserve, void *context,
				     size_t outer_live, size_t *retained_plan_heap)
{
	checked(zone_reset_item_accounting_compile_bounded(command, plan, reserve, context,
							   outer_live, retained_plan_heap));
	need(retained_plan_heap, EINVAL);
	size_t verify_live = outer_live;
	need(observation_storage_add(verify_live, *retained_plan_heap) &&
		     observation_storage_add(verify_live, 2 * sizeof(economic_coin_vector)) &&
		     reserve(verify_live, context),
	     ENOBUFS);
	need(plan->children.empty() && plan->item_events.size() == value.image.items.size());
	size_t piles = 0, issuance = 0;
	for (const auto &effect : plan->accounts)
	{
		if (effect.key.kind == economic_account_kind::issuance)
		{
			++issuance;
			need(effect.key.authority_id == 1 && !effect.key.context_id &&
			     effect.key.lineage.bytes ==
				     value.intent.admission.metadata.lineage.bytes &&
			     effect.before == economic_coin_vector{} &&
			     effect.after == economic_coin_vector{} && !effect.before_revision &&
			     !effect.after_revision);
			continue;
		}
		need(effect.key.kind == economic_account_kind::pile && !effect.key.context_id);
		const auto coin = std::find_if(value.image.coins.begin(), value.image.coins.end(),
					       [&](const auto &c)
					       { return c.item_uid == effect.key.authority_id; });
		need(coin != value.image.coins.end() &&
		     effect.key.lineage.bytes == value.intent.admission.metadata.lineage.bytes &&
		     effect.before == economic_coin_vector{} &&
		     effect.after == coin->denominations && effect.before_revision == 0 &&
		     effect.after_revision == 1);
		++piles;
	}
	need(piles == value.image.coins.size() && issuance <= 1);
}

void observation_decode_bounded(const critical_native_recovery_envelope &original,
				original_values *output, flatfile_scratch_reserve_fn reserve,
				void *context, size_t outer_live, size_t *retained_heap)
{
	need(output && reserve, EINVAL);
	size_t base = outer_live;
	need(observation_storage_add(base, sizeof(original_values)) && reserve(base, context),
	     ENOBUFS);
	original_values candidate;
	// This is the original repeated valid predicate, not a replacement for the
	// independently executed initial predicate at the public observer boundary.
	need(zone_reset_item_recovery_valid_bounded(original, reserve, context, base), EINVAL);
	checked(zone_reset_item_command_decode_bounded(original.command, &candidate.image, reserve,
						       context, base));
	size_t image_heap = 0, context_heap = 0, live = base;
	need(observation_image_heap(candidate.image, &image_heap) &&
		     observation_storage_add(live, image_heap),
	     ENOBUFS);
	// Recovery's existing bounded decoder takes its actual wire span by value.
	// It is constructed at this call, so its object must be in caller allowance.
	size_t recovery_live = live;
	need(observation_storage_add(recovery_live, sizeof(std::span<const uint8_t>)) &&
		     reserve(recovery_live, context),
	     ENOBUFS);
	checked(zone_reset_item_recovery_decode_bounded(original.command, original.attachment,
							&candidate.context, reserve, context,
							recovery_live, &context_heap));
	need(observation_storage_add(live, context_heap) &&
		     observation_storage_add(live, sizeof(std::span<const uint8_t>)) &&
		     reserve(live, context),
	     ENOBUFS);
	const std::span<const uint8_t> intent_wire(original.command.accounting_intent);
	checked(economic_intent_decode_bounded(intent_wire, &candidate.intent, reserve, context,
					       live));
	need(observation_storage_add(live, candidate.intent.admission.facts.capacity()), ENOBUFS);
	checked(economic_intent_verify_binding_bounded(original.command, candidate.intent, reserve,
						       context, live));
	need(candidate.intent.admission.metadata.source_event && candidate.image.placement &&
		     !candidate.image.placement->fall_selected,
	     ENOTSUP);
	for (const auto &item : candidate.image.items)
		need(item.type >= ITEM_LOWEST && item.type <= ITEM_LAST &&
			     item.type != ITEM_CORPSE && !(item.extra_flags & ITEM_ARTIFACT),
		     ENOTSUP);
	size_t heap = image_heap;
	need(observation_storage_add(heap, context_heap) &&
		     observation_storage_add(heap, candidate.intent.admission.facts.capacity()),
	     ENOBUFS);
	static_assert(std::is_nothrow_move_assignable_v<original_values>);
	*output = std::move(candidate);
	if (retained_heap)
		*retained_heap = heap;
}

void observation_historical_authority_bounded(const std::string &root,
					      const flatfile_authority_lock &lock,
					      const critical_command &command,
					      const original_values &value,
					      flatfile_scratch_reserve_fn reserve, void *context,
					      size_t outer_live)
{
	size_t live = outer_live;
	need(observation_storage_add(live, sizeof(flatfile_economic_control) +
						   sizeof(flatfile_economic_epoch)) &&
		     reserve(live, context),
	     ENOBUFS);
	flatfile_economic_control control;
	flatfile_economic_epoch epoch;
	checked(flatfile_economic_control_read_bounded(root, lock, &control, reserve, context,
						       live));
	const auto &metadata = value.intent.admission.metadata;
	const size_t bucket = command.operation_id.bytes[0];
	need(control.lineage.bytes == metadata.lineage.bytes &&
	     (control.evidence_initialized[bucket / 8] & (1U << (bucket % 8))));
	checked(flatfile_economic_epoch_read_bounded(root, lock, metadata.lineage, metadata.epoch,
						     &epoch, reserve, context, live));
}

void observation_current_authority_bounded(const std::string &root,
					   const flatfile_authority_lock &lock,
					   const original_values &value,
					   flatfile_scratch_reserve_fn reserve, void *context,
					   size_t outer_live)
{
#ifndef __NO_MYSQL__
	(void)root;
	(void)lock;
	(void)value;
	(void)reserve;
	(void)context;
	(void)outer_live;
	need(false, ENOTSUP);
#else
	const char *configured = persistence_mode_flatfile_root();
	need(persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
		     !persistence_mode_requires_mysql() && configured && root == configured &&
		     lock.matches(root),
	     EACCES);
	size_t live = outer_live;
	need(observation_storage_add(
		     live, sizeof(flatfile_economic_authority_snapshot) +
				   sizeof(flatfile_season_state) +
				   sizeof(std::span<const flatfile_economic_mapping_request>)) &&
		     reserve(live, context),
	     ENOBUFS);
	flatfile_economic_authority_snapshot authority;
	flatfile_season_state season;
	const std::span<const flatfile_economic_mapping_request> requests;
	const auto &metadata = value.intent.admission.metadata;
	size_t authority_heap = 0;
	checked(economic_flatfile_lock_authority_bounded(root, lock, metadata.lineage,
							 metadata.epoch, requests, &authority,
							 reserve, context, live, &authority_heap));
	need(observation_storage_add(live, authority_heap), ENOBUFS);
	const auto read = flatfile_season_state_read_locked_bounded(root, lock, &season, reserve,
								    context, live);
	need(read == flatfile_season_state_result::ok,
	     read == flatfile_season_state_result::io_error  ? EIO :
	     read == flatfile_season_state_result::not_found ? ENODATA :
							       ESTALE);
	need(season.status == flatfile_season_status::active &&
		     season.epoch == value.image.season_epoch,
	     ESTALE);
#endif
}

unsigned int observation_read_origin_bounded(const std::string &root,
					     const flatfile_authority_lock &lock, uint64_t uid,
					     zone_reset_item_retained_origin *output,
					     flatfile_scratch_reserve_fn reserve, void *context,
					     size_t outer_live)
{
	need(output && lock.matches(root), EINVAL);
	size_t live = outer_live;
	need(observation_storage_add(live, 2 * sizeof(std::string) + sizeof(std::vector<uint8_t>) +
						   sizeof(zone_reset_item_retained_origin) +
						   sizeof(zone_reset_item_image) +
						   sizeof(std::span<const uint8_t>)) &&
		     reserve(live, context),
	     ENOBUFS);
	std::string directory, name;
	std::vector<uint8_t> bytes;
	zone_reset_item_retained_origin observed;
	zone_reset_item_image image;
	observation_origin_paths_bounded(root, uid, &directory, &name, reserve, context, live);
	need((directory.capacity() <= 15 ||
	      (directory.capacity() != SIZE_MAX &&
	       observation_storage_add(live, directory.capacity() + 1))) &&
		     (name.capacity() <= 15 ||
		      (name.capacity() != SIZE_MAX &&
		       observation_storage_add(live, name.capacity() + 1))),
	     ENOBUFS);
	const auto read = flatfile_read_bounded(directory, name, ZONE_RESET_ITEM_ORIGIN_MAX_BYTES,
						&bytes, reserve, context, live);
	if (read != flatfile_read_result::ok)
		return read == flatfile_read_result::not_found ? ENOENT :
		       read == flatfile_read_result::io_error  ? EIO :
								 EILSEQ;
	need(observation_storage_add(live, bytes.capacity()), ENOBUFS);
	const std::span<const uint8_t> wire(bytes);
	size_t origin_heap = 0;
	checked(zone_reset_item_origin_decode_bounded(wire, &observed, reserve, context, live,
						      &origin_heap));
	need(observation_storage_add(live, origin_heap), ENOBUFS);
	checked(zone_reset_item_command_decode_bounded(observed.original, &image, reserve, context,
						       live));
	need(image.items.front().object_uid == uid);
	need(lock.matches(root), EINVAL);
	observed.present = true;
	static_assert(std::is_nothrow_move_assignable_v<zone_reset_item_retained_origin>);
	*output = std::move(observed);
	return 0;
}

struct initial_observation_reservation
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	bool rejected = false;
};
bool initial_observation_reserve(size_t bytes, void *context) noexcept
{
	auto *state = static_cast<initial_observation_reservation *>(context);
	const bool admitted = state->reserve(bytes, state->context);
	state->rejected = state->rejected || !admitted;
	return admitted;
}
// These actual seven references persist through every nested provider call.
// Admit this named object in fixed before its construction; its values continue
// to reflect each successful transfer exactly as the original live sum did.
struct initial_observation_live_state
{
	const size_t &fixed;
	const size_t &value_heap;
	const size_t &world_heap;
	const size_t &custody_heap;
	const size_t &expected_heap;
	const std::vector<uint8_t> &expected_plan;
	const std::vector<uint8_t> &actual_plan;
	bool operator()(size_t *output) const noexcept
	{
		*output = fixed;
		return observation_storage_add(*output, value_heap) &&
		       observation_storage_add(*output, world_heap) &&
		       observation_storage_add(*output, custody_heap) &&
		       observation_storage_add(*output, expected_heap) &&
		       observation_storage_add(*output, expected_plan.capacity()) &&
		       observation_storage_add(*output, actual_plan.capacity());
	}
};
struct initial_observation_workspace
{
	original_values value;
	flatfile_accounting_record existing;
	flatfile_initial_room_reset_world_stage world;
	flatfile_initial_room_reset_custody_stage custody;
	economic_accounting_plan expected;
	std::vector<uint8_t> expected_plan, actual_plan;
	std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> expected_typed{}, actual_typed{};
	zone_reset_item_retained_origin origin;
	flatfile_accounting_pile_state pile;
};
#endif
} // namespace

unsigned int flatfile_accounting_zone_reset_item_transaction::observe_initial_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept
{
	if (!reserve_scratch_peak)
		return EINVAL;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)original;
	(void)context;
	(void)outer_live_scratch;
	return ENOTSUP;
#else
	size_t fixed = outer_live_scratch;
	if (!observation_storage_add(fixed, sizeof(initial_observation_workspace)) ||
	    !observation_storage_add(fixed, sizeof(initial_observation_reservation)) ||
	    !observation_storage_add(fixed, sizeof(initial_observation_live_state)) ||
	    !reserve_scratch_peak(fixed, context))
		return ENOBUFS;
	initial_observation_reservation reservation{ reserve_scratch_peak, context };
	try
	{
		initial_observation_workspace work;
		auto &value = work.value;
		auto &existing = work.existing;
		auto &world = work.world;
		auto &custody = work.custody;
		auto &expected = work.expected;
		auto &expected_plan = work.expected_plan;
		auto &actual_plan = work.actual_plan;
		auto &expected_typed = work.expected_typed;
		auto &actual_typed = work.actual_typed;
		auto &origin = work.origin;
		auto &pile = work.pile;
		auto reserve = initial_observation_reserve;
		void *state = &reservation;
		size_t value_heap = 0, world_heap = 0, custody_heap = 0, expected_heap = 0;
		const initial_observation_live_state live_bytes{ fixed,		value_heap,
								 world_heap,	custody_heap,
								 expected_heap, expected_plan,
								 actual_plan };
		size_t live = fixed;
		need(!root.empty() && lock.matches(root) &&
			     zone_reset_item_recovery_initial_bounded(original, reserve, state,
								      fixed),
		     EINVAL);
		observation_decode_bounded(original, &value, reserve, state, fixed, &value_heap);
		need(live_bytes(&live), ENOBUFS);
		observation_historical_authority_bounded(root, lock, original.command, value,
							 reserve, state, live);
		observation_current_authority_bounded(root, lock, value, reserve, state, live);
		const auto receipt = flatfile_accounting_lookup_bounded(
			root, lock, original.command, &existing, reserve, state, live);
		need(receipt == flatfile_accounting_status::not_found,
		     receipt == flatfile_accounting_status::ok	     ? EEXIST :
		     receipt == flatfile_accounting_status::io_error ? EIO :
								       EILSEQ);
		checked(flatfile_initial_room_reset_world_storage::prepare_locked_bounded(
			root, lock, original, &world, reserve, state, live, &world_heap));
		need(live_bytes(&live), ENOBUFS);
		checked(flatfile_initial_room_reset_custody_storage::prepare_locked_bounded(
			root, lock, original, world, &custody, reserve, state, live,
			&custody_heap));
		need(live_bytes(&live), ENOBUFS);
		observation_command_equal_bounded(world.original_command, original.command, reserve,
						  state, live);
		need(world.room_revision_before == value.image.expected_room_revision &&
		     world.room_revision_after == value.image.expected_room_revision + 1 &&
		     custody.owner_revision_before == world.room_revision_before &&
		     custody.owner_revision_after == world.room_revision_after);
		observation_verify_plan_bounded(original.command, value, &expected, reserve, state,
						live, &expected_heap);
		need(live_bytes(&live), ENOBUFS);
		observation_plan_encode_bounded(expected, &expected_plan, reserve, state, live);
		need(live_bytes(&live), ENOBUFS);
		observation_plan_encode_bounded(custody.plan, &actual_plan, reserve, state, live);
		need(expected_plan == actual_plan);
		need(live_bytes(&live), ENOBUFS);
		size_t typed_peak = live;
		need(observation_storage_add(typed_peak, 2 * sizeof(item_transfer_result)) &&
			     reserve(typed_peak, state),
		     ENOBUFS);
		need(item_transfer_command_encode_result(expected_result(value.image),
							 &expected_typed) &&
		     item_transfer_command_encode_result(custody.result, &actual_typed) &&
		     expected_typed == actual_typed);
		const auto prior_origin = observation_read_origin_bounded(
			root, lock, value.image.items.front().object_uid, &origin, reserve, state,
			live);
		need(prior_origin == ENOENT, prior_origin ? prior_origin : EEXIST);
		for (const auto &effect : expected.accounts)
			if (effect.key.kind == economic_account_kind::pile)
			{
				const auto prior = flatfile_accounting_pile_state_read_bounded(
					root, lock, effect.key.authority_id, &pile, reserve, state,
					live);
				need(prior == flatfile_accounting_status::not_found,
				     prior == flatfile_accounting_status::ok	   ? EEXIST :
				     prior == flatfile_accounting_status::io_error ? EIO :
										     EILSEQ);
			}
		need(lock.matches(root), EINVAL);
		return 0;
	}
	catch (const failure &error)
	{
		return reservation.rejected ? ENOBUFS : error.code;
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

unsigned int flatfile_zone_reset_item_publication_storage::observe_initial_locked_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
	size_t outer_live_scratch) noexcept
{
	return flatfile_accounting_zone_reset_item_transaction::observe_initial_locked_bounded(
		root, lock, original, reserve_scratch_peak, context, outer_live_scratch);
}
