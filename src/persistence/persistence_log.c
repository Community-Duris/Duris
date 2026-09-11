#include "persistence/persistence_log.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
struct log_record
{
	std::array<char, PERSISTENCE_LOG_RECORD_BYTES> text{};
	time_t created = 0;
};

struct log_worker
{
	std::array<log_record, PERSISTENCE_LOG_CAPACITY> queue{};
	std::mutex mutex;
	std::condition_variable available, drained;
	size_t head = 0, count = 0;
	bool writing = false;
	std::string file_path, wiz_path;
	std::atomic<uint64_t> accepted{ 0 }, completed{ 0 }, file_failures{ 0 }, wiz_failures{ 0 };
};

// Intentionally process-lifetime storage: a stuck filesystem must not cause an
// unbounded static-destructor join or a use-after-free after a drain timeout.
log_worker *worker = nullptr;
std::atomic<uint64_t> rejected{ 0 };

bool append_record(const std::string &path, const char *line, size_t length)
try
{
	std::error_code error;
	const auto parent = std::filesystem::path(path).parent_path();
	if (!parent.empty())
	{
		std::filesystem::create_directories(parent, error);
		if (error)
			return false;
	}
	// Never call logit: it has shared formatting state and a recursive fallback.
	// Opening for each record follows rename/create rotation without stale handles.
	const int fd =
		open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC | O_NONBLOCK, 0644);
	if (fd < 0)
		return false;
	struct stat status = {};
	bool success = fstat(fd, &status) == 0 && S_ISREG(status.st_mode);
	if (success)
	{
		// One append, no replay after a partial/ambiguous failure (which could
		// duplicate a record). Failures are counted independently for each sink.
		success = write(fd, line, length) == static_cast<ssize_t>(length);
	}
	if (close(fd) != 0)
		success = false;
	return success;
}

catch (...)
{
	return false;
}

void run(log_worker *state)
{
	for (;;)
	{
		log_record record;
		{
			std::unique_lock<std::mutex> lock(state->mutex);
			state->available.wait(lock, [state] { return state->count != 0; });
			record = state->queue[state->head];
			state->head = (state->head + 1) % PERSISTENCE_LOG_CAPACITY;
			--state->count;
			state->writing = true;
		}
		char timestamp[64]{};
		struct tm local = {};
		localtime_r(&record.created, &local);
		strftime(timestamp, sizeof(timestamp), "%a %b %d %H:%M:%S %Y", &local);
		char line[PERSISTENCE_LOG_RECORD_BYTES + 96];
		const int length = snprintf(line, sizeof(line), "%s::PERSISTENCE: %s\n", timestamp,
					    record.text.data());
		if (length < 0 || static_cast<size_t>(length) >= sizeof(line))
		{
			++state->file_failures;
			++state->wiz_failures;
		}
		else
		{
			if (!append_record(state->file_path, line, static_cast<size_t>(length)))
				++state->file_failures;
			if (!append_record(state->wiz_path, line, static_cast<size_t>(length)))
				++state->wiz_failures;
		}
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			++state->completed;
			state->writing = false;
		}
		state->drained.notify_all();
	}
}
}

bool persistence_log_start(const char *file_path, const char *wiz_path)
{
	if (worker)
		return true;
	try
	{
		auto *state = new log_worker;
		try
		{
			state->file_path = file_path;
			state->wiz_path = wiz_path;
			std::thread thread(run, state);
			thread.detach();
		}
		catch (...)
		{
			delete state;
			throw;
		}
		worker = state;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool persistence_log_submit(const char *record)
{
	const size_t length = record ? strnlen(record, PERSISTENCE_LOG_RECORD_BYTES) : 0;
	if (!worker || !length || length >= PERSISTENCE_LOG_RECORD_BYTES)
	{
		++rejected;
		return false;
	}
	std::unique_lock<std::mutex> lock(worker->mutex, std::try_to_lock);
	if (!lock.owns_lock() || worker->count == PERSISTENCE_LOG_CAPACITY)
	{
		++rejected;
		return false;
	}
	auto &slot = worker->queue[(worker->head + worker->count) % PERSISTENCE_LOG_CAPACITY];
	memcpy(slot.text.data(), record, length + 1);
	slot.created = time(nullptr);
	++worker->count;
	++worker->accepted;
	lock.unlock();
	worker->available.notify_one();
	return true;
}

persistence_log_metrics persistence_log_snapshot()
{
	if (!worker)
		return { 0, 0, rejected.load(), 0, 0 };
	return { worker->accepted.load(), worker->completed.load(), rejected.load(),
		 worker->file_failures.load(), worker->wiz_failures.load() };
}

bool persistence_log_drain(unsigned timeout_ms)
{
	if (!worker)
		return true;
	std::unique_lock<std::mutex> lock(worker->mutex);
	return worker->drained.wait_for(lock, std::chrono::milliseconds(timeout_ms),
					[] { return worker->count == 0 && !worker->writing; });
}
