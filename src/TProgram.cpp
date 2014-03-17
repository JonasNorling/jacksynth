#include "TGlobal.h"
#include "TDelayFx.h"
#include "TGigOscillator.h"
#include "TInputOscillator.h"
#include "TMinBlepPulseOscillator.h"
#include "TMinBlepSawOscillator.h"
#include "TProgram.h"
#include "TReverbFx.h"
#include "TSampleOscillator.h"
#include "TSineOscillator.h"
#include "TWavetableOscillator.h"

#include <json/json.h>

#include <cassert>
#include <map>
#include <fstream>

TSampleLoader TProgram::SampleLoader("sample.ogg");
TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/maestro_concert_grand_v2.gig");
//TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/BeeThree.gig");
//TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/MelloBrass.gig");
//TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/Worra's Prophet ProphOrg.gig");

TProgram::TProgram(int patch)
: Patch(patch),
  Voices(),
  ParameterChanged(0),
  TimerProcess("Process"),
  TimerUpdates("Updates"),
  TimerSamples("Samples"),
  InputToEffectsMix(0),
  PitchBend(0),
  ModWheel(0),
  Breath(0),
  Sustain(0),
  QuarterNoteTime(500, 1),
  Modulations()
{
    switch (Patch % 6) {
    case 0: Patch0(); break;
    case 1: Patch1(); break;
    case 2: LoadFromFile("patches/2.json"); break;
    case 3: LoadFromFile("patches/3.json"); break;
    case 4: Patch4(); break;
    case 5: Patch5(); break;
    }
}

TProgram::~TProgram()
{
}

float TProgram::JsonParsePitch(const Json::Value& value)
{
    if (value.isNull()) {
        return 0.0f;
    }
    else if (value.isConvertibleTo(Json::realValue)) {
        return value.asFloat();
    }
    else {
        float v = 0.0f;
        char c = '\0';
        sscanf(value.asString().c_str(), "%f%c", &v, &c);
        switch (c) {
            case 'o': return octaves(v);
            case 's': return semitones(v);
            case 'c': return cents(v);
        }
    }
    return 0.0f;
}

TOscType TProgram::JsonParseOscillatorType(const Json::Value& value)
{
    if (!value.isConvertibleTo(Json::stringValue)) {
        return OSC_OFF;
    }

    const std::string type = value.asString();
    if (type == "off") return OSC_OFF;
    else if (type == "sine") return OSC_SINE;
    else if (type == "square") return OSC_SQUARE;
    else if (type == "saw") return OSC_SAW;
    else if (type == "wt") return OSC_WT;
    else if (type == "sample") return OSC_SAMPLE;
    else if (type == "input") return OSC_INPUT;
    else if (type == "gig") return OSC_GIG;
    else return OSC_OFF;
}

TModulation::TSource TProgram::JsonParseModSource(const Json::Value& value)
{
    if (!value.isConvertibleTo(Json::stringValue)) {
        return TModulation::CONSTANT;
    }

    const std::string src = value.asString();
    if (src == "CONSTANT") return TModulation::CONSTANT;
    else if (src == "KEY") return TModulation::KEY;
    else if (src == "PITCHBEND") return TModulation::PITCHBEND;
    else if (src == "LFO1") return TModulation::LFO1;
    else if (src == "LFO2") return TModulation::LFO2;
    else if (src == "EG1") return TModulation::EG1;
    else if (src == "VELOCITY") return TModulation::VELOCITY;
    else if (src == "EG1_TIMES_VELO") return TModulation::EG1_TIMES_VELO;
    else if (src == "MODWHEEL") return TModulation::MODWHEEL;
    else if (src == "BREATH") return TModulation::BREATH;
    else return TModulation::CONSTANT;
}

TModulation::TDestination TProgram::JsonParseModDestination(const Json::Value& value)
{
    if (!value.isConvertibleTo(Json::stringValue)) {
        return TModulation::NO_DESTINATION;
    }

    const std::string dst = value.asString();
    if (dst == "NO_DESTINATION") return TModulation::NO_DESTINATION;
    if (dst == "OSC1_FREQ") return TModulation::OSC1_FREQ;
    if (dst == "OSC1_PW") return TModulation::OSC1_PW;
    if (dst == "OSC1_LEVEL") return TModulation::OSC1_LEVEL;
    if (dst == "OSC1_PAN") return TModulation::OSC1_PAN;
    if (dst == "OSC2_FREQ") return TModulation::OSC2_FREQ;
    if (dst == "OSC2_PW") return TModulation::OSC2_PW;
    if (dst == "OSC2_LEVEL") return TModulation::OSC2_LEVEL;
    if (dst == "OSC2_PAN") return TModulation::OSC2_PAN;
    if (dst == "OSC3_FREQ") return TModulation::OSC3_FREQ;
    if (dst == "OSC3_PW") return TModulation::OSC3_PW;
    if (dst == "OSC3_LEVEL") return TModulation::OSC3_LEVEL;
    if (dst == "OSC3_PAN") return TModulation::OSC3_PAN;
    if (dst == "F1_CUTOFF") return TModulation::F1_CUTOFF;
    if (dst == "F1_PAN") return TModulation::F1_PAN;
    if (dst == "F1_RESONANCE") return TModulation::F1_RESONANCE;
    if (dst == "F2_CUTOFF") return TModulation::F2_CUTOFF;
    if (dst == "F2_PAN") return TModulation::F2_PAN;
    if (dst == "F2_RESONANCE") return TModulation::F2_RESONANCE;
    if (dst == "VOLUME") return TModulation::VOLUME;
    else return TModulation::NO_DESTINATION;
}

