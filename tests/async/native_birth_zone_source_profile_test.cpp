// Passive public Zone getters using the entire unchanged production provider.
// No observer, callback, logger, database or game operation is invoked.
#ifndef EXPECT_NATIVE_SOURCE_SUPPORTED
#error Select the genuine supported or assertions-refusal profile.
#endif
#include "world/zone_reset_item_owner.c"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Fixture assertions must be enabled.
#endif

namespace passive_profile_regression
{
using getter = bool (*)(size_t *) noexcept;
constexpr bool expected_supported = EXPECT_NATIVE_SOURCE_SUPPORTED != 0;
constexpr size_t sentinel = 123456789;
constexpr size_t P = sizeof(void *), B = sizeof(bool);
constexpr size_t ID = sizeof(std::thread::id), H = sizeof(std::thread::native_handle_type);

void check(getter query, size_t expected)
{
    assert(!query(nullptr));
    size_t output = sentinel;
    assert(query(&output) == expected_supported);
    assert(output == (expected_supported ? expected : sentinel));
}

int run()
{
    static_assert(P == 8 && sizeof(size_t) == 8 && ID == 8 && H == 8 && B == 1);
    // Genuine untouched observer forests plus independently typed native path.
    constexpr size_t native_thread = 2 * ID + P + 2 * H + 2 * B;
    check(zone_reset_item_owner::flat_global_current_header_source_frame_bytes,
          flat_root_global_graph_source::base_without_thread + native_thread);
    check(zone_reset_item_owner::flat_global_literal_current_header_source_frame_bytes,
          flat_root_global_graph_source::literal_without_thread + native_thread);
    std::printf("NATIVE_SOURCE_PROFILE {\"family\":\"zone\",\"supported\":%s,"
                "\"getters\":2,\"null_refusals\":2,\"strong_outputs\":2}\n",
                expected_supported ? "true" : "false");
    return 0;
}
}

int main() { return passive_profile_regression::run(); }
