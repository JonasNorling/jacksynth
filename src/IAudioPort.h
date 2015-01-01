/* -*- mode: c++ -*- */
#pragma once

#include <math.h>
#include <jack/jack.h>
#include <algorithm>
#include <cassert>
#include <vector>

#include "util.h"

typedef jack_default_audio_sample_t TSample;

/**
 * Wrapper around a TSample[], doesn't own the data.
 */
class TSampleBuffer
{
public:
    TSampleBuffer(TSample* begin, jack_nframes_t count)
            : Begin(begin), End(begin + count), Count(count)
    {
    }

    TSample* begin()
    {
        return Begin;
    }
    const TSample* begin() const
    {
        return Begin;
    }
    TSample* end()
    {
        return End;
    }
    const TSample* end() const
    {
        return End;
    }

    size_t GetCount() const
    {
        return Count;
    }
    TSample& operator[](size_t n)
    {
        return Begin[n];
    }
    const TSample& operator[](size_t n) const
    {
        return Begin[n];
    }
    void Clear()
    {
        std::fill(Begin, End, 0);
    }

    void AddSamples(TSampleBuffer& b)
    {
        assert(GetCount() == b.GetCount());
        std::transform(begin(), end(), b.begin(), begin(),
                [](TSample s1, TSample s2) -> TSample {
                    return s1 + s2;
                });
    }

    TSampleBuffer Slice(jack_nframes_t start, jack_nframes_t end)
    {
        assert(start <= end);
        assert(end <= Count);
        return TSampleBuffer(Begin + start, end - start);
    }

    TSample PeakAmplitude()
    {
        TSample max = 0.0f;
        std::for_each(begin(), end(), [&max](const TSample& s) { max = std::max(max, fabsf(s)); } );
        return max;
    }

private:
    TSample* Begin;
    TSample* End;
    jack_nframes_t Count;
};


class IAudioPort
{
UNCOPYABLE(IAudioPort)
    ;

public:
    enum TDirection
    {
        INPUT = JackPortIsInput, OUTPUT = JackPortIsOutput
    };
    IAudioPort()
    {
    }
    virtual ~IAudioPort()
    {
    }

    virtual TSampleBuffer GetBuffer(jack_nframes_t nframes) = 0;
    virtual void Commit() = 0;
};

typedef std::vector<IAudioPort*> TAudioPortCollection;
typedef std::vector<TSampleBuffer*> TSampleBufferCollection;
