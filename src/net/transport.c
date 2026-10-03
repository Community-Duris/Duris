/* Persistent client transport; the child alone authenticates and mutates the world. */
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/mm.h"
#include "core/game_loop_watchdog.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "net/mccp.h"
#include "net/network_readiness.h"
#include "net/transport.h"
#include "net/transport_protocol.h"
#include "net/ttype.h"
#include "net/websocket.h"
#include "persistence/copyover.h"
#include "persistence/latency_trace.h"
#include <chrono>
#include <climits>
#include <fcntl.h>
#include <map>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <poll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <vector>

extern P_desc descriptor_list;
extern struct mm_ds *dead_desc_pool;
extern int used_descs, avail_descs;
extern int shutdownflag, _reboot;
extern int new_descriptor(int, int);
extern int init_socket(int);
extern int process_input(P_desc);
extern int process_output(P_desc);
extern int ssl_negotiate(gnutls_session_t);
extern void nonblock(int);
extern unsigned long long ne_event_tick;

namespace
{
using namespace duris_transport;
using steady = std::chrono::steady_clock;
channel ipc_link;
bool frontend = false;
bool is_world = false;
bool paused = false;
bool pause_received = false;
bool pause_allowed = false;
bool commit_received = false;
bool welcome_received = false;
bool backend_ready = false;
bool committed = false;
uint64_t epoch = 0;
uint64_t barrier = 0;
size_t frontend_input_bytes = 0;
std::string handoff_digest;
steady::time_point pause_deadline;
volatile sig_atomic_t frontend_signal = 0;

struct metadata
{
	std::string host, client, client_version, terminal;
	unsigned websocket = 0, gmcp = 0, sga = 0, mtts = 0, ttype = 0, cp437 = 0, tls = 0,
		 mccp = 0;
	uint64_t revision = 0;
};
struct session_binding
{
	unsigned slot = 0, connected = 0, authenticated = 0, playing = 0, pid = 0;
	unsigned cp437 = 0, term = 0, verified = 0, backend = 0;
	std::string account, player, last_input, client, client_version, world_state;
	uint64_t metadata_revision = 0;
};
struct input_event
{
	uint64_t sequence = 0;
	unsigned kind = 0;
	std::string payload;
	bool sent = false;
};
struct session
{
	P_desc descriptor = nullptr;
	unsigned slot = 0;
	uint64_t id = 0, input_sequence = 0, ack = 0, output_sequence = 0;
	bool opened = false, close_sent = false, orderly_close = false;
	metadata meta;
	session_binding binding;
	std::deque<input_event> inputs;
	size_t input_bytes = 0;
};
struct restoration
{
	uint64_t id = 0, ack = 0, output_sequence = 0;
	metadata meta;
	session_binding binding;
	bool claimed = false;
};
std::map<uint64_t, session> sessions;
std::map<unsigned, restoration> handoff;
std::map<uint64_t, std::string> last_world_state;
std::map<uint64_t, uint64_t> metadata_revisions;
std::map<uint64_t, bool> tls_sessions;
std::map<uint64_t, message> world_pending_inputs;
size_t world_pending_input_bytes = 0;

uint64_t nonce()
{
	uint64_t value = 0;
	if (RAND_bytes(reinterpret_cast<unsigned char *>(&value), sizeof(value)) != 1 || !value)
		fatal_boot_error("transport", "Cannot generate session identity");
	return value;
}

bool send_message(type kind, uint64_t id = 0, uint64_t sequence = 0, unsigned detail = 0,
		  const std::string &payload = {})
{
	if (ipc_link.queue(kind, id, sequence, detail, payload))
		return true;
	logit(LOG_STATUS, "Persistent IPC send failed kind=%u bytes=%zu",
	      static_cast<unsigned>(kind), payload.size());
	ipc_link.failed = true;
	return false;
}

metadata read_metadata(P_desc d)
{
	return { d->host,
		 d->client_name,
		 d->client_version,
		 d->ttype_last,
		 static_cast<unsigned>(d->websocket),
		 static_cast<unsigned>(d->gmcp_enabled),
		 static_cast<unsigned>(d->sga_disabled),
		 static_cast<unsigned>(d->mtts_flags),
		 static_cast<unsigned>(d->ttype_state),
		 d->cp437,
		 static_cast<unsigned>(d->sslses != nullptr || transport_descriptor_tls(d)),
		 static_cast<unsigned>(d->out_compress),
		 metadata_revisions[d->transport_session] };
}
void encode_metadata(writer &w, const metadata &m)
{
	w.string(m.host);
	w.string(m.client);
	w.string(m.client_version);
	w.string(m.terminal);
	w.number(m.websocket, 1);
	w.number(m.gmcp, 1);
	w.number(m.sga, 1);
	w.number(m.mtts, 4);
	w.number(m.ttype, 1);
	w.number(m.cp437, 1);
	w.number(m.tls, 1);
	w.number(m.mccp, 1);
	w.number(m.revision);
}
metadata decode_metadata(reader &r)
{
	metadata m;
	m.host = r.string(49);
	m.client = r.string(63);
	m.client_version = r.string(31);
	m.terminal = r.string(127);
	m.websocket = r.number(1);
	m.gmcp = r.number(1);
	m.sga = r.number(1);
	m.mtts = r.number(4);
	m.ttype = r.number(1);
	m.cp437 = r.number(1);
	m.tls = r.number(1);
	m.mccp = r.number(1);
	m.revision = r.number();
	if (!m.revision || m.host.empty() || m.websocket > 1 || m.gmcp > 1 || m.sga > 1 ||
	    m.mtts > 65535 || m.ttype > 3 || m.cp437 > 1 || m.tls > 1 || m.mccp > MCCP_VER2 ||
	    (m.websocket && (m.tls || m.mccp)))
		r.valid = false;
	return m;
}
void apply_metadata(P_desc d, const metadata &m)
{
	auto &revision = metadata_revisions[d->transport_session];
	if (m.revision < revision)
		return; // Held input cannot rewind later Telnet negotiation or client metadata.
	revision = m.revision;
	tls_sessions[d->transport_session] = m.tls;
	strlcpy(d->host, m.host.c_str(), sizeof(d->host));
	strlcpy(d->client_name, m.client.c_str(), sizeof(d->client_name));
	strlcpy(d->client_version, m.client_version.c_str(), sizeof(d->client_version));
	strlcpy(d->ttype_last, m.terminal.c_str(), sizeof(d->ttype_last));
	d->websocket = m.websocket;
	d->gmcp_enabled = m.gmcp;
	d->sga_disabled = m.sga;
	d->mtts_flags = m.mtts;
	d->ttype_state = m.ttype;
	d->cp437 = m.cp437;
	d->out_compress = m.mccp; // Status only; compressor objects stay in the frontend.
	if (m.websocket)
	{
		d->ws_state = WS_STATE_OPEN;
		d->ws_handshake_done = 1;
	}
}

session_binding read_identity(P_desc d)
{
	session_binding b;
	b.slot = d->descriptor;
	b.connected = static_cast<unsigned char>(d->connected);
	b.authenticated = d->account != nullptr;
	b.playing = transport_descriptor_eligible(d);
	if (b.playing)
	{
		b.pid = GET_ID(d->character);
		b.account = d->account->acct_name;
		b.player = GET_NAME(d->character);
		// The frontend stores these opaque bytes without interpreting game state.
		// Durable world records remain the authoritative portable copyover codec.
		P_char ch = d->character;
		const auto deadline = ch->specials.wait_until_pulse;
		const auto lag =
			!CAN_ACT(ch) && deadline > ne_event_tick ? deadline - ne_event_tick : 0ULL;
		writer state;
		state.bytes = "DWS1";
		state.number(ch->specials.position, 1);
		state.number(d->wait > 0 ? d->wait : 0, 4);
		state.number(static_cast<unsigned short>(ch->specials.timer), 2);
		state.number(lag, 4);
		b.world_state = std::move(state.bytes);
	}
	b.last_input = d->last_input;
	b.cp437 = d->cp437;
	b.term = d->term_type;
	b.verified = d->durisweb_verified;
	b.backend = d->durisweb_backend;
	b.client = d->client_name;
	b.client_version = d->client_version;
	b.metadata_revision = metadata_revisions[d->transport_session];
	return b;
}
void encode_identity(writer &w, const session_binding &b)
{
	w.number(b.slot, 4);
	w.number(b.connected, 1);
	w.number(b.authenticated, 1);
	w.number(b.playing, 1);
	w.number(b.pid, 4);
	w.string(b.account);
	w.string(b.player);
	w.string(b.last_input);
	w.number(b.cp437, 1);
	w.number(b.term, 1);
	w.number(b.verified, 1);
	w.number(b.backend, 1);
	w.string(b.client);
	w.string(b.client_version);
	w.number(b.metadata_revision);
	w.string(b.world_state);
}
session_binding decode_identity(reader &r)
{
	session_binding b;
	b.slot = r.number(4);
	b.connected = r.number(1);
	b.authenticated = r.number(1);
	b.playing = r.number(1);
	b.pid = r.number(4);
	b.account = r.string(128);
	b.player = r.string(128);
	b.last_input = r.string(MAX_INPUT_LENGTH - 1);
	b.cp437 = r.number(1);
	b.term = r.number(1);
	b.verified = r.number(1);
	b.backend = r.number(1);
	b.client = r.string(63);
	b.client_version = r.string(31);
	b.metadata_revision = r.number();
	b.world_state = r.string(64);
	if (!b.metadata_revision || !b.slot || b.slot > session_limit || b.authenticated > 1 ||
	    b.playing > 1 || b.cp437 > 1 || b.verified > 1 || b.backend > 1 ||
	    (b.playing && (!b.authenticated || !b.pid || b.account.empty() || b.player.empty() ||
			   b.world_state.empty())))
		r.valid = false;
	return b;
}
std::string encode_restoration(const restoration &s)
{
	writer w;
	w.number(s.id);
	w.number(s.ack);
	w.number(s.output_sequence);
	encode_metadata(w, s.meta);
	encode_identity(w, s.binding);
	return w.bytes;
}
P_desc find_descriptor(uint64_t id)
{
	for (P_desc d = descriptor_list; d; d = d->next)
		if (d->transport_session == id)
			return d;
	return nullptr;
}

bool digest_file(const char *path, std::string &result, bool make_durable)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	struct stat st
	{
	};
	if (fd < 0)
		return false;
	bool valid = !fstat(fd, &st) && S_ISREG(st.st_mode) && st.st_uid == geteuid() &&
		     !(st.st_mode & 0077) && st.st_size >= 0 && st.st_size <= 1024LL * 1024 * 1024;
	if (valid && make_durable)
		valid = fsync(fd) == 0;
	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	if (!ctx || EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1)
		valid = false;
	char data[16384];
	while (valid)
	{
		ssize_t n = read(fd, data, sizeof(data));
		if (n < 0 && errno == EINTR)
			continue;
		if (!n)
			break;
		if (n < 0 || EVP_DigestUpdate(ctx, data, n) != 1)
			valid = false;
	}
	unsigned char hash[EVP_MAX_MD_SIZE]{};
	unsigned length = 0;
	if (valid && (EVP_DigestFinal_ex(ctx, hash, &length) != 1 || length != 32))
		valid = false;
	if (valid)
		result.assign(reinterpret_cast<char *>(hash), length);
	EVP_MD_CTX_free(ctx);
	close(fd);
	if (valid && make_durable)
	{
		std::string parent = path;
		const size_t slash = parent.rfind('/');
		parent = slash == std::string::npos ? "." : parent.substr(0, slash);
		if (parent.empty())
			parent = "/";
		fd = open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
		valid = fd >= 0 && fsync(fd) == 0;
		if (fd >= 0)
			close(fd);
	}
	return valid;
}

