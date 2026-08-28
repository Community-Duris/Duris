#include "redis_connection.h"

#include <hiredis/hiredis_ssl.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <mutex>
#include <new>
#include <string>
#include <sys/un.h>
#include <sys/time.h>

struct redis_connection_settings
{
	std::string host;
	std::string unix_socket;
	int port = 0;
	int connect_timeout_msec = 0;
	int command_timeout_msec = 0;
	int database = 0;
	std::string username;
	std::string password;
	bool tls = false;
	std::string ca_cert;
	std::string server_name;
	SSL_CTX *ssl_context = nullptr;
};

namespace
{
std::once_flag openssl_initialized;

bool status_ok(redisReply *reply)
{
	const bool ok = reply && reply->type == REDIS_REPLY_STATUS && reply->str &&
			strcmp(reply->str, "OK") == 0;
	if (reply)
		freeReplyObject(reply);
	return ok;
}
} // namespace

redis_connection_settings *
redis_connection_settings_create(const struct redis_connection_options *options)
{
	if (!options)
		return nullptr;
	const bool tcp = options->host && *options->host;
	const bool unix_socket = options->unix_socket && *options->unix_socket;
	if (tcp == unix_socket || (tcp && (options->port <= 0 || options->port > 65535)) ||
	    (unix_socket && (options->unix_socket[0] != '/' ||
			     strlen(options->unix_socket) >= sizeof(sockaddr_un::sun_path))) ||
	    (unix_socket && (options->tls || options->require_tls)) ||
	    options->connect_timeout_msec <= 0 || options->command_timeout_msec <= 0 ||
	    options->database < 0 || options->database > 255 ||
	    (options->username && *options->username &&
	     (!options->password || !*options->password)) ||
	    (options->tls && (!options->ca_cert || !*options->ca_cert)) ||
	    (options->require_tls && !options->tls))
		return nullptr;

	redis_connection_settings *settings = nullptr;
	try
	{
		settings = new redis_connection_settings;
		settings->host = tcp ? options->host : "";
		settings->unix_socket = unix_socket ? options->unix_socket : "";
		settings->port = options->port;
		settings->connect_timeout_msec = options->connect_timeout_msec;
		settings->command_timeout_msec = options->command_timeout_msec;
		settings->database = options->database;
		settings->username = options->username ? options->username : "";
		settings->password = options->password ? options->password : "";
		settings->tls = options->tls;
		settings->ca_cert = options->ca_cert ? options->ca_cert : "";
		settings->server_name = tcp && options->server_name && *options->server_name ?
						options->server_name :
						settings->host;
	}
	catch (const std::bad_alloc &)
	{
		delete settings;
		return nullptr;
	}

	if (settings->tls)
	{
		std::call_once(openssl_initialized, redisInitOpenSSL);
		settings->ssl_context = SSL_CTX_new(TLS_client_method());
		if (!settings->ssl_context ||
		    SSL_CTX_set_min_proto_version(settings->ssl_context, TLS1_2_VERSION) != 1 ||
		    SSL_CTX_load_verify_locations(settings->ssl_context, settings->ca_cert.c_str(),
						  nullptr) != 1)
		{
			redis_connection_settings_destroy(settings);
			return nullptr;
		}
		SSL_CTX_set_verify(settings->ssl_context, SSL_VERIFY_PEER, nullptr);
	}
	return settings;
}

void redis_connection_settings_destroy(struct redis_connection_settings *settings)
{
	if (!settings)
		return;
	if (settings->ssl_context)
		SSL_CTX_free(settings->ssl_context);
	delete settings;
}

redisContext *redis_connection_open(const struct redis_connection_settings *settings)
{
	return redis_connection_open_with_timeout(settings, 0);
}

redisContext *redis_connection_open_with_timeout(const struct redis_connection_settings *settings,
						 int minimum_command_timeout_msec)
{
	if (!settings)
		return nullptr;
	const int command_timeout_msec =
		std::max(settings->command_timeout_msec, minimum_command_timeout_msec);
	struct timeval connect_timeout = { settings->connect_timeout_msec / 1000,
					   (settings->connect_timeout_msec % 1000) * 1000 };
	struct timeval command_timeout = { command_timeout_msec / 1000,
					   (command_timeout_msec % 1000) * 1000 };
	redisContext *context =
		settings->unix_socket.empty() ?
			redisConnectWithTimeout(settings->host.c_str(), settings->port,
						connect_timeout) :
			redisConnectUnixWithTimeout(settings->unix_socket.c_str(), connect_timeout);
	if (!context || context->err)
		return context;
	if (settings->tls)
	{
		SSL *ssl = SSL_new(settings->ssl_context);
		unsigned char address[sizeof(struct in6_addr)] = {};
		const bool numeric_address =
			inet_pton(AF_INET, settings->server_name.c_str(), address) == 1 ||
			inet_pton(AF_INET6, settings->server_name.c_str(), address) == 1;
		X509_VERIFY_PARAM *verify = ssl ? SSL_get0_param(ssl) : nullptr;
		if (!ssl || !verify ||
		    SSL_set_tlsext_host_name(ssl, settings->server_name.c_str()) != 1 ||
		    (numeric_address ?
			     X509_VERIFY_PARAM_set1_ip_asc(verify, settings->server_name.c_str()) !=
				     1 :
			     (X509_VERIFY_PARAM_set_hostflags(verify,
							      X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS),
			      X509_VERIFY_PARAM_set1_host(verify, settings->server_name.c_str(),
							  0) != 1)) ||
		    redisInitiateSSL(context, ssl) != REDIS_OK)
		{
			if (ssl && !context->privctx)
				SSL_free(ssl);
			redisFree(context);
			return nullptr;
		}
	}
	if (redisSetTimeout(context, command_timeout) != REDIS_OK)
	{
		redisFree(context);
		return nullptr;
	}

	redisReply *reply = nullptr;
	if (!settings->username.empty())
		reply = (redisReply *)redisCommand(context, "AUTH %b %b", settings->username.data(),
						   settings->username.size(),
						   settings->password.data(),
						   settings->password.size());
	else if (!settings->password.empty())
		reply = (redisReply *)redisCommand(context, "AUTH %b", settings->password.data(),
						   settings->password.size());
	if (reply && !status_ok(reply))
	{
		redisFree(context);
		return nullptr;
	}
	if ((!settings->username.empty() || !settings->password.empty()) && !reply)
	{
		if (context->err)
			return context;
		redisFree(context);
		return nullptr;
	}

	reply = (redisReply *)redisCommand(context, "SELECT %d", settings->database);
	if (!reply && context->err)
		return context;
	if (!status_ok(reply))
	{
		redisFree(context);
		return nullptr;
	}
	return context;
}
