#include "TAudioFileWriter.h"
#include <sndfile.h>
#include <iostream>

TAudioFileWriter::TAudioPort::TAudioPort(const TAudioPort& p) :
  Writer(p.Writer), Channel(p.Channel)
{
}

TAudioFileWriter::TAudioPort::TAudioPort(TAudioFileWriter& writer, unsigned channel) :
  Writer(writer), Channel(channel)
{
}

TSampleBuffer TAudioFileWriter::TAudioPort::GetBuffer(jack_nframes_t nframes)
{
    Buffer.resize(nframes);
    return TSampleBuffer(Buffer.data(), Buffer.size());
}

void TAudioFileWriter::TAudioPort::Commit()
{
    Writer.Write(TSampleBuffer(Buffer.data(), Buffer.size()), Channel);
    Buffer.clear();
}

TAudioFileWriter::TAudioFileWriter(std::string filename, unsigned channels, unsigned samplerate) :
        Channels(channels)
{
    SF_INFO info = {};
    info.channels = channels;
    info.samplerate = samplerate;
    info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    Handle = sf_open(filename.c_str(), SFM_WRITE, &info);
    if (!Handle) {
        std::cerr << "Failed to open audio file " << filename << std::endl;
    }

    for (unsigned c = 0; c < channels; c++) {
        Ports.push_back(std::unique_ptr<TAudioFileWriter::TAudioPort>(new TAudioFileWriter::TAudioPort(*this, c)));
    }

    ChannelWritten.resize(Channels);
}

TAudioFileWriter::~TAudioFileWriter()
{
    if (Handle) {
        sf_close(Handle);
    }
}

TAudioPortCollection TAudioFileWriter::GetPorts()
{
    std::vector<IAudioPort*> v;
    for (std::unique_ptr<TAudioFileWriter::TAudioPort>& p : Ports) {
        v.push_back(p.get());
    }
    return v;
}

void TAudioFileWriter::Write(const TSampleBuffer& data, unsigned channel)
{
    if (Buffer.empty()) {
        Buffer.resize(data.GetCount() * Channels);
    }
    ChannelWritten[channel] = true;

    for (unsigned s = 0; s < data.GetCount(); s++) {
        Buffer[Channels * s + channel] = data[s];
    }

    bool allWritten = true;
    for (unsigned c = 0; c < Channels; c++) {
        allWritten &= ChannelWritten[c];
    }

    if (allWritten) {
        sf_write_float(Handle, Buffer.data(), Buffer.size());
        for (unsigned c = 0; c < Channels; c++) {
            ChannelWritten[c] = false;
        }
        Buffer.clear();
    }
}
