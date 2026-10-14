#include "world/corpse_raise_custody.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/item_ownership_runtime.h"
#include "persistence/corpse_lifecycle_command.h"
#include "persistence/corpse_raise_diagnostics.h"
#include "persistence/persistence_diagnostics.h"
#include "world/vnum.obj.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

extern P_obj object_list;

// Root supplies the genuine SQL-loaded PC corpse and the ORIGINAL committed
// raise callback's typed payload/result, before either runtime application.
// Link complete handler.c, sql_player.c, db.c and the original cache/providers.
// A fabricated object, raw origin setter, extracted body or synthetic command
// result is not a positive fixture. These controls have no main or bootstrap.
// Run only in the exclusive isolated native fixture on its bound game thread.
namespace
{
template <class T> struct restore_field
{
	T &field;
	T saved;
	~restore_field() { field = saved; }
};

bool same_entry(const item_ownership_runtime_entry &a, const item_ownership_runtime_entry &b)
{
	return a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
	       a.parent_item_uid == b.parent_item_uid &&
	       item_owner_identity_equal(a.owner, b.owner) && a.item_revision == b.item_revision &&
	       a.owner_revision == b.owner_revision && a.vnum == b.vnum && a.state == b.state;
}

bool same_entries(const std::vector<item_ownership_runtime_entry> &a,
		  const std::vector<item_ownership_runtime_entry> &b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), same_entry);
}

bool refuses_unchanged(P_obj corpse, const corpse_lifecycle_payload &payload,
		       const corpse_lifecycle_result &result)
{
	const item_owner_identity owner = {
		item_owner_type::corpse, item_corpse_owner_id(payload.owner_pid, payload.save_id), 0
	};
	uint64_t before_revision = 0, after_revision = 0;
	const bool had_revision =
		item_ownership_runtime_peek_owner_revision(owner, &before_revision);
	const size_t before_size = item_ownership_runtime_size();
	std::vector<item_ownership_runtime_entry> before, after;
	if (!item_ownership_runtime_snapshot_owner(owner, ITEM_TRANSFER_MAX_ITEMS + 1, &before))
		return false;
	const std::vector<uint64_t> sentinel{ UINT64_MAX, UINT64_MAX - 1 };
	auto output = sentinel;
	return !persistence_collect_corpse_raise_discarded(corpse, payload, result, &output) &&
	       output == sentinel && item_ownership_runtime_size() == before_size &&
	       item_ownership_runtime_peek_owner_revision(owner, &after_revision) == had_revision &&
	       (!had_revision || after_revision == before_revision) &&
	       item_ownership_runtime_snapshot_owner(owner, ITEM_TRANSFER_MAX_ITEMS + 1, &after) &&
	       same_entries(before, after);
}

P_obj loaded_cash(P_obj corpse, bool with_origin)
{
	std::vector<P_obj> items;
	for (P_obj item = corpse->contains; item; item = item->next_content)
		items.push_back(item);
	for (size_t index = 0; index < items.size(); ++index)
	{
		P_obj item = items[index];
		if (OBJ_VNUM(item) == VOBJ_COINS && item->type == ITEM_MONEY &&
		    item->corpse_money_origin.present() == with_origin)
			return item;
		for (P_obj child = item->contains; child; child = child->next_content)
			items.push_back(child);
	}
	return nullptr;
}
}

