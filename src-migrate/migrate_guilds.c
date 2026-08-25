// migrate_guilds.c
// guild migration for pfile migration tool

#include "migrate_common.h"

// count guild files
static int count_guilds(void)
{
	int total = 0;
	int miss_count = 0;
	for (int guild_num = 1; miss_count < 20; guild_num++)
	{
		char filename[300];
		snprintf(filename, sizeof(filename), "Players/Assocs/asc.%d", guild_num);
		FILE *f = fopen(filename, "r");
		if (!f)
		{
			miss_count++;
		}
		else
		{
			fclose(f);
			total++;
			miss_count = 0;
		}
	}
	return total;
}

int migrate_guilds_from_files(void)
{
	FILE *file;
	char filename[300], lbuf[300], mem_name[MAX_NAME_LENGTH + 1];
	int mem_bits, mem_debt;
	int count = 0;
	int errors = 0;
	int miss_count = 0;
	int processed = 0;

	int total = count_guilds();
	struct progress_bar pb;
	progress_init(&pb, total, "guilds");

	for (int guild_num = 1; miss_count < 20; guild_num++)
	{
		snprintf(filename, sizeof(filename), "Players/Assocs/asc.%d", guild_num);
		file = fopen(filename, "r");

		if (!file)
		{
			miss_count++;
			continue;
		}
		miss_count = 0;

		Guild *new_guild = new Guild();

		// get the guild name
		fgets(new_guild->name, ASC_MAX_STR, file);
		// cut the carriage return off
		char *nl = strchr(new_guild->name, '\n');
		if (nl)
			*nl = '\0';

		// then the guild number and frag info
		fscanf(file, "%u %lu %lu %s\n", &(new_guild->racewar), &(new_guild->frags.frags),
		       &(new_guild->frags.top_frags), new_guild->frags.topfragger);

		new_guild->id_number = guild_num;

		// then get the default guild titles
		for (int i = 0; i < ASC_NUM_RANKS; i++)
		{
			fgets(lbuf, ASC_MAX_STR_RANK + 1, file);
			// cut the carriage return off
			lbuf[strlen(lbuf) - 1] = '\0';
			snprintf(new_guild->titles[i], ASC_MAX_STR_RANK, "%s", lbuf);
		}

		// then get the guild bits, prestige and construction
		fgets(lbuf, sizeof(lbuf), file);
		sscanf(lbuf, "%u %lu %lu\n", &new_guild->bits, &new_guild->prestige,
		       &new_guild->construction);

		// then get the money for the guild
		fscanf(file, "%u %u %u %u\n", &(new_guild->platinum), &(new_guild->gold),
		       &(new_guild->silver), &(new_guild->copper));

		// then get members
		new_guild->members = NULL;
		new_guild->member_count = 0;
		P_member last_member = NULL;

		while (fscanf(file, "%s %u %u\n", mem_name, (unsigned int *)&mem_bits,
			      (unsigned int *)&mem_debt) == 3)
		{
			P_member new_member = new guild_member();
			snprintf(new_member->name, MAX_NAME_LENGTH + 1, "%s", mem_name);
			new_member->bits = mem_bits;
			new_member->debt = mem_debt;
			new_member->next = NULL;

			if (last_member == NULL)
			{
				new_guild->members = new_member;
			}
			else
			{
				last_member->next = new_member;
			}
			last_member = new_member;
			new_guild->member_count++;
		}

		fclose(file);

		// save to database
		if (sql_save_guild(new_guild))
		{
			count++;
		}
		else
		{
			errors++;
		}

		// free the guild
		P_member mem = new_guild->members;
		while (mem)
		{
			P_member next = mem->next;
			delete mem;
			mem = next;
		}
		delete new_guild;

		processed++;
		progress_update(&pb, processed);
	}

	progress_finish(&pb);
	printf("guilds: %d migrated, %d errors\n", count, errors);
	return count;
}