void send_world_state(P_desc d)
{
	writer w;
	encode_identity(w, read_identity(d));
	if (last_world_state[d->transport_session] != w.bytes)
	{
		if (send_message(type::state, d->transport_session, 0, 0, w.bytes))
			last_world_state[d->transport_session] = std::move(w.bytes);
	}
}

bool consume_world_frame(const message &m)
{
	if (m.kind == type::snapshot && !welcome_received)
	{
		reader r{ m.payload };
		restoration s;
		s.id = r.number();
		s.ack = r.number();
		s.output_sequence = r.number();
		s.meta = decode_metadata(r);
		s.binding = decode_identity(r);
		if (!r.done() || !s.id || !s.binding.playing || handoff.count(s.binding.slot))
			return false;
		handoff.emplace(s.binding.slot, std::move(s));
		return handoff.size() <= session_limit;
	}
	if (m.kind == type::welcome && !welcome_received && m.sequence == epoch)
	{
		if (m.detail > 1 || (m.detail && m.payload.size() != 32) ||
		    (!m.detail && (!m.payload.empty() || !handoff.empty())))
			return false;
		handoff_digest = m.payload;
		welcome_received = true;
		return true;
	}
	if (!welcome_received)
		return false;
	if (m.kind == type::paused)
	{
		if (!barrier || m.session != epoch || m.sequence != barrier)
			return true; // A timed-out attempt cannot approve a later handoff.
		pause_received = true;
		pause_allowed = m.detail == 1;
		return m.detail <= 1;
	}
	if (m.kind == type::committed)
	{
		if (!barrier || m.session != epoch || m.sequence != barrier)
			return true;
		commit_received = true;
		return true;
	}
	P_desc d = find_descriptor(m.session);
	if (m.kind == type::open)
	{
		if (paused || d || !m.session)
			return false;
		reader r{ m.payload };
		metadata meta = decode_metadata(r);
		if (!r.done() || !m.detail || m.detail > session_limit)
			return false;
		for (P_desc existing = descriptor_list; existing; existing = existing->next)
			if (static_cast<unsigned>(existing->descriptor) == m.detail)
				return false;
		d = static_cast<P_desc>(mm_get(dead_desc_pool));
		if (!d)
			return false;
		memset(d, 0, sizeof(*d));
		d->descriptor = m.detail;
		d->transport_session = m.session;
		d->term_type = TERM_ANSI;
		d->wait = 1;
		apply_metadata(d, meta);
		d->next = descriptor_list;
		descriptor_list = d;
		++used_descs;
		comm_transport_greet(d);
		send_world_state(d);
		return true;
	}
	// A delayed close or input for a retired ID cannot attach to a reused slot.
	if (!d)
		return m.kind == type::close || m.kind == type::input || m.kind == type::metadata;
	if (m.kind == type::close)
	{
		if (m.detail > 1)
			return false;
		if (m.detail)
			// An orderly close follows any final input on this ordered channel.
			// Offer that input for one world pulse, as in the native network loop.
			d->network_close_pending = 1;
		else
			close_socket(d);
		return true;
	}
	if (m.kind == type::metadata)
	{
		reader r{ m.payload };
		metadata meta = decode_metadata(r);
		if (!r.done() || meta.websocket != static_cast<unsigned>(d->websocket))
			return false;
		apply_metadata(d, meta);
		return true;
	}
	if (m.kind != type::input || !m.sequence)
		return false;
	if (m.sequence <= d->transport_ack)
		return send_message(type::ack, m.session, d->transport_ack);
	if (d->transport_command == m.sequence)
		return true;
	if (d->transport_command || m.sequence != d->transport_ack + 1)
		return false;
	const auto pending = world_pending_inputs.find(m.session);
	if (pending != world_pending_inputs.end())
		return pending->second.sequence == m.sequence;
	if (d->input.head)
	{
		// Authentication/reconnect completion may enqueue a world-generated
		// look after the previous client event was ACKed. Keep the next event
		// unstarted until that queue clears, rather than executing it twice or
		// mistaking ordinary asynchronous completion for a protocol violation.
		if (m.payload.size() > frontend_input_limit - world_pending_input_bytes)
			return false;
		world_pending_input_bytes += m.payload.size();
		world_pending_inputs.emplace(m.session, m);
		return true;
	}
	reader r{ m.payload };
	metadata meta = decode_metadata(r);
	std::string input = r.string(payload_limit - 2048);
	if (!r.done() || meta.websocket != static_cast<unsigned>(d->websocket) ||
	    input.find('\0') != std::string::npos)
		return false;
	apply_metadata(d, meta);
	d->transport_command = m.sequence;
	d->transport_command_started = false;
	switch (static_cast<input_kind>(m.detail))
	{
	case input_kind::line:
		comm_transport_line(d, input.data());
		break;
	case input_kind::websocket:
		websocket_dispatch_message(d, input.data(), input.size());
		break;
	case input_kind::gmcp:
		gmcp_handle_input(d, input.data(), input.size());
		break;
	default:
		return false;
	}
	return true;
}

