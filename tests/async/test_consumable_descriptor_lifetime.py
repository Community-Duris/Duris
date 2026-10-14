#!/usr/bin/env python3
"""Actual descriptor creator/close bodies, real MM reuse, private queue/loan controls.

Root executes the native recipe. Service, character and syscall boundaries are
explicit controlled fixtures, not full Source/current-G/gameplay/SQL/ACK proof.
"""
from pathlib import Path
import hashlib
import os
import subprocess
import tempfile

from _paths import ROOT, extract_function

created = extract_function("comm.c", "int new_descriptor(int s, int conn_type)")
closed = extract_function("comm.c", "void close_socket(struct descriptor_data *d)")
new_hook = "\tconsumable_descriptor_lifetime_hooks::created(newd, dead_desc_pool,\n\t\tconsumable_descriptor_lifetime_hooks::origin::socket);\n"
close_hook = "\tconsumable_descriptor_lifetime_hooks::invalidated(d);\n"
assert created.count(new_hook) == closed.count(close_hook) == 1
assert hashlib.sha256(created.replace(new_hook, "").encode()).hexdigest() == "61f907d5a785aac8eef9f18bf3ad767f660789e3d230ca2651673487790176b7"
assert hashlib.sha256(closed.replace(close_hook, "").encode()).hexdigest() == "c393671ce746e236040a1218bbca8a07cade5faa6201ce9c57161447b9900af3"
assert created.index(new_hook) < created.index("descriptor_list = newd;")
assert closed.index(close_hook) < closed.index("int is_morphed") < closed.index("compress_end(d, TRUE)")

transport = extract_function("transport.c", "bool consume_world_frame(const message &m)")
transport_hook = "\t\tduris_consumable_detail::transport_descriptor_creator_access::install(d, dead_desc_pool,\n\t\t\tduris_consumable_detail::transport_descriptor_creator_access::source);\n"
assert hashlib.sha256(transport.replace(transport_hook, "").encode()).hexdigest() == "9290222e22482bec977dd475634953219070998b055a7ee104cfa90a29ed9261"
recover = extract_function("copyover.c", "int copyover_recover(int *mother_desc, int *mother_desc_ssl, int *ws_desc)")
original_recover = recover.replace("\tconst uint64_t consumable_recovery_before = consumable_descriptor_lifetime_hooks::recovery_begin();\n", "")
original_recover = original_recover.replace("\t\tconsumable_descriptor_lifetime_hooks::created(d, dead_desc_pool,\n\t\t\tconsumable_descriptor_lifetime_hooks::origin::copyover);\n", "")
original_recover = original_recover.replace("\t\t\t\tconsumable_descriptor_lifetime_hooks::invalidated(d);\n", "")
original_recover = original_recover.replace("\t\t\tconsumable_descriptor_lifetime_hooks::invalidated(d);\n", "")
original_recover = original_recover.replace("\tconsumable_descriptor_lifetime_hooks::recovery_failed(consumable_recovery_before);\n", "")
assert hashlib.sha256(original_recover.encode()).hexdigest() == "8775c9553d9e61621319526dddffa9680978a1e74abe7961704b960c525a091b"
assert recover.count("mm_release(dead_desc_pool, d);") == 3

MM = "\n\n".join(extract_function("mm.c", signature) for signature in (
    "static char *mm_next(", "static void mm_set_next(", "struct mm_ds *mm_create(",
    "void mm_release(", "void *_mm_get(", "void *mm_try_get(",
    "bool mm_try_reserve_free_slot(", "void mm_alloc_chunk(",
    "unsigned mm_find_best_chunk(",
))
queue = (ROOT / "src/net/consumable_prepared_output_queue.c").read_text()
write = extract_function("comm.c", "void write_to_q(")
get = extract_function("comm.c", "int get_from_q(")
flush = extract_function("comm.c", "void flush_queues(")
clock = extract_function("comm.c", "static uint64_t loop_monotonic_us(void)")

