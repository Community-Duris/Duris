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
static int assigned_reward_vnum(const char *account)
{
    char escaped[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    MYSQL_RES *res;
    MYSQL_ROW row;
    int vnum = 0;

    mysql_real_escape_string(DB, escaped, account, strlen(account));
    res = db_query("SELECT reward_vnum FROM account_bound_rewards WHERE account_name = '%s' LIMIT 1", escaped);
    if (!res)
        return 0;
    row = mysql_fetch_row(res);
    if (row && row[0])
        vnum = atoi(row[0]);
    mysql_free_result(res);
    return vnum;
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
               "ON DUPLICATE KEY UPDATE reward_vnum = VALUES(reward_vnum), granted_by = VALUES(granted_by), updated_at = CURRENT_TIMESTAMP",
               escaped_account,
               vnum,
               escaped_granter);
}
#endif

#ifndef __NO_MYSQL__
static void clear_saved_rewards(const char *account)
{
    char escaped_account[ACCOUNT_REWARD_ACCOUNT_MAX * 2 + 1];
    char escaped_marker[(sizeof(ACCOUNT_REWARD_MARKER) - 1 + ACCOUNT_REWARD_ACCOUNT_MAX) * 2 + 1];
    char marker[sizeof(ACCOUNT_REWARD_MARKER) + ACCOUNT_REWARD_ACCOUNT_MAX];

    mysql_real_escape_string(DB, escaped_account, account, strlen(account));
    snprintf(marker, sizeof(marker), "%s%s %%", ACCOUNT_REWARD_MARKER, account);
    mysql_real_escape_string(DB, escaped_marker, marker, strlen(marker));
    if (!qry("DELETE pi FROM player_items pi JOIN player_data pd ON pd.pid = pi.pid "
             "WHERE (pd.account_name = '%s' OR EXISTS (SELECT 1 FROM account_characters ac "
             "WHERE ac.char_name = pd.name AND ac.account_name = '%s')) AND pi.name LIKE '%s'",
             escaped_account,
             escaped_account,
             escaped_marker))
    {
        logit(LOG_WIZ, "divineclaim: failed to clear saved duplicate rewards for account %s", account);
    }
}
#endif

static bool summon_reward(P_char ch, bool announce)
{
    const char *account = reward_account(ch);
    P_obj keep = NULL;
    P_obj next;
    P_char previous_owner;
    int vnum;

    if (!account)
        return false;

#ifdef __NO_MYSQL__
    (void)announce;
    return false;
#else
    vnum = assigned_reward_vnum(account);
    if (!vnum)
        return false;
#endif

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

#ifndef __NO_MYSQL__
    clear_saved_rewards(account);
#endif

    for (P_obj obj = object_list; obj; obj = next)
    {
        next = obj->next;
        if (obj != keep && reward_marker_matches(obj, account))
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

    if (IS_PC(ch) && !do_save_silent(ch, 1))
        logit(LOG_WIZ, "divineclaim: failed to save reward for %s", GET_NAME(ch));

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

void account_bound_reward_on_login(P_char ch)
{
    summon_reward(ch, true);
}

void do_divineclaim(P_char ch, char *argument, int cmd)
{
    char account[MAX_INPUT_LENGTH];
    char vnum_arg[MAX_INPUT_LENGTH];
    char *rest = argument;

    if (IS_TRUSTED(ch))
    {
        rest = one_argument(rest, account);
        rest = one_argument(rest, vnum_arg);
        if (!*account)
        {
            send_to_char("Syntax: divineclaim <account> [reward vnum]\r\n", ch);
            return;
        }

#ifdef __NO_MYSQL__
        send_to_char("Account rewards require the database-enabled server.\r\n", ch);
        return;
#else
        int vnum = *vnum_arg ? atoi(vnum_arg) : DEFAULT_ACCOUNT_REWARD_VNUM;
        if (!account_exists(account))
        {
            send_to_char("That account does not exist.\r\n", ch);
            return;
        }
        if (vnum <= 0 || real_object(vnum) < 0)
        {
            send_to_char("That reward object vnum does not exist.\r\n", ch);
            return;
        }
        if (!assign_reward(account, GET_NAME(ch), vnum))
        {
            send_to_char("The divine claim could not be recorded.\r\n", ch);
            return;
        }
        send_to_char("The account-bound divine claim has been assigned.\r\n", ch);
        logit(LOG_WIZ, "%s assigned divineclaim vnum %d to account %s", J_NAME(ch), vnum, account);
        return;
#endif
    }

    if (!summon_reward(ch, true))
        send_to_char("No divine claim has been granted to your account.\r\n", ch);
}
