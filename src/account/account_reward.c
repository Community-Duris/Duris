#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "item/item_movement_transaction.h"
#include "account/account_reward.h"
#include "account/account_reward_config.h"
#include "account/account_reward_snapshot.h"
#include "net/comm.h"
#include "world/vnum.obj.h"
#ifndef __NO_MYSQL__
#include "sql/sql.h"
#include "sql/sql_player.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

extern P_obj object_list;

struct RewardGrant
{
	unsigned long long id;
	std::string account;
	int vnum;
	int template_version;
	std::string template_json;
	std::string display_name;
	std::string granted_by;
	long long age_seconds;
	long long expires_seconds;
	int remaining_pwipes;
};

struct RewardMarker
{
	unsigned long long grant_id;
	char account[ACCOUNT_REWARD_ACCOUNT_MAX + 1];
	bool legacy;
};

/** Return the authenticated account name, rejecting missing or unknown identities. */
static const char *reward_account(P_char ch)
{
	if (!ch)
		return NULL;
	const char *account = get_account_name_safe(ch);
	if (!account || !*account || !strcasecmp(account, "Unknown"))
		return NULL;
	return account;
}

static bool parse_reward_marker(P_obj obj, RewardMarker *parsed)
{
	const char *marker;
	unsigned long long id;
	char owner[ACCOUNT_REWARD_ACCOUNT_MAX + 1];
	if (!obj || !obj->name || !parsed)
		return false;
	marker = strstr(obj->name, ACCOUNT_REWARD_MARKER);
	if (!marker)
		return false;
	marker += strlen(ACCOUNT_REWARD_MARKER);
	if (sscanf(marker, "%llu:%50s", &id, owner) == 2 && id > 0)
	{
		parsed->grant_id = id;
		snprintf(parsed->account, sizeof(parsed->account), "%s", owner);
		parsed->legacy = false;
		return true;
	}
	if (sscanf(marker, "%50s", owner) == 1)
	{
		parsed->grant_id = 0;
		snprintf(parsed->account, sizeof(parsed->account), "%s", owner);
		parsed->legacy = true;
		return true;
	}
	return false;
}

#ifndef __NO_MYSQL__
static bool reward_marker_matches(P_obj obj, const char *account, unsigned long long grant_id)
{
	RewardMarker marker;
	if (!account || !parse_reward_marker(obj, &marker) || strcasecmp(marker.account, account))
		return false;
	return grant_id == 0 ? marker.legacy : (!marker.legacy && marker.grant_id == grant_id);
}

/** Match a grant ID, or the legacy account marker and reward prototype. */
static bool grant_marker_matches(P_obj obj, const RewardGrant &grant)
{
	if (reward_marker_matches(obj, grant.account.c_str(), grant.id))
		return true;
	return grant.template_version == 0 &&
	       reward_marker_matches(obj, grant.account.c_str(), 0) && OBJ_VNUM(obj) == grant.vnum;
}
#endif

/** Match a flagged reward marker to the owning account, including legacy markers. */
bool account_bound_reward_owner(P_char ch, P_obj obj)
{
	RewardMarker marker;
	const char *account = reward_account(ch);
	return obj && IS_OBJ_STAT2(obj, ITEM2_ACCOUNT_BOUND) && account &&
	       parse_reward_marker(obj, &marker) && !strcasecmp(marker.account, account);
}

#ifndef __NO_MYSQL__
static P_char reward_item_owner(P_obj obj)
{
	P_obj top = obj;
	while (top && OBJ_INSIDE(top))
		top = top->loc.inside;
	if (!top)
		return NULL;
	if (OBJ_CARRIED(top))
		return top->loc.carrying;
	if (OBJ_WORN(top))
		return top->loc.wearing;
	return NULL;
}

static const char *const PRETTY_BINDING_TAG =
	"&+W[&+CD&+Bi&+Wv&+Yi&+Cn&+Be&+Wl&+Yy &+CB&+Bo&+Wu&+Yn&+Cd&+W]&n";
static const char *const LEGACY_BINDING_TAG = " &+L(soulbound)&n";

static bool beautify_reward_item(P_obj obj)
{
	if (!obj || !obj->short_description || strstr(obj->short_description, PRETTY_BINDING_TAG))
		return false;
	std::string description = obj->short_description;
	size_t legacy = description.find(LEGACY_BINDING_TAG);
	if (legacy != std::string::npos)
		description.erase(legacy, strlen(LEGACY_BINDING_TAG));
	description += " ";
	description += PRETTY_BINDING_TAG;
	set_short_description(obj, description.c_str());
	return true;
}

static void mark_reward_item(P_obj obj, const char *account, unsigned long long grant_id)
{
	char name[MAX_STRING_LENGTH];
	if (!obj || !account || grant_id == 0)
		return;
	snprintf(name, sizeof(name), "%s%llu:%s %s", ACCOUNT_REWARD_MARKER, grant_id, account,
		 obj->name ? obj->name : "reward");
	if ((obj->str_mask & STRUNG_KEYS) && obj->name)
		str_free(obj->name);
	obj->str_mask |= STRUNG_KEYS;
	obj->name = str_dup(name);
	beautify_reward_item(obj);
	SET_BIT(obj->extra_flags, ITEM_NOSELL | ITEM_NORENT | ITEM_NODROP | ITEM_NOREPAIR);
	SET_BIT(obj->extra2_flags, ITEM2_SOULBIND | ITEM2_ACCOUNT_BOUND | ITEM2_NOLOOT);
	REMOVE_BIT(obj->extra_flags, ITEM_SECRET | ITEM_INVISIBLE);
}