HARNESS = r'''

#include "net/consumable_descriptor_lifetime.c"
#include "world/event_thread_owner.c"
#include "core/utils.h"
#include "core/files.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "account/account_recovery.h"
#include "account/account_async.h"
#include "account/password_async.h"
#include "account/password_hash.h"
#include "player/player_load_pipeline.h"
#include "telemetry/telemetry_runtime.h"
#include "net/network_readiness.h"
#include "net/ttype.h"
#include "net/websocket.h"
#include "persistence/latency_trace.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/mman.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>
#include <fcntl.h>
#include <string>

P_desc descriptor_list = nullptr, next_to_process = nullptr;
mm_ds *dead_desc_pool = nullptr;
mm_ds_list *mmds_list = nullptr;
int used_descs = 0, avail_descs = 100, max_descs = 0, max_descs_this_hour = 0;
static size_t cpp_calls, cpp_cut = SIZE_MAX, tag_calls, tag_cut = SIZE_MAX, live_tag, admit_calls,
			 admit_cut = SIZE_MAX;
static int next_fd = 42;
static consumable_prepared_output_owner *watch = nullptr;
static bool closing_observed;
bool watch_current();
static std::vector<std::pair<void *, size_t>> mappings;
void *operator new(size_t n)
{
	if (cpp_calls++ == cpp_cut)
		throw std::bad_alloc();
	if (auto *p = std::malloc(n ? n : 1))
		return p;
	throw std::bad_alloc();
}
void operator delete(void *p) noexcept
{
	std::free(p);
}
void operator delete(void *p, size_t) noexcept
{
	std::free(p);
}
void *operator new(size_t n, const std::nothrow_t &) noexcept
{
	try
	{
		return ::operator new(n);
	}
	catch (...)
	{
		return nullptr;
	}
}
void operator delete(void *p, const std::nothrow_t &) noexcept
{
	std::free(p);
}
void *__malloc(size_t n, const char *, const char *, int)
{
	auto *p = std::calloc(1, n);
	assert(p);
	++live_tag;
	return p;
}
void *__try_malloc(size_t n, const char *, const char *, int) noexcept
{
	if (tag_calls++ == tag_cut)
		return nullptr;
	auto *p = std::calloc(1, n);
	if (p)
		++live_tag;
	return p;
}
void __free(void *p, const char *, int)
{
	if (p)
	{
		assert(live_tag);
		--live_tag;
		std::free(p);
	}
}
void *test_mmap(void *a, size_t b, int c, int d, int e, off_t f)
{
	auto *p = ::mmap(a, b, c, d, e, f);
	if (p != MAP_FAILED)
		mappings.emplace_back(p, b);
	return p;
}
P_char find_character_by_runtime_id(uint64_t)
{
	return nullptr;
} // These lifecycle cases have no character.
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...)
{
	std::abort();
}
void loginlog(int, const char *, ...)
{
	std::abort();
}
void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
void fatal_boot_error(const char *, const char *, ...)
{
	std::abort();
}
bool player_load_pipeline_cancel(uint64_t)
{
	std::abort();
}
void account_recovery_descriptor_closed(P_desc)
{
	if (watch)
	{
		assert(!watch_current());
		closing_observed = true;
	}
}
void account_async_cancel(P_desc) {}
void password_async_cancel(P_desc) {}
extern "C" void password_login_release(password_login_job *p)
{
	assert(!p);
}
int compress_end(P_desc, int)
{
	return 0;
}
bool poll_wizard_active(P_char)
{
	std::abort();
}
void poll_wizard_cancel(P_char)
{
	std::abort();
}
void ssl_close(gnutls_session_t)
{
	std::abort();
}
gnutls_session_t ssl_new(int)
{
	return nullptr;
}
bool affected_by_spell(P_char, int)
{
	std::abort();
}
int IS_MORPH(P_char)
{
	std::abort();
}
P_char un_morph(P_char)
{
	std::abort();
}
void send_to_char(const char *, P_char)
{
	std::abort();
}
void send_to_char(const char *, P_char, int)
{
	std::abort();
}
void act(const char *, int, P_char, P_obj, void *, int)
{
	std::abort();
}
void rem_char_from_snoopby_list(snoop_by_data **, P_char)
{
	std::abort();
}
telemetry_capture_result
telemetry_runtime_game_connection_transition(P_char, P_desc, telemetry_connection_transition_kind)
{
	std::abort();
}
telemetry_capture_result telemetry_runtime_game_evidence(P_char, P_desc,
							 telemetry_runtime_evidence_kind)
{
	std::abort();
}
void sql_disconnectIP(P_char)
{
	std::abort();
}
void redis_player_offline(P_char)
{
	std::abort();
}
int NumAttackers(P_char)
{
	std::abort();
}
void sql_log(P_char, const char *, const char *, ...)
{
	std::abort();
}
bool persistence_save_character_terminal(P_char, int)
{
	std::abort();
}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
	std::abort();
}
void persistence_schedule_character_save(P_char, int, int, const char *)
{
	std::abort();
}
void item_creation_grant_cancel_batch_before_entry(P_char)
{
	std::abort();
}
void free_char(P_char)
{
	std::abort();
}
P_acct free_account(P_acct p)
{
	assert(!p);
	return nullptr;
}
void websocket_free(P_desc d)
{
	assert(!d->websocket);
}
void telnet_free_output(P_desc d)
{
	assert(!d->telnet_output_buffer);
}
int new_connection(int)
{
	return next_fd;
}
void resolve_descriptor_hostname_async(const char *, int) {}
std::string strip_ansi(const char *p)
{
	return p;
}
void ttype_negotiate(P_desc) {}
void advertise_mccp(P_desc) {}
void gmcp_negotiate(P_desc) {}
static void greet(P_desc d)
{
	d->connected = CON_PLAYING;
}
bool transport_world_active()
{
	return false;
}
bool transport_frontend_active()
{
	return false;
}
void transport_frontend_accepted(P_desc) {}
void transport_descriptor_closed(P_desc) {}
int test_close(int)
{
	return 0;
}
int test_shutdown(int, int)
{
	return 0;
}
int test_getpeername(int, sockaddr *address, socklen_t *n)
{
	assert(*n >= sizeof(sockaddr_in6));
	std::memset(address, 0, sizeof(sockaddr_in6));
	reinterpret_cast<sockaddr_in6 *>(address)->sin6_family = AF_INET6;
	return 0;
}
bool admit(size_t, void *) noexcept
{
	return admit_calls++ != admit_cut;
}
int is_desc_valid(P_desc d)
{
	for (auto *n = descriptor_list; n; n = n->next)
		if (n == d)
			return 1;
	return 0;
}

#undef CREATE
#undef RECREATE
#undef FREE
#define CREATE(result, type, count, tag)                                                        \
	do                                                                                      \
	{                                                                                       \
		result = (type *)__try_malloc(sizeof(type) * (count), tag, __FILE__, __LINE__); \
		assert(result);                                                                 \
	} while (false)
#define RECREATE(result, type, count)                                                           \
	do                                                                                      \
	{                                                                                       \
		void *p = __try_malloc(sizeof(type) * (count), "M", __FILE__, __LINE__);        \
		assert(p);                                                                      \
		std::memcpy(p, result,                                                          \
			    std::min(std::strlen((char *)result) + 1, sizeof(type) * (count))); \
		__free(result, __FILE__, __LINE__);                                             \
		result = (type *)p;                                                             \
	} while (false)
#define FREE(p)                                \
	do                                     \
	{                                      \
		__free(p, __FILE__, __LINE__); \
		p = nullptr;                   \
	} while (false)
'''

