#include "TSampleOscillator.h"

void TSampleOscillator::Process(TSampleBuffer& in, TSampleBuffer& out)
{
  if (Sample) {
    for (TSample& outs : out) {
      if (Scanpos <= Sample->GetCount()) {
	outs = (*Sample)[Scanpos] * 4 * TGlobal::OscAmplitude;
      }
      Scanpos += float(Hz)/TGlobal::HzA4;
    }
  }
}