static bool promote_reward_contents(P_obj container)
{
	if (!container || !container->contains)
		return true;

	P_obj parent = OBJ_INSIDE(container) ? container->loc.inside : NULL;
	P_char owner = OBJ_CARRIED(container) ?
			       container->loc.carrying :
			       (OBJ_WORN(container) ? container->loc.wearing : NULL);
	int room = OBJ_ROOM(container) ? container->loc.room : NOWHERE;
	if (!parent && !owner && room == NOWHERE)
		return false;

	/* obj_to_char intentionally extracts CRUMBLELOOT for ordinary PCs. Refuse
       the whole promotion before moving any sibling rather than dereference a
       freed child or silently destroy container contents. */
	if (owner && IS_PC(owner) && !IS_TRUSTED(owner))
		for (P_obj child = container->contains; child; child = child->next_content)
			if (IS_OBJ_STAT2(child, ITEM2_CRUMBLELOOT))
				return false;

	while (container->contains)
	{
		P_obj child = container->contains;
		/* obj_to_room may merge and extract a coin source after transferring
           its value into an existing pile; that is a successful placement. */
		bool room_coin_merge = room != NOWHERE && OBJ_VNUM(child) == VOBJ_COINS;
		obj_from_obj(child);
		if (parent)
			obj_to_obj_at_end(child, parent);
		else if (owner)
			obj_to_char(child, owner);
		else
			obj_to_room(child, room);

		if (room_coin_merge)
			continue;
		bool placed = (parent && OBJ_INSIDE(child) && child->loc.inside == parent) ||
			      (owner && OBJ_CARRIED(child) && child->loc.carrying == owner) ||
			      (room != NOWHERE && OBJ_ROOM(child) && child->loc.room == room);
		if (!placed)
		{
			if (OBJ_INSIDE(child))
				obj_from_obj(child);
			else if (OBJ_CARRIED(child))
				obj_from_char(child);
			else if (OBJ_ROOM(child))
				obj_from_room(child);
			obj_to_obj_at_end(child, container);
			return false;
		}
	}
	return true;
}

static std::string human_duration(long long seconds, bool round_up = false)
{
	char text[96];
	if (seconds < 0)
		seconds = 0;
	if (seconds >= 86400)
	{
		long long value = round_up ? (seconds + 86399) / 86400 : seconds / 86400;
		snprintf(text, sizeof(text), "%lld day%s", value, value == 1 ? "" : "s");
	}
	else if (seconds >= 3600)
	{
		long long value = round_up ? (seconds + 3599) / 3600 : seconds / 3600;
		snprintf(text, sizeof(text), "%lld hour%s", value, value == 1 ? "" : "s");
	}
	else if (seconds >= 60)
	{
		long long value = round_up ? (seconds + 59) / 60 : seconds / 60;
		snprintf(text, sizeof(text), "%lld minute%s", value, value == 1 ? "" : "s");
	}
	else
		snprintf(text, sizeof(text), "%lld second%s", seconds, seconds == 1 ? "" : "s");
	return text;
}

static std::string cooldown_countdown(long long seconds)
{
	char text[96];
	if (seconds <= 0)
		return "ready now";
	long long minutes = (seconds + 59) / 60;
	snprintf(text, sizeof(text), "%lld real-world minute%s", minutes, minutes == 1 ? "" : "s");
	return text;
}

static std::string lifetime_text(const RewardGrant &grant)
{
	if (grant.remaining_pwipes > 0)
	{
		char text[96];
		snprintf(text, sizeof(text), "after %d more player wipe%s", grant.remaining_pwipes,
			 grant.remaining_pwipes == 1 ? "" : "s");
		return text;
	}
	if (grant.expires_seconds >= 0)
		return "in " + human_duration(grant.expires_seconds, true);
	return "never (permanent)";
}
#endif

#ifndef __NO_MYSQL__
static std::string escape_sql(const char *value)
{
	if (!value)
		return std::string();
	std::vector<char> escaped(strlen(value) * 2 + 1);
	unsigned long length = mysql_real_escape_string(DB, escaped.data(), value, strlen(value));
	return std::string(escaped.data(), length);
}

static bool canonical_account(const char *requested, std::string *canonical)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	std::string escaped;
	if (!requested || !*requested || !canonical)
		return false;
	escaped = escape_sql(requested);
	res = db_query(
		"SELECT account_name FROM accounts WHERE LOWER(account_name)=LOWER('%s') LIMIT 1",
		escaped.c_str());
	if (!res)
		return false;
	row = mysql_fetch_row(res);
	if (row && row[0])
		*canonical = row[0];
	mysql_free_result(res);
	return row != NULL;
}

static std::vector<RewardGrant> query_grants(const char *account, bool expired_only,
					     bool *query_ok = NULL)
{
	std::vector<RewardGrant> grants;
	MYSQL_RES *res;
	MYSQL_ROW row;
	if (query_ok)
		*query_ok = false;
	std::string where =
		expired_only ?
			"expires_at IS NOT NULL AND expires_at<=NOW()" :
			"(expires_at IS NULL OR expires_at>NOW()) AND (remaining_pwipes IS NULL OR remaining_pwipes>0)";
	if (account && *account)
		where += " AND account_name='" + escape_sql(account) + "'";
	res = db_query(
		"SELECT id,account_name,reward_vnum,template_version,template_json,display_name,granted_by,"
		"GREATEST(0,TIMESTAMPDIFF(SECOND,created_at,NOW())),"
		"CASE WHEN expires_at IS NULL THEN -1 ELSE GREATEST(0,TIMESTAMPDIFF(SECOND,NOW(),expires_at)) END,"
		"COALESCE(remaining_pwipes,0) FROM account_bound_rewards WHERE %s ORDER BY account_name,id",
		where.c_str());
	if (!res)
		return grants;
	if (query_ok)
		*query_ok = true;
	while ((row = mysql_fetch_row(res)) != NULL)
	{
		RewardGrant grant;
		grant.id = row[0] ? strtoull(row[0], NULL, 10) : 0;
		grant.account = row[1] ? row[1] : "";
		grant.vnum = row[2] ? atoi(row[2]) : 0;
		grant.template_version = row[3] ? atoi(row[3]) : 0;
		grant.template_json = row[4] ? row[4] : "";
		grant.display_name = row[5] ? row[5] : "";
		grant.granted_by = row[6] ? row[6] : "";
		grant.age_seconds = row[7] ? strtoll(row[7], NULL, 10) : 0;
		grant.expires_seconds = row[8] ? strtoll(row[8], NULL, 10) : -1;
		grant.remaining_pwipes = row[9] ? atoi(row[9]) : 0;
		if (grant.id > 0 && grant.vnum > 0)
			grants.push_back(grant);
	}
	mysql_free_result(res);
	return grants;
}

