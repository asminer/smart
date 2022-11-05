
#include "expl_states.h"
#include "../Options/options.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/mod_inst.h"
#include "../include/heap.h"

// External libs
#include "../_StateLib/statelib.h"

// **************************************************************************
// *                         substate_colls methods                         *
// **************************************************************************

substate_colls::substate_colls(int K) : shared_object()
{
  num_levels = K;
  is_static = false;
}

substate_colls::~substate_colls()
{
}

bool substate_colls::Print(OutputStream &, int) const
{
  DCASSERT(0);
  return false;
}

bool substate_colls::Equals(const shared_object* o) const
{
  // lazy
  return (o==this);
}

// **************************************************************************
// *                                                                        *
// *                          separate_colls class                          *
// *                                                                        *
// **************************************************************************

class separate_colls : public substate_colls {
  StateLib::state_db** locals;
public:
  separate_colls(int K, StateLib::state_db** dbs);
protected:
  virtual ~separate_colls();
public:
  virtual long findSubstate(int k, const int* state, int size);
  virtual long addSubstate(int k, const int* state, int size);
  virtual int getSubstate(int k, long i, int* state, int size) const;
  virtual long getMaxIndex(int k) const;
  virtual void convertToStatic();
  virtual void Report(OutputStream &) const;
};

// **************************************************************************
// *                         separate_colls methods                         *
// **************************************************************************

separate_colls::separate_colls(int K, StateLib::state_db** dbs)
 : substate_colls(K)
{
  locals = dbs;
#ifdef DEVELOPMENT_CODE
  for (int i=0; i<=K; i++) {
    if (dbs[i]) DCASSERT(dbs[i]->CollectionUsesIndexes());
  }
#endif
}

separate_colls::~separate_colls()
{
  for (int k=0; k<=num_levels; k++)
    delete locals[k];
  delete[] locals;
}

long separate_colls::findSubstate(int k, const int* state, int size)
{
  if (is_static) return -5;
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(locals);
  DCASSERT(locals[k]);
  return locals[k]->FindState(state, size);
}

long separate_colls::addSubstate(int k, const int* state, int size)
{
  if (is_static) return -5;
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(locals);
  DCASSERT(locals[k]);
  return locals[k]->InsertState(state, size);
}

int separate_colls::getSubstate(int k, long i, int* state, int size) const
{
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(locals);
  DCASSERT(locals[k]);
  return locals[k]->GetStateKnown(i, state, size);
}

long separate_colls::getMaxIndex(int k) const
{
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(locals);
  DCASSERT(locals[k]);
  return locals[k]->Size();
}

void separate_colls::convertToStatic()
{
  if (is_static) return;
  DCASSERT(locals);
  for (int k=1; k<=num_levels; k++) {
    DCASSERT(locals[k]);
    locals[k]->ConvertToStatic(true);
  }
}

void separate_colls::Report(OutputStream &s) const
{
  size_t total = sizeof(StateLib::state_coll*) * num_levels;
  for (int k=1; k<=num_levels; k++) {
    total += locals[k]->ReportMemTotal();
  }
  s << "Separated " << num_levels << " substate collections require ";
  s.PutMemoryCount(total, 2);
  s.Put('\n');
}


// **************************************************************************
// *                                                                        *
// *                        synchronized_colls class                        *
// *                                                                        *
// **************************************************************************

class synchronized_colls : public substate_colls {
  StateLib::state_db* common;
public:
  synchronized_colls(int K, StateLib::state_db* db);
protected:
  virtual ~synchronized_colls();
public:
  virtual long findSubstate(int k, const int* state, int size);
  virtual long addSubstate(int k, const int* state, int size);
  virtual int getSubstate(int k, long i, int* state, int size) const;
  virtual long getMaxIndex(int k) const;
  virtual void convertToStatic();
  virtual void Report(OutputStream &) const;
};

// **************************************************************************
// *                       synchronized_colls methods                       *
// **************************************************************************

synchronized_colls::synchronized_colls(int K, StateLib::state_db* db)
 : substate_colls(K)
{
  DCASSERT(db->CollectionUsesIndexes());
  common = db;
}

synchronized_colls::~synchronized_colls()
{
  delete common;
}

long synchronized_colls::findSubstate(int k, const int* state, int size)
{
  if (is_static) return -5;
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(common);
  return common->FindState(state, size);
}

long synchronized_colls::addSubstate(int k, const int* state, int size)
{
  if (is_static) return -5;
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(common);
  return common->InsertState(state, size);
}

int synchronized_colls::getSubstate(int k, long i, int* state, int size) const
{
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(common);
  return common->GetStateKnown(i, state, size);
}

long synchronized_colls::getMaxIndex(int k) const
{
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(common);
  return common->Size();
}

