
/* $Id$ */

/*
  Simple utility to randomly shuffle arguments and print them
*/

#include "rng.h"
#include <iostream>
#include <time.h>

inline int equilikely(rng_stream* s, int a, int b)
{
  return a + int((b-a+1) * s->Uniform32());
}

inline void swap(char* &A, char* &B)
{
  char* T = A;
  A = B;
  B = T;
}

int usage(const char* who)
{
  const char* stripped = who;
  for (const char* p = who; *p; p++) {
    if ('/' == *p) stripped = p+1;
  }
  std::cerr << "Usage: " << stripped << "list of args\n";
  std::cerr << "    Prints a random shuffle of the arguments\n\n";
  return 0;
}

int main(int argc, char** argv)
{
  if (argc<2) return usage(argv[0]);

  int seed = time(0);

  rng_manager* rm = RNG_MakeStreamManager();
  rng_stream* s = rm->NewStreamFromSeed(seed);

  // Shuffle arguments

  for (int i=1; i<argc-1; i++) {
    int j = equilikely(s, i, argc-1);
    swap(argv[i], argv[j]);
  }

  // Display arguments
  for (int i=1; i<argc; i++) {
    std::cout << argv[i] << " ";
  }
  std::cout << "\n";

  // cleanup

  delete s;
  delete rm;
}