static bool clear_saved_grant(const RewardGrant &grant)
{
	char stable_marker[256];
	snprintf(stable_marker, sizeof(stable_marker), "%s%llu:%s ", ACCOUNT_REWARD_MARKER,
		 grant.id, grant.account.c_str());
	std::string stable_q = escape_sql(stable_marker);
	if (grant.template_version == 0)
	{
		char legacy_marker[256];
		snprintf(legacy_marker, sizeof(legacy_marker), "%s%s ", ACCOUNT_REWARD_MARKER,
			 grant.account.c_str());
		std::string legacy_q = escape_sql(legacy_marker),
			    account_q = escape_sql(grant.account.c_str());
		if (!qry("UPDATE player_items child JOIN player_items reward ON child.container_id=reward.id JOIN player_data pd ON pd.pid=reward.pid SET child.container_id=reward.container_id WHERE (pd.account_name='%s' OR EXISTS (SELECT 1 FROM account_characters ac WHERE ac.char_name=pd.name AND ac.account_name='%s')) AND (LEFT(reward.name,CHAR_LENGTH('%s'))='%s' OR LEFT(reward.name,CHAR_LENGTH('%s'))='%s') AND reward.vnum=%d",
			 account_q.c_str(), account_q.c_str(), stable_q.c_str(), stable_q.c_str(),
			 legacy_q.c_str(), legacy_q.c_str(), grant.vnum))
			return false;
		return qry(
			"DELETE pi FROM player_items pi JOIN player_data pd ON pd.pid=pi.pid WHERE (pd.account_name='%s' OR EXISTS (SELECT 1 FROM account_characters ac WHERE ac.char_name=pd.name AND ac.account_name='%s')) AND (LEFT(pi.name,CHAR_LENGTH('%s'))='%s' OR LEFT(pi.name,CHAR_LENGTH('%s'))='%s') AND pi.vnum=%d",
			account_q.c_str(), account_q.c_str(), stable_q.c_str(), stable_q.c_str(),
			legacy_q.c_str(), legacy_q.c_str(), grant.vnum);
	}
	if (!qry("UPDATE player_items child JOIN player_items reward ON child.container_id=reward.id SET child.container_id=reward.container_id WHERE LEFT(reward.name,CHAR_LENGTH('%s'))='%s'",
		 stable_q.c_str(), stable_q.c_str()))
		return false;
	return qry("DELETE FROM player_items WHERE LEFT(name,CHAR_LENGTH('%s'))='%s'",
		   stable_q.c_str(), stable_q.c_str());
}

static void revoke_live_grant(const RewardGrant &grant)
{
	std::vector<P_char> changed;
	for (P_obj obj = object_list, next; obj; obj = next)
	{
		next = obj->next;
		bool match = grant_marker_matches(obj, grant);
		if (!match)
			continue;
		P_char owner = reward_item_owner(obj);
		if (!promote_reward_contents(obj))
		{
			logit(LOG_WIZ,
			      "divineclaim: could not safely release contents while revoking reward #%llu",
			      grant.id);
			continue;
		}
		extract_obj(obj);
		if (owner)
		{
			bool known = false;
			for (P_char prior : changed)
				if (prior == owner)
					known = true;
			if (!known)
				changed.push_back(owner);
		}
	}
	for (P_char owner : changed)
		if (!do_save_silent(owner, 1))
			logit(LOG_WIZ, "divineclaim: failed to save revoked reward owner %s",
			      GET_NAME(owner));
}

static void purge_expired_grants(void)
{
	bool lookup_ok = false;
	std::vector<RewardGrant> expired = query_grants(NULL, true, &lookup_ok);
	if (!lookup_ok)
	{
		logit(LOG_WIZ, "divineclaim: expired grant lookup failed");
		return;
	}
	for (const RewardGrant &grant : expired)
	{
		if (!sql_begin_transaction())
		{
			logit(LOG_WIZ, "divineclaim: failed to begin expiry cleanup for grant %llu",
			      grant.id);
			continue;
		}
		if (!clear_saved_grant(grant) ||
		    !qry("DELETE FROM account_bound_rewards WHERE id=%llu", grant.id))
		{
			sql_rollback();
			logit(LOG_WIZ,
			      "divineclaim: failed to purge expired grant %llu; grant retained for retry",
			      grant.id);
			continue;
		}
		if (!sql_commit())
		{
			sql_rollback();
			logit(LOG_WIZ,
			      "divineclaim: failed to commit expiry cleanup for grant %llu",
			      grant.id);
			continue;
		}
		revoke_live_grant(grant);
	}
}

static long long cooldown_remaining(unsigned long long grant_id, int pid)
{
	MYSQL_RES *res = db_query(
		"SELECT CASE WHEN recovery_ready<>0 THEN 0 ELSE GREATEST(0,%d-TIMESTAMPDIFF(SECOND,last_summoned_at,NOW())) END FROM account_bound_reward_summons WHERE grant_id=%llu AND pid=%d",
		account_reward_config_cooldown_seconds(), grant_id, pid);
	if (!res)
		return -1;
	MYSQL_ROW row = mysql_fetch_row(res);
	long long remaining = (row && row[0]) ? strtoll(row[0], NULL, 10) : 0;
	mysql_free_result(res);
	return remaining;
}

static P_obj existing_character_instance(P_char ch, const RewardGrant &grant,
					 bool remove_duplicates)
{
	P_obj keep = NULL;
	for (P_obj obj = object_list, next; obj; obj = next)
	{
		next = obj->next;
		bool match = grant_marker_matches(obj, grant);
		if (!match || reward_item_owner(obj) != ch)
			continue;
		if (!keep)
			keep = obj;
		else if (remove_duplicates)
		{
			if (promote_reward_contents(obj))
				extract_obj(obj);
			else
				logit(LOG_WIZ,
				      "divineclaim: duplicate reward #%llu retained because its contents could not be released safely",
				      grant.id);
		}
	}
	return keep;
}

