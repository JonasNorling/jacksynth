/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include <gig.h>
#include <memory>

class TGigInstrument
{
    UNCOPYABLE(TGigInstrument)
        ;

public:
    TGigInstrument(std::string filename);

    std::unique_ptr<RIFF::File> RiffFile;
    std::unique_ptr<gig::File> GigFile;
    gig::Instrument* Instrument;
};
