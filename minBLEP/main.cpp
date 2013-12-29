#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

float *GenerateMinBLEP(int zeroCrossings, int overSampling);

int main(int argc, char* argv[])
{
  int zeroCrossings = 0;
  int overSampling = 0;
  bool header = false;
  
  struct option longopts[] = {{ "zerocrossings", 1, 0, 'z' },
                              { "oversampling", 1, 0, 'o'},
                              { "header", 0, 0, 'h'},
                              { 0, 0, 0, 0}};
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
    default:
      fprintf(stderr, "See source code for options\n");
      return 1;
    }
  }


  float *b = GenerateMinBLEP(zeroCrossings,
			     overSampling);

  int n = (zeroCrossings * 2 * overSampling) + 1;

  if (!header) {
    for (int i=0; i<n; i++) {
      printf("%.3f\n", b[i]);
    }
  } else {
    printf("#pragma once\n"
	   "/*\n"
	   " * This is a minBLEP table generated from a sinc with\n"
	   " * %d zero crossings and is meant for use with\n"
	   " * %d times oversampling.\n"
	   " */\n\n",
	   zeroCrossings, overSampling);

    printf("namespace minblep {\n"
	   "static const int zeroCrossings = %d;\n"
	   "static const int  overSampling = %d;\n"
	   "static const int   tablelength = %d;\n\n"
	   "static const int        length = %d;\n\n",
	   zeroCrossings, overSampling, n-1, (n-1)/overSampling);
    
    printf("static const float table[tablelength] = {\n");
    for (int i=0; i<n-1; i+=4) {
      printf("    %.13f, %.13f, %.13f, %.13f,\n", b[i], b[i+1], b[i+2], b[i+3]);
    }
    printf("};\n"
	   "}\n");
  }
  delete b;
}
