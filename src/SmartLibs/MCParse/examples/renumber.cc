
// $Id$

// A utility to renumber states in a Markov chain.
// Note that two or more states can map to the same new state,
// and the edges are summed; this allows for lumping.

#include <stdio.h>
#include <stdlib.h>

#include "mcparse.h"
#include "graphlib.h"

// #define DEBUG_MAP_PARSE

// ******************************************************************
// *                    Handy renumbering class                     *
// ******************************************************************

class renumberer {
  bool is_identity;
  long* data;
  long alloc;
  long srcmax;
  long targmax;
  long* invdata;
public:
  renumberer();
  ~renumberer();

  inline bool isIdentity() const { return is_identity; }
  void setIdentity();
  void setIdentitySize(long ns);

  inline long operator[](long i) const { 
    return data ? data[i] : i;
  }

  inline void addPair(long src, long target) {
    if (src>=alloc) enlarge(src);
    if (src>srcmax) srcmax = src;
    if (target>targmax) targmax = target;
    if (data[src]>=0) {
      fprintf(stderr, "Duplicate mapping for source state %ld\n", src);
      exit(2);
    }
    data[src] = target;
#ifdef DEBUG_MAP_PARSE
    fprintf(stderr, "added %ld --> %ld\n", src, target);
#endif
  }

  inline long firstToTarget(long t) const { 
    return invdata ? invdata[t] : t;
  }

  inline long getSrcSize() const { return srcmax+1; }
  inline long getTargetSize() const { return targmax+1; }

  void buildInverse();

  

protected:
  void enlarge(long maxind);
};

renumberer::renumberer()
{
  alloc = 16;
  data = (long*) malloc(alloc*sizeof(long));
  for (int i=0; i<alloc; i++) data[i] = -1;
  srcmax = -1;
  targmax = -1;
  invdata = 0;
  is_identity = false;
}

renumberer::~renumberer()
{
  free(data);
  free(invdata);
}

void renumberer::setIdentity()
{
  free(data);
  data = 0;
  is_identity = true;
}

void renumberer::setIdentitySize(long ns)
{
  srcmax = ns-1;
  targmax = ns-1;
}

void renumberer::enlarge(long maxind)
{
  long newalloc = alloc;
  if (0==newalloc) newalloc = 16;
  while (newalloc <= maxind) {
    if (newalloc < 4096)  newalloc += 4096;
    else                  newalloc *= 2;
  }
  long* newdata = (long*) realloc(data, newalloc*sizeof(long));
  if (0==newdata) {
    fprintf(stderr, "Couldn't expand mapping array (new size: %ld)\n", newalloc);
    exit(1);
  }
  for (int i=alloc; i<newalloc; i++) newdata[i] = -1;
  data = newdata;
  alloc = newalloc;
}

void renumberer::buildInverse()
{
  if (is_identity) return;
  // check for gaps
  for (long i=0; i<=srcmax; i++) {
    if (data[i]>=0) continue;
    fprintf(stderr, "Error: no mapping for source state %ld\n", i);
    exit(2);
  }
  invdata = (long*) malloc(getTargetSize()*sizeof(long));
  if (0==invdata) {
    fprintf(stderr, "Couldn't build inverse array\n");
    exit(1);
  }
  for (long i=targmax; i>=0; i--) invdata[i] = -1;
  for (long i=srcmax; i>=0; i--) {
    invdata[data[i]] = i; 
  }
  for (long i=0; i<=targmax; i++) {
    if (invdata[i]>=0) continue;
    fprintf(stderr, "Error: nothing maps to target state %ld\n", i);
    exit(2);
  }
}

// ******************************************************************
// *                   Handy sparse vector class                    *
// ******************************************************************

class sparse_vect {
  long* indexes;
  double* values;
  long size;
  long alloc;
public:
  sparse_vect();
  ~sparse_vect();

  void addElement(long i, double v);

  inline void clear() { size = 0; }

  bool closeEnough(const sparse_vect &x) const;

  void printAsRow(FILE* f, long row) const;
};

sparse_vect::sparse_vect()
{
  alloc = 16;
  indexes = (long*) malloc(alloc*sizeof(long));
  values = (double*) malloc(alloc*sizeof(double));
  size = 0;
}

sparse_vect::~sparse_vect()
{
  free(indexes);
  free(values);
}

