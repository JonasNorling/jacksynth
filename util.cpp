#include "util.h"
#include <cstdio>

TTimer::TTimer(const std::string& name)
        : Name(name), StartTime(), Nanoseconds(0)
{
}

TTimer::~TTimer()
{
    fprintf(stderr, "Timer %s: %lld ms\n", Name.c_str(), GetMs());
}
