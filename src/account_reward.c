#include "prototypes.h"
#include "structs.h"
#include "utility.h"
#include "utils.h"
#include "interp.h"
#include "account_reward.h"
#include "sql.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

extern P_obj object_list;

static const char *reward_account(P_char ch)
{
    const char *account = get_account_name_safe(ch);
    if (!account || !*account || strcasecmp(account, "Unknown") == 0)
        return NULL;
    return account;
}

static bool reward_marker_matches(P_obj obj, const char *account)
{
    char owner[ACCOUNT_REWARD_ACCOUNT_MAX + 1];
    const char *marker;

    if (!obj || !obj->name || !account)
        return false;
    marker = strstr(obj->name, ACCOUNT_REWARD_MARKER);
    if (!marker)
        return false;
    marker += strlen(ACCOUNT_REWARD_MARKER);
    if (sscanf(marker, "%49s", owner) != 1)
        return false;
    return strcasecmp(owner, account) == 0;
}

bool account_bound_reward_owner(P_char ch, P_obj obj)
{
    const char *account = reward_account(ch);
    return account && reward_marker_matches(obj, account);
}

static void mark_reward_item(P_obj obj, const char *account)
{
    char name[MAX_STRING_LENGTH];
    char short_description[MAX_STRING_LENGTH];

    if (!obj || !account)
        return;

    if (!reward_marker_matches(obj, account))
    {
        snprintf(name, sizeof(name), "%s%s %s", ACCOUNT_REWARD_MARKER, account, obj->name ? obj->name : "reward");
        if ((obj->str_mask & STRUNG_KEYS) && obj->name)
            str_free(obj->name);
        obj->str_mask |= STRUNG_KEYS;
        obj->name = str_dup(name);
    }

    if (!obj->short_description || !strstr(obj->short_description, "soulbound"))
    {
        snprintf(short_description,
                 sizeof(short_description),
                 "%s &+L(soulbound)&n",
                 obj->short_description ? obj->short_description : "a divine reward");
        set_short_description(obj, short_description);
    }

    SET_BIT(obj->extra_flags, ITEM_NOSELL | ITEM_NORENT | ITEM_NODROP | ITEM_NOREPAIR);
    SET_BIT(obj->extra2_flags, ITEM2_SOULBIND | ITEM2_ACCOUNT_BOUND | ITEM2_NOLOOT);
    REMOVE_BIT(obj->extra_flags, ITEM_SECRET | ITEM_INVISIBLE);
}

static void detach_reward_item(P_obj obj)
{
    if (OBJ_CARRIED(obj))
    {
        obj_from_char(obj);
    }
    else if (OBJ_WORN(obj))
    {
        P_char wearer = WEARER(obj);
        for (int pos = 0; wearer && pos < MAX_WEAR; ++pos)
        {
            if (wearer->equipment[pos] == obj)
            {
                obj = unequip_char(wearer, pos);
                break;
            }
        }
    }
    else if (OBJ_INSIDE(obj))
    {
        obj_from_obj(obj);
    }
    else if (OBJ_ROOM(obj))
    {
        obj_from_room(obj);
    }
}

#ifndef __NO_MYSQL__
static std::vector<int> assigned_reward_vnums(const char *account)
{
    char escaped[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    MYSQL_RES *res;
    MYSQL_ROW row;
    std::vector<int> vnums;

    mysql_real_escape_string(DB, escaped, account, strlen(account));
    res = db_query("SELECT reward_vnum FROM account_bound_rewards WHERE account_name = '%s' ORDER BY reward_vnum", escaped);
    if (!res)
        return vnums;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        int vnum = row[0] ? atoi(row[0]) : 0;
        if (vnum > 0)
            vnums.push_back(vnum);
    }
    mysql_free_result(res);
    return vnums;
}

