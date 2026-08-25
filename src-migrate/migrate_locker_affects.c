// migrate_locker_affects.c
// re-extract item affects from backup locker pfiles and update database
// this re-runs the locker parsing and inserts ONLY missing affects
// usage: ./migrate_locker_affects <backup_players_dir>

#include "migrate_common.h"

// external from migrate_objects.c
extern struct mig_obj *parse_locker_items(char **buf);

static int g_affects_inserted = 0;
static int g_items_with_affects = 0;
static int g_items_skipped = 0;
static int g_lockers_processed = 0;
static int g_lockers_found = 0;

// insert affects for an item if it has them and db doesn't
static int insert_missing_affects(int item_id, struct mig_obj *obj)
{
	if (!obj->affected_set)
		return 0;

	// check if item already has affects in db
	MYSQL_RES *check =
		db_query("SELECT COUNT(*) FROM locker_item_affects WHERE item_id=%d", item_id);
	if (!check)
		return 0;
	MYSQL_ROW row = mysql_fetch_row(check);
	int existing = row ? atoi(row[0]) : 0;
	mysql_free_result(check);

	if (existing > 0)
	{
		g_items_skipped++;
		return 0; // already has affects
	}

	int inserted = 0;
	for (int i = 0; i < MAX_OBJ_AFFECT; i++)
	{
		if (obj->affected[i].location != 0 || obj->affected[i].modifier != 0)
		{
			// skip duplicates
			int is_dup = 0;
			for (int j = 0; j < i; j++)
			{
				if (obj->affected[j].location == obj->affected[i].location &&
				    obj->affected[j].modifier == obj->affected[i].modifier)
				{
					is_dup = 1;
					break;
				}
			}
			if (is_dup)
				continue;

			if (qry("INSERT INTO locker_item_affects (item_id, location, modifier) "
				"VALUES (%d, %d, %d)",
				item_id, obj->affected[i].location, obj->affected[i].modifier))
			{
				inserted++;
			}
		}
	}

	if (inserted > 0)
	{
		g_affects_inserted += inserted;
		g_items_with_affects++;
	}
	return inserted;
}

// recursively match items and insert affects
// matching by vnum + position in container list
static void process_items_recursive(int locker_id, struct mig_obj *items, int container_id)
{
	// get list of items at this container level from db
	char query[512];
	if (container_id > 0)
	{
		snprintf(
			query, sizeof(query),
			"SELECT id, vnum FROM locker_items WHERE locker_id=%d AND container_id=%d ORDER BY id",
			locker_id, container_id);
	}
	else
	{
		snprintf(
			query, sizeof(query),
			"SELECT id, vnum FROM locker_items WHERE locker_id=%d AND container_id IS NULL ORDER BY id",
			locker_id);
	}

	MYSQL_RES *res = db_query("%s", query);
	if (!res)
		return;

	// build array of db items
	int db_count = mysql_num_rows(res);
	int *db_ids = (int *)malloc(db_count * sizeof(int));
	int *db_vnums = (int *)malloc(db_count * sizeof(int));

	for (int i = 0; i < db_count; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		db_ids[i] = row ? atoi(row[0]) : 0;
		db_vnums[i] = row ? atoi(row[1]) : 0;
	}
	mysql_free_result(res);

	// match pfile items with db items by vnum order
	int pfile_idx = 0;
	int db_idx = 0;

	for (struct mig_obj *obj = items; obj; obj = obj->next, pfile_idx++)
	{
		// find matching db item with same vnum
		while (db_idx < db_count && db_vnums[db_idx] != obj->vnum)
		{
			db_idx++;
		}

		if (db_idx < db_count)
		{
			int item_id = db_ids[db_idx];

			// insert affects if missing
			insert_missing_affects(item_id, obj);

			// process contained items
			if (obj->contains)
			{
				process_items_recursive(locker_id, obj->contains, item_id);
			}

			db_idx++;
		}
	}

	free(db_ids);
	free(db_vnums);
}