static bool summon_one(P_char ch, const RewardGrant &grant, bool explain)
{
	P_obj existing = existing_character_instance(ch, grant, true);
	const char *name = grant.display_name.empty() ? "a divine reward" :
							grant.display_name.c_str();
	if (existing)
	{
		if (beautify_reward_item(existing) && !do_save_silent(ch, 1))
			logit(LOG_WIZ,
			      "divineclaim: failed to save updated binding label for reward #%llu on %s",
			      grant.id, GET_NAME(ch));
		if (explain)
			send_to_char_f(
				ch,
				"Your divine account reward is already with this character: %s.&n\r\n",
				existing->short_description ? existing->short_description : name);
		return false;
	}
	long long remaining = cooldown_remaining(grant.id, GET_PID(ch));
	if (remaining < 0)
	{
		logit(LOG_WIZ, "divineclaim: cooldown lookup failed for grant %llu pid %d",
		      grant.id, GET_PID(ch));
		if (explain)
			send_to_char_f(
				ch,
				"The divine records for %s are temporarily unavailable. Nothing was created or changed; please try again later.\r\n",
				name);
		return false;
	}
	if (remaining > 0)
	{
		if (explain)
			send_to_char_f(
				ch,
				"Your divine account reward %s is still recovering. You may summon it again in %s.\r\n",
				name, cooldown_countdown(remaining).c_str());
		return false;
	}
	P_obj obj = read_object(grant.vnum, VIRTUAL);
	if (!obj)
	{
		logit(LOG_WIZ, "divineclaim: vnum %d could not be loaded for grant %llu",
		      grant.vnum, grant.id);
		return false;
	}
	if (grant.template_version > 0 &&
	    !account_reward_snapshot_apply(obj, grant.template_json.c_str(),
					   grant.template_version))
	{
		logit(LOG_WIZ, "divineclaim: exact template could not be applied for grant %llu",
		      grant.id);
		extract_obj(obj);
		return false;
	}
	if (obj->type == ITEM_CONTAINER)
		REMOVE_BIT(obj->value[1], CONT_CLOSED);
	mark_reward_item(obj, grant.account.c_str(), grant.id);
	if (!qry("INSERT INTO account_bound_reward_summons(grant_id,pid,last_summoned_at,recovery_ready) VALUES(%llu,%d,NOW(),0) ON DUPLICATE KEY UPDATE last_summoned_at=NOW(),recovery_ready=0",
		 grant.id, GET_PID(ch)))
	{
		extract_obj(obj, FALSE);
		logit(LOG_WIZ, "divineclaim: failed to reserve grant %llu for %s", grant.id,
		      GET_NAME(ch));
		return false;
	}
	if (!item_creation_grant_submit_to_player(ch, obj, ch))
	{
		(void)qry(
			"UPDATE account_bound_reward_summons SET recovery_ready=1 WHERE grant_id=%llu AND pid=%d",
			grant.id, GET_PID(ch));
		extract_obj(obj, FALSE);
		logit(LOG_WIZ, "divineclaim: failed to submit grant %llu for %s", grant.id,
		      GET_NAME(ch));
		return false;
	}
	if (explain)
	{
		send_to_char("\r\n", ch);
		send_to_char_f(ch, "A divine account reward begins to materialize: %s.&n\r\n",
			       obj->short_description ? obj->short_description : name);
		send_to_char_f(
			ch,
			"It is bound to account %s. Each character on that account may summon one copy.\r\n",
			grant.account.c_str());
		send_to_char_f(
			ch,
			"If it is lost, DIVINECLAIM can restore it after %s. This grant expires %s.\r\n\r\n",
			cooldown_countdown(account_reward_config_cooldown_seconds()).c_str(),
			lifetime_text(grant).c_str());
	}
	return true;
}

static bool parse_positive(const char *text, int *value);

static bool player_grants(P_char ch, std::vector<RewardGrant> *grants)
{
	const char *account = reward_account(ch);
	if (!account || !grants)
		return false;
	purge_expired_grants();
	bool lookup_ok = false;
	*grants = query_grants(account, false, &lookup_ok);
	if (!lookup_ok)
	{
		logit(LOG_WIZ, "divineclaim: grant lookup failed for account %s", account);
		send_to_char(
			"The divine account-reward records are temporarily unavailable. Nothing was created or changed; please try again later.\r\n",
			ch);
		return false;
	}
	return true;
}

static const char *player_instance_status(P_char ch, const RewardGrant &grant)
{
	P_obj instance = existing_character_instance(ch, grant, false);
	if (!instance)
		return NULL;
	if (beautify_reward_item(instance) && !do_save_silent(ch, 1))
		logit(LOG_WIZ,
		      "divineclaim: failed to save updated binding label for reward #%llu on %s",
		      grant.id, GET_NAME(ch));
	P_obj top = instance;
	while (top && OBJ_INSIDE(top))
		top = top->loc.inside;
	return top && OBJ_WORN(top) ? "Equipped" : "Carried";
}

static void player_divineclaim_help(P_char ch)
{
	send_to_char("Divine reward commands:\r\n"
		     "  divineclaim list\r\n"
		     "  divineclaim summon <number>\r\n"
		     "  divineclaim dismiss <number>\r\n",
		     ch);
}

static void list_player_grants(P_char ch)
{
	std::vector<RewardGrant> grants;
	if (!player_grants(ch, &grants))
		return;
	if (grants.empty())
	{
		send_to_char("Your account has no active divine rewards.\r\n", ch);
		player_divineclaim_help(ch);
		return;
	}
	send_to_char("Your divine account rewards:\r\n", ch);
	for (size_t i = 0; i < grants.size(); ++i)
	{
		const RewardGrant &grant = grants[i];
		const char *present = player_instance_status(ch, grant);
		std::string status;
		if (present)
			status = present;
		else
		{
			long long remaining = cooldown_remaining(grant.id, GET_PID(ch));
			if (remaining < 0)
				status = "Status unavailable";
			else if (remaining > 0)
				status = "Recovering: " + cooldown_countdown(remaining);
			else
				status = "Ready";
		}
		std::string claim = account_reward_config_show_claim_ids() ?
					    " [claim #" + std::to_string(grant.id) + "]" :
					    "";
		send_to_char_f(ch, "  %zu. &+W[%s]&n %s — expires %s%s\r\n", i + 1, status.c_str(),
			       grant.display_name.empty() ? "a divine reward" :
							    grant.display_name.c_str(),
			       lifetime_text(grant).c_str(), claim.c_str());
	}
	player_divineclaim_help(ch);
}

static bool resolve_player_grant(P_char ch, const char *number, RewardGrant *selected)
{
	int index = 0;
	if (!parse_positive(number, &index))
	{
		send_to_char(
			"Choose a reward using the positive list number shown by DIVINECLAIM LIST.\r\n",
			ch);
		return false;
	}
	std::vector<RewardGrant> grants;
	if (!player_grants(ch, &grants))
		return false;
	if ((size_t)index > grants.size())
	{
		send_to_char(
			"That reward number is not active. Use DIVINECLAIM LIST for the current numbered list.\r\n",
			ch);
		return false;
	}
	*selected = grants[(size_t)index - 1];
	return true;
}

