
#include "multstrm.h"

//#define CHECK_CACHE

#define NO_CACHE

// ------------------------------------------------------------------
//   hash table.  eventually replace with a nice general class.
// ------------------------------------------------------------------

int hashsizes[15] = { 	0, 251, 503, 1013, 2027, 4057, 8117, 16249, 32503, 
			65011, 130027, 260081, 520193, 1040387, 2147483647 };

struct hashnode {
    void* data;
    hashnode *next;
};

template <class DATA>
class myhash {
  int size_index;
  int count;
  int maxchain;
  hashnode** table;
  Manager <hashnode> *node_pile;
protected:
  inline unsigned int HashFunc(hashnode *x) {
    return ((DATA*)x->data)->Signature(Size());
  }
public:
  myhash(Manager <hashnode>* np);
  ~myhash() { free(table); }
  DATA* Find(DATA *);
  DATA* Insert(DATA *x);
  void Resize();
  void Dump(); // for debugging
  inline int Entries() const { return count; }
  inline int Size() const { return hashsizes[size_index]; }
  inline int MaxChain() const { return maxchain; }
};

template <class DATA>
myhash<DATA>::myhash(Manager <hashnode>* np)
{
  size_index = 1;
  table = (hashnode **) malloc(sizeof(void*) * Size());
  int i;
  for (i=0; i<Size(); i++) table[i] = NULL;
  count = 0;
  maxchain = 0;
  node_pile = np; 
}

template <class DATA>
void myhash<DATA>::Resize()
{
  // build huge list
  hashnode* list = NULL;
  int i = 0;
  for (i=0; i<Size(); i++) {
    while (table[i]) {
      hashnode* link = table[i]->next;
      // check if this is a stale entry
      if (Stale((DATA*)table[i]->data)) {
	// Don't add to the list; recycle the hash node instead
        node_pile->FreeObject(table[i]);
	count--;
      } else {
	// Add to the front of the list
        table[i]->next = list;
        list = table[i];
      }
      table[i] = link;
    }
  }
  // hash table = one huge list
  
  bool sizechange = false;
  if (count > hashsizes[size_index+1]) {
    size_index++;
    sizechange = true;
  }
  if (count < hashsizes[size_index-1]) {
    size_index--;
    sizechange = true;
  }
  if (sizechange) {
    table = (hashnode **) realloc(table, sizeof(void*) * hashsizes[size_index]);
    for (i=0; i<Size(); i++) table[i] = NULL;
  }

  // put them back into the list
  while (list) {
    int h = HashFunc(list);
    hashnode* link = list->next;
    list->next = table[h];
    table[h] = list;
    list = link;
  }
  maxchain = 0;
}

template <class DATA>
DATA* myhash<DATA>::Find(DATA *m)
{
  unsigned int h = m->Signature(Size());
  hashnode *ptr;
  hashnode *prev = NULL;
  for (ptr = table[h]; ptr; ptr=ptr->next) {
    if (0 == Compare((DATA*)ptr->data, m)) {
      if (prev) {  // not at front, make it so
	prev->next = ptr->next;
	ptr->next = table[h];
	table[h] = ptr;
      }
      return (DATA*)ptr->data;
    }
  } // for
  return NULL;
}

template <class DATA>
DATA* myhash<DATA>::Insert(DATA *m)
{
  if (count > hashsizes[size_index+1]) Resize();
  unsigned int h = m->Signature(Size());
  hashnode *ptr;
  hashnode *prev = NULL;
  int time = 0;
  ptr = table[h];
  while (ptr) {
    time++;
    if (0 == Compare((DATA*)ptr->data, m)) {
      if (prev) {  // not at front, make it so
	prev->next = ptr->next;
	ptr->next = table[h];
	table[h] = ptr;
      }
      maxchain = MAX(maxchain, time);
      return (DATA*)ptr->data;
    }
    // since we're here, check if this entry is stale
    if (Stale((DATA*)ptr->data)) {
      // remove this link
      hashnode* nxt = ptr->next;
      if (prev) prev->next = nxt;
      else table[h] = nxt;
      node_pile->FreeObject(ptr);
      ptr = nxt;
      count--;
    } else {
      prev = ptr;
      ptr = ptr->next;
    }
  } // for
  // not there, insert it
  count++;
  time++;
  maxchain = MAX(maxchain, time);
  ptr = node_pile->NewObject();
  ptr->data = m;
  ptr->next = table[h];
  table[h] = ptr;
  return m;
}

