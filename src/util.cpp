#include "util.h"
#include <cstdio>

TTimer::TTimer(const std::string& name)
        : Name(name), StartTime(), Nanoseconds(0), MaxNs(0), Count(0)
{
}

TTimer::~TTimer()
{
    Log();
}

void TTimer::Log() const
{
    float max = MaxNs / 1000000.0;
    float average = Nanoseconds / (Count > 0 ? Count : 1) / 1000000.0;

    printf("[Timer] %s: %d invocations, total %u ms, average %.3f ms, max %.3f ms\n",
            Name.c_str(), Count, GetMs(), average, max);
}
