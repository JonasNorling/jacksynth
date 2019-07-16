#include "TGlobal.h"
#include "TDelayFx.h"
#ifdef HAVE_LIBGIG
#include "TGigOscillator.h"
#endif
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
#ifdef HAVE_LIBGIG
TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/maestro_concert_grand_v2.gig");
//TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/BeeThree.gig");
//TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/MelloBrass.gig");
//TGigInstrument TProgram::GigInstrument("/home/jonas/gigas/Worra's Prophet ProphOrg.gig");
#endif

TProgram::TProgram(int patch)
: Patch(patch),
  Voices(TGlobal::HardVoiceLimit),
  ParameterChanged(0),
  TimerProcess("Process"),
  TimerUpdates("Updates"),
  TimerSamples("Samples"),
  FilterCutoff{0},
  FilterResonance{0},
  LfoFrequency{0},
  OscType{OSC_OFF},
  OscSync{false},
  OscPw{0},
  OscLevel{0},
  WaveShaper{0},
  Envelope{{0}, {0}},
  InputToEffectsMix(0),
  PitchBend(0),
  ModWheel(0),
  Breath(0),
  Sustain(0),
  QuarterNoteTime(500, 1),
  Modulations()
{
    switch (Patch % 7) {
    case 0: LoadFromFile("patches/0.json"); break;
    case 1: LoadFromFile("patches/1.json"); break;
    case 2: LoadFromFile("patches/2.json"); break;
    case 3: LoadFromFile("patches/microQ.json"); break;
    case 4: LoadFromFile("patches/gig.json"); break;
    case 5: LoadFromFile("patches/gfx.json"); break;
    case 6: LoadFromFile("patches/wt.json"); break;
    }
}

TProgram::~TProgram()
{
}

