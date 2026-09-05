#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "net/mccp.h"
#include <errno.h>
#include <gnutls/gnutls.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "persistence/copyover.h"
#include "net/gmcp.h"
#include "core/json_utils.h"
#include "net/telnet.h"
#include "net/ttype.h"
#include "net/unicode.h"
#include "net/websocket.h"

/* external variables used by this module */
extern P_desc descriptor_list;
extern long sentbytes;

static constexpr size_t telnet_output_limit = 1024 * 1024;

/* global variables provided by this module */
int mccp_alloc = 0;
int mccp_free = 0;

const unsigned char compress_on_str[] = { IAC, WILL, TELOPT_COMPRESS };
const unsigned char compress2_on_str[] = { IAC, WILL, TELOPT_COMPRESS2 };
const unsigned char enable_compress[] = { IAC, SB, TELOPT_COMPRESS, WILL, SE };
const unsigned char enable_compress2[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE };
const unsigned char sga_will_str[] = { IAC, WILL, TELOPT_SGA };
const unsigned char ga_str[] = { IAC, GA };

void *zlib_alloc(void *opaque, unsigned int items, unsigned int size);
void zlib_free(void *opaque, void *address);
int raw_write_to_descriptor(P_desc desc, const char *txt, const int total);

void *zlib_alloc(void * /*opaque*/, unsigned int items, unsigned int size)
{
	char *p;
	mccp_alloc++;
	CREATE(p, char, items *size, MEM_TAG_ZSTREAM);
	return (void *)p;
}

void zlib_free(void * /*opaque*/, void *address)
{
	mccp_free++;
	FREE(address);
}

void advertise_mccp(P_desc desc)
{
	write_to_descriptor_binary(desc, compress2_on_str, sizeof compress2_on_str);
	write_to_descriptor_binary(desc, compress_on_str, sizeof compress_on_str);
}

void sga_negotiate(P_desc desc)
{
	write_to_descriptor_binary(desc, sga_will_str, 3);
}

int send_ga(P_desc desc)
{
	if (desc && !desc->sga_disabled && !desc->websocket)
		return write_to_descriptor_binary(desc, ga_str, 2);
	return 0;
}

/* parse telnet options and return amount of characters
 * to "cut" from input stream
 * If you ever need to make it "the right way", look into
 * sources of telnetd demon */
int parse_telnet_options(P_desc player, char *buf, int buflen)
{
	ubyte *p = (ubyte *)(buf);

	if (buflen < 1 || *p != IAC)
		return 0;
	if (buflen < 2)
		return 0;

	switch (*(p + 1))
	{
	case IAC: // ignore escaped 255 byte: illegal in UTF-8, redundant in CP437
		return 2;
	case DO:
		if (buflen < 3)
			return 0;
		switch (*(p + 2))
		{
		case TELOPT_COMPRESS:
			compress_start(player, MCCP_VER1);
			return 3;
		case TELOPT_COMPRESS2:
			compress_start(player, MCCP_VER2);
			return 3;
		case TELOPT_GMCP:
			gmcp_handle_negotiation(player, DO);
			return 3;
		case TELOPT_SGA:
			player->sga_disabled = 1;
			return 3;
		}
		return 3;
	case DONT:
		if (buflen < 3)
			return 0;
		switch (*(p + 2))
		{
		case TELOPT_GMCP:
			gmcp_handle_negotiation(player, DONT);
			return 3;
		case TELOPT_SGA:
			player->sga_disabled = 0;
			return 3;
		}
		return 3;
	case WILL:
		if (buflen < 3)
			return 0;
		if (*(p + 2) == TELOPT_TTYPE)
			ttype_handle_negotiation(player, WILL);
		return 3;
	case WONT:
		if (buflen < 3)
			return 0;
		if (*(p + 2) == TELOPT_TTYPE)
			ttype_handle_negotiation(player, WONT);
		return 3;
	case SB: /* subnegotiation */
	{
		if (buflen < 3)
			return 0;
		int len = 3;
		while (len + 1 < buflen && !(p[len] == IAC && p[len + 1] == SE))
			len++;

		/* incomplete, wait for more */
		if (len + 1 >= buflen)
			return 0;

		len += 2; /* include IAC SE */

		if (p[2] == TELOPT_TTYPE)
		{
			if (p[3] == TELQUAL_IS)
			{
				ttype_handle_subnegotiation(player, p + 3, len - 5);
			}
			else
			{
				player->ttype_state = TTYPE_COMPLETE;
				return len;
			}
		}
		/* If GMCP subnegotiation, pass data to handler */
		else if (p[2] == TELOPT_GMCP && len > 5)
		{
			gmcp_handle_input(player, (const char *)(p + 3), len - 5);
		}
		return len;
	}
	}

	return 1; /* lets cut at least IAC from stream */
}

