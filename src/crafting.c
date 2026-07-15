/*
 * Modern Craft and Forge shared calculations.
 * Command ownership moves here in subsequent extraction slices; the plan is
 * already shared so preview and execution cannot drift on material costs.
 */
#include "prototypes.h"
#include "structs.h"
#include "crafting.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int crafting_level_gate = 3;
static double crafting_material_quantity_multiplier = 1.0;

static void load_crafting_config(void)
{
	FILE *fp;
	char line[512];
	char *key, *value, *equals, *end;

	/* A restart/boot always starts from historical defaults. */
	crafting_level_gate = 3;
	crafting_material_quantity_multiplier = 1.0;

	fp = fopen("lib/crafting.cfg", "r");
	if (fp == NULL)
	{
		fprintf(stderr, "WARNING: Cannot open lib/crafting.cfg — using defaults.\r\n");
		logit(LOG_STATUS, "WARNING: Cannot open lib/crafting.cfg — using defaults.");
		return;
	}

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		key = line;
		while (*key == ' ' || *key == '\t') key++;
		if (*key == '\0' || *key == '\n' || *key == '#') continue;
		equals = strchr(key, '=');
		if (equals == NULL) continue;
		*equals = '\0';
		end = equals - 1;
		while (end >= key && (*end == ' ' || *end == '\t')) *end-- = '\0';
		value = equals + 1;
		while (*value == ' ' || *value == '\t') value++;
		end = value + strlen(value);
		while (end > value && (end[-1] == '\n' || end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';

		if (!strcmp(key, "crafting.level.gate.multiplier") && atoi(value) > 0)
			crafting_level_gate = atoi(value);
		else if (!strcmp(key, "crafting.material.quantity.multiplier") && strtod(value, NULL) > 0.0)
			crafting_material_quantity_multiplier = strtod(value, NULL);
	}
	fclose(fp);
}

bool crafting_build_plan(P_obj item, struct crafting_plan *plan)
{
	int item_value;
	int low_material_vnum;
	int high_material_count;

	if (item == NULL || plan == NULL)
	{
		return FALSE;
	}

	item_value = itemvalue(item);
	low_material_vnum = get_matstart(item);
	if (item_value < 1 || low_material_vnum <= 0)
	{
		return FALSE;
	}

	high_material_count = (item_value + 4) / 5;
	plan->item_value = item_value;
	plan->low_material_vnum = low_material_vnum;
	plan->high_material_vnum = low_material_vnum + 4;
	plan->high_material_count = (int)ceil(high_material_count * crafting_material_quantity_multiplier);
	plan->low_material_count = (int)ceil(((item_value + 4) - high_material_count * 5) * crafting_material_quantity_multiplier);
	plan->magical = has_affect(item);
	return TRUE;
}

void boot_crafting_system(void)
{
	load_crafting_config();
}

int crafting_level_gate_multiplier(void)
{
	return crafting_level_gate;
}

void crafting_handle_command(P_char ch, enum crafting_mode mode, char *argument)
{
	(void)ch;
	(void)mode;
	(void)argument;
}
