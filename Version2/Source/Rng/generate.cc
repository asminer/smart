
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
    stream[i].JumpStream(&stream[i-1]);
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

void PackBit(bool value)
{
  static unsigned char byte = 0;
  static int bit = 8;
  const unsigned char mask[8] = {0x80, 0x40, 0x20, 0x10, 
  				 0x08, 0x04, 0x02, 0x01};

  bit--;
  if (value) byte += mask[bit];
  if (0==bit) {
    fputc(byte, stdout);
    bit = 8;
    byte = 0;
  }
}

void BitGenerate(int strms, int samples)
{
const unsigned int mask[32] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
			  0x08000000, 0x04000000, 0x02000000, 0x01000000,

			  0x00800000, 0x00400000, 0x00200000, 0x00100000,
			  0x00080000, 0x00040000, 0x00020000, 0x00010000,

			  0x00008000, 0x00004000, 0x00002000, 0x00001000,
			  0x00000800, 0x00000400, 0x00000200, 0x00000100,

			  0x00000080, 0x00000040, 0x00000020, 0x00000010,
			  0x00000008, 0x00000004, 0x00000002, 0x00000001 };


  int s;
  int n;
  unsigned long genseq[MAX_STREAMS]; 
  for (n=0; n<samples; n++) {
    // Generate from each stream
    for (s=0; s<strms; s++) {
      genseq[s] = stream[s].lrand();
    }
    // Interleave bits for the mixed stream
    for (int bit=0; bit<32; bit++) {
      for (s=0; s<strms; s++) {
	bool b = (genseq[s] & mask[bit]);
	PackBit(b);
      } // s
    } // bit
  } // n
}

void ByteGenerate(int strms, int samples)
{
  int s,n;
  unsigned long genseq[MAX_STREAMS];
  for (n=0; n<samples; n++) {
    // Generate from each stream
    for (s=0; s<strms; s++) {
      genseq[s] = stream[s].lrand();
    }
    // Interleave bytes (nice hack)
    // write first byte from each sample
    char* foo = (char*) genseq;
    for (s=0; s<strms; s++) {
      fputc(foo[0], stdout);
      foo += 4;  
    }
    // write the second byte from each sample
    foo = (char*) genseq;
    for (s=0; s<strms; s++) {
      fputc(foo[1], stdout);
      foo += 4;  
    }
    // write the third byte from each sample
    foo = (char*) genseq;
    for (s=0; s<strms; s++) {
      fputc(foo[2], stdout);
      foo += 4;  
    }
    // write the fourth byte from each sample
    foo = (char*) genseq;
    for (s=0; s<strms; s++) {
      fputc(foo[3], stdout);
      foo += 4;  
    }
  } // n
}

void WordGenerate(int strms, int samples)
{
  int s,n;
  for (n=0; n<samples; n++) {
    // Generate and write from each stream
    for (s=0; s<strms; s++) {
      unsigned long sample = stream[s].lrand();
      char* foo = (char*)&sample;
      fputc(foo[0], stdout);
      fputc(foo[1], stdout);
      fputc(foo[2], stdout);
      fputc(foo[3], stdout);
      // by hand instead of write to fix the order.
    }
  }
}

int main(int argc, char** argv)
{
  if (argc<3) {
    fprintf(stderr, "\n\tUsage: %s t|b|y|w <#streams> <#samples>\n\n", argv[0]);
    fprintf(stderr, "\t #streams is the number of parallel streams to use\n");
    fprintf(stderr, "\t #samples is the number of samples (per stream) to generate\n");
    fprintf(stderr, "\t t  means output will be text mode, #streams per line\n");
    fprintf(stderr, "\t b  means output stream will be interleaved bits\n");
    fprintf(stderr, "\t y  means output stream will be interleaved bytes\n");
    fprintf(stderr, "\t w  means output stream will be interleaved words\n\n");
    return 0;
  }

  switch (argv[1][0]) {
    case 't' :
    case 'b' : 
    case 'y' : 
    case 'w' : 
	break;
    default:
    	fprintf(stderr, "Expecting t|b|y|w for first argument\n");
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

  switch (argv[1][0]) {
    case 't':	TextGenerate(streams, samples); 	return 0;
    case 'b': 	BitGenerate(streams, samples);		return 0;
    case 'y':	ByteGenerate(streams, samples);		return 0;
    case 'w':	WordGenerate(streams, samples);		return 0;
  }
  return 0;
}