void sparse_vect::addElement(long i, double v)
{
  if (size == alloc) {
    long newalloc = (alloc < 1024) ? alloc * 2 : alloc + 1024;
    indexes = (long*) realloc(indexes, newalloc * sizeof(long));
    values = (double*) realloc(values, newalloc * sizeof(double));
    if (indexes==0 || values==0) {
      fprintf(stderr, "Couldn't enlarge sparse vector (size=%ld)\n", newalloc);
      exit(1);
    }
    alloc = newalloc;
  }
  indexes[size] = i;
  values[size] = v;
  size++;
}

bool sparse_vect::closeEnough(const sparse_vect &x) const
{
  if (size != x.size) return false;
  for (long z = 0; z<size; z++) {
    if (indexes[z] != x.indexes[z]) return false;
    double diff = values[z] - x.values[z];
    if (0==diff) continue;
    if (values[z]) diff /= values[z]; else diff /= x.values[z];
    if (diff > 1e-6) return false;
  }
  return true;
}

void sparse_vect::printAsRow(FILE* f, long row) const
{
  for (long z=0; z<size; z++) {
    fprintf(f, "  %ld : %ld : %lg\n", row, indexes[z], values[z]);
  }
}

// ******************************************************************
// *                     MC  renumbering class                      *
// ******************************************************************

class mc_renumber : public mc_builder {
    class grab_row : public GraphLib::generic_graph::element_visitor {
        sparse_vect &x;
      public:
        grab_row(sparse_vect &row) : x(row) { }
        virtual bool visit(long from, long to, void* wt) {
          double r = ((double*) wt)[0];
          x.addElement(to, r);
          return false;
        }
    };
    bool is_discrete;
    double* vect;
    GraphLib::merged_weighted_digraph <double> *edges;
    renumberer &map;
    bool mid_transient_list;
    bool mid_absorbing_list;
  public:
    mc_renumber(FILE* err, renumberer &m);
    virtual ~mc_renumber();

  protected:
    virtual bool startDTMC(const char* name);
    virtual bool startCTMC(const char* name);

    virtual bool specifyStates(long ns);

    virtual bool startInitial();
    virtual bool addInitial(long state, double weight);
    virtual bool doneInitial();

    virtual bool startEdges(long num_edges = -1);
    virtual bool addEdge(long from, long to, double wt);
    virtual bool doneEdges();

    virtual bool startMeasureCollection(solution_type which, double time);
    virtual bool startMeasureCollection(solution_type which, int time);
    virtual bool startMeasure(const char* name);
    virtual bool addToMeasure(long state, double value);
    virtual bool doneMeasure();
    virtual bool doneMeasureCollection();

    /// Check number of recurrent classes of size greater than one.
    virtual bool assertClasses(long nc);

    virtual bool assertAbsorbing(long st);
    virtual bool assertTransient(long s);

    virtual bool startRecurrentAssertion();
    virtual bool assertRecurrent(long s);
    virtual bool doneRecurrentAssertion();

  private:
    void clearVect() {
      for (long i=map.getTargetSize()-1; i>=0; i--) vect[i] = 0;
    }
    void printVect(FILE* out, int tabdepth) const {
      for (long i=0; i<map.getTargetSize(); i++) if (vect[i]) {
        printf("%*s%ld : %lg\n", tabdepth, "", i, vect[i]);
      }
    }
};

mc_renumber::mc_renumber(FILE* err, renumberer &m)
 : mc_builder(err), map(m)
{
  vect = 0;
  edges = 0;
  mid_transient_list = 0;
  mid_absorbing_list = 0;
}

mc_renumber::~mc_renumber()
{
  delete[] vect;
  delete edges;
}

bool mc_renumber::startDTMC(const char* name)
{
  is_discrete = true;
  if (name) printf("DTMC %s\n", name);
  else      printf("DTMC\n");
  return true;
}

bool mc_renumber::startCTMC(const char* name)
{
  is_discrete = false;
  if (name) printf("CTMC %s\n", name);
  else      printf("CTMC\n");
  return true;
}

bool mc_renumber::specifyStates(long ns)
{
  if (map.isIdentity()) {
    map.setIdentitySize(ns);
  }
  if (ns != map.getSrcSize()) {
    startError();
    fprintf(errlog, "Wrong number of states (mapping has %ld)", map.getSrcSize());
    doneError();
    return false;
  }
  printf("STATES %ld\n", map.getTargetSize());
  return true;
}

