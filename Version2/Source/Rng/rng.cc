
// $Id$

#include "rng.h"

#define INCLUDE_JUMP
#ifdef INCLUDE_JUMP

// Allow jumping of RNG state (necessary for simulation)

typedef unsigned long submatrix[32];
typedef submatrix *midmatrix[16][16];

#include "jump.h"

const unsigned int mask[32] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
			  0x08000000, 0x04000000, 0x02000000, 0x01000000,

			  0x00800000, 0x00400000, 0x00200000, 0x00100000,
			  0x00080000, 0x00040000, 0x00020000, 0x00010000,

			  0x00008000, 0x00004000, 0x00002000, 0x00001000,
			  0x00000800, 0x00000400, 0x00000200, 0x00000100,

			  0x00000080, 0x00000040, 0x00000020, 0x00000010,
			  0x00000008, 0x00000004, 0x00000002, 0x00000001 };

inline void vsm_mult(unsigned long v, submatrix *m, unsigned long &answer)
{
  if (NULL==m) return;
  if (0==v) return;
  for (int b=31; b>=0; b--) if ((*m)[b]) if (v & mask[b]) answer ^= (*m)[b];
}

// in and out are subvectors, by magic of pointer arithmetic!
void vmm_mult(unsigned long *in, midmatrix *m, unsigned long *out)
{
  int i,j;
  for (i=15; i>=0; i--)
    for (j=15; j>=0; j--)
      vsm_mult(in[i], (*m)[i][j], out[j]);
}

void Rng::JumpStream(Rng *input)
{
  state.initialized = input->state.initialized;
  state.stateptr = input->state.stateptr;

  // vector-matrix multiply
  int i,j;
  for (i=MT_STATE_SIZE-1; i>=0; i--) state.statevec[i] = 0;
  for (i=0; i<39; i++) {
    unsigned long *in = input->state.statevec + i*16;
    for (j=0; j<39; j++)
      vmm_mult(in, Jump[i][j], state.statevec + j*16);
  } // for i
}

#else

#include "../Base/streams.h"

// Disallow jumping of RNG state (faster compile, smaller exec.)

void Rng::JumpStream(Rng*)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Can't jump RNG state, jump module excluded from build!";
  Internal.Stop();
}

#endif
