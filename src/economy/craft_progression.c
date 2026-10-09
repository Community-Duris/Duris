#include "core/prototypes.h"
#include "core/files.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/db.h"
#include "magic/spells.h"
#include "player/craft_progression_hooks.h"
#include "player/player_save_pipeline.h"
#include "net/comm.h"

#include <algorithm>
#include <chrono>
#include <new>
#include <array>
#include <map>

extern int top_of_world;

namespace
{
struct progression_attempt
{
	uint32_t pid = 0;
	player_craft_receipt_snapshot receipt = {};
	bool applied = false;
	bool acknowledged = false;
	bool save_pending = false;
	std::chrono::steady_clock::time_point next_save = {};
};

std::map<std::array<uint8_t, 16>, progression_attempt> attempts;
constexpr size_t MAX_PROGRESSION_ATTEMPTS = 1024;

std::array<uint8_t, 16> operation_key(const critical_operation_id &operation)
{
	return operation.bytes;
}

bool receipt_equal(const player_craft_receipt_snapshot &left,
		   const player_craft_receipt_snapshot &right)
{
	return left.operation_id.bytes == right.operation_id.bytes &&
	       left.discipline == right.discipline && left.experience == right.experience;
}

bool valid_receipt(const player_craft_receipt_snapshot &receipt)
{
	return std::any_of(receipt.operation_id.bytes.begin(), receipt.operation_id.bytes.end(),
			   [](uint8_t byte) { return byte != 0; }) &&
	       receipt.discipline >= 1 && receipt.discipline <= 6 &&
	       (receipt.discipline <= 2 || !receipt.experience) && receipt.experience <= INT_MAX;
}

bool pending_receipts(uint32_t pid, std::vector<player_craft_receipt_snapshot> *receipts)
{
	if (!pid || !receipts)
		return false;
	try
	{
		std::vector<player_craft_receipt_snapshot> pending;
		for (const auto &[key, attempt] : attempts)
		{
			(void)key;
			if (attempt.pid == pid && attempt.applied && !attempt.acknowledged)
			{
				if (pending.size() == PLAYER_CRAFT_RECEIPT_MAX)
					return false;
				pending.push_back(attempt.receipt);
			}
		}
		*receipts = std::move(pending);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

void save_completed(uint32_t pid, bool committed, const player_craft_receipt_snapshot *receipts,
		    size_t count)
{
	for (size_t index = 0; receipts && index < count; ++index)
	{
		const auto found = attempts.find(operation_key(receipts[index].operation_id));
		if (found == attempts.end() || found->second.pid != pid ||
		    !receipt_equal(found->second.receipt, receipts[index]))
			continue;
		if (committed)
			found->second.acknowledged = true;
		else
		{
			found->second.save_pending = false;
			found->second.next_save = {};
		}
	}
}

bool recover_receipts(uint32_t pid, const player_craft_receipt_snapshot *receipts, size_t count)
{
	if (!pid || count > PLAYER_CRAFT_RECEIPT_MAX || (count && !receipts))
		return false;
	for (size_t index = 0; index < count; ++index)
	{
		if (!valid_receipt(receipts[index]))
			return false;
		for (size_t prior = 0; prior < index; ++prior)
			if (receipts[prior].operation_id.bytes ==
			    receipts[index].operation_id.bytes)
				return false;
		const auto found = attempts.find(operation_key(receipts[index].operation_id));
		if (found != attempts.end() &&
		    (found->second.pid != pid ||
		     !receipt_equal(found->second.receipt, receipts[index])))
			return false;
	}
	for (const auto &[key, attempt] : attempts)
	{
		(void)key;
		if (attempt.pid != pid || !attempt.applied || attempt.acknowledged)
			continue;
		bool durable = false;
		for (size_t index = 0; index < count; ++index)
			durable = durable || receipt_equal(attempt.receipt, receipts[index]);
		// A live checkpoint can still be in flight. Do not load an older
		// progression image and apply the same award while it commits.
		if (!durable)
			return false;
	}
	try
	{
		for (size_t index = 0; index < count; ++index)
		{
			const auto key = operation_key(receipts[index].operation_id);
			auto found = attempts.find(key);
			if (found != attempts.end())
			{
				if (found->second.pid != pid ||
				    !receipt_equal(found->second.receipt, receipts[index]))
					return false;
				found->second.acknowledged = true;
				continue;
			}
			if (attempts.size() >= MAX_PROGRESSION_ATTEMPTS)
				return false;
			progression_attempt attempt;
			attempt.pid = pid;
			attempt.receipt = receipts[index];
			attempt.applied = true;
			attempt.acknowledged = true;
			attempts.emplace(key, attempt);
		}
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

craft_progression_publication_result publish(const critical_operation_id &operation, P_char actor,
					     const craft_recipe_continuation &terms)
{
	if (!actor || IS_NPC(actor) || GET_PID(actor) <= 0 ||
	    static_cast<uint32_t>(GET_PID(actor)) != terms.player_pid ||
	    terms.experience > INT_MAX ||
	    (terms.discipline != craft_recipe_discipline::craft &&
	     terms.discipline != craft_recipe_discipline::forge &&
	     !craft_recipe_is_alchemy(terms.discipline)))
		return craft_progression_publication_result::failed;
	try
	{
		const player_craft_receipt_snapshot receipt = {
			operation, static_cast<uint32_t>(terms.discipline), terms.experience
		};
		if (!valid_receipt(receipt))
			return craft_progression_publication_result::failed;
		const auto key = operation_key(operation);
		auto found = attempts.find(key);
		if (found == attempts.end())
		{
			if (attempts.size() >= MAX_PROGRESSION_ATTEMPTS)
				return craft_progression_publication_result::waiting;
			progression_attempt attempt;
			attempt.pid = terms.player_pid;
			attempt.receipt = receipt;
			found = attempts.emplace(key, attempt).first;
		}
		auto &attempt = found->second;
		if (attempt.pid != terms.player_pid || !receipt_equal(attempt.receipt, receipt))
			return craft_progression_publication_result::failed;
		if (attempt.acknowledged)
			return craft_progression_publication_result::ready;
		if (!attempt.applied)
		{
			if (terms.frozen_progression &&
			    actor->only.pc->skills[terms.discipline ==
								   craft_recipe_discipline::craft ?
							   SKILL_CRAFT :
							   SKILL_FORGE]
					    .learned !=
				    static_cast<int>(terms.progression_skill_before))
				return craft_progression_publication_result::failed;
			const auto pending =
				std::count_if(attempts.begin(), attempts.end(),
					      [&](const auto &entry)
					      {
						      return entry.second.pid == terms.player_pid &&
							     entry.second.applied &&
							     !entry.second.acknowledged;
					      });
			if (static_cast<size_t>(pending) >= PLAYER_CRAFT_RECEIPT_MAX)
				return craft_progression_publication_result::waiting;
			// Retain identity before gameplay effects. Capture/journal failures can
			// retry the checkpoint without rerolling a notch or granting XP twice.
			attempt.applied = true;
			if (terms.discipline == craft_recipe_discipline::poison)
				skill_notch_apply(actor, SKILL_MIXPOISON, terms.notch);
			else if (terms.frozen_progression)
				skill_notch_apply(actor,
						  terms.discipline ==
								  craft_recipe_discipline::craft ?
							  SKILL_CRAFT :
							  SKILL_FORGE,
						  terms.notch);
			else if (!craft_recipe_is_alchemy(terms.discipline))
				notch_skill(actor,
					    terms.discipline == craft_recipe_discipline::craft ?
						    SKILL_CRAFT :
						    SKILL_FORGE,
					    50);
			if (terms.experience)
				gain_exp(actor, nullptr, static_cast<int>(terms.experience),
					 EXP_BOON);
		}
		const auto now = std::chrono::steady_clock::now();
		if (!attempt.save_pending && now >= attempt.next_save)
		{
			attempt.next_save = now + std::chrono::milliseconds(500);
			const auto result =
				player_save_pipeline_request(actor, CRAFT_PROGRESSION_COMPONENTS,
							     RENT_CRASH, ROOM_VNUM(actor->in_room));
			// Keep the admitted revision stable until its exact completion. Repeated
			// publication pulses must not supersede it before the worker can admit it.
			attempt.save_pending = result == player_save_pipeline_result::queued ||
					       result == player_save_pipeline_result::coalesced;
		}
		return craft_progression_publication_result::waiting;
	}
	catch (const std::bad_alloc &)
	{
		return craft_progression_publication_result::waiting;
	}
}

void acknowledged(const critical_operation_id &operation)
{
	attempts.erase(operation_key(operation));
}

void notify(P_char actor, bool committed, const craft_recipe_continuation &terms)
{
	if (terms.frozen_progression)
	{
		if (!committed)
		{
			send_to_char(
				"Your work could not be committed; your requirements were retained.\r\n",
				actor);
			return;
		}
		for (P_obj output = actor->carrying; output; output = output->next_content)
			if (output->obj_uid == terms.output_uid &&
			    OBJ_VNUM(output) == static_cast<int>(terms.recipe_vnum))
			{
				act("&+W$n &+Lfinishes their work, admiring their new $p.&N", TRUE,
				    actor, output, nullptr, TO_ROOM);
				act("&+WYou &+Lfinish your work, admiring your new $p.&N", FALSE,
				    actor, output, nullptr, TO_CHAR);
				break;
			}
		return;
	}
	if (terms.discipline == craft_recipe_discipline::refine)
	{
		if (!committed)
		{
			send_to_char(
				"The refining attempt could not be committed; your materials, ore and coins were preserved.\r\n",
				actor);
			return;
		}
		send_to_char("You take the item and gently pour the melted ore over the item...\n",
			     actor);
		send_to_char("You take the item and gently pour the melted ore over the item...\n",
			     actor);
		const bool success = craft_refine_succeeded(terms);
		const bool coins = !success && terms.refine_ore_count != 1;
		// The consumed material name is frozen data, passed as a value rather than an
		// act format or a freed object's pointer. Room text needs no temporary object.
		send_to_char_f(
			actor,
			"&+LYou &+Ltake your %s&+L and &+rh&+Rea&+Yt &+Lit in the &+yforge&+L.\r\n"
			"&+LYou &+Lgently remove the &+rm&+Ro&+Ylt&+Re&+rn %s &+Land start to spread it about your %ss&+L, which %s&N\r\n",
			coins ? "&+Wcoins" : "&+yore", coins ? "&+ymetal" : "&+yore",
			terms.refine_material_name.c_str(),
			success ? "&+ycrack &+Land &+yreform&+L under the intense &+rheat&+L." :
				  "&-L&+Rshatters&n &+Lfrom the intense &+rheat&+L!");
		act(success ?
			    "&+W$n &+Ltakes their &+yore&+L to the &+yforge&+L and spreads the molten ore about their materials, which &+ycrack &+Land &+yreform&+L under the intense &+rheat&+L.&N" :
		    coins ? "&+W$n &+Ltakes their &+Wcoins&+L to the &+yforge&+L and spreads the molten metal about their materials, which &-L&+Rshatter&n &+Lfrom the intense &+rheat&+L!&N" :
			    "&+W$n &+Ltakes their &+yore&+L to the &+yforge&+L and spreads the molten ore about their materials, which &-L&+Rshatter&n &+Lfrom the intense &+rheat&+L!&N",
		    TRUE, actor, 0, 0, TO_ROOM);
		return;
	}
	if (!craft_recipe_is_alchemy(terms.discipline))
		return;
	if (!committed)
	{
		send_to_char(
			terms.discipline == craft_recipe_discipline::harvester ?
				"The Harvester's craft could not be committed; your soul shards were preserved.\r\n" :
				"The craft could not be committed; your ingredients were preserved.\r\n",
			actor);
		return;
	}
	switch (terms.discipline)
	{
	case craft_recipe_discipline::poison:
		send_to_char_f(actor, "You finish mixing %u poison%s.\r\n", terms.output_count,
			       terms.output_count == 1 ? "" : "s");
		break;
	case craft_recipe_discipline::encrust:
		act("...creating a real masterpiece!", TRUE, actor, 0, 0, TO_ROOM);
		act("Hurrah! Hurrah!", FALSE, actor, 0, 0, TO_CHAR);
		wizlog(56, "Encrust committed for %s.", GET_NAME(actor));
		break;
	case craft_recipe_discipline::encrust_failure:
		act("You broke your item in the process.", FALSE, actor, 0, 0, TO_CHAR);
		act("...and breaks it in the process.", TRUE, actor, 0, 0, TO_ROOM);
		wizlog(56, "%s ruined an encrust attempt.", GET_NAME(actor));
		break;
	case craft_recipe_discipline::harvester:
		send_to_char(
			"The Harvester accepts the soul shards and gives you a greater orb.\r\n",
			actor);
		break;
	default:
		break;
	}
}
}

void craft_progression_initialize(void)
{
	attempts.clear();
	craft_progression_hooks = { publish,	      pending_receipts, save_completed,
				    recover_receipts, acknowledged,	notify };
}