bool TProgram::LoadFromFile(std::string filename)
{
    Modulations.clear();
    SetupConstantModulations();

    std::fstream filestream(filename);

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(filestream, root, true)) {
        fprintf(stderr, "Failed to parse %s: %s\n", filename.c_str(), reader.getFormattedErrorMessages().c_str());
        return false;
    }

    const std::string program = root.get("program", "subtractive").asString();
    const std::string name = root.get("name", "Unnamed").asString();

    printf("Loading %s program: %s\n", program.c_str(), name.c_str());

    // Load oscillator settings
    {
        struct {
            float detune;
            float octave;
            float pb;
            float pan;
        } settings[TGlobal::Oscillators] = { 0 };

        const Json::Value oscs = root["oscs"];
        for (unsigned i = 0; i < oscs.size() && i < TGlobal::Oscillators; i++) {
            const Json::Value osc = oscs[i];

            OscType[i] = JsonParseOscillatorType(osc["type"]);

            OscLevel[i] = osc["level"].asFloat();
            OscPw[i] = osc["pw"].asFloat();
            OscSync[i] = osc["sync"].asBool();

            settings[i].detune = JsonParsePitch(osc["detune"]);
            settings[i].octave = JsonParsePitch(osc["octave"]);
            settings[i].pb = JsonParsePitch(osc["pb"]);
            settings[i].pan = osc["pan"].asFloat();
        }

        Modulations[C_OSC1_DETUNE].Amount = settings[0].detune;
        Modulations[C_OSC2_DETUNE].Amount = settings[1].detune;
        Modulations[C_OSC3_DETUNE].Amount = settings[2].detune;
        Modulations[C_OSC1_PAN].Amount = settings[0].pan;
        Modulations[C_OSC2_PAN].Amount = settings[1].pan;
        Modulations[C_OSC3_PAN].Amount = settings[2].pan;
        Modulations[C_OSC1_OCTAVE].Amount = settings[0].octave;
        Modulations[C_OSC2_OCTAVE].Amount = settings[1].octave;
        Modulations[C_OSC3_OCTAVE].Amount = settings[2].octave;
        Modulations[C_OSC1_PB].Amount = settings[0].pb;
        Modulations[C_OSC2_PB].Amount = settings[1].pb;
        Modulations[C_OSC3_PB].Amount = settings[2].pb;
    }

    // Load LFO settings
    {
        const Json::Value lfos = root["lfos"];
        for (unsigned i = 0; i < lfos.size() && i < TGlobal::Lfos; i++) {
            const Json::Value lfo = lfos[i];
            LfoFrequency[i] = lfo["freq"].asFloat();
        }
    }

    // Load filter settings
    {
        struct {
            float pan;
        } settings[TGlobal::Filters] = { 0 };

        const Json::Value filters = root["filters"];
        for (unsigned i = 0; i < filters.size() && i < TGlobal::Filters; i++) {
            const Json::Value filter = filters[i];
            FilterCutoff[i] = filter["cutoff"].asFloat();
            FilterResonance[i] = filter["resonance"].asFloat();

            settings[i].pan = filter["pan"].asFloat();
        }

        Modulations[C_F1_PAN].Amount = settings[0].pan;
        Modulations[C_F2_PAN].Amount = settings[1].pan;
    }

    // Load ADSR settings
    {
        const Json::Value envs = root["envs"];
        for (unsigned i = 0; i < envs.size(); i++) {
            const Json::Value env = envs[i];
            Envelope[i] = { Attack: env["attack"].asFloat(),
                    Decay: env["decay"].asFloat(),
                    Sustain: env["sustain"].asFloat(),
                    Release: env["release"].asFloat() };
        }
    }

    // Load modulation routing
    {
        const Json::Value mods = root["mods"];
        for (unsigned i = 0; i < mods.size(); i++) {
            const Json::Value mod = mods[i];
            float amount = JsonParsePitch(mod["amount"]);
            Modulations.push_back( { JsonParseModSource(mod["src"]), amount, JsonParseModDestination(mod["dst"]) });
        }
    }

    // Load effects
    {
        const Json::Value effects = root["fx"];
        for (unsigned i = 0; i < effects.size() && i < TGlobal::Effects; i++) {
            Effects[i].reset();

            const Json::Value fx = effects[i];
            const std::string type = fx.get("type", "delay").asString();
            TFraction mix = fx["mix"].asFloat();

            if (type == "delay") {
                float delay = fx["delay"].asFloat();
                float feedback = fx["feedback"].asFloat();
                float distortion = fx["distortion"].asFloat();

                TDelayFx* fx = new TDelayFx();
                fx->SetDelay(delay);
                fx->SetFeedback(feedback);
                fx->SetDistortion(distortion);
                Effects[i].reset(fx);
                Effects[i]->SetMix(mix);
            }
            else if (type == "reverb") {
                Effects[i].reset(new TReverbFx);
                Effects[i]->SetMix(mix);
            }
        }
    }

    return true;
}

/**
 * Well, a constant modulation isn't much of a modulation of
 * course... This is the set of modulations that we want to be able to
 * hard-wire to CCs and knobs, and always have sane default values.
 */