int compress_start(P_desc player, int mccp_version)
{
	z_stream *s;

	if (!player || player->websocket)
		return 0;
	if (player->z_str)
		return 0;

	CREATE(s, z_stream, 1, MEM_TAG_ZSTREAM);
	// s = (z_stream *) malloc(sizeof(z_stream));
	CREATE(player->out_compress_buf, char, COMPRESS_BUF_SIZE, MEM_TAG_BUFFER);
	// player->out_compress_buf = (char *) malloc(COMPRESS_BUF_SIZE);

	s->next_in = NULL;
	s->avail_in = 0;
	s->next_out = (Bytef *)player->out_compress_buf;
	s->avail_out = COMPRESS_BUF_SIZE;
	s->zalloc = zlib_alloc;
	s->zfree = zlib_free;
	s->opaque = NULL;

	if (deflateInit(s, COMPRESS_EFFICIENCY) != Z_OK)
	{
		FREE(player->out_compress_buf);
		FREE(s);
		logit(LOG_DEBUG, "MCCP: deflateInit failed");
		return -1;
	}

	if (mccp_version == MCCP_VER1)
	{
		write_to_descriptor_binary(player, enable_compress, sizeof enable_compress);
	}
	else if (mccp_version == MCCP_VER2)
	{
		write_to_descriptor_binary(player, enable_compress2, sizeof enable_compress2);
	}
	else
	{
		logit(LOG_DEBUG, "MCCP: unknown version %d", mccp_version);
	}
	player->out_compress = mccp_version;
	player->z_str = s;

	return 0;
}

/* ZMUD seems not to handle Z_FINISH event properly, so socket needs
 to be closed immediatly after stopping compression! */
int compress_end(P_desc player, int flush)
{
	unsigned char dummy[1] = { ' ' };
	int status, len;

	if (!player->out_compress || !player->z_str)
		return 0;

	player->z_str->avail_in = 0;
	player->z_str->next_in = dummy;
	player->z_str->next_out = (Bytef *)player->out_compress_buf;
	player->z_str->avail_out = COMPRESS_BUF_SIZE;

	/* flush all pending data, Z_OK means there's still more data to process */
	if (flush)
	{
		do
		{
			status = deflate(player->z_str, Z_FINISH);
			if (status != Z_STREAM_END && status != Z_OK)
			{
				break;
			}
			len = (long)player->z_str->next_out - (long)player->out_compress_buf;
			if (raw_write_to_descriptor(player, player->out_compress_buf, len) < 0)
				break;
		} while (status != Z_STREAM_END);
	}

	deflateEnd(player->z_str); /* free memory allocated by zlib */
	FREE(player->out_compress_buf);
	FREE(player->z_str);
	player->out_compress = 0;

	return 0;
}

/* use this function whenever you want to send anything to player,
 do not attempt to call raw_write_to_descriptor, or you may
 screw up compression */
int write_to_descriptor(P_desc player, const char *txt)
{
	if (!player || !txt)
		return -1;
	if (player->write_failed)
		return -1;

	/* WebSocket connections need JSON-wrapped text frames */
	if (player->websocket)
	{
		int result;
		char *escaped = json_escape_ansi_string(txt);
		if (!escaped)
			return -1;
		if (escaped[0] == '\0')
		{
			free(escaped);
			return 0;
		}

		/* Skip empty messages */
		char *json_msg = NULL;
		size_t msg_len = strlen(escaped) + 64;
		json_msg = (char *)malloc(msg_len);
		if (!json_msg)
		{
			free(escaped);
			return -1;
		}
		int written = snprintf(json_msg, msg_len,
				       "{\"type\":\"text\",\"category\":\"info\",\"data\":\"%s\"}",
				       escaped);
		if (written < 0 || (size_t)written >= msg_len)
		{
			free(json_msg);
			free(escaped);
			return -1;
		}
		result = websocket_send_text(player, json_msg);
		free(json_msg);
		free(escaped);
		return result;
	}

	size_t input_len = strnlen(txt, telnet_output_limit + 1);
	if (input_len > telnet_output_limit)
	{
		logit(LOG_COMM, "Telnet text exceeded %zu bytes", telnet_output_limit);
		player->write_failed = 1;
		return -1;
	}
	std::string converted;
	converted.reserve(input_len * 2);
	for (size_t i = 0; i < input_len; ++i)
	{
		if (txt[i] == '\n')
			converted += "\r\n";
		else if (txt[i] != '\r')
			converted += txt[i];
	}
	if (player->cp437)
	{
		/* Downgrading emits at most one byte per input byte, plus its terminator. */
		std::string downgraded(converted.size() + 1, '\0');
		downgrade_string(downgraded.data(), converted.c_str(), u_cp437);
		return write_to_descriptor_binary(player, (const unsigned char *)downgraded.data(),
						  strlen(downgraded.c_str()));
	}
	return write_to_descriptor_binary(player, (const unsigned char *)converted.data(),
					  converted.size());
}

