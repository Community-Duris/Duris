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
