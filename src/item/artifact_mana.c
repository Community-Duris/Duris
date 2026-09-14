#include "item/artifact_mana.h"
#include "item/item_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "persistence/persistence_mode.h"

#include <chrono>
#include <map>
#include <thread>
#include <time.h>

extern P_obj object_list;
extern P_index obj_index;

namespace
{
std::map<uint64_t, artifact_mana_profile> profiles;
std::map<int, uint64_t> bindings;
std::unique_ptr<artifact_mana_runtime> runtime;

uint64_t monotonic_ms()
{
	timespec now = {};
	if (clock_gettime(CLOCK_MONOTONIC, &now) || now.tv_sec < 0)
		return 0;
	return uint64_t(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
}

const artifact_mana_profile *profile_for(P_obj source)
{
	if (!source || !source->obj_uid)
		return nullptr;
	const auto found = bindings.find(OBJ_VNUM(source));
	if (found == bindings.end())
		return nullptr;
	// Duplicate materializations share no spend authority. Check live objects,
	// including containers and unloaded holders, before accepting any debit.
	size_t matches = 0;
	bool exact = false;
	for (P_obj object = object_list; object; object = object->next)
		if (object->obj_uid == source->obj_uid)
		{
			++matches;
			exact = exact || object == source;
		}
	return matches == 1 && exact ? &profiles.at(found->second) : nullptr;
}

artifact_mana_runtime &service()
{
	if (!runtime)
		runtime = std::make_unique<artifact_mana_runtime>(artifact_mana_persistent_backend(
			persistence_mode_requires_mysql(), persistence_mode_flatfile_root()));
	return *runtime;
}
} // namespace

bool artifact_mana_can_publish(int vnum, const artifact_mana_profile &profile)
{
	if (!nevent_is_game_thread() || vnum <= 0 || !artifact_mana_valid(profile))
		return false;
	const auto bound = bindings.find(vnum);
	if (bound != bindings.end() && bound->second != profile.id)
		return false;
	const auto old = profiles.find(profile.id);
	if (old != profiles.end() &&
	    (profile.revision < old->second.revision ||
	     (profile.revision == old->second.revision && profile != old->second)))
		return false;
	return true;
}

bool artifact_mana_publish(int vnum, const artifact_mana_profile &profile)
{
	if (!artifact_mana_can_publish(vnum, profile))
		return false;
	profiles[profile.id] = profile;
	bindings[vnum] = profile.id;
	return true;
}

bool artifact_mana_inspect(P_obj source, artifact_mana_record &record)
{
	if (!nevent_is_game_thread())
		return false;
	const auto *profile = profile_for(source);
	const time_t wall = time(nullptr);
	try
	{
		return profile && wall >= 0 &&
		       service().inspect(source->obj_uid, *profile, uint64_t(wall), monotonic_ms(),
					 record);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool artifact_mana_debit(P_obj source, uint64_t cost, bool passive, uint64_t token)
{
	if (!nevent_is_game_thread() || get_property("itemActions.enabled", 0.0) != 1 ||
	    get_property("itemActions.mana.enabled", 0.0) != 1)
		return false;
	const auto *profile = profile_for(source);
	const time_t wall = time(nullptr);
	try
	{
		const uint64_t mono = monotonic_ms();
		const bool paid = profile && wall >= 0 &&
				  service().debit(source->obj_uid, *profile, uint64_t(wall), mono,
						  cost, passive, token);
		if (!paid && item_actions_telemetry_enabled())
		{
			artifact_mana_record record;
			const bool readable = profile && wall >= 0 && runtime &&
					      runtime->spending_ready(source->obj_uid, mono) &&
					      runtime->inspect(source->obj_uid, *profile,
							       uint64_t(wall), mono, record);
			const bool insufficient =
				readable &&
				(cost > record.reserve ||
				 (passive && record.reserve - cost < profile->passive_floor));
			item_actions_note(insufficient ? item_action_metric::insufficient_mana :
							 item_action_metric::mana_unavailable);
		}
		return paid;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

void artifact_mana_pulse()
{
	if (runtime)
		runtime->pulse(monotonic_ms());
}

void artifact_mana_shutdown()
{
	if (!runtime)
		return;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (std::chrono::steady_clock::now() < deadline)
	{
		artifact_mana_pulse();
		const auto health = runtime->health();
		if (!health.dirty && !health.outstanding)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	const auto health = runtime->health();
	if (health.dirty || health.outstanding)
		logit(LOG_STATUS,
		      "Artifact mana shutdown: %zu dirty pools, %zu outstanding requests; bounded crash-refund policy applies.",
		      health.dirty, health.outstanding);
	runtime->stop();
	runtime.reset();
}

void do_itemmana(P_char ch, char *argument, int /*cmd*/)
{
	char name[MAX_INPUT_LENGTH] = {};
	one_argument(argument, name);
	if (!strcmp(name, "metrics") && IS_TRUSTED(ch))
	{
		item_actions_dump_telemetry(ch);
		return;
	}
	if (!*name)
	{
		send_to_char("Inspect which carried or equipped item's mana?\r\n", ch);
		return;
	}
	P_obj source = get_obj_in_list_vis(ch, name, ch->carrying);
	int slot = -1;
	if (!source)
		source = get_object_in_equip_vis(ch, name, &slot);
	if (!source || !profile_for(source))
	{
		send_to_char(
			"You do not hold an item with an available mana pool by that name.\r\n",
			ch);
		return;
	}
	artifact_mana_record record;
	if (!artifact_mana_inspect(source, record))
	{
		send_to_char("That item's mana record is not ready. Try again shortly.\r\n", ch);
		return;
	}
	char message[256];
	snprintf(message, sizeof(message),
		 "Item mana: %llu.%03llu / %llu.%03llu; regeneration %llu.%03llu per second.\r\n",
		 static_cast<unsigned long long>(record.reserve / 1000),
		 static_cast<unsigned long long>(record.reserve % 1000),
		 static_cast<unsigned long long>(record.capacity / 1000),
		 static_cast<unsigned long long>(record.capacity % 1000),
		 static_cast<unsigned long long>(record.regeneration / 1000),
		 static_cast<unsigned long long>(record.regeneration % 1000));
	send_to_char(message, ch);
	const auto *profile = profile_for(source);
	snprintf(message, sizeof(message),
		 "Automatic powers preserve %llu.%03llu mana. Paid powers: %s.\r\n",
		 static_cast<unsigned long long>(profile->passive_floor / 1000),
		 static_cast<unsigned long long>(profile->passive_floor % 1000),
		 get_property("itemActions.enabled", 0.0) != 1 ||
				 get_property("itemActions.mana.enabled", 0.0) != 1 ?
			 "disabled" :
			 (runtime->spending_ready(source->obj_uid, monotonic_ms()) ?
				  "available subject to ability cost" :
				  "waiting for storage"));
	send_to_char(message, ch);
}
