#include <stdio.h>

#define AREA_LIST "AREA.MINI"
#define WLD_DIR "wld"
#define OBJ_DIR "obj"
#define MOB_DIR "mob"
#define ZON_DIR "zon"

#define ALL_ZON "mini.zon"
#define ALL_WLD "mini.wld"
#define ALL_OBJ "mini.obj"
#define ALL_MOB "mini.mob"

void punt(char *msg)
{
	fprintf(stderr, "%s\n");
	exit(-1);
}

main()
{
	FILE *area_list, *all_zon, *all_wld, *all_obj, *all_mob, *tmp_zon, *tmp_wld, *tmp_obj,
		*tmp_mob;
	char area[8192], buf[8192], area_name[80], zon_name[80], wld_name[80], obj_name[80],
		mob_name[80];
	int area_count, zon_count, wld_count, obj_count, mob_count;

	/*
     *	Open all the stupid files up
     */
	area_list = fopen(AREA_LIST, "r");
	if (area_list == NULL)
		punt("AREA file cannot be opened");

	all_zon = fopen(ALL_ZON, "w");
	if (all_zon == NULL)
		punt("world.zon cannot be opened");

	all_wld = fopen(ALL_WLD, "w");
	if (all_wld == NULL)
		punt("world.wld cannot be opened");

	all_obj = fopen(ALL_OBJ, "w");
	if (all_obj == NULL)
		punt("world.obj cannot be opened");

	all_mob = fopen(ALL_MOB, "w");
	if (all_mob == NULL)
		punt("world.mob cannot be opened");

	area_count = 0;
	zon_count = -1;
	wld_count = -1;
	obj_count = -1;
	mob_count = -1;
	for (;;)
	{
		fgets(area, 8191, area_list);
		if (area == NULL)
			break;
		if (feof(area_list))
			break;
		if (area[0] == '*') /* a comment */
			continue;
		area_count++;
		sscanf(area, "%s", area_name);
		fprintf(stdout, "Area[%2d] : %s\n", area_count, area_name);
		/*
	 * open up individual zon, wld, obj, mob files for each area
         */

		sprintf(zon_name, "%s/%s.zon", ZON_DIR, area_name);
		tmp_zon = fopen(zon_name, "r");
		if (tmp_zon == NULL)
		{
			fprintf(stdout, "\twarning: %s not found\n", zon_name);
		}
		else
		{
			for (;;)
			{
				fgets(buf, 8191, tmp_zon);
				if (buf == NULL)
					break;
				if (feof(tmp_zon))
					break;
				if (*buf == '#')
					zon_count++;
				fputs(buf, all_zon);
			}
			fputs("$~", all_zon);
			fclose(tmp_zon);
		}

		sprintf(wld_name, "%s/%s.wld", WLD_DIR, area_name);
		tmp_wld = fopen(wld_name, "r");
		if (tmp_wld == NULL)
		{
			fprintf(stdout, "\twarning: %s not found\n", wld_name);
		}
		else
		{
			for (;;)
			{
				fgets(buf, 8191, tmp_wld);
				if (buf == NULL)
					break;
				if (feof(tmp_wld))
					break;
				if (*buf == '#')
					wld_count++;
				fputs(buf, all_wld);
			}
			fclose(tmp_wld);
		}

		sprintf(obj_name, "%s/%s.obj", OBJ_DIR, area_name);
		tmp_obj = fopen(obj_name, "r");
		if (tmp_obj == NULL)
		{
			fprintf(stdout, "\twarning: %s not found\n", obj_name);
		}
		else
		{
			for (;;)
			{
				fgets(buf, 8191, tmp_obj);
				if (buf == NULL)
					break;
				if (feof(tmp_obj))
					break;
				if (*buf == '#')
					obj_count++;
				fputs(buf, all_obj);
			}
			fclose(tmp_obj);
		}

		sprintf(mob_name, "%s/%s.mob", MOB_DIR, area_name);
		tmp_mob = fopen(mob_name, "r");
		if (tmp_mob == NULL)
		{
			fprintf(stdout, "\twarning: %s not found\n", mob_name);
		}
		else
		{
			for (;;)
			{
				fgets(buf, 8191, tmp_mob);
				if (buf == NULL)
					break;
				if (feof(tmp_mob))
					break;
				if (*buf == '#')
					mob_count++;
				fputs(buf, all_mob);
			}
			fclose(tmp_mob);
		}
	}

	/*
     *	Close em all now, like good little boys
     */
	fclose(area_list);
	fclose(all_zon);
	fclose(all_wld);
	fclose(all_obj);
	fclose(all_mob);

	fprintf(stdout, "\nSummary\t%d zones\t%d rooms\t%d objects\t%d mobs\n\n", zon_count,
		wld_count, obj_count, mob_count);

	fprintf(stdout, "Done\n");
}
