#include "TGigOscillator.h"

void TGigOscillator::SetInstrument(TGigInstrument& instrument)
{
    Instrument = &instrument;
    Region = Instrument->Instrument->GetRegion(NoteData.Note);
    if (Region) {
        //fprintf(stderr, "[%d] Region with %d dimensions totalling %d dim regions\n",
        //        NoteData.Note, Region->Dimensions, Region->DimensionRegions);
        for (unsigned d = 0; d < Region->Dimensions; d++) {
            //gig::dimension_def_t& dim = Region->pDimensionDefinitions[d];
            //fprintf(stderr, "  Dimension %d is %d with %d bits\n", d, dim.dimension, dim.bits);
        }

        uint dimValues[8] = {0, NoteData.Velocity, 0, 0, 0, 0, 0, 0};
        gig::DimensionRegion* dimReg = Region->GetDimensionRegionByValue(dimValues);

        //fprintf(stderr, "  Dimension Region: %d loops, note %d\n", dimReg->SampleLoops, dimReg->UnityNote);
        //fprintf(stderr, "  Unity note %d + 0x%x\n", Region->UnityNote, Region->FineTune);

        Sample = dimReg->pSample;
        if (Sample) {
            //fprintf(stderr, "  Sample for note %d, %d chan, %d Hz, %d block align, %d B frame, %d bits, format %d, %lu samples\n",
            //        Sample->MIDIUnityNote, Sample->Channels, Sample->SamplesPerSecond, Sample->BlockAlign,
            //        Sample->FrameSize, Sample->BitDepth, Sample->FormatTag, Sample->SamplesTotal);
            gig::buffer_t sampleData = Sample->GetCache();
            if (sampleData.pStart == NULL) {
                sampleData = Sample->LoadSampleData();
            }
            if (dimReg->PitchTrack) {
                UnityHz = NOTE2HZ(Sample->MIDIUnityNote + double(Sample->FineTune) / double(0x100000000LL));
            }
            else {
                UnityHz = NOTE2HZ(NoteData.Note);
            }
            //fprintf(stderr, "    Unity note %d + 0x%x -> %.2f Hz\n", Sample->MIDIUnityNote, Sample->FineTune, UnityHz);

            StreamId = Instrument->StartStreaming(dimReg);
        }
        else {
            fprintf(stderr, "No sample\n");
        }
    }
    else {
        fprintf(stderr, "No region\n");
    }
}

TGigOscillator::~TGigOscillator()
{
    if (StreamId != -1) {
        Instrument->StopStream(StreamId);
    }
}

void TGigOscillator::Process(TSampleBuffer& in, TSampleBuffer& out,
        TSampleBuffer& syncin, TSampleBuffer& syncout)
{
    syncout.Clear();

    if (Region && Sample) {
        if (!Stream) {
            Stream = Instrument->FindStream(StreamId);
            if (Stream) {
                //fprintf(stderr, "Got stream @ pos %.1lf\n", PhaseAccumulator);
            }
        }

        const double rateCorrection = double(Sample->SamplesPerSecond) / TGlobal::SampleRate;
        const double scanspeed = rateCorrection * (1.0 + (Hz - UnityHz) / UnityHz);
        const TSample gain = TSample(1.0f / 0x8000) * 10 * TGlobal::OscAmplitude;

        if (Stream && Stream->Readable() >= 1024) {
            const int startsampleno = int(PhaseAccumulator);
            for (TSample& outs : out) {
                const int sampleno = int(PhaseAccumulator);
                int offset = sampleno - Stream->SampleAtReadPos;
                const int16_t& s0 = *reinterpret_cast<const int16_t*>
                    (&Stream->Buffer[(Stream->ReadPos + Sample->FrameSize * offset) % Stream->BufferSize]);
                const int16_t& s1 = *reinterpret_cast<const int16_t*>
                    (&Stream->Buffer[(Stream->ReadPos + Sample->FrameSize * (offset+1)) % Stream->BufferSize]);
                outs = linterpolate(gain * s0, gain * s1, PhaseAccumulator);
                PhaseAccumulator += scanspeed;
            }
            const int sampleno = int(PhaseAccumulator);
            const int consumedSamples = sampleno - startsampleno;
            Stream->SampleAtReadPos += consumedSamples;
            Stream->Read(Sample->FrameSize * consumedSamples);
        }
        else {
            gig::buffer_t sampleData = Sample->GetCache();
            const uint8_t* buffer = reinterpret_cast<uint8_t*>(sampleData.pStart);
            const size_t bufferSize = sampleData.Size;

            for (TSample& outs : out) {
                const int sampleno = int(PhaseAccumulator);
                if ((Sample->FrameSize * sampleno + 1) < bufferSize) {
                    const int16_t& s0 = *reinterpret_cast<const int16_t*>(&buffer[Sample->FrameSize * sampleno]);
                    const int16_t& s1 = *reinterpret_cast<const int16_t*>(&buffer[Sample->FrameSize * (sampleno + 1)]);
                    outs = linterpolate(gain * s0, gain * s1, PhaseAccumulator);
                }
                PhaseAccumulator += scanspeed;
            }
        }
    }
}