// process a single locker file - same parsing as migrate_lockers.c
static int process_locker_file(const char *filepath, const char *locker_name)
{
	FILE *f = fopen(filepath, "rb");
	if (!f)
		return 0;

	char buffer[250000];
	int size = fread(buffer, 1, sizeof(buffer), f);
	fclose(f);

	if (size < 50)
		return 0;

	char *bufptr = buffer;

	// check pfile header
	int save_vers = MIG_GET_BYTE(bufptr);
	if (save_vers != SAV_SAVEVERS)
		return 0;

	int ss = MIG_GET_BYTE(bufptr);
	int is = MIG_GET_BYTE(bufptr);
	int ls = MIG_GET_BYTE(bufptr);
	if (ss != 2 || is != 4 || ls != 8)
		return 0;

	MIG_GET_BYTE(bufptr); // rent type

	// read offsets
	mig_getInt(&bufptr); // skill_off
	mig_getInt(&bufptr); // witness_off
	mig_getInt(&bufptr); // affect_off
	int item_off = mig_getInt(&bufptr);
	mig_getInt(&bufptr); // size_off
	mig_getInt(&bufptr); // act3/surname
	mig_getInt(&bufptr); // room
	mig_getLong(&bufptr); // save time

	if (item_off < 0 || item_off >= size)
		return 0;

	// get locker_id from database
	char clean_name[128];
	strncpy(clean_name, locker_name, sizeof(clean_name) - 1);
	clean_name[sizeof(clean_name) - 1] = '\0';

	char *esc_name = sql_escape_string(clean_name);
	if (!esc_name)
		return 0;

	MYSQL_RES *res = db_query("SELECT id FROM lockers WHERE locker_name = '%s'", esc_name);
	free(esc_name);

	if (!res)
		return 0;
	MYSQL_ROW row = mysql_fetch_row(res);
	int locker_id = row ? atoi(row[0]) : 0;
	mysql_free_result(res);

	if (locker_id <= 0)
		return 0;

	g_lockers_found++;

	// parse items from backup
	bufptr = buffer + item_off;
	struct mig_obj *items = parse_locker_items(&bufptr);

	if (items)
	{
		process_items_recursive(locker_id, items, 0);
		free_mig_obj(items);
		g_lockers_processed++;
	}

	return 1;
}

// walk callback
static int walk_callback(const char *filepath, const char *filename, void *userdata)
{
	struct progress_bar *pb = (struct progress_bar *)userdata;
	process_locker_file(filepath, filename);
	if (pb)
		progress_update(pb, pb->current + 1);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <backup_players_dir>\n", argv[0]);
		fprintf(stderr, "example: %s Players/Backup/1768215643\n", argv[0]);
		return 1;
	}

	const char *backup_dir = argv[1];

	// load .env file
	FILE *env = fopen(".env", "r");
	if (!env)
	{
		printf("error: .env file not found\n");
		return 1;
	}

	char line[256];
	while (fgets(line, sizeof(line), env))
	{
		if (line[0] == '#' || line[0] == '\n')
			continue;
		char *nl = strchr(line, '\n');
		if (nl)
			*nl = '\0';
		char *eq = strchr(line, '=');
		if (eq)
		{
			*eq = '\0';
			setenv(line, eq + 1, 1);
		}
	}
	fclose(env);
	printf("loaded .env file\n");

	// connect to database
	if (!initialize_mysql())
	{
		fprintf(stderr, "failed to connect to database\n");
		return 1;
	}
	printf("database connected\n");

	printf("scanning backup directory: %s\n", backup_dir);

	// count locker files
	int total = count_player_dir_files(backup_dir, ".locker", "exclude");
	printf("found %d locker files\n", total);

	if (total == 0)
	{
		printf("no locker files found\n");
		// cleanup handled by program exit
		return 0;
	}

	struct progress_bar pb;
	progress_init(&pb, total, "lockers");

	// process each locker file
	walk_player_dirs(backup_dir, ".locker", "exclude", walk_callback, &pb);

	progress_finish(&pb);

	printf("\nresults:\n");
	printf("  locker files scanned: %d\n", total);
	printf("  lockers found in db: %d\n", g_lockers_found);
	printf("  lockers with items: %d\n", g_lockers_processed);
	printf("  items updated with affects: %d\n", g_items_with_affects);
	printf("  total affects inserted: %d\n", g_affects_inserted);
	printf("  items already had affects: %d\n", g_items_skipped);

	// cleanup handled by program exit
	return 0;
}
