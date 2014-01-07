#include "TSampleOscillator.h"

void TSampleOscillator::Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
        TSampleBuffer& syncout)
{
    if (Sample) {
        for (TSample& outs : out) {
            if (LoopPoint2 >= -1 && Scanpos > LoopPoint2 && State <= TEnvelope::SUSTAIN ) {
                Scanpos = LoopPoint1 + Scanpos - LoopPoint2;
            }
            if (Scanpos <= Sample->GetCount()) {
                outs = (*Sample)[int(Scanpos)] * 4 * TGlobal::OscAmplitude;
            }
            Scanpos += Hz / Scanspeed;
        }
    }
    syncout.Clear();
}