bool corpse_raise_cash_native_controls(P_obj corpse, const corpse_lifecycle_payload &payload,
				       const corpse_lifecycle_result &result)
{
	std::vector<uint64_t> baseline;
	if (!persistence_collect_corpse_raise_discarded(corpse, payload, result, &baseline))
		return false;
	P_obj cash = loaded_cash(corpse, true);
	if (!cash || cash->contains || baseline.size() != result.discarded_item_count ||
	    std::find(baseline.begin(), baseline.end(), cash->obj_uid) != baseline.end())
		return false;
	const uint64_t owner_id = item_corpse_owner_id(payload.owner_pid, payload.save_id);
	item_ownership_runtime_entry absent = {};
	if (!cash->corpse_money_origin.matches(cash, cash->obj_uid, owner_id) ||
	    cash->corpse_money_origin.matches(corpse, cash->obj_uid, owner_id) ||
	    cash->corpse_money_origin.matches(cash, cash->obj_uid, owner_id ^ 1) ||
	    item_ownership_runtime_lookup(cash->obj_uid, &absent))
		return false;
	{
		restore_field restore{ cash->corpse_money_origin, cash->corpse_money_origin };
		cash->corpse_money_origin = {};
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ cash->obj_uid, cash->obj_uid };
		cash->obj_uid ^= 1;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ cash->type, cash->type };
		cash->type = ITEM_CONTAINER;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ cash->contains, cash->contains };
		cash->contains = corpse;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ cash->loc.inside, cash->loc.inside };
		cash->loc.inside = cash;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ cash->next_content, cash->next_content };
		cash->next_content = cash;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ cash->value[0], cash->value[0] };
		cash->value[0] = cash->value[0] == INT32_MAX ? 0 : cash->value[0] + 1;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		restore_field restore{ corpse->obj_uid, corpse->obj_uid };
		corpse->obj_uid = cash->obj_uid;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		P_obj outside = nullptr;
		for (P_obj object = object_list; object; object = object->next)
		{
			P_obj ancestor = object;
			size_t depth = 0;
			while (ancestor != corpse && OBJ_INSIDE(ancestor) && ancestor->loc.inside &&
			       depth++ <= ITEM_TRANSFER_MAX_ITEMS)
				ancestor = ancestor->loc.inside;
			if (ancestor != corpse)
			{
				outside = object;
				break;
			}
		}
		if (!outside)
			return false;
		restore_field restore{ outside->obj_uid, outside->obj_uid };
		outside->obj_uid = cash->obj_uid;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		if (!object_list)
			return false;
		restore_field restore{ object_list->next, object_list->next };
		object_list->next = object_list;
		if (!refuses_unchanged(corpse, payload, result))
			return false;
	}
	{
		auto stale = result;
		++stale.discarded_item_count;
		if (!refuses_unchanged(corpse, payload, stale))
			return false;
	}
	// Any genuine cache row, including retained destruction/quarantine history,
	// contradicts this loaded scalar origin. Original hydrate/forget calls only
	// arrange a scoped negative fixture at the SAME existing corpse owner clock.
	const item_owner_identity owner = { item_owner_type::corpse, owner_id, 0 };
	uint64_t owner_revision = 0;
	if (!item_ownership_runtime_peek_owner_revision(owner, &owner_revision))
		return false;
	for (const auto state : { item_custody_state::active, item_custody_state::destroyed,
				  item_custody_state::quarantined })
	{
		struct forget_fixture_row
		{
			uint64_t uid;
			~forget_fixture_row() { item_ownership_runtime_forget(uid); }
		} remove{ cash->obj_uid };
		const item_ownership_runtime_entry conflict{
			cash->obj_uid, cash->obj_uid, 0, owner, 1, owner_revision, VOBJ_COINS, state
		};
		if (!item_ownership_runtime_hydrate(conflict) ||
		    !refuses_unchanged(corpse, payload, result))
			return false;
	}
	// The real stored-UID transient from the same SQL corpse must also retain
	// its exact active owner, parent/root and vnum. Arrange each negative with
	// the original native cache API, then restore the unchanged original row.
	item_ownership_runtime_entry stored = {};
	if (baseline.empty() || !item_ownership_runtime_lookup(baseline.front(), &stored) ||
	    stored.root_item_uid == stored.item_uid || !stored.parent_item_uid)
		return false;
	{
		struct restore_fixture_row
		{
			item_ownership_runtime_entry original;
			~restore_fixture_row() { (void)item_ownership_runtime_hydrate(original); }
		} restore{ stored };
		std::array<item_ownership_runtime_entry, 5> wrong;
		wrong.fill(stored);
		wrong[0].state = item_custody_state::destroyed;
		wrong[1].state = item_custody_state::quarantined;
		wrong[2].root_item_uid = stored.item_uid;
		wrong[3].parent_item_uid = 0;
		wrong[4].vnum = stored.vnum == 0 ? 1 : 0;
		for (const auto &entry : wrong)
			if (!item_ownership_runtime_hydrate(entry) ||
			    !refuses_unchanged(corpse, payload, result))
				return false;
		const item_owner_identity player{ item_owner_type::player,
						  payload.destination_player_pid, 0 };
		uint64_t player_revision = 0;
		if (!item_ownership_runtime_peek_owner_revision(player, &player_revision))
			return false;
		auto wrong_owner = stored;
		wrong_owner.owner = player;
		wrong_owner.owner_revision = player_revision;
		if (!item_ownership_runtime_hydrate(wrong_owner) ||
		    !refuses_unchanged(corpse, payload, result))
			return false;
	}
	std::vector<uint64_t> restored;
	return persistence_collect_corpse_raise_discarded(corpse, payload, result, &restored) &&
	       restored == baseline && !item_ownership_runtime_lookup(cash->obj_uid, &absent);
}