void TProgram::SetupConstantModulations()
{
    Modulations.resize(C_LAST);

    Modulations[C_OSC1_PAN] = {TModulation::CONSTANT, 0, TModulation::OSC1_PAN};
    Modulations[C_OSC1_DETUNE] = {TModulation::CONSTANT, 0, TModulation::OSC1_FREQ};
    Modulations[C_OSC1_OCTAVE] = {TModulation::CONSTANT, 0, TModulation::OSC1_FREQ};
    Modulations[C_OSC1_PB] = {TModulation::PITCHBEND, semitones(2), TModulation::OSC1_FREQ};

    Modulations[C_OSC2_PAN] = {TModulation::CONSTANT, 0, TModulation::OSC2_PAN};
    Modulations[C_OSC2_DETUNE] = {TModulation::CONSTANT, 0, TModulation::OSC2_FREQ};
    Modulations[C_OSC2_OCTAVE] = {TModulation::CONSTANT, 0, TModulation::OSC2_FREQ};
    Modulations[C_OSC2_PB] = {TModulation::PITCHBEND, semitones(2), TModulation::OSC2_FREQ};

    Modulations[C_OSC3_PAN] = {TModulation::CONSTANT, 0, TModulation::OSC3_PAN};
    Modulations[C_OSC3_DETUNE] = {TModulation::CONSTANT, 0, TModulation::OSC3_FREQ};
    Modulations[C_OSC3_OCTAVE] = {TModulation::CONSTANT, 0, TModulation::OSC3_FREQ};
    Modulations[C_OSC3_PB] = {TModulation::PITCHBEND, semitones(2), TModulation::OSC3_FREQ};

    Modulations[C_F1_PAN] = {TModulation::CONSTANT, 0, TModulation::F1_PAN};
    Modulations[C_F2_PAN] = {TModulation::CONSTANT, 0, TModulation::F2_PAN};
}

void TProgram::Patch0()
{
    Modulations.clear();
    SetupConstantModulations();

    OscType[0] = OSC_SAW;
    OscType[1] = OSC_SAW;
    OscType[2] = OSC_OFF;

    OscPw[0] = 0;
    OscPw[1] = 0;
    OscPw[2] = 0;

    OscSync[0] = false;
    OscSync[1] = false;
    OscSync[2] = false;

    OscLevel[0] = 0.0;
    OscLevel[1] = 1.0;
    OscLevel[2] = 0.0;

    WaveShaper[0] = 0.0;
    WaveShaper[1] = 0.0;
    LfoFrequency[0] = 1;
    LfoFrequency[1] = 1;
    FilterCutoff[0] = 20000;
    FilterCutoff[1] = 20000;
    FilterResonance[0] = 0.15;
    FilterResonance[1] = 0.15;
    Envelope[0] = {Attack: 20, Decay: 0, Sustain: 1.0, Release: 20};
    Envelope[1] = {Attack: 20, Decay: 0, Sustain: 1.0, Release: 20};

    Effects[0].reset();
}

void TProgram::Patch1()
{
    Modulations.clear();
    SetupConstantModulations();
    // Oscillators start out at the key's frequency, modulations are
    // applied multiplicatively to the pitch (i.e. exponentially to the
    // frequency).

    OscType[0] = OscType[1] = OscType[2] = OSC_SQUARE;

    Modulations[C_OSC1_DETUNE].Amount = cents(10);
    Modulations[C_OSC2_DETUNE].Amount = cents(-10);
    Modulations[C_OSC1_PAN].Amount = 1;
    Modulations[C_OSC2_PAN].Amount = -1;
    Modulations[C_F1_PAN].Amount = 1;
    Modulations[C_F2_PAN].Amount = -1;

    Modulations.push_back( { TModulation::LFO1, cents(10), TModulation::OSC1_FREQ });
    Modulations.push_back( { TModulation::LFO1, cents(10), TModulation::OSC2_FREQ });

    OscPw[0] = 0.5;
    OscPw[1] = 0.4;
    OscPw[2] = 0;

    OscSync[0] = false;
    OscSync[1] = false;
    OscSync[2] = false;

    OscLevel[0] = 0.5;
    OscLevel[1] = 0.5;
    OscLevel[2] = 1.0;

    WaveShaper[0] = 0.0;
    WaveShaper[1] = 0.0;

    Envelope[0] = {Attack: 20, Decay: 0, Sustain: 1.0, Release: 1000};
    Envelope[1] = {Attack: 1300, Decay: 1300, Sustain: 0.8, Release: 1000};

    LfoFrequency[0] = 7;
    LfoFrequency[1] = 0.3;

    Modulations.push_back( { TModulation::LFO2, 1, TModulation::OSC1_PW });
    Modulations.push_back( { TModulation::LFO2, -1, TModulation::OSC2_PW });

    // Sweep filter based on envelope 1: start out at 100 Hz and follow
    // the envelope up to 25600 Hz (8 octaves) (which is capped at Fs/2).
    FilterCutoff[0] = 100;
    FilterCutoff[1] = 100;
    FilterResonance[0] = 0.15;
    FilterResonance[1] = 0.15;
    Modulations.push_back( { TModulation::EG1, octaves(7), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::EG1, octaves(7), TModulation::F2_CUTOFF });
    // 100% keytracking
    //Modulations.push_back({ TModulation::KEY, octaves(1), TModulation::F1_CUTOFF });
    // Open up the filter quicker at higher velocities
    Modulations.push_back( { TModulation::VELOCITY, octaves(5), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::VELOCITY, octaves(5), TModulation::F2_CUTOFF });

    Modulations.push_back( { TModulation::MODWHEEL, -octaves(7), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::MODWHEEL, -octaves(7), TModulation::F2_CUTOFF });

    TDelayFx* fx = new TDelayFx();
    fx->SetDelay(500);
    fx->SetFeedback(0.5);
    fx->SetDistortion(0.15);
    Effects[0].reset(fx);
    Effects[0]->SetMix(0.5);
}

/*
 * Gig sample test patch
 */
