#include "TGigInstrument.h"

TGigInstrument::TGigInstrument(std::string filename)
    : RiffFile(), GigFile(), Instrument(NULL)
{
    try {
        RiffFile.reset(new RIFF::File(filename));
        GigFile.reset(new gig::File(RiffFile.get()));
        Instrument = GigFile->GetFirstInstrument();
    }
    catch (RIFF::Exception &e) {
        e.PrintMessage();
    }

    if (Instrument) {
        fprintf(stderr, "Loaded instrument from %s:\n", filename.c_str());
        fprintf(stderr, "# %d:%d:%d %s\n", Instrument->MIDIBankCoarse, Instrument->MIDIBankFine,
                Instrument->MIDIProgram, Instrument->pInfo->Name.c_str());
        fprintf(stderr, "# att=%d dB, finetune=%d, pbrange=%d, releasemode=%d, dim range=%d..%d\n",
                Instrument->Attenuation, Instrument->FineTune, Instrument->PitchbendRange,
                Instrument->PianoReleaseMode,
                Instrument->DimensionKeyRange.low, Instrument->DimensionKeyRange.high);
    }
    else {
        fprintf(stderr, "Failed to load instrument\n");
    }
}
