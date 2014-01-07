#include "TSampleLoader.h"
#include <sndfile.h>
#include <cstring>
#include <iostream>

TSampleLoader::TSampleLoader(const std::string& filename)
        : Buffer(0, 0), Data(), SampleRate(0)
{
    SF_INFO info;
    memset(&info, 0, sizeof(info));
    SNDFILE* sf = sf_open(filename.c_str(), SFM_READ, &info);
    if (sf != 0) {
        std::cerr << "Opened sample file: " << info.frames << " frames, " << info.samplerate
                << " Hz, " << info.channels << " channels." << std::endl;
        int n = info.frames * info.channels;
        // FIXME: We're reading all channels interleaved here...
        Data.resize(n);
        sf_read_float(sf, Data.data(), n);
        Buffer = TSampleBuffer(Data.data(), n);
        SampleRate = info.samplerate;
        sf_close(sf);
    }
    else {
        std::cerr << "Failed to open sample file " << filename << std::endl;
    }
}