void TProgram::Patch4()
{
    Modulations.clear();
    SetupConstantModulations();

    OscType[0] = OSC_GIG;
    OscType[1] = OSC_OFF;
    OscType[2] = OSC_OFF;

    OscPw[0] = 0;
    OscPw[1] = 0;
    OscPw[2] = 0;

    OscSync[0] = false;
    OscSync[1] = false;
    OscSync[2] = false;

    OscLevel[0] = 1.0;
    OscLevel[1] = 0.0;
    OscLevel[2] = 0.0;

    WaveShaper[0] = 0.0;
    WaveShaper[1] = 0.0;
    LfoFrequency[0] = 1;
    LfoFrequency[1] = 1;
    FilterCutoff[0] = 20000;
    FilterCutoff[1] = 20000;
    FilterResonance[0] = 0.15;
    FilterResonance[1] = 0.15;
    Envelope[0] = {Attack: 5, Decay: 0, Sustain: 1.0, Release: 500};
    Envelope[1] = {Attack: 20, Decay: 0, Sustain: 1.0, Release: 20};

    Modulations.push_back( { TModulation::BREATH, octaves(-2), TModulation::OSC1_FREQ });
    Modulations.push_back( { TModulation::MODWHEEL, -octaves(5), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::MODWHEEL, -octaves(5), TModulation::F2_CUTOFF });
    Modulations.push_back( { TModulation::MODWHEEL, 4, TModulation::F1_RESONANCE });
    Modulations.push_back( { TModulation::MODWHEEL, 4, TModulation::F2_RESONANCE });

    TDelayFx* fx = new TDelayFx();
    fx->SetDelay(100);
    fx->SetFeedback(0.7);
    fx->SetDistortion(0.0);
    Effects[0].reset(fx);
    Effects[0]->SetMix(0.0);
}

/**
 * A guitarr effect patch
 */
void TProgram::Patch5()
{
    Modulations.clear();
    SetupConstantModulations();

    InputToEffectsMix = 3.0f;

    TDelayFx* fx = new TDelayFx();
    fx->SetDelay(500);
    fx->SetFeedback(0.6);
    fx->SetDistortion(0.2);
    fx->SetLfo(10, 0.3f, 0.6f);
    Effects[0].reset(fx);
    Effects[0]->SetMix(0.5);
}