float TProgram::JsonParsePitch(const Json::Value& value, float def)
{
    if (value.isNull()) {
        return def;
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
    return def;
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
#ifdef HAVE_LIBGIG
    else if (type == "gig") return OSC_GIG;
#endif
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

    // Load input (feedthrough) mixin level
    InputToEffectsMix = root["input-level"].asFloat();

    // Load oscillator settings
    {
        struct {
            float detune;
            float octave;
            float pb;
            float pan;
        } settings[TGlobal::Oscillators] = { };

        const Json::Value oscs = root["oscs"];
        for (unsigned i = 0; i < oscs.size() && i < TGlobal::Oscillators; i++) {
            const Json::Value osc = oscs[i];

            OscType[i] = JsonParseOscillatorType(osc["type"]);

            OscLevel[i] = osc["level"].asFloat();
            OscPw[i] = osc["pw"].asFloat();
            OscSync[i] = osc["sync"].asBool();

            settings[i].detune = JsonParsePitch(osc["detune"]);
            settings[i].octave = JsonParsePitch(osc["octave"]);
            settings[i].pb = JsonParsePitch(osc["pb"], semitones(2));
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
        } settings[TGlobal::Filters] = { };

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
            Envelope[i] = { env["attack"].asFloat(),
                    env["decay"].asFloat(),
                    env["sustain"].asFloat(),
                    env["release"].asFloat() };
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
        for (TVoice& voice: Voices) {
            TVoice* v = &voice;
            if (v->State == TVoice::TState::FINISHED) {
                continue;
            }

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
            v->Oscillators[0]->SetFrequency(key_hz * ModulationFactor(TModulation::OSC1_FREQ, *v));
            v->Oscillators[1]->SetFrequency(key_hz * ModulationFactor(TModulation::OSC2_FREQ, *v));
            v->Oscillators[2]->SetFrequency(key_hz * ModulationFactor(TModulation::OSC3_FREQ, *v));
            v->Oscillators[0]->SetPulseWidth(OscPw[0] * ModulationFactor(TModulation::OSC1_PW, *v));
            v->Oscillators[1]->SetPulseWidth(OscPw[1] * ModulationFactor(TModulation::OSC2_PW, *v));
            v->Oscillators[2]->SetPulseWidth(OscPw[2] * ModulationFactor(TModulation::OSC3_PW, *v));
            v->Oscillators[0]->SetSync(OscSync[0]);
            v->Oscillators[1]->SetSync(OscSync[1]);
            v->Oscillators[2]->SetSync(OscSync[2]);
            v->OscPan[0].SetPan(OscLevel[0] * ModulationFactor(TModulation::OSC1_LEVEL, *v),
                    ModulationValue(TModulation::OSC1_PAN, *v));
            v->OscPan[1].SetPan(OscLevel[1] * ModulationFactor(TModulation::OSC2_LEVEL, *v),
                    ModulationValue(TModulation::OSC2_PAN, *v));
            v->OscPan[2].SetPan(OscLevel[2] * ModulationFactor(TModulation::OSC3_LEVEL, *v),
                    ModulationValue(TModulation::OSC3_PAN, *v));
            v->Filters[0].SetCutoff(FilterCutoff[0] * ModulationFactor(TModulation::F1_CUTOFF, *v));
            v->Filters[1].SetCutoff(FilterCutoff[1] * ModulationFactor(TModulation::F2_CUTOFF, *v));
            v->Filters[0].SetResonance(FilterResonance[0] * ModulationFactor(TModulation::F1_RESONANCE, *v));
            v->Filters[1].SetResonance(FilterResonance[1] * ModulationFactor(TModulation::F1_RESONANCE, *v));
            v->FiltPan[0].SetPan(1.0, ModulationValue(TModulation::F1_PAN, *v));
            v->FiltPan[1].SetPan(1.0, ModulationValue(TModulation::F2_PAN, *v));
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
        TSampleBuffer into1(into[0]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into2(into[1]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into3(into[2]->Slice(sampleno, sampleno + subframelen));
        TSampleBuffer into4(into[3]->Slice(sampleno, sampleno + subframelen));

        // Buffer for keeping track of hardsync between oscillators
        TSample hardsyncdata[subframelen];
        TSampleBuffer hardsync(hardsyncdata, subframelen);

        TimerSamples.Start();
        // Generate audio samples for left and right channel
        for (TVoice& voice: Voices) {
            if (voice.State == TVoice::FINISHED) {
                continue;
            }

            bufFilter[0].Clear();
            bufFilter[1].Clear();
            bufChannel[0].Clear();
            bufChannel[1].Clear();

            // Render oscillators and pan them to two buffers for the filters.
            // Hard sync is propagated from one oscillator to the next.
            hardsync.Clear();
            for (unsigned i = 0; i < TGlobal::Oscillators; i++) {
                bufOscillator.Clear();
                voice.Oscillators[i]->Process(bufIn[0], bufOscillator, hardsync, hardsync);
                voice.OscPan[i].Process(bufOscillator, bufFilter[0], bufFilter[1]);
                into1.AddSamples(bufFilter[0]); // All oscillators going to filter 1
                into2.AddSamples(bufFilter[1]); // All oscillators going to filter 2
            }

            // Add distortion to the signal going to the filters in place,
            // render filters in place, pan filters to left and right channels
            for (unsigned i = 0; i < TGlobal::Filters; i++) {
        	voice.WaveShaper[i].Process(bufFilter[i], bufFilter[i]);
        	voice.Filters[i].Process(bufFilter[i], bufFilter[i]);
        	voice.FiltPan[i].Process(bufFilter[i], bufChannel[0], bufChannel[1]);
            }

            into3.AddSamples(bufFilter[0]); // All oscillators filter 1 output
            into4.AddSamples(bufFilter[1]); // All oscillators filter 2 output

            // Output volume adjusted data
            // FIXME: shouldn't the amp envelope be applied before filter, distortion,
            // etc? That would make for a more dynamic sound, and it would be saner.
            for (jack_nframes_t j = 0; j < subframelen; j++) {
        	voice.AmpEg.Step(1);
                bufProgramSlice[0][j] += bufChannel[0][j] * voice.Velocity / 128.0 * voice.AmpEg.GetValue();
                bufProgramSlice[1][j] += bufChannel[1][j] * voice.Velocity / 128.0 * voice.AmpEg.GetValue();
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
    for (unsigned i = 0; i < TGlobal::Effects; i++) {
        if (Effects[i]) Effects[i]->Process(programBuffers, programBuffers);
    }

    // Add samples to output buffers
    out[0]->AddSamples(bufProgram[0]);
    out[1]->AddSamples(bufProgram[1]);

    if (!sounding) {
        sounding = out[0]->PeakAmplitude() > TGlobal::OscAmplitude * 0.01 ||
                out[1]->PeakAmplitude() > TGlobal::OscAmplitude * 0.01;
    }

    TimerProcess.Stop();

    return sounding;
}

inline float TProgram::ModulationFactor(TModulation::TDestination d, const TVoice& voice)
{
    float mod = ModulationValue(d, voice);
    return mod != 0 ? fpow2(mod) : 1;
    //return mod != 0 ? exp2f(mod) : 1;
}

inline float TProgram::ModulationValue(TModulation::TDestination d, const TVoice& voice)
{
    float mod = 0;
    for (const TModulation& m: Modulations) {
        if (m.Destination == d) {
            switch (m.Source) {
            case TModulation::CONSTANT: mod += m.Amount; break;
            case TModulation::KEY: mod += m.Amount * semitones((int)voice.Note - (int)TGlobal::MidiNoteE3); break;
            case TModulation::PITCHBEND: mod += m.Amount * PitchBend; break;
            case TModulation::LFO1: mod += m.Amount * voice.Lfos[0].GetValue(); break;
            case TModulation::LFO2: mod += m.Amount * voice.Lfos[1].GetValue(); break;
            case TModulation::EG1: mod += m.Amount * voice.FiltEg.GetValue(); break;
            case TModulation::VELOCITY: mod += m.Amount * voice.Velocity/128.0; break;
            case TModulation::EG1_TIMES_VELO: mod += m.Amount * voice.FiltEg.GetValue() * voice.Velocity/128.0f; break;
            case TModulation::MODWHEEL: mod += m.Amount * ModWheel; break;
            case TModulation::BREATH: mod += m.Amount * Breath; break;
            }
        }
    }
    return mod;
}

TVoice* TProgram::AllocateVoice()
{
    size_t count = 0;
    TVoice* voice = NULL;

    for (size_t i = 0; i < Voices.size(); i++) {
	if (voice == NULL && Voices[i].State == TVoice::TState::FINISHED) {
	    voice = &Voices[i];
	}
	count += Voices[i].State != TVoice::TState::FINISHED ? 1 : 0;
    }

    if (count > TGlobal::SoftVoiceLimit) {
        fprintf(stderr, "Now %zu voices\n", count);

        // Steal an old voice
        // FIXME: Implement!
        /*
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
        */
    }

    return voice;
}

void TProgram::NoteOn(TUnsigned7 note, TUnsigned7 velocity)
{
    TVoice* voice = AllocateVoice();
    if (!voice) {
	fprintf(stderr, "Too many voices\n");
	return;
    }

    float hz = NOTE2HZ(note);
    voice->Reset(note, hz, velocity);

    TNoteData noteData({ note, velocity });

    for (unsigned i = 0; i < TGlobal::Oscillators; i++) {
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
#ifdef HAVE_LIBGIG
        case OSC_GIG: {
            TGigOscillator *o = new TGigOscillator(noteData);
            o->SetInstrument(GigInstrument);
            voice->Oscillators[i].reset(o);
            break;
        }
#endif
        case OSC_OFF:
        default:
            voice->Oscillators[i].reset(new TBaseOscillator(noteData));
        }
    }

    voice->AmpEg.Reset(Envelope[0]);
    voice->FiltEg.Reset(Envelope[1]);
}

void TProgram::NoteOff(TUnsigned7 note, TUnsigned7 velocity)
{
    for (TVoice& voice: Voices) {
        if (voice.State != TVoice::TState::FINISHED && voice.Note == note) {
            if (voice.AmpEg.GetState() < TEnvelope::RELEASE) {
                if (voice.Hold) {
                    // Held by sostenuto pedal; don't release
                    voice.State = TVoice::TState::RELEASED;
                }
                else if (Sustain == 0) {
                    voice.State = TVoice::TState::RELEASED;
                    voice.AmpEg.Release(velocity);
                    voice.FiltEg.Release(velocity);
                }
                else {
                    voice.State = TVoice::TState::SUSTAINED;
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
            for (TVoice& voice: Voices) {
                if (voice.State == TVoice::TState::SUSTAINED) {
                    voice.State = TVoice::TState::RELEASED;
                    voice.AmpEg.Release(0x40);
                    voice.FiltEg.Release(0x40);
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
            for (TVoice& voice: Voices) {
                if (voice.State != TVoice::TState::FINISHED && voice.Hold) {
                    voice.Hold = false;
                    if (voice.State == TVoice::TState::RELEASED && Sustain == 0) {
                        voice.AmpEg.Release(0x40);
                        voice.FiltEg.Release(0x40);
                    }
                    else if (Sustain) {
                        voice.State = TVoice::TState::SUSTAINED;
                    }
                }
            }
        }
        else {
            // Latch all currently held notes
            for (TVoice& voice: Voices) {
                if (voice.State == TVoice::TState::PLAYING) {
                    voice.Hold = true;
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
    else if (param == PARAM_FX_TYPE) {
        if (value == 0) { // Delay
            Effects[unit].reset(new TDelayFx());
        }
        else if (value == 1) { // Reverb
            Effects[unit].reset(new TReverbFx);
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