bool consume_world(const message &m)
{
	if (consume_world_frame(m))
		return true;
	logit(LOG_STATUS, "Persistent world rejected IPC kind=%u detail=%u sequence=%llu bytes=%zu",
	      static_cast<unsigned>(m.kind), m.detail, static_cast<unsigned long long>(m.sequence),
	      m.payload.size());
	return false;
}

bool wait_world(bool &condition, unsigned milliseconds = barrier_timeout_ms)
{
	const auto deadline = steady::now() + std::chrono::milliseconds(milliseconds);
	while (!condition && !ipc_link.failed && steady::now() < deadline)
	{
		ipc_link.flush();
		ipc_link.receive(consume_world);
		if (!condition)
		{
			pollfd p{ ipc_link.fd, POLLIN, 0 };
			poll(&p, 1, 20);
		}
	}
	return condition && !ipc_link.failed;
}

void update_frontend_binding(session &s, const session_binding &b)
{
	s.binding = b;
	if (!s.descriptor)
		return;
	P_desc d = s.descriptor;
	// Real frontend descriptors have no characters. Keep the world's state in
	// the binding; shared protocol diagnostics must never treat them as players.
	d->connected = b.connected == CON_FLUSH ? CON_FLUSH : CON_GET_ACCT_NAME;
	d->transport_authenticated = b.authenticated;
	d->term_type = b.term;
	d->durisweb_verified = b.verified;
	d->durisweb_backend = b.backend;
	if (b.metadata_revision == metadata_revisions[d->transport_session])
	{
		d->cp437 = b.cp437;
		strlcpy(d->client_name, b.client.c_str(), sizeof(d->client_name));
		strlcpy(d->client_version, b.client_version.c_str(), sizeof(d->client_version));
	}
	s.meta = read_metadata(d);
}

