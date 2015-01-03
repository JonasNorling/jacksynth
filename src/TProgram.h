/* -*- mode: c++ -*- */
#pragma once

#include "TBaseEffect.h"
#include "TSampleLoader.h"
#ifdef HAVE_LIBGIG
#include "TGigInstrument.h"
#endif
#include "TVoice.h"
#include "util.h"

#include <list>
#include <vector>
#include <memory>

#include <json/json.h>

class TModulation
{
public:
    enum TSource
    {
        CONSTANT, // 1
        KEY, // Octave count above E3 (note 64, 330Hz)
        PITCHBEND, // -1..1
        LFO1, // -1..1
        LFO2, // -1..1
        EG1, // 0..1
        VELOCITY, // 0..1
        EG1_TIMES_VELO, // 0..1
        MODWHEEL, // 0..1 (Joystick +Y)
        BREATH, // 0..1 (Joystick -Y)
    } Source;
    float Amount; // in octaves (typically -1..1)
    enum TDestination
    {
        NO_DESTINATION,
        OSC1_FREQ,
        OSC1_PW,
        OSC1_LEVEL,
        OSC1_PAN,
        OSC2_FREQ,
        OSC2_PW,
        OSC2_LEVEL,
        OSC2_PAN,
        OSC3_FREQ,
        OSC3_PW,
        OSC3_LEVEL,
        OSC3_PAN,
        F1_CUTOFF,
        F1_PAN,
        F1_RESONANCE,
        F2_CUTOFF,
        F2_PAN,
        F2_RESONANCE,
        VOLUME
    } Destination;
};

enum TParameter
{
    PARAM_FILTER_CUTOFF_HZ = 0x00,
    PARAM_LFO_FREQUENCY_FRACHZ = 0x01,
    PARAM_PULSE_WIDTH = 0x02,
    PARAM_DISTORTION = 0x03,
    PARAM_ENVELOPE = 0x04,
    PARAM_OSC_LEVEL = 0x05,
    PARAM_OSC_TYPE = 0x06,
    PARAM_OSC_DETUNE = 0x07,
    PARAM_OSC_OCTAVE = 0x08,
    PARAM_OSC_SYNC = 0x09,
    PARAM_FILTER_RESONANCE = 0x0a,
    PARAM_FX_MIX = 0x0b,
    PARAM_FX_FEEDBACK = 0x0c,
    PARAM_FX_DELAY = 0x0d,
    PARAM_FX_INSERT_GAIN = 0x0e,
    PARAM_FX_TYPE = 0x0f,
    PARAM_MODULATION0 = 0x20
};

enum TOscType
{
    OSC_OFF = 0,
    OSC_SINE = 1,
    OSC_SQUARE = 2,
    OSC_SAW = 3,
    OSC_WT = 4,
    OSC_SAMPLE = 5,
    OSC_INPUT = 6,
#ifdef HAVE_LIBGIG
    OSC_GIG = 7,
#endif
};

/*
 * A program playing a patch. Like a MIDI channel.
 *
 * The synthesizer topology is defined in this class, by how Process() pipes its data.
 * The audio pipeline is pictured below. There is also a conceptual control pipeline, not
 * pictured. Each MIDI note event causes a new TVoice to be instantiated. That class
 * holds the state of the voice, but does not do any processing on its own.
 *
 * --------------------------------------- Programs -------------------------------------
 * ----------------------- Voices (one per key) ----------------
 *                                               audio inputs --.                  ... --.
 *                                                        ... --+                  ... --+
 * oscillators[3] -> wave shapers[2] -> filters[2] -> ADSR[2] --+--> stereo effects[1] --+--> audio output
 *                                                        ... --´                  ... --´
 *
 */
class TProgram
{
UNCOPYABLE(TProgram)
    ;

private:
    static float JsonParsePitch(const Json::Value& value, float def = 0.0f);
    static TOscType JsonParseOscillatorType(const Json::Value& value);
    static TModulation::TSource JsonParseModSource(const Json::Value& value);
    static TModulation::TDestination JsonParseModDestination(const Json::Value& value);

public:
    typedef std::function<void(int, int, int)> parameter_callback_t;

    TProgram(int patch);
    ~TProgram();

    bool LoadFromFile(std::string filename);

    bool Process(TSampleBufferCollection& in, TSampleBufferCollection& out,
            TSampleBufferCollection& into);

    void SetupConstantModulations();

    TVoice* AllocateVoice();
    void NoteOn(TUnsigned7 note, TUnsigned7 velocity);
    void NoteOff(TUnsigned7 note, TUnsigned7 velocity);
    void SetController(TUnsigned7 cc, TUnsigned7 value);
    void SetPitchBend(float bend); ///< -1..1
    void SetQuarterNoteTime(float t); ///< Inverse of tempo

    void SetParameter(int unit, TParameter param, int value, bool echo = false);
    void DumpParameters();
    void SetParameterChangedCallback(parameter_callback_t cb)
    {
        ParameterChanged = cb;
    }

private:
    enum TConstantModulations
    {
        C_OSC1_PAN = 0,
        C_OSC1_DETUNE,
        C_OSC1_OCTAVE,
        C_OSC1_PB,
        C_OSC2_PAN,
        C_OSC2_DETUNE,
        C_OSC2_OCTAVE,
        C_OSC2_PB,
        C_OSC3_PAN,
        C_OSC3_DETUNE,
        C_OSC3_OCTAVE,
        C_OSC3_PB,
        C_F1_PAN,
        C_F2_PAN,
        C_LAST
    };

    float ModulationFactor(TModulation::TDestination d, const TVoice& voice);
    float ModulationValue(TModulation::TDestination d, const TVoice& voice);

    int Patch;

    std::vector<TVoice> Voices;
    parameter_callback_t ParameterChanged;

    TTimer TimerProcess;
    TTimer TimerUpdates;
    TTimer TimerSamples;

    // Patch configuration
    TFrequency FilterCutoff[TGlobal::Filters];
    TFraction FilterResonance[TGlobal::Filters];
    TFrequency LfoFrequency[TGlobal::Lfos];
    TOscType OscType[TGlobal::Oscillators];
    bool OscSync[TGlobal::Oscillators];
    TFraction OscPw[TGlobal::Oscillators];
    TFraction OscLevel[TGlobal::Oscillators];
    TFraction WaveShaper[TGlobal::Filters];
    TEnvelope::TSettings Envelope[2];
    TFraction InputToEffectsMix;

    // Active modulations
    float PitchBend;
    float ModWheel;
    float Breath;
    TUnsigned7 Sustain;

    // MIDI clock
    THysteresis<float> QuarterNoteTime;

    static TSampleLoader SampleLoader;
#ifdef HAVE_LIBGIG
    static TGigInstrument GigInstrument;
#endif
    std::unique_ptr<TBaseEffect> Effects[TGlobal::Effects];

    std::vector<TModulation> Modulations;
};
