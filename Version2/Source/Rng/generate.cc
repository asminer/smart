
// $Id$

/* Test of rng.  Dumps to an ascii stream for use with diehard. */

#include "rng.h"

#include <stdio.h>

const int MAX_STREAMS = 32;

Rng stream[MAX_STREAMS];

void InitStreams(int strms, unsigned long seed)
{
  int i;
  stream[0].seed32(seed);
  for (i=1; i<strms; i++) {
    stream[i].JumpStream(stream[i-1]);
  }
}

void TextGenerate(int strms, int samples)
{
  int s,n;
  for (n=0; n<samples; n++) {
    for (s=0; s<strms; s++) {
      if (s) printf("  ");
      printf("%lf", stream[s].drand());
    }
    printf("\n");
  }
}

void PackBit(FILE* out, bool value)
{
  static unsigned char byte = 0;
  static int bit = 8;
  const unsigned char mask[8] = {0x80, 0x40, 0x20, 0x10, 
  				 0x08, 0x04, 0x02, 0x01};

  bit--;
  if (value) byte += mask[bit];
  if (0==bit) {
    fputc(byte, out);
    bit = 8;
    byte = 0;
  }
}

void BinGenerate(int strms, int samples)
{
const unsigned int mask[32] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
			  0x08000000, 0x04000000, 0x02000000, 0x01000000,

			  0x00800000, 0x00400000, 0x00200000, 0x00100000,
			  0x00080000, 0x00040000, 0x00020000, 0x00010000,

			  0x00008000, 0x00004000, 0x00002000, 0x00001000,
			  0x00000800, 0x00000400, 0x00000200, 0x00000100,

			  0x00000080, 0x00000040, 0x00000020, 0x00000010,
			  0x00000008, 0x00000004, 0x00000002, 0x00000001 };


  FILE* out[MAX_STREAMS];
  int s;
  char buffer[255];
  for (s=0; s<strms; s++) {
    buffer[0] = 0;
    sprintf(buffer, "bits%d.bin", s);
    printf("Opening %s\n", buffer);
    out[s] = fopen(buffer, "w");
  }

  printf("Opening mixed.bin\n");
  FILE* mixed = fopen("mixed.bin", "w");

  int n;
  unsigned long genseq[MAX_STREAMS]; 
  for (n=0; n<samples; n++) {
    // Generate and write from each stream
    for (s=0; s<strms; s++) {
      genseq[s] = stream[s].lrand();
      fwrite(genseq+s, sizeof(unsigned long), 1, out[s]);
    }
    // Interleave bits for the mixed stream
    for (int bit=0; bit<32; bit++) {
      for (s=0; s<strms; s++) {
	bool b = (genseq[s] & mask[bit]);
	PackBit(mixed, b);
      } // s
    } // bit
  } // n

  fclose(mixed);
  for (s=0; s<strms; s++) fclose(out[s]);
  
}

int main(int argc, char** argv)
{
  if (argc<3) {
    fprintf(stderr, "\n\tUsage: %s t|b <#streams> <#samples>\n\n", argv[0]);
    fprintf(stderr, "\t #streams is the number of parallel streams to use\n");
    fprintf(stderr, "\t #samples is the number of samples (per stream) to generate\n");
    fprintf(stderr, "\t t  means output will be text mode, #streams per line\n");
    fprintf(stderr, "\t b  means output will be binary files\n");
    fprintf(stderr, "\t\t  bits0.bin ... bits(n-1).bin mixed.bin\n\n");
    return 0;
  }

  bool binary = false;
  switch (argv[1][0]) {
    case 'b' : 
    	binary = true;
	break;
    case 't' :
    	binary = false;
	break;
    default:
    	fprintf(stderr, "Expecting t|b for first argument\n");
	return 0;
  }

  int streams = atoi(argv[2]);
  if ((streams < 1) ||(streams > MAX_STREAMS)) {
    fprintf(stderr, "Invalid number of streams %d\n", streams);
    return 0;
  }

  int samples = atoi(argv[3]);
  if (samples < 1) {
    fprintf(stderr, "Invalid number of samples %d\n", samples);
    return 0;
  }

  InitStreams(streams, 7309259);

  if (binary)
    BinGenerate(streams, samples);
  else
    TextGenerate(streams, samples);

  return 0;
}
