
// $Id$

/* 
  Simple test of state storage:
  stores all combinations of N (nonnegative) integers
  that sum to M, in order,
  then reads them back to make sure they were encoded correctly.

*/

#include <stdio.h>
#include <string.h>
#include "statelib.h"
#include "timers.h"

using namespace StateLib;

const int extra_comparisons = 9;

void InitState(int* s, int N, int M)
{
  for (int i=0; i<N-1; i++) s[i] = 0;
  s[N-1] = M;
}

bool IncState(int* s, int N)
{
  int n = N-2;
  if (n<0) return false;
  // if last digit is 0, we need to set the
  // first nonzero to zero, then continue...

  if (0==s[N-1]) {
    while (0==s[n]) {
       n--;
    }
    if (0==n) return false;
    s[N-1] = s[n];
    s[n] = 0;
    n--;
  }

  // increment current digit, decrement final digit
  s[n]++;
  s[N-1]--; 
  return true;
}

void PrintState(int* s, int N)
{
  printf("[ %d", s[0]);
  for (int n=1; n<N; n++)
    printf(", %d", s[n]);
  printf("]\n");
}

void CountStates(int N, int M)
{
  timer watch;
  watch.Start();
  int* bits = new int[N];
  InitState(bits, N, M);
  long count;
  for (count=1; ; count++) {
    if (! IncState(bits, N)) break;
  }
  watch.Stop();
  printf("There are %ld states\n", count);
  printf("Enumeration took %lf\n", watch.User_Seconds());
}


int RunTest(int N, int M, bool useind, bool storesize)
{
  state_coll* foo = CreateCollection(useind, storesize);

  printf("Using collection with ");
  if (useind) printf("index"); else printf("arbitrary");
  printf(" handles, ");
  if (storesize) printf("stored"); else printf("known");
  printf(" sizes\n");
 
  printf("Storing sequences...");
  fflush(stdout);
  timer watch;
  watch.Start();

  int* state1 = new int[N];
  InitState(state1, N, M);
  for (;;) {
    long hndl;
    try {
      hndl = foo->AddState(state1, N);
    }
    catch (StateLib::error e) {
      printf("Couldn't store state: ");
      PrintState(state1, N);
      printf("\n\t%s\n", e.getName());
      return  1;
    }
    if (! IncState(state1, N)) break;
  }
  watch.Stop();
  printf("done, took %lf seconds\n", watch.User_Seconds());
  printf("Checking sequences");
  if (extra_comparisons) printf(" (%d times each)", extra_comparisons+1);
  printf("...");
  fflush(stdout);
  watch.Start();

  long h = foo->FirstHandle();
  InitState(state1, N, M);
  for (;;) {
    if (h<0) {
      printf("Got invalid handle %ld\n", h);
      printf("State should have been ");
      PrintState(state1, N);
      return 1;
    }
#ifdef DEBUG
    printf("Checking handle %ld\n", h);
#endif

    int cmp;
    if (useind) {

      // Using indexes;
      // Use fast comparison
      long newh;
      try {
        newh = foo->AddState(state1, N);
      }
      catch (StateLib::error e) {
        printf("Couldn't temporarily add state ");
        PrintState(state1, N);
        printf("\n\t%s\n", e.getName());
        return 1;
      }
      for (int z=0; z<extra_comparisons; z++) {
        foo->CompareHH(h, newh);
      }
      cmp = foo->CompareHH(h, newh);
      if (!foo->PopLast(newh)) {
        printf("Couldn't remove temporary state %ld : ", newh);
        PrintState(state1, N);
        return 1;
      }

    } else { // else for "if useindex"

      // Not using indexes;
      // requires slow comparison
      for (int z=0; z<extra_comparisons; z++) {
        foo->CompareHF(h, N, state1);
      }
      cmp = foo->CompareHF(h, N, state1);

    } // end "if useindex"

    // printf("%d\n", h);

    // were the states equal?
    if (cmp!=0) {
      printf("State mismatch from handle %ld : \n", h);
      PrintState(state1, N);
      return 1;
    }

    // advance 
    if (! IncState(state1, N)) break;
    h = foo->NextHandle(h);
    if (h<1) {
      printf("Out of stored states, next one should be ");
      PrintState(state1, N);
      return 1;
    }

  } // for loop
  watch.Stop();
  printf("done, took %lf seconds\n", watch.User_Seconds());
  delete[] state1;

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
  printf("\tAvg. bytes per state: %f\n\n", bps);
  printf("End of report\n");

  delete foo;
  return 0;
}

int main()
{
  puts(LibraryVersion());
  int N;
  int M;
  printf("Enter number of integers N:\n");
  scanf("%d", &N);
  if (N<1) return 0;

  printf("Enter target sum M:\n");
  scanf("%d", &M);
  if (M<1) return 0;

  printf("Examining sequences of %d integers that sum to %d\n", N, M);

  CountStates(N, M);

  if (RunTest(N, M, false, false)) return 1;
  if (RunTest(N, M, false, true)) return 1;
  if (RunTest(N, M, true, false)) return 1;
  if (RunTest(N, M, true, true)) return 1;

  printf("Done!\n");
  return 0;
}
