#include "TGigInstrument.h"

#include <fcntl.h>

TGigInstrument::TGigInstrument(std::string filename)
    : RiffFile(), GigFile(), Instrument(NULL), GigThread()
{
    try {
        RiffFile.reset(new RIFF::File(filename));
        GigFile.reset(new gig::File(RiffFile.get()));
        Instrument = GigFile->GetFirstInstrument();
    }
    catch (RIFF::Exception &e) {
        e.PrintMessage();
    }

    if (Instrument) {
        fprintf(stderr, "Loaded instrument from %s:\n", filename.c_str());
        fprintf(stderr, "# %d:%d:%d %s\n", Instrument->MIDIBankCoarse, Instrument->MIDIBankFine,
                Instrument->MIDIProgram, Instrument->pInfo->Name.c_str());
        fprintf(stderr, "# att=%d dB, finetune=%d, pbrange=%d, releasemode=%d, dim range=%d..%d\n",
                Instrument->Attenuation, Instrument->FineTune, Instrument->PitchbendRange,
                Instrument->PianoReleaseMode,
                Instrument->DimensionKeyRange.low, Instrument->DimensionKeyRange.high);

        GigThread.reset(new TGigThread(*this));
        GigThread->Start();
    }
    else {
        fprintf(stderr, "Failed to load instrument\n");
    }
}

int TGigInstrument::StartStreaming(gig::DimensionRegion* dimReg)
{
    if (GigThread) {
        return GigThread->StartStreaming(dimReg);
    }
    else {
        return -1;
    }
}

void TGigInstrument::StopStream(int id)
{
    if (GigThread) {
        GigThread->StopStream(id);
    }
}

TGigStream* TGigInstrument::FindStream(int id)
{
    if (GigThread) {
        return GigThread->FindStream(id);
    }
    else {
        return NULL;
    }
}
/*
 * ******************************************************
 */

TGigThread::TGigThread(TGigInstrument& instrument)
    : Instrument(instrument), StopMe(false), Thread(), StreamRequests(),
      NextStreamId(0), Streams()
{
}

void TGigThread::Start()
{
    Thread.reset(new std::thread([this](){Loop();}));
}

TGigThread::~TGigThread()
{
    StopMe = true;
    Thread->join();
}

void TGigThread::Loop()
{
    // Find all samples and cache them
    fprintf(stderr, "Caching samples...\n");
    for (gig::Region* region = Instrument.Instrument->GetFirstRegion();
            region != NULL; region = Instrument.Instrument->GetNextRegion()) {
        gig::Sample* sample = region->GetSample();
        if (sample) {
            sample->LoadSampleData(TGigInstrument::CachedSamples);
        }

        for (size_t dimReg = 0; dimReg < region->DimensionRegions; dimReg++) {
            sample = region->pDimensionRegions[dimReg]->pSample;
            if (sample) {
                sample->LoadSampleData(TGigInstrument::CachedSamples);
            }
        }
    }
    fprintf(stderr, "Caching finished!\n");


    while (!StopMe) {
        TGigStreamRequest request;
        bool haveRequest = false;
        if (!StreamRequests.Empty()) {
            request = StreamRequests.First();
            StreamRequests.Remove();
            haveRequest = true;
        }

        if (haveRequest) {
            if (request.Action == TGigStreamRequest::START) {
                fprintf(stderr, "Request %d for sample %p\n", request.StreamId, request.DimReg->pSample);
                std::unique_ptr<TGigStream> stream(new TGigStream(request));
                Streams[request.StreamId] = std::move(stream);
            }
            else if (request.Action == TGigStreamRequest::STOP) {
                fprintf(stderr, "Removing stream %d\n", request.StreamId);
                Streams.erase(request.StreamId);
            }
        }

        for (auto s = Streams.begin(); s != Streams.end(); s++) {
            const std::unique_ptr<TGigStream>& stream = s->second;
            gig::Sample* sample = stream->DimReg->pSample;

            if (stream->Writable() >= stream->ReadSize) {
                void* buffer = &stream->Buffer[stream->WritePos];
                size_t readsamples = std::min(stream->ReadSize, stream->LinearlyWritable()) / sample->FrameSize;
                readsamples = sample->ReadAndLoop(buffer, readsamples, &stream->State, stream->DimReg);
                stream->Write(readsamples * sample->FrameSize);
                //fprintf(stderr, "Stream @0x%lx, readable %ld, writable %ld\n",
                //        stream->State.position, stream->Readable(), stream->Writable());
            }
        }

        timespec ts{ 0, 10000000};
        ::nanosleep(&ts, NULL);
    }
}

int TGigThread::StartStreaming(gig::DimensionRegion* dimReg)
{
    // FIXME: This function is called from the JACK thread, we should really use a lockfree
    // data structure here (circular buffer)
    TGigStreamRequest request{TGigStreamRequest::START, dimReg, NextStreamId++};
    if (StreamRequests.Add(request)) {
        return request.StreamId;
    }
    else {
        return -1;
    }
}

void TGigThread::StopStream(int id)
{
    TGigStreamRequest request{TGigStreamRequest::STOP, NULL, id};
    StreamRequests.Add(request);
}

TGigStream* TGigThread::FindStream(int id)
{
    const auto streamit = Streams.find(id);
    if (streamit != Streams.end()) {
        return streamit->second.get();
    }
    else {
        return NULL;
    }
}

/*
 * ******************************************************
 */

TGigStream::TGigStream(const TGigStreamRequest& request)
    : DimReg(request.DimReg), State(), ReadPos(0), WritePos(0), SampleAtReadPos(0)
{
    State.position         = 0;
    State.reverse          = false;
    State.loop_cycles_left = DimReg->pSample->LoopPlayCount;
}
