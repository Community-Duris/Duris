#include "prototypes.h"
#include "account_reward_config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCOUNT_REWARD_COOLDOWN_MAX 604800
#define ACCOUNT_REWARD_ACTIVE_MAX 100

static int configured_cooldown_seconds = 3600;
static int configured_max_active_rewards = 0;
static bool configured_preserve_on_pwipe = true;
static bool configured_show_claim_ids = true;

static bool parse_long_value(const char *text, long *result)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno || end == text || !end || *end != '\0')
        return false;
    *result = value;
    return true;
}

static bool parse_bool_value(const char *text, bool *result)
{
    if (!strcmp(text, "true") || !strcmp(text, "1"))
    {
        *result = true;
        return true;
    }
    if (!strcmp(text, "false") || !strcmp(text, "0"))
    {
        *result = false;
        return true;
    }
    return false;
}

static void apply_value(const char *key, const char *value)
{
    if (!strcmp(key, "summon.cooldown.seconds"))
    {
        long parsed;
        if (!parse_long_value(value, &parsed) || parsed < 0 || parsed > ACCOUNT_REWARD_COOLDOWN_MAX)
        {
            logit(LOG_STATUS,
                  "Invalid account reward config %s=%s; retaining %d seconds (valid range 0-%d).",
                  key, value, configured_cooldown_seconds, ACCOUNT_REWARD_COOLDOWN_MAX);
            return;
        }
        configured_cooldown_seconds = (int)parsed;
        return;
    }

    if (!strcmp(key, "player.max.active.rewards"))
    {
        long parsed;
        if (!parse_long_value(value, &parsed) || parsed < 0 || parsed > ACCOUNT_REWARD_ACTIVE_MAX)
        {
            logit(LOG_STATUS,
                  "Invalid account reward config %s=%s; retaining %d (valid range 0-%d, 0=unlimited).",
                  key, value, configured_max_active_rewards, ACCOUNT_REWARD_ACTIVE_MAX);
            return;
        }
        configured_max_active_rewards = (int)parsed;
        return;
    }

    if (!strcmp(key, "player.show.claim.ids"))
    {
        bool parsed;
        if (!parse_bool_value(value, &parsed))
        {
            logit(LOG_STATUS,
                  "Invalid account reward config %s=%s; retaining %s.",
                  key, value, configured_show_claim_ids ? "true" : "false");
            return;
        }
        configured_show_claim_ids = parsed;
        return;
    }

    if (!strcmp(key, "pwipe.preserve"))
    {
        bool parsed;
        if (!parse_bool_value(value, &parsed))
        {
            logit(LOG_STATUS,
                  "Invalid account reward config %s=%s; retaining %s.",
                  key, value, configured_preserve_on_pwipe ? "true" : "false");
            return;
        }
        configured_preserve_on_pwipe = parsed;
        return;
    }

    logit(LOG_STATUS, "Unknown account reward config key: %s", key);
}

void boot_account_reward_config(void)
{
    FILE *fp;
    char line[256], key[128], value[128];

    configured_cooldown_seconds = 3600;
    configured_max_active_rewards = 0;
    configured_preserve_on_pwipe = true;
    configured_show_claim_ids = true;

    fp = fopen("lib/account_rewards.cfg", "r");
    if (!fp)
    {
        logit(LOG_STATUS, "Account reward config unavailable; using cooldown=3600 seconds, player.max.active.rewards=0 (unlimited), player.show.claim.ids=true, pwipe.preserve=true.");
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        if (sscanf(line, " %127[^=]= %127s", key, value) != 2)
        {
            logit(LOG_STATUS, "Ignoring malformed account reward config line: %s", line);
            continue;
        }
        apply_value(key, value);
    }
    fclose(fp);

    logit(LOG_STATUS,
          "Loaded account reward config: summon cooldown %d seconds, max active rewards %d%s, preserve on pwipe %s, show player claim IDs %s.",
          configured_cooldown_seconds,
          configured_max_active_rewards,
          configured_max_active_rewards == 0 ? " (unlimited)" : "",
          configured_preserve_on_pwipe ? "true" : "false",
          configured_show_claim_ids ? "true" : "false");
}

int account_reward_config_cooldown_seconds(void)
{
    return configured_cooldown_seconds;
}

int account_reward_config_max_active_rewards(void)
{
    return configured_max_active_rewards;
}

bool account_reward_config_show_claim_ids(void)
{
    return configured_show_claim_ids;
}

bool account_reward_config_preserve_on_pwipe(void)
{
    return configured_preserve_on_pwipe;
}
