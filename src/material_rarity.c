/*
 * File: material_rarity.c
 * Usage: read-only static material-composition reporting for object templates.
 *
 * This intentionally reports template composition frequency, not live economic
 * scarcity.  It uses the booted object prototypes and itemvalue() so its recipe
 * interpretation matches the game rather than a separately-maintained parser.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "db.h"
#include "defines.h"
#include "material_rarity.h"
#include "objmisc.h"
#include "prototypes.h"
#include "structs.h"
#include "tradeskill.h"
#include "utils.h"

#define MATERIAL_RARITY_MIN_VNUM 400000
#define MATERIAL_RARITY_MAX_VNUM 400209
#define MATERIAL_RARITY_COUNT (MATERIAL_RARITY_MAX_VNUM - MATERIAL_RARITY_MIN_VNUM + 1)

typedef struct material_rarity_tally
{
	long units;
	long templates;
} material_rarity_tally;

extern P_index obj_index;
extern int     top_of_objt;

static void csv_field(FILE *file, const char *value)
{
	const char *p;

	fputc('"', file);
	for (p = value ? value : ""; *p; ++p)
	{
		if (*p == '"')
			fputc('"', file);
		fputc(*p, file);
	}
	fputc('"', file);
}

static void display_name(char *out, size_t out_size, const char *value)
{
	size_t used = 0;

	if (!out_size)
		return;
	for (; value && *value && used + 1 < out_size; ++value)
	{
		/* Strip Duris colour markup for stable CSV labels. */
		if (*value == '&')
		{
			if (value[1] == '+')
			{
				if (value[2])
					value += 2;
				continue;
			}
			if (value[1])
			{
				++value;
				continue;
			}
		}
		out[used++] = *value;
	}
	out[used] = '\0';
}

/* get_matstart() defaults unknown material families to nonsubstantial.  Do
 * not silently fold those templates into that family in an audit report. */
static int material_family_is_mapped(int material)
{
	switch (material)
	{
		case MAT_NONSUBSTANTIAL:
		case MAT_FLESH:
		case MAT_CLOTH:
		case MAT_BARK:
		case MAT_SOFTWOOD:
		case MAT_HARDWOOD:
		case MAT_CRYSTAL:
		case MAT_BONE:
		case MAT_STONE:
		case MAT_HIDE:
		case MAT_LEATHER:
		case MAT_CURED_LEATHER:
		case MAT_IRON:
		case MAT_STEEL:
		case MAT_BRASS:
		case MAT_MITHRIL:
		case MAT_ADAMANTIUM:
		case MAT_BRONZE:
		case MAT_COPPER:
		case MAT_SILVER:
		case MAT_ELECTRUM:
		case MAT_GOLD:
		case MAT_PLATINUM:
		case MAT_GEM:
		case MAT_DIAMOND:
		case MAT_RUBY:
		case MAT_EMERALD:
		case MAT_SAPPHIRE:
		case MAT_IVORY:
		case MAT_DRAGONSCALE:
		case MAT_OBSIDIAN:
		case MAT_GRANITE:
		case MAT_MARBLE:
		case MAT_BAMBOO:
		case MAT_REEDS:
		case MAT_HEMP:
		case MAT_GLASSTEEL:
		case MAT_CHITINOUS:
		case MAT_REPTILESCALE:
		case MAT_RUBBER:
		case MAT_FEATHER:
		case MAT_PEARL:
			return TRUE;
		default:
			return FALSE;
	}
}

static int material_template_exists(int vnum)
{
	P_obj raw = read_object(vnum, VIRTUAL);
	if (!raw)
		return FALSE;
	extract_obj(raw);
	return TRUE;
}

static void material_template_name(int vnum, char *out, size_t out_size)
{
	P_obj raw = read_object(vnum, VIRTUAL);
	if (!raw)
	{
		snprintf(out, out_size, "missing material template %d", vnum);
		return;
	}
	display_name(out, out_size, raw->short_description);
	extract_obj(raw);
}

static void write_exclusion(FILE *file, P_obj obj, int vnum, const char *reason, int ival)
{
	char name[MAX_STRING_LENGTH];
	display_name(name, sizeof(name), obj->short_description);
	fprintf(file, "%d,%d,%d,", vnum, obj->material, ival);
	csv_field(file, reason);
	fputc(',', file);
	csv_field(file, name);
	fputc('\n', file);
}

