/* -*- mode: c++ -*- */
#pragma once

#include "filters.h"
#include "util.h"

/**
 * Recover an incoming MIDI clock
 */
class TClockRecovery
{
    UNCOPYABLE(TClockRecovery)
        ;

public:
    TClockRecovery();

    /**
     * Register a MIDI clock beat
     * @param timestamp time of the event in samples
     */
    void Beat(uint64_t timestamp);

    void Report();

    /**
     * Get time of a quarter note in samples
     */
    float GetQuarterNoteTime() const;

    /**
     * Return tempo in BPM
     */
    float GetTempo() const;

private:
    float Interval;
    uint64_t LastBeat;
    uint64_t LastReport;
    int MinBeatInterval;
    int MaxBeatInterval;

    TOnePoleLpFilter ShortFilter;
};