bool mc_renumber::startInitial()
{
  vect = new double[map.getTargetSize()];
  clearVect();
  return true;
}

bool mc_renumber::addInitial(long state, double weight)
{
  vect[map[state]] += weight;
  return true;
}

bool mc_renumber::doneInitial()
{
  printf("INIT\n");
  printVect(stdout, 2);
  return true;
}


bool mc_renumber::startEdges(long num_edges)
{
  // NOTE: edges rows are according to src, and columns according to target.
  edges = new GraphLib::merged_weighted_digraph <double> (true);
  
  try {
    edges->addNodes(map.getSrcSize());
    return true;
  }
  catch (GraphLib::error e) {
    fprintf(stderr, "Couldn't create edges: %s\n", e.getString());
    return false;
  }
}

bool mc_renumber::addEdge(long from, long to, double wt)
{
  try {
    edges->addEdge(from, map[to], wt);
    return true;
  }
  catch (GraphLib::error e) {
    fprintf(stderr, "Couldn't add edge: %s\n", e.getString());
    return false;
  }
}

bool mc_renumber::doneEdges()
{
  try {
    GraphLib::generic_graph::finish_options fo;
    edges->finish(fo);
  }
  catch (GraphLib::error e) {
    fprintf(stderr, "Couldn't finish edges: %s\n", e.getString());
    return false;
  }
  sparse_vect x, y;
  grab_row rx(x);
  grab_row ry(y);
  // check lumpability : rows of lumped states must be equal.
  for (long s=0; s<map.getSrcSize(); s++) {
    long srep = map.firstToTarget(map[s]);
    if (srep == s) continue;

    x.clear();
    y.clear();
    edges->traverseFrom(srep, rx);
    edges->traverseFrom(s, ry);

    if (x.closeEnough(y)) continue;
    
    fprintf(stderr, "Error, Markov chain is NOT lumpable w.r.t. given mapping:\n");
    fprintf(stderr, "Lumped states %ld and %ld have different outgoing edges\n", s, srep);
    fprintf(stderr, "Outgoing from %ld:\n", s);
    y.printAsRow(stderr, s);
    fprintf(stderr, "Outgoing from %ld:\n", srep); 
    x.printAsRow(stderr, srep);
    return false;
  } // for s


  printf("ARCS\n");
  for (long t=0; t<map.getTargetSize(); t++) {
    x.clear();
    edges->traverseFrom(map.firstToTarget(t), rx);
    x.printAsRow(stdout, t);
  }
  printf("END\n");
  delete edges;
  edges = 0;
  return true;
}


bool mc_renumber::startMeasureCollection(solution_type which, double time)
{
  mid_transient_list = false;
  mid_absorbing_list = false;
  switch (which) {
    case Steady_state:
        printf("STEADY\n");
        return true;

    case Accumulated:
        printf("ACCUMULATED\n");
        return true;

    case Transient:
        printf("TIME %lg\n", time);
        return true;

    default:
        fprintf(stderr, "Strange solution type\n");
        return false;
  }
  return false;
}

bool mc_renumber::startMeasureCollection(solution_type which, int time)
{
  switch (which) {
    case Steady_state:
        printf("STEADY\n");
        return true;

    case Accumulated:
        printf("ACCUMULATED\n");
        return true;

    case Transient:
        printf("TIME %d\n", time);
        return true;

    default:
        fprintf(stderr, "Strange solution type\n");
        return false;
  }
  return false;
}

bool mc_renumber::startMeasure(const char* name)
{
  printf("  %s\n", name);
  clearVect();
  return true;
}

bool mc_renumber::addToMeasure(long state, double value)
{
  vect[map[state]] += value;
  return true;
}

bool mc_renumber::doneMeasure()
{
  printVect(stdout, 4);
  return true;
}

bool mc_renumber::doneMeasureCollection()
{
  return true;
}


bool mc_renumber::assertClasses(long nc)
{
  printf("ASSERT CLASSES %ld\n", nc);
  return true;
}


