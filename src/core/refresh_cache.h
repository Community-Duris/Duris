#ifndef DURIS_REFRESH_CACHE_H
#define DURIS_REFRESH_CACHE_H

#include <chrono>
#include <future>
#include <memory>
#include <string>

// Only the loader runs on a worker. All methods and publication belong to the
// game thread. One outstanding refresh bounds work; failed loads retain data.
template <class T> class refresh_cache
{
    public:
	using loader = bool (*)(T &, std::string &);
	bool request(loader load)
	{
		if (pending.valid())
		{
			followup = load;
			return true;
		}
		try
		{
			pending = std::async(std::launch::async,
					     [load]
					     {
						     result next;
						     try
						     {
							     next.value = std::make_unique<T>();
							     if (!load(*next.value, next.error))
								     next.value.reset();
						     }
						     catch (...)
						     {
							     next.value.reset();
							     next.error = "content load failed";
						     }
						     return next;
					     });
			return true;
		}
		catch (...)
		{
			error = "could not start content refresh";
			return false;
		}
	}
	bool poll()
	{
		if (!pending.valid() ||
		    pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			return false;
		auto next = pending.get();
		if (followup)
		{
			const auto load = followup;
			followup = nullptr;
			// A refresh requested after this load started supersedes its result.
			request(load);
			return true;
		}
		error = std::move(next.error);
		if (next.value)
		{
			current = std::move(next.value);
			++generation;
		}
		return true;
	}
	void shutdown()
	{
		while (pending.valid())
		{
			pending.wait();
			poll();
		}
	}
	const T *get() const { return current.get(); }
	bool busy() const { return pending.valid(); }
	std::string status() const
	{
		return std::string(current ? "ready" : "unavailable") + ", generation " +
		       std::to_string(generation) + (busy() ? ", refreshing" : "") +
		       (followup ? ", follow-up queued" : "") + (error.empty() ? "" : ", " + error);
	}

    private:
	struct result
	{
		std::unique_ptr<T> value;
		std::string error;
	};
	std::unique_ptr<T> current;
	std::future<result> pending;
	loader followup = nullptr;
	std::string error;
	unsigned long generation = 0;
};

#endif
