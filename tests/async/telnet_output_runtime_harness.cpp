#include "core/structs.h"
#include "net/mccp.h"
#include <gnutls/gnutls.h>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <zlib.h>

extern "C"
{
	long sentbytes = 0;
}
void logit(const char *, const char *, ...) {}
void panic_corruption(const char *, const char *, ...)
{
	abort();
}
char *json_escape_ansi_string(const char *)
{
	abort();
}
int websocket_send_text(P_desc, const char *)
{
	abort();
}
int write_to_descriptor(P_desc, const char *);
int raw_write_to_descriptor(P_desc, const char *, int);
int write_to_descriptor_binary(P_desc, const unsigned char *, size_t);
static std::string delivered, retry_record;
static std::vector<int> results;
static size_t call_index;
static bool tls;

static ssize_t send_bytes(const void *data, size_t len)
{
	if (tls && !retry_record.empty())
	{
		assert(data == nullptr && len == 0);
		data = retry_record.data();
		len = retry_record.size();
	}
	int result = call_index < results.size() ? results[call_index++] : 16384;
	if (result < 0)
	{
		if (tls)
		{
			if (retry_record.empty())
				retry_record.assign((const char *)data, len);
			return result;
		}
		errno = -result;
		return -1;
	}
	size_t count = std::min(len, (size_t)result);
	delivered.append((const char *)data, count);
	retry_record.clear();
	return count;
}
extern "C" ssize_t gnutls_record_send(gnutls_session_t, const void *data, size_t len)
{
	return send_bytes(data, len);
}
extern "C" const char *gnutls_strerror(int)
{
	return "injected TLS failure";
}
extern "C" ssize_t __wrap_write(int, const void *data, size_t len)
{
	return send_bytes(data, len);
}
static void reset(bool use_tls, std::vector<int> sequence)
{
	delivered.clear();
	retry_record.clear();
	results = sequence;
	call_index = 0;
	tls = use_tls;
	sentbytes = 0;
}
int main()
{
	std::string banner;
	for (int i = 0; i < 50000; ++i)
		banner += (char)('a' + i % 26);
	for (bool use_tls : { false, true })
	{
		descriptor_data d{};
		if (use_tls)
			d.sslses = (gnutls_session_t)1;
		reset(use_tls, {});
		assert(raw_write_to_descriptor(&d, banner.data(), banner.size()) == 0);
		assert(delivered == banner && sentbytes == (long)banner.size());
		assert(!d.telnet_output_buffer && !d.telnet_output_len);

		reset(use_tls, { 101, use_tls ? GNUTLS_E_AGAIN : -EAGAIN,
				 use_tls ? GNUTLS_E_INTERRUPTED : -EINTR, 7, 0, 13 });
		assert(raw_write_to_descriptor(&d, banner.data(), banner.size()) == 0);
		assert(delivered == banner.substr(0, 101));
		assert(raw_write_to_descriptor(&d, "\0END", 4) == 0);
		assert(d.telnet_output_len && !d.write_failed);
		for (int tick = 0; tick < 10 && d.telnet_output_len; ++tick)
			assert(telnet_flush_output(&d) == 0);
		assert(delivered == banner + std::string("\0END", 4));
		assert(sentbytes == (long)delivered.size() && !d.telnet_output_buffer);

		reset(use_tls, { use_tls ? GNUTLS_E_AGAIN : -EAGAIN });
		assert(raw_write_to_descriptor(&d, banner.data(), banner.size()) == 0);
		std::string excess(1024 * 1024, 'x');
		assert(raw_write_to_descriptor(&d, excess.data(), excess.size()) < 0);
		assert(d.write_failed && telnet_flush_output(&d) < 0);
		telnet_free_output(&d);
		assert(!d.telnet_output_buffer && !d.telnet_output_len && !d.telnet_tls_retry);

		d = {};
		if (use_tls)
			d.sslses = (gnutls_session_t)1;
		reset(use_tls, { use_tls ? GNUTLS_E_PUSH_ERROR : -EPIPE });
		assert(raw_write_to_descriptor(&d, "fatal", 5) < 0 && d.write_failed);
		telnet_free_output(&d);

		d = {};
		if (use_tls)
			d.sslses = (gnutls_session_t)1;
		reset(use_tls, { 5, use_tls ? GNUTLS_E_AGAIN : -EAGAIN,
				 use_tls ? GNUTLS_E_AGAIN : -EAGAIN });
		z_stream stream{};
		assert(deflateInit(&stream, Z_DEFAULT_COMPRESSION) == Z_OK);
		char compressed[COMPRESS_BUF_SIZE];
		d.out_compress = MCCP_VER2;
		d.z_str = &stream;
		d.out_compress_buf = compressed;
		assert(write_to_descriptor_binary(&d, (const unsigned char *)banner.data(),
						  banner.size()) == 0);
		assert(write_to_descriptor_binary(&d, (const unsigned char *)"tail", 4) == 0);
		for (int tick = 0; tick < 10 && d.telnet_output_len; ++tick)
			assert(telnet_flush_output(&d) == 0);
		assert(!d.telnet_output_len);
		z_stream decoder{};
		assert(inflateInit(&decoder) == Z_OK);
		std::string decoded(banner.size() + 100, '\0');
		decoder.next_in = (Bytef *)delivered.data();
		decoder.avail_in = delivered.size();
		decoder.next_out = (Bytef *)decoded.data();
		decoder.avail_out = decoded.size();
		assert(inflate(&decoder, Z_SYNC_FLUSH) == Z_OK);
		decoded.resize(decoder.total_out);
		assert(decoded == banner + "tail");
		inflateEnd(&decoder);
		deflateEnd(&stream);
		telnet_free_output(&d);
	}
	for (bool use_cp437 : { false, true })
	{
		descriptor_data d{};
		d.sslses = (gnutls_session_t)1;
		d.cp437 = use_cp437;
		reset(true, { 19, GNUTLS_E_AGAIN });
		std::string large, expected;
		for (int i = 0; i < 30000; ++i)
		{
			large += "abc\r\né\n";
			expected += use_cp437 ? "abc\r\n\x82\r\n" : "abc\r\né\r\n";
		}
		assert(write_to_descriptor(&d, large.c_str()) == 0);
		assert(telnet_flush_output(&d) == 0);
		assert(delivered == expected && !d.telnet_output_buffer);
		std::string oversized(1024 * 1024 + 1, 'a');
		assert(write_to_descriptor(&d, oversized.c_str()) < 0 && d.write_failed);
		telnet_free_output(&d);
	}
	puts("Telnet/TLS short-write, backpressure, failure, cleanup and MCCP tests passed");
}