static bool account_exists(const char *account)
{
    char escaped[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    MYSQL_RES *res;
    MYSQL_ROW row;
    bool exists = false;

    mysql_real_escape_string(DB, escaped, account, strlen(account));
    res = db_query("SELECT 1 FROM accounts WHERE account_name = '%s' LIMIT 1", escaped);
    if (!res)
        return false;
    row = mysql_fetch_row(res);
    exists = row != NULL;
    mysql_free_result(res);
    return exists;
}

static bool assign_reward(const char *account, const char *granted_by, int vnum)
{
    char escaped_account[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    char escaped_granter[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];

    mysql_real_escape_string(DB, escaped_account, account, strlen(account));
    mysql_real_escape_string(DB, escaped_granter, granted_by, strlen(granted_by));
    return qry("INSERT INTO account_bound_rewards (account_name, reward_vnum, granted_by) VALUES ('%s', %d, '%s') "
               "ON DUPLICATE KEY UPDATE granted_by = VALUES(granted_by), updated_at = CURRENT_TIMESTAMP",
               escaped_account,
               vnum,
               escaped_granter);
}

static int remove_reward_claims(const char *account, int vnum, bool remove_all)
{
    char escaped_account[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    bool ok;

    mysql_real_escape_string(DB, escaped_account, account, strlen(account));
    if (remove_all)
        ok = qry("DELETE FROM account_bound_rewards WHERE account_name = '%s'", escaped_account);
    else
        ok = qry("DELETE FROM account_bound_rewards WHERE account_name = '%s' AND reward_vnum = %d",
                 escaped_account,
                 vnum);
    if (!ok)
        return -1;
    return (int)mysql_affected_rows(DB);
}

static bool list_reward_claims(P_char ch, const char *account)
{
    char escaped_account[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    MYSQL_RES *res;
    MYSQL_ROW row;
    bool found = false;

    mysql_real_escape_string(DB, escaped_account, account, strlen(account));
    res = db_query("SELECT reward_vnum, granted_by FROM account_bound_rewards WHERE account_name = '%s' ORDER BY reward_vnum",
                   escaped_account);
    if (!res)
        return false;
    while ((row = mysql_fetch_row(res)) != NULL)
    {
        char line[MAX_STRING_LENGTH];
        snprintf(line,
                 sizeof(line),
                 "Account %s has divine claim vnum %s (granted by %s).\r\n",
                 account,
                 row[0] ? row[0] : "0",
                 row[1] && *row[1] ? row[1] : "unknown");
        send_to_char(line, ch);
        found = true;
    }
    mysql_free_result(res);
    return found;
}
#endif

#ifndef __NO_MYSQL__
static void clear_saved_rewards(const char *account, int vnum)
{
    char escaped_account[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    char escaped_marker[(sizeof(ACCOUNT_REWARD_MARKER) - 1 + ACCOUNT_REWARD_ACCOUNT_MAX + 2) * 2 + 1];
    char marker[sizeof(ACCOUNT_REWARD_MARKER) + ACCOUNT_REWARD_ACCOUNT_MAX + 2];
    bool ok;

    mysql_real_escape_string(DB, escaped_account, account, strlen(account));
    snprintf(marker, sizeof(marker), "%s%s %%", ACCOUNT_REWARD_MARKER, account);
    mysql_real_escape_string(DB, escaped_marker, marker, strlen(marker));
    if (vnum > 0)
    {
        ok = qry("DELETE pi FROM player_items pi JOIN player_data pd ON pd.pid = pi.pid "
                 "WHERE (pd.account_name = '%s' OR EXISTS (SELECT 1 FROM account_characters ac "
                 "WHERE ac.char_name = pd.name AND ac.account_name = '%s')) "
                 "AND pi.name LIKE '%s' AND pi.vnum = %d",
                 escaped_account,
                 escaped_account,
                 escaped_marker,
                 vnum);
    }
    else
    {
        ok = qry("DELETE pi FROM player_items pi JOIN player_data pd ON pd.pid = pi.pid "
                 "WHERE (pd.account_name = '%s' OR EXISTS (SELECT 1 FROM account_characters ac "
                 "WHERE ac.char_name = pd.name AND ac.account_name = '%s')) AND pi.name LIKE '%s'",
                 escaped_account,
                 escaped_account,
                 escaped_marker);
    }
    if (!ok)
        logit(LOG_WIZ, "divineclaim: failed to clear saved rewards for account %s vnum %d", account, vnum);
}
#endif

static bool summon_reward_vnum(P_char ch, const char *account, int vnum, bool announce)
{
    P_obj keep = NULL;
    P_obj next;
    P_char previous_owner;

    for (P_obj obj = object_list; obj; obj = next)
    {
        next = obj->next;
        if (reward_marker_matches(obj, account) && OBJ_VNUM(obj) == vnum && !keep)
            keep = obj;
    }

    if (!keep)
        keep = read_object(vnum, VIRTUAL);
    if (!keep)
    {
        logit(LOG_WIZ, "divineclaim: reward vnum %d could not be loaded for account %s", vnum, account);
        return false;
    }

    for (P_obj obj = object_list; obj; obj = next)
    {
        next = obj->next;
        if (obj != keep && reward_marker_matches(obj, account) && OBJ_VNUM(obj) == vnum)
            extract_obj(obj);
    }

    previous_owner = OBJ_CARRIED(keep) ? keep->loc.carrying : (OBJ_WORN(keep) ? keep->loc.wearing : NULL);
    detach_reward_item(keep);
    if (previous_owner && previous_owner != ch && !do_save_silent(previous_owner, 1))
        logit(LOG_WIZ, "divineclaim: failed to save previous reward owner %s", GET_NAME(previous_owner));
    mark_reward_item(keep, account);
    obj_to_char(keep, ch);
    if (!OBJ_CARRIED(keep) || keep->loc.carrying != ch)
    {
        logit(LOG_WIZ, "divineclaim: reward vnum %d failed to enter inventory for %s", vnum, GET_NAME(ch));
        extract_obj(keep);
        return false;
    }

    if (announce)
    {
        char message[MAX_STRING_LENGTH];
        snprintf(message,
                 sizeof(message),
                 "The gods bestow %s upon you.\r\n",
                 keep->short_description ? keep->short_description : "a divine reward");
        send_to_char(message, ch);
    }
    return true;
}

static bool summon_rewards(P_char ch, bool announce)
{
    const char *account = reward_account(ch);
    bool summoned = false;

    if (!account)
        return false;

#ifdef __NO_MYSQL__
    (void)announce;
    return false;
#else
    std::vector<int> vnums = assigned_reward_vnums(account);
    if (vnums.empty())
        return false;

    clear_saved_rewards(account, 0);
    for (int vnum : vnums)
    {
        if (summon_reward_vnum(ch, account, vnum, announce))
            summoned = true;
    }
    if (summoned && IS_PC(ch) && !do_save_silent(ch, 1))
        logit(LOG_WIZ, "divineclaim: failed to save rewards for %s", GET_NAME(ch));
    return summoned;
#endif
}

static void revoke_live_rewards(const char *account, int vnum, bool remove_all)
{
    std::vector<P_char> changed_owners;
    P_obj next;

    for (P_obj obj = object_list; obj; obj = next)
    {
        next = obj->next;
        if (!reward_marker_matches(obj, account) || (!remove_all && OBJ_VNUM(obj) != vnum))
            continue;

        P_char owner = OBJ_CARRIED(obj) ? obj->loc.carrying : (OBJ_WORN(obj) ? obj->loc.wearing : NULL);
        extract_obj(obj);
        if (owner)
        {
            bool known = false;
            for (P_char changed : changed_owners)
            {
                if (changed == owner)
                {
                    known = true;
                    break;
                }
            }
            if (!known)
                changed_owners.push_back(owner);
        }
    }

    for (P_char owner : changed_owners)
    {
        if (!do_save_silent(owner, 1))
            logit(LOG_WIZ, "divineclaim: failed to save revoked reward owner %s", GET_NAME(owner));
    }
}

void account_bound_reward_on_login(P_char ch)
{
    summon_rewards(ch, true);
}

void do_divineclaim(P_char ch, char *argument, int cmd)
{
    char first[MAX_INPUT_LENGTH];
    char second[MAX_INPUT_LENGTH];
    char third[MAX_INPUT_LENGTH];
    char *rest = argument;

    if (IS_TRUSTED(ch))
    {
        rest = one_argument(rest, first);
        rest = one_argument(rest, second);
        one_argument(rest, third);
        if (!*first)
        {
            send_to_char("Syntax: divineclaim <account> [reward vnum]\r\n"
                         "        divineclaim list <account>\r\n"
                         "        divineclaim remove <account> <reward vnum|all>\r\n",
                         ch);
            return;
        }

#ifdef __NO_MYSQL__
        send_to_char("Account rewards require the database-enabled server.\r\n", ch);
        return;
#else
        if (strcasecmp(first, "list") == 0)
        {
            if (!*second)
            {
                send_to_char("Syntax: divineclaim list <account>\r\n", ch);
                return;
            }
            if (!account_exists(second))
            {
                send_to_char("That account does not exist.\r\n", ch);
                return;
            }
            if (!list_reward_claims(ch, second))
                send_to_char("That account has no divine claims.\r\n", ch);
            return;
        }

        if (strcasecmp(first, "remove") == 0)
        {
            bool remove_all;
            int vnum;
            int removed;

            if (!*second || !*third)
            {
                send_to_char("Syntax: divineclaim remove <account> <reward vnum|all>\r\n", ch);
                return;
            }
            if (!account_exists(second))
            {
                send_to_char("That account does not exist.\r\n", ch);
                return;
            }
            remove_all = strcasecmp(third, "all") == 0;
            vnum = remove_all ? 0 : atoi(third);
            if (!remove_all && vnum <= 0)
            {
                send_to_char("Specify a positive reward vnum or all.\r\n", ch);
                return;
            }
            removed = remove_reward_claims(second, vnum, remove_all);
            if (removed < 0)
            {
                send_to_char("The divine claim could not be removed.\r\n", ch);
                return;
            }
            clear_saved_rewards(second, vnum);
            revoke_live_rewards(second, vnum, remove_all);
            if (removed == 0)
                send_to_char("No matching divine claim was assigned. Stale marked items were still cleared.\r\n", ch);
            else if (remove_all)
                send_to_char("All account-bound divine claims have been removed.\r\n", ch);
            else
                send_to_char("The account-bound divine claim has been removed.\r\n", ch);
            logit(LOG_WIZ,
                  "%s removed %s divineclaim%s from account %s",
                  J_NAME(ch),
                  remove_all ? "all" : third,
                  removed == 1 ? "" : "s",
                  second);
            return;
        }

        int vnum = *second ? atoi(second) : DEFAULT_ACCOUNT_REWARD_VNUM;
        if (!account_exists(first))
        {
            send_to_char("That account does not exist.\r\n", ch);
            return;
        }
        if (vnum <= 0 || real_object(vnum) < 0)
        {
            send_to_char("That reward object vnum does not exist.\r\n", ch);
            return;
        }
        if (!assign_reward(first, GET_NAME(ch), vnum))
        {
            send_to_char("The divine claim could not be recorded.\r\n", ch);
            return;
        }
        send_to_char("The account-bound divine claim has been assigned.\r\n", ch);
        logit(LOG_WIZ, "%s assigned divineclaim vnum %d to account %s", J_NAME(ch), vnum, first);
        return;
#endif
    }

    if (!summon_rewards(ch, true))
        send_to_char("No divine claim has been granted to your account.\r\n", ch);
}