DRIVER = r'''

class item_consumable_publication_owner
{
    public:
	static auto prepare(P_desc d, std::span<const std::string_view> messages)
	{
		return consumable_prepared_output_owner::prepare(d, messages, admit, nullptr, 0);
	}
	static bool current(const consumable_prepared_output_owner &p) { return p.current(); }
	static bool transfer(consumable_prepared_output_owner &p) { return p.transfer(); }
};
bool watch_current()
{
	return watch && item_consumable_publication_owner::current(*watch);
}
std::vector<std::string> image(const txt_q &q)
{
	std::vector<std::string> result;
	for (auto *n = q.head; n; n = n->next)
		result.emplace_back(n->text);
	return result;
}
int main()
{
	nevent_bind_game_thread();
	dead_desc_pool = mm_create("SOCKET", sizeof(descriptor_data),
				   offsetof(descriptor_data, next),
				   mm_find_best_chunk(sizeof(descriptor_data), 25, 110));
	assert(mm_try_reserve_free_slot(dead_desc_pool));
	// Use one actual free slot so the next original creator MUST reuse it.
	std::vector<void *> held;
	while (dead_desc_pool->head && dead_desc_pool->head != dead_desc_pool->tail)
		held.push_back(mm_try_get(dead_desc_pool));
	const std::array<std::string_view, 2> messages = { "one\0unused", " two" };
	assert(!new_descriptor(0, 0));
	auto *d = descriptor_list;
	auto owner = item_consumable_publication_owner::prepare(d, messages);
	assert(owner && item_consumable_publication_owner::current(*owner));
	watch = owner.get();
	close_socket(d);
	assert(closing_observed && !descriptor_list);
	watch = nullptr;
	assert(mappings.size() == 1);
	assert(!mprotect(mappings[0].first, mappings[0].second, PROT_NONE));
	assert(!item_consumable_publication_owner::current(*owner) &&
	       !item_consumable_publication_owner::transfer(*owner));
	assert(!mprotect(mappings[0].first, mappings[0].second, PROT_READ | PROT_WRITE));
	// Keep old control/loan alive; exact pool address/fd/fields are reused.
	assert(!new_descriptor(0, 0));
	auto *reused = descriptor_list;
	assert(reused == d && reused->descriptor == 42);
	assert(!item_consumable_publication_owner::current(*owner) &&
	       !item_consumable_publication_owner::transfer(*owner));
	assert(!reused->output.head);
	owner.reset();
	auto valid = item_consumable_publication_owner::prepare(reused, messages);
	assert(valid);
	std::thread foreign(
		[&]
		{
			assert(!item_consumable_publication_owner::current(*valid) &&
			       !item_consumable_publication_owner::transfer(*valid));
		});
	foreign.join();
	assert(!reused->output.head);
	txt_q reference{};
	for (auto s : messages)
	{
		std::string text(s);
		write_to_q(text.c_str(), &reference, 1);
	}
	auto expected = image(reference);
	assert(item_consumable_publication_owner::transfer(*valid));
	assert(image(reused->output) == expected);
	assert(!item_consumable_publication_owner::transfer(*valid));
	valid.reset();
	while (reference.head)
	{
		char text[MAX_STRING_LENGTH];
		assert(get_from_q(&reference, text));
	}
	// Supported-protocol/current-state refusal leaves real original queues.
	const auto before = image(reused->output);
	for (int profile = 0; profile < 4; ++profile)
	{
		if (profile == 0)
			reused->out_compress = MCCP_VER2;
		if (profile == 1)
			reused->transport_session = 9;
		if (profile == 2)
			reused->websocket = 1;
		if (profile == 3)
			reused->write_failed = 1;
		assert(!item_consumable_publication_owner::prepare(reused, messages));
		reused->out_compress = MCCP_NONE;
		reused->transport_session = 0;
		reused->websocket = 0;
		reused->write_failed = 0;
		assert(image(reused->output) == before);
	}
	auto cancel = item_consumable_publication_owner::prepare(reused, messages);
	assert(cancel);
	cancel.reset();
	assert(image(reused->output) == before);
	// Refuse both actual C++/tagged preparation allocation cuts, strong state.
	cpp_calls = tag_calls = admit_calls = 0;
	{
		auto p = item_consumable_publication_owner::prepare(reused, messages);
		assert(p);
	}
	const size_t cpp_total = cpp_calls, tag_total = tag_calls, admit_total = admit_calls;
	for (size_t cut = 0; cut < cpp_total; ++cut)
	{
		cpp_calls = 0;
		cpp_cut = cut;
		auto p = item_consumable_publication_owner::prepare(reused, messages);
		cpp_cut = SIZE_MAX;
		assert(!p && image(reused->output) == before);
	}
	for (size_t cut = 0; cut < tag_total; ++cut)
	{
		tag_calls = 0;
		tag_cut = cut;
		auto p = item_consumable_publication_owner::prepare(reused, messages);
		tag_cut = SIZE_MAX;
		assert(!p && image(reused->output) == before);
	}
	for (size_t cut = 0; cut < admit_total; ++cut)
	{
		admit_calls = 0;
		admit_cut = cut;
		auto p = item_consumable_publication_owner::prepare(reused, messages);
		admit_cut = SIZE_MAX;
		assert(!p && image(reused->output) == before);
	}
	// Missing owner refuses before reading even a wholly inaccessible address.
	auto *unknown = static_cast<P_desc>(
		mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
	assert(unknown != MAP_FAILED);
	assert(!item_consumable_publication_owner::prepare(unknown, messages));
	assert(!munmap(unknown, 4096));
	close_socket(reused);
	// Actual incarnation setup allocation/space exhaustion does not reject old
	// original accepted connections; their prepared route remains unavailable.
	cpp_calls = 0;
	cpp_cut = 0;
	assert(!new_descriptor(0, 0));
	cpp_cut = SIZE_MAX;
	assert(descriptor_list &&
	       !item_consumable_publication_owner::prepare(descriptor_list, messages));
	close_socket(descriptor_list);
	consumable_descriptor_lifetime_registry::next_incarnation = UINT64_MAX;
	assert(!new_descriptor(0, 0));
	assert(!item_consumable_publication_owner::prepare(descriptor_list, messages));
	close_socket(descriptor_list);
	assert(!consumable_descriptor_lifetime_registry::objects);
	for (auto *p : held)
		mm_release(dead_desc_pool, p);
	for (auto [p, n] : mappings)
		assert(!munmap(p, n));
	while (mmds_list)
	{
		auto *p = mmds_list;
		mmds_list = p->next;
		__free(p->mmds, __FILE__, __LINE__);
		__free(p, __FILE__, __LINE__);
	}
	assert(!live_tag);
}
'''

scratch = Path(os.environ.get("DURIS_TEST_SCRATCH", str(ROOT / "bin/tests")))
scratch.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="consumable-descriptor-life-", dir=scratch) as directory:
    temp = Path(directory)
    source = temp / "harness.cpp"
    source.write_text(HARNESS + "\n#define mmap test_mmap\n" + MM + "\n#undef mmap\n" + write + "\n" + get + "\n" + flush + "\n" + queue + "\n" + clock + "\n#define close test_close\n#define shutdown test_shutdown\n#define getpeername test_getpeername\n" + created + "\n" + closed + "\n#undef close\n#undef shutdown\n#undef getpeername\n" + DRIVER)
    binary = temp / "descriptor-lifetime"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-DDURIS_PERSISTENT_TRANSPORT", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-pthread", "-I" + str(ROOT / "src/no_mysql"), "-I" + str(ROOT / "src"),
        str(source), str(ROOT / "src/persistence/latency_trace.c"), "-lbsd", "-o", str(binary),
    ], check=True)
    environment = os.environ.copy()
    environment.update({"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                        "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
    subprocess.run([str(binary)], check=True, env=environment)
print("original descriptor lifetime and connected private queue controls passed")