template <class DATA>
void myhash<DATA>::Dump()
{
  int i = 0;
  for (i=0; i<Size(); i++) {
    for (hashnode* ptr = table[i]; ptr; ptr=ptr->next) 
      DumpObject((DATA*)ptr->data);
  }
}

void DumpObject(bitmatrix *x)
{
  Output << "\tSubmatrix ";
  Output.PutHex((unsigned int)x);
  Output << "\t ptrcount: " << x->ptrcount;
  Output << "\t cachecount: " << x->cachecount;
  Output << "\n";
  Output.flush();
}


// ------------------------------------------------------------------
//   Global vars.  Initialized below.
// ------------------------------------------------------------------

bitmatrix* IDENTITY = NULL;

struct bincache {
  bitmatrix *b;
  bitmatrix *c;
  bitmatrix *answer;
  inline unsigned int Signature(int prime) {
    return (((unsigned int) b) * 256 + (unsigned int) c) % prime;
  }
};

void DumpObject(bincache *x)
{
  Output << "\t(";
  Output.PutHex((unsigned int) x->b);
  Output << " ";
  Output.PutHex((unsigned int) x->c);
  Output << " ";
  Output.PutHex((unsigned int) x->answer);
  Output << ") \t";
  Output << " (" << x->b->ptrcount << ", " << x->b->cachecount << ")";
  Output << " (" << x->c->ptrcount << ", " << x->c->cachecount << ")";
  if (x->answer)
    Output << " (" << x->answer->ptrcount << ", " << x->answer->cachecount << ")";
  Output << "\n";
  Output.flush();
}

Manager <hashnode> hash_node_pile(1024);

Manager <bitmatrix> matrix_pile(1024);
Manager <bincache> cache_pile(1024);

myhash <bitmatrix> UniqueTable(&hash_node_pile);
myhash <bincache> ComputeTable(&hash_node_pile);

int cachetries = 0;
int cachehits = 0;

inline int Compare(bitmatrix *a, bitmatrix *b)
{
  for (int i=0; i<32; i++) {
    if (a->row[i] < b->row[i]) return -1;
    if (a->row[i] > b->row[i]) return 1;
  }
  return 0;
}

inline bool Stale(bitmatrix *m) 
{ 
  if ((m->ptrcount) || (m->cachecount)) return false;

  matrix_pile.FreeObject(m);
  return true;
}

inline bitmatrix* Reduce(bitmatrix *a)
{
  if (a->is_zero()) {
    matrix_pile.FreeObject(a);
    return NULL;
  }
  bitmatrix *d = UniqueTable.Insert(a);
  if (d!=a) matrix_pile.FreeObject(a);
  else {
    // a is new and unique...
    a->ptrcount = a->cachecount = 0;
  }
  return d;
}

inline int Compare(bincache *x, bincache *y)
{
  // 0 means equal

  if ((x->b == y->b) && (x->c == y->c)) return 0;

// supposed to be like strcmp, but only used for equality, so no matter
  return 1;
}

inline bool Stale(bincache *x)
{
  if (x->answer) {
    if (x->b->ptrcount && x->c->ptrcount && x->answer->ptrcount) 
      return false;
  } else {
    if (x->b->ptrcount && x->c->ptrcount) 
      return false;
  }

  x->b->cachecount--;
  x->c->cachecount--;
  if (x->answer) x->answer->cachecount--;

#ifdef CHECK_CACHE
  Output << "  Remove ";
  Output.PutHex((unsigned int)x->b);
  Output << " ";
  Output.PutHex((unsigned int)x->c);
  Output << " got ";
  Output.PutHex((unsigned int)x->answer);
  Output << "\n";
  Output.flush();
#endif
  
  cache_pile.FreeObject(x);
  return true;
}


void CacheAdd(bitmatrix *b, bitmatrix *c, bitmatrix *ans)
{
#ifdef NO_CACHE
  return;
#else
  b->cachecount++;
  c->cachecount++;
  if (ans) ans->cachecount++;
  bincache* foo = cache_pile.NewObject();
  foo->b = b;
  foo->c = c;
  foo->answer = ans;
  ComputeTable.Insert(foo);
#endif
}

bool CacheHit(bitmatrix* b, bitmatrix* c, bitmatrix* &ans)
{
  cachetries++;
  bincache tmp;
  tmp.b = b;
  tmp.c = c;
  bincache *find = ComputeTable.Find(&tmp);
  if (find) {
    cachehits++;
    ans = find->answer;
    return true;
  }
  return false;
}

