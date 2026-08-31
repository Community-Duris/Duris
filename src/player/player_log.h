#ifndef __PLAYER_LOG_H__
#define __PLAYER_LOG_H__

#include <list>
#include <string>
#include <vector>
using namespace std;

#define PUBLIC_LOG_SIZE 600
#define PRIVATE_LOG_SIZE 200

#define LOG_NONE -1
#define LOG_PUBLIC 0
#define LOG_PRIVATE 1
#define NUM_LOGS 2

#define ITERATE_LOG(ch, log)                                                 \
	for (PlayerLog::iterator _log_iter = GET_PLAYER_LOG(ch)->begin(log); \
	     _log_iter != GET_PLAYER_LOG(ch)->end(log); _log_iter++)

#define ITERATE_LOG_LIMIT(ch, log, limit)                                           \
	for (PlayerLog::iterator _log_iter = GET_PLAYER_LOG(ch)->begin(log, limit); \
	     _log_iter != GET_PLAYER_LOG(ch)->end(log); _log_iter++)

#define ITERATE_LOG_REVERSE(ch, log)                                                  \
	for (PlayerLog::reverse_iterator _log_iter = GET_PLAYER_LOG(ch)->rbegin(log); \
	     _log_iter != GET_PLAYER_LOG(ch)->rend(log); _log_iter++)

#define LOG_MSG() ((_log_iter)->c_str())

class PlayerLog
{
    public:
	PlayerLog()
	{
		log_size[LOG_PUBLIC] = 0;
		log_size[LOG_PRIVATE] = 0;
		log_max_size[LOG_PUBLIC] = PUBLIC_LOG_SIZE;
		log_max_size[LOG_PRIVATE] = PRIVATE_LOG_SIZE;
	}

	~PlayerLog() { clear(); }

	void clear()
	{
		for (int i = 0; i < NUM_LOGS; i++)
		{
			logs[i].clear();
			log_size[i] = 0;
		}
	}

	void write(int log, const char *str)
	{
		check_log_index(log);
		logs[log].push_back(string(str));
		if (log_size[log] >= log_max_size[log])
		{
			logs[log].pop_front();
		}
		else
		{
			log_size[log]++;
		}
	}

	const char *read(int log, int bytes)
	{
		check_log_index(log);

		if (read_buf.size() > 0)
			read_buf.clear();

		list<string> strings;
		int size = 0;
		for (reverse_iterator it = rbegin(log); it != rend(log); it++)
		{
			size += it->length();
			if (size >= bytes)
				break;

			strings.push_back(*it);
		}

		for (list<string>::reverse_iterator it = strings.rbegin(); it != strings.rend();
		     it++)
		{
			read_buf += (*it);
		}

		return read_buf.c_str();
	}

	/* forward iteration */
	typedef list<string>::iterator iterator;

	iterator begin(int log)
	{
		check_log_index(log);
		return logs[log].begin();
	}

	iterator begin(int log, int limit)
	{
		check_log_index(log);
		if (limit > log_size[log])
			return logs[log].begin();

		int offset = log_size[log] - limit;

		list<string>::iterator it;
		int i = 0;
		for (it = logs[log].begin(); it != logs[log].end() && i < offset; it++, i++)
			;

		return it;
	}

	iterator end(int log)
	{
		check_log_index(log);
		return logs[log].end();
	}

	/* reverse iteration */
	typedef list<string>::reverse_iterator reverse_iterator;

	reverse_iterator rbegin(int log)
	{
		check_log_index(log);
		return logs[log].rbegin();
	}

	reverse_iterator rend(int log)
	{
		check_log_index(log);
		return logs[log].rend();
	}

    private:
	void check_log_index(int log)
	{
		if (log < 0 || log >= NUM_LOGS)
		{
			throw "Invalid log index!";
			return;
		}
	}

	list<string> logs[NUM_LOGS];
	int log_size[NUM_LOGS];
	int log_max_size[NUM_LOGS];
	string read_buf;
};

#endif // __PLAYER_LOG_H__
