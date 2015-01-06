/* -*- mode: c++ -*- */
#pragma once

#include <cmath>
#include <algorithm>
#include "util.h"
#include "TBaseOscillator.h"
#include "TGlobal.h"

class TWavetable
{
public:
    TWavetable();

    const TSample* GetTable(unsigned table) const {
        return &Data[Length * table];
    }

    TSample* GetTable(unsigned table) {
        return &Data[Length * table];
    }

    size_t Length;
    unsigned BaseNote; // Highest note number for the first wavetable
    unsigned NotesPerTable;
    size_t Tables;

    TSample* Data;
};

class TWavetableOscillator: public TBaseOscillator
{
UNCOPYABLE(TWavetableOscillator)
    ;

public:
    static const int Wavelength = 2048;

    TWavetableOscillator(const TNoteData& noteData);

    void Process(TSampleBuffer& in, TSampleBuffer& out, TSampleBuffer& syncin,
            TSampleBuffer& syncout)
    {
        for (TSample& outs : out) {
            outs = linterpolate(Subtable, Table.Length, PhaseAccumulator);
            PhaseAccumulator += (Hz * Table.Length) / TGlobal::SampleRate;
            if (PhaseAccumulator >= Table.Length) PhaseAccumulator -= Table.Length;
        }
        syncout.Clear();
    }

private:
    static const TWavetable Table;
    const TSample* Subtable;
};
