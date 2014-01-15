#include "TGigOscillator.h"

void TGigOscillator::SetInstrument(TGigInstrument& instrument)
{
    Instrument = &instrument;
    Region = Instrument->Instrument->GetRegion(NoteData.Note);
    if (Region) {
        fprintf(stderr, "[%d] Region with %d dimensions totalling %d dim regions\n",
                NoteData.Note, Region->Dimensions, Region->DimensionRegions);
        for (unsigned d = 0; d < Region->Dimensions; d++) {
            gig::dimension_def_t& dim = Region->pDimensionDefinitions[d];
            //fprintf(stderr, "  Dimension %d is %d with %d bits\n", d, dim.dimension, dim.bits);
        }

        uint dimValues[8] = {0, NoteData.Velocity, 0, 0, 0, 0, 0, 0};
        gig::DimensionRegion* dimReg = Region->GetDimensionRegionByValue(dimValues);

        fprintf(stderr, "  Dimension Region: %d loops, note %d\n", dimReg->SampleLoops, dimReg->UnityNote);
        //fprintf(stderr, "  Unity note %d + 0x%x\n", Region->UnityNote, Region->FineTune);

        Sample = dimReg->pSample;
        if (Sample) {
            fprintf(stderr, "  Sample for note %d, %d chan, %d Hz, %d block align, %d B frame, %d bits, format %d, %lu samples\n",
                    Sample->MIDIUnityNote, Sample->Channels, Sample->SamplesPerSecond, Sample->BlockAlign,
                    Sample->FrameSize, Sample->BitDepth, Sample->FormatTag, Sample->SamplesTotal);
            SampleData = Sample->GetCache();
            if (SampleData.pStart == NULL) {
                SampleData = Sample->LoadSampleData();
            }
            if (dimReg->PitchTrack) {
                UnityHz = NOTE2HZ(Sample->MIDIUnityNote + double(Sample->FineTune) / double(0x100000000LL));
            }
            else {
                UnityHz = NOTE2HZ(NoteData.Note);
            }
            //fprintf(stderr, "    Unity note %d + 0x%x -> %.2f Hz\n", Sample->MIDIUnityNote, Sample->FineTune, UnityHz);
        }
        else {
            fprintf(stderr, "No sample\n");
        }
    }
    else {
        fprintf(stderr, "No region\n");
    }
}

void TGigOscillator::Process(TSampleBuffer& in, TSampleBuffer& out,
        TSampleBuffer& syncin, TSampleBuffer& syncout)
{
    syncout.Clear();

    if (Region) {
        if (Sample) {
            const double rateCorrection = double(Sample->SamplesPerSecond) / TGlobal::SampleRate;
            const double Scanspeed = rateCorrection * (1.0 + (Hz - UnityHz) / UnityHz);
            const int stride = Sample->FrameSize / 2; // Assuming 2-byte samples
            const TSample gain = TSample(1.0f / 0x8000) * 10 * TGlobal::OscAmplitude;

            for (TSample& outs : out) {
                int sampleno = int(PhaseAccumulator) % Sample->SamplesTotal;
                const int16_t& s0 = ((int16_t*)SampleData.pStart)[stride * sampleno];
                const int16_t& s1 = ((int16_t*)SampleData.pStart)[stride * (sampleno + 1)];
                outs = linterpolate(gain * s0, gain * s1, PhaseAccumulator);
                PhaseAccumulator += Scanspeed;
            }
        }
    }
}
