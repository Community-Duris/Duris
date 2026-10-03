#ifndef DURIS_NETWORK_READINESS_H
#define DURIS_NETWORK_READINESS_H

#include "core/structs.h"
#include "net/websocket.h"
#include <gnutls/gnutls.h>
#include <poll.h>
#include <stdint.h>

/* Distinguish orderly input EOF from transport/protocol failure. */
constexpr int NETWORK_INPUT_EOF = -2;

inline bool network_has_unoffered_input(P_desc descriptor)
{
	const size_t allowance = descriptor->websocket ? WS_INPUT_BUFFER_SIZE :
							 MAX_QUEUE_LENGTH - 1;
	return descriptor->network_input_remaining < allowance ||
	       (descriptor->websocket && descriptor->ws_pending_application);
}

inline bool network_transport_pending(P_desc descriptor)
{
	return descriptor->telnet_output_offset < descriptor->telnet_output_len ||
	       descriptor->ws_output_offset < descriptor->ws_output_len ||
	       descriptor->ws_control_output_offset < descriptor->ws_control_output_len;
}

inline short network_read_interest(P_desc descriptor)
{
	// A retained TLS send owns its retry. Register only that operation's
	// direction; unread plaintext cannot spin the loop while a send is blocked.
	if (descriptor->telnet_tls_retry ||
	    (descriptor->connected != CON_SSLNEGO && !descriptor->network_input_remaining) ||
	    (descriptor->websocket && websocket_input_paused(descriptor)))
		return 0;
	return descriptor->tls_read_interest ? descriptor->tls_read_interest : POLLIN;
}

inline short network_write_interest(P_desc descriptor)
{
	if (descriptor->connected == CON_SSLNEGO || !network_transport_pending(descriptor))
		return 0;
	return descriptor->tls_write_interest ? descriptor->tls_write_interest : POLLOUT;
}

inline bool network_buffered_input(P_desc descriptor)
{
	return (descriptor->ws_input_pending && !websocket_input_paused(descriptor)) ||
	       (descriptor->sslses && descriptor->network_input_remaining &&
		descriptor->connected != CON_SSLNEGO && !descriptor->telnet_tls_retry &&
		descriptor->tls_read_interest != POLLOUT &&
		gnutls_record_check_pending(descriptor->sslses) > 0);
}

inline int network_timeout_ms(uint64_t deadline_us, uint64_t now_us)
{
	if (now_us >= deadline_us)
		return 0;
	const uint64_t remaining = deadline_us - now_us;
	// Round up to avoid sub-millisecond spinning. Never sleep past a pulse by
	// more than poll's millisecond granularity (plus OS scheduling latency).
	return static_cast<int>((remaining > OPT_USEC ? OPT_USEC : remaining) + 999) / 1000;
}

inline uint64_t network_next_pulse_us(uint64_t started_us, uint64_t finished_us)
{
	const uint64_t deadline_us = started_us + OPT_USEC;
	// Missed wall-clock slots are discarded. One world pulse advances one
	// logical tick; scheduler callback debt remains owned by ne_events().
	return finished_us >= deadline_us ? finished_us + OPT_USEC : deadline_us;
}

#endif