void synchronized_colls::convertToStatic()
{
  if (is_static) return;
  DCASSERT(common);
  common->ConvertToStatic(true);
}

void synchronized_colls::Report(OutputStream &s) const
{
  size_t total = common->ReportMemTotal();
  s << "Synchronized substate collection requires ";
  s.PutMemoryCount(total, 2);
  s.Put('\n');
}



// **************************************************************************
// *                                                                        *
// *                          unsynch_colls  class                          *
// *                                                                        *
// **************************************************************************

class unsynch_colls : public substate_colls {
  StateLib::state_db* common;
  // local index to submarking conversions
  int* i2s_size;
  int* i2s_alloc;
  long** i2s;
  // submarking to local index conversions
  int* s2i_alloc;
  int** s2i;
public:
  unsynch_colls(int K, StateLib::state_db* db);
protected:
  virtual ~unsynch_colls();
public:
  virtual long findSubstate(int k, const int* state, int size);
  virtual long addSubstate(int k, const int* state, int size);
  virtual int getSubstate(int k, long i, int* state, int size) const;
  virtual long getMaxIndex(int k) const;
  virtual void convertToStatic();
  virtual void Report(OutputStream &) const;
};

// **************************************************************************
// *                         unsynch_colls  methods                         *
// **************************************************************************

unsynch_colls::unsynch_colls(int K, StateLib::state_db* db) : substate_colls(K)
{
  DCASSERT(db->CollectionUsesIndexes());
  common = db;
  i2s_size = new int[K+1];
  i2s_alloc = new int[K+1];
  i2s = new long*[K+1];
  s2i_alloc = new int[K+1];
  s2i = new int*[K+1];
  for (int k=0; k<=K; k++) {
    i2s_size[k] = 0;
    i2s_alloc[k] = 0;
    i2s[k] = 0;
    s2i_alloc[k] = 0;
    s2i[k] = 0;
  }
}

unsynch_colls::~unsynch_colls()
{
  for (int k=0; k<=num_levels; k++) {
    free(i2s[k]);
    free(s2i[k]);
  }
  delete[] i2s_size;
  delete[] i2s_alloc;
  delete[] i2s;
  delete[] s2i_alloc;
  delete[] s2i;
  delete common;
}

long unsynch_colls::findSubstate(int k, const int* state, int size)
{
  if (is_static) return -5;
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(common);
  long h = common->FindState(state, size);
  if (h<0) return h;
  if (h>=s2i_alloc[k]) return -1;
  return s2i[k][h];
}

long unsynch_colls::addSubstate(int k, const int* state, int size)
{
  if (is_static) return -5;
  CHECK_RANGE(1, k, num_levels+1);
  DCASSERT(common);
  long h = common->InsertState(state, size);
  if (h<0) return h;
  if (h >= s2i_alloc[k]) {
    // enlarge s2i[k]
    int nalloc = common->Size();
    DCASSERT(h < nalloc);
    int* ns2i = (int*) realloc(s2i[k], sizeof(int)*nalloc);
    if (0==ns2i) return -2;
    for (int i=s2i_alloc[k]; i<nalloc; i++) ns2i[i] = -1;
    s2i[k] = ns2i;
    s2i_alloc[k] = nalloc;
  }
  CHECK_RANGE(0, h, s2i_alloc[k]);
  DCASSERT(s2i[k]);
  if (s2i[k][h] >= 0) return s2i[k][h];
  // not yet in local list, add it
  if (i2s_size[k] >= i2s_alloc[k]) {
    // enlarge i2s[k]
    int delta = MIN(256, MAX(i2s_alloc[k], 4)); // delta between 4 and 256
    int nalloc = i2s_alloc[k] + delta;
    long* ni2s = (long*) realloc(i2s[k], sizeof(long)*nalloc);
    if (0==ni2s) return -2;
    i2s[k] = ni2s;
    i2s_alloc[k] = nalloc;
  }
  CHECK_RANGE(0, i2s_size[k], i2s_alloc[k]);
  DCASSERT(i2s[k]);
  i2s[k][i2s_size[k]] = h;
  s2i[k][h] = i2s_size[k];
  return i2s_size[k]++;
}

int unsynch_colls::getSubstate(int k, long i, int* state, int size) const
{
  CHECK_RANGE(1, k, num_levels+1);
  CHECK_RANGE(0, i, i2s_size[k]);
  DCASSERT(i2s);
  long h = i2s[k][i];
  DCASSERT(common);
  return common->GetStateKnown(h, state, size);
}

long unsynch_colls::getMaxIndex(int k) const
{
  CHECK_RANGE(1, k, num_levels+1);
  return i2s_size[k];
}