bool consume_frontend_frame(const message &m)
{
	if (m.kind == type::hello)
	{
		if (!m.sequence || (epoch && !committed) ||
		    m.detail != static_cast<unsigned>(committed))
			return false;
		epoch = m.sequence;
		backend_ready = false;
		for (const auto &[slot, s] : handoff)
		{
			(void)slot;
			if (!send_message(type::snapshot, s.id, 0, 0, encode_restoration(s)))
				return false;
		}
		return send_message(type::welcome, 0, epoch, committed ? 1 : 0, handoff_digest);
	}
	if (!epoch)
		return false;
	if (m.kind == type::ready)
	{
		if (m.sequence != epoch || backend_ready)
			return false;
		if (committed)
		{
			for (auto &[id, s] : sessions)
			{
				(void)id;
				s.close_sent = false;
				for (auto &event : s.inputs)
					event.sent = false;
			}
		}
		backend_ready = true;
		paused = false;
		committed = false;
		barrier = 0;
		handoff.clear();
		handoff_digest.clear();
		for (auto &[id, s] : sessions)
			if (s.opened && s.descriptor)
			{
				writer w;
				encode_metadata(w, read_metadata(s.descriptor));
				if (!send_message(type::metadata, id, 0, 0, w.bytes))
					return false;
			}
		return true;
	}
	if (m.kind == type::pause)
	{
		if (m.session != epoch || !m.sequence || paused || !backend_ready)
			return false;
		barrier = m.sequence;
		paused = true;
		pause_deadline = steady::now() + std::chrono::seconds(replacement_timeout_seconds);
		bool allowed = true;
		for (const auto &[id, s] : sessions)
		{
			(void)id;
			if (!s.opened || !s.binding.playing)
				allowed = false;
		}
		return send_message(type::paused, epoch, barrier, allowed ? 1 : 0);
	}
	if (m.kind == type::abort)
	{
		if (m.session != epoch || m.sequence != barrier)
			return true; // A stale abort must not resume a newer paused attempt.
		paused = false;
		committed = false;
		handoff.clear();
		handoff_digest.clear();
		return true;
	}
	if (m.kind == type::commit)
	{
		if (m.session != epoch || m.sequence != barrier || !paused || committed ||
		    m.payload.size() != 32)
			return false;
		std::string actual;
		if (!digest_file(copyover_state_file(), actual, false) || actual != m.payload)
			return false;
		handoff.clear();
		for (const auto &[id, s] : sessions)
			if (s.opened)
			{
				if (!s.binding.playing || handoff.count(s.slot))
					return false;
				handoff.emplace(s.slot, restoration{ id, s.ack, s.output_sequence,
								     s.meta, s.binding, false });
			}
		handoff_digest = actual;
		committed = true;
		return send_message(type::committed, epoch, barrier);
	}
	const auto found = sessions.find(m.session);
	if (found == sessions.end())
		return m.kind == type::close || m.kind == type::output || m.kind == type::state ||
		       m.kind == type::ack;
	session &s = found->second;
	if (m.kind == type::state)
	{
		reader r{ m.payload };
		session_binding b = decode_identity(r);
		if (!r.done() || b.slot != s.slot ||
		    (s.descriptor && b.metadata_revision > metadata_revisions[m.session]))
			return false;
		update_frontend_binding(s, b);
		return true;
	}
	if (m.kind == type::ack)
	{
		if (!s.descriptor)
		{
			if (m.sequence > s.input_sequence)
				return false;
			if (m.sequence > s.ack)
				s.ack = m.sequence;
			return true;
		}
		if (m.sequence <= s.ack)
			return true;
		if (s.inputs.empty() || !s.inputs.front().sent ||
		    m.sequence != s.inputs.front().sequence)
			return false;
		s.ack = m.sequence;
		s.input_bytes -= s.inputs.front().payload.size();
		frontend_input_bytes -= s.inputs.front().payload.size();
		s.inputs.pop_front();
		return true;
	}
	if (m.kind == type::close)
	{
		if (s.descriptor)
			s.descriptor->connected = CON_FLUSH;
		frontend_input_bytes -= s.input_bytes;
		sessions.erase(found);
		return true;
	}
	if (m.kind != type::output || !m.sequence)
		return false;
	if (m.sequence <= s.output_sequence)
		return true;
	if (m.sequence != s.output_sequence + 1)
		return false;
	s.output_sequence = m.sequence;
	if (!s.descriptor)
		return true;
	P_desc d = s.descriptor;
	int result = -1;
	switch (static_cast<output_kind>(m.detail))
	{
	case output_kind::text:
		result = write_to_descriptor(d, m.payload.c_str());
		break;
	case output_kind::binary:
		result = write_to_descriptor_binary(
			d, reinterpret_cast<const unsigned char *>(m.payload.data()),
			m.payload.size());
		break;
	case output_kind::ws_text:
		result = websocket_send_text(d, m.payload.c_str());
		break;
	case output_kind::ws_binary:
		result = websocket_send_binary(d, m.payload.data(), m.payload.size());
		break;
	case output_kind::ws_close:
		if (m.payload.size() >= 2)
			result = websocket_send_close(
				d,
				(static_cast<unsigned char>(m.payload[0]) << 8) |
					static_cast<unsigned char>(m.payload[1]),
				m.payload.c_str() + 2);
		break;
	}
	// Queue saturation disconnects this client; acknowledged text is never discarded silently.
	if (result < 0)
		d->write_failed = 1;
	size_t queued = 0;
	for (P_desc client = descriptor_list; client; client = client->next)
		queued += client->telnet_output_len + client->ws_output_len +
			  client->ws_control_output_len;
	if (queued > frontend_output_limit)
		d->write_failed = 1;
	return true;
}

bool consume_frontend(const message &m)
{
	if (consume_frontend_frame(m))
		return true;
	logit(LOG_STATUS,
	      "Persistent frontend rejected IPC kind=%u detail=%u sequence=%llu bytes=%zu",
	      static_cast<unsigned>(m.kind), m.detail, static_cast<unsigned long long>(m.sequence),
	      m.payload.size());
	return false;
}

bool frontend_deliver_input(session &s)
{
	if (s.inputs.empty() || s.inputs.front().sent)
		return true;
	input_event &event = s.inputs.front();
	if (!send_message(type::input, s.id, event.sequence, event.kind, event.payload))
		return false;
	event.sent = true;
	return true;
}

void frontend_deliver_close(session &s)
{
	if (s.opened && !s.close_sent && backend_ready && !paused && !ipc_link.failed)
		s.close_sent = send_message(type::close, s.id, 0, s.orderly_close ? 1 : 0);
}

