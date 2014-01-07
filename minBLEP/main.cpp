#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>
#include <string>
#include <algorithm>
#include <cmath>

float* GenerateMinBLEP(int zeroCrossings, int overSampling);

float* GenerateHardstep(int length, int overSampling)
{
    int len = length * overSampling;
    float* step = new float[len];
    std::fill(&step[0], &step[len], 1.0f);
    return step;
}

float* GenerateFromSines(int length, int overSampling)
{
    int len = length * overSampling;
    float* step = new float[len];
    std::fill(&step[0], &step[len], 0.0f);

    for (int sample = 0; sample < len; sample++) {
        float phase = float(sample) * 2.0f * M_PI / len;
        for (int component = 1; component < length; component++) {
            step[sample] += sinf(component * phase / 2.0f - M_PI / 2) / component;
        }
    }

    // Normalize
    float offset = -step[0];
    for (int sample = 0; sample < len; sample++) {
        step[sample] += offset;
    }
    float scale = 1.0f / step[len - 1];
    for (int sample = 0; sample < len; sample++) {
        step[sample] *= scale;
    }

    return step;
}

int main(int argc, char* argv[])
{
    int zeroCrossings = 0;
    int overSampling = 0;
    bool header = false;
    const char* type = "minblep";

    struct option longopts[] = { { "zerocrossings", 1, 0, 'z' }, { "oversampling", 1, 0, 'o' }, {
            "header", 0, 0, 'h' }, { "type", 1, 0, 't' }, { 0, 0, 0, 0 } };
    int opt;
    while ((opt = getopt_long(argc, argv, "h", longopts, 0)) != -1) {
        switch (opt) {
        case 'z':
            zeroCrossings = ::atoi(optarg);
            break;
        case 'o':
            overSampling = ::atoi(optarg);
            break;
        case 'h':
            header = true;
            break;
        case 't':
            type = optarg;
            break;
        default:
            fprintf(stderr, "See source code for options\n");
            return 1;
        }
    }

    float* step = 0;
    int n = 0;

    if (std::string("minblep") == type) {
        // Generate a minBLEP
        step = GenerateMinBLEP(zeroCrossings, overSampling);
        n = zeroCrossings * 2 * overSampling;
    }
    else if (std::string("hard") == type) {
        // Generate a simple step that will cause aliasing
        int length = zeroCrossings * 2;
        step = GenerateHardstep(length, overSampling);
        n = length * overSampling;
    }
    else if (std::string("sines") == type) {
        // Compose a step from sine components
        int length = zeroCrossings * 2;
        step = GenerateFromSines(length, overSampling);
        n = length * overSampling;
    }
    else {
        fprintf(stderr, "Requested step type not known\n");
        return 1;
    }

    float integral = 0;
    for (int i = 0; i < n; i++) {
        integral += step[i] / overSampling;
    }

    if (!header) {
        for (int i = 0; i < n; i++) {
            printf("%.3f\n", step[i]);
        }
    }
    else {
        printf("#pragma once\n"
                "/*\n"
                " * This is a minBLEP table generated from a sinc with\n"
                " * %d zero crossings and is meant for use with\n"
                " * %d times oversampling.\n"
                " */\n\n", zeroCrossings, overSampling);

        printf("namespace minblep {\n"
                "static const int zeroCrossings = %d;\n"
                "static const int  overSampling = %d;\n"
                "static const int   tablelength = %d;\n"
                "static const int        length = %d;\n"
                "static const float    integral = %f;\n\n", zeroCrossings, overSampling, n,
                n / overSampling, integral);

        printf("static const float table[tablelength] = {\n");
        for (int i = 0; i < n; i += 4) {
            printf("    %.13f, %.13f, %.13f, %.13f,\n", step[i], step[i + 1], step[i + 2],
                    step[i + 3]);
        }
        printf("};\n"
                "}\n");
    }
    delete step;
}
