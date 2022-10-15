
#include "rng.h"

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>

#ifdef DCASSERTS_ON 
#define DCASSERT(X) assert(X)
#else
#define DCASSERT(X)
#endif

/*
    Random Number Generator Library Implementation.
*/

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 3;

// ******************************************************************
// *                       rng_stream methods                       *
// ******************************************************************

rng_stream::rng_stream()
{
}

rng_stream::~rng_stream()
{
}

// ******************************************************************
// *                                                                *
// *                     well1024a_stream class                     *
// *                                                                *
// ******************************************************************

/**
    Random number generator stream.
    The algorithm is the WELL1024a RNG as described in
    "Improved Long-Period Generators Based on Linear Recurrences Modulo 2"
    by
    Francois Panneton, Pierre L'Ecuyer, Makoto Matsumoto
*/
class well1024a_stream : public rng_stream {
protected:
  unsigned int* state;
  int state_ptr;
public:
  well1024a_stream();
  virtual ~well1024a_stream();

  virtual unsigned int RandomWord();
  virtual double Uniform32();
  virtual double Uniform64();

  friend class well1024a_manager_32;

  inline void Clear() {
    memset(state, 0, 32*sizeof(unsigned int));
  }

protected:
  inline unsigned int MAT3POS(int t, unsigned int v)  { return v^(v>>t); }
  inline unsigned int MAT3NEG(int t, unsigned int v)  { return v^(v<<t); }

  inline unsigned int raw_word() {
    // The WELL 1024 algorithm
    const int M1=3;
    const int M2=24;
    const int M3=10;

    unsigned int z0 = state[ (state_ptr+31) & 0x0000001fUL ];

    unsigned int z1 = 
      state[ state_ptr ] 
      ^ 
      MAT3POS( 8, state[ (state_ptr+M1) & 0x0000001fUL ]);

    unsigned int z2 = 
      MAT3NEG(19, state[ (state_ptr+M2) & 0x0000001fUL ]) 
      ^ 
      MAT3NEG(14, state[ (state_ptr+M3) & 0x0000001fUL ]);

    state[state_ptr] = z1 ^ z2;
    state[(state_ptr+31) & 0x0000001fUL ] = 
    MAT3NEG(11, z0) ^ MAT3NEG(7, z1) ^ MAT3NEG(13, z2);
    state_ptr = (state_ptr + 31) & 0x0000001fUL;
    return state[state_ptr];
  }
};

// ******************************************************************
// *                    well1024a_stream methods                    *
// ******************************************************************

well1024a_stream::well1024a_stream()
{
  // note: calloc will clear the memory (desired).
  state = (unsigned int*) calloc(32, sizeof(unsigned int));
  state_ptr = 0;
}

well1024a_stream::~well1024a_stream()
{
  free(state);
}

unsigned int well1024a_stream::RandomWord()
{
  return raw_word();
}

double well1024a_stream::Uniform32()
{
  unsigned int u;
  do { u = raw_word(); } while (0==u);
  return u * (1.0 / 4294967296.0);
}

double well1024a_stream::Uniform64()
{
  unsigned int u[2];
  u[0] = raw_word();
  u[1] = raw_word();
  int which = 0;
  while (0==u[0] && 0==u[1]) {
    u[which] = raw_word();
    which = (which+1) % 2;
  }
  double unif = u[0];
  unif *= 4294967296.0;
  unif += u[1];
  return unif * (1.0 / 9223372036854775808.0);
}



// ******************************************************************
// *                      rng_manager  methods                      *
// ******************************************************************

rng_manager::rng_manager()
{
}

rng_manager::~rng_manager()
{
}

// ******************************************************************
// *                                                                *
// *                    well1024a_manager  class                    *
// *                                                                *
// ******************************************************************

class well1024a_manager_32 : public rng_manager {
  unsigned int* jump_matrix;
  int jump_distance;
  static const int MIN_D = 1;
  static const int MAX_D = 1000;
  char* buffer;
public:
  well1024a_manager_32(int jd);
  virtual ~well1024a_manager_32();

  virtual rng_stream* NewBlankStream();
  virtual void InitStreamFromSeed(rng_stream* s, int seed);
  virtual void InitStreamByJumping(rng_stream* s, const rng_stream* j);
  virtual rng_stream* NewStreamFromSeed(int seed);
  virtual rng_stream* NewStreamByJumping(const rng_stream* j);
  virtual bool SetJumpValue(int d);
  virtual int GetJumpValue() { return jump_distance; }
  virtual int  MinimumJumpValue() { return MIN_D; }
  virtual int  MaximumJumpValue() { return MAX_D; }
  virtual const char*  GetVersion() const { return buffer; }

protected:
  void Initialize(well1024a_stream* s, int seed);
  void JumpStream(well1024a_stream* s, const well1024a_stream* j);
 