bool TProgram::Process(TSampleBufferCollection& in, TSampleBufferCollection& out,
        TSampleBufferCollection& into)
{
    TimerProcess.Start();

    bool sounding = false;
    const jack_nframes_t framelen = in[0]->GetCount();
    const jack_nframes_t subframelen = 8;
    assert(framelen % subframelen == 0);
    assert(subframelen <= framelen);

    TSample programData[2][framelen];
    TSampleBuffer bufProgram[2] = { {programData[0], framelen}, {programData[1], framelen} };
    bufProgram[0].Clear();
    bufProgram[1].Clear();

    for (jack_nframes_t sampleno = 0; sampleno < framelen; sampleno += subframelen) {
        TimerUpdates.Start();
        for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
            TVoice* v = voice->voice.get();
            // Step LFOs and envelopes
            v->FiltEg.Step(subframelen);
            v->Lfos[0].SetFrequency(LfoFrequency[0]);
            v->Lfos[1].SetFrequency(LfoFrequency[1]);
            v->Lfos[0].Step(subframelen);
            v->Lfos[1].Step(subframelen);

            if (v->AmpEg.GetState() == TEnvelope::FINISHED) {
                v->State = TVoice::TState::FINISHED;
            }
            else {
                sounding = true;
            }

            // Update modulations
            const float key_hz = v->Hz; // Voice's key frequency

            v->WaveShaper[0].SetDepth(WaveShaper[0]);
            v->WaveShaper[1].SetDepth(WaveShaper[1]);

            v->Oscillators[0]->SetState(v->AmpEg.GetState());
            v->Oscillators[1]->SetState(v->AmpEg.GetState());
            v->Oscillators[2]->SetState(v->AmpEg.GetState());
            v->Oscillators[0]->SetFrequency(key_hz * ModulationFactor(TModulation::OSC1_FREQ, *voice));
            v->Oscillators[1]->SetFrequency(key_hz * ModulationFactor(TModulation::OSC2_FREQ, *voice));
            v->Oscillators[2]->SetFrequency(key_hz * ModulationFactor(TModulation::OSC3_FREQ, *voice));
            v->Oscillators[0]->SetPulseWidth(OscPw[0] * ModulationFactor(TModulation::OSC1_PW, *voice));
            v->Oscillators[1]->SetPulseWidth(OscPw[1] * ModulationFactor(TModulation::OSC2_PW, *voice));
            v->Oscillators[2]->SetPulseWidth(OscPw[2] * ModulationFactor(TModulation::OSC3_PW, *voice));
            v->Oscillators[0]->SetSync(OscSync[0]);
            v->Oscillators[1]->SetSync(OscSync[1]);
            v->Oscillators[2]->SetSync(OscSync[2]);
            v->OscPan[0].SetPan(OscLevel[0] * ModulationFactor(TModulation::OSC1_LEVEL, *voice),
                    ModulationValue(TModulation::OSC1_PAN, *voice));
            v->OscPan[1].SetPan(OscLevel[1] * ModulationFactor(TModulation::OSC2_LEVEL, *voice),
                    ModulationValue(TModulation::OSC2_PAN, *voice));
            v->OscPan[2].SetPan(OscLevel[2] * ModulationFactor(TModulation::OSC3_LEVEL, *voice),
                    ModulationValue(TModulation::OSC3_PAN, *voice));
            v->Filters[0].SetCutoff(FilterCutoff[0] * ModulationFactor(TModulation::F1_CUTOFF, *voice));
            v->Filters[1].SetCutoff(FilterCutoff[1] * ModulationFactor(TModulation::F2_CUTOFF, *voice));
            v->Filters[0].SetResonance(FilterResonance[0] * ModulationFactor(TModulation::F1_RESONANCE, *voice));
            v->Filters[1].SetResonance(FilterResonance[1] * ModulationFactor(TModulation::F1_RESONANCE, *voice));
            v->FiltPan[0].SetPan(1.0, ModulationValue(TModulation::F1_PAN, *voice));
            v->FiltPan[1].SetPan(1.0, ModulationValue(TModulation::F2_PAN, *voice));
        }
        TimerUpdates.Stop();

        // Buffers for rendering samples into
        TSample data[5][subframelen];
        TSampleBuffer bufOscillator(data[0], subframelen);
        TSampleBuffer bufFilter[2] = { {data[1], subframelen}, {data[2], subframelen} };
        TSampleBuffer bufChannel[2] = { {data[3], subframelen}, {data[4], subframelen} };
        TSampleBuffer bufProgramSlice[2] = { bufProgram[0].Slice(sampleno, sampleno + subframelen),
                bufProgram[1].Slice(sampleno, sampleno + subframelen)};

        // Buffers for communicating samples with the rest of the system
        TSampleBuffer bufIn[2] = { in[0]->Slice(sampleno, sampleno + subframelen),
                in[1]->Slice(sampleno, sampleno + subframelen)};
        TSampleBuffer inr(in[1]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into1(into[0]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into2(into[1]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into3(into[2]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into4(into[3]->Slice(sampleno, sampleno + subframelen));

        // Buffer for keeping track of hardsync between oscillators
        TSample hardsyncdata[subframelen];
        TSampleBuffer hardsync(hardsyncdata, subframelen);

        TimerSamples.Start();
        // Generate audio samples for left and right channel
        for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
            TVoice* v = voice->voice.get();

            bufFilter[0].Clear();
            bufFilter[1].Clear();
            bufChannel[0].Clear();
            bufChannel[1].Clear();

            // Render oscillators and pan them to two buffers for the filters.
            // Hard sync is propagated from one oscillator to the next.
            hardsync.Clear();
            for (int i = 0; i < TGlobal::Oscillators; i++) {
                bufOscillator.Clear();
                v->Oscillators[i]->Process(bufIn[0], bufOscillator, hardsync, hardsync);
                v->OscPan[i].Process(bufOscillator, bufFilter[0], bufFilter[1]);
                into1.AddSamples(bufFilter[0]); // All oscillators going to filter 1
                into2.AddSamples(bufFilter[1]); // All oscillators going to filter 2
            }

            // Add distortion to the signal going to the filters in place,
            // render filters in place, pan filters to left and right channels
            for (int i = 0; i < TGlobal::Filters; i++) {
                v->WaveShaper[i].Process(bufFilter[i], bufFilter[i]);
                v->Filters[i].Process(bufFilter[i], bufFilter[i]);
                v->FiltPan[i].Process(bufFilter[i], bufChannel[0], bufChannel[1]);
            }

            into3.AddSamples(bufFilter[0]); // All oscillators filter 1 output
            into4.AddSamples(bufFilter[1]); // All oscillators filter 2 output

            // Output volume adjusted data
            // FIXME: shouldn't the amp envelope be applied before filter, distortion,
            // etc? That would make for a more dynamic sound, and it would be saner.
            for (jack_nframes_t j = 0; j < subframelen; j++) {
                v->AmpEg.Step(1);
                bufProgramSlice[0][j] += bufChannel[0][j] * v->Velocity / 128.0 * v->AmpEg.GetValue();
                bufProgramSlice[1][j] += bufChannel[1][j] * v->Velocity / 128.0 * v->AmpEg.GetValue();
            }
        }
        TimerSamples.Stop();
    }

    // Add audio inputs
    if (InputToEffectsMix > 0.0f) {
        TSample tempData[framelen];
        TSampleBuffer bufTemp(tempData, framelen);

        TAmplifier amplifier(InputToEffectsMix);
        amplifier.Process(*in[0], bufTemp);
        bufProgram[0].AddSamples(bufTemp);
        amplifier.Process(*in[1], bufTemp);
        bufProgram[1].AddSamples(bufTemp);
    }

    // Render effects (for downmixed voices)
    TSampleBufferCollection programBuffers( { &bufProgram[0], &bufProgram[1] });
    for (int i = 0; i < TGlobal::Effects; i++) {
        if (Effects[i]) Effects[i]->Process(programBuffers, programBuffers);
    }

    // Add samples to output buffers
    out[0]->AddSamples(bufProgram[0]);
    out[1]->AddSamples(bufProgram[1]);

    for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
        if (voice->voice->State == TVoice::TState::FINISHED) {
            Voices.erase(voice);
            break;
        }
    }

    if (!sounding) {
        sounding = out[0]->PeakAmplitude() > TGlobal::OscAmplitude * 0.01 ||
                out[1]->PeakAmplitude() > TGlobal::OscAmplitude * 0.01;
    }

    TimerProcess.Stop();

    return sounding;
}

inline float TProgram::ModulationFactor(TModulation::TDestination d, const TSoundingVoice& voice)
{
    float mod = ModulationValue(d, voice);
    return mod != 0 ? fpow2(mod) : 1;
    //return mod != 0 ? exp2f(mod) : 1;
}

inline float TProgram::ModulationValue(TModulation::TDestination d, const TSoundingVoice& voice)
{
    float mod = 0;
    for (const TModulation& m: Modulations) {
        if (m.Destination == d) {
            switch (m.Source) {
            case TModulation::CONSTANT: mod += m.Amount; break;
            case TModulation::KEY: mod += m.Amount * semitones(voice.note - TGlobal::MidiNoteE3); break;
            case TModulation::PITCHBEND: mod += m.Amount * PitchBend; break;
            case TModulation::LFO1: mod += m.Amount * voice.voice->Lfos[0].GetValue(); break;
            case TModulation::LFO2: mod += m.Amount * voice.voice->Lfos[1].GetValue(); break;
            case TModulation::EG1: mod += m.Amount * voice.voice->FiltEg.GetValue(); break;
            case TModulation::VELOCITY: mod += m.Amount * voice.voice->Velocity/128.0; break;
            case TModulation::EG1_TIMES_VELO: mod += m.Amount * voice.voice->FiltEg.GetValue() * voice.voice->Velocity/128.0f; break;
            case TModulation::MODWHEEL: mod += m.Amount * ModWheel; break;
            case TModulation::BREATH: mod += m.Amount * Breath; break;
            }
        }
    }
    return mod;
}

void TProgram::NoteOn(TUnsigned7 note, TUnsigned7 velocity)
{
    float hz = NOTE2HZ(note);
    TVoice* voice = new TVoice(hz, velocity);

    TNoteData noteData;
    noteData.Note = note;
    noteData.Velocity = velocity;

    for (int i = 0; i < TGlobal::Oscillators; i++) {
        switch (OscType[i]) {
        case OSC_SINE:
            voice->Oscillators[i].reset(new TSineOscillator(noteData));
            break;
        case OSC_SQUARE:
            voice->Oscillators[i].reset(new TMinBlepPulseOscillator(noteData));
            break;
        case OSC_SAW:
            voice->Oscillators[i].reset(new TMinBlepSawOscillator(noteData));
            break;
        case OSC_WT:
            voice->Oscillators[i].reset(new TWavetableOscillator(noteData));
            break;
        case OSC_SAMPLE: {
            TSampleOscillator *o = new TSampleOscillator(noteData);
            o->SetSample(SampleLoader.GetBuffer(), TGlobal::HzA4 * double(TGlobal::SampleRate) / SampleLoader.GetSampleRate());
            o->SetLoopPoints(SampleLoader.GetBuffer()->GetCount() * 0.6,
                    SampleLoader.GetBuffer()->GetCount() * 0.8);
            voice->Oscillators[i].reset(o);
            break;
        }
        case OSC_INPUT:
            voice->Oscillators[i].reset(new TInputOscillator(noteData));
            break;
        case OSC_GIG: {
            TGigOscillator *o = new TGigOscillator(noteData);
            o->SetInstrument(GigInstrument);
            voice->Oscillators[i].reset(o);
            break;
        }
        case OSC_OFF:
        default:
            voice->Oscillators[i].reset(new TBaseOscillator(noteData));
        }
    }

    voice->AmpEg.Set(Envelope[0]);
    voice->FiltEg.Set(Envelope[1]);
    Voices.push_back( { note, std::unique_ptr<TVoice>(voice) });

    if (Voices.size() > TGlobal::SoftVoiceLimit) {
        fprintf(stderr, "Now %zu voices\n", Voices.size());

        // Steal an old voice
        for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
            if (!voice->voice->Stolen) {
                if (voice->voice->AmpEg.GetState() < TEnvelope::RELEASE) {
                    voice->voice->State = TVoice::TState::RELEASED;
                    voice->voice->AmpEg.Release(64);
                    voice->voice->FiltEg.Release(64);
                }
                voice->voice->Stolen = true;
                break;
            }
        }
    }
}

