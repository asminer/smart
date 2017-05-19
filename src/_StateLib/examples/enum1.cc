
// $Id$

/* 
  Simple test of state storage:
  stores all 2^n combinations of n-bit states, in order,
  then reads them back to make sure they were encoded correctly.

*/

#include <stdio.h>
#include <string.h>
#include "statelib.h"
#include "timerlib.h"

using namespace StateLib;

// #define DEBUG

const int extra_comparisons = 9;

void InitState(int* s, int N)
{
  for (int i=0; i<N; i++) s[i] = 0;
}

bool IncState(int* s, int N)
{
  int n;
  for (n=N-1; n>=0; n--) {
    if (0==s[n]) {
      s[n] = 1;
      return true;
    }
    s[n] = 0;
  }
  return false;
}

void PrintState(int* s, int N)
{
  for (int n=0; n<N; n++) 
    if (s[n])   fputc('1', stdout);
    else        fputc('0', stdout);
  fputc('\n', stdout);
}

void CountStates(int N)
{
  timer watch;
  int* bits = new int[N];
  InitState(bits, N);
  long count;
  for (count=1; ; count++) {
    if (! IncState(bits, N)) break;
  }
  printf("There are %ld states\n", count);
  printf("Enumeration took %lf\n", watch.elapsed_seconds());
}

int TestFor(int N, bool useindex, bool storesizes)
{
  state_coll* foo = CreateCollection(useindex, storesizes);
 
  printf("Using collection with ");
  if (useindex) printf("index"); else printf("arbitrary");
  printf(" handles, ");
  if (storesizes) printf("stored"); else printf("known");
  printf(" sizes\n");
  printf("Storing all %d-bit states...", N);
  fflush(stdout);
  timer watch;
  int* bits = new int[N];
  InitState(bits, N);
  for (;;) {
#ifdef DEBUG
    printf("Encoding state: ");
    PrintState(bits, N);
#endif
    long hndl;
    try {
      hndl = foo->AddState(bits, N);
    }
    catch (StateLib::error e) {
      printf("Couldn't store state: ");
      PrintState(bits, N);
      printf("\n\t%s\n", e.getName());
      return  1;
    }

#ifdef DEBUG
    printf("Got handle %d\n\n", hndl);
#endif

    if (! IncState(bits, N)) break;
  }
  printf("done, took %lf seconds\n", watch.elapsed_seconds());

  printf("Checking all %d-bit states", N);
  if (extra_comparisons) printf(" (%d times each)", extra_comparisons+1);
  printf("...");
  fflush(stdout);
  watch.reset();

  long h = foo->FirstHandle();
  InitState(bits, N);
  for (;;) {
    if (h<0) {
  printf("Got invalid handle %ld\n", h);
  printf("State should have been ");
  PrintState(bits, N);
  return 1;
    }
#ifdef DEBUG
    printf("Checking handle %ld\n", h);
#endif

    int cmp;
    if (useindex) {

      // Using indexes;
      // Use fast comparison
      long newh;
      try {
        newh = foo->AddState(bits, N);
      }
      catch (StateLib::error e) {
        printf("Couldn't temporarily add state ");
        PrintState(bits, N);
        printf("\n\t%s\n", e.getName());
        return 1;
      }
      for (int z=0; z<extra_comparisons; z++) {
        foo->CompareHH(h, newh);
      }
      cmp = foo->CompareHH(h, newh);
      if (!foo->PopLast(newh)) {
        printf("Couldn't remove temporary state %ld : ", newh);
        PrintState(bits, N);
        return 1;
      }

    } else { // else for "if useindex"

      // Not using indexes;
      // requires slow comparison

      for (int z=0; z<extra_comparisons; z++) {
        foo->CompareHF(h, N, bits);
      }

      cmp = foo->CompareHF(h, N, bits);

    } // end "if useindex"

    // printf("%d\n", h);

    // were the states equal?
    if (cmp!=0) {
        printf("State mismatch from handle %ld : \n", h);
        PrintState(bits, N);
        return 1;
    }

    // advance 
    if (! IncState(bits, N)) break;
    h = foo->NextHandle(h);
    if (h<1) {
      printf("Out of stored states, next one should be ");
      PrintState(bits, N);
      return 1;
    }

  } // for loop
  printf("done, took %lf seconds\n", watch.elapsed_seconds());
  delete[] bits;

  printf("Report for collection:\n");
  for (int m=0; m<foo->NumEncodingMethods(); m++) {
    int cnt = foo->ReportEncodingCount(m);
    if (0==cnt) continue;
    printf("\t %9d encodings are %s\n", cnt, foo->EncodingMethod(m));
  }
  printf("\t %9ld states total\n", foo->Size());
  printf("\tTotal memory: %ld\n", foo->ReportMemTotal());
  double bps = foo->ReportMemTotal();
  bps /= foo->Size();
  printf("\tAvg. bytes per state: %f\n", bps);
  printf("End of report\n");

  delete foo;
  return 0;
}

int main()
{
  puts(LibraryVersion());
  int N;
  printf("Enter number of bits N:\n");
  scanf("%d", &N);
  if (N<1) return 0;

  CountStates(N);

  if (TestFor(N, false, false)) return 1;
  if (TestFor(N, false, true)) return 1;
  if (TestFor(N, true, false)) return 1;
  if (TestFor(N, true, true)) return 1;

  printf("Done!\n");
  return 0;
}
