
#include "glue_meddly.h"
#include "biginttype.h"
#include "../Options/options.h"
#include "../ExprLib/startup.h"

// #define DEBUG_PLUS

// #define DEBUG_SHAREDEDGE

// #define DEBUG_ASSOC
// #define DEBUG_CARD 

//#define SHOW_CREATE_MINTERMS

// ******************************************************************
// *                                                                *
// *                      smart_output methods                      *
// *                                                                *
// ******************************************************************

smart_output::smart_output(OutputStream &DS) : output(), ds(DS)
{
}

smart_output::~smart_output()
{
}

void smart_output::put(char x)
{
  ds.Put(x);
}

void smart_output::put(const char* x, int w)
{
  ds.Put(x, w);
}

void smart_output::put(long x, int w)
{
  ds.Put(x, w);
}

void smart_output::put(unsigned long x, int w)
{
  ds.Put(x, w);
}

void smart_output::put_hex(unsigned long x, int w)
{
  ds.PutHex(x); // width?
}

void smart_output::put(double x, int w, int p, char f)
{
  OutputStream::real_format old_rf = ds.GetRealFormat();

  switch (f) {
    case 'e':
        ds.SetRealFormat(OutputStream::RF_SCIENTIFIC);
        break;

    case 'f':
        ds.SetRealFormat(OutputStream::RF_FIXED);
        break;

    default:
        ds.SetRealFormat(OutputStream::RF_GENERAL);
        break;
  };
  ds.Put(x, w, p); 

  ds.SetRealFormat(old_rf);
}

size_t smart_output::write(size_t bytes, const unsigned char* buffer)
{
  // hmm.
  for (int i=0; i<bytes; i++) {
    ds.Put(buffer[i]);
  }
  return bytes; // everything will work!  no worries, right?
}

void smart_output::flush()
{
  ds.flush();
}

// ******************************************************************
// *                                                                *
// *                     shared_domain  methods                     *
// *                                                                *
// ******************************************************************

shared_domain::shared_domain(MEDDLY::variable** v, int nv)
{
  D = MEDDLY::createDomain(v, nv);
}

shared_domain::shared_domain(MEDDLY::domain *d)
{
  D = d;
}

shared_domain::~shared_domain()
{
  MEDDLY::destroyDomain(D);
}

bool shared_domain::Print(OutputStream &s, int) const 
{ 
  s << "Meddly domain";
  return true;
}
  
bool shared_domain::Equals(const shared_object* x) const
{
  const shared_domain* foo = dynamic_cast <const shared_domain*> (x);
  if (0==foo) return false;
  return D == foo->D;
}


// ******************************************************************
// *                                                                *
// *                      mdd  library credits                      *
// *                                                                *
// ******************************************************************

class mdd_lib : public library {
  const char* version;
public:
  mdd_lib();
  virtual ~mdd_lib();
  virtual const char* getVersionString() const;
  virtual bool hasFixedPointer() const { return false; }
  virtual void printCopyright(doc_formatter* df) const;
  virtual void printReleaseDate(doc_formatter* df) const;
};

mdd_lib::mdd_lib() : library(true, true)
{
  version = MEDDLY::getLibraryInfo(0);
}

mdd_lib::~mdd_lib()
{
}

const char* mdd_lib::getVersionString() const
{
  return version;
}

void mdd_lib::printCopyright(doc_formatter* df) const
{
  df->begin_indent();
  df->Out() << MEDDLY::getLibraryInfo(1) << "\n";
  df->Out() << MEDDLY::getLibraryInfo(2) << "\n";
  df->Out() << MEDDLY::getLibraryInfo(3) << "\n";
  df->end_indent();
}

void mdd_lib::printReleaseDate(doc_formatter* df) const
{
  df->Out() << MEDDLY::getLibraryInfo(5);
}

mdd_lib mdd_lib_data;


// ******************************************************************
// *                                                                *
// *                      shared_ddedge  class                      *
// *                                                                *
// ******************************************************************

shared_ddedge::shared_ddedge(MEDDLY::forest* p)
: shared_object(), E(p) 
{ 
#ifdef DEBUG_SHAREDEDGE
  fprintf(stderr, "created shared_ddedge\n");
#endif
  iter = 0;
}

