#include "persistence/operation_log.h"
#include "player/player_save_worker.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
enum class admission : uint8_t
{
	pending,
	durable,
	aborted,
	uncertain
};
struct retained_operation
{
	critical_command command;
	critical_apply_fn project;
	void *context;
	uint64_t generation;
	std::atomic<admission> state{ admission::pending };
};

std::mutex admission_mutex;
std::shared_mutex lifetime_mutex;
critical_apply_fn projector = nullptr;
void *projector_context = nullptr;
std::atomic<bool> ready{ false };
std::atomic<size_t> pending{ 0 };
uint64_t writer_owner = 0;
uint64_t generation = 0;
std::string receipt_directory;
std::unordered_map<std::string, std::shared_ptr<retained_operation>> admitted;

std::string identity(const critical_operation_id &id)
{
	char hex[CRITICAL_COMMAND_ID_HEX_SIZE]{};
	if (!critical_operation_id_to_hex(id, hex, sizeof(hex)))
		return {};
	return hex;
}

// Store a bounded receipt, not another full player snapshot/item tree. A hash
// covers the complete request and a second digest protects the receipt itself.
bool request_digest(critical_command command, std::array<uint8_t, SHA256_DIGEST_LENGTH> *digest)
{
	command.accepted_at_usec = 1;
	if (command.type != critical_command_type::player_checkpoint)
	{
		command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		command.outcome_revision = 0;
		command.outcome_payload.clear();
	}
	std::vector<uint8_t> encoded;
	if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return false;
	SHA256(encoded.data(), encoded.size(), digest->data());
	return true;
}

bool retain_receipt(const critical_command &command, unsigned int error_code = 0)
{
	critical_operation_receipt receipt;
	receipt.operation_id = command.operation_id;
	receipt.outcome_revision = command.outcome_revision;
	receipt.outcome_payload = command.outcome_payload;
	receipt.error_code = error_code;
	if (!request_digest(command, &receipt.request_digest))
		return false;
	std::vector<uint8_t> bytes;
	return critical_operation_receipt_encode(receipt, &bytes) &&
	       flatfile_atomic_write(receipt_directory, identity(command.operation_id), bytes,
				     nullptr);
}

critical_receipt_status lookup_locked(const critical_command &request,
				      critical_apply_result *receipt)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> expected{};
	if (!receipt || !request_digest(request, &expected))
		return critical_receipt_status::unavailable;
	const auto key = identity(request.operation_id);
	const auto pending_receipt = admitted.find(key);
	if (pending_receipt != admitted.end())
	{
		const auto &command = pending_receipt->second->command;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> actual{};
		if (!request_digest(command, &actual))
			return critical_receipt_status::unavailable;
		if (actual != expected)
			return critical_receipt_status::identity_conflict;
		*receipt = { critical_apply_outcome::already_applied, command.outcome_revision, 0 };
		receipt->result_size = command.outcome_payload.size();
		std::copy(command.outcome_payload.begin(), command.outcome_payload.end(),
			  receipt->result_payload.begin());
		return critical_receipt_status::found;
	}
	struct stat status
	{
	};
	if (lstat((receipt_directory + "/" + key).c_str(), &status))
		return errno == ENOENT ? critical_receipt_status::missing :
					 critical_receipt_status::unavailable;
	if (!S_ISREG(status.st_mode) || status.st_uid != geteuid() || status.st_nlink != 1 ||
	    (status.st_mode & 0077))
		return critical_receipt_status::unavailable;
	std::vector<uint8_t> bytes;
	const auto read =
		flatfile_read(receipt_directory, key, CRITICAL_RECEIPT_MAX_BYTES, &bytes, nullptr);
	if (read == flatfile_read_result::not_found)
		return critical_receipt_status::missing;
	critical_operation_receipt stored;
	if (read != flatfile_read_result::ok ||
	    !critical_operation_receipt_decode(bytes.data(), bytes.size(), &stored) ||
	    !critical_operation_id_equal(request.operation_id, stored.operation_id))
		return critical_receipt_status::unavailable;
	if (expected != stored.request_digest)
		return critical_receipt_status::identity_conflict;
	*receipt = { stored.error_code ? critical_apply_outcome::terminal_failure :
					 critical_apply_outcome::already_applied,
		     stored.outcome_revision, stored.error_code };
	receipt->result_size = stored.outcome_payload.size();
	std::copy(stored.outcome_payload.begin(), stored.outcome_payload.end(),
		  receipt->result_payload.begin());
	return critical_receipt_status::found;
}

