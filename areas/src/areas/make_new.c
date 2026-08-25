#include <stdio.h>
#include <stdlib.h>

#define AREA_LIST "AREA"
#define WLD_DIR "wld"
#define OBJ_DIR "obj"
#define MOB_DIR "mob"
#define ZON_DIR "zon"
#define QST_DIR "qst"
#define SHP_DIR "shp"

void punt(char *msg)
{
	fprintf(stderr, "%s\n");
	exit(-1);
}

main()
{
	FILE *area_list, *all_zon, *all_wld, *all_obj, *all_mob, *all_qst, *all_shp, *tmp_zon,
		*tmp_wld, *tmp_obj, *tmp_mob, *tmp_qst, *tmp_shp;
	char area[8192], buf[8192], area_name[80], zon_name[80], wld_name[80], obj_name[80],
		mob_name[80], qst_name[80], qst_out[80], shp_name[80], shp_out[80], buf2[80],
		zon2_name[80], wld2_name[80], obj2_name[80], mob2_name[80];
	int area_count, zon_count, wld_count, obj_count, mob_count, shp_count, qst_count;

	/*
     *	Open all the stupid files up
     */
	area_list = fopen(AREA_LIST, "r");
	if (area_list == NULL)
		punt("AREA file cannot be opened");

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
		fprintf(stderr, "Area[%2d] : %s\n", area_count, area_name);
		/*
	 * open up individual zon, wld, obj, mob files for each area
         */
		sprintf(wld_name, "%s/%s.wld", WLD_DIR, area_name);
		sprintf(wld2_name, "%s.wld", area_name);
		tmp_wld = fopen(wld_name, "r");
		all_wld = fopen(wld2_name, "w");
		if (tmp_wld == NULL)
		{
			fprintf(stderr, "\twarning: %s not found\n", wld_name);
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
			fputs("$~", all_wld);
			fclose(all_wld);
		}
		sprintf(mob_name, "%s/%s.mob", MOB_DIR, area_name);
		sprintf(mob2_name, "%s.mob", area_name);
		tmp_wld = fopen(mob2_name, "w");
		tmp_mob = fopen(mob_name, "r");
		if (tmp_mob == NULL)
		{
			fprintf(stderr, "\twarning: %s not found\n", mob_name);
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
			fputs("$~", all_mob);
			fclose(all_mob);
		}
		sprintf(obj_name, "%s/%s.obj", OBJ_DIR, area_name);
		tmp_obj = fopen(obj_name, "r");
		sprintf(obj2_name, "%s.obj", area_name);
		all_obj = fopen(obj2_name, "w");
		if (tmp_obj == NULL)
		{
			fprintf(stderr, "\twarning: %s not found\n", obj_name);
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
			fputs("$~", all_obj);
			fclose(all_obj);
		}

		sprintf(zon_name, "%s/%s.zon", ZON_DIR, area_name);
		tmp_zon = fopen(zon_name, "r");
		sprintf(zon2_name, "%s.zon", area_name);
		all_zon = fopen(zon2_name, "w");
		if (tmp_zon == NULL)
		{
			fprintf(stderr, "\twarning: %s not found\n", zon_name);
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
			fclose(tmp_zon);
			fputs("$~", all_zon);
			fclose(all_zon);
		}

		sprintf(qst_name, "%s/%s.qst", QST_DIR, area_name);
		tmp_qst = fopen(qst_name, "r");
		sprintf(qst_out, "%s.qst", area_name);
		all_qst = fopen(qst_out, "w");
		if (tmp_qst == NULL)
		{
			fprintf(stderr, "\twarning: %s not found\n", qst_name);
		}
		else
		{
			for (;;)
			{
				fgets(buf, 8191, tmp_qst);
				if (buf == NULL)
					break;
				if (feof(tmp_qst))
					break;
				if (*buf == '#')
					qst_count++;
				fputs(buf, all_qst);
			}
			fclose(tmp_qst);
			fputs("$~", all_qst);
			fclose(all_qst);
		}

		sprintf(shp_name, "%s/%s.shp", SHP_DIR, area_name);
		tmp_shp = fopen(shp_name, "r");
		sprintf(shp_out, "%s.shp", area_name);
		all_shp = fopen(shp_out, "w");
		if (tmp_shp == NULL)
		{
			fprintf(stderr, "\twarning: %s not found\n", shp_name);
		}
		else
		{
			for (;;)
			{
				fgets(buf, 8191, tmp_shp);
				if (buf == NULL)
					break;
				if (feof(tmp_shp))
					break;
				if (*buf == '#')
					shp_count++;
				fputs(buf, all_shp);
			}
			fclose(tmp_shp);
			fputs("$~", all_shp);
			fclose(all_shp);
		}
	}

	/*
     *	Close em all now, like good little boys
     */
	fclose(area_list);

	fprintf(stderr,
		"\nSummary\t%d zones\t%d rooms\t%d objects\t%d mobs\n\t%d quests\t%d shops\n\n",
		zon_count, wld_count, obj_count, mob_count, qst_count, shp_count);

	fprintf(stderr, "Done\n");
}