// Separate required SQL fixture: this coin row HAS a stored UID and was hydrated
// by the original loader. Its UID remains in the discarded custody selection.
bool corpse_raise_tracked_cash_native_controls(P_obj corpse,
					       const corpse_lifecycle_payload &payload,
					       const corpse_lifecycle_result &result)
{
	std::vector<uint64_t> baseline;
	if (!persistence_collect_corpse_raise_discarded(corpse, payload, result, &baseline))
		return false;
	P_obj cash = loaded_cash(corpse, false);
	item_ownership_runtime_entry original = {};
	if (!cash || std::find(baseline.begin(), baseline.end(), cash->obj_uid) == baseline.end() ||
	    !item_ownership_runtime_lookup(cash->obj_uid, &original))
		return false;
	bool refused = false;
	{
		struct restore_fixture_row
		{
			item_ownership_runtime_entry original;
			~restore_fixture_row() { (void)item_ownership_runtime_hydrate(original); }
		} restore{ original };
		item_ownership_runtime_forget(cash->obj_uid);
		refused = refuses_unchanged(corpse, payload, result);
	}
	std::vector<uint64_t> restored;
	item_ownership_runtime_entry after = {};
	return refused && item_ownership_runtime_lookup(cash->obj_uid, &after) &&
	       same_entry(original, after) &&
	       persistence_collect_corpse_raise_discarded(corpse, payload, result, &restored) &&
	       restored == baseline;
}

bool corpse_raise_command_body_native_controls(const corpse_lifecycle_payload &actual_payload)
{
	const auto *body = corpse_raise_diagnostic_command_observe();
	if (!corpse_raise_diagnostics_enabled() || !body || !body->sequence ||
	    body->overwritten != body->sequence - 1 ||
	    body->status != corpse_raise_command_capture_status::encoded ||
	    body->codec_result != critical_command_codec_result::ok || !body->encoded_size ||
	    body->encoded_size > CRITICAL_COMMAND_MAX_ENCODED_BYTES ||
	    body->corpse_key !=
		    item_corpse_owner_id(actual_payload.owner_pid, actual_payload.save_id))
		return false;
	critical_command command;
	if (critical_command_decode(body->encoded.data(), body->encoded_size, &command) !=
		    critical_command_codec_result::ok ||
	    !critical_operation_id_equal(command.operation_id, body->operation) ||
	    command.schema_version != body->schema_version ||
	    static_cast<uint16_t>(command.type) != body->command_type ||
	    command.payload_version != body->payload_version)
		return false;
	std::vector<uint8_t> original_payload, roundtrip;
	return corpse_lifecycle_command_encode_payload(actual_payload, &original_payload) &&
	       command.payload == original_payload &&
	       critical_command_encode(command, &roundtrip) == critical_command_codec_result::ok &&
	       roundtrip.size() == body->encoded_size &&
	       std::equal(roundtrip.begin(), roundtrip.end(), body->encoded.begin());
}

// Separate process profile: the original optional flag is absent at boot.
bool corpse_raise_diagnostics_disabled_native_control()
{
	return !corpse_raise_diagnostics_enabled() &&
	       corpse_raise_diagnostic_command_observe() == nullptr;
}

// Invoke AFTER the real original callback. The accepted positive profile must
// show both genuine calls once, in order, with true returns. The separate first
// refusal profile must have no second entry/result. No callback is invoked here.
bool corpse_raise_stage_native_controls(bool expect_success)
{
	const auto *body = corpse_raise_diagnostic_command_observe();
	if (!body || body->status != corpse_raise_command_capture_status::encoded)
		return false;
	persistence_trace_filter filter;
	filter.by_operation = true;
	filter.operation = body->operation;
	const auto history = persistence_trace_copy(filter);
	if (!history.available || history.dropped || history.overwritten ||
	    history.matching_events > history.events.size())
		return false;
	std::array<uint64_t, 4> sequence = {};
	std::array<uint32_t, 4> count = {}, outcome = {};
	for (size_t index = 0; index < history.count; ++index)
	{
		const auto &event = history.events[index];
		size_t phase = 4;
		switch (event.stage)
		{
		case persistence_trace_stage::corpse_raise_discard_enter:
			phase = 0;
			break;
		case persistence_trace_stage::corpse_raise_discard_result:
			phase = 1;
			break;
		case persistence_trace_stage::corpse_raise_transfer_enter:
			phase = 2;
			break;
		case persistence_trace_stage::corpse_raise_transfer_result:
			phase = 3;
			break;
		default:
			break;
		}
		if (phase < count.size())
		{
			++count[phase];
			sequence[phase] = event.sequence;
			outcome[phase] = event.outcome;
		}
	}
	if (count[0] != 1 || count[1] != 1 || sequence[0] >= sequence[1])
		return false;
	if (!expect_success)
		return outcome[1] == 0 && count[2] == 0 && count[3] == 0;
	return outcome[1] == 1 && count[2] == 1 && count[3] == 1 && outcome[3] == 1 &&
	       sequence[1] < sequence[2] && sequence[2] < sequence[3];
}