bitmatrix* cache_mult(bitmatrix *b, bitmatrix *c)
{
  if (NULL==b) return NULL;
  if (NULL==c) return NULL;

  // special case (keeps cache down)
  /*
  if (b == IDENTITY) return c;
  if (c == IDENTITY) return b;
  */
   
  bitmatrix *answer;
  if (CacheHit(b, c, answer)) return answer;

  answer = matrix_pile.NewObject();
  mm_mult(answer, b, c); 

  answer = Reduce(answer);
  CacheAdd(b, c, answer);

#ifdef CHECK_CACHE
  Output << "Multiply ";
  Output.PutHex((unsigned int)b);
  Output << " ";
  Output.PutHex((unsigned int)c);
  Output << " got ";
  Output.PutHex((unsigned int)answer);
  Output << "\n";
  Output.flush();
#endif
  
  return answer;
}

bitmatrix* nocache_mult(bitmatrix *b, bitmatrix *c)
{
  if (NULL==b) return NULL;
  if (NULL==c) return NULL;

  bitmatrix *answer;
  answer = matrix_pile.NewObject();
  mm_mult(answer, b, c); 

  if (answer->is_zero()) {
    matrix_pile.FreeObject(answer);
    answer = NULL;
  }
  return answer;
}

// ------------------------------------------------------------------
//   Shared matrix class.  Zero submatrices beome NULL.
// ------------------------------------------------------------------


shared_matrix::shared_matrix(int n)
{
  N = n;
  ptrs = new bitmatrix**[N];
  int i,j;
  for (i=0; i<N; i++) {
    ptrs[i] = new bitmatrix*[N];
  }
  initialized = false;
}

shared_matrix::~shared_matrix()
{
  int i;
  for (i=0; i<N; i++) delete[] ptrs[i];
  delete[] ptrs;
}

void shared_matrix::MakeB(int M, unsigned int A)
{
  int i,j;
  for (i=0; i<N; i++) 
    for (j=0; j<N; j++)
      ptrs[i][j] = NULL;

  // Make the submatrices

  InitMatrix(); // just in case

  bitmatrix* L = matrix_pile.NewObject();
  L->row[0] = 0;
  for (i=1; i<31; i++) L->row[i] = mask[i+1];
  L->row[31] = A;  
  L = Reduce(L);

  bitmatrix* U = matrix_pile.NewObject();
  U->row[0] = mask[1];
  for (i=1; i<32; i++) U->row[i] = 0;
  U = Reduce(U);

  // Fill
  for (i=1; i<N; i++)
    SetPtr(i-1, i, IDENTITY);
  SetPtr(N-M-1, 0, IDENTITY);
  SetPtr(N-2, 0, L);
  SetPtr(N-1, 0, U);

  initialized = true;
}

void shared_matrix::MakeBN(int M, unsigned int A)
{
  int i,j;
  for (i=0; i<N; i++) 
    for (j=0; j<N; j++)
      ptrs[i][j] = NULL;

  // Make the submatrices

  InitMatrix(); // just in case

  bitmatrix* L = matrix_pile.NewObject();
  L->row[0] = 0;
  for (i=1; i<31; i++) L->row[i] = mask[i+1];
  L->row[31] = A;  
  L = Reduce(L);

  bitmatrix* U = matrix_pile.NewObject();
  U->row[0] = mask[1];
  for (i=1; i<32; i++) U->row[i] = 0;
  U = Reduce(U);

  // Init: identity  (will be overwritten)
  for (i=0; i<N; i++) SetPtr(i, i, IDENTITY);
  
  // Fill (see MT paper pseudocode to explain) 

  int k;
  for (k=N-1; k>M-1; k--) {
    SetPtr(k-M, k, IDENTITY);
    SetPtr(k-1, k, L);
    SetPtr(k, k, U); 
  }
  for (k=M-1; k; k--) {
    ColCpy(k, k+N-M); 
    SetPtr(k-1, k, L);
    SetPtr(k, k, U); 
  }
  ColCpy(0, N-M);

  SetPtr(0, 0, U);
  if (ptrs[(N-1)-M][0]) {
    bitmatrix* crust = matrix_pile.NewObject();
    mm_acc(crust, ptrs[(N-1)-M][0]);
    mm_acc(crust, L);
    SetPtr((N-1)-M, 0, Reduce(crust));
  } else {
    SetPtr((N-1)-M, 0, L);
  }
  SetPtr(N-2, 0, cache_mult(L, L));
  SetPtr(N-1, 0, cache_mult(U, L));

  initialized = true;
}