bool replay(critical_command command, void *)
{
	// Never load a partly recovered world. A failure leaves this record and
	// every later record intact for the next recovery attempt.
	critical_apply_result receipt{};
	const auto found = lookup_locked(command, &receipt);
	if (found == critical_receipt_status::unavailable ||
	    found == critical_receipt_status::identity_conflict)
		return false;
	if (command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION)
	{
		// A pre-cutover journal contains an accepted request, not a RAM
		// decision. Reconcile it under its original database identity before
		// adopting the resulting receipt; never submit a different schema to
		// the materializer for an existing operation ID.
		const auto result = projector(command, projector_context);
		const bool rejected = result.outcome == critical_apply_outcome::terminal_failure &&
				      result.error_code;
		if ((!rejected && result.outcome != critical_apply_outcome::applied &&
		     result.outcome != critical_apply_outcome::already_applied) ||
		    (!rejected && (result.error_code || !result.result_size)) ||
		    result.result_size > result.result_payload.size())
			return false;
		command.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
		command.outcome_revision = result.durable_revision;
		command.outcome_payload.assign(result.result_payload.begin(),
					       result.result_payload.begin() + result.result_size);
		if (rejected)
			return (found != critical_receipt_status::found ||
				(receipt.error_code == result.error_code &&
				 receipt.durable_revision == result.durable_revision &&
				 receipt.result_size == result.result_size &&
				 std::equal(command.outcome_payload.begin(),
					    command.outcome_payload.end(),
					    receipt.result_payload.begin()))) &&
			       retain_receipt(command, result.error_code) &&
			       critical_command_journal_checkpoint(command.operation_id) ==
				       critical_command_journal_result::ok;
		return (found != critical_receipt_status::found ||
			operation_log_result_matches(command, receipt)) &&
		       retain_receipt(command) &&
		       critical_command_journal_checkpoint(command.operation_id) ==
			       critical_command_journal_result::ok;
	}
	if (found == critical_receipt_status::found &&
	    !operation_log_result_matches(command, receipt))
		return false;
	const auto result = projector(command, projector_context);
	return operation_log_result_matches(command, result) && retain_receipt(command) &&
	       critical_command_journal_checkpoint(command.operation_id) ==
		       critical_command_journal_result::ok;
}