/* Retain wire bytes, including compressed output, until the transport accepts them. */
void telnet_free_output(P_desc d)
{
	free(d->telnet_output_buffer);
	d->telnet_output_buffer = NULL;
	d->telnet_output_len = 0;
	d->telnet_output_offset = 0;
	d->telnet_tls_retry = 0;
}

int telnet_flush_output(P_desc d)
{
	if (d->write_failed)
		return -1;
	while (d->telnet_output_offset < d->telnet_output_len)
	{
		const unsigned char *data = d->telnet_output_buffer + d->telnet_output_offset;
		size_t remaining = d->telnet_output_len - d->telnet_output_offset;
		ssize_t written;
		if (d->sslses)
		{
			/* GnuTLS owns the interrupted record; resume it before sending new bytes. */
			written = gnutls_record_send(d->sslses, d->telnet_tls_retry ? NULL : data,
						     d->telnet_tls_retry ? 0 : remaining);
			if (written == GNUTLS_E_AGAIN || written == GNUTLS_E_INTERRUPTED)
			{
				d->telnet_tls_retry = 1;
				return 0;
			}
			d->telnet_tls_retry = 0;
			if (written < 0)
				logit(LOG_COMM, "Write to SSL socket error: %s (ret=%zd)",
				      gnutls_strerror(written), written);
		}
		else
		{
			written = write(d->descriptor, data, remaining);
			if (written < 0 && (errno == EAGAIN || errno == EINTR
#if EWOULDBLOCK != EAGAIN
					    || errno == EWOULDBLOCK
#endif
					    ))
				return 0;
			if (written < 0)
				logit(LOG_COMM, "Write to socket error: %s (errno=%d)",
				      strerror(errno), errno);
		}
		if (written < 0 || (size_t)written > remaining)
		{
			d->write_failed = 1;
			return -1;
		}
		if (written == 0)
			return 0;
		d->telnet_output_offset += written;
		sentbytes += written;
		if (d->character && !IS_NPC(d->character))
			d->character->only.pc->send_data += written;
	}
	telnet_free_output(d);
	return 0;
}

/* Only the binary/compression layer may enqueue transport bytes here. */
int raw_write_to_descriptor(P_desc d, const char *txt, const int total)
{
	if (d->write_failed || total < 0)
		return -1;
	size_t pending = d->telnet_output_len - d->telnet_output_offset;
	/* Bound memory for clients that stop reading, without silently dropping output. */
	if ((size_t)total > telnet_output_limit - pending)
	{
		logit(LOG_COMM, "Telnet output queue exceeded %zu bytes", telnet_output_limit);
		d->write_failed = 1;
		return -1;
	}
	if (total)
	{
		if (d->telnet_output_offset)
			memmove(d->telnet_output_buffer,
				d->telnet_output_buffer + d->telnet_output_offset, pending);
		d->telnet_output_offset = 0;
		d->telnet_output_len = pending;
		unsigned char *buffer =
			(unsigned char *)realloc(d->telnet_output_buffer, pending + total);
		if (!buffer)
		{
			d->write_failed = 1;
			return -1;
		}
		d->telnet_output_buffer = buffer;
		memcpy(buffer + pending, txt, total);
		d->telnet_output_len += total;
	}
	return telnet_flush_output(d);
}

/* Write binary data (like telnet subnegotiations) with compression support
 * but without text conversions (no \n->\r\n, no charset downgrade) */
int write_to_descriptor_binary(P_desc player, const unsigned char *data, size_t len)
{
	int status;
	long out_len;

	if (!player || !data || len == 0)
		return 0;
	if (player->websocket)
		return 0; /* WebSocket output must always use framed send APIs. */

	if (player->write_failed)
		return -1;

	if (!player->out_compress)
	{
		if (raw_write_to_descriptor(player, (const char *)data, len) < 0)
			return -1;
	}
	else
	{
		if (player->z_str)
		{
			player->z_str->next_in = (unsigned char *)data;
			player->z_str->avail_in = len;

			while (player->z_str->avail_in)
			{
				do
				{
					player->z_str->next_out = (Bytef *)player->out_compress_buf;
					player->z_str->avail_out = COMPRESS_BUF_SIZE;

					status = deflate(player->z_str, Z_SYNC_FLUSH);
					if (status != Z_OK)
					{
						return -1;
					}

					out_len = (long)player->z_str->next_out -
						  (long)player->out_compress_buf;
					if (raw_write_to_descriptor(
						    player, player->out_compress_buf, out_len) < 0)
						return -1;
				} while (player->z_str->avail_out == 0);
			}
		}
	}
	return 0;
}

int compress_get_ratio(P_desc player)
{
	if (!player->z_str)
		return 0;

	if (player->z_str->total_in == 0 || player->z_str->total_out == 0)
		return 0;

	return (player->z_str->total_in - player->z_str->total_out) * 100 / player->z_str->total_in;
}