void frontend_deliver()
{
	if (!backend_ready || paused || ipc_link.failed)
		return;
	for (auto &[id, s] : sessions)
	{
		if (!s.descriptor)
		{
			frontend_deliver_close(s);
			continue;
		}
		P_desc d = s.descriptor;
		if (!s.opened)
		{
			if (d->connected == CON_SSLNEGO || (d->websocket && !d->ws_handshake_done))
				continue;
			writer w;
			encode_metadata(w, read_metadata(d));
			if (!send_message(type::open, id, 0, s.slot, w.bytes))
				return;
			s.opened = true;
		}
		if (d->connected != CON_FLUSH && !frontend_deliver_input(s))
			return;
	}
}

void record_signal(int number)
{
	frontend_signal = number;
}

pid_t start_world(const std::vector<std::string> &arguments)
{
	int pair[2];
	if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0, pair))
		return -1;
	int pass = 1;
	setsockopt(pair[0], SOL_SOCKET, SO_PASSCRED, &pass, sizeof(pass));
	setsockopt(pair[1], SOL_SOCKET, SO_PASSCRED, &pass, sizeof(pass));
	const pid_t parent = getpid();
	const pid_t child = fork();
	if (!child)
	{
		close(pair[0]);
		fcntl(pair[1], F_SETFD, 0);
		const std::string fd = std::to_string(pair[1]), pid = std::to_string(parent);
		setenv("DURIS_TRANSPORT_FD", fd.c_str(), 1);
		setenv("DURIS_TRANSPORT_PARENT", pid.c_str(), 1);
		std::vector<char *> argv;
		for (const auto &argument : arguments)
			argv.push_back(const_cast<char *>(argument.c_str()));
		argv.push_back(nullptr);
		execv(argv[0], argv.data());
		_exit(127);
	}
	close(pair[1]);
	if (child < 0)
	{
		close(pair[0]);
		return -1;
	}
	ipc_link = channel{};
	if (!ipc_link.attach(pair[0], child))
	{
		close(pair[0]);
		kill(child, SIGTERM);
		waitpid(child, nullptr, 0);
		return -1;
	}
	return child;
}

void fail_clients()
{
	backend_ready = false;
	while (descriptor_list)
	{
		P_desc d = descriptor_list;
		write_to_descriptor(
			d,
			"\r\nWorld connection lost. Please reconnect; queued commands will not be replayed.\r\n");
		close_socket(d);
	}
	sessions.clear();
	handoff.clear();
	last_world_state.clear();
	metadata_revisions.clear();
	tls_sessions.clear();
	frontend_input_bytes = 0;
	paused = false;
	committed = false;
	epoch = 0;
	barrier = 0;
	handoff_digest.clear();
}
}

bool transport_frontend_active()
{
	return frontend;
}
bool transport_world_active()
{
	return is_world;
}
bool transport_frontend_ready()
{
	return frontend && backend_ready && !paused && !ipc_link.failed;
}

bool transport_world_configure()
{
	const char *fd_text = getenv("DURIS_TRANSPORT_FD");
	const char *pid_text = getenv("DURIS_TRANSPORT_PARENT");
	if (!fd_text && !pid_text)
		return true;
	if (!fd_text || !pid_text)
		return false;
	char *end = nullptr;
	errno = 0;
	const long fd = strtol(fd_text, &end, 10);
	if (errno == ERANGE || !*fd_text || *end || fd < 3 || fd > INT_MAX)
		return false;
	errno = 0;
	const long pid = strtol(pid_text, &end, 10);
	if (errno == ERANGE || !*pid_text || *end || pid != getppid() || !ipc_link.attach(fd, pid))
		return false;
	nonblock(fd);
	is_world = true;
	return true;
}

bool transport_world_boot(bool restoring)
{
	if (!is_world)
		return true;
	epoch = nonce();
	if (!send_message(type::hello, 0, epoch, restoring ? 1 : 0) ||
	    !wait_world(welcome_received))
		return false;
	return restoring == !handoff_digest.empty();
}

void transport_world_ready()
{
	if (!is_world)
		return;
	for (const auto &[slot, s] : handoff)
	{
		(void)slot;
		if (!s.claimed)
			fatal_boot_error("transport",
					 "Incomplete authenticated session restoration");
	}
	backend_ready = true;
	if (!send_message(type::ready, 0, epoch))
		fatal_boot_error("transport", "Transport rejected world readiness");
}

void transport_world_pump()
{
	if (!is_world)
		return;
	for (P_desc d = descriptor_list, next; d; d = next)
	{
		next = d->next;
		if (d->network_close_pending)
			close_socket(d);
	}
	ipc_link.flush();
	ipc_link.receive(consume_world);
	for (auto it = world_pending_inputs.begin();
	     !paused && !ipc_link.failed && it != world_pending_inputs.end();)
	{
		P_desc d = find_descriptor(it->first);
		if (d && (d->input.head || d->transport_command))
		{
			++it;
			continue;
		}
		message input = std::move(it->second);
		world_pending_input_bytes -= input.payload.size();
		world_pending_inputs.erase(it);
		if (!consume_world(input))
			ipc_link.failed = true;
		// A reconnect handler may close a different descriptor and its pending event.
		it = world_pending_inputs.upper_bound(input.session);
	}
	if (ipc_link.failed && !shutdownflag)
	{
		logit(LOG_STATUS,
		      "Persistent frontend lost; saving world and closing logical sessions");
		while (descriptor_list)
			close_socket(descriptor_list);
		_reboot = 1;
		shutdownflag = 1;
	}
}

void transport_world_finish_pulse()
{
	if (!is_world)
		return;
	for (P_desc d = descriptor_list; d; d = d->next)
	{
		if (d->transport_command && !d->input.head)
		{
			d->transport_ack = d->transport_command;
			d->transport_command = 0;
			d->transport_command_started = false;
			send_message(type::ack, d->transport_session, d->transport_ack);
		}
		send_world_state(d);
	}
	ipc_link.flush();
}

bool transport_descriptor_eligible(P_desc d)
{
	return d && d->transport_session && is_world && d->connected == CON_PLAYING &&
	       d->character && IS_PC(d->character) && !d->original && !d->str && !d->editor &&
	       !d->showstr_count && !d->login_password_job && !d->password_request &&
	       !d->account_request && !d->player_load_request_id && !d->network_close_pending &&
	       d->account && d->account->acct_name && GET_ID(d->character) > 0 &&
	       GET_NAME(d->character) &&
	       (CAN_ACT(d->character) ||
		d->character->specials.wait_until_pulse <= ne_event_tick + INT_MAX) &&
	       !(d->input.head && (!d->transport_command || d->transport_command_started));
}