void shared_matrix::ColCpy(int dest, int src)
{
  int k;
  for (k=0; k<N; k++) 
    SetPtr(k, dest, ptrs[k][src]);
}

void shared_matrix::show(OutputStream &s)
{
  int i,j;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j])
    ptrs[i][j]->flag = 0;

  // number the submatrices (and dump them)
  int subcnt = 1;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j])
    if (0==ptrs[i][j]->flag) {
      ptrs[i][j]->flag = subcnt++;
      ptrs[i][j]->show(s);
    }
  
  for (i=0; i<N; i++) {
    s << "[";
    for (j=0; j<N; j++) {
      if (j) s << ", ";
      if (ptrs[i][j]) 
	s << ptrs[i][j]->flag;
      else
	s << '0';
    }
    s << "]\n";
    s.flush();
  }
}

void shared_matrix::write(OutputStream &s)
{
  int i,j;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j])
    ptrs[i][j]->flag = 0;

  int subcnt = 1;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j])
    if (0==ptrs[i][j]->flag) ptrs[i][j]->flag = subcnt++;

  s << subcnt << "\n";

  subcnt = 1;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j])
    if (subcnt == ptrs[i][j]->flag) {
      subcnt++;
      ptrs[i][j]->write(s);
      s << "\n";
      s.flush();
    }

  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      if (ptrs[i][j]) s << ptrs[i][j]->flag;
      else s << '0';
      s.Pad(1);
    } // for j
    s << "\n";
    s.flush();
  }
}

void shared_matrix::read(InputStream &s)
{
  int subcnt;
  s.Get(subcnt);
  bitmatrix** map = new bitmatrix*[subcnt];
  map[0] = NULL; // nice trick
  int i;
  for (i=1; i<subcnt; i++) {
    map[i] = matrix_pile.NewObject();
    bool ok = map[i]->read(s);
    if (!ok) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Error reading file\n";
      Internal.Stop();
      return;
    }
    map[i] = Reduce(map[i]);
    map[i]->cachecount = 1;  // prevent this from being removed
  }
  int j;
  for (i=0; i<N; i++) for (j=0; j<N; j++) ptrs[i][j] = NULL;

  for (i=0; i<N; i++) for (j=0; j<N; j++) {
    int which;
    s.Get(which);
    SetPtr(i, j, map[which]);
  }
 
  // allow removal now
  for (i=1; i<subcnt; i++) map[i]->cachecount = 0;
  
  delete[] map;

  initialized = true;
}

int shared_matrix::Distinct()
{
  int d = 0;
  int i,j;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j])
    ptrs[i][j]->flag = 0;
  for (i=0; i<N; i++) for (j=0; j<N; j++) if (ptrs[i][j]) {
    if (ptrs[i][j]->flag) continue;
    d++;
    ptrs[i][j]->flag = 1;
  }
  return d;
}

int shared_matrix::Multiply(shared_matrix *b, shared_matrix *c)
{
  int i,j,k;
  if (!initialized) {
    for (i=0; i<N; i++) for (j=0; j<N; j++) ptrs[i][j] = NULL;
    initialized = true;
  }
  int nnz = 0;
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      bitmatrix* acc = matrix_pile.NewObject();
      acc->zero();
      for (k=0; k<N; k++) {
	bitmatrix* term = cache_mult(b->ptrs[i][k], c->ptrs[k][j]);
	if (term) mm_acc(acc, term);
      } // for k
      acc = Reduce(acc);
      SetPtr(i, j, acc);
      if (acc) nnz++;
    } // for j
  } // for i
  return nnz;
}  

bool shared_matrix::CheckMultiply(shared_matrix *b, shared_matrix *c)
{
  int nnz = 0;
  int i,j,k;
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      bitmatrix* acc = matrix_pile.NewObject();
      acc->zero();
      for (k=0; k<N; k++) {
	bitmatrix* term = cache_mult(b->ptrs[i][k], c->ptrs[k][j]);
	if (term) mm_acc(acc, term);
      } // for k
      acc = Reduce(acc);
      if (ptrs[i][j] != acc) {
	return false;
      }
    } // for j
  } // for i
  return true;
}  