  inline void SWAP(unsigned int * &A, unsigned int* &B) {
    unsigned int* C = A;
    A = B;
    B = C;
  }
  void VMMult(const unsigned int* x, const unsigned int* M, unsigned int* y);
  void MMMult(const unsigned int* B, const unsigned int* C, unsigned int* M) {
    for (int rows=1024; rows; rows--) {
      VMMult(B, C, M);
      B += 32;
      M += 32;
    } // for rows
  }
  inline void SetSubmatrix(int i, int j, const unsigned int* sub) {
    for (int w=0; w<32; w++) jump_matrix[(i*32+w)*32 + j] = sub[w];
  }
  void BuildJumpMatrix();
};

// ******************************************************************
// *                   well1024a_manager  methods                   *
// ******************************************************************

well1024a_manager_32::well1024a_manager_32(int jd)
{
  jump_distance = 40;
  if ((jd >= MIN_D) && (jd <= MAX_D))  jump_distance = jd;
  jump_matrix = 0;

  const int bsize = 80;
  buffer = new char[bsize];
  snprintf(buffer, bsize, "Rng library version %d.%d (WELL 1024a, 32-bit)",
     MAJOR_VERSION, MINOR_VERSION);

  // TBD - revision number?
}

well1024a_manager_32::~well1024a_manager_32()
{
  free(buffer);
  free(jump_matrix);
}

rng_stream* well1024a_manager_32::NewBlankStream()
{
  return new well1024a_stream;
}

void well1024a_manager_32::InitStreamFromSeed(rng_stream* s, int seed)
{
  well1024a_stream* ws = dynamic_cast <well1024a_stream*> (s);
  if (ws)  Initialize(ws, seed);
}

void well1024a_manager_32::InitStreamByJumping(rng_stream* s, const rng_stream* j)
{
  well1024a_stream* ws = dynamic_cast <well1024a_stream*> (s);
  const well1024a_stream* wj = dynamic_cast <const well1024a_stream*> (j);
  if (0==ws)  return;
  if (0==wj) {
    ws->Clear();
    return;
  }
  JumpStream(ws, wj); 
}

rng_stream* well1024a_manager_32::NewStreamFromSeed(int seed)
{
  if (seed < 1)  return 0;
  well1024a_stream* s = new well1024a_stream;
  Initialize(s, seed);
  return s;  // success
}

rng_stream* well1024a_manager_32::NewStreamByJumping(const rng_stream* j)
{
  const well1024a_stream* wj = dynamic_cast <const well1024a_stream*> (j);
  if (0==wj)  return 0;
  well1024a_stream* s = new well1024a_stream;
  JumpStream(s, wj);
  return s; // success
}

bool well1024a_manager_32::SetJumpValue(int d)
{
  if (d<MIN_D) return false;
  if (d>MAX_D) return false;
  if (jump_distance == d) return true;
  free(jump_matrix);
  jump_matrix = 0;
  jump_distance = d;
  return true;
}

void well1024a_manager_32::Initialize(well1024a_stream* s, int seed)
{
  DCASSERT(s);
  // Fill state using Park's old RNG
  const int A = 48271;
  const int M = 2147483647;
  const int Q = M / A;
  const int R = M % A;
  s->state[31] = seed;
  for (int i=30; i>=0; i--) {
    int t1 = A * (s->state[i+1] % Q);
    int t2 = R * (s->state[i+1] / Q);
    if (A>R) s->state[i] = t1-t2;
    else s->state[i] = (M-t2)+t1;
  }
  // advance state 32 times to clear out the "bad" RNG values.
  for (int i=31; i>=0; i--) s->raw_word();
}

void well1024a_manager_32
 ::JumpStream(well1024a_stream* s, const well1024a_stream* j)
{
  DCASSERT(s);
  DCASSERT(j);
  BuildJumpMatrix();  // NO-OP if already built
  VMMult(j->state, jump_matrix, s->state);
}

