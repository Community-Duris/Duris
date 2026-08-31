#include <stdio.h>
#include <stdlib.h>

#include "../area_file.h"

#define AREA_LIST "AREA"
#define OBJ_DIR "obj"

#define ALL_OBJ "tworld.obj"

void punt(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

int main()
{
	FILE *area_list, *all_obj, *tmp_obj;
	char area[8192], buf[8192], area_name[80], obj_name[sizeof(area_name) + 16];
	int area_count, obj_count;

	/*
     *	Open the obj files up
     */
	area_list = fopen(AREA_LIST, "r");
	if (area_list == NULL)
		punt("AREA file cannot be opened");

	all_obj = fopen(ALL_OBJ, "w");
	if (all_obj == NULL)
		punt("world.obj cannot be opened");

	area_count = 0;
	obj_count = -1;
	for (;;)
	{
		fgets(area, 8191, area_list);
		if (area == NULL)
			break;
		if (feof(area_list))
			break;
		if (area[0] == '*') /* a comment */
			continue;
		if (sscanf(area, "%79s", area_name) != 1)
			continue;
		fprintf(stdout, "Area[%2d] : %s\n", area_count++, area_name);
		/*
	 * open up individual obj for each area
         */

		if (snprintf(obj_name, sizeof(obj_name), "%s/%s.obj", OBJ_DIR, area_name) < 0)
			punt("object filename cannot be formatted");
		tmp_obj = fopen_area_file(obj_name);
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
	}

	/*
     *	Close obj, like a good little boy
     */
	fclose(area_list);
	fclose(all_obj);

	fprintf(stdout, "\nSummary: %d objects\n\n", obj_count);

	system("chmod 600 tworld.obj");

	fprintf(stdout, "Done\n");
}
