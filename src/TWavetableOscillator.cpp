#include "TWavetableOscillator.h"
#include "util.h"

TWavetable::TWavetable()
{
}

TWavetable generateSquareTable(unsigned wavelength)
{
    TTimer timer("Generate square table");
    timer.Start();

    TWavetable table;
    table.BaseNote = 8;
    table.Tables = 37;
    table.NotesPerTable = 3;
    table.Length = wavelength;

    table.Data = new TSample[table.Tables * table.Length];

    unsigned nyquist = 44100 / 2;

    for (unsigned t = 0; t < table.Tables; t++) {
        const unsigned f = NOTE2HZ(table.BaseNote + (t + 1) * table.NotesPerTable);

        unsigned harmonics = 1;
        while (f * (harmonics + 2) < nyquist) {
            harmonics += 2;
        }

        //printf("Table %d: up to %u Hz, %u harmonics\n", t, f, harmonics);

        for (unsigned i = 0; i < table.Length; i++) {
            double phase = 2.0 * M_PI * double(i) / table.Length;
            double v = 0.0;

            for (int h = harmonics; h > 0; h -= 2) {
                v += sinf(phase * h) * 1.0 / h;
            }

            table.GetTable(t)[i] = v * TGlobal::OscAmplitude;
        }
    }

    timer.Stop();
    return table;
}

TWavetable generateSineTable(unsigned wavelength)
{
    TTimer timer("Generate sine table");
    timer.Start();

    TWavetable table;
    table.BaseNote = 1;
    table.Tables = 1;
    table.NotesPerTable = 12;
    table.Length = wavelength;

    table.Data = new TSample[table.Tables * table.Length];

    for (unsigned i = 0; i < wavelength; i++) {
        double phase = 2.0 * M_PI * double(i) / wavelength;
        table.GetTable(0)[i] = sin(phase) * TGlobal::OscAmplitude;
    }

    timer.Stop();
    return table;
}

const TWavetable TWavetableOscillator::Table(generateSquareTable(Wavelength));

TWavetableOscillator::TWavetableOscillator(const TNoteData& noteData)
        : TBaseOscillator(noteData)
{
    int n = (noteData.Note - Table.BaseNote) / Table.NotesPerTable;
    n = clamp(n, 0, (int)Table.Tables - 1);
    Subtable = Table.GetTable(n);
}
