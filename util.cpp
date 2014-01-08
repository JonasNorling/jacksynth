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
    float max = MaxNs / 1000000.0f;
    float average = Nanoseconds / Count / 1000000.0f;

    fprintf(stderr, "[Timer] %s: %d invocations, total %u ms, average %.3f ms, max %.3f ms\n",
            Name.c_str(), Count, GetMs(), average, max);
}
