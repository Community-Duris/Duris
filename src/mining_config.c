#include "prototypes.h"
#include "utils.h"
#include "mining_config.h"

#include <stdio.h>
#include <string.h>

#define MINING_REGION_COUNT 5
#define MINING_GEM_COUNT 40

struct mining_region_config
{
	int start;
	int end;
	int duration;
	int max_mines;
};

struct mining_gem_weight
{
	int vnum;
	int weight;
};

/* Defaults exactly preserve the legacy region table and 1..170 gem switch. */
static struct mining_region_config regions[MINING_REGION_COUNT] = {
    {500000, 659999, 9, 0}, {700000, 859999, 9, 0}, {110000, 119999, 9, 0}, {500000, 659999, 15, 0}, {700000, 859999, 15, 0}};

static struct mining_gem_weight gems[MINING_GEM_COUNT] = {
    {504, 8}, {505, 8}, {506, 6}, {507, 6}, {508, 6}, {509, 6}, {510, 6}, {511, 6}, {512, 6}, {513, 6},
    {514, 5}, {515, 5}, {516, 5}, {517, 5}, {518, 5}, {519, 5}, {520, 5}, {521, 5}, {522, 4}, {523, 4},
    {524, 4}, {525, 4}, {526, 4}, {527, 4}, {528, 4}, {529, 4}, {530, 3}, {531, 3}, {532, 3}, {533, 3},
    {534, 3}, {535, 3}, {536, 3}, {537, 3}, {538, 2}, {539, 2}, {540, 2}, {541, 2}, {542, 1}, {543, 1}};

static const char *region_names[MINING_REGION_COUNT] = {"map", "ud", "tharnrift", "mapg", "udg"};

static void set_region_value(int region, const char *field, int value)
{
	if (region < 0 || region >= MINING_REGION_COUNT || value < 0)
		return;
	if (!strcmp(field, "start")) regions[region].start = value;
	else if (!strcmp(field, "end")) regions[region].end = value;
	else if (!strcmp(field, "duration") && value >= 4) regions[region].duration = value;
	else if (!strcmp(field, "max")) regions[region].max_mines = value;
}

void mining_config_boot(void)
{
	FILE *fp = fopen("lib/mining.cfg", "r");
	char line[256], key[128], value[128], name[32], field[32];
	if (!fp)
	{
		logit(LOG_STATUS, "Mining config unavailable; using compiled defaults.");
		return;
	}
	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '#' || sscanf(line, "%127[^=]=%127s", key, value) != 2)
			continue;
		if (sscanf(key, "mining.region.%31[^.].%31s", name, field) == 2)
		{
			for (int i = 0; i < MINING_REGION_COUNT; ++i)
				if (!strcmp(name, region_names[i])) set_region_value(i, field, atoi(value));
		}
		else
		{
			int vnum;
			if (sscanf(key, "mining.gem.%d.weight", &vnum) == 1)
				for (int i = 0; i < MINING_GEM_COUNT; ++i)
					if (gems[i].vnum == vnum && atoi(value) >= 0) gems[i].weight = atoi(value);
		}
	}
	fclose(fp);
}

int mining_config_region_value(int region, const char *field, int fallback)
{
	if (region < 0 || region >= MINING_REGION_COUNT) return fallback;
	if (!strcmp(field, "start")) return regions[region].start;
	if (!strcmp(field, "end")) return regions[region].end;
	if (!strcmp(field, "duration")) return regions[region].duration;
	if (!strcmp(field, "max") && regions[region].max_mines > 0) return regions[region].max_mines;
	return fallback;
}

int mining_config_gem_vnum(void)
{
	int total = 0;
	for (int i = 0; i < MINING_GEM_COUNT; ++i) total += gems[i].weight;
	if (total <= 0) return 504;
	int roll = number(1, total);
	for (int i = 0; i < MINING_GEM_COUNT; ++i)
		if ((roll -= gems[i].weight) <= 0) return gems[i].vnum;
	return 504;
}