static int active_player_grant_count(P_char ch, unsigned long long selected_id,
				     bool *selected_active)
{
	if (selected_active)
		*selected_active = false;
	std::vector<RewardGrant> grants;
	if (!player_grants(ch, &grants))
		return -1;
	int active_count = 0;
	for (const RewardGrant &grant : grants)
	{
		if (!existing_character_instance(ch, grant, false))
			continue;
		++active_count;
		if (selected_active && grant.id == selected_id)
			*selected_active = true;
	}
	return active_count;
}

static bool active_reward_capacity_allows(P_char ch, const RewardGrant &selected)
{
	int active_limit = account_reward_config_max_active_rewards();
	if (active_limit == 0)
		return true;
	bool selected_active = false;
	int active_count = active_player_grant_count(ch, selected.id, &selected_active);
	if (active_count < 0)
		return false;
	if (selected_active)
		return true;
	if (active_count >= active_limit)
	{
		send_to_char_f(
			ch,
			"You already have the configured maximum of %d active divine reward%s. Dismiss one before summoning another.\r\n",
			active_limit, active_limit == 1 ? "" : "s");
		return false;
	}
	return true;
}

static void dismiss_player_grant(P_char ch, const RewardGrant &selected)
{
	P_obj instance = existing_character_instance(ch, selected, true);
	if (!instance)
	{
		send_to_char(
			"That divine reward is not currently carried or equipped by this character.\r\n",
			ch);
		return;
	}
	if (instance->contains)
	{
		send_to_char(
			"You cannot dismiss that divinely bound container while it contains items. Empty it first, then try again.\r\n",
			ch);
		return;
	}
	std::string name = instance->short_description ?
				   instance->short_description :
				   (selected.display_name.empty() ? "your divine reward" :
								    selected.display_name);
	extract_obj(instance);
	bool saved = do_save_silent(ch, 1);
	long long remaining = cooldown_remaining(selected.id, GET_PID(ch));
	send_to_char_f(
		ch,
		"You release %s.&n It remains on your account and is no longer carried or equipped.\r\n",
		name.c_str());
	if (remaining > 0)
		send_to_char_f(ch, "It may be summoned again in %s.\r\n",
			       cooldown_countdown(remaining).c_str());
	else if (remaining == 0)
		send_to_char("It is ready to summon again now.\r\n", ch);
	else
		send_to_char("Its recovery countdown is temporarily unavailable.\r\n", ch);
	if (!saved)
		logit(LOG_WIZ, "divineclaim: failed to save dismissed reward #%llu for %s",
		      selected.id, GET_NAME(ch));
}