int shared_matrix::CheckShift(shared_matrix *b)
{
  int cnt = 0;
  int i,j;
  for (i=0; i<N; i++) for (j=1; j<N; j++)
    if (b->ptrs[i][j-1] != ptrs[i][j]) 
      cnt++;
  return cnt;
}

// ------------------------------------------------------------------
//    Good old memory-hoggin' explicit
// ------------------------------------------------------------------

fullmatrix::fullmatrix(int n)
{
  N = n;
  ptrs = new bitmatrix**[N];
  int i,j;
  for (i=0; i<N; i++) {
    ptrs[i] = new bitmatrix*[N];
  }
}

fullmatrix::~fullmatrix()
{
  int i;
  for (i=0; i<N; i++) delete[] ptrs[i];
  delete[] ptrs;
}

void fullmatrix::show(OutputStream &s)
{
  int i,j;
  for (i=0; i<N; i++) {
    s << "[";
    for (j=0; j<N; j++) {
      if (j) s << ", ";
      if (ptrs[i][j]) {
        ptrs[i][j]->flag = false;
	if (IDENTITY == ptrs[i][j]) s.Put('I');
	else s.PutHex((unsigned int)ptrs[i][j]);
      } else {
	s << "0";
      }
    }
    s << "]\n";
    s.flush();
  }
  // dump the submatrices
  for (i=0; i<N; i++) for (j=0; j<N; j++) {
    if (NULL == ptrs[i][j]) continue;
    if (IDENTITY == ptrs[i][j]) continue;
    if (ptrs[i][j]->flag) continue;
    ptrs[i][j]->show(s);
    ptrs[i][j]->flag = true;
  }
}

int fullmatrix::Multiply(fullmatrix *b, fullmatrix *c)
{
  int nnz = 0;
  int i,j,k;
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      if (NULL == ptrs[i][j])
	ptrs[i][j] = matrix_pile.NewObject();
      ptrs[i][j]->zero();
      for (k=0; k<N; k++) {
	bitmatrix* term = nocache_mult(b->ptrs[i][k], c->ptrs[k][j]);
	if (term) {
	  mm_acc(ptrs[i][j], term);
	  matrix_pile.FreeObject(term);
	}
      } // for k
      if (ptrs[i][j]->is_zero()) {
	matrix_pile.FreeObject(ptrs[i][j]);
	ptrs[i][j] = NULL;
      } else nnz++;
    } // for j
  } // for i
  return nnz;
}  

void fullmatrix::FillFrom(const shared_matrix &a)
{
  int i,j;
  for (i=0; i<N; i++) for (j=0; j<N; j++) {
    if (a.ptrs[i][j]) {
      ptrs[i][j] = matrix_pile.NewObject();
      ptrs[i][j]->FillFrom(a.ptrs[i][j]);
    } else {
      ptrs[i][j] = NULL;
    }
  }
}

// ------------------------------------------------------------------

void InitMatrix()
{
  if (IDENTITY) return; // already initialized

  IDENTITY = matrix_pile.NewObject();
  int i;
  for (i=0; i<32; i++) IDENTITY->row[i] = mask[i]; 
  IDENTITY = Reduce(IDENTITY);
  IDENTITY->ptrcount = 1;

  Output << "Initialized identity\n";
}

void MatrixStats()
{
  Output << "\tSubmatrices: \t";
  Output << matrix_pile.ActiveObjects() << " active, ";
  Output << matrix_pile.PeakObjects() << " peak\n";

  Output << "\tHash tables: \t";
  Output << hash_node_pile.ActiveObjects() << " active, ";
  Output << hash_node_pile.PeakObjects() << " peak\n";

  Output << "\tUnique table: \t";
  Output << UniqueTable.Size() << " size, ";
  Output << UniqueTable.Entries() << " entries, ";
  Output << UniqueTable.MaxChain() << " maxchain\n";

#ifndef NO_CACHE
  Output << "\tCompute table: \t";
  Output << ComputeTable.Size() << " size, ";
  Output << ComputeTable.Entries() << " entries, ";
  Output << ComputeTable.MaxChain() << " maxchain\n";
  Output << "\t               \t";
  Output << cachehits << " hits / " << cachetries << " pings\n";
  cachehits = cachetries = 0;
#endif

  Output.flush();
}

void GarbageCollect()
{
  ComputeTable.Resize();
  UniqueTable.Resize();
/*
  Output << "Unique Table:\n";
  UniqueTable.Dump();

  Output << "Compute Table:\n";
  ComputeTable.Dump();
  */
}

