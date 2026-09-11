#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/comm.h"
#include "net/ws_handlers.h"
#include "account/password_async.h"
#include <string>
#include <memory>

static std::string snapshot(const char *s)
{
	return s ? s : "";
}
struct password_request
{
	password_login_job *job = nullptr;
	password_completion finish;
	int state;
	P_acct account;
	P_char character;
	int room;
	std::string name, credential, email, expected;
	~password_request() { password_login_release(job); }
};

bool password_async_start(P_desc d, password_login_job *job, const char *expected_hash,
			  password_completion finish)
{
	if (!d || !job || d->password_request || d->login_password_job)
	{
		password_login_release(job);
		return false;
	}
	auto request = std::make_unique<password_request>();
	request->job = job;
	request->finish = std::move(finish);
	request->state = STATE(d);
	request->account = d->account;
	request->character = d->character;
	request->room = d->character ? d->character->in_room : -1;
	request->name = snapshot(d->account ? d->account->acct_name : nullptr);
	request->credential = snapshot(d->account ? d->account->acct_password : nullptr);
	request->email = snapshot(d->account ? d->account->acct_email : nullptr);
	request->expected = snapshot(expected_hash);
	d->password_request = request.release();
	return true;
}
void password_async_cancel(P_desc d)
{
	delete d->password_request;
	d->password_request = nullptr;
}
bool password_async_pulse(P_desc d)
{
	if (!d->password_request)
		return false;
	auto *r = d->password_request;
	if (STATE(d) != r->state || d->account != r->account || d->character != r->character ||
	    r->name != snapshot(d->account ? d->account->acct_name : nullptr) ||
	    r->credential != snapshot(d->account ? d->account->acct_password : nullptr) ||
	    r->email != snapshot(d->account ? d->account->acct_email : nullptr) ||
	    (d->character && (d->character->in_room != r->room ||
			      (r->state == CON_PLAYING && !IS_ALIVE(d->character)))))
	{
		// An unfinished registration must never inherit a saved same-name account.
		// Only detach the original account; a replacement belongs to the new session.
		const bool failed_registration = r->account && r->credential.empty() &&
						 d->account == r->account;
		password_async_cancel(d);
		if (failed_registration)
		{
			d->account = free_account(d->account);
			STATE(d) = CON_GET_ACCT_NAME;
			if (d->websocket)
				ws_send_auth_failed(d, "Registration cancelled; please try again");
			else
			{
				echo_on(d);
				SEND_TO_Q(
					"Registration cancelled; please try again.\r\nAccount Name: ",
					d);
			}
			return true;
		}
		SEND_TO_Q("Password operation cancelled because your session changed.\r\n", d);
		return true;
	}
	int valid = 0;
	char *hash = nullptr;
	if (!password_login_poll(r->job, r->expected.c_str(), &valid, &hash))
		return true;
	std::unique_ptr<password_request> completed(r);
	d->password_request = nullptr;
	completed->finish(d, valid, hash);
	free(hash);
	return true;
}
