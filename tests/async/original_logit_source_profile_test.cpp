// Real-profile regression linked with the whole production utility.c providers.
// Define EXPECT_ORIGINAL_LOGIT_SOURCE_SUPPORTED
// to 1 for production flags, 0 for each actual unsupported compiler/profile.
#include "core/utility.h"
#include <cassert>
#include <cstdio>
#include <thread>
#include <sys/stat.h>
#include <time.h>
#ifndef EXPECT_ORIGINAL_LOGIT_SOURCE_SUPPORTED
#error Specify actual expected native profile support
#endif
#ifdef NDEBUG
#error Assertions must be enabled for qualification
#endif
int main()
{
    using getter = bool (*)(size_t *) noexcept;
    const getter getters[] = {
        diagnostic_original_logit_additional_source_frame_bytes,
        diagnostic_original_logit_working_query_source_frame_bytes,
        diagnostic_original_logit_with_queries_source_frame_bytes,
    };
    const size_t P = sizeof(void *), N = sizeof(size_t), I = sizeof(int), B = sizeof(bool);
    const size_t additional = 53*P+8*N+7*I+B+3*sizeof(time_t)+sizeof(mode_t);
    const size_t working = 16*P+9*N+3*I+4*B+2*sizeof(std::thread::id)+
        2*sizeof(std::thread::native_handle_type);
    const size_t expected[] = {additional, working, additional+working+N-(5*P+N+2*I)};
    static_assert(diagnostic_original_logit_source_query_frame_bytes() ==
        sizeof(size_t *)+sizeof(bool));
    for (size_t i=0; i<3; ++i)
    {
        assert(!getters[i](nullptr));
        size_t output=123456789;
        const bool supported=getters[i](&output);
        assert(supported == bool(EXPECT_ORIGINAL_LOGIT_SOURCE_SUPPORTED));
        assert(output == (supported ? expected[i] : size_t(123456789)));
    }
    std::printf("UTILITY2_PROFILE_GETTERS {\"supported\":%s,\"getters\":3,\"null_refusals\":3,\"strong_outputs\":3}\n",
        EXPECT_ORIGINAL_LOGIT_SOURCE_SUPPORTED ? "true" : "false");
}
