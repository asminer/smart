
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "mtwist.h"
#include "../Base/timers.h"

//#define USE_SHARED

void smart_exit()
{
}

#ifdef USE_SHARED
void Braised(int n, shared_matrix* B, shared_matrix *tmp, shared_matrix *answer)
#else
void Braised(int n, fullmatrix* B, fullmatrix *tmp, fullmatrix *answer)
#endif
{
  int nnz; 
  if (2==n) {
    nnz = answer->Multiply(B, B);
  } else if (n%2 == 0) {
    // even, square it
    Braised(n/2, B, answer, tmp);
    nnz = answer->Multiply(tmp, tmp);
  } else {
    // odd
    Braised(n-1, B, answer, tmp);
    nnz = answer->Multiply(B, tmp);
  }
  Output << "Computed B^" << n << "\t\t\t";
#ifdef USE_SHARED
  Output << answer->Distinct() << " distinct / ";
#endif
  Output << nnz << " nzs\n";
  Output.flush();
}

const unsigned int A = 0x9908b0df;
const int N = 624;
const int M = 397;

int CYCLES = 1;

#ifdef USE_SHARED
void MakeMatrix(shared_matrix &answer)
#else
void MakeMatrix(fullmatrix &fullans)
#endif
{
  Output << "Initializing...\n";
  Output.flush();

  if (CYCLES < 2) {
#ifndef USE_SHARED
    shared_matrix answer(N);
#endif
    answer.MakeBN(M, A);
#ifndef USE_SHARED
    fullans.FillFrom(answer);
#endif
    return;
  }

  shared_matrix BN(N);
#ifdef USE_SHARED
  shared_matrix tmp(N);
#else
  fullmatrix fullBN(N);
  fullmatrix tmp(N);
#endif

  BN.MakeBN(M, A);
  Output << "Computed B\n";
  Output.flush();

#ifdef USE_SHARED
  Braised(CYCLES, &BN, &tmp, &answer);
#else
  fullBN.FillFrom(BN);
  Braised(CYCLES, &fullBN, &tmp, &fullans);
#endif

  MatrixStats();

}

int main()
{
  Output << "How many refresh cycles?\n";
  Output.flush();

  Input.Get(CYCLES);

  timer compute;
  compute.Start();
  
#ifdef USE_SHARED
  shared_matrix refresh(N);
#else
  fullmatrix refresh(N);
#endif
  MakeMatrix(refresh);

  compute.Stop();
  Output << "That took: " << compute << "\n";

  Output << "Comparing against MT19937\n";
  Output.flush();
  int c;
  int rep;

  compute.Start();

  mt_state mine1, mine2;
  mts_seed32(&mine1, 7309259);
  mt_seed32(7309259);

  mt_state* current = &mine1;
  mt_state* next = &mine2;
 
  for (rep=0; rep<1000; rep++) {
    // advance seeds CYCLES times
    // us:
    refresh.vector_multiply(next->statevec, current->statevec);
    SWAP(next, current);
    // them:
    for (c=0; c<CYCLES; c++) mts_refresh(&mt_default_state);

    // compare vectors
    int i;
    for (i=0; i<N; i++) {
      if (current->statevec[i] != mt_default_state.statevec[i]) {
        Output << "Replication " << rep << "\n";
        Output << "Difference at position " << i << ":\n";
	Output << "Us  : " << (int) current->statevec[i] << "\n";
	Output << "Them: " << (int) mt_default_state.statevec[i] << "\n";
	Output.flush();
	return 0;
      }
    } // for i
  }
  
  compute.Stop();
  Output << "Comparison took: " << compute << "\n";
  Output << "That's all for now\n";
  Output.flush();
  return 0;
}
