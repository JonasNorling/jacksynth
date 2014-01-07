/* -*- mode: c++ -*- */
#pragma once

#include <string>
#include <memory>
#include "IAudioPort.h"

class TSampleLoader
{
public:
    TSampleLoader(const std::string& filename);
    TSampleBuffer* GetBuffer()
    {
        return &Buffer;
    }

private:
    TSampleBuffer Buffer;
    std::vector<TSample> Data;
};

