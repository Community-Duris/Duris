#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "combat/chaos_config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int validate_environment_overrides(void)
{
	char error[256];
	if (!chaos_config_validate_environment(error, sizeof(error)))
	{
		fprintf(stderr, "%s\n", error);
		logit(LOG_STATUS, "%s", error);
		return -1;
	}
	return 0;
}

/* Load .env before selecting a persistence backend. */
int load_env_file(void)
{
	int fd = open(".env", O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fd < 0)
	{
		if (errno != ENOENT)
		{
			logit(LOG_STATUS, "Unable to open .env securely");
			return -1;
		}
		logit(LOG_STATUS, "No .env file found; explicit process environment is required.");
		return validate_environment_overrides();
	}

	struct stat file_stat;
	if (fstat(fd, &file_stat) || !S_ISREG(file_stat.st_mode) || file_stat.st_uid != geteuid() ||
	    (file_stat.st_mode & 0177))
	{
		logit(LOG_STATUS,
		      "Unsafe .env metadata: require an owner-controlled regular file with mode 0600 or stricter");
		close(fd);
		return -1;
	}

	FILE *f = fdopen(fd, "r");
	if (!f)
	{
		close(fd);
		return -1;
	}

	char line[256];
	int count = 0;
	while (fgets(line, sizeof(line), f))
	{
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
			continue;

		char *nl = strchr(line, '\n');
		if (nl)
			*nl = '\0';
		nl = strchr(line, '\r');
		if (nl)
			*nl = '\0';
		if (line[0] == '\0')
			continue;

		char *eq = strchr(line, '=');
		if (eq)
		{
			*eq = '\0';
			setenv(line, eq + 1, 0);
			count++;
		}
	}
	fclose(f);

	logit(LOG_STATUS, "Loaded %d environment variables from .env file.", count);
	if (validate_environment_overrides() < 0)
		return -1;
	return count;
}