void well1024a_manager_32::VMMult(const unsigned int* x, const unsigned int* M, unsigned int* y) 
{
  for (int jw=0; jw<32; jw++) {
    for (unsigned mask = 0x80000000UL; mask; mask /= 2) {
      if (x[0] & mask) {
        // add next row of M to y
        y[ 0] ^= M[ 0];
        y[ 1] ^= M[ 1];
        y[ 2] ^= M[ 2];
        y[ 3] ^= M[ 3];
        y[ 4] ^= M[ 4];
        y[ 5] ^= M[ 5];
        y[ 6] ^= M[ 6];
        y[ 7] ^= M[ 7];
        y[ 8] ^= M[ 8];
        y[ 9] ^= M[ 9];
        y[10] ^= M[10];
        y[11] ^= M[11];
        y[12] ^= M[12];
        y[13] ^= M[13];
        y[14] ^= M[14];
        y[15] ^= M[15];
        y[16] ^= M[16];
        y[17] ^= M[17];
        y[18] ^= M[18];
        y[19] ^= M[19];
        y[20] ^= M[20];
        y[21] ^= M[21];
        y[22] ^= M[22];
        y[23] ^= M[23];
        y[24] ^= M[24];
        y[25] ^= M[25];
        y[26] ^= M[26];
        y[27] ^= M[27];
        y[28] ^= M[28];
        y[29] ^= M[29];
        y[30] ^= M[30];
        y[31] ^= M[31];
      }
      // next row of M
      M += 32;
    } // jb
    x++;
  } // jw
}


void well1024a_manager_32::BuildJumpMatrix()
{
  if (jump_matrix) return;

  jump_matrix = (unsigned int*) calloc(1024*32, sizeof(unsigned int));

  // Build extremely magic jump matrix, characteristic of WELL1024.

  const unsigned int sub1[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,262144,131072,65536,32768,16384,8192,4096,2048,1024,512,256,128,64,32,16,8,4,2,1};
  
  const unsigned int sub2[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,2164260864ul,1082130432,541065216,270532608,135266304,67633152,33816576,16908288,8454144,4227072,2113536,1056768,528384,264192,132096,66048,33024,16512,8256,4128,2064,1032,516,258,129};

  const unsigned int sub3[32] = {3229614080u,1614807040,807403520,403701760,201850880,100925440,50462720,2172715008ul,1086357504,543178752,271589376,135794688,67897344,33948672,16974336,8487168,4243584,2121792,1060896,530448,265224,132612,66306,33153,16512,8256,4128,2064,1032,516,258,129};

  const unsigned int sub4[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,2147745792ul,1073872896,536936448,268468224,134234112,67117056,2181042176ul,1090521088,545260544,272630272,136315136,68157568,34078784,17039392,8519696,4259848,2129924,1064962,532481};

  const unsigned int sub5[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,2147745792ul,3221356544ul,1610678272,805339136,402669568,201334784,100667392,50333696,25166848,12583424,6291712,3145856,1572928,786464,2147876880ul,1073938440,536969220,268484610,134242305};

  const unsigned int sub6[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,2148532224ul,1074266112,537133056,268566528,134283264,67141632,33570816,16785408,8392704,4196352,2098176,1049088,524544,262272,131136,65568,32784,16392,8196,4098,2049};

  const unsigned int sub7[32] = {2155872256u,1077936128,538968064,269484032,134742016,67371008,33685504,16842752,8421376,4210688,2105344,1052672,526336,263168,131584,65792,32896,16448,8224,4112,2056,1028,514,257,128,64,32,16,8,4,2,1};

  const unsigned int sub8[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,262144,131072,65536,32768,16384,8192,2147487744ul,1073743872,536871936,268435968,134217984,67108992,33554496,16777248,8388624,4194312,2097156,1048578,524289};

  const unsigned int sub9[32] = {2147483648u,1073741824,536870912,268435456,134217728,67108864,33554432,16777216,8388608,4194304,2097152,1048576,524288,262144,2147614720ul,1073807360,536903680,268451840,134225920,67112960,33556480,16778240,8389120,4194560,2097280,1048640,524320,262160,131080,65540,32770,16385};

  for (int t=0; t<31; t++) SetSubmatrix(t, t+1, sub1);
  SetSubmatrix(0, 0, sub2);
  SetSubmatrix(3, 0, sub3);
  SetSubmatrix(24, 0, sub4);
  SetSubmatrix(10, 0, sub5);
  SetSubmatrix(31, 0, sub6);
  SetSubmatrix(3, 1, sub7);
  SetSubmatrix(24, 1, sub8);
  SetSubmatrix(10, 1, sub9);

  // Now, square it ssep times
  unsigned* tmp = (unsigned*) malloc (1024*32*sizeof(unsigned));
  for (int d = 0; d<jump_distance; d++) {
    memset(tmp, 0, 1024*32*sizeof(unsigned));
    MMMult(jump_matrix, jump_matrix, tmp);
    SWAP(jump_matrix, tmp);
  }
  free(tmp);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

rng_manager* RNG_MakeStreamManager()
{
  return new well1024a_manager_32(40);
}

