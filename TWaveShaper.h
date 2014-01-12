/* -*- mode: c++ -*- */
#pragma once

#include "util.h"
#include "TGlobal.h"

class TWaveShaper
{
UNCOPYABLE(TWaveShaper)
    ;

public:
    TSample Rectify(TSample s)
    {
        // FIXME: Respect Depth
        // FIXME: High-pass or leak a bit to remove the DC component.
        return fabsf(s);
    }

    TSample Polynomial(TSample s)
    {
        // Polynomials will create overtones up to their order
        const float a = Depth;
        const float b = TGlobal::OscAmplitude;
        s = s / b;
        return b * ((1 + a) * s - a * s * s * s);
    }

    TSample MusicDsp1(TSample x)
    {
        // Clippy, tube-like distortion
        // References : Posted by Partice Tarrabia and Bram de Jong
        const float k = 2 * Depth / (1 - Depth);
        const float b = TGlobal::OscAmplitude;
        x = x / b;
        return b * (1 + k) * x / (1 + k * fabsf(x));
    }

    TSample Tanh(TSample x)
    {
        return Depth * tanhf(x) + (1 - Depth) * x;
    }

    TWaveShaper(TFraction depth = 0.0)
            : Depth(depth)
    {
    }

    void Process(TSampleBuffer& in, TSampleBuffer& out)
    {
        std::transform(in.begin(), in.end(), out.begin(), [this](const TSample& s) -> TSample {
            return MusicDsp1(s);
        });
    }

    void SetDepth(TFraction depth)
    {
        Depth = clamp(depth, 0.0f, 0.99f);
    }

private:
    TFraction Depth;
};
