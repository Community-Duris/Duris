#ifndef DURIS_NETWORK_WAKEUP_H
#define DURIS_NETWORK_WAKEUP_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* A process-lifetime, nonblocking hint channel. Completion queues remain the
 * authority and retain their game-thread publication boundaries. CLOEXEC makes
 * copyover reconstruct this channel; workers never receive descriptor pointers.
 * Header-local implementation also lets isolated worker harnesses use the real
 * notification path without linking the server's networking translation unit. */
struct network_wakeup_pipe
{
	int descriptors[2] = { -1, -1 };

	network_wakeup_pipe()
	{
		if (pipe(descriptors) < 0)
			return;
		for (int descriptor : descriptors)
		{
			if (fcntl(descriptor, F_SETFL, O_NONBLOCK) < 0 ||
			    fcntl(descriptor, F_SETFD, FD_CLOEXEC) < 0)
			{
				close(descriptors[0]);
				close(descriptors[1]);
				descriptors[0] = descriptors[1] = -1;
				return;
			}
		}
	}
};

inline network_wakeup_pipe &network_wakeup_channel()
{
	// Keep both ends open until process exit, including worker static teardown.
	static network_wakeup_pipe channel;
	return channel;
}

inline int network_wakeup_fd()
{
	return network_wakeup_channel().descriptors[0];
}

inline void network_wakeup_notify()
{
	const int saved_errno = errno;
	const unsigned char hint = 1;
	const int descriptor = network_wakeup_channel().descriptors[1];
	if (descriptor >= 0)
		while (write(descriptor, &hint, sizeof(hint)) < 0 && errno == EINTR)
		{
		}
	// EAGAIN means a wakeup is already pending. No queue result is discarded.
	errno = saved_errno;
}

inline void network_wakeup_drain()
{
	const int saved_errno = errno;
	unsigned char hints[512];
	for (int attempt = 0; attempt < 16; ++attempt)
	{
		const ssize_t received = read(network_wakeup_fd(), hints, sizeof(hints));
		if (received < 0 && errno == EINTR)
			continue;
		if (received <= 0)
			break;
	}
	errno = saved_errno;
}

#endif