bool mc_renumber::assertAbsorbing(long st)
{
  if (!mid_absorbing_list) {
    printf("ASSERT ABSORBING\n");
    mid_absorbing_list = true;
    mid_transient_list = false;
  }
  printf("  %ld\n", map[st]);
  return true;
}

bool mc_renumber::assertTransient(long s)
{
  if (!mid_transient_list) {
    printf("ASSERT ABSORBING\n");
    mid_transient_list = true;
    mid_absorbing_list = false;
  }
  printf("  %ld\n", map[s]);
  return true;
}


bool mc_renumber::startRecurrentAssertion()
{
  mid_transient_list = false;
  mid_absorbing_list = false;
  printf("ASSERT RECURRENT\n");
  return true;
}

bool mc_renumber::assertRecurrent(long s)
{
  printf("  %ld\n", map[s]);
  return true;
}

bool mc_renumber::doneRecurrentAssertion()
{
  return true;
}


// ******************************************************************
// *                           Front end                            *
// ******************************************************************

long nextNumber(FILE* in)
{
  int c;
  long answer = 0;
  for (;;) {
    c = fgetc(in);
    if (c>='0' && c<='9') break;
    if (feof(in)) return -1;
  }
  for (;;) {
    answer *= 10;
    answer += (c - '0');
    c = fgetc(in);
    if (c<'0' || c>'9') break;
  }
  return answer;
}

bool grabArrow(FILE* in)
{
  int c;
  for (;;) {
    c = fgetc(in);
    if (feof(in)) return false;
    if ('-'==c) break;
    if ('='==c) break;
    if ('>'==c) return true;
  }
  for (;;) {
    c = fgetc(in);
    if (feof(in)) return false;
    if ('-'==c) continue;
    if ('='==c) continue;
    if ('>'==c) return true;
    return false;
  }
}

int parseMapfile(const char* filename, renumberer &r)
{
  FILE* mapf = fopen(filename, "r");
  if (0==mapf) {
    fprintf(stderr, "Couldn't open file %s\n", filename);
    return 4;
  }
  fprintf(stderr, "Reading map file %s\n", filename);
    
  for (;;) {

    long src = nextNumber(mapf);
    if (src<0) break;

    if (!grabArrow(mapf)) {
      fprintf(stderr, "Parse error in map file: expecting ->\n");
      return 5;
    }
    long target = nextNumber(mapf);
    if (target<0) {
      fprintf(stderr, "Parse error in map file: expecting target state\n");
      return 5;
    }

    r.addPair(src, target);
  } // infinite loop to read file

  fclose(mapf);
  return 0;
}

int Usage(const char* name)
{
  fprintf(stderr, "\nMarkov chain state renumbering tool\n");
  fprintf(stderr, "\t%s\n", GraphLib::Version());
  fprintf(stderr, "\t%s\n", mc_builder::getParserVersion());
  fprintf(stderr, "\nUsage: %s <mapfile>\n\n", name);
  fprintf(stderr, "\tThe mapfile is a text file with entries\n\n");
  fprintf(stderr, "\t\t src --> target    or    src ==> target\n\n");
  fprintf(stderr, "\tindicating how source states from the input chain (read\n");
  fprintf(stderr, "\tfrom standard input) are mapped to the output chain\n");
  fprintf(stderr, "\t(written to standard output).  If multiple input states\n");
  fprintf(stderr, "\tare mapped to a single output state, the states will be\n");
  fprintf(stderr, "\tlumped if possible (requires strong lumpability).\n\n");
  fprintf(stderr, "\tIf no mapfile is specified, states are not renumbered.\n\n");
  return 0;
}

int main(int argc, const char** argv)
{
  if (argc>2) return Usage(argv[0]);

  renumberer map;

  if (argc==2) {
    int code = parseMapfile(argv[1], map);
    if (code) return code;
    map.buildInverse();
    if (map.getSrcSize() == map.getTargetSize()) {
      fprintf(stderr, "Renumbering Markov chain\n");
    }
    if (map.getSrcSize() > map.getTargetSize()) {
      fprintf(stderr, "Lumping Markov chain\n");
    }
  } else {
    map.setIdentity();
    fprintf(stderr, "Rewriting Markov chain\n");
  }

  mc_renumber parser(stderr, map);
  if (!parser.parse_file(stdin, 0)) return 6;
  if (!parser.done_parsing()) return 6;

  return 0;
}