void unsynch_colls::convertToStatic()
{
  if (is_static) return;
  DCASSERT(common);
  common->ConvertToStatic(true);
}

void unsynch_colls::Report(OutputStream &s) const
{
  size_t total = common->ReportMemTotal();
  for (int k=1; k<=num_levels; k++) {
    total += i2s_alloc[k]*sizeof(long) + s2i_alloc[k]*sizeof(int);
  }
  s << "Shared substate collection (and index maps) requires ";
  s.PutMemoryCount(total, 2);
  s.Put('\n');
}

// **************************************************************************
// *                         exp_state_lib  methods                         *
// **************************************************************************

exp_state_lib::exp_state_lib() : library(false, false)
{
}

// **************************************************************************
// *                                                                        *
// *                         my_exp_state_lib class                         *
// *                                                                        *
// **************************************************************************

class my_exp_state_lib : public exp_state_lib {
  static long max_stack_depth;
  unsigned storage;
  static const unsigned HASHING  = 0;
  static const unsigned RED_BLACK  = 1;
  static const unsigned SPLAY  = 2;
  // methods for substate dbs
  unsigned substate_style;
  static const unsigned SEPARATED = 0;
  static const unsigned SHARED = 1;
  static const unsigned SYNCHRONIZED = 2;
public:
  my_exp_state_lib(exprman* em);
  virtual const char* getVersionString() const;
  virtual bool hasFixedPointer() const;
  virtual const char* getDBMethod() const;
  virtual StateLib::state_db* createStateDB(bool indexed, bool store_sizes)
  const;

  virtual substate_colls* createSubstateDBs(int K, bool store_sizes) const;
};

long my_exp_state_lib::max_stack_depth;

// **************************************************************************
// *                        my_exp_state_lib methods                        *
// **************************************************************************

my_exp_state_lib::my_exp_state_lib(exprman* em) : exp_state_lib()
{
  radio_button** es_list = new radio_button*[3];
  es_list[HASHING] = new radio_button(
    "HASHING",
    "States are stored in a hash table.",
    HASHING
  );
  es_list[RED_BLACK] = new radio_button(
    "RED_BLACK",
    "States are stored in a red-black tree.",
    RED_BLACK
  );
  es_list[SPLAY] = new radio_button(
    "SPLAY",
    "States are stored in a splay tree.",
    SPLAY
  );
  storage = HASHING;    // Default.  Currently fastest.
  // storage = SPLAY;
  em->addOption(
    MakeRadioOption(
      "ExplicitStateStorage",
      "Data structure to use for explicitly storing states.",
      es_list, 3, storage
    )
  );

  int shift = sizeof(long)*8-2;
  max_stack_depth = 1L << shift;
  em->addOption(
    MakeIntOption("ExplicitStateStackLimit",
      "Maximum stack size to use for search trees for explicit state storage.",
      max_stack_depth, 1, max_stack_depth
    )
  );

  radio_button** ss_list = new radio_button*[3];
  ss_list[SEPARATED] = new radio_button(
    "SEPARATED",
    "Substates are stored in separate collections.",
    SEPARATED
  );
  ss_list[SHARED] = new radio_button(
    "SHARED",
    "Substates are stored in a shared collection, but substate indexes are different for each submodel.",
    SHARED
  );
  ss_list[SYNCHRONIZED] = new radio_button(
    "SYNCHRONIZED",
    "Substates are stored in a common collection, with the same indexes.",
    SYNCHRONIZED
  );
  substate_style = SHARED;
  em->addOption(
    MakeRadioOption(
      "SubstateStorageStyle",
      "For a model composed of submodels, how should the substates be stored.",
      ss_list, 3, substate_style
    )
  );
}

const char* my_exp_state_lib::getVersionString() const
{
  return StateLib::LibraryVersion();
}

bool my_exp_state_lib::hasFixedPointer() const
{
  return false;
}

const char* my_exp_state_lib::getDBMethod() const
{
  switch (storage) {
    case HASHING:     return "hash table";
    case RED_BLACK:   return "red-black tree";
    default:          return "splay tree";
  }
  return "keep dumb compilers happy";
}

StateLib::state_db*
my_exp_state_lib::createStateDB(bool indexed, bool store_sizes) const
{
  StateLib::state_db* sdb = 0;
  switch (storage) {
    case HASHING:
      sdb = StateLib::CreateStateDB(StateLib::SDBT_Hash, indexed, store_sizes);
      break;

    case RED_BLACK:
      sdb = StateLib::CreateStateDB(StateLib::SDBT_RedBlack, indexed,
        store_sizes);
      break;

    default:
      sdb = StateLib::CreateStateDB(StateLib::SDBT_Splay, indexed,
        store_sizes);
  }
  if (sdb) {
    sdb->SetMaximumStackSize(max_stack_depth);
  }
  return sdb;
}

