#include "TGlobal.h"
#include "TDelayFx.h"
#include "TInputOscillator.h"
#include "TMinBlepPulseOscillator.h"
#include "TMinBlepSawOscillator.h"
#include "TProgram.h"
#include "TReverbFx.h"
#include "TSampleOscillator.h"
#include "TSineOscillator.h"
#include "TWavetableOscillator.h"
#include <cassert>
#include <map>

TSampleLoader TProgram::SampleLoader("sample.ogg");

TProgram::TProgram()
: Voices(),
  ParameterChanged(0),
  TimerProcess("Process"),
  TimerUpdates("Updates"),
  TimerSamples("Samples"),
  PitchBend(0),
  ModWheel(0),
  Breath(0),
  Sustain(0),
  Modulations()
{
    fprintf(stderr, "Create program\n");
}

TProgram::~TProgram()
{
    fprintf(stderr, "Delete program\n");
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

void TProgram::SetPatch(int p)
{
    switch (p % 5) {
    case 0: Patch0(); break;
    case 1: Patch1(); break;
    case 2: Patch2(); break;
    case 3: Patch3(); break;
    case 4: Patch4(); break;
    }
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

    Effects[0].reset(new TDelayFx());
    Effects[0]->SetMix(0.5);
}

/*
 * Samples
 */
void TProgram::Patch2()
{
    Modulations.clear();
    SetupConstantModulations();

    OscType[0] = OscType[1] = OscType[2] = OSC_SAMPLE;

    OscPw[0] = 0;
    OscPw[1] = 0;
    OscPw[2] = 0;

    OscSync[0] = false;
    OscSync[1] = false;
    OscSync[2] = false;

    OscLevel[0] = 1.0;
    OscLevel[1] = 0.6;
    OscLevel[2] = 1.0/8;

    WaveShaper[0] = 0.0;
    WaveShaper[1] = 0.0;

    Modulations[C_OSC1_DETUNE].Amount = cents(10);
    Modulations[C_OSC2_DETUNE].Amount = cents(-10);
    Modulations[C_OSC1_OCTAVE].Amount = octaves(1);
    Modulations[C_OSC2_OCTAVE].Amount = octaves(0);
    Modulations[C_OSC3_OCTAVE].Amount = octaves(2);
    Modulations[C_OSC1_PAN].Amount = 0; // Modulated by LFO
    Modulations[C_OSC2_PAN].Amount = 0; // Modulated by LFO
    Modulations[C_OSC3_PAN].Amount = 0;
    Modulations[C_F1_PAN].Amount = 1;
    Modulations[C_F2_PAN].Amount = -1;

    Modulations.push_back( { TModulation::KEY, -2, TModulation::OSC2_LEVEL });

    Envelope[0] = {Attack: 3, Decay: 400, Sustain: 0.3, Release: 150};
    Envelope[1] = {Attack: 300, Decay: 0, Sustain: 1.0, Release: 300};

    FilterCutoff[0] = 6 * TGlobal::HzE3;
    FilterCutoff[1] = 5 * TGlobal::HzE3;
    FilterResonance[0] = 0.15;
    FilterResonance[1] = 0.20;
    Modulations.push_back( { TModulation::KEY, semitones(6), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::KEY, semitones(6), TModulation::F2_CUTOFF });
    Modulations.push_back( { TModulation::EG1, -octaves(2), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::EG1, -octaves(2), TModulation::F2_CUTOFF });

    Modulations.push_back( { TModulation::MODWHEEL, -2.0, TModulation::OSC1_LEVEL });
    Modulations.push_back( { TModulation::MODWHEEL, -2.0, TModulation::OSC2_LEVEL });
    Modulations.push_back( { TModulation::MODWHEEL, 4.0, TModulation::OSC3_LEVEL });

    Modulations.push_back( { TModulation::BREATH, octaves(-2), TModulation::OSC1_FREQ });
    Modulations.push_back( { TModulation::BREATH, octaves(-2), TModulation::OSC2_FREQ });
    Modulations.push_back( { TModulation::BREATH, octaves(-2), TModulation::OSC3_FREQ });

    LfoFrequency[0] = 1.3;
    Modulations.push_back( { TModulation::LFO1, cents(10), TModulation::OSC1_FREQ });
    Modulations.push_back( { TModulation::LFO1, cents(-10), TModulation::OSC2_FREQ });

    LfoFrequency[1] = 0.5;
    Modulations.push_back( { TModulation::LFO2, 0.4, TModulation::OSC1_PAN });
    Modulations.push_back( { TModulation::LFO2, -0.4, TModulation::OSC2_PAN });

    Effects[0].reset(new TReverbFx);
    Effects[0]->SetMix(0.25);
}

/*
 * An attempt to duplicate Blofeld's B003 microQ Plus
 */
void TProgram::Patch3()
{
    Modulations.clear();
    SetupConstantModulations();

    OscType[0] = OSC_SAW;
    OscType[1] = OSC_SAW;
    OscType[2] = OSC_SAW;

    OscSync[0] = false;
    OscSync[1] = false;
    OscSync[2] = false;

    Modulations[C_OSC1_OCTAVE].Amount = octaves(-1); // 16'
    Modulations[C_OSC1_PAN].Amount = -1;
    Modulations[C_OSC1_DETUNE].Amount = cents(+8 / 64.0 * 100.0);
    Modulations[C_OSC2_OCTAVE].Amount = octaves(-1); // 16'
    Modulations[C_OSC2_PAN].Amount = 1;
    Modulations[C_OSC2_DETUNE].Amount = cents(-8 / 64.0 * 100.0);
    Modulations[C_OSC3_OCTAVE].Amount = octaves(-1); // 16'
    Modulations[C_OSC3_PAN].Amount = 0;
    Modulations[C_OSC3_DETUNE].Amount = cents(0);

    // Bass boost
    Modulations.push_back( { TModulation::KEY, -0.2, TModulation::OSC1_LEVEL });
    Modulations.push_back( { TModulation::KEY, -0.2, TModulation::OSC2_LEVEL });
    Modulations.push_back( { TModulation::KEY, -0.2, TModulation::OSC3_LEVEL });

    OscLevel[0] = 1.0f;
    OscLevel[1] = 1.0f;
    OscLevel[2] = 1.0f;

    Modulations.push_back( { TModulation::BREATH, octaves(-1), TModulation::OSC1_FREQ });
    Modulations.push_back( { TModulation::BREATH, octaves(-1), TModulation::OSC2_FREQ });
    Modulations.push_back( { TModulation::BREATH, octaves(-1), TModulation::OSC3_FREQ });

    Modulations[C_F1_PAN].Amount = -1;
    Modulations[C_F2_PAN].Amount = 1;

    // F1: PPG LP, cutoff 25 (49Hz), resonance 43, env velocity: 28
    // F2: PPG LP, cutoff 24 (46Hz), resonance 41, env velocity: 28
    FilterCutoff[0] = 49;
    FilterCutoff[1] = 46;
    FilterResonance[0] = 0.3;
    FilterResonance[1] = 0.3;
    Modulations.push_back( { TModulation::EG1_TIMES_VELO, octaves(8), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::EG1_TIMES_VELO, octaves(8), TModulation::F2_CUTOFF });

    Modulations.push_back( { TModulation::MODWHEEL, octaves(6), TModulation::F1_CUTOFF });
    Modulations.push_back( { TModulation::MODWHEEL, octaves(6), TModulation::F2_CUTOFF });

    WaveShaper[0] = 0.0;
    WaveShaper[1] = 0.0;
    LfoFrequency[0] = 1;
    LfoFrequency[1] = 1;
    Envelope[0] = {Attack: 10, Decay: 0, Sustain: 1.0, Release: 30};
    Envelope[1] = {Attack: 100, Decay: 3000, Sustain: 31/127.0f, Release: 40};

    TDelayFx* fx = new TDelayFx();
    fx->SetDelay(250);
    fx->SetFeedback(0.5);
    Effects[0].reset(fx);
    Effects[0]->SetMix(0.5);
}

/*
 * Hardsync test patch
 */
void TProgram::Patch4()
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

    // Enable hard-sync
    OscSync[1] = true;
    Modulations[C_OSC2_DETUNE].Amount = octaves(1);
    Modulations.push_back( { TModulation::MODWHEEL, octaves(1), TModulation::OSC2_FREQ });
    Modulations.push_back( { TModulation::BREATH, octaves(-1), TModulation::OSC2_FREQ });

    WaveShaper[0] = 0.0;
    WaveShaper[1] = 0.0;
    LfoFrequency[0] = 1;
    LfoFrequency[1] = 1;
    FilterCutoff[0] = 10000;
    FilterCutoff[1] = 10000;
    FilterResonance[0] = 0.15;
    FilterResonance[1] = 0.15;
    Envelope[0] = {Attack: 20, Decay: 0, Sustain: 1.0, Release: 20};
    Envelope[1] = {Attack: 20, Decay: 0, Sustain: 1.0, Release: 20};

    Effects[0].reset();
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
            v->Oscillators[0]->SetFrequency(
                    key_hz * ModulationFactor(TModulation::OSC1_FREQ, *voice));
            v->Oscillators[1]->SetFrequency(
                    key_hz * ModulationFactor(TModulation::OSC2_FREQ, *voice));
            v->Oscillators[2]->SetFrequency(
                    key_hz * ModulationFactor(TModulation::OSC3_FREQ, *voice));
            v->Oscillators[0]->SetPulseWidth(
                    OscPw[0] * ModulationFactor(TModulation::OSC1_PW, *voice));
            v->Oscillators[1]->SetPulseWidth(
                    OscPw[1] * ModulationFactor(TModulation::OSC2_PW, *voice));
            v->Oscillators[2]->SetPulseWidth(
                    OscPw[2] * ModulationFactor(TModulation::OSC3_PW, *voice));
            v->Oscillators[0]->SetSync(OscSync[0]);
            v->Oscillators[1]->SetSync(OscSync[1]);
            v->Oscillators[2]->SetSync(OscSync[2]);
            v->OscPan[0].SetPan(OscLevel[0] * ModulationFactor(TModulation::OSC1_LEVEL, *voice),
                    ModulationValue(TModulation::OSC1_PAN, *voice));
            v->OscPan[1].SetPan(OscLevel[1] * ModulationFactor(TModulation::OSC2_LEVEL, *voice),
                    ModulationValue(TModulation::OSC2_PAN, *voice));
            v->OscPan[2].SetPan(OscLevel[2] * ModulationFactor(TModulation::OSC3_LEVEL, *voice),
                    ModulationValue(TModulation::OSC3_PAN, *voice));
            v->Filters[0].SetCutoff(
                    FilterCutoff[0] * ModulationFactor(TModulation::F1_CUTOFF, *voice));
            v->Filters[1].SetCutoff(
                    FilterCutoff[1] * ModulationFactor(TModulation::F2_CUTOFF, *voice));
            v->Filters[0].SetResonance(FilterResonance[0]);
            v->Filters[1].SetResonance(FilterResonance[1]);
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

static inline float fpow2(const float y)
{
    // musicdsp.org, Johannes M.-R.
    union
    {
        float f;
        int i;
    } c;

    int integer = (int) y;
    if (y < 0) integer = integer - 1;

    float frac = y - (float) integer;

    c.i = (integer + 127) << 23;
    c.f *= 0.33977f * frac * frac + (1.0f - 0.33977f) * frac + 1.0f;

    return c.f;
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
    float hz = exp2f((note - TGlobal::MidiNoteA4) / 12.0) * TGlobal::HzA4;
    TVoice* voice = new TVoice(hz, velocity);

    for (int i = 0; i < TGlobal::Oscillators; i++) {
        switch (OscType[i]) {
        case OSC_SINE:
            voice->Oscillators[i].reset(new TSineOscillator());
            break;
        case OSC_SQUARE:
            voice->Oscillators[i].reset(new TMinBlepPulseOscillator());
            break;
        case OSC_SAW:
            voice->Oscillators[i].reset(new TMinBlepSawOscillator());
            break;
        case OSC_WT:
            voice->Oscillators[i].reset(new TWavetableOscillator());
            break;
        case OSC_SAMPLE: {
            TSampleOscillator *o = new TSampleOscillator();
            o->SetSample(SampleLoader.GetBuffer(), TGlobal::HzA4 * double(TGlobal::SampleRate) / SampleLoader.GetSampleRate());
            o->SetLoopPoints(SampleLoader.GetBuffer()->GetCount() * 0.6,
                    SampleLoader.GetBuffer()->GetCount() * 0.8);
            voice->Oscillators[i].reset(o);
            break;
        }
        case OSC_INPUT:
            voice->Oscillators[i].reset(new TInputOscillator());
            break;
        case OSC_OFF:
        default:
            voice->Oscillators[i].reset(new TBaseOscillator());
        }
    }

    voice->AmpEg.Set(Envelope[0]);
    voice->FiltEg.Set(Envelope[1]);
    Voices.push_back( { note, std::unique_ptr<TVoice>(voice) });

    if (Voices.size() > TGlobal::SoftVoiceLimit) {
        fprintf(stderr, "Now %zu voices\n", Voices.size());
        TSoundingVoice& steal = Voices.front();
        if (steal.voice->AmpEg.GetState() < TEnvelope::RELEASE) {
            steal.voice->State = TVoice::TState::RELEASED;
            steal.voice->AmpEg.Release(velocity);
            steal.voice->FiltEg.Release(velocity);
        }
    }
}

void TProgram::NoteOff(TUnsigned7 note, TUnsigned7 velocity)
{
    for (auto voice = Voices.begin(); voice != Voices.end(); voice++) {
        if (voice->note == note) {
            if (voice->voice->AmpEg.GetState() < TEnvelope::RELEASE) {
                if (Sustain == 0) {
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
    case MIDI_CC_SOSTENUTO: // Sostenuto
        // FIXME: Implement
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