void TProgram::NoteOff(TUnsigned7 note, TUnsigned7 velocity)
{
    for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
        if (voice->note == note) {
            if (voice->voice->AmpEg.GetState() < TEnvelope::RELEASE) {
                if (voice->voice->Hold) {
                    // Held by sostenuto pedal; don't release
                    voice->voice->State = TVoice::TState::RELEASED;
                }
                else if (Sustain == 0) {
                    voice->voice->State = TVoice::TState::RELEASED;
                    voice->voice->AmpEg.Release(velocity);
                    voice->voice->FiltEg.Release(velocity);
                }
                else {
                    voice->voice->State = TVoice::TState::SUSTAINED;
                }
            }
        }
    }
}

void TProgram::SetController(TUnsigned7 cc, TUnsigned7 value)
{
    //printf("CC 0x%x (%d) = %d\n", cc, cc, value);

    switch (cc) {
    case 1: // Modwheel (Joystick +Y)
        ModWheel = value / 128.0;
        break;
    case 2: // Breath (Joystick -Y)
        Breath = value / 128.0;
        break;
    case MIDI_CC_SUSTAIN:
        Sustain = value;
        if (Sustain == 0) {
            // Release any sustained notes
            for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
                if (voice->voice->State == TVoice::TState::SUSTAINED) {
                    voice->voice->State = TVoice::TState::RELEASED;
                    voice->voice->AmpEg.Release(0x40);
                    voice->voice->FiltEg.Release(0x40);
                }
            }
        }
        break;
    case MIDI_CC_SOSTENUTO: // Sostenuto (hold pedal)
        // Use as a modifier for the guitar FX program
        if (Patch == 5) {
            SetParameter(0, PARAM_FX_INSERT_GAIN, value, true);
        }

        if (value == 0) {
            // Release all currently latched notes
            for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
                if (voice->voice->Hold) {
                    voice->voice->Hold = false;
                    if (voice->voice->State == TVoice::TState::RELEASED && Sustain == 0) {
                        voice->voice->AmpEg.Release(0x40);
                        voice->voice->FiltEg.Release(0x40);
                    }
                    else if (Sustain) {
                        voice->voice->State = TVoice::TState::SUSTAINED;
                    }
                }
            }
        }
        else {
            // Latch all currently held notes
            for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
                if (voice->voice->State == TVoice::TState::PLAYING) {
                    voice->voice->Hold = true;
                }
            }
        }

        break;

        /* We use the same CC numbers as Waldorf gear at the moment,
         making my setup a bit simpler. */

    case 16: // LFO1 speed
        SetParameter(0, PARAM_LFO_FREQUENCY_FRACHZ, VAL2HZ_LO(value) * 128, true);
        break;
    case 20: // LFO2 speed
        SetParameter(1, PARAM_LFO_FREQUENCY_FRACHZ, VAL2HZ_LO(value) * 128, true);
        break;
    case 24: // LFO3 speed
        //SetParameter(2, PARAM_LFO_FREQUENCY_FRACHZ, VAL2HZ_LO(value)*128, true);
        break;

    case 27: // OSC1 octave
        SetParameter(0, PARAM_OSC_OCTAVE, value, true);
        break;
    case 29: // OSC1 detune
        SetParameter(0, PARAM_OSC_DETUNE, value, true);
        break;
    case 33: // OSC1 pulsewidth
        SetParameter(0, PARAM_PULSE_WIDTH, value, true);
        break;
    case 52: // OSC1 level
        SetParameter(0, PARAM_OSC_LEVEL, value, true);
        break;

    case 35: // OSC2 octave
        SetParameter(1, PARAM_OSC_OCTAVE, value, true);
        break;
    case 37: // OSC2 detune
        SetParameter(1, PARAM_OSC_DETUNE, value, true);
        break;
    case 40: // OSC2 pulsewidth
        SetParameter(1, PARAM_PULSE_WIDTH, value, true);
        break;
    case 56: // OSC2 level
        SetParameter(1, PARAM_OSC_LEVEL, value, true);
        break;
    case 49: // OSC3 hard syncing OSC2
        SetParameter(1, PARAM_OSC_SYNC, value, true);
        break;

    case 42: // OSC3 octave
        SetParameter(2, PARAM_OSC_OCTAVE, value, true);
        break;
    case 44: // OSC3 detune
        SetParameter(2, PARAM_OSC_DETUNE, value, true);
        break;
    case 47: // OSC3 pulsewidth
        SetParameter(2, PARAM_PULSE_WIDTH, value, true);
        break;
    case 58: // OSC3 level

    case 69: // 0x45 F1 cutoff
        SetParameter(0, PARAM_FILTER_CUTOFF_HZ, VAL2HZ_HI(value), true);
        break;
    case 70: // 0x46 F1 resonance
        SetParameter(0, PARAM_FILTER_RESONANCE, value, true);
        break;
    case 71: // 0x47 F1 drive
        SetParameter(0, PARAM_DISTORTION, value, true);
        break;

    case 80: // 0x50 F2 cutoff
        SetParameter(1, PARAM_FILTER_CUTOFF_HZ, VAL2HZ_HI(value), true);
        break;
    case 81: // 0x51 F2 resonance
        SetParameter(1, PARAM_FILTER_RESONANCE, value, true);
        break;
    case 82: // 0x52 F2 drive
        SetParameter(1, PARAM_DISTORTION, value, true);
        break;

    case 93: // FX1 mix
        SetParameter(0, PARAM_FX_MIX, value, true);
        break;
    case 94: // FX2 mix
        SetParameter(1, PARAM_FX_MIX, value, true);
        break;

    case 95: // Filter env attack
        SetParameter(0x00, PARAM_ENVELOPE, VAL2MS(value), true);
        break;
    case 96: // Filter env decay
        SetParameter(0x01, PARAM_ENVELOPE, VAL2MS(value), true);
        break;
    case 97: // Filter env sustain
        SetParameter(0x02, PARAM_ENVELOPE, value, true);
        break;
    case 100: // Filter env release
        SetParameter(0x03, PARAM_ENVELOPE, VAL2MS(value), true);
        break;

    case 101: // Amp env attack
        SetParameter(0x10, PARAM_ENVELOPE, VAL2MS(value), true);
        break;
    case 102: // Amp env decay
        SetParameter(0x11, PARAM_ENVELOPE, VAL2MS(value), true);
        break;
    case 103: // Amp env sustain
        SetParameter(0x12, PARAM_ENVELOPE, value, true);
        break;
    case 106: // Amp env release
        SetParameter(0x13, PARAM_ENVELOPE, VAL2MS(value), true);
        break;

    default:
        break;
    }
}