player_save_apply_result project(const std::shared_ptr<retained_operation> &operation)
try
{
	// Shutdown cannot release the journal lease or projector context while a
	// callback is running. A queued callback from an older lifetime cannot touch
	// a subsequently opened journal, even if the writer was detached at shutdown.
	std::shared_lock<std::shared_mutex> lifetime(lifetime_mutex);
	if (operation->generation != generation || !projector)
		return { player_save_apply_outcome::retryable_failure, 0, ESHUTDOWN };
	const auto state = operation->state.load();
	if (state == admission::aborted)
		return { player_save_apply_outcome::terminal_failure, 0, EIO };
	if (state != admission::durable)
		return { player_save_apply_outcome::retryable_failure, 0, EAGAIN };
	const auto result = operation->project(operation->command, operation->context);
	if (!operation_log_result_matches(operation->command, result))
	{
		if (result.outcome != critical_apply_outcome::retryable_failure &&
		    result.outcome != critical_apply_outcome::ambiguous_commit)
			ready = false;
		// Do not deliver a rejection to gameplay or let a following save pass
		// a durable operation that the materialized base cannot reproduce.
		return { player_save_apply_outcome::retryable_failure, 0,
			 result.error_code ? result.error_code : EILSEQ };
	}
	// No exception after durable admission may escape as a terminal writer
	// failure. In particular, receipt encoding allocates after projection has
	// committed, and a later write must not overtake an uncheckpointed record.
	const auto key = identity(operation->command.operation_id);
	if (!retain_receipt(operation->command) ||
	    critical_command_journal_checkpoint(operation->command.operation_id) !=
		    critical_command_journal_result::ok)
		return { player_save_apply_outcome::retryable_failure, 0, EIO };
	{
		std::lock_guard<std::mutex> lock(admission_mutex);
		admitted.erase(key);
		--pending;
	}
	return { player_save_apply_outcome::applied, result.durable_revision, 0 };
}
catch (const std::bad_alloc &)
{
	return { player_save_apply_outcome::retryable_failure, 0, ENOMEM };
}
catch (...)
{
	return { player_save_apply_outcome::retryable_failure, 0, EIO };
}
} // namespace

bool operation_log_result_matches(const critical_command &command,
				  const critical_apply_result &result) noexcept
{
	return (result.outcome == critical_apply_outcome::applied ||
		result.outcome == critical_apply_outcome::already_applied) &&
	       command.schema_version == CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION &&
	       critical_command_outcome_matches(command, result.durable_revision, result.error_code,
						result.result_payload.data(), result.result_size);
}

bool operation_log_init(const char *directory, critical_apply_fn callback, void *context)
{
	std::unique_lock<std::shared_mutex> lifetime(lifetime_mutex);
	std::lock_guard<std::mutex> lock(admission_mutex);
	if (projector || !callback || !critical_command_journal_init(directory))
		return false;
	projector = callback;
	projector_context = context;
	++generation;
	try
	{
		receipt_directory = std::string(directory) + "/receipts";
		if (mkdir(receipt_directory.c_str(), 0700) != 0 && errno != EEXIST)
			return false;
		struct stat status
		{
		};
		if (lstat(receipt_directory.c_str(), &status) || !S_ISDIR(status.st_mode) ||
		    status.st_uid != geteuid() || (status.st_mode & 0077))
			return false;
		const int fd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
		if (fd < 0)
			return false;
		const bool synced = fsync(fd) == 0;
		const bool closed = close(fd) == 0;
		if (!synced || !closed)
			return false;
		ready = critical_command_journal_replay(replay, nullptr) ==
			critical_command_journal_result::ok;
	}
	catch (...)
	{
		ready = false;
	}
	return ready;
}