static void player_divineclaim(P_char ch, char *argument)
{
	char action[MAX_INPUT_LENGTH], number[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
	char *rest = argument;
	rest = one_argument(rest, action);
	rest = one_argument(rest, number);
	one_argument(rest, extra);
	if (!*action || (!strcasecmp(action, "list") && !*number))
	{
		list_player_grants(ch);
		return;
	}
	if ((!strcasecmp(action, "summon") || !strcasecmp(action, "dismiss")) && *number && !*extra)
	{
		RewardGrant selected;
		if (!resolve_player_grant(ch, number, &selected))
			return;
		if (!strcasecmp(action, "dismiss"))
			dismiss_player_grant(ch, selected);
		else
		{
			if (!active_reward_capacity_allows(ch, selected))
				return;
			if (summon_one(ch, selected, true) && !do_save_silent(ch, 1))
				logit(LOG_WIZ,
				      "divineclaim: failed to save summoned reward #%llu for %s",
				      selected.id, GET_NAME(ch));
		}
		return;
	}
	player_divineclaim_help(ch);
}

static bool summon_login_reward(P_char ch)
{
	std::vector<RewardGrant> grants;
	if (!player_grants(ch, &grants) || grants.empty())
		return false;
	const char *account = reward_account(ch);
	std::string account_q = escape_sql(account);
	MYSQL_RES *res = db_query(
		"SELECT abr.id FROM account_bound_rewards abr LEFT JOIN account_bound_reward_summons s "
		"ON s.grant_id=abr.id AND s.pid=%d WHERE abr.account_name='%s' "
		"AND (abr.expires_at IS NULL OR abr.expires_at>NOW()) "
		"AND (abr.remaining_pwipes IS NULL OR abr.remaining_pwipes>0) "
		"ORDER BY s.last_summoned_at IS NULL, s.last_summoned_at DESC, abr.id LIMIT 1",
		GET_PID(ch), account_q.c_str());
	if (!res)
	{
		logit(LOG_WIZ, "divineclaim: login reward selection failed for %s", GET_NAME(ch));
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(res);
	unsigned long long selected_id = (row && row[0]) ? strtoull(row[0], NULL, 10) : 0;
	mysql_free_result(res);
	RewardGrant selected = grants.front();
	for (const RewardGrant &grant : grants)
		if (grant.id == selected_id)
		{
			selected = grant;
			break;
		}
	if (!active_reward_capacity_allows(ch, selected))
		return false;
	bool summoned = summon_one(ch, selected, true);
	if (summoned && !do_save_silent(ch, 1))
		logit(LOG_WIZ, "divineclaim: failed to save login reward #%llu for %s", selected.id,
		      GET_NAME(ch));
	return summoned;
}

static bool parse_positive(const char *text, int *value)
{
	char *end = NULL;
	long parsed;
	if (!text || !*text)
		return false;
	errno = 0;
	parsed = strtol(text, &end, 10);
	if (errno || !end || *end != '\0' || parsed <= 0 || parsed > INT_MAX)
		return false;
	*value = (int)parsed;
	return true;
}

static bool insert_exact_grant(P_char ch, P_obj source, const std::string &account,
			       const char *mode, int amount, unsigned long long *new_id)
{
	char *json = account_reward_snapshot_serialize(source);
	if (!json)
		return false;
	std::string account_q = escape_sql(account.c_str()), granter_q = escape_sql(GET_NAME(ch)),
		    json_q = escape_sql(json),
		    display_q = escape_sql(source->short_description ? source->short_description :
								       "a divine reward");
	free(json);
	bool ok;
	if (!strcasecmp(mode, "days"))
		ok = qry(
			"INSERT INTO account_bound_rewards(account_name,reward_vnum,template_version,template_json,display_name,granted_by,expires_at,remaining_pwipes) VALUES('%s',%d,%d,'%s','%s','%s',NOW()+INTERVAL %d DAY,NULL)",
			account_q.c_str(), OBJ_VNUM(source), ACCOUNT_REWARD_TEMPLATE_VERSION,
			json_q.c_str(), display_q.c_str(), granter_q.c_str(), amount);
	else if (!strcasecmp(mode, "wipes"))
		ok = qry(
			"INSERT INTO account_bound_rewards(account_name,reward_vnum,template_version,template_json,display_name,granted_by,expires_at,remaining_pwipes) VALUES('%s',%d,%d,'%s','%s','%s',NULL,%d)",
			account_q.c_str(), OBJ_VNUM(source), ACCOUNT_REWARD_TEMPLATE_VERSION,
			json_q.c_str(), display_q.c_str(), granter_q.c_str(), amount);
	else
		ok = qry(
			"INSERT INTO account_bound_rewards(account_name,reward_vnum,template_version,template_json,display_name,granted_by,expires_at,remaining_pwipes) VALUES('%s',%d,%d,'%s','%s','%s',NULL,NULL)",
			account_q.c_str(), OBJ_VNUM(source), ACCOUNT_REWARD_TEMPLATE_VERSION,
			json_q.c_str(), display_q.c_str(), granter_q.c_str());
	if (ok && new_id)
		*new_id = mysql_insert_id(DB);
	return ok && (!new_id || *new_id > 0);
}

static bool assign_legacy_grant(const std::string &account, const char *granter, int vnum,
				unsigned long long *id)
{
	std::string account_q = escape_sql(account.c_str()), granter_q = escape_sql(granter);
	MYSQL_RES *res = db_query(
		"SELECT id FROM account_bound_rewards WHERE account_name='%s' AND reward_vnum=%d AND template_version=0 ORDER BY id LIMIT 1",
		account_q.c_str(), vnum);
	if (!res)
		return false;
	MYSQL_ROW row = mysql_fetch_row(res);
	unsigned long long existing = (row && row[0]) ? strtoull(row[0], NULL, 10) : 0;
	mysql_free_result(res);
	if (existing)
	{
		if (!qry("UPDATE account_bound_rewards SET granted_by='%s',updated_at=CURRENT_TIMESTAMP WHERE id=%llu",
			 granter_q.c_str(), existing))
			return false;
		if (id)
			*id = existing;
		return true;
	}
	if (!qry("INSERT INTO account_bound_rewards(account_name,reward_vnum,template_version,display_name,granted_by) VALUES('%s',%d,0,'','%s')",
		 account_q.c_str(), vnum, granter_q.c_str()))
		return false;
	if (id)
		*id = mysql_insert_id(DB);
	return !id || *id > 0;
}

static bool list_grants(P_char ch, const char *account)
{
	purge_expired_grants();
	bool lookup_ok = false;
	std::vector<RewardGrant> grants = query_grants(account, false, &lookup_ok);
	if (!lookup_ok)
	{
		logit(LOG_WIZ, "divineclaim: staff grant listing query failed");
		send_to_char(
			"The divine account-reward records are temporarily unavailable. Nothing was changed.\r\n",
			ch);
		return true;
	}
	if (grants.empty())
		return false;
	send_to_char("Active divine account rewards:\r\n", ch);
	for (const RewardGrant &grant : grants)
	{
		MYSQL_RES *res = db_query(
			"SELECT GROUP_CONCAT(CONCAT(pd.name,' (',GREATEST(0,TIMESTAMPDIFF(SECOND,s.last_summoned_at,NOW())),'s ago)') ORDER BY pd.name SEPARATOR ', ') FROM account_bound_reward_summons s LEFT JOIN player_data pd ON pd.pid=s.pid WHERE s.grant_id=%llu",
			grant.id);
		MYSQL_ROW row = res ? mysql_fetch_row(res) : NULL;
		std::string instances = res ? ((row && row[0] && *row[0]) ? row[0] : "none yet") :
					      "unavailable";
		if (!res)
			logit(LOG_WIZ, "divineclaim: instance listing failed for grant %llu",
			      grant.id);
		if (res)
			mysql_free_result(res);
		send_to_char_f(
			ch,
			"  #%llu account=%s reward=%s (vnum %d) granted_by=%s granted=%s ago expires=%s instances=%s\r\n",
			grant.id, grant.account.c_str(),
			grant.display_name.empty() ? "legacy vnum reward" :
						     grant.display_name.c_str(),
			grant.vnum, grant.granted_by.empty() ? "unknown" : grant.granted_by.c_str(),
			human_duration(grant.age_seconds).c_str(), lifetime_text(grant).c_str(),
			instances.c_str());
	}
	return true;
}

static std::vector<RewardGrant> grants_for_removal(const char *account, int vnum, bool all,
						   unsigned long long id, bool *lookup_ok)
{
	if (lookup_ok)
		*lookup_ok = false;
	if (id > 0)
	{
		bool ok = false;
		std::vector<RewardGrant> all_grants = query_grants(NULL, false, &ok), result;
		if (lookup_ok)
			*lookup_ok = ok;
		for (const RewardGrant &g : all_grants)
			if (g.id == id)
				result.push_back(g);
		return result;
	}
	bool ok = false;
	std::vector<RewardGrant> candidates = query_grants(account, false, &ok), result;
	if (lookup_ok)
		*lookup_ok = ok;
	for (const RewardGrant &g : candidates)
		if (all || g.vnum == vnum)
			result.push_back(g);
	return result;
}

static int remove_grants(const std::vector<RewardGrant> &grants)
{
	std::vector<RewardGrant> revoked;
	if (!sql_begin_transaction())
		return -1;
	for (const RewardGrant &grant : grants)
	{
		if (!clear_saved_grant(grant))
		{
			sql_rollback();
			return -1;
		}
		if (!qry("DELETE FROM account_bound_rewards WHERE id=%llu", grant.id))
		{
			sql_rollback();
			return -1;
		}
		if (mysql_affected_rows(DB) > 0)
			revoked.push_back(grant);
	}
	if (!sql_commit())
	{
		sql_rollback();
		return -1;
	}
	for (const RewardGrant &grant : revoked)
		revoke_live_grant(grant);
	return (int)revoked.size();
}
#endif

bool account_bound_rewards_on_successful_pwipe(void)
{
#ifdef __NO_MYSQL__
	return true;
#else
	if (!sql_begin_transaction())
		return false;
	if (!qry("INSERT INTO account_bound_reward_pwipe_state(id,last_processed_at) VALUES (1,NULL) ON DUPLICATE KEY UPDATE id=VALUES(id)"))
	{
		sql_rollback();
		return false;
	}

	MYSQL_RES *guard_result = db_query(
		"SELECT CASE WHEN last_processed_at IS NULL OR last_processed_at <= NOW() - INTERVAL 28 DAY THEN 1 ELSE 0 END "
		"FROM account_bound_reward_pwipe_state WHERE id=1 FOR UPDATE");
	if (!guard_result)
	{
		sql_rollback();
		return false;
	}
	MYSQL_ROW guard_row = mysql_fetch_row(guard_result);
	bool has_guard_row = guard_row && guard_row[0];
	bool should_process = has_guard_row && atoi(guard_row[0]) == 1;
	mysql_free_result(guard_result);
	if (!has_guard_row)
	{
		sql_rollback();
		return false;
	}
	if (!should_process)
	{
		if (!sql_commit())
		{
			sql_rollback();
			return false;
		}
		logit(LOG_DEBUG,
		      "account rewards: successful pwipe already processed within the last 28 days; duplicate policy invocation ignored");
		return true;
	}

	bool ok = qry("DELETE FROM account_bound_reward_summons");
	if (ok && account_reward_config_preserve_on_pwipe())
	{
		ok = qry(
			"UPDATE account_bound_rewards SET remaining_pwipes = remaining_pwipes - 1 WHERE remaining_pwipes IS NOT NULL AND remaining_pwipes > 0");
		if (ok)
			ok = qry(
				"DELETE FROM account_bound_rewards WHERE (remaining_pwipes IS NOT NULL AND remaining_pwipes <= 0) OR (expires_at IS NOT NULL AND expires_at <= NOW())");
	}
	else if (ok)
		ok = qry("DELETE FROM account_bound_rewards");
	if (ok)
		ok = qry(
			"UPDATE account_bound_reward_pwipe_state SET last_processed_at=NOW() WHERE id=1");
	if (!ok || !sql_commit())
	{
		sql_rollback();
		return false;
	}
	return true;
#endif
}

#ifndef __NO_MYSQL__
static void dissolve_reward_containers(P_char ch, P_obj parent, const std::string &account)
{
	if (!ch || !parent)
		return;
	for (P_obj obj = parent->contains, next; obj; obj = next)
	{
		next = obj->next_content;
		dissolve_reward_containers(ch, obj, account);

		RewardMarker marker;
		if (obj->type != ITEM_CONTAINER || !parse_reward_marker(obj, &marker) ||
		    strcasecmp(marker.account, account.c_str()) != 0)
			continue;

		if (!promote_reward_contents(obj))
		{
			logit(LOG_WIZ,
			      "divineclaim: retained reward container #%llu because corpse contents could not be released safely",
			      marker.grant_id);
			continue;
		}

		bool recovery_ready = qry(
			"UPDATE account_bound_reward_summons SET recovery_ready=1 WHERE grant_id=%llu AND pid=%d",
			marker.grant_id, GET_PID(ch));
		if (!recovery_ready)
			logit(LOG_WIZ,
			      "divineclaim: failed to reset death recovery for grant %llu pid %d",
			      marker.grant_id, GET_PID(ch));

		act("&+WThe divinely bound $p&+W fades from existence, leaving its contents behind.&n",
		    TRUE, ch, obj, 0, TO_CHAR);
		act("&+WThe divinely bound $p&+W fades from existence, leaving its contents behind.&n",
		    TRUE, ch, obj, 0, TO_ROOM);
		extract_obj(obj);
	}
}
#endif

void account_bound_reward_prepare_player_corpse(P_char ch, P_obj corpse)
{
#ifndef __NO_MYSQL__
	if (ch && corpse && IS_PC(ch))
	{
		const char *resolved = reward_account(ch);
		if (resolved && *resolved)
			dissolve_reward_containers(ch, corpse, resolved);
	}
#else
	(void)ch;
	(void)corpse;
#endif
}

void account_bound_reward_on_login(P_char ch)
{
#ifndef __NO_MYSQL__
	summon_login_reward(ch);
#else
	(void)ch;
#endif
}

#ifndef __NO_MYSQL__
static void divineclaim_help(P_char ch)
{
	send_to_char(
		"Divine account reward commands:\r\n"
		"  divineclaim <inventory item> <account> [permanent|days <count>|wipes <count>]\r\n"
		"  divineclaim list [account]\r\n"
		"  divineclaim remove <claim-id>\r\n"
		"  divineclaim remove <account> <reward vnum|all>\r\n"
		"  divineclaim <account> [reward vnum]   (legacy vnum grant)\r\n"
		"Example: divineclaim hammer Cwial wipes 2\r\n"
		"Omitting a lifetime makes the grant permanent. The source item remains in your inventory.\r\n"
		"Each character on that account may summon one copy.\r\n",
		ch);
}
#endif

void do_divineclaim(P_char ch, char *argument, int cmd)
{
	(void)cmd;
#ifdef __NO_MYSQL__
	(void)argument;
	if (IS_TRUSTED(ch))
		send_to_char("Account rewards require the database-enabled server.\r\n", ch);
	else
		send_to_char("Your account has no active divine rewards.\r\n", ch);
	return;
#else
	if (!IS_TRUSTED(ch))
	{
		player_divineclaim(ch, argument);
		return;
	}
	char first[MAX_INPUT_LENGTH], second[MAX_INPUT_LENGTH], third[MAX_INPUT_LENGTH],
		fourth[MAX_INPUT_LENGTH], extra[MAX_INPUT_LENGTH];
	char *rest = argument;
	rest = one_argument(rest, first);
	rest = one_argument(rest, second);
	rest = one_argument(rest, third);
	rest = one_argument(rest, fourth);
	one_argument(rest, extra);
	if (!*first)
	{
		divineclaim_help(ch);
		return;
	}

	if (!strcasecmp(first, "list"))
	{
		std::string account;
		const char *filter = NULL;
		if (*second)
		{
			if (!canonical_account(second, &account))
			{
				send_to_char(
					"No account by that name exists. Use DIVINECLAIM LIST with no account to see every grant.\r\n",
					ch);
				return;
			}
			filter = account.c_str();
		}
		if (!list_grants(ch, filter))
			send_to_char(filter ? "That account has no active divine rewards.\r\n" :
					      "No active divine account rewards exist.\r\n",
				     ch);
		return;
	}

	if (!strcasecmp(first, "remove"))
	{
		std::vector<RewardGrant> grants;
		bool removal_lookup_ok = false;
		int numeric = 0;
		if (*second && !*third && parse_positive(second, &numeric))
			grants = grants_for_removal(NULL, 0, false, (unsigned long long)numeric,
						    &removal_lookup_ok);
		else if (*second && *third)
		{
			std::string account;
			if (!canonical_account(second, &account))
			{
				send_to_char("No account by that name exists.\r\n", ch);
				return;
			}
			bool all = !strcasecmp(third, "all");
			int vnum = 0;
			if (!all && !parse_positive(third, &vnum))
			{
				send_to_char(
					"Use a positive reward vnum or ALL. For exact grants, prefer DIVINECLAIM REMOVE <claim-id>.\r\n",
					ch);
				return;
			}
			grants = grants_for_removal(account.c_str(), vnum, all, 0,
						    &removal_lookup_ok);
		}
		else
		{
			send_to_char(
				"Syntax: divineclaim remove <claim-id>\r\n        divineclaim remove <account> <reward vnum|all>\r\n",
				ch);
			return;
		}
		if (!removal_lookup_ok)
		{
			send_to_char(
				"The divine account-reward records are temporarily unavailable. Nothing was removed.\r\n",
				ch);
			return;
		}
		if (grants.empty())
		{
			send_to_char(
				"No active divine reward matched that request. Use DIVINECLAIM LIST to see claim IDs.\r\n",
				ch);
			return;
		}
		int removed = remove_grants(grants);
		if (removed < 0)
			send_to_char(
				"The reward revocation failed; check the server log before retrying.\r\n",
				ch);
		else
			send_to_char_f(
				ch,
				"Revoked %d divine account reward%s and removed all saved or live copies.\r\n",
				removed, removed == 1 ? "" : "s");
		return;
	}

	std::string first_account;
	int legacy_vnum = 0;
	if (canonical_account(first, &first_account) &&
	    (!*second || parse_positive(second, &legacy_vnum)))
	{
		if (!legacy_vnum)
			legacy_vnum = DEFAULT_ACCOUNT_REWARD_VNUM;
		if (real_object(legacy_vnum) < 0)
		{
			send_to_char("That legacy reward vnum does not exist.\r\n", ch);
			return;
		}
		unsigned long long id = 0;
		if (!assign_legacy_grant(first_account, GET_NAME(ch), legacy_vnum, &id))
		{
			send_to_char("The legacy divine reward could not be recorded.\r\n", ch);
			return;
		}
		send_to_char_f(
			ch,
			"Legacy divine reward #%llu assigned to account %s using vnum %d. It is permanent; each account character may summon one copy.\r\n",
			id, first_account.c_str(), legacy_vnum);
		return;
	}

	if (!*second)
	{
		divineclaim_help(ch);
		return;
	}
	P_obj source = get_obj_in_list_vis(ch, first, ch->carrying);
	if (!source)
	{
		send_to_char_f(
			ch,
			"You are not carrying an item matching '%s'. Put the exact source item in your inventory and try again.\r\n",
			first);
		return;
	}
	RewardMarker marker;
	if (parse_reward_marker(source, &marker))
	{
		send_to_char(
			"That item is already an account reward. Use its original source item instead.\r\n",
			ch);
		return;
	}
	if (IS_ARTIFACT(source))
	{
		send_to_char(
			"Artifacts and globally unique items cannot become account rewards because each account character receives an independent copy. The source item was not changed.\r\n",
			ch);
		return;
	}
	if (source->contains)
	{
		send_to_char(
			"That container is not empty. Empty it first; DIVINECLAIM snapshots one exact item and never duplicates or silently omits contents.\r\n",
			ch);
		return;
	}
	std::string account;
	if (!canonical_account(second, &account))
	{
		send_to_char(
			"No account by that name exists. The item was not consumed or changed.\r\n",
			ch);
		return;
	}
	const char *mode = "permanent";
	int amount = 0;
	if (*third)
	{
		mode = third;
		if (!strcasecmp(mode, "permanent"))
		{
			if (*fourth)
			{
				divineclaim_help(ch);
				return;
			}
		}
		else if (!strcasecmp(mode, "days") || !strcasecmp(mode, "wipes"))
		{
			if (!parse_positive(fourth, &amount) || *extra)
			{
				send_to_char(
					"DAYS and WIPES require one positive whole-number count.\r\n",
					ch);
				return;
			}
		}
		else
		{
			send_to_char(
				"Lifetime must be PERMANENT, DAYS <count>, or WIPES <count>.\r\n",
				ch);
			return;
		}
	}
	unsigned long long id = 0;
	if (!insert_exact_grant(ch, source, account, mode, amount, &id))
	{
		send_to_char(
			"The exact divine reward could not be recorded. The source item remains unchanged.\r\n",
			ch);
		return;
	}
	const char *display = source->short_description ? source->short_description :
							  "a divine reward";
	if (!strcasecmp(mode, "days"))
		send_to_char_f(
			ch,
			"Created divine reward #%llu for account %s from the exact item %s.&n It expires in %d day%s.\r\n",
			id, account.c_str(), display, amount, amount == 1 ? "" : "s");
	else if (!strcasecmp(mode, "wipes"))
		send_to_char_f(
			ch,
			"Created divine reward #%llu for account %s from the exact item %s.&n It expires after %d successful player wipe%s.\r\n",
			id, account.c_str(), display, amount, amount == 1 ? "" : "s");
	else
		send_to_char_f(
			ch,
			"Created permanent divine reward #%llu for account %s from the exact item %s.&n\r\n",
			id, account.c_str(), display);
	send_to_char(
		"The source item remains in your inventory. Each character on that account may summon one copy with DIVINECLAIM.\r\n",
		ch);
	logit(LOG_WIZ, "%s created exact divineclaim #%llu vnum %d for account %s (%s)", J_NAME(ch),
	      id, OBJ_VNUM(source), account.c_str(), mode);
#endif
}
