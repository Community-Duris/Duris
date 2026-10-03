#include "account/account_async.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"

#include <memory>
#include <utility>

namespace
{
std::string text(const char *value)
{
	return value ? value : "";
}

void apply_snapshot(P_acct account, const account_load_snapshot &snapshot)
{
	// clear_account also clears next; the account's position in account_list belongs
	// to the game thread and must survive loading, just as read_account preserves it.
	P_acct next = account->next;
	clear_account(account);
	account->next = next;
	account->acct_name = str_dup(snapshot.name.c_str());
	account->acct_email = str_dup(snapshot.email.c_str());
	account->acct_password = str_dup(snapshot.password.c_str());
	account->acct_confirmation = str_dup(snapshot.confirmation.c_str());
	account->acct_blocked = snapshot.blocked;
	account->acct_confirmed = snapshot.confirmed;
	account->acct_confirmation_sent = snapshot.confirmation_sent;
	account->acct_last = snapshot.last;
	account->acct_good = snapshot.good;
	account->acct_evil = snapshot.evil;
	account->acct_flags1 = snapshot.flags[0];
	account->acct_flags2 = snapshot.flags[1];
	account->acct_flags3 = snapshot.flags[2];
	account->acct_flags4 = snapshot.flags[3];
	account->num_ips = static_cast<int>(snapshot.ips.size());
	account->num_chars = static_cast<int>(snapshot.characters.size());
	struct acct_ip **ip_tail = &account->acct_unique_ips;
	for (const auto &source : snapshot.ips)
	{
		struct acct_ip *ip;
		CREATE(ip, struct acct_ip, 1, MEM_TAG_OTHER);
		memset(ip, 0, sizeof(*ip));
		ip->hostname = str_dup(source.hostname.c_str());
		ip->ip_address = str_dup(source.address.c_str());
		ip->count = source.count;
		*ip_tail = ip;
		ip_tail = &ip->next;
	}
	struct acct_chars **character_tail = &account->acct_character_list;
	for (const auto &source : snapshot.characters)
	{
		struct acct_chars *character;
		CREATE(character, struct acct_chars, 1, MEM_TAG_OTHER);
		memset(character, 0, sizeof(*character));
		character->pid = source.pid;
		character->charname = str_dup(source.name.c_str());
		character->count = source.count;
		character->last = source.last;
		character->blocked = source.blocked;
		character->racewar = source.racewar;
		character->level = source.level;
		character->race = source.race;
		character->m_class = source.primary_class;
		character->secondary_class = source.secondary_class;
		character->last_room = source.last_room;
		character->last_save = source.last_save;
		*character_tail = character;
		character_tail = &character->next;
	}
}
} // namespace

struct account_request
{
	account_load_job *job = nullptr;
	account_load_completion finish;
	uint64_t id = 0, deadline = 0;
	int state = 0, descriptor = 0;
	P_acct account = nullptr;
	P_char character = nullptr;
	std::string name, credential, email, confirmation;
	char blocked = 0;
	~account_request() { account_load_release(job); }
};

bool account_async_start(P_desc d, account_load_completion finish)
{
	if (!d || !d->account || !d->account->acct_name || !finish || d->password_request ||
	    d->login_password_job)
		return false;
	// A replacement request detaches its predecessor before it can publish.
	account_async_cancel(d);
	try
	{
		auto request = std::make_unique<account_request>();
		request->finish = std::move(finish);
		request->id = account_load_next_id();
		request->deadline = account_load_now_usec() + ACCOUNT_LOAD_TIMEOUT_USEC;
		request->state = STATE(d);
		request->descriptor = d->descriptor;
		request->account = d->account;
		request->character = d->character;
		request->name = text(d->account->acct_name);
		request->credential = text(d->account->acct_password);
		request->email = text(d->account->acct_email);
		request->confirmation = text(d->account->acct_confirmation);
		request->blocked = d->account->acct_blocked;
		request->job =
			account_load_submit({ request->id, request->deadline, request->name });
		if (!request->job)
			return false;
		d->account_request = request.release();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void account_async_cancel(P_desc d)
{
	if (!d)
		return;
	delete d->account_request;
	d->account_request = nullptr;
}

bool account_async_pulse(P_desc d)
{
	if (!d || !d->account_request)
		return false;
	auto *request = d->account_request;
	if (STATE(d) != request->state || d->descriptor != request->descriptor ||
	    d->account != request->account || d->character != request->character ||
	    request->name != text(d->account ? d->account->acct_name : nullptr) ||
	    request->credential != text(d->account ? d->account->acct_password : nullptr) ||
	    request->email != text(d->account ? d->account->acct_email : nullptr) ||
	    request->confirmation != text(d->account ? d->account->acct_confirmation : nullptr) ||
	    (d->account && d->account->acct_blocked != request->blocked))
	{
		account_async_cancel(d);
		return true;
	}
	account_load_result result = {};
	if (account_load_now_usec() >= request->deadline)
		result.outcome = account_load_outcome::timed_out;
	else if (!account_load_poll(request->job, &result))
		return true;
	else if (result.id != request->id || result.request_name != request->name ||
		 (result.outcome == account_load_outcome::loaded &&
		  strcasecmp(result.snapshot.name.c_str(), request->name.c_str())))
		result.outcome = account_load_outcome::load_failed;
	std::unique_ptr<account_request> completed(request);
	d->account_request = nullptr;
	if (result.outcome == account_load_outcome::loaded)
		apply_snapshot(d->account, result.snapshot);
	// Async output needs the same IAC GA framing as password completions.
	d->prompt_mode = TRUE;
	completed->finish(d, result.outcome);
	return true;
}