void TProgram::SetPitchBend(float bend)
{
    PitchBend = bend;
}

void TProgram::SetQuarterNoteTime(float t)
{
    QuarterNoteTime = t;

    TDelayFx* delayFx = dynamic_cast<TDelayFx*>(Effects[0].get());
    if (delayFx) {
        //delayFx->SetDelaySamples(QuarterNoteTime);
    }
}

void TProgram::SetParameter(int unit, TParameter param, int value, bool echo)
{
    //printf("Sysex: param 0x%x[%d] = %d\n", param, unit, value);

    TFraction fractvalue = value / 128.0f;

    if (param == PARAM_FILTER_CUTOFF_HZ) {
        FilterCutoff[unit] = value;
    }
    else if (param == PARAM_FILTER_RESONANCE) {
        FilterResonance[unit] = fractvalue;
    }
    else if (param == PARAM_LFO_FREQUENCY_FRACHZ) {
        LfoFrequency[unit] = fractvalue;
    }
    else if (param == PARAM_PULSE_WIDTH) {
        OscPw[unit] = fractvalue;
    }
    else if (param == PARAM_OSC_SYNC) {
        OscSync[unit] = !!value;
    }
    else if (param == PARAM_DISTORTION) {
        WaveShaper[unit] = fractvalue;
    }
    else if (param == PARAM_ENVELOPE) {
        TEnvelope::TSettings& e = Envelope[unit >> 4];
        switch (unit & 0x0f) {
        case 0: e.Attack = value; break;
        case 1: e.Decay = value; break;
        case 2: e.Sustain = fractvalue; break;
        case 3: e.Release = value; break;
        }
    }
    else if (param == PARAM_OSC_LEVEL) {
        OscLevel[unit] = fractvalue;
    }
    else if (param == PARAM_OSC_TYPE) {
        OscType[unit] = static_cast<TOscType>(value);
    }
    else if (param == PARAM_OSC_DETUNE) {
        switch (unit) {
        case 0: Modulations[C_OSC1_DETUNE].Amount = cents(value); break;
        case 1: Modulations[C_OSC2_DETUNE].Amount = cents(value); break;
        case 2: Modulations[C_OSC3_DETUNE].Amount = cents(value); break;
        }
    }
    else if (param == PARAM_OSC_OCTAVE) {
        switch (unit) {
        case 0: Modulations[C_OSC1_OCTAVE].Amount = semitones(value); break;
        case 1: Modulations[C_OSC2_OCTAVE].Amount = semitones(value); break;
        case 2: Modulations[C_OSC3_OCTAVE].Amount = semitones(value); break;
        }
    }
    else if (param == PARAM_FX_MIX) {
        if (Effects[unit]) {
            Effects[unit]->SetMix(fractvalue);
        }
    }
    else if (param == PARAM_FX_FEEDBACK) {
        if (Effects[unit]) {
            TDelayFx* delayFx = dynamic_cast<TDelayFx*>(Effects[unit].get());
            if (delayFx) {
                delayFx->SetFeedback(fractvalue);
            }
        }
    }
    else if (param == PARAM_FX_DELAY) {
        if (Effects[unit]) {
            TDelayFx* delayFx = dynamic_cast<TDelayFx*>(Effects[unit].get());
            if (delayFx) {
                delayFx->SetDelay(value);
            }
        }
    }
    else if (param == PARAM_FX_INSERT_GAIN) {
        if (Effects[unit]) {
            TDelayFx* delayFx = dynamic_cast<TDelayFx*>(Effects[unit].get());
            if (delayFx) {
                fprintf(stderr, "Insert gain %.1f\n", fractvalue);
                delayFx->SetInsertGain(fractvalue);
            }
        }
    }
    else if ((param & 0xe0) == 0x20) {
        int n = 0;
        for (TModulation& m : Modulations) {
            if (n == (param & 0x1f)) {
                switch (unit) {
                case 0: m.Source = static_cast<TModulation::TSource>(value); break;
                case 1: m.Destination = static_cast<TModulation::TDestination>(value); break;
                case 2: m.Amount = value / 600.0; break;
                }
                break;
            }
            n++;
        }
    }
    else {
        printf("Strange sysex\n");
    }

    if (echo) {
        ParameterChanged(unit, param, value);
    }
}

