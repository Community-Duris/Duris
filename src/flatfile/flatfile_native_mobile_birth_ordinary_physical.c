#include "flatfile/flatfile_native_mobile_birth_ordinary_physical.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "economy/native_mobile_birth_cash_role_command.h"

#include <algorithm>
#include <cerrno>
#include <new>

unsigned int
flatfile_native_mobile_birth_ordinary_physical_storage::verify_catalog_namespaces_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence *output,
	std::string *error) noexcept
{
	if (root.empty() || !output || !identity_lock.matches(root) ||
	    !authority_lock.matches(root))
		return EINVAL;
	try
	{
		if (!native_mobile_birth_cash_role_recovery_valid(original))
			return EINVAL;
		quest_mobile_native_image image;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
		economic_frozen_intent intent;
		if (native_mobile_birth_cash_role_command_decode(original.command, &image, &recipes,
								 &role) !=
			    economic_accounting_error::ok ||
		    role.role != native_mobile_birth_cash_role::ordinary_wallet ||
		    economic_intent_decode(original.command.accounting_intent, &intent) !=
			    economic_accounting_error::ok ||
		    economic_intent_verify_binding(original.command, intent) !=
			    economic_accounting_error::ok ||
		    !intent.admission.metadata.source_event)
			return EINVAL;
		std::vector<uint64_t> born;
		born.reserve(image.items.size());
		for (const auto &literal : image.items)
			born.push_back(literal.object_uid);
		std::sort(born.begin(), born.end());
		if (std::adjacent_find(born.begin(), born.end()) != born.end() ||
		    (!born.empty() && !born.front()))
			return EINVAL;
		flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence observed;
		auto status =
			flatfile_native_mobile_birth_ordinary_player_physical_storage::verify_locked(
				root, identity_lock, authority_lock, original, &observed.players,
				error);
		if (status)
			return status;
		status =
			flatfile_native_mobile_birth_ordinary_auction_physical_storage::verify_locked(
				root, authority_lock, original, &observed.auction, error);
		if (status)
			return status;
		status = flatfile_native_mobile_birth_ordinary_collector_physical_storage::
			verify_locked(root, authority_lock, original, &observed.collector, error);
		if (status)
			return status;

		{
			std::vector<flatfile_corpse_record> corpses;
			std::vector<flatfile_room_item_record> rooms;
			std::vector<flatfile_saved_world_item_record> saved;
			const auto loaded = flatfile_world_item_recovery_list_all_locked(
				root, authority_lock, &corpses, &rooms, &saved, error);
			if (loaded != flatfile_world_item_result::ok &&
			    loaded != flatfile_world_item_result::not_found)
				return loaded == flatfile_world_item_result::io_error ? EIO :
											EILSEQ;
			observed.corpses = corpses.size();
			observed.rooms = rooms.size();
			observed.saved_world_records = saved.size();
			for (const auto &corpse : corpses)
				for (const auto &item : corpse.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.world_item_rows;
				}
			for (const auto &room : rooms)
				for (const auto &item : room.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.world_item_rows;
				}
			for (const auto &record : saved)
				for (const auto &item : record.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.world_item_rows;
				}
		}
		{
			std::vector<flatfile_locker_record> lockers;
			const auto loaded = flatfile_locker_recovery_list_locked(
				root, authority_lock, &lockers, error);
			if (loaded != flatfile_locker_result::ok &&
			    loaded != flatfile_locker_result::not_found)
				return loaded == flatfile_locker_result::io_error ? EIO : EILSEQ;
			observed.lockers = lockers.size();
			for (const auto &locker : lockers)
				for (const auto &chest : locker.chests)
				{
					++observed.locker_chests;
					for (const auto &item : chest.items)
					{
						if (std::binary_search(born.begin(), born.end(),
								       item.object_uid))
							return EEXIST;
						++observed.locker_item_rows;
					}
				}
		}
		{
			std::vector<flatfile_shopkeeper_record> keepers;
			const auto loaded = flatfile_shopkeeper_list_locked(root, authority_lock,
									    &keepers, error);
			if (loaded != flatfile_shopkeeper_result::ok &&
			    loaded != flatfile_shopkeeper_result::not_found)
				return loaded == flatfile_shopkeeper_result::io_error ? EIO :
											EILSEQ;
			observed.shopkeepers = keepers.size();
			for (const auto &keeper : keepers)
				for (const auto &item : keeper.items)
				{
					if (std::binary_search(born.begin(), born.end(),
							       item.object_uid))
						return EEXIST;
					++observed.shop_item_rows;
				}
		}
		if (!identity_lock.matches(root) || !authority_lock.matches(root))
			return EINVAL;
		// The genuine caller separately joins all remaining original namespaces,
		// custody UID/root/parent scopes, source/history and the atomic root bundle.
		*output = observed;
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

#include "economy/economic_accounting_intent.h"
#include "player/player_snapshot_codec.h"
#include <type_traits>

namespace
{
struct ordinary_physical_refusal
{
	unsigned int error;
};
size_t ordinary_physical_add(size_t left, size_t right)
{
	if (right > SIZE_MAX - left)
		throw ordinary_physical_refusal{ ENOBUFS };
	return left + right;
}
size_t ordinary_physical_product(size_t count, size_t width)
{
	if (width && count > SIZE_MAX / width)
		throw ordinary_physical_refusal{ ENOBUFS };
	return count * width;
}
struct ordinary_physical_reservation
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	bool refused = false;
	static bool callback(size_t bytes, void *opaque) noexcept
	{
		auto &self = *static_cast<ordinary_physical_reservation *>(opaque);
		if (self.refused || !self.reserve || !self.reserve(bytes, self.context))
		{
			self.refused = true;
			return false;
		}
		return true;
	}
};
struct ordinary_physical_workspace
{
	quest_mobile_native_image image;
	std::vector<native_mobile_birth_item_recipe> recipes;
	native_mobile_birth_cash_role_recipe role;
	economic_frozen_intent intent;
	std::span<const uint8_t> intent_wire;
	std::vector<uint64_t> born;
	flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence observed;
	size_t image_heap = 0, recipe_heap = 0;
	ordinary_physical_reservation admission;
};
// Genuine non-recursive sorting scope: the carrier is the actual workspace,
// not a guessed multiplier for opaque library recursive sort frames.
struct ordinary_physical_sort_workspace
{
	size_t count = 0, start = 0, end = 0, root = 0, child = 0;
	uint64_t temporary = 0;
};
void ordinary_physical_sort(std::vector<uint64_t> &values,
			    ordinary_physical_sort_workspace &work) noexcept
{
	work.count = values.size();
	if (work.count < 2)
		return;
	work.start = work.count / 2;
	work.end = work.count;
	for (;;)
	{
		if (work.start)
		{
			--work.start;
			work.temporary = values[work.start];
			work.root = work.start;
		}
		else
		{
			--work.end;
			if (!work.end)
				return;
			work.temporary = values[work.end];
			values[work.end] = values[0];
			work.root = 0;
		}
		while (work.root < work.end / 2)
		{
			work.child = work.root * 2 + 1;
			if (work.child + 1 < work.end &&
			    values[work.child] < values[work.child + 1])
				++work.child;
			if (work.temporary >= values[work.child])
				break;
			values[work.root] = values[work.child];
			work.root = work.child;
		}
		values[work.root] = work.temporary;
	}
}
size_t ordinary_physical_heap(const ordinary_physical_workspace &work)
{
	return ordinary_physical_add(
		work.image_heap,
		ordinary_physical_add(
			work.recipe_heap,
			ordinary_physical_add(work.intent.admission.facts.capacity(),
					      ordinary_physical_product(work.born.capacity(),
									sizeof(uint64_t)))));
}
struct ordinary_physical_budget
{
	size_t outer, fixed;
	ordinary_physical_workspace &work;
	size_t live() const
	{
		return ordinary_physical_add(ordinary_physical_add(outer, fixed),
					     ordinary_physical_heap(work));
	}
	void admit(size_t extra = 0) const
	{
		if (!ordinary_physical_reservation::callback(ordinary_physical_add(live(), extra),
							     &work.admission))
			throw ordinary_physical_refusal{ ENOBUFS };
	}
	unsigned int codec(economic_accounting_error error) const noexcept
	{
		if (work.admission.refused || error == economic_accounting_error::capacity ||
		    error == economic_accounting_error::overflow)
			return ENOBUFS;
		return error == economic_accounting_error::unresolved ? ENOTSUP : EINVAL;
	}
	unsigned int storage(bool io) const noexcept
	{
		if (work.admission.refused || errno == ENOBUFS || errno == EOVERFLOW ||
		    errno == ENOSPC)
			return ENOBUFS;
		if (errno == ENOMEM)
			return ENOMEM;
		if (errno == ENOTSUP)
			return ENOTSUP;
		return io ? EIO : EILSEQ;
	}
};
void ordinary_physical_increment(size_t &value)
{
	value = ordinary_physical_add(value, 1);
}
bool ordinary_physical_contains(const std::vector<uint64_t> &born, uint64_t uid) noexcept
{
	size_t first = 0, last = born.size(), middle = 0;
	while (first < last)
	{
		middle = first + (last - first) / 2;
		if (born[middle] < uid)
			first = middle + 1;
		else
			last = middle;
	}
	return first < born.size() && born[first] == uid;
}
size_t ordinary_physical_keeper_heap(const std::vector<flatfile_shopkeeper_record> &keepers)
{
	size_t bytes =
		ordinary_physical_product(keepers.capacity(), sizeof(flatfile_shopkeeper_record));
	for (const auto &keeper : keepers)
	{
		bytes = ordinary_physical_add(
			bytes,
			ordinary_physical_product(keeper.affects.capacity(),
						  sizeof(flatfile_shopkeeper_affect_record)));
		bytes = ordinary_physical_add(
			bytes, ordinary_physical_product(keeper.items.capacity(),
							 sizeof(player_item_snapshot)));
		for (const auto &item : keeper.items)
		{
			size_t heap = 0;
			if (!player_item_snapshot_current_heap_bytes(item, &heap))
				throw ordinary_physical_refusal{ ENOBUFS };
			bytes = ordinary_physical_add(bytes, heap);
		}
	}
	return bytes;
}
} // namespace

unsigned int
flatfile_native_mobile_birth_ordinary_physical_storage::verify_catalog_namespaces_locked_bounded(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	const critical_native_recovery_envelope &original,
	flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence *output,
	flatfile_scratch_reserve_fn reserve, void *context, size_t outer_live) noexcept
{
	if (root.empty() || !output || !reserve || !identity_lock.matches(root) ||
	    !authority_lock.matches(root))
		return EINVAL;
#if !defined(__linux__) || !defined(__x86_64__) || !defined(__LP64__) || \
	!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 ||          \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)original;
	(void)context;
	(void)outer_live;
	return ENOTSUP;
#else
	try
	{
		// Complete wrapper, argument/result, callback, traversal, scalar and
		// exact allocation-free sort/search scopes. Native emitted/library proof
		// remains separate from these source-owned carriers.
		const size_t fixed =
			sizeof(ordinary_physical_workspace) + sizeof(ordinary_physical_budget) +
			sizeof(ordinary_physical_sort_workspace) + 22 * sizeof(void *) +
			16 * sizeof(size_t) + 3 * sizeof(unsigned int) +
			2 * sizeof(economic_accounting_error) + 6 * sizeof(bool) +
			player_item_snapshot_copy_frame_bytes();
		size_t first = ordinary_physical_add(outer_live, fixed);
		if (!reserve(first, context))
			return ENOBUFS;
		ordinary_physical_workspace work;
		work.admission = { reserve, context, false };
		ordinary_physical_budget budget{ outer_live, fixed, work };
		auto callback = &ordinary_physical_reservation::callback;
		void *reservation = &work.admission;
		auto error = native_mobile_birth_cash_role_recovery_validate_bounded(
			original, callback, reservation, budget.live());
		if (error != economic_accounting_error::ok)
			return budget.codec(error);
		error = native_mobile_birth_cash_role_command_decode_bounded(
			original.command, &work.image, &work.recipes, &work.role, callback,
			reservation, budget.live(), &work.image_heap, &work.recipe_heap);
		if (error != economic_accounting_error::ok)
			return budget.codec(error);
		if (work.role.role != native_mobile_birth_cash_role::ordinary_wallet)
			return EINVAL;
		work.intent_wire = original.command.accounting_intent;
		error = economic_intent_decode_bounded(work.intent_wire, &work.intent, callback,
						       reservation, budget.live());
		if (error != economic_accounting_error::ok)
			return budget.codec(error);
		error = economic_intent_verify_binding_bounded(
			original.command, work.intent, callback, reservation, budget.live());
		if (error != economic_accounting_error::ok)
			return budget.codec(error);
		if (!work.intent.admission.metadata.source_event)
			return EINVAL;
		budget.admit(ordinary_physical_product(work.image.items.size(), sizeof(uint64_t)));
		work.born.reserve(work.image.items.size());
		for (const auto &literal : work.image.items)
			work.born.push_back(literal.object_uid);
		budget.admit();
		ordinary_physical_sort_workspace sorting;
		ordinary_physical_sort(work.born, sorting);
		for (size_t index = 0; index < work.born.size(); ++index)
			if (!work.born[index] ||
			    (index && work.born[index] == work.born[index - 1]))
				return EINVAL;
		auto status = flatfile_native_mobile_birth_ordinary_player_physical_storage::
			verify_locked_bounded(root, identity_lock, authority_lock, original,
					      &work.observed.players, callback, reservation,
					      budget.live());
		if (status)
			return work.admission.refused ? ENOBUFS : status;
		status = flatfile_native_mobile_birth_ordinary_auction_physical_storage::
			verify_locked_bounded(root, authority_lock, original,
					      &work.observed.auction, callback, reservation,
					      budget.live());
		if (status)
			return work.admission.refused ? ENOBUFS : status;
		status = flatfile_native_mobile_birth_ordinary_collector_physical_storage::
			verify_locked_bounded(root, authority_lock, original,
					      &work.observed.collector, callback, reservation,
					      budget.live());
		if (status)
			return work.admission.refused ? ENOBUFS : status;
		{
			constexpr size_t own =
				sizeof(std::vector<flatfile_corpse_record>) +
				sizeof(std::vector<flatfile_room_item_record>) +
				sizeof(std::vector<flatfile_saved_world_item_record>) +
				sizeof(size_t) + sizeof(flatfile_world_item_result);
			budget.admit(own);
			std::vector<flatfile_corpse_record> corpses;
			std::vector<flatfile_room_item_record> rooms;
			std::vector<flatfile_saved_world_item_record> saved;
			size_t heap = 0;
			errno = 0;
			const auto loaded = flatfile_world_item_recovery_list_all_locked_bounded(
				root, authority_lock, &corpses, &rooms, &saved, callback,
				reservation, ordinary_physical_add(budget.live(), own), &heap);
			if (loaded != flatfile_world_item_result::ok &&
			    loaded != flatfile_world_item_result::not_found)
				return budget.storage(loaded ==
						      flatfile_world_item_result::io_error);
			budget.admit(ordinary_physical_add(own, heap));
			work.observed.corpses = corpses.size();
			work.observed.rooms = rooms.size();
			work.observed.saved_world_records = saved.size();
			for (const auto &corpse : corpses)
				for (const auto &item : corpse.items)
				{
					if (ordinary_physical_contains(work.born, item.object_uid))
						return EEXIST;
					ordinary_physical_increment(work.observed.world_item_rows);
				}
			for (const auto &room : rooms)
				for (const auto &item : room.items)
				{
					if (ordinary_physical_contains(work.born, item.object_uid))
						return EEXIST;
					ordinary_physical_increment(work.observed.world_item_rows);
				}
			for (const auto &record : saved)
				for (const auto &item : record.items)
				{
					if (ordinary_physical_contains(work.born, item.object_uid))
						return EEXIST;
					ordinary_physical_increment(work.observed.world_item_rows);
				}
		}
		{
			constexpr size_t own = sizeof(std::vector<flatfile_locker_record>) +
					       sizeof(size_t) + sizeof(flatfile_locker_result);
			budget.admit(own);
			std::vector<flatfile_locker_record> lockers;
			size_t heap = 0;
			errno = 0;
			const auto loaded = flatfile_locker_recovery_list_locked_bounded(
				root, authority_lock, &lockers, callback, reservation,
				ordinary_physical_add(budget.live(), own), &heap);
			if (loaded != flatfile_locker_result::ok &&
			    loaded != flatfile_locker_result::not_found)
				return budget.storage(loaded == flatfile_locker_result::io_error);
			budget.admit(ordinary_physical_add(own, heap));
			work.observed.lockers = lockers.size();
			for (const auto &locker : lockers)
				for (const auto &chest : locker.chests)
				{
					ordinary_physical_increment(work.observed.locker_chests);
					for (const auto &item : chest.items)
					{
						if (ordinary_physical_contains(work.born,
									       item.object_uid))
							return EEXIST;
						ordinary_physical_increment(
							work.observed.locker_item_rows);
					}
				}
		}
		{
			constexpr size_t own = sizeof(std::vector<flatfile_shopkeeper_record>) +
					       sizeof(size_t) + sizeof(flatfile_shopkeeper_result);
			budget.admit(own);
			std::vector<flatfile_shopkeeper_record> keepers;
			errno = 0;
			const auto loaded = flatfile_shopkeeper_list_locked_bounded(
				root, authority_lock, &keepers, callback, reservation,
				ordinary_physical_add(budget.live(), own));
			if (loaded != flatfile_shopkeeper_result::ok &&
			    loaded != flatfile_shopkeeper_result::not_found)
				return budget.storage(loaded ==
						      flatfile_shopkeeper_result::io_error);
			const size_t heap = ordinary_physical_keeper_heap(keepers);
			budget.admit(ordinary_physical_add(own, heap));
			work.observed.shopkeepers = keepers.size();
			for (const auto &keeper : keepers)
				for (const auto &item : keeper.items)
				{
					if (ordinary_physical_contains(work.born, item.object_uid))
						return EEXIST;
					ordinary_physical_increment(work.observed.shop_item_rows);
				}
		}
		budget.admit();
		if (!identity_lock.matches(root) || !authority_lock.matches(root))
			return EINVAL;
		static_assert(std::is_nothrow_copy_assignable_v<
			      flatfile_native_mobile_birth_ordinary_catalog_namespaces_absence>);
		*output = work.observed;
		return 0;
	}
	catch (const ordinary_physical_refusal &failure)
	{
		return failure.error;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
#endif
}