static critical_submit_result submit(const critical_command &command,
				     const player_snapshot *snapshot)
{
	std::lock_guard<std::mutex> lock(admission_mutex);
	if (!ready)
		return critical_submit_result::unavailable;
	if (command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION ||
	    !critical_command_valid(command))
		return critical_submit_result::invalid;
	std::shared_ptr<retained_operation> operation;
	std::string key;
	try
	{
		critical_apply_result prior{};
		const auto status = lookup_locked(command, &prior);
		if (status == critical_receipt_status::found)
			return operation_log_result_matches(command, prior) ?
				       critical_submit_result::attached :
				       critical_submit_result::identity_conflict;
		if (status == critical_receipt_status::identity_conflict)
			return critical_submit_result::identity_conflict;
		if (status == critical_receipt_status::unavailable)
		{
			ready = false;
			return critical_submit_result::unavailable;
		}
		// Everything the projector will retain is allocated before the append.
		operation = std::make_shared<retained_operation>();
		operation->command = command;
		operation->project = projector;
		operation->context = projector_context;
		operation->generation = generation;
		key = identity(command.operation_id);
		admitted.emplace(key, operation);
		const size_t bytes =
			sizeof(retained_operation) + command.payload.size() +
			command.outcome_payload.size() +
			command.keys.size() * sizeof(critical_entity_key) +
			command.expected_revisions.size() * sizeof(critical_expected_revision);
		const auto submitted =
			snapshot ?
				player_save_worker_submit_logged(*snapshot, [operation]
								 { return project(operation); }) :
				persistence_writer_submit(persistence_job_kind::critical,
							  ++writer_owner, bytes, [operation]
							  { return project(operation); });
		if (submitted != player_save_submit_result::accepted)
		{
			admitted.erase(key);
			return critical_submit_result::overloaded;
		}
	}
	catch (...)
	{
		admitted.erase(key);
		return critical_submit_result::overloaded;
	}
	const auto appended = critical_command_journal_append(command);
	if (appended == critical_command_journal_result::ok)
	{
		++pending;
		operation->state = admission::durable;
		return critical_submit_result::accepted;
	}
	if (appended == critical_command_journal_result::append_uncertain)
	{
		// Restart/recovery resolves an ambiguous append. Neither gameplay nor
		// later database writes may escape this boundary in the current process.
		ready = false;
		++pending;
		operation->state = admission::uncertain;
		return critical_submit_result::journal_uncertain;
	}
	operation->state = admission::aborted;
	admitted.erase(key);
	return critical_submit_result::journal_failure;
}

critical_submit_result operation_log_submit(const critical_command &command)
{
	return submit(command, nullptr);
}

critical_submit_result operation_log_submit_player(const critical_command &command,
						   const player_snapshot &snapshot)
{
	return submit(command, &snapshot);
}

bool operation_log_ready()
{
	return ready;
}
bool operation_log_remember_rejection(const critical_command &request,
				      const critical_apply_result &result)
{
	std::shared_lock<std::shared_mutex> lifetime(lifetime_mutex);
	std::lock_guard<std::mutex> lock(admission_mutex);
	if (!ready || request.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    !critical_command_valid(request) ||
	    result.outcome != critical_apply_outcome::terminal_failure || !result.error_code ||
	    result.result_size > result.result_payload.size())
		return false;
	try
	{
		critical_apply_result receipt;
		const auto found = lookup_locked(request, &receipt);
		if (found == critical_receipt_status::found)
			return receipt.error_code == result.error_code &&
			       receipt.durable_revision == result.durable_revision &&
			       receipt.result_size == result.result_size &&
			       std::equal(result.result_payload.begin(),
					  result.result_payload.begin() + result.result_size,
					  receipt.result_payload.begin());
		if (found != critical_receipt_status::missing)
			return false;
		auto stored = request;
		stored.outcome_revision = result.durable_revision;
		stored.outcome_payload.assign(result.result_payload.begin(),
					      result.result_payload.begin() + result.result_size);
		return retain_receipt(stored, result.error_code);
	}
	catch (...)
	{
		return false;
	}
}
bool operation_log_projection_caught_up()
{
	return ready && !pending.load();
}

critical_receipt_status operation_log_lookup(const critical_command &request,
					     critical_apply_result *receipt)
{
	std::lock_guard<std::mutex> lock(admission_mutex);
	if (!ready)
		return critical_receipt_status::unavailable;
	try
	{
		const auto result = lookup_locked(request, receipt);
		if (result == critical_receipt_status::unavailable)
			ready = false;
		return result;
	}
	catch (...)
	{
		return critical_receipt_status::unavailable;
	}
}

void operation_log_shutdown()
{
	std::unique_lock<std::shared_mutex> lifetime(lifetime_mutex);
	std::lock_guard<std::mutex> lock(admission_mutex);
	ready = false;
	projector = nullptr;
	projector_context = nullptr;
	pending = 0;
	admitted.clear();
	receipt_directory.clear();
	++generation;
	critical_command_journal_shutdown();
}
