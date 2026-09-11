#include "core/prototypes.h"
#include "core/utils.h"
#include "item/locker_identify.h"
#include "item/locker_receipt.h"
#include "cmd/item_lore.h"
#include "economy/currency_transaction.h"
#include <cerrno>
#include <chrono>
#include <cstring>
#include <future>
#include <memory>
#include <optional>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
constexpr size_t maximum_pending = 16;
using clock_type = std::chrono::steady_clock;
enum class phase
{
	loading,
	preparing,
	submitting,
	payment,
	recording,
	showing
};
struct io_result
{
	flatfile_read_result outcome = flatfile_read_result::io_error;
	locker_receipt value;
};
struct request
{
	locker_receipt value;
	std::optional<locker_receipt> candidate;
	std::future<io_result> io;
	phase stage = phase::loading;
	clock_type::time_point retry{};
	bool warned = false;
};
struct payment_context
{
	uint32_t pid;
	critical_operation_id operation;
};
std::unordered_map<uint32_t, std::unique_ptr<request>> requests;
std::string receipt_directory;
int receipt_lock = -1;
bool enabled = false;

bool owner_matches(P_char ch, const locker_receipt &value)
{
	currency_command_payload payment = {};
	const char *account = ch ? get_account_name_safe(ch) : nullptr;
	return ch && account && currency_command_decode_payload(value.payment, &payment) &&
	       payment.pid == static_cast<uint32_t>(GET_PID(ch)) &&
	       payment.racewar == static_cast<uint8_t>(GET_RACEWAR(ch)) &&
	       !strcasecmp(account, payment.account_name.data());
}
void notice(P_char ch, request &entry)
{
	if (ch && !entry.warned)
	{
		send_to_char(
			"Your identification request could not finish. Use stat receipt at the lockers to check or recover it.\r\n",
			ch);
		entry.warned = true;
	}
}
void write_async(request &entry, phase next)
{
	entry.stage = next;
	entry.retry = clock_type::now() + std::chrono::seconds(1);
	try
	{
		entry.io = std::async(
			std::launch::async,
			[directory = receipt_directory, value = entry.value]() mutable
			{
				const bool saved = locker_receipt_write(directory, value);
				return io_result{ saved ? flatfile_read_result::ok :
							  flatfile_read_result::io_error,
						  std::move(value) };
			});
	}
	catch (...)
	{ /* The durable prepared command remains recoverable. */
	}
}
void paid(P_char ch, bool committed, const currency_command_result &, unsigned int error_code,
	  const uint8_t *context, size_t size)
{
	if (!context || size != sizeof(payment_context))
		return;
	payment_context payment{};
	memcpy(&payment, context, sizeof(payment));
	const auto found = requests.find(payment.pid);
	if (found == requests.end() ||
	    !critical_operation_id_equal(found->second->value.payment.operation_id,
					 payment.operation))
		return;
	auto &entry = *found->second;
	if (entry.stage != phase::payment)
		return; // duplicate completion cannot replace an in-flight receipt write
	if (committed)
		entry.value.state = locker_receipt_state::paid;
	else if (error_code == ENOSPC || error_code == ESTALE)
		entry.value.state = locker_receipt_state::failed; // definite no-mutation outcomes
	else
	{
		// Unknown/malformed publication is not proof that the payment failed.
		// Preserve the original operation for ledger reconciliation on retry.
		notice(ch, entry);
		entry.stage = phase::submitting;
		entry.retry = clock_type::now() + std::chrono::seconds(5);
		return;
	}
	write_async(entry, phase::recording);
}
void enqueue(P_char ch, std::optional<locker_receipt> candidate)
{
	if (!ch || IS_NPC(ch) || GET_PID(ch) <= 0)
		return;
	const uint32_t pid = static_cast<uint32_t>(GET_PID(ch));
	if (!enabled || requests.size() >= maximum_pending || requests.count(pid))
	{
		if (candidate)
			send_to_char("The locker clerk is busy. Please try again shortly.\r\n", ch);
		return;
	}
	try
	{
		auto entry = std::make_unique<request>();
		entry->candidate = std::move(candidate);
		auto &stored = *requests.emplace(pid, std::move(entry)).first->second;
		stored.io = std::async(std::launch::async,
				       [directory = receipt_directory, pid]
				       {
					       io_result result;
					       result.outcome = locker_receipt_read(directory, pid,
										    &result.value);
					       return result;
				       });
	}
	catch (...)
	{
		requests.erase(pid);
		send_to_char("The locker identification service is temporarily unavailable.\r\n",
			     ch);
	}
}
}