shared_ddedge::shared_ddedge(const shared_ddedge &e)
: shared_object(), E(e.E) 
{ 
#ifdef DEBUG_SHAREDEDGE
  fprintf(stderr, "created shared_ddedge\n");
#endif
  iter = 0;
}


shared_ddedge::~shared_ddedge()
{
#ifdef DEBUG_SHAREDEDGE
  fprintf(stderr, "destroyed shared_ddedge\n");
#endif
  delete iter;
}

bool shared_ddedge::Print(OutputStream &s, int) const 
{ 
  s << "Meddly edge: " << E.getNode();
  return true;
}
  
bool shared_ddedge::Equals(const shared_object* x) const
{
  const shared_ddedge* foo = dynamic_cast <const shared_ddedge*> (x);
  if (0==foo) return false;
  return E == foo->E;
}

void shared_ddedge::startIterator()
{
  using namespace MEDDLY;
  if (iter && iter->getType() != enumerator::FULL) {
    delete iter;
    iter = 0;
  }
  if (0==iter) {
    iter = new enumerator(enumerator::FULL, E.getForest());
  }
  iter->start(E);
}

void shared_ddedge::startIteratorRow(const int* rmt)
{
  using namespace MEDDLY;
  if (iter && iter->getType() != enumerator::ROW_FIXED) {
    delete iter;
    iter = 0;
  }

  if (0==iter) {
    iter = new enumerator(enumerator::ROW_FIXED, E.getForest());
  }
  iter->startFixedRow(E, rmt);
}

void shared_ddedge::startIteratorCol(const int* cmt)
{
  using namespace MEDDLY;
  if (iter && iter->getType() != enumerator::COL_FIXED) {
    delete iter;
    iter = 0;
  }

  if (0==iter) {
    iter = new enumerator(enumerator::COL_FIXED, E.getForest());
  }
  iter->startFixedColumn(E, cmt);
}

void shared_ddedge::freeIterator()
{
  delete iter;
  iter = 0;
}

// ******************************************************************
// *                                                                *
// *                    Helper  functions/macros                    *
// *                                                                *
// ******************************************************************

/*
inline void reportMeddlyError(const exprman* em, const expr* cause,
    const char* what, MEDDLY::error e)
{
  if (em->startError()) {
    em->causedBy(cause);
    if (what)  em->cerr() << "Error while " << what << ": ";
    em->cerr() << e.getName();
    em->stopIO();
  }
}
*/

// ******************************************************************
// *                                                                *
// *                     meddly_encoder methods                     *
// *                                                                *
// ******************************************************************

bool meddly_encoder::image_star_uses_saturation;

meddly_encoder::meddly_encoder(const char* n, MEDDLY::forest *f) : sv_encoder()
{
  F = f;
  name = n;
}

meddly_encoder::~meddly_encoder()
{
  MEDDLY::destroyForest(F);
}

void meddly_encoder::dumpNode(OutputStream &s, shared_object* e) const
{
  shared_ddedge* me = dynamic_cast <shared_ddedge*> (e);
  if (0==me) throw  Invalid_Edge;
  s.Put(me->E.getNode());
}

void meddly_encoder::showNodeGraph(OutputStream &s, shared_object* e) const
{
  shared_ddedge* me = dynamic_cast <shared_ddedge*> (e);
  if (0==me) throw  Invalid_Edge;
  smart_output sout(s);
  me->E.show(sout, 2);
}

void meddly_encoder::dumpForest(OutputStream &s) const
{
  DCASSERT(F);
  smart_output sout(s);
  F->showInfo(sout, 1);
}

shared_object* meddly_encoder::makeEdge(const shared_object* e)
{
  DCASSERT(F);
  if (e)  {
    const shared_ddedge* sde = smart_cast <const shared_ddedge*> (e);
    DCASSERT(sde);
    return new shared_ddedge(*sde);
  } else {
    return new shared_ddedge(F);
  }
}

bool meddly_encoder::isValidEdge(const shared_object* x) const
{
  return (0 != dynamic_cast<const shared_ddedge*> (x) );
}

void meddly_encoder::copyEdge(const shared_object* src, shared_object* dest) const
{
  const shared_ddedge* source = dynamic_cast<const shared_ddedge*> (src);
  if (0==source) throw Invalid_Edge;
  shared_ddedge* destination = dynamic_cast<shared_ddedge*> (dest);
  if (0==destination) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (destination->numRefs()>1) throw Shared_Output_Edge;
#endif
  DCASSERT(destination);
  destination->E = source->E;
}

