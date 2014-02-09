#include "TClockRecovery.h"
#include "TGlobal.h"

#include <cstdio>
#include <cstdint>

TClockRecovery::TClockRecovery()
: Interval(1000.0f), LastBeat(0), LastReport(0),
  MinBeatInterval(INT32_MAX), MaxBeatInterval(INT32_MIN),
  ShortFilter(1000.0f)
{
    ShortFilter.SetCutoff(0.01);
}

void TClockRecovery::Beat(uint64_t timestamp)
{
    int beatInterval = timestamp - LastBeat;

    // Nudge our timer towards the MIDI beat. This is like a "P" controller, or
    // a one pole LP fiter, I think.
    // FIXME: Needs improvement: will never converge, bad at reacting to changes
    /*
    float delta = beatInterval - Interval;
    Interval += 0.1 * delta;
     */
    Interval = ShortFilter.ProcessOne(beatInterval);
    MinBeatInterval = std::min(MinBeatInterval, beatInterval);
    MaxBeatInterval = std::max(MaxBeatInterval, beatInterval);

    LastBeat = timestamp;

    if (timestamp > LastReport + 1 * TGlobal::SampleRate) {
        Report();
        LastReport = timestamp;
    }
}

float TClockRecovery::GetQuarterNoteTime() const
{
    return Interval * MIDI_BEATS_PER_Q;
}

float TClockRecovery::GetTempo() const
{
    return 60.0f * TGlobal::SampleRate / GetQuarterNoteTime();
}

void TClockRecovery::Report()
{
    printf("Tempo: %.1f BPM (interval %.1f [%d..%d])\n", GetTempo(), Interval, MinBeatInterval, MaxBeatInterval);
    MinBeatInterval = INT32_MAX;
    MaxBeatInterval = INT32_MIN;
}
