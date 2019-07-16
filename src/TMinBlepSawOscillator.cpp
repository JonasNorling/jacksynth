#include "TMinBlepSawOscillator.h"
#include "TGlobal.h"

TMinBlepSawOscillator::TMinBlepSawOscillator(const TNoteData& noteData)
        : TBaseOscillator(noteData), BufferPos(0)
{
    PhaseAccumulator = 0.5f;
    std::fill(Buffer, Buffer + BufferLen, 1.0f);
}

/*
 *  |    |    |
 *  |   /|   /|
 *  |  / |  / |
 *  | /  | /  | /
 *  |/   |/   |/
 *
 *  ^    ^    ^  Place negative minBLEPs here.
 */
void TMinBlepSawOscillator::Process(TSampleBuffer& in, TSampleBuffer& out,
        TSampleBuffer& syncin, TSampleBuffer& syncout)
{
    const float A = TGlobal::OscAmplitude;
    for (size_t sn = 0; sn < out.GetCount(); sn++) {
        float pace = Hz / TGlobal::SampleRate;
        assert(pace > 0);
        if (pace > 0.99) pace = 0.99;

        double lastpos = PhaseAccumulator;
        PhaseAccumulator += pace;
        if (PhaseAccumulator >= 1) {
            PhaseAccumulator -= 1;
        }

        float stepA = 1.0f;
        if (Sync && syncin[sn] > 0) {
            stepA = lastpos; // FIXME: This is not precise
            PhaseAccumulator = syncin[sn] * pace; // FIXME: Is this right?
        }

        assert(PhaseAccumulator >= 0);
        assert(PhaseAccumulator <= 1);

        if (lastpos > PhaseAccumulator) {
            // Wrapped around or synced to beginning of wave. Place a step here.
            syncout[sn] = PhaseAccumulator / pace + 0.0000001;
            const int offset = PhaseAccumulator / pace * minblep::overSampling;
            for (int i = 0; i < BufferLen; i++) {
                Buffer[(BufferPos + i) % BufferLen] += stepA
                        * (-1
                                + minblep::table[i * minblep::overSampling
                                        + offset]);
            }
        }
        else {
            syncout[sn] = 0;
        }

        // Correct for the fact that the BLEP has an average (integral) that is different from a perfect step
        TSample offsetCorrection = (minblep::integral - minblep::length) * pace;

        out[sn] += A
                * (0.5 + PhaseAccumulator - Buffer[BufferPos % BufferLen]
                        + offsetCorrection);
        Buffer[BufferPos % BufferLen] = 1.0f;
        BufferPos++;
    }
}
