#include "TWavetableOscillator.h"

std::vector<TSample> generateSine()
{
    const int len = TWavetableOscillator::Wavelength;
    std::vector<TSample> wave(len);
    for (int i = 0; i < len; i++) {
        wave[i] = sin(double(i) / len * 2 * M_PI) * TGlobal::OscAmplitude;
    }
    return wave;
}

/**
 * This squarewave table doesn't have enough harmonics to actually
 * behave like a square wave, but it clearly illustrates the severe
 * aliasing that is caused by these harmonics when higher notes are
 * played.
 */
std::vector<TSample> generateSquare()
{
    const int len = TWavetableOscillator::Wavelength;
    std::vector<TSample> wave(len);
    for (int i = 0; i < len; i++) {
        wave[i] = (sin(double(i) / len * 2 * M_PI) * 1.0 / 1
                + sin(double(3 * i) / len * 2 * M_PI) * 1.0 / 3
                + sin(double(5 * i) / len * 2 * M_PI) * 1.0 / 5
                + sin(double(7 * i) / len * 2 * M_PI) * 1.0 / 7
                + sin(double(9 * i) / len * 2 * M_PI) * 1.0 / 9
                + sin(double(11 * i) / len * 2 * M_PI) * 1.0 / 11
                + sin(double(13 * i) / len * 2 * M_PI) * 1.0 / 13
                + sin(double(15 * i) / len * 2 * M_PI) * 1.0 / 15
                + sin(double(17 * i) / len * 2 * M_PI) * 1.0 / 17
                + sin(double(19 * i) / len * 2 * M_PI) * 1.0 / 19
                + sin(double(21 * i) / len * 2 * M_PI) * 1.0 / 21
                + sin(double(23 * i) / len * 2 * M_PI) * 1.0 / 23
                + sin(double(25 * i) / len * 2 * M_PI) * 1.0 / 25
                + sin(double(27 * i) / len * 2 * M_PI) * 1.0 / 27
                + sin(double(29 * i) / len * 2 * M_PI) * 1.0 / 29
                + sin(double(31 * i) / len * 2 * M_PI) * 1.0 / 31
                + sin(double(33 * i) / len * 2 * M_PI) * 1.0 / 33
                + sin(double(35 * i) / len * 2 * M_PI) * 1.0 / 35) * TGlobal::OscAmplitude;
    }
    return wave;
}

const std::vector<TSample> TWavetableOscillator::SineTable(generateSine());
const std::vector<TSample> TWavetableOscillator::SquareTable(generateSquare());

TWavetableOscillator::TWavetableOscillator()
        : TBaseOscillator(), Wave(SineTable.data())
{
}