bool transport_descriptor_tls(P_desc d)
{
	const auto found = tls_sessions.find(d->transport_session);
	return is_world && found != tls_sessions.end() && found->second;
}

bool transport_world_quiesce()
{
	if (!is_world)
		return true;
	transport_world_finish_pulse();
	pause_received = false;
	pause_allowed = false;
	barrier = nonce();
	if (!send_message(type::pause, epoch, barrier))
		return false;
	paused = true;
	if (!wait_world(pause_received) || !pause_allowed)
		return false;
	// Application messages received before PAUSED may already have completed
	// without a command queue. Commit their ACKs so exec cannot replay them.
	transport_world_finish_pulse();
	for (P_desc d = descriptor_list; d; d = d->next)
		if (!transport_descriptor_eligible(d))
			return false;
	return true;
}

bool transport_world_commit(const char *path)
{
	if (!is_world)
		return true;
	// Output queued by persistence drains belongs before the handoff boundary.
	for (P_desc d = descriptor_list; d; d = d->next)
	{
		if (process_output(d) < 0)
			return false;
		send_world_state(d);
	}
	std::string digest;
	commit_received = false;
	return digest_file(path, digest, true) &&
	       send_message(type::commit, epoch, barrier, 0, digest) && wait_world(commit_received);
}

void transport_world_abort()
{
	if (!is_world || !paused)
		return;
	send_message(type::abort, epoch, barrier);
	paused = false;
}

bool transport_world_verify_file(const char *path)
{
	if (!is_world)
		return true;
	std::string actual;
	return !handoff_digest.empty() && digest_file(path, actual, false) &&
	       actual == handoff_digest;
}

bool transport_restore_descriptor(P_desc d, const char *name)
{
	if (!is_world)
		return false;
	const auto found = handoff.find(d->descriptor);
	if (found == handoff.end() || found->second.claimed || !name ||
	    strcasecmp(found->second.binding.player.c_str(), name))
		return false;
	restoration &s = found->second;
	d->transport_session = s.id;
	d->transport_ack = s.ack;
	d->transport_output_sequence = s.output_sequence;
	apply_metadata(d, s.meta);
	if (s.binding.metadata_revision == s.meta.revision)
		d->cp437 = s.binding.cp437;
	d->term_type = s.binding.term;
	d->durisweb_verified = s.binding.verified;
	strlcpy(d->last_input, s.binding.last_input.c_str(), sizeof(d->last_input));
	s.claimed = true;
	return true;
}

bool transport_restore_identity(P_desc d)
{
	const auto found = handoff.find(d->descriptor);
	if (found == handoff.end() || !d->character || !d->account || !d->account->acct_name)
		return false;
	const session_binding &b = found->second.binding;
	return b.pid == static_cast<unsigned>(GET_ID(d->character)) &&
	       !strcasecmp(b.account.c_str(), d->account->acct_name) &&
	       !strcasecmp(b.player.c_str(), GET_NAME(d->character));
}

bool transport_restore_gameplay(P_desc d)
{
	if (!transport_restore_identity(d))
		return false;
	const auto &state = handoff.at(d->descriptor).binding.world_state;
	if (state.size() != 15 || state.compare(0, 4, "DWS1"))
		return false;
	reader r{ state };
	r.offset = 4;
	const unsigned position = r.number(1), wait = r.number(4);
	const unsigned timer = r.number(2), lag = r.number(4);
	if (!r.done() || wait > INT_MAX || lag > INT_MAX)
		return false;
	SET_POS(d->character, position);
	d->wait = wait;
	d->character->specials.timer = static_cast<short>(timer);
	if (lag)
		CharWait(d->character, lag);
	return true;
}

void transport_frontend_accepted(P_desc d)
{
	if (!frontend)
		return;
	unsigned slot = 1;
	for (; slot <= session_limit; ++slot)
	{
		bool used = false;
		for (const auto &[id, s] : sessions)
		{
			(void)id;
			if (s.slot == slot)
				used = true;
		}
		if (!used)
			break;
	}
	if (slot > session_limit)
	{
		d->write_failed = 1;
		return;
	}
	fcntl(d->descriptor, F_SETFD, FD_CLOEXEC);
	do
	{
		d->transport_session = nonce();
	} while (sessions.count(d->transport_session));
	metadata_revisions[d->transport_session] = 1;
	session s;
	s.descriptor = d;
	s.slot = slot;
	s.id = d->transport_session;
	s.meta = read_metadata(d);
	if (d->sslses)
		d->ws_handshake_started = time(nullptr);
	sessions.emplace(s.id, std::move(s));
}

bool transport_frontend_input(P_desc d, unsigned kind, const char *data, size_t size)
{
	if (!frontend)
		return false;
	// Malformed application text belongs to this client, not the shared IPC
	// channel. The world rejects NUL-bearing strings as a protocol violation.
	if (size && memchr(data, '\0', size))
	{
		d->write_failed = 1;
		return true;
	}
	transport_frontend_metadata(d);
	const auto found = sessions.find(d->transport_session);
	if (found == sessions.end())
	{
		d->write_failed = 1;
		return true;
	}
	session &s = found->second;
	writer w;
	encode_metadata(w, read_metadata(d));
	w.string(std::string(data, size));
	if (w.bytes.size() > session_input_limit - s.input_bytes ||
	    w.bytes.size() > frontend_input_limit - frontend_input_bytes ||
	    s.inputs.size() >= session_input_count || s.input_sequence == UINT64_MAX)
	{
		d->write_failed = 1;
		return true;
	}
	s.input_bytes += w.bytes.size();
	frontend_input_bytes += w.bytes.size();
	s.inputs.push_back({ ++s.input_sequence, kind, std::move(w.bytes), false });
	return true;
}

void transport_frontend_metadata(P_desc d)
{
	if (!frontend)
		return;
	const auto found = sessions.find(d->transport_session);
	if (found == sessions.end())
		return;
	writer before, after;
	encode_metadata(before, found->second.meta);
	metadata current = read_metadata(d);
	encode_metadata(after, current);
	if (before.bytes == after.bytes)
		return;
	if (current.revision == UINT64_MAX)
	{
		d->write_failed = 1;
		return;
	}
	current.revision = ++metadata_revisions[d->transport_session];
	found->second.meta = current;
	if (found->second.opened && backend_ready && !paused)
	{
		writer changed;
		encode_metadata(changed, current);
		send_message(type::metadata, d->transport_session, 0, 0, changed.bytes);
	}
}