void meddly_encoder::buildSymbolicConst(bool t, shared_object* ans)
{
  shared_ddedge* answer = dynamic_cast<shared_ddedge*> (ans);
  if (0==answer) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (answer->numRefs()>1) throw Shared_Output_Edge;
#endif
  DCASSERT(answer);

  try {
    switch (F->getRangeType()) {
      case MEDDLY::forest::INTEGER:
        F->createEdge(long(t), answer->E);
        return;

      case MEDDLY::forest::REAL:
        F->createEdge(float(t), answer->E);
        return;

      default:
        F->createEdge(t, answer->E);
        return;
    } // switch
  }
  catch (MEDDLY::error fe) {
    convert(fe);
  }
}

void meddly_encoder::buildSymbolicConst(long t, shared_object* ans)
{
  shared_ddedge* answer = dynamic_cast<shared_ddedge*> (ans);
  if (0==answer) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (answer->numRefs()>1) throw Shared_Output_Edge;
#endif
  DCASSERT(answer);

  try {
    if (MEDDLY::forest::REAL == F->getRangeType()) {
      F->createEdge(float(t), answer->E);
    } else {
      F->createEdge(long(t), answer->E);
    }
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder::buildSymbolicConst(double t, shared_object* ans)
{
  shared_ddedge* answer = dynamic_cast<shared_ddedge*> (ans);
  if (0==answer) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (answer->numRefs()>1) throw Shared_Output_Edge;
#endif
  DCASSERT(answer);

  try {
    F->createEdge(float(t), answer->E);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}


const int* 
meddly_encoder::firstMinterm(shared_object* set) const
{
  shared_ddedge* S = dynamic_cast<shared_ddedge*> (set);
  if (0==S) return 0;
  S->startIterator();
  if (S->isIterDone()) return 0;
  return S->getIterMinterm();
}

const int* 
meddly_encoder::nextMinterm(shared_object* set) const
{
  shared_ddedge* S = dynamic_cast<shared_ddedge*> (set);
  if (0==S) return 0;
  if (!S->hasIterator()) return 0; // forgot to call firstMinterm?
  S->incIter();
  if (S->isIterDone()) return 0;
  return S->getIterMinterm();
}



void meddly_encoder::createMinterms(const int* const* mts, int n, shared_object* answer)
{
  if (n<1) return;
  if (0==mts) throw Failed;
  shared_ddedge* dd = dynamic_cast<shared_ddedge*> (answer);
  if (0==dd) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (dd->numRefs()>1) throw Shared_Output_Edge;
#endif

#ifdef SHOW_CREATE_MINTERMS
  fprintf(stderr, "Creating edge for minterms:\n");
  for (int i=0; i<n; i++) {
    int j = F->getDomain()->getNumVariables()-1;
    fprintf(stderr, "\t[");
    for (;;) {
      fprintf(stderr, "%d", mts[i][j]);
      j--;
      if (0==j) break;
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
  }
#endif

  try {
    F->createEdge(mts, n, dd->E);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}


void meddly_encoder::createMinterms(const int* const* from, const int* const* to, int n, shared_object* answer)
{
  if (n<1) return;
  if (0==from || 0==to) throw Failed;
  shared_ddedge* dd = dynamic_cast<shared_ddedge*> (answer);
  if (0==dd) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (dd->numRefs()>1) throw Shared_Output_Edge;
#endif

#ifdef SHOW_CREATE_MINTERMS
  fprintf(stderr, "Creating edge for minterm pairs:\n");
  for (int i=0; i<n; i++) {
    int j = F->getDomain()->getNumVariables()-1;
    fprintf(stderr, "\t[");
    for (;;) {
      fprintf(stderr, "%d", from[i][j]);
      j--;
      if (0==j) break;
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "] \t[");
    j = F->getDomain()->getNumVariables()-1;
    for (;;) {
      fprintf(stderr, "%d", to[i][j]);
      j--;
      if (0==j) break;
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
  }
#endif

  try {
    F->createEdge(from, to, n, dd->E);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder::createMinterms(const int* const* from, const int* const* to, const float* v, int n, shared_object* answer)
{
  if (n<1) return;
  if (0==from || 0==to) throw Failed;
  shared_ddedge* dd = dynamic_cast<shared_ddedge*> (answer);
  if (0==dd) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (dd->numRefs()>1) throw Shared_Output_Edge;
#endif

#ifdef SHOW_CREATE_MINTERMS
  fprintf(stderr, "Creating edge for minterm pairs:\n");
  for (int i=0; i<n; i++) {
    int j = F->getDomain()->getNumVariables()-1;
    fprintf(stderr, "\t[");
    for (;;) {
      fprintf(stderr, "%d", from[i][j]);
      j--;
      if (0==j) break;
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "] \t[");
    j = F->getDomain()->getNumVariables()-1;
    for (;;) {
      fprintf(stderr, "%d", to[i][j]);
      j--;
      if (0==j) break;
      fprintf(stderr, ", ");
    }
    fprintf(stderr, "] \t%f\n", v[i]);
  }
#endif

  try {
    F->createEdge(from, to, v, n, dd->E);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::buildUnary(exprman::unary_opcode op, const shared_object* opnd, 
              shared_object* answer)
{
  const shared_ddedge* opdd = dynamic_cast<const shared_ddedge*> (opnd);
  if (0==opdd) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (answer);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  MEDDLY::dd_edge out(F);

  try {
    switch (op) {
      case exprman::uop_not: {
        if (MEDDLY::forest::BOOLEAN == F->getRangeType()) {
          F->createEdge(true, out);
          out -= opdd->E;
        } else {
          F->createEdge(long(0), out);
          MEDDLY::apply(MEDDLY::EQUAL, out, opdd->E, out);
        }
        ans->E = out;
        return;
      } 

      case exprman::uop_neg: {
        F->createEdge(long(0), out);
        out -= opdd->E;
        ans->E = out;
        return;
      }

      default:
        throw Failed;
    } // switch
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}


void meddly_encoder
::buildBinary(const shared_object* left, exprman::binary_opcode op, 
              const shared_object* right, shared_object* answer)
{
  const shared_ddedge* meL = dynamic_cast<const shared_ddedge*> (left);
  if (0==meL) throw Invalid_Edge;
  const shared_ddedge* meR = dynamic_cast<const shared_ddedge*> (right);
  if (0==meR) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (answer);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  MEDDLY::dd_edge out(getForest());

  try {
    switch (op) {
      case exprman::bop_equals:
        MEDDLY::apply(
          MEDDLY::EQUAL, meL->E, meR->E, out
        );
        break;

      case exprman::bop_nequal:
        MEDDLY::apply(
          MEDDLY::NOT_EQUAL, meL->E, meR->E, out
        );
        break;

      case exprman::bop_le:
        MEDDLY::apply(
          MEDDLY::LESS_THAN_EQUAL, meL->E, meR->E, out
        );
        break;

      case exprman::bop_ge:
        MEDDLY::apply(
          MEDDLY::GREATER_THAN_EQUAL, meL->E, meR->E, out
        );
        break;

      case exprman::bop_gt:
        MEDDLY::apply(
          MEDDLY::GREATER_THAN, meL->E, meR->E, out
        );
        break;

      case exprman::bop_lt:
        MEDDLY::apply(
          MEDDLY::LESS_THAN, meL->E, meR->E, out
        );
        break;

      case exprman::bop_diff:
        MEDDLY::apply(
          MEDDLY::DIFFERENCE, meL->E, meR->E, out
        );
        break;

      case exprman::bop_mod:
        MEDDLY::apply(
          MEDDLY::MODULO, meL->E, meR->E, out
        );
        break;

      case exprman::bop_implies:
        MEDDLY::apply(
          MEDDLY::LESS_THAN_EQUAL, meL->E, meR->E, out
        );
        break;
        
      default:
        throw Failed;

    } // switch
    ans->E = out;
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}


void meddly_encoder
::buildAssoc(const shared_object* left, bool flip, exprman::assoc_opcode op, 
             const shared_object* right, shared_object* answer)
{
  const shared_ddedge* meL = dynamic_cast<const shared_ddedge*> (left);
  if (0==meL) throw Invalid_Edge;
  const shared_ddedge* meR = dynamic_cast<const shared_ddedge*> (right);
  if (0==meR) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (answer);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  MEDDLY::dd_edge out(getForest());

  try {
    switch (op) {

      case exprman::aop_semi:
      case exprman::aop_and:
        DCASSERT(!flip);
        if (MEDDLY::forest::BOOLEAN != F->getRangeType()
            && MEDDLY::forest::EVPLUS != F->getEdgeLabeling())
          MEDDLY::apply(
            MEDDLY::MINIMUM, meL->E, meR->E, out
          );
        else
          MEDDLY::apply(
            MEDDLY::INTERSECTION, meL->E, meR->E, out
          );
        break;

      case exprman::aop_times:
        if (flip) {
          MEDDLY::apply(
            MEDDLY::DIVIDE, meL->E, meR->E, out
          );
        } else {
          MEDDLY::apply(
            MEDDLY::MULTIPLY, meL->E, meR->E, out
          );
        }
        break;

      case exprman::aop_or:
      case exprman::aop_union:
        DCASSERT(!flip);
        if (MEDDLY::forest::BOOLEAN != F->getRangeType())
          MEDDLY::apply(
            MEDDLY::MAXIMUM, meL->E, meR->E, out
          );
        else 
          MEDDLY::apply(
            MEDDLY::UNION, meL->E, meR->E, out
          );
        break;

      case exprman::aop_plus:
        if (flip) {
          MEDDLY::apply(
            MEDDLY::MINUS, meL->E, meR->E, out
          );
        } else {
          MEDDLY::apply(
            MEDDLY::PLUS, meL->E, meR->E, out
          );
        }
        break;

      default:
        throw Failed;

    } // switch

#ifdef DEBUG_ASSOC
    DisplayStream dump(stdout);
    dump << "Built DD for assoc op " << meL->E.getNode() << " (op) ";
    dump << meR->E.getNode() << " = " << out.getNode() << "\n";
    dumpForest(dump);
    dump.flush();
#endif
    ans->E = out;
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::getCardinality(const shared_object* x, long &card)
{
  const shared_ddedge* S = dynamic_cast<const shared_ddedge*> (x);
  if (0==S) throw Invalid_Edge;
#ifdef DEBUG_CARD
  S->E.show(stderr, 3);
#endif
  try {
    MEDDLY::apply(MEDDLY::CARDINALITY, S->E, card);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::getCardinality(const shared_object* x, double &card)
{
  const shared_ddedge* S = dynamic_cast<const shared_ddedge*> (x);
  if (0==S) throw Invalid_Edge;
  try {
    MEDDLY::apply(MEDDLY::CARDINALITY, S->E, card);
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::getCardinality(const shared_object* x, result &card)
{
  const shared_ddedge* S = dynamic_cast<const shared_ddedge*> (x);
  if (0==S) throw Invalid_Edge;
#ifdef HAVE_LIBGMP
  mpz_t mpz_card;
  mpz_init(mpz_card);
#else
  long mpz_card;
#endif
  try {
    MEDDLY::apply(MEDDLY::CARDINALITY, S->E, mpz_card);
    card.setPtr(new bigint(mpz_card));
#ifdef HAVE_LIBGMP
    mpz_clear(mpz_card);
#endif
  } 
  catch (MEDDLY::error e) {
#ifdef HAVE_LIBGMP
    mpz_clear(mpz_card);
#endif
    card.setNull();
    convert(e);
  }
}

void meddly_encoder
::isEmpty(const shared_object* x, bool &empty)
{
  const shared_ddedge* S = dynamic_cast<const shared_ddedge*> (x);
  if (0==S) throw Invalid_Edge;
  empty = (0==S->E.getNode());
}

void meddly_encoder
::preImage(const shared_object* x, const shared_object* E, shared_object* a)
{
  const shared_ddedge* mex = dynamic_cast<const shared_ddedge*> (x);
  if (0==mex) throw Invalid_Edge;
  const shared_ddedge* meE = dynamic_cast<const shared_ddedge*> (E);
  if (0==meE) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (a);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  try {
    MEDDLY::apply(
      MEDDLY::PRE_IMAGE, mex->E, meE->E, ans->E
    );
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::postImage(const shared_object* x, const shared_object* E, shared_object* a)
{
  const shared_ddedge* mex = dynamic_cast<const shared_ddedge*> (x);
  if (0==mex) throw Invalid_Edge;
  const shared_ddedge* meE = dynamic_cast<const shared_ddedge*> (E);
  if (0==meE) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (a);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  try {
    MEDDLY::apply(
      MEDDLY::POST_IMAGE, mex->E, meE->E, ans->E
    );
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::preImageStar(const shared_object* x, const shared_object* E, 
                shared_object* a)
{
  const shared_ddedge* mex = dynamic_cast<const shared_ddedge*> (x);
  if (0==mex) throw Invalid_Edge;
  const shared_ddedge* meE = dynamic_cast<const shared_ddedge*> (E);
  if (0==meE) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (a);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  try {
    if (image_star_uses_saturation) {
      MEDDLY::apply(
        MEDDLY::REVERSE_REACHABLE_DFS, mex->E, meE->E, ans->E
      );  
    } else {
      MEDDLY::apply(
        MEDDLY::REVERSE_REACHABLE_BFS, mex->E, meE->E, ans->E
      );  
    }
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::postImageStar(const shared_object* x, const shared_object* E, 
                shared_object* a)
{
  const shared_ddedge* mex = dynamic_cast<const shared_ddedge*> (x);
  if (0==mex) throw Invalid_Edge;
  const shared_ddedge* meE = dynamic_cast<const shared_ddedge*> (E);
  if (0==meE) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (a);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  try {
    if (image_star_uses_saturation) {
      MEDDLY::apply(
        MEDDLY::REACHABLE_STATES_DFS, mex->E, meE->E, ans->E
      );  
    } else {
      MEDDLY::apply(
        MEDDLY::REACHABLE_STATES_BFS, mex->E, meE->E, ans->E
      );  
    }
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::selectRows(const shared_object* E, const shared_object* rows,
              shared_object* a)
{
  const shared_ddedge* meE = dynamic_cast<const shared_ddedge*> (E);
  if (0==meE) throw Invalid_Edge;
  const shared_ddedge* meRows = dynamic_cast<const shared_ddedge*> (rows);
  if (0==meRows) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (a);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  try {
    MEDDLY::dd_edge one(meRows->getForest());
    switch (meRows->getForest()->getRangeType()) {
      case MEDDLY::forest::BOOLEAN:
        meRows->getForest()->createEdge(true, one);
        break;

      case MEDDLY::forest::INTEGER:
         meRows->getForest()->createEdge(long(1), one);
        break;

      case MEDDLY::forest::REAL:
        meRows->getForest()->createEdge(float(1.0), one);
        break;

      default:
        throw MEDDLY::error(MEDDLY::error::INVALID_OPERATION);

    } // switch
    MEDDLY::dd_edge tmp(meE->getForest());
    MEDDLY::apply(MEDDLY::CROSS, meRows->E, one, tmp);
    if (meE->getForest()->getRangeType() == MEDDLY::forest::BOOLEAN) {
      MEDDLY::apply(MEDDLY::INTERSECTION, meE->E, tmp, ans->E);
    } else {
      MEDDLY::apply(MEDDLY::MULTIPLY, meE->E, tmp, ans->E);
    }
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

void meddly_encoder
::selectCols(const shared_object* E, const shared_object* cols,
              shared_object* a)
{
  const shared_ddedge* meE = dynamic_cast<const shared_ddedge*> (E);
  if (0==meE) throw Invalid_Edge;
  const shared_ddedge* meCols = dynamic_cast<const shared_ddedge*> (cols);
  if (0==meCols) throw Invalid_Edge;
  shared_ddedge* ans = dynamic_cast<shared_ddedge*> (a);
  if (0==ans) throw Invalid_Edge;
#ifdef DEVELOPMENT_CODE
  if (ans->numRefs()>1) throw Shared_Output_Edge;
#endif

  try {
    MEDDLY::dd_edge one(meCols->getForest());
    switch (meCols->getForest()->getRangeType()) {
      case MEDDLY::forest::BOOLEAN:
        meCols->getForest()->createEdge(true, one);
        break;

      case MEDDLY::forest::INTEGER:
         meCols->getForest()->createEdge(long(1), one);
        break;

      case MEDDLY::forest::REAL:
        meCols->getForest()->createEdge(float(1.0), one);
        break;

      default:
        throw MEDDLY::error(MEDDLY::error::INVALID_OPERATION);

    } // switch
    MEDDLY::dd_edge tmp(meE->getForest());
    MEDDLY::apply(MEDDLY::CROSS, one, meCols->E, tmp);
    if (meE->getForest()->getRangeType() == MEDDLY::forest::BOOLEAN) {
      MEDDLY::apply(MEDDLY::INTERSECTION, meE->E, tmp, ans->E);
    } else {
      MEDDLY::apply(MEDDLY::MULTIPLY, meE->E, tmp, ans->E);
    }
  }
  catch (MEDDLY::error e) {
    convert(e);
  }
}

inline void putNodes(OutputStream &out, const char* pre, const char* f, long n)
{
  out << "\t    ";
  out.Put(pre, -8);
  if (f) out.Put(f);
  out << " nodes: " << n << "\n";
}

inline void putMem(OutputStream &out, const char* pre, const char* f, size_t m)
{
  out << "\t    ";
  out.Put(pre, -8);
  if (f) out.Put(f);
  out << " memory: ";
  out.PutMemoryCount(m, 3);
  out << "\n";
}

void meddly_encoder::reportStats(OutputStream &out)
{
  if (0==F->getPeakNumNodes()) return;
  out << "\t" << name << " forest stats:\n";
  putNodes(out, "Current", name, F->getCurrentNumNodes());
  putNodes(out, "Peak", name, F->getPeakNumNodes());
  putMem(out, "Current", name, F->getCurrentMemoryUsed());
  putMem(out, "Peak", name, F->getPeakMemoryUsed());
}

shared_ddedge* meddly_encoder::fold(const MEDDLY::binary_opname* op, 
                      shared_ddedge** list, int N, named_msg* debug) const
{
  if (0==N || 0==list) return 0;
  DCASSERT(F);

  while (N>1) {

    if (debug && debug->startReport()) {
      debug->report().Put(long(N), 9);
      debug->report() << " terms to accumulate ";
      debug->report().Put(long(F->getCurrentNumNodes()), 10);
      debug->report() << " forest nodes\n";
      debug->stopIO();
    }

    // Combine adjacent pairs
    for (int i=0; i<N; i+=2) {
      if (i+1==N) continue;
      if (0==list[i+1] || 0==list[i]) continue;
      MEDDLY::apply(op, list[i]->E, list[i+1]->E, list[i]->E);
      Delete(list[i+1]);
      list[i+1] = 0;
    } // for i

    // Compact list
    int newlength = 0;
    for (int i=0; i<N; i++) {
      if (0==list[i]) continue;
      if (i==newlength) {
        newlength++;
        continue;
      }
      list[newlength] = list[i];
      list[i] = 0;
      newlength++;
    }

    N = newlength;
        
  } // while length

  shared_ddedge* answer = list[0];
  list[0] = 0;
  delete[] list;
  return answer;
}

shared_ddedge* meddly_encoder::accumulate(const MEDDLY::binary_opname* op,
                      shared_ddedge** list, int N, named_msg* debug) const
{
  if (0==N || 0==list) return 0;
  DCASSERT(F);

  for (int i=1; i<N; i++) {
    if (debug && debug->startReport()) {
      debug->report().Put(long(N-i), 9);
      debug->report() << " terms to accumulate ";
      debug->report().Put(long(F->getCurrentNumNodes()), 10);
      debug->report() << " forest nodes\n";
      debug->stopIO();
    }
    if (0==list[i]) continue;
    if (0==list[0]) {
      SWAP(list[i], list[0]);
      continue;
    }
    MEDDLY::apply(op, list[0]->E, list[i]->E, list[0]->E);
    Delete(list[i]);
    list[i] = 0;
  } // for i
  shared_ddedge* answer = list[0];
  list[0] = 0;
  delete[] list;
  return answer;
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_meddly : public initializer {
  public:
    init_meddly();
    virtual bool execute();
};
init_meddly the_meddly_initializer;

init_meddly::init_meddly() : initializer("init_meddly")
{
  usesResource("em");
  buildsResource("MEDDLY");
}

bool init_meddly::execute()
{
  if (0==em)  return false;
  
  // Library registry
  em->registerLibrary(  &mdd_lib_data  );

  // Options
  //
  // TBD - this one belongs under rgr_meddly or somewhere else
  //
  meddly_encoder::image_star_uses_saturation = true;
  em->addOption(
    MakeBoolOption("MeddlyImageStarUsesSaturation",
      "If true, MEDDLY uses saturation for forward and backward reachability during CTL model checking; otherwise, the traditional iteration is used.",
      meddly_encoder::image_star_uses_saturation
    )
  );

  // initialize the library.
  MEDDLY::initialize();

  return true;
}

