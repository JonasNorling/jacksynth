#include "TSampleOscillator.h"

void TSampleOscillator::Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
        TSampleBuffer& syncout)
{
    if (Sample) {
        for (TSample& outs : out) {
            if (LoopPoint2 >= -1 && PhaseAccumulator > LoopPoint2 && State <= TEnvelope::SUSTAIN ) {
                PhaseAccumulator = LoopPoint1 + PhaseAccumulator - LoopPoint2;
            }
            if (PhaseAccumulator <= Sample->GetCount()) {
                outs = (*Sample)[int(PhaseAccumulator)] * 4 * TGlobal::OscAmplitude;
            }
            PhaseAccumulator += Hz / Scanspeed;
        }
    }
    syncout.Clear();
}