void transport_descriptor_closed(P_desc d)
{
	if (!d->transport_session)
		return;
	metadata_revisions.erase(d->transport_session);
	tls_sessions.erase(d->transport_session);
	if (is_world)
	{
		const auto pending = world_pending_inputs.find(d->transport_session);
		if (pending != world_pending_inputs.end())
		{
			world_pending_input_bytes -= pending->second.payload.size();
			world_pending_inputs.erase(pending);
		}
		send_message(type::close, d->transport_session);
		last_world_state.erase(d->transport_session);
	}
	else if (frontend)
	{
		const auto found = sessions.find(d->transport_session);
		if (found == sessions.end())
			return;
		session &s = found->second;
		s.orderly_close = d->network_close_pending && !d->write_failed;
		// A WebSocket message and CLOSE can arrive in the same socket read.
		// Forward at most the one outstanding event before the ordered close.
		if (s.orderly_close && s.opened && backend_ready && !paused && !ipc_link.failed)
			frontend_deliver_input(s);
		s.descriptor = nullptr;
		frontend_input_bytes -= s.input_bytes;
		s.inputs.clear();
		s.input_bytes = 0;
		if (!s.opened)
			sessions.erase(found);
		else
			frontend_deliver_close(s);
	}
}

int transport_world_output(P_desc d, unsigned kind, const void *data, size_t size)
{
	if (!is_world || !d->transport_session || d->write_failed || size > session_output_limit ||
	    d->transport_output_sequence == UINT64_MAX || (size && !data))
		return -1;
	send_world_state(d); // Authentication and output settings precede their application text.
	if (size + header_size + 65536 > channel_limit - ipc_link.queued_bytes)
	{
		d->write_failed = 1;
		return -1;
	}
	const std::string payload(size ? static_cast<const char *>(data) : "", size);
	if (!send_message(type::output, d->transport_session, ++d->transport_output_sequence, kind,
			  payload))
	{
		d->write_failed = 1;
		return -1;
	}
	return 0;
}

