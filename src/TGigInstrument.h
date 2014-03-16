/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include <gig.h>
#include <map>
#include <memory>
#include <queue>
#include <thread>

class TGigInstrument;

/**
 * Circular buffer class. Holds size objects of type T.
 */
template<typename T, int size>
class TCircularBuffer
{
public:
    TCircularBuffer() :
        Read(0), Count(0)
    { }

    /// Add object to end of buffer.
    bool Add(const T& t)
    {
        if (Count >= size) {
            return false;
        }
        Data[(Read + Count) % size] = t;
        Count++;
        return true;
    }

    /// Remove object from beginning of buffer.
    void Remove()
    {
        //LOG("TCircularBuffer::Remove\n");
        assert(Count > 0);
        Read = (Read + 1) % size;
        Count--;
    }

    /// Return object at beginning of buffer. Does not alter the buffer.
    const T& First() const
    {
        assert(Count > 0);
        return Data[Read];
    }

    /// Empty predicate.
    bool Empty() const
    {
        return Count == 0;
    }

private:
    T Data[size];
    size_t Read;
    size_t Count;
};

struct TGigStreamRequest
{
    enum {
        START,
        STOP,
    } Action;
    gig::DimensionRegion* DimReg;
    int StreamId;
};

class TGigStream
{
public:
    TGigStream(const TGigStreamRequest& request);
    gig::DimensionRegion* DimReg;
    gig::playback_state_t State;

    static const size_t ReadSize = 8 * 1024;
    static const size_t BufferSize = 32 * 1024;
    uint8_t Buffer[BufferSize];
    size_t ReadPos;
    size_t WritePos;
    size_t SampleAtReadPos;

    size_t Readable() const {
        return (BufferSize + WritePos - ReadPos) % BufferSize;
    }

    size_t Writable() const {
        return (BufferSize + ReadPos - WritePos - 1) % BufferSize;
    }

    size_t LinearlyWritable() const {
        return std::min(BufferSize - WritePos, Writable());
    }

    void Write(size_t n) {
        WritePos = (WritePos + n) % BufferSize;
    }

    void Read(size_t n) {
        ReadPos = (ReadPos + n) % BufferSize;
    }
};

class TGigThread
{
    UNCOPYABLE(TGigThread)
        ;

public:
    TGigThread(TGigInstrument& instrument);
    virtual ~TGigThread();

    void Start();

    /**
     * Request the disk thread to start streaming this sample
     * @return stream ID
     */
    int StartStreaming(gig::DimensionRegion* dimReg);
    void StopStream(int id);
    TGigStream* FindStream(int id);

private:
    void Loop();

    TGigInstrument& Instrument;
    volatile bool StopMe;
    std::unique_ptr<std::thread> Thread;

    TCircularBuffer<TGigStreamRequest, 64> StreamRequests;
    int NextStreamId;
    std::map<int, std::unique_ptr<TGigStream>> Streams;
};

class TGigInstrument
{
    UNCOPYABLE(TGigInstrument)
        ;

public:
    TGigInstrument(std::string filename);
    int StartStreaming(gig::DimensionRegion* dimReg);
    void StopStream(int id);
    TGigStream* FindStream(int id);

    std::unique_ptr<RIFF::File> RiffFile;
    std::unique_ptr<gig::File> GigFile;
    gig::Instrument* Instrument;

    static const size_t CachedSamples = 8 * 1024;

private:
    std::unique_ptr<TGigThread> GigThread;
};
