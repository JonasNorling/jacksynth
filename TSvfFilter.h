/* -*- mode: c++ -*- */
#pragma once

#include <map>
#include "IAudioPort.h"
#include "TWaveShaper.h"

/**
 * Digital State Variable Filter implementation, second order
 *
 *   http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
 *   http://www.fpga.synth.net/pmwiki/pmwiki.php?n=FPGASynth.SVF
 *   http://www.music.mcgill.ca/~ich/classes/FiltersChap2.pdf (different topology)
 *
 *      .-- *-1 <-------------------------------.
 *      |  .-> HP     .-> BP                    |
 *      v  |          |                         |
 * in ->+----> *f ->+--> z^-1 -> *f ->+-----------> LP
 *      ^           ^         |       ^         |
 *      |           `---------|       `- z^-1 <-´
 *      |                     |
 *      `-- *-1 <--- *q <-----´
 *
 * The filter is run twice to create a 24dB/octave 4-pole filter. A tanh wave shaper
 * is employed to soft limit the maximum output value, which can otherwise be enormous
 * when running with high Q.
 *
 * The implementation is oversampled because of stability issues when the cutoff gets
 * close to Nyquist. The downsampling is naive: only a decimation is implemented. This
 * takes care of the filter's own needs (it doesn't create overtones), but aliasing from
 * the wave shaping should ideally be suppressed with a proper decimation filter.
 *
 * FIXME:
 *  * proper decimation to remove aliasing from the soft limiter.
 *    - test case: high pitched clean saw through high-Q filter with cutoff near a harmonic
 *  * Parameter to set output mix between LP, HP, BP. (BR is LP + HP).
 */
class TSvfFilter
{
UNCOPYABLE(TSvfFilter)
    ;

public:
    TSvfFilter();

    void SetCutoff(int cutoff);
    void SetResonance(TFraction resonance);
    void Process(TSampleBuffer& in, TSampleBuffer& out);

private:
    struct TCoeffs
    {
        TSample f;
        TSample q;
    } Coeffs;

    TSample State1[2];
    TSample State2[2];
    float Mix[3];
    static const int Oversample = 4;

    TWaveShaper WaveShaper;

    void Crunch(const TCoeffs coeffs, TSample* state, const TSample& ins, TSample& lp, TSample& hp,
            TSample& bp);
};