int transport_frontend_main(int argc, char **argv, int port, int tls_port)
{
	frontend = true;
	std::vector<std::string> arguments;
	char executable[4096];
	if (!realpath("/proc/self/exe", executable))
		fatal_boot_error("transport", "Cannot resolve world executable");
	arguments.emplace_back(executable);
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "--persistent-transport"))
			continue;
		if (argv[i][0] == '-' && argv[i][1] == 'd')
		{
			// main has already entered this directory; a relative -d must
			// not be interpreted again relative to the parent's new cwd.
			if (!argv[i][2])
				++i;
			arguments.emplace_back("-d");
			arguments.emplace_back(".");
		}
		else
			arguments.emplace_back(argv[i]);
	}
	// The parent never boots the world or starts persistence/authentication workers.
	dead_desc_pool = mm_create("TRANSPORT", sizeof(descriptor_data),
				   offsetof(descriptor_data, next),
				   mm_find_best_chunk(sizeof(descriptor_data), 25, 110));
	avail_descs = session_limit + 1;
	const int telnet = init_socket(port), tls = init_socket(tls_port),
		  ws = websocket_init(WS_PORT);
	fcntl(telnet, F_SETFD, FD_CLOEXEC);
	fcntl(tls, F_SETFD, FD_CLOEXEC);
	if (ws >= 0)
		fcntl(ws, F_SETFD, FD_CLOEXEC);
	struct sigaction action
	{
	};
	action.sa_handler = record_signal;
	sigemptyset(&action.sa_mask);
	for (int number : { SIGTERM, SIGINT, SIGHUP, SIGUSR1, SIGUSR2, SIGRTMIN })
		sigaction(number, &action, nullptr);
	signal(SIGPIPE, SIG_IGN);
	pid_t child = start_world(arguments);
	if (child < 0)
		fatal_boot_error("transport", "Cannot start world child");
	game_loop_watchdog_delegate(child);
	logit(LOG_STATUS, "Persistent transport pid=%ld world pid=%ld protocol=%u",
	      static_cast<long>(getpid()), static_cast<long>(child), version);
	bool stopping = false;
	bool restarting = false;
	int supervisor_status = 0;
	unsigned failed_starts = 0;
	auto stop_deadline = steady::time_point::max();
	while (true)
	{
		if (frontend_signal)
		{
			const int number = frontend_signal;
			frontend_signal = 0;
			kill(child, number);
			if (number == SIGTERM || number == SIGINT || number == SIGUSR2)
			{
				stopping = true;
				stop_deadline = steady::now() + std::chrono::seconds(30);
			}
		}
		if (!stopping)
		{
			for (const auto &[listener, kind] : std::array<std::pair<int, int>, 3>{
				     { { telnet, 0 }, { tls, 1 }, { ws, 2 } } })
				if (listener >= 0)
					for (unsigned i = 0;
					     i < 16 && new_descriptor(listener, kind) == 0; ++i)
					{
					}
		}
		ipc_link.flush();
		ipc_link.receive(consume_frontend);
		for (P_desc d = descriptor_list, next; d; d = next)
		{
			next = d->next;
			d->network_input_remaining = d->websocket ? WS_INPUT_BUFFER_SIZE :
								    MAX_QUEUE_LENGTH - 1;
			if (d->network_revents & (POLLERR | POLLNVAL | POLLPRI))
				d->write_failed = 1;
			if (d->connected == CON_SSLNEGO)
			{
				if (d->tls_handshake_deadline_us &&
				    latency_trace_monotonic_us() >= d->tls_handshake_deadline_us)
					d->write_failed = 1;
				else if (d->network_revents & (network_read_interest(d) | POLLHUP))
				{
					const int result = ssl_negotiate(d->sslses);
					if (result > 1)
						d->write_failed = 1;
					else if (result == 1)
						d->tls_read_interest =
							gnutls_record_get_direction(d->sslses) ?
								POLLOUT :
								POLLIN;
					else
					{
						d->tls_read_interest = 0;
						d->tls_handshake_deadline_us = 0;
						d->connected = CON_GET_TERM;
						ttype_negotiate(d);
						advertise_mccp(d);
						gmcp_negotiate(d);
					}
				}
			}
			else
			{
				// Resume an interrupted TLS send before any other TLS operation.
				if ((d->network_revents & network_write_interest(d)) &&
				    (d->websocket ? websocket_flush_output(d) :
						    telnet_flush_output(d)) < 0)
					d->write_failed = 1;
				const auto found = sessions.find(d->transport_session);
				// Stop socket reads before filling the application queue; TCP provides backpressure.
				if (d->connected != CON_FLUSH && !d->write_failed &&
				    !d->telnet_tls_retry && found != sessions.end() &&
				    found->second.input_bytes < session_input_limit / 2 &&
				    frontend_input_bytes < frontend_input_limit / 2 &&
				    found->second.inputs.size() < session_input_count / 2 &&
				    ((d->network_revents & (network_read_interest(d) | POLLHUP)) ||
				     network_buffered_input(d)))
				{
					const int result = process_input(d);
					if (result == NETWORK_INPUT_EOF)
						d->network_close_pending = 1;
					else if (result < 0)
						d->write_failed = 1;
				}
				if ((d->network_revents & POLLHUP) && !network_read_interest(d))
					d->write_failed = 1;
			}
			transport_frontend_metadata(d);
			size_t buffered_input = frontend_input_bytes;
			for (P_desc client = descriptor_list; client; client = client->next)
				buffered_input += client->ws_message_len + client->ws_fragment_len +
						  client->ws_handshake_len;
			if (buffered_input > frontend_input_limit)
				d->write_failed = 1;
			if (d->websocket && d->ws_state == WS_STATE_OPEN)
			{
				const time_t now = time(nullptr);
				if (d->ws_ping_outstanding &&
				    now - d->ws_last_ping > WS_PING_TIMEOUT)
					d->write_failed = 1;
				if (!d->ws_ping_queued && !d->ws_ping_outstanding &&
				    now - d->ws_last_ping >= WS_PING_INTERVAL)
				{
					if (websocket_send_ping(d) < 0)
						d->write_failed = 1;
					else
						d->ws_ping_queued = 1;
				}
			}
			if (d->ws_ping_queued && !d->ws_control_output_len)
			{
				d->ws_ping_queued = 0;
				d->ws_ping_outstanding = 1;
				d->ws_pong_received = 0;
				d->ws_last_ping = time(nullptr);
			}
			if (d->websocket && !d->ws_handshake_done &&
			    time(nullptr) - d->ws_handshake_started > WS_HANDSHAKE_TIMEOUT)
				d->write_failed = 1;
			if (d->ws_state == WS_STATE_CLOSING && !d->write_failed)
				d->network_close_pending = 1;
			if (d->write_failed || d->network_close_pending ||
			    (d->connected == CON_FLUSH && !d->telnet_output_len &&
			     !d->ws_output_len && !d->ws_control_output_len) ||
			    d->ws_state == WS_STATE_CLOSING)
				close_socket(d);
		}
		frontend_deliver();
		if (backend_ready)
			failed_starts = 0;
		if (paused && steady::now() >= pause_deadline)
		{
			logit(LOG_STATUS,
			      "Persistent transport replacement deadline expired; failing sessions closed");
			ipc_link.failed = true;
		}
		int status = 0;
		const pid_t exited = waitpid(child, &status, WNOHANG);
		if (exited == child)
		{
			logit(LOG_STATUS,
			      "Persistent world exited status=%d; transport sessions retired",
			      status);
			close(ipc_link.fd);
			fail_clients();
			// Ordinary shutdown exits the supervisor. Reboots/crashes restart cold;
			// a command with uncertain execution is never replayed onto that world.
			if (stopping)
				break;
			if (WIFEXITED(status) && (WEXITSTATUS(status) == 55 ||
						  (!restarting && WEXITSTATUS(status) == 0)))
			{
				supervisor_status = WEXITSTATUS(status);
				break;
			}
			if (++failed_starts > 3)
			{
				supervisor_status = 1;
				break;
			}
			restarting = false;
			if (access("bin/server/dms", X_OK) == 0)
			{
				if (realpath("bin/server/dms", executable))
					arguments[0] = executable;
			}
			child = start_world(arguments);
			if (child < 0)
			{
				supervisor_status = 1;
				break;
			}
			logit(LOG_STATUS, "Persistent transport cold world restart pid=%ld",
			      static_cast<long>(child));
			game_loop_watchdog_delegate(child);
		}
		else if (ipc_link.failed && !stopping && !restarting)
		{
			fail_clients();
			kill(child, SIGTERM);
			restarting = true;
			stop_deadline = steady::now() + std::chrono::seconds(30);
		}
		if (stopping && steady::now() >= stop_deadline)
		{
			kill(child, SIGKILL);
			waitpid(child, nullptr, 0);
			break;
		}
		if (restarting && steady::now() >= stop_deadline)
		{
			kill(child, SIGKILL);
			stop_deadline = steady::now() + std::chrono::seconds(30);
		}
		// poll bounds idle CPU without coupling transport progress to world ticks.
		std::vector<pollfd> fds{
			{ ipc_link.fd,
			  static_cast<short>(POLLIN | (ipc_link.pending_bytes ? POLLOUT : 0)), 0 }
		};
		for (int listener : { telnet, tls, ws })
			if (listener >= 0)
				fds.push_back({ listener, POLLIN, 0 });
		const size_t client_start = fds.size();
		std::vector<P_desc> polled_clients;
		int timeout_ms = 20;
		for (P_desc d = descriptor_list; d; d = d->next)
		{
			const auto found = sessions.find(d->transport_session);
			short events = found != sessions.end() &&
						       found->second.input_bytes <
							       session_input_limit / 2 &&
						       frontend_input_bytes <
							       frontend_input_limit / 2 &&
						       found->second.inputs.size() <
							       session_input_count / 2 ?
					       network_read_interest(d) :
					       0;
			if (events && network_buffered_input(d))
				timeout_ms = 0;
			events |= network_write_interest(d);
			fds.push_back({ d->descriptor, static_cast<short>(events | POLLPRI), 0 });
			polled_clients.push_back(d);
		}
		if (poll(fds.data(), fds.size(), timeout_ms) >= 0)
			for (size_t i = 0; i < polled_clients.size(); ++i)
				polled_clients[i]->network_revents = fds[client_start + i].revents;
	}
	while (descriptor_list)
		close_socket(descriptor_list);
	close(telnet);
	close(tls);
	if (ws >= 0)
		close(ws);
	return supervisor_status;
}