void write_material_rarity_report(const char *output_dir)
{
	char                    summary_path[512], recipes_path[512], exclusions_path[512], readme_path[512];
	char                    source_name[MAX_STRING_LENGTH], low_name[MAX_STRING_LENGTH], high_name[MAX_STRING_LENGTH];
	FILE                   *summary = NULL, *recipes = NULL, *exclusions = NULL, *readme = NULL;
	material_rarity_tally   tally[MATERIAL_RARITY_COUNT];
	long                    total_material_units = 0;
	long                    qualifying_templates = 0;
	long                    excluded_templates = 0;
	int                     rnum;

	if (!output_dir || !*output_dir)
		output_dir = "material-rarity-report";
	if (mkdir(output_dir, 0755) < 0 && errno != EEXIST)
	{
		fprintf(stderr, "material rarity report: cannot create %s: %s\n", output_dir, strerror(errno));
		return;
	}

	snprintf(summary_path, sizeof(summary_path), "%s/material-rarity-summary.csv", output_dir);
	snprintf(recipes_path, sizeof(recipes_path), "%s/material-rarity-recipes.csv", output_dir);
	snprintf(exclusions_path, sizeof(exclusions_path), "%s/material-rarity-exclusions.csv", output_dir);
	snprintf(readme_path, sizeof(readme_path), "%s/material-rarity-README.txt", output_dir);

	if (!(summary = fopen(summary_path, "w")) || !(recipes = fopen(recipes_path, "w")) || !(exclusions = fopen(exclusions_path, "w")) || !(readme = fopen(readme_path, "w")))
	{
		fprintf(stderr, "material rarity report: cannot open output in %s: %s\n", output_dir, strerror(errno));
		if (summary) fclose(summary);
		if (recipes) fclose(recipes);
		if (exclusions) fclose(exclusions);
		if (readme) fclose(readme);
		return;
	}

	memset(tally, 0, sizeof(tally));
	fprintf(recipes, "template_vnum,itemvalue,material_family,ingredient_vnum,ingredient_role,declared_quantity,contributed_units,template_name,ingredient_name\n");
	fprintf(exclusions, "template_vnum,material_family,itemvalue,reason,template_name\n");

	for (rnum = 0; rnum <= top_of_objt; ++rnum)
	{
		P_obj obj = read_object(rnum, REAL);
		int   ival, low_vnum, high_vnum, high_quantity, low_quantity;
		int   vnum;

		if (!obj)
			continue;
		vnum = obj_index[rnum].virtual_number;
		ival = itemvalue(obj);
		if (ival < 1)
		{
			write_exclusion(exclusions, obj, vnum, "nonpositive-itemvalue", ival);
			excluded_templates++;
			extract_obj(obj);
			continue;
		}
		if (!material_family_is_mapped(obj->material))
		{
			write_exclusion(exclusions, obj, vnum, "unmapped-material-family", ival);
			excluded_templates++;
			extract_obj(obj);
			continue;
		}

		low_vnum = get_matstart(obj);
		high_vnum = low_vnum + 4;
		if (low_vnum < MATERIAL_RARITY_MIN_VNUM || high_vnum > MATERIAL_RARITY_MAX_VNUM ||
		    !material_template_exists(low_vnum) || !material_template_exists(high_vnum))
		{
			write_exclusion(exclusions, obj, vnum, "missing-material-template", ival);
			excluded_templates++;
			extract_obj(obj);
			continue;
		}

		/* Canonical craft decomposition: one high-quality unit every five itemvalue
		 * points, with the remainder represented by lowest-quality units. */
		high_quantity = (ival + 4) / 5;
		low_quantity = (ival + 4) - (high_quantity * 5);
		display_name(source_name, sizeof(source_name), obj->short_description);
		material_template_name(low_vnum, low_name, sizeof(low_name));
		material_template_name(high_vnum, high_name, sizeof(high_name));

		/* This report's denominator deliberately counts one unit for every required
		 * ingredient identity, not its declared stack quantity. */
		tally[high_vnum - MATERIAL_RARITY_MIN_VNUM].units++;
		tally[high_vnum - MATERIAL_RARITY_MIN_VNUM].templates++;
		total_material_units++;
		fprintf(recipes, "%d,%d,%d,%d,highest,%d,1,", vnum, ival, obj->material, high_vnum, high_quantity);
		csv_field(recipes, source_name);
		fputc(',', recipes);
		csv_field(recipes, high_name);
		fputc('\n', recipes);

		if (low_quantity > 0 && low_vnum != high_vnum)
		{
			tally[low_vnum - MATERIAL_RARITY_MIN_VNUM].units++;
			tally[low_vnum - MATERIAL_RARITY_MIN_VNUM].templates++;
			total_material_units++;
			fprintf(recipes, "%d,%d,%d,%d,lowest,%d,1,", vnum, ival, obj->material, low_vnum, low_quantity);
			csv_field(recipes, source_name);
			fputc(',', recipes);
			csv_field(recipes, low_name);
			fputc('\n', recipes);
		}
		qualifying_templates++;
		extract_obj(obj);
	}

	fprintf(summary, "material_vnum,material_name,template_contributions,total_material_units,share_percent\n");
	for (rnum = 0; rnum < MATERIAL_RARITY_COUNT; ++rnum)
	{
		int vnum = MATERIAL_RARITY_MIN_VNUM + rnum;
		if (!tally[rnum].units)
			continue;
		material_template_name(vnum, low_name, sizeof(low_name));
		fprintf(summary, "%d,", vnum);
		csv_field(summary, low_name);
		fprintf(summary, ",%ld,%ld,%.6f\n", tally[rnum].units, total_material_units,
		        ((double)tally[rnum].units * 100.0) / (double)total_material_units);
	}
	fprintf(readme, "DurisMUD static material-composition report\n\n");
	fprintf(readme, "qualifying_templates=%ld\nexcluded_templates=%ld\ningredient_contributions=%ld\n\n",
	        qualifying_templates, excluded_templates, total_material_units);
	fprintf(readme, "count_mode=one-per-required-material\n");
	fprintf(readme, "Each template contributes one to each distinct required material identity.\n");
	fprintf(readme, "Declared recipe stack quantities remain in material-rarity-recipes.csv.\n\n");
	fprintf(readme, "Scope: static object templates only. This is not reset frequency, loot probability,\n");
	fprintf(readme, "player stock, replenishment rate, or economic scarcity.\n");

	fclose(summary);
	fclose(recipes);
	fclose(exclusions);
	fclose(readme);
	fprintf(stderr, "material rarity report: %ld qualifying templates, %ld ingredient contributions written to %s\n",
	        qualifying_templates, total_material_units, output_dir);
}
