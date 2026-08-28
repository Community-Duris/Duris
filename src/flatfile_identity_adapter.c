#include "flatfile_identity_adapter.h"

#include "flatfile_identity_repository.h"
#include "persistence_mode.h"

namespace
{
const char *state_root()
{
	return persistence_mode_flatfile_root();
}
} // namespace

bool flatfile_player_identity_exists(const char *name, bool *exists, std::string *error)
{
	const char *root = state_root();
	if (!root || !name || !exists)
		return false;
	flatfile_identity_record record;
	const flatfile_identity_result result =
		flatfile_identity_lookup_name(root, name, &record, error);
	if (result == flatfile_identity_result::ok)
	{
		*exists = true;
		return true;
	}
	if (result == flatfile_identity_result::not_found)
	{
		*exists = false;
		return true;
	}
	return false;
}

bool flatfile_player_identity_pid(const char *name, int32_t *pid, std::string *error)
{
	const char *root = state_root();
	if (!root || !name || !pid)
		return false;
	flatfile_identity_record record;
	if (flatfile_identity_lookup_name(root, name, &record, error) !=
	    flatfile_identity_result::ok)
		return false;
	*pid = record.pid;
	return true;
}

bool flatfile_player_identity_allocate(int32_t *pid, std::string *error)
{
	const char *root = state_root();
	return root &&
	       flatfile_identity_allocate_pid(root, pid, error) == flatfile_identity_result::ok;
}

bool flatfile_player_identity_highest(int32_t *pid, std::string *error)
{
	const char *root = state_root();
	return root && flatfile_identity_current_highest_pid(root, pid, error) ==
			       flatfile_identity_result::ok;
}

bool flatfile_player_identity_claim(int32_t pid, const char *name, const char *account,
				    std::string *error)
{
	const char *root = state_root();
	return root && name && account &&
	       flatfile_identity_claim(root, pid, name, account, error) ==
		       flatfile_identity_result::ok;
}