bool locker_identify_init(const char *journal_directory)
{
	if (enabled || !journal_directory || !*journal_directory)
		return enabled;
	receipt_directory = std::string(journal_directory) + "/locker-identification";
	if (mkdir(receipt_directory.c_str(), 0700) && errno != EEXIST)
		return false;
	// The receipt directory itself must survive before a payment can be accepted.
	const int parent = open(journal_directory, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
	if (parent < 0)
		return false;
	const bool synced = fsync(parent) == 0;
	close(parent);
	std::string error;
	enabled = synced &&
		  flatfile_lock_acquire(receipt_directory, ".service-lock", &receipt_lock, &error);
	return enabled;
}
void locker_identify_shutdown()
{
	enabled = false;
	for (auto &[pid, entry] : requests)
	{
		(void)pid;
		if (entry->io.valid())
			entry->io.wait();
	}
	requests.clear();
	flatfile_lock_release(receipt_lock);
	receipt_lock = -1;
}
void locker_identify_replay(P_char ch)
{
	enqueue(ch, std::nullopt);
}
void locker_identify_pulse()
{
	// At most 16 requests/futures; disk work never executes in this loop.
	for (auto it = requests.begin(); it != requests.end();)
	{
		request &entry = *it->second;
		P_char ch = find_player_by_pid(it->first);
		const bool online = ch && ch->desc && ch->desc->connected == CON_PLAYING;
		if (entry.io.valid())
		{
			if (entry.io.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			{
				++it;
				continue;
			}
			io_result result;
			try
			{
				result = entry.io.get();
			}
			catch (...)
			{
			}
			if (entry.stage == phase::loading)
			{
				if (result.outcome != flatfile_read_result::ok &&
				    result.outcome != flatfile_read_result::not_found)
				{
					notice(ch, entry);
					it = requests.erase(it);
					continue;
				}
				if (result.outcome == flatfile_read_result::ok && ch &&
				    !owner_matches(ch, result.value))
				{
					notice(ch, entry);
					it = requests.erase(it);
					continue;
				}
				if (entry.candidate &&
				    (result.outcome == flatfile_read_result::not_found ||
				     result.value.state != locker_receipt_state::prepared))
				{
					entry.value = std::move(*entry.candidate);
					entry.candidate.reset();
					write_async(entry, phase::preparing);
				}
				else if (result.outcome == flatfile_read_result::not_found)
				{
					it = requests.erase(it);
					continue;
				}
				else
				{
					if (entry.candidate && ch)
						send_to_char(
							"Your previous identification is being recovered. Request the new item after it completes.\r\n",
							ch);
					entry.candidate.reset();
					entry.value = std::move(result.value);
					entry.stage =
						entry.value.state ==
								locker_receipt_state::prepared ?
							phase::submitting :
							phase::showing;
				}
			}
			else if (result.outcome == flatfile_read_result::ok)
			{
				entry.stage = entry.stage == phase::preparing ? phase::submitting :
										phase::showing;
				entry.retry = {};
			}
			else
				notice(ch,
				       entry); // retry writing; do not charge or discard uncertain evidence
		}
		if (!online && !entry.io.valid())
		{
			it = requests.erase(it);
			continue;
		} // the exact request is retained on disk
		if (entry.stage == phase::showing)
		{
			if (owner_matches(ch, entry.value))
			{
				if (entry.value.state == locker_receipt_state::paid)
				{
					send_to_char(
						"Locker identification receipt (already paid):\r\n",
						ch);
					send_to_char(entry.value.text.c_str(), ch);
				}
				else
					send_to_char(
						"The identification payment failed; no identification was purchased.\r\n",
						ch);
			}
			it = requests.erase(it);
			continue;
		}
		if (!entry.io.valid() && clock_type::now() >= entry.retry)
		{
			if (entry.stage == phase::preparing || entry.stage == phase::recording)
				write_async(entry, entry.stage);
			else if (entry.stage == phase::submitting && online &&
				 owner_matches(ch, entry.value))
			{
				const payment_context context{ it->first,
							       entry.value.payment.operation_id };
				entry.stage = phase::payment;
				if (!currency_transaction_submit_prepared(ch, entry.value.payment,
									  paid, &context,
									  sizeof(context)))
				{
					entry.stage = phase::submitting;
					entry.retry = clock_type::now() + std::chrono::seconds(1);
					notice(ch, entry);
				}
			}
		}
		++it;
	}
}

void locker_identify(P_char ch, P_obj obj, int cost)
{
	if (!ch || !obj || IS_NPC(ch) || !OBJ_CARRIED_BY(obj, ch))
		return;
	if (IS_SET(obj->extra_flags, ITEM_NOIDENTIFY) && GET_LEVEL(ch) < 50)
	{
		send_to_char("That item cannot be identified.\r\n", ch);
		return;
	}
	if (IS_FIGHTING(ch))
	{
		send_to_char("You can't concentrate on that right now.\r\n", ch);
		return;
	}
	try
	{
		locker_receipt receipt;
		if (!currency_transaction_prepare_identify(ch, cost, &receipt.payment))
		{
			send_to_char(
				"The identification payment is unavailable; check your purse or bank and try again.\r\n",
				ch);
			return;
		}
		receipt.text =
			"The member of the &+YStorage Locker Safety Commission&n takes " +
			std::to_string(cost) + " copper worth of coins.\r\n" +
			"The member of the &+YStorage Locker Safety Commission&n says 'This is:'\r\n" +
			item_lore_description(ch, obj);
		if (receipt.text.size() > LOCKER_RECEIPT_TEXT_MAX)
		{
			send_to_char("That item's description is too large to identify.\r\n", ch);
			return;
		}
		enqueue(ch, std::move(receipt));
	}
	catch (...)
	{
		send_to_char("The locker identification service is temporarily unavailable.\r\n",
			     ch);
	}
}