void TProgram::DumpParameters()
{
    ParameterChanged(0, PARAM_FILTER_CUTOFF_HZ, FilterCutoff[0]);
    ParameterChanged(1, PARAM_FILTER_CUTOFF_HZ, FilterCutoff[1]);
    ParameterChanged(0, PARAM_FILTER_RESONANCE, FilterResonance[0] * 128);
    ParameterChanged(1, PARAM_FILTER_RESONANCE, FilterResonance[1] * 128);
    ParameterChanged(0, PARAM_LFO_FREQUENCY_FRACHZ, LfoFrequency[0] * 128);
    ParameterChanged(1, PARAM_LFO_FREQUENCY_FRACHZ, LfoFrequency[1] * 128);
    ParameterChanged(0, PARAM_PULSE_WIDTH, OscPw[0] * 128);
    ParameterChanged(1, PARAM_PULSE_WIDTH, OscPw[1] * 128);
    ParameterChanged(2, PARAM_PULSE_WIDTH, OscPw[2] * 128);
    ParameterChanged(0, PARAM_OSC_LEVEL, OscLevel[0] * 128);
    ParameterChanged(1, PARAM_OSC_LEVEL, OscLevel[1] * 128);
    ParameterChanged(2, PARAM_OSC_LEVEL, OscLevel[2] * 128);
    ParameterChanged(0, PARAM_OSC_TYPE, OscType[0]);
    ParameterChanged(1, PARAM_OSC_TYPE, OscType[1]);
    ParameterChanged(2, PARAM_OSC_TYPE, OscType[2]);
    ParameterChanged(0, PARAM_OSC_SYNC, OscSync[0]);
    ParameterChanged(1, PARAM_OSC_SYNC, OscSync[1]);
    ParameterChanged(2, PARAM_OSC_SYNC, OscSync[2]);
    ParameterChanged(0, PARAM_DISTORTION, WaveShaper[0] * 128);
    ParameterChanged(1, PARAM_DISTORTION, WaveShaper[1] * 128);

    ParameterChanged(0x00, PARAM_ENVELOPE, Envelope[0].Attack);
    ParameterChanged(0x01, PARAM_ENVELOPE, Envelope[0].Decay);
    ParameterChanged(0x02, PARAM_ENVELOPE, Envelope[0].Sustain * 128);
    ParameterChanged(0x03, PARAM_ENVELOPE, Envelope[0].Release);
    ParameterChanged(0x10, PARAM_ENVELOPE, Envelope[1].Attack);
    ParameterChanged(0x11, PARAM_ENVELOPE, Envelope[1].Decay);
    ParameterChanged(0x12, PARAM_ENVELOPE, Envelope[1].Sustain * 128);
    ParameterChanged(0x13, PARAM_ENVELOPE, Envelope[1].Release);

    int n = 0;
    for (const TModulation& m : Modulations) {
        ParameterChanged(0, PARAM_MODULATION0 + n, m.Source);
        ParameterChanged(1, PARAM_MODULATION0 + n, m.Destination);
        ParameterChanged(2, PARAM_MODULATION0 + n, m.Amount * 600);
        n++;
    }
}