substate_colls* my_exp_state_lib
::createSubstateDBs(int K, bool ss) const
{
  switch (substate_style) {
    case SEPARATED: {
        StateLib::state_db** dbs = new StateLib::state_db*[K+1];
        dbs[0] = 0;
        for (int k=1; k<=K; k++) dbs[k] = createStateDB(true, ss);
        return new separate_colls(K, dbs);
    }

    case SHARED:
        return new unsynch_colls(K, createStateDB(true, ss));

    case SYNCHRONIZED:
        return new synchronized_colls(K, createStateDB(true, ss));

    default:
        DCASSERT(0);
  }
  return 0;
}


// ******************************************************************
// *                                                                *
// *                       coll_sorter  class                       *
// *                                                                *
// ******************************************************************

class coll_sorter {
  const StateLib::state_coll* ss;
  long* map;
  // temps
  shared_state* full1;
  shared_state* full2;
public:
  coll_sorter(const hldsm* own, const StateLib::state_coll* sc, long* m);
  ~coll_sorter();
  // required for heapsort:
  inline int Compare(long i, long j) {
    ss->GetStateKnown(map[i], full1->writeState(), full1->getStateSize());
    ss->GetStateKnown(map[j], full2->writeState(), full2->getStateSize());
    return memcmp(full1->readState(), full2->readState(), full1->getStateSize() * sizeof(int));
  }
  inline void Swap(long i, long j) {
    CHECK_RANGE(0, i, ss->Size());
    CHECK_RANGE(0, j, ss->Size());
    long tmp = map[i];
    map[i] = map[j];
    map[j] = tmp;
  }
};

coll_sorter::coll_sorter(const hldsm* owner,
  const StateLib::state_coll* sc, long* m)
{
  DCASSERT(false == owner->containsListVar());

  ss = sc;
  map = m;

  full1 = new shared_state(owner);
  full2 = new shared_state(owner);

  // initialize map array
  long h = ss->FirstHandle();
  for (long i=0; i<ss->Size(); i++) {
    DCASSERT(h>=0);
    map[i] = h;
    h = ss->NextHandle(h);
  }
}

coll_sorter::~coll_sorter()
{
  Delete(full1);
  Delete(full2);
}

// ******************************************************************
// *                                                                *
// *                       coll_sorter2 class                       *
// *                                                                *
// ******************************************************************

class coll_sorter2 {
  const StateLib::state_coll* ss;
  const long* s2h;
  long* map;
  // temps
  shared_state* full1;
  shared_state* full2;
public:
  coll_sorter2(const hldsm* p, const StateLib::state_coll* sc,
    const long* s2h, long* m);
  ~coll_sorter2();
  // required for heapsort:
  inline int Compare(long i, long j) {
    ss->GetStateKnown(s2h[map[i]], full1->writeState(), full1->getStateSize());
    ss->GetStateKnown(s2h[map[j]], full2->writeState(), full2->getStateSize());
    return memcmp(full1->readState(), full2->readState(), full1->getStateSize() * sizeof(int));
  }
  inline void Swap(long i, long j) {
    CHECK_RANGE(0, i, ss->Size());
    CHECK_RANGE(0, j, ss->Size());
    long tmp = map[i];
    map[i] = map[j];
    map[j] = tmp;
  }
};

coll_sorter2
::coll_sorter2(const hldsm* p, const StateLib::state_coll* sc,
  const long* sh, long* m)
{
  DCASSERT(false == p->containsListVar());

  ss = sc;
  s2h = sh;
  map = m;

  full1 = new shared_state(p);
  full2 = new shared_state(p);

  // initialize map array
  for (long i=0; i<ss->Size(); i++) map[i] = i;
}

coll_sorter2::~coll_sorter2()
{
  Delete(full1);
  Delete(full2);
}

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

const exp_state_lib* InitExplicitStateStorage(exprman* em)
{
  static const exp_state_lib* foo = 0;
  if (!foo) {
    foo = new my_exp_state_lib(em);
    em->registerLibrary(foo);
  }
  return foo;
}

void LexicalSort(const hldsm* hm, const StateLib::state_coll* ss, long* map)
{
  DCASSERT(hm);
  DCASSERT(ss);
  DCASSERT(map);

  coll_sorter foo(hm, ss, map);
  HeapSortAbstract(&foo, ss->Size());
}

void LexicalSort(const hldsm* hm, const StateLib::state_coll* ss,
  const long* sh, long* m)
{
  DCASSERT(hm);
  DCASSERT(ss);
  DCASSERT(sh);
  DCASSERT(m);

  coll_sorter2 foo(hm, ss, sh, m);
  HeapSortAbstract(&foo, ss->Size());
}

