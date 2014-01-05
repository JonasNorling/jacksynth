/* -*- mode: c++ -*- */
#pragma once

#include <map>
#include "IAudioPort.h"

/**
 * Digital State Variable Filter implementation, second order
 *
 *   http://www.earlevel.com/main/2003/03/02/the-digital-state-variable-filter/
 *   http://www.fpga.synth.net/pmwiki/pmwiki.php?n=FPGASynth.SVF
 *
 *      .-- *-1 <-------------------------------.
 *      |  .-> HP     .-> BP                    |
 *      v  |          |                         |
 * in ->+----> *f ->+--> z^-1 -> *f ->+-----------> LP
 *      ^           ^         |       ^         |
 *      |           `---------|       `- z^-1 <-´
 *      |                     |
 *      `-- *-1 <--- *q <-----´
 */
class TSvfFilter
{
  UNCOPYABLE(TSvfFilter);

public:
  TSvfFilter();

  void SetCutoff(int cutoff);
  void SetResonance(TFraction resonance);
  void Process(TSampleBuffer& in, TSampleBuffer& out);

private:
  struct TCoeffs {
    TSample f;
    TSample q;
  } Coeffs;

  TSample State1[2];
  TSample State2[2];
  static const int Oversample = 2;

  static void Crunch(const TCoeffs coeffs, TSample* state,
	      const TSample& ins,
	      TSample& lp, TSample& hp,
	      TSample& bp, TSample& br);
};
