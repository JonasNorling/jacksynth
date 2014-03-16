/*
 * Measurements at 880Hz PW=50%, sample rate 44100Hz.
 *   --> 50.1 samples per period.
 * nzc=16, os=32 (--> 32 samples blep length):
 *    fundamental at -34dB, harmonics at -44 -48 -51 -53 -54 -57
 *    worst aliasing at -98dB, first peak at 538Hz.
 * nzc= 8, os=32 (--> 16 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -98dB, first peak at 538Hz.
 * nzc=16, os=16 (--> 32 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -91dB, first peak at 710Hz.
 * nzc= 8, os=64 (--> 16 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -103dB, first peak at 237Hz.
 *    Double peaks above 16.5kHz
 * nzc=16, os=64 (--> 32 samples blep length):
 *    fundamental at -34dB
 *    worst aliasing at -104dB, first peak at 237Hz.
 *    Double peaks above 19kHz
 * nzc= 8, os=128 (--> 16 samples blep length):
 *    fundamental at -34dB
 *    horrible double peaks above 15kHz
 */

#include "TMinBlepPulseOscillator.h"
#include "TGlobal.h"

TMinBlepPulseOscillator::TMinBlepPulseOscillator(const TNoteData& noteData)
        : TBaseOscillator(noteData), BufferPos(0), Target(0), Pw(0), NextPw(0)
{
    PhaseAccumulator = 0.99999999;
    std::fill(Buffer, Buffer + BufferLen, 0);
}

void TMinBlepPulseOscillator::Process(TSampleBuffer& in, TSampleBuffer& out,
        TSampleBuffer& syncin, TSampleBuffer& syncout)
{
    const float A = TGlobal::OscAmplitude;
    for (TSample& outs : out) {
        float pace = Hz / TGlobal::SampleRate;
        assert(pace > 0);
        if (pace > 0.99) pace = 0.99;

        double lastpos = PhaseAccumulator;
        PhaseAccumulator += pace;
        if (PhaseAccumulator >= 1) {
            PhaseAccumulator -= 1;
            Pw = NextPw;
        }
        const float pw = 0.5 + 0.5 * Pw;
        assert(PhaseAccumulator >= 0);
        assert(PhaseAccumulator <= 1);

        // If Scanpos just went past 0, we need to place a positive
        // edge. If it just went past pw, we need to place a negative
        // edge. The step is constructed from minblep::length samples
        // chosen from minblep::table.

        if ((lastpos < pw && PhaseAccumulator >= pw)
                || (lastpos < pw && lastpos > PhaseAccumulator)) {
            int offset = (PhaseAccumulator - pw) / pace * minblep::overSampling;
            if (PhaseAccumulator < pw) {
                offset = ((1 + PhaseAccumulator) - pw) / pace * minblep::overSampling;
            }
            for (int i = 0; i < BufferLen; i++) {
                Buffer[(BufferPos + i) % BufferLen] -= minblep::table[i
                        * minblep::overSampling + offset];
            }
            Target = 0;
        }
        if (lastpos > PhaseAccumulator) {
            const int offset = PhaseAccumulator / pace * minblep::overSampling;
            for (int i = 0; i < BufferLen; i++) {
                Buffer[(BufferPos + i) % BufferLen] += minblep::table[i
                        * minblep::overSampling + offset];
            }
            Target = 1;
        }

        outs += A * (Buffer[BufferPos % BufferLen] - pw);
        Buffer[BufferPos % BufferLen] = Target;
        BufferPos++;
    }
    syncout.Clear();
}
