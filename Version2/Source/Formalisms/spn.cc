
// $Id$

#include "spn.h"

#include "../Base/api.h"
#include "../Base/memtrack.h"
#include "../Language/api.h"
#include "../Main/tables.h"
#include "../Templates/listarray.h"
#include "../Templates/sparsevect.h"
#include "../States/ops_state.h"

#include "dsm.h"

//#define DEBUG_PN
#define DEBUG_PRIO

// options!

option* PN_Marking_Style;

option_const ms_safe   ("SAFE", "\aFormat is [p1, p3:2, p6]");
option_const ms_sparse ("SPARSE", "\aFormat is [p1:1, p3:2, p6:1]");
option_const ms_indexed("INDEXED", "\aFormat is [p1:1, p2:0, p3:2, p4:0, p5:0, p6:1]");
option_const ms_vector ("VECTOR", "\aFormat is [1, 0, 2, 0, 0, 1]");


// ******************************************************************
// *                                                                *
// *                        Petri net structs                       *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                        spn_arcinfo class                       *
// ******************************************************************

/** Information associated with each arc.
    I.e., what we need to store for an inhibitor, input, or output arc.
    (During model construction only.)
*/
struct spn_arcinfo {
  model_var* place;
  int const_card;  
  expr* proc_card;
  const char* filename;
  int linenumber;
  void Clear() { Delete(proc_card); }
  void FillCard(expr* card) {
    if (NULL==card) {
      const_card = 1;
      proc_card = NULL;
      return;
    }
    if (card->Type(0) == INT) {
      // constant cardinality, compute it
      result x;
      SafeCompute(card, 0, x);
      if (x.isNormal()) {
        const_card = x.ivalue;
        proc_card = NULL;
      } else {
        // error, print the error a bit later
        proc_card = ERROR;
      }
      return;
    } 
    // proc int
    const_card = 0;
    proc_card = card->Substitute(0);
  }
  inline bool operator>(const spn_arcinfo &a) const { 
    return place > a.place; 
  }
  inline bool operator==(const spn_arcinfo &a) const { 
    return place == a.place; 
  }
  void operator+=(const spn_arcinfo &a) {
    const_card += a.const_card;
    if (a.proc_card) {
	  if (proc_card) {
	    proc_card = MakeBinaryOp(proc_card, PLUS, a.proc_card,
	      a.proc_card->Filename(), a.proc_card->Linenumber());
	  } else {
	    proc_card = a.proc_card;
	  }
    } // if a.proc_card
    // multiple arcs are merged, they can be in different files even
    filename = NULL; 
    linenumber = -1;
  }
  inline expr* MakeCard() const {
    if (NULL==proc_card) 
      return MakeConstExpr(PROC_INT, const_card, filename, linenumber);
    if (0==const_card) 
      return Copy(proc_card);
    // we have a constant portion and an expression, merge them
    expr* cc = MakeConstExpr(PROC_INT, const_card, filename, linenumber);
    return MakeBinaryOp(cc, PLUS, proc_card, NULL, -1);
  }
};

/// Useful for debugging
OutputStream& operator<< (OutputStream& s, const spn_arcinfo &a)
{
  s << "Place: " << a.place << "\t const_card: " << a.const_card;
  s << "\t proc_card: " << a.proc_card;
  return s;
}

// ******************************************************************
// *                        transition  class                       *
// ******************************************************************

const char c_nondeterm = 'n';
const char c_immediate = 'i';
const char c_timed = 't';

/** Used to store information during model construction.
    Once the model has been finalized, most of this info
    goes away, and instead we store a pointer to the event
    that was created to represent this transition.
*/
class transition : public model_var {
  /// Data used during model construction
  struct tdata {
      /// Are we immediate, timed, or nondeterministic (default)
      char timed_or_immed;
      /// List of inputs
      int inputs;
      /// List of outputs
      int outputs;
      /// List of inhibitors
      int inhibitors;
      /// guard expression
      expr* guard;
      /// firing distribution
      expr* firing;
      /// weights, for immediate
      expr* weight;
  };
  /// Active during model construction
  tdata* build_info;
  /// Active after model construction
  event* model_event;
  /// Priority set
  ArraySplay <void*> *pset; 
public:
  transition(const char* fn, int line, char* n);
  inline int Inputs() const {
    DCASSERT(build_info);
    return build_info->inputs;
  }
  inline void SetInputs(int I) {
    DCASSERT(build_info);
    build_info->inputs = I;
  }
  inline int Outputs() const {
    DCASSERT(build_info);
    return build_info->outputs;
  }
  inline void SetOutputs(int O) {
    DCASSERT(build_info);
    build_info->outputs = O;
  }
  inline int Inhibitors() const {
    DCASSERT(build_info);
    return build_info->inhibitors;
  }
  inline void SetInhibitors(int H) {
    DCASSERT(build_info);
    build_info->inhibitors = H;
  }
  inline expr* Guard() const {
    DCASSERT(build_info);
    return build_info->guard;
  }
  inline void SetGuard(expr* g) {
    DCASSERT(build_info);
    build_info->guard = g;
  }
  inline char FireType() const {
    DCASSERT(build_info);
    return build_info->timed_or_immed;
  }
  inline expr* Firing() const {
    DCASSERT(build_info);
    return build_info->firing;
  }
  inline void SetFiring(expr* f) {
    DCASSERT(build_info);
    DCASSERT(build_info->timed_or_immed == c_nondeterm);
    build_info->firing = f;
    build_info->timed_or_immed = c_timed;
  }
  inline expr* Weight() const {
    DCASSERT(build_info);
    return build_info->weight;
  }
  inline void SetWeight(expr* w) {
    DCASSERT(build_info);
    DCASSERT(build_info->timed_or_immed == c_nondeterm);
    build_info->weight = w;
    build_info->timed_or_immed = c_immediate;
  }
  inline event* Event() const { return model_event; }

  inline void HasPrioOver(symbol* s) {
    if (NULL==pset) pset = new ArraySplay <void*>;
    pset->AddElement(s);
  }

  /// Build the event and trash the other data.
  void Compile(const char* mn, listarray <spn_arcinfo> *a);

  /// Change the priority list from transition to event.
  void CompilePrio();
};

transition::transition(const char* fn, int line, char* n)
 : model_var(fn, line, TRANS, n)
{
  model_event = NULL;
  build_info = new tdata;
  build_info->timed_or_immed = c_nondeterm;
  build_info->inputs = -1;
  build_info->outputs = -1;
  build_info->inhibitors = -1;
  build_info->guard = NULL;
  build_info->firing = NULL;
  build_info->weight = NULL;
  pset = NULL;
}

// Compile is implemented way, way below

void transition::CompilePrio()
{
  // Note: this must be done after each transition
  // has an associated event; that's why this must be
  // separate from "Compile".

  if (NULL==pset) return;  // that's easy
  // compact set
  int plen = pset->NumNodes();
  void** parray = pset->Compress();
  delete pset; 
  pset = NULL;
  // exchange transition with its event in the list
  for (int i=0; i<plen; i++) {
    transition *t = (transition *) parray[i];
    DCASSERT(t);
    parray[i] = t->Event();
  }
  // affix to our own event
  model_event->SetPriorityList((event**) parray, plen);
}

// ******************************************************************
// *                                                                *
// *            "compiled" Petri net enabling expression            *
// *                                                                *
// ******************************************************************

class transition_enabled : public expr {
  // We have a list of comparisons, split out by comparison type
  // and stored in a not user-friendly way (in favor of speed)
  int end_lower;
  int end_upper;
  int end_expr_lower;
  int end_expr_upper;
  int* lower; // dimension is "end_lower"
  int* upper; // dimension is "end_upper-end_lower"
  model_var** places; // dimension is "end_expr_upper"
  expr** boundlist;   // dimension is "end_expr_upper-end_upper"
  expr* guard;
public:
  // Very heavy constructor, all "compilation" takes place here!
  transition_enabled(listarray <spn_arcinfo> *a, transition *t);
  virtual ~transition_enabled();
  virtual type Type(int i) const {
    DCASSERT(i==0);
    return PROC_BOOL;
  }
  virtual void ClearCache() { DCASSERT(0); }
  virtual void Compute(const state &, int i, result &x);
  virtual expr* Substitute(int i) { DCASSERT(0); return NULL; }
  virtual int GetProducts(int i, List <expr> *prods);
  virtual int GetSymbols(int i, List <symbol> *syms);
  virtual void show(OutputStream &s) const;
};

// ******************************************************************

transition_enabled::transition_enabled(listarray <spn_arcinfo> *a, 
	transition *t) : expr(NULL, -1)
{
  ALLOC("transition_enabled", sizeof(transition_enabled));
  DCASSERT(a);
  DCASSERT(a->IsStatic());
  DCASSERT(t);
 
  List <model_var> plist(4);

  // traverse inputs for constant lower bounds
  DataList <int> lowlist(4);
  if (t->Inputs()>=0) {
    for (int ptr = a->list_pointer[t->Inputs()]; 
	 ptr < a->list_pointer[t->Inputs()+1]; ptr++)  {
      // check if this is a constant arc
      if (a->value[ptr].proc_card) continue;  // there's an expression
      // add this to the comparison list
      plist.Append(a->value[ptr].place);
      lowlist.AppendBlank(); 
      lowlist.data[lowlist.last-1] = a->value[ptr].const_card;
    } // for ptr
  }  // if inputs
  end_lower = plist.Length();
  lower = lowlist.CopyAndClearArray();

  // traverse inhibitors for constant upper bounds
  DataList <int> highlist(4);
  if (t->Inhibitors()>=0) {
    for (int ptr = a->list_pointer[t->Inhibitors()]; 
	 ptr < a->list_pointer[t->Inhibitors()+1]; ptr++)  {
      // check if this is a constant arc
      if (a->value[ptr].proc_card) continue;  // there's an expression
      // add this to the comparison list
      plist.Append(a->value[ptr].place);
      highlist.AppendBlank(); 
      highlist.data[highlist.last-1] = a->value[ptr].const_card;
    } // for ptr
  }  // if inputs
  end_upper = plist.Length();
  upper = highlist.CopyAndClearArray();

  // traverse inputs for variable lower bounds
  List <expr> exprlist(4);
  if (t->Inputs()>=0) {
    for (int ptr = a->list_pointer[t->Inputs()]; 
	 ptr < a->list_pointer[t->Inputs()+1]; ptr++)  {
      if (NULL==a->value[ptr].proc_card) continue;  // constant arc
      // add this to the comparison list
      plist.Append(a->value[ptr].place);
      exprlist.Append(a->value[ptr].MakeCard());
    } // for ptr
  }  // if inputs
  end_expr_lower = plist.Length();

  // traverse inhibitors for variable upper bounds
  if (t->Inhibitors()>=0) {
    for (int ptr = a->list_pointer[t->Inhibitors()]; 
	 ptr < a->list_pointer[t->Inhibitors()+1]; ptr++)  {
      if (NULL==a->value[ptr].proc_card) continue;  // constant arc
      // add this to the comparison list
      plist.Append(a->value[ptr].place);
      exprlist.Append(a->value[ptr].MakeCard());
    } // for ptr
  }  // if inputs
  end_expr_upper = plist.Length();

  places = plist.MakeArray(); 
  boundlist = exprlist.MakeArray();
  
  guard = t->Guard();
}

transition_enabled::~transition_enabled()
{
  FREE("transition_enabled", sizeof(transition_enabled));
  delete[] lower;
  delete[] upper;
  delete[] places;
  // delete the bound expressions, we own them
  for (int i=0; i<end_expr_upper - end_upper; i++) {
    Delete(boundlist[i]);
  }
  delete[] boundlist;
  Delete(guard);
}

void transition_enabled::Compute(const state &s, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  x.bvalue = false;
  int i;
  model_var **p = places;
  int* L = lower;
  for (i=0; i<end_lower; i++) {
    if (L[0] > s.Read(p[0]->state_index).ivalue) return;
    L++;
    p++;
  }
  int* U = upper;
  for (; i<end_upper; i++) {
    if (U[0] <= s.Read(p[0]->state_index).ivalue) return;
    U++;
    p++;
  }
  expr** b = boundlist;
  for (; i<end_expr_lower; i++) {
    SafeCompute(b[0], s, 0, x);
    if (x.isNormal()) {
      if (x.ivalue > s.Read(p[0]->state_index).ivalue) {
        x.bvalue = false;
        return;
      }
    } else {
      if (x.isInfinity()) {
        x.Clear();
        if (x.ivalue>0) {
	  // infinity <= tk(p), definitely false
	  x.bvalue = false;
	  return;
        }
      } else {
        return;  
      }
    }
    b++;
    p++;
  }
  for (; i<end_expr_upper; i++) {
    SafeCompute(b[0], s, 0, x);
    if (x.isNormal()) {
      if (x.ivalue <= s.Read(p[0]->state_index).ivalue) {
        x.bvalue = false;
        return;
      }
    } else {
      if (x.isInfinity()) {
        x.Clear();
        if (x.ivalue<0) {
	  // -infinity > tk(p), definitely false
	  x.bvalue = false;
	  return;
        }
      } else {
        return;  
      }
    }
    b++;
    p++;
  }
  if (guard) {
    SafeCompute(guard, s, 0, x);
    return;
  }
  x.bvalue = true;
}

int transition_enabled::GetProducts(int a, List <expr> *prods)
{
  DCASSERT(0==a);
  int answer = end_expr_upper;
  if (NULL==prods) {
    if (guard) answer += guard->GetProducts(0, NULL);
    return answer;
  }
  DCASSERT(prods); 
  int i;
  model_var **p = places;
  int* L = lower;
  for (i=0; i<end_lower; i++) {
    prods->Append(MakeConstCompare(p[0], GE, L[0], NULL, -1)); 
    L++;
    p++;
  }
  int* U = upper;
  for (; i<end_upper; i++) {
    prods->Append(MakeConstCompare(p[0], LT, U[0], NULL, -1));
    U++;
    p++;
  }
  expr** b = boundlist;
  for (; i<end_expr_lower; i++) {
    prods->Append(MakeExprCompare(p[0], GE, Copy(b[0]), NULL, -1));
    b++;
    p++;
  }
  for (; i<end_expr_upper; i++) {
    prods->Append(MakeExprCompare(p[0], LT, Copy(b[0]), NULL, -1));
    b++;
    p++;
  }
  if (guard) answer += guard->GetProducts(a, prods);
  return answer;
}

int transition_enabled::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  int answer = end_expr_upper;
  if (syms) {
    for (int i=0; i<answer; i++) syms->Append(places[i]);
  }
  for (int i=0; i<end_expr_upper - end_upper; i++) {
    DCASSERT(boundlist[i]);
    answer += boundlist[i]->GetSymbols(0, syms);
  }
  if (guard) answer += guard->GetSymbols(0, syms);
  return answer; 
}

void transition_enabled::show(OutputStream &s) const
{
  bool printed = false;
  int i;
  model_var **p = places;
  int* L = lower;
  for (i=0; i<end_lower; i++) {
    if (printed) s << " & ";
    s << "(" << L[0] << "<=" << p[0]->Name() << ")";
    printed = true;
    L++;
    p++;
  }
  int* U = upper;
  for (; i<end_upper; i++) {
    if (printed) s << " & ";
    s << "(" << p[0]->Name() << "<" << U[0] << ")";
    printed = true;
    U++;
    p++;
  }
  expr** b = boundlist;
  for (; i<end_expr_lower; i++) {
    if (printed) s << " & ";
    s << "(" << b[0] << "<=" << p[0]->Name() << ")";
    printed = true;
    b++;
    p++;
  }
  for (; i<end_expr_upper; i++) {
    if (printed) s << " & ";
    s << "(" << p[0]->Name() << "<" << b[0] << ")";
    printed = true;
    b++;
    p++;
  }
  if (guard) {
    if (printed) s << " & ";
    printed = true;
    s << guard;
  }
  if (!printed) s << "true";
}

// ******************************************************************
// *                                                                *
// *             "compiled" Petri net firing expression             *
// *                                                                *
// ******************************************************************

class transition_fire : public expr {
  // First, a list of integer changes to places;
  // then, a list of expression changes to places.
  // Stored in a not user-friendly way (in favor of speed)
  int end_const;
  int* delta; // dimension is "end_const"
  int end_expr;
  expr** exprlist;    // dimension is "end_expr - end_const"
  bool* negate_expr;  // dimension is "end_expr - end_const"
  model_var** places; // dimension is "end_expr"
  const char* modelname;
public:
  // Very heavy constructor, all "compilation" takes place here!
  transition_fire(const char* mn, listarray <spn_arcinfo> *a, transition *t);
  virtual ~transition_fire();
  virtual type Type(int i) const {
    DCASSERT(i==0);
    return PROC_STATE;
  }
  virtual void ClearCache() { DCASSERT(0); }
  virtual void NextState(const state &, state &next, result &x);
  virtual expr* Substitute(int i) { DCASSERT(0); return NULL; }
  virtual int GetProducts(int i, List <expr> *prods);
  virtual int GetSymbols(int i, List <symbol> *syms);
  virtual void show(OutputStream &s) const;
};

// ******************************************************************

transition_fire::transition_fire(const char* mn,
	listarray <spn_arcinfo> *a, transition *t) : expr(NULL, -1)
{
  ALLOC("transition_fire", sizeof(transition_fire));
  DCASSERT(a);
  DCASSERT(a->IsStatic());
  DCASSERT(t);
 
  modelname = mn;
  List <model_var> plist(4);
  DataList <int> dlist(4);

  // traverse inputs and outputs simultaneously for constant "deltas";
  // a place with both contant input and constant output will
  // have a single, constant delta.

  int in_ptr, in_last;
  if (t->Inputs()>=0) {
    in_ptr = a->list_pointer[t->Inputs()];
    in_last = a->list_pointer[t->Inputs()+1];
  } else {
    in_ptr = 1;
    in_last = 0;
  }
  int out_ptr, out_last;
  if (t->Outputs()>=0) {
    out_ptr = a->list_pointer[t->Outputs()];
    out_last = a->list_pointer[t->Outputs()+1];
  } else {
    out_ptr = 1;
    out_last = 0;
  }
  
  while (in_ptr<in_last && out_ptr<out_last) {
    // advance past any zero constants
    if (a->value[in_ptr].const_card==0) {
      in_ptr++;
      continue;
    }
    if (a->value[out_ptr].const_card ==0) {
      out_ptr++;
      continue;
    }
    model_var* i_pl = a->value[in_ptr].place;
    model_var* o_pl = a->value[out_ptr].place;
    if (i_pl < o_pl) {
      // this place is connected only as input, subtract arc card
      plist.Append(i_pl);
      dlist.AppendBlank(); 
      dlist.data[dlist.last-1] = - a->value[in_ptr].const_card;
      // advance input arc ptr
      in_ptr++;
      continue;
    } // if input only place
    if (o_pl < i_pl) {
      // this place is connected only as output, add arc card
      plist.Append(o_pl);
      dlist.AppendBlank(); 
      dlist.data[dlist.last-1] = a->value[out_ptr].const_card;
      // advance output arc ptr
      out_ptr++;
      continue;
    } // if output only place
    // still here, must have o_pl == i_pl
    DCASSERT(o_pl == i_pl);  // sanity check ;^)
    // this place is connected both as an inhibitor and as input
    // Add (outcard - incard), unless it is zero
    int d = a->value[out_ptr].const_card - a->value[in_ptr].const_card;
    if (d) {
      plist.Append(o_pl);
      dlist.AppendBlank(); 
      dlist.data[dlist.last-1] = d;
    }
    // advance input and output arc ptrs
    in_ptr++;
    out_ptr++;
  } // while
  // At most one of these loops will execute
  for (; in_ptr < in_last; in_ptr++) {
    if (a->value[in_ptr].const_card==0) continue;
    // this place is connected only as input, subtract arc card
    plist.Append(a->value[in_ptr].place);
    dlist.AppendBlank(); 
    dlist.data[dlist.last-1] = - a->value[in_ptr].const_card;
  } // for the rest of the input places
  for (; out_ptr < out_last; out_ptr++) {
    if (a->value[out_ptr].const_card==0) continue;
    // this place is connected only as output, add arc card
    plist.Append(a->value[out_ptr].place);
    dlist.AppendBlank(); 
    dlist.data[dlist.last-1] = a->value[out_ptr].const_card;
  } // for the rest of the output places
    
  end_const = plist.Length();
  delta = dlist.CopyAndClearArray();

  List <expr> elist(4);
  DataList <bool> neglist(4);

  // traverse inputs and outputs simultaneously for expression "deltas"
  // the boolean is used to negate the expression (for input arcs)

  if (t->Inputs()>=0) {
    in_ptr = a->list_pointer[t->Inputs()];
    in_last = a->list_pointer[t->Inputs()+1];
  } else {
    in_ptr = 1;
    in_last = 0;
  }
  if (t->Outputs()>=0) {
    out_ptr = a->list_pointer[t->Outputs()];
    out_last = a->list_pointer[t->Outputs()+1];
  } else {
    out_ptr = 1;
    out_last = 0;
  }
  
  while (in_ptr<in_last && out_ptr<out_last) {
    // advance past any non-expressions
    if (a->value[in_ptr].proc_card ==NULL) {
      in_ptr++;
      continue;
    }
    if (a->value[out_ptr].proc_card ==NULL) {
      out_ptr++;
      continue;
    }
    model_var* i_pl = a->value[in_ptr].place;
    model_var* o_pl = a->value[out_ptr].place;
    if (i_pl < o_pl) {
      // this place is connected only as input, subtract arc card
      plist.Append(i_pl);
      elist.Append(Copy(a->value[in_ptr].proc_card));
      neglist.AppendBlank();
      neglist.data[neglist.last-1] = true;  // negate the cardinality
      // advance input arc ptr
      in_ptr++;
      continue;
    } // if input only place
    if (o_pl < i_pl) {
      // this place is connected only as output, add arc card
      plist.Append(o_pl);
      elist.Append(Copy(a->value[out_ptr].proc_card));
      neglist.AppendBlank();
      neglist.data[neglist.last-1] = false; 
      // advance output arc ptr
      out_ptr++;
      continue;
    } // if output only place
    // still here, must have o_pl == i_pl
    DCASSERT(o_pl == i_pl);  // sanity check ;^)
    // this place is connected both as an inhibitor and as input
    // Add (outcard - incard), unless it is zero
    expr *d = MakeBinaryOp(
			Copy(a->value[out_ptr].proc_card),
			MINUS,
			Copy(a->value[in_ptr].proc_card),
			NULL, -1
              );
    DCASSERT(d);
    plist.Append(o_pl);
    elist.Append(d);
    neglist.AppendBlank();
    neglist.data[neglist.last-1] = false; 
    // advance ptrs
    in_ptr++;
    out_ptr++;
  } // while
  // At most one of these loops will execute
  for (; in_ptr < in_last; in_ptr++) {
    if (a->value[in_ptr].proc_card==NULL) continue;
    // this place is connected only as input, subtract arc card
    plist.Append(a->value[in_ptr].place);
    elist.Append(Copy(a->value[in_ptr].proc_card));
    neglist.AppendBlank();
    neglist.data[neglist.last-1] = true;  // negate the cardinality
  } // for the rest of the input places
  for (; out_ptr < out_last; out_ptr++) {
    if (a->value[out_ptr].proc_card==NULL) continue;
    // this place is connected only as output, add arc card
    plist.Append(a->value[out_ptr].place);
    elist.Append(Copy(a->value[out_ptr].proc_card));
    neglist.AppendBlank();
    neglist.data[neglist.last-1] = false; 
  } // for the rest of the output places
    
  end_expr = plist.Length();
  exprlist = elist.MakeArray();
  negate_expr = neglist.CopyAndClearArray();
  places = plist.MakeArray(); 
}

transition_fire::~transition_fire()
{
  FREE("transition_fire", sizeof(transition_fire));
  delete[] delta;
  delete[] places;
  // delete the expressions, we own them
  for (int i=0; i<end_expr - end_const; i++) {
    Delete(exprlist[i]);
  }
  delete[] exprlist;
  delete[] negate_expr;
}

void transition_fire::NextState(const state &cur, state &next, result &x)
{
  x.Clear();
  int i;
  model_var **p = places;
  int* D = delta;
  for (i=0; i<end_const; i++) {
    // update state; check bounds
    if ( (next[p[0]->state_index].ivalue += (*D)) < 0) {
      // underflow or overflow?
      Error.Start(NULL, -1);
      if (*D > 0) Error << "Overflow ";
      else Error << "Underflow ";
      Error << "of place " << p[0]->Name();
      Error << " in model " << modelname;    
      Error.Stop();
      x.setError(); 
      return;
    }
    D++;
    p++;
  }
  expr** b = exprlist;
  bool* neg = negate_expr;
  for (; i<end_expr; i++) {
    // compute delta expression
    SafeCompute((*b), cur, 0, x);
    if (!x.isNormal()) return;
    if (neg[0]) x.ivalue *= -1; 
    if ( (next[p[0]->state_index].ivalue += x.ivalue) < 0) {
      // underflow or overflow?
      Error.Start(NULL, -1);
      if (x.ivalue > 0) Error << "Overflow ";
      else Error << "Underflow ";
      Error << "of place " << p[0]->Name();
      Error << " in model " << modelname;    
      Error.Stop();
      x.setError(); 
      return;
    }
    b++;
    neg++;
    p++;
  }
}

int transition_fire::GetProducts(int a, List <expr> *prods)
{
  DCASSERT(0==a);
  int answer = end_expr;
  if (NULL==prods) return answer;
  DCASSERT(prods); 
  int i;
  model_var **p = places;
  int* D = delta;
  for (i=0; i<end_const; i++) {
    prods->Append(ChangeStateVar(modelname, p[0], *D, NULL, -1));
    D++;
    p++;
  }
  expr** b = exprlist;
  bool* neg = negate_expr;
  for (; i<end_expr; i++) {
    int OP = (*neg) ? MINUS : PLUS;
    prods->Append(ChangeStateVar(modelname, p[0], OP, Copy(*b), NULL, -1));
    b++;
    neg++;
    p++;
  }
  return answer;
}

int transition_fire::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  int answer = end_expr;
  if (syms) {
    for (int i=0; i<answer; i++) syms->Append(places[i]);
  }
  for (int i=0; i<end_expr - end_const; i++) {
    DCASSERT(exprlist[i]);
    answer += exprlist[i]->GetSymbols(0, syms);
  }
  return answer; 
}


void transition_fire::show(OutputStream &s) const
{
  bool printed = false;
  int i;
  model_var **p = places;
  int* D = delta;
  for (i=0; i<end_const; i++) {
    if (printed) s << "; ";
    s << p[0]->Name();
    if (*D<0) s << " -= " << - (*D);
    else s << " += " << (*D);
    printed = true;
    D++;
    p++;
  }
  expr** b = exprlist;
  bool* neg = negate_expr;
  for (; i<end_expr; i++) {
    if (printed) s << "; ";
    s << p[0]->Name();
    if (*neg) s << " -= "; else s << " += ";
    s << (*b);
    printed = true;
    b++;
    neg++;
    p++;
  }
}

// ******************************************************************
// *      The remaining method for class "transition": Compile      *
// ******************************************************************

void transition::Compile(const char* mn, listarray <spn_arcinfo> *arcs)
{
  DCASSERT(NULL==model_event);
  DCASSERT(build_info);
  model_event = new event(Filename(), Linenumber(), TRANS, strdup(Name()));
  model_event->setEnabling(new transition_enabled(arcs, this));
  model_event->setNextstate(new transition_fire(mn, arcs, this));
  switch (build_info->timed_or_immed) {
    case c_nondeterm:
    	model_event->setNondeterministic();
	break;
    case c_immediate:
    	model_event->setImmediate(build_info->weight);
	break;
    case c_timed:
    	model_event->setTimed(build_info->firing);
	break;
    default:
    	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad firing type for transition " << Name();
	Internal.Stop();
  }
  // done with build_info
  delete build_info;
  build_info = NULL; 
}

// ******************************************************************
// *                                                                *
// *                          spn_dsm class                         *
// *                                                                *
// ******************************************************************

/** A discrete-state model (internel representation) for Stochastic Petri nets.
    Because of the new state_model class, this is significantly smaller.
    We need only state information:
    	the list of places,
	the initial marking.
    Everything else is handled by the state_model "events".
*/
class spn_dsm : public state_model {
protected:
  model_var** places;
  int num_places;
  sparse_vector <int> *initial;
public:
  spn_dsm(const char* fn, int line, char* name, event** ed, int ne,
	  model_var** p, int np, sparse_vector <int> *init);
  virtual ~spn_dsm();

  virtual bool UsesConstantStateSize() const { return true; }

  // This will need to change once we handle phase firing delays
  virtual int GetConstantStateSize() const { return num_places; }

  virtual void ShowState(OutputStream &s, const state &x) const;

  virtual int NumInitialStates() const { return 1; }
  virtual void GetInitialState(int n, state &s) const;
};



// ******************************************************************
// *                         spn_dsm methods                        *
// ******************************************************************

spn_dsm::spn_dsm(const char* fn, int line, char* name, event** ed, int ne,
	model_var** p, int np, sparse_vector <int> *init) 
 : state_model(fn, line, PN, name, ed, ne)
{
  places = p;
  num_places = np;
  initial = init;
}

spn_dsm::~spn_dsm()
{
  // places and transitions themselves are deleted by the model, right?
  free(places);
  delete initial;
}

void spn_dsm::ShowState(OutputStream &s, const state &x) const
{
  DCASSERT(x.Size() >= num_places);
  const option_const* ms_option = PN_Marking_Style->GetEnum();
  int i;
  bool printed = false;
  bool handled = false;
  s << "[";
  if (ms_option == &ms_sparse) {
    // format is [p1:1, p3:2, p4:1]
    for (i=0; i<num_places; i++) {
      if (x.Read(i).isNormal()) {
        if (0==x.Read(i).ivalue) continue;
      }
      if (printed) s << ", ";
      else printed = true;
      s << places[i] << ":";
      PrintResult(s, INT, x.Read(i), -1, -1);
    }
    handled = true;
  }
  if (ms_option == &ms_safe) {
    // format is [p1, p3:2, p4]
    for (i=0; i<num_places; i++) {
      if (x.Read(i).isNormal()) {
        if (0==x.Read(i).ivalue) continue;
      }
      if (printed) s << ", ";
      else printed = true;
      s << places[i];
      if (x.Read(i).isNormal() && x.Read(i).ivalue==1) continue;
      s << ":";
      PrintResult(s, INT, x.Read(i), -1, -1);
    }
    handled = true;
  }
  if (ms_option == &ms_indexed) {
    // format is [p1:1, p2:0, p3:2, p4:1]
    s << places[0] << ":";
    PrintResult(s, INT, x.Read(0), -1, -1);
    for (i=1; i<num_places; i++) {
      s << ", " << places[i] << ":";
      PrintResult(s, INT, x.Read(i), -1, -1);
    }
    handled = true;
  }
  if (ms_option == &ms_vector) {
    // format is [p1:1, p2:0, p3:2, p4:1]
    PrintResult(s, INT, x.Read(0), -1, -1);
    for (i=1; i<num_places; i++) {
      s << ", ";
      PrintResult(s, INT, x.Read(i), -1, -1);
    }
    handled = true;
  }
  s << "]";
  
  if (!handled) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Marking style " << ms_option << " not handled\n";
    Internal.Stop();
  }
}

void spn_dsm::GetInitialState(int n, state &s) const
{
  DCASSERT(n==0);
  if (s.Size() < num_places) {
    // initialize
    FreeState(s);  // in case it is just too small
    AllocState(s, num_places);
  }
  int lasti = 0;
  if (initial) for (int z=0; z<initial->NumNonzeroes(); z++) {
    int thisi = initial->index[z];
    for (int i=lasti; i<thisi; i++) {
      s[i].Clear();
      s[i].ivalue = 0;
    }
    s[thisi].Clear();
    s[thisi].ivalue = initial->value[z];
    lasti = thisi+1;
  }
  for (int i=lasti; i<num_places; i++) {
    s[i].Clear();
    s[i].ivalue = 0;
  }
}


// ******************************************************************
// *                                                                *
// *                        spn_model  class                        *
// *                                                                *
// ******************************************************************

/** Smart support for the Petri net "formalism".
*/
class spn_model : public model {
private:
  // used during state model "compilation"
  List <expr> *exprlist;
protected:
  List <model_var> *placelist;
  List <transition> *translist;
  sparse_vector <int> *initial;
  listarray <spn_arcinfo> *arcs;
public:
  spn_model(const char* fn, int line, type t, char *n, 
		formal_param **pl, int np);

  virtual ~spn_model();

  // For construction
  void AddInput(model_var* pl, model_var* tr, expr* card, 
	 	const char *fn, int ln);
  void AddOutput(model_var* tr, model_var* pl, expr* card, 
		const char *fn, int ln);
  void AddInhibitor(model_var* pl, model_var* tr, expr* card, 
		const char *fn, int ln);
  void AddGuard(model_var* tr, expr* guard);
  void AddFiring(model_var* tr, expr* firing);
  void AddWeight(model_var* tr, expr* w);
  void AddInit(model_var* pl, int tokens, const char *fn, int ln);

  // Required for models:
  virtual model_var* MakeModelVar(const char *fn, int l, type t, char* n);
  virtual void InitModel();
  virtual void FinalizeModel(result &);
  virtual shared_object* BuildStateModel(const char *fn, int ln);
};

// ******************************************************************
// *                       spn_model  methods                       *
// ******************************************************************

spn_model::spn_model(const char *fn, int line, type t, char *n, 
 formal_param **pl, int np) : model(fn, line, t, n, pl, np)
{
  exprlist = new List <expr> (16);
  placelist = NULL;
  translist = NULL;
  arcs = NULL;
  initial = NULL;
}

spn_model::~spn_model()
{
  delete exprlist;
  // These *should* be null already
  delete placelist;
  delete translist;
  delete arcs;
  delete initial;
}


void spn_model::AddInput(model_var* pl, model_var* tr, expr* card, 
			const char *fn, int ln)
{
  DCASSERT(pl);
  CHECK_RANGE(0, pl->state_index, placelist->Length());
  DCASSERT(placelist->Item(pl->state_index) == pl);

  DCASSERT(tr);
  CHECK_RANGE(0, tr->state_index, translist->Length());
  DCASSERT(translist->Item(tr->state_index) == tr);
  
  DCASSERT(arcs);

  transition *t = dynamic_cast <transition*> (tr);
  DCASSERT(t);
  if (t->Inputs()<0) t->SetInputs( arcs->NewList() );

  int list = t->Inputs();
  spn_arcinfo data;
  data.place = pl;
  data.filename = fn;
  data.linenumber = ln;
  data.FillCard(card);
  // check for errors
  if (data.proc_card == ERROR) {
    Error.Start(card->Filename(), card->Linenumber());
    Error << "bad cardinality on arc\n";
    Error << "\tfrom " << pl << " to " << t;
    Error << " in PN " << Name();
    Error.Stop();
    return; 
  }

  if (arcs->AddItemInOrder(list, data) == arcs->NumItems()-1) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate arc\n";
  Warning << "\tfrom " << pl << " to " << t;
  Warning << " in PN " << Name();
  Warning.Stop();
}

void spn_model::AddOutput(model_var* tr, model_var* pl, expr* card, 
			const char *fn, int ln)
{
  DCASSERT(pl);
  CHECK_RANGE(0, pl->state_index, placelist->Length());
  DCASSERT(placelist->Item(pl->state_index) == pl);

  DCASSERT(tr);
  CHECK_RANGE(0, tr->state_index, translist->Length());
  DCASSERT(translist->Item(tr->state_index) == tr);
  
  DCASSERT(arcs);

  transition *t = dynamic_cast <transition*> (tr);
  DCASSERT(t);
  if (t->Outputs()<0) t->SetOutputs( arcs->NewList() );

  int list = t->Outputs();
  spn_arcinfo data;
  data.place = pl;
  data.filename = fn;
  data.linenumber = ln;
  data.FillCard(card);
  // check for errors
  if (data.proc_card == ERROR) {
    Error.Start(card->Filename(), card->Linenumber());
    Error << "bad cardinality on arc\n";
    Error << "\tfrom " << t << " to " << pl;
    Error << " in PN " << Name();
    Error.Stop();
    return; 
  }

  if (arcs->AddItemInOrder(list, data) == arcs->NumItems()-1) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate arc\n";
  Warning << "\tfrom " << t << " to " << pl;
  Warning << " in PN " << Name();
  Warning.Stop();
}

void spn_model::AddInhibitor(model_var* pl, model_var* tr, expr* card, 
			const char *fn, int ln)
{
  DCASSERT(pl);
  CHECK_RANGE(0, pl->state_index, placelist->Length());
  DCASSERT(placelist->Item(pl->state_index) == pl);

  DCASSERT(tr);
  CHECK_RANGE(0, tr->state_index, translist->Length());
  DCASSERT(translist->Item(tr->state_index) == tr);
  
  DCASSERT(arcs);

  transition *t = dynamic_cast <transition*> (tr);
  DCASSERT(t);
  if (t->Inhibitors()<0) t->SetInhibitors( arcs->NewList() );

  int list = t->Inhibitors();
  spn_arcinfo data;
  data.place = pl;
  data.filename = fn;
  data.linenumber = ln;
  data.FillCard(card);
  // check for errors
  if (data.proc_card == ERROR) {
    Error.Start(card->Filename(), card->Linenumber());
    Error << "bad cardinality on inhibitor arc\n";
    Error << "\tfrom " << pl << " to " << t;
    Error << " in PN " << Name();
    Error.Stop();
    return; 
  }

  if (arcs->AddItemInOrder(list, data) == arcs->NumItems()-1) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate inhibitor arc\n";
  Warning << "\tfrom " << pl << " to " << t;
  Warning << " in PN " << Name();
  Warning.Stop();
}

void spn_model::AddGuard(model_var* tr, expr* guard)
{
  DCASSERT(tr);
  CHECK_RANGE(0, tr->state_index, translist->Length());
  DCASSERT(translist->Item(tr->state_index) == tr);
  
  transition *t = dynamic_cast <transition*> (tr);
  DCASSERT(t);
  if (NULL==t->Guard()) {
    t->SetGuard( guard->Substitute(0) );
    return;
  }
  Warning.Start(guard->Filename(), guard->Linenumber());
  Warning << "Merging duplicate guard on transition " << t;
  Warning << " in PN " << Name();
  Warning.Stop(); 
  expr* merge = MakeBinaryOp(t->Guard(), AND, guard->Substitute(0), NULL, -1);
  t->SetGuard( merge );
}

void spn_model::AddFiring(model_var* tr, expr* firing)
{
  DCASSERT(tr);
  CHECK_RANGE(0, tr->state_index, translist->Length());
  DCASSERT(translist->Item(tr->state_index) == tr);
  
  transition *t = dynamic_cast <transition*> (tr);
  DCASSERT(t);
  if (NULL==t->Firing()) {
    t->SetFiring( firing->Substitute(0) );
    return;
  }
  Warning.Start(firing->Filename(), firing->Linenumber());
  Warning << "Ignoring duplicate firing for transition " << t;
  Warning << " in PN " << Name();
  Warning.Stop();
}

void spn_model::AddWeight(model_var* tr, expr* w)
{
  DCASSERT(tr);
  CHECK_RANGE(0, tr->state_index, translist->Length());
  DCASSERT(translist->Item(tr->state_index) == tr);
  
  transition *t = dynamic_cast <transition*> (tr);
  DCASSERT(t);
  if (NULL==t->Weight()) {
    t->SetWeight( w->Substitute(0) );
    return;
  }
  Warning.Start(w->Filename(), w->Linenumber());
  Warning << "Ignoring duplicate weight assignment for transition " << t;
  Warning << " in PN " << Name();
  Warning.Stop();
}

void spn_model::AddInit(model_var* pl, int tokens, const char* fn, int ln)
{
  DCASSERT(pl);
  int place = pl->state_index;
  CHECK_RANGE(0, place, placelist->Length());
  DCASSERT(placelist->Item(place) == pl);
  DCASSERT(tokens);

  int z = initial->BinarySearchIndex(place);
  if (z<0) { // no initialization yet for this place
    initial->SortedAppend(place, tokens);
    return;
  }
  initial->value[z] += tokens;
  Warning.Start(fn, ln);
  Warning << "Summing duplicate initialization for place ";
  Warning << pl << " in PN " << Name();
  Warning.Stop();
}

model_var* spn_model::MakeModelVar(const char *fn, int l, type t, char* n)
{
  model_var* p = NULL;
  transition* tr = NULL;
  int index;
  switch (t) {
    case PLACE:
 	p = new model_var(fn, l, t, n);
        index = placelist->Length();
	p->SetIndex(index);
	p->setLowerBound(0);
	placelist->Append(p);
	break;

    case TRANS:
        tr = new transition(fn, l, n);
	index = translist->Length();
	tr->SetIndex(index);
	translist->Append(tr);
        p = tr;
	break;

    default:
	Internal.Start(__FILE__, __LINE__, fn, l);
	Internal << "Bad type for PN model variable: " << GetType(t) << "\n";
	Internal.Stop();
	return NULL;  // Never get here
  }
#ifdef DEBUG_PN
  Output << "\tModel " << Name() << " created " << GetType(t);
  Output << " " << p << " index " << index << "\n"; 
  Output.flush();
#endif
  return p;
}

void spn_model::InitModel()
{
  DCASSERT(NULL==placelist);
  DCASSERT(NULL==translist); 
  placelist = new List <model_var> (16);
  translist = new List <transition> (16);
  arcs = new listarray <spn_arcinfo>;
  initial = new sparse_vector <int> (8);
}

void spn_model::FinalizeModel(result &x)
{
  // Check that there is an initial marking
  if (0==initial->NumNonzeroes()) {
    Warning.Start();
    Warning << "Assuming zero initial marking in PN " << Name();
    Warning.Stop();
  }
  // Check firing and such
  List <transition> t_list(4);
  List <transition> i_list(4);
  List <transition> n_list(4);
  for (int a=0; a<translist->Length(); a++) {
    transition *t = translist->Item(a);
    switch (t->FireType()) {
      case c_nondeterm:
	n_list.Append(t);
	break;
      case c_immediate:
	i_list.Append(t);
	break;
      case c_timed:
	t_list.Append(t);
	break;
      default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad firing type\n";
  	Internal.Stop();
    }
  } // for a
  if (0==translist->Length()) {
    Warning.Start();
    Warning << "PN " << Name() << " has no transitions\n";
    Warning.Stop();
  } else if (n_list.Length() && n_list.Length() < translist->Length()) {
    Warning.Start();
    Warning << "PN " << Name() << " has no firing distributions for transitions:\n";
    for (int p=0; p<n_list.Length(); p++) {
      transition *t = n_list.Item(p);
      Warning << "\t\t" << t << "\n";
    }
    Warning.Stop();
  } else if (i_list.Length() == translist->Length()) {
    Warning.Start();
    Warning << "PN " << Name() << " has only immediate transitions";
    Warning.Stop();
  }
  // Give immediate events priority over timed ones
  for (int im=0; im<i_list.Length(); im++) {
    transition *it = i_list.Item(im);
    for (int p=0; p<t_list.Length(); p++) {
      it->HasPrioOver(t_list.Item(p));
    } 
  }
#ifdef DEBUG_PN
  int i;
  Output << "Initial marking: [";
  for (i=0; i<initial->NumNonzeroes(); i++) {
    if (i) Output << ", ";
    Output << placelist->Item(initial->index[i]) << ":";
    Output << initial->value[i];
  }
  Output << "]\n";
  Output.flush();
  for (i=0; i<translist->Length(); i++) {
    transition *t = translist->Item(i);
    Output << "Transition " << t << "\n";
    if (t->Inputs()>=0) {
      Output << "\tinputs: ";
      arcs->ShowNodeList(Output, t->Inputs());
    }
    if (t->Outputs()>=0) {
      Output << "\toutputs: ";
      arcs->ShowNodeList(Output, t->Outputs());
    }
    if (t->Inhibitors()>=0) {
      Output << "\tinhibitors: ";
      arcs->ShowNodeList(Output, t->Inhibitors());
    }
    Output.flush();
  }
#endif
  // success:
  x.Clear();
  x.other = Share(this);

  arcs->ConvertToStatic();
}

shared_object* spn_model::BuildStateModel(const char* fn, int ln)
{
  // This must now "compile" our nice PN representation
  // into "events" as required by state_models.

  // 
  // Make an event for each transition
  // 
  int num_events = translist->Length();
  event** event_data = new event*[num_events];
  for (int i=0; i<num_events; i++) {
    transition* t = translist->Item(i);
    t->Compile(Name(), arcs);
    event_data[i] = t->Event();
  }
  for (int i=0; i<num_events; i++) {
    transition* t = translist->Item(i);
    t->CompilePrio();  // must be done after all events are created
  }
  delete translist;
  // clear out the arcs 
  for (int e=0; e<arcs->items_alloc; e++) arcs->value[e].Clear();
  delete arcs;
  translist = NULL;

  //
  // Places: specific to PN state model
  //
  int num_places = placelist->Length();
  model_var** plist = placelist->MakeArray();
  delete placelist; 
  placelist = NULL;

  //
  // Construction, finally
  //
  state_model *m = new spn_dsm(fn, ln, strdup(Name()), 
			event_data, num_events,
  			plist, num_places, initial);

  // check cycles in priority
  if (m->PrioCycle()) {
    delete m;
    return NULL;
  }
  
  // no problems
  return m;
}


// ******************************************************************
// *                                                                *
// *                      PN-specific functions                     *
// *                                                                *
// ******************************************************************

// ********************************************************
// *                         init                         *
// ********************************************************

void compute_spn_init(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);
#ifdef DEBUG_PN
  Output << "Inside init for pn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_PN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
    Output.flush();
#endif
    result pl, tk;
    SafeCompute(pp[i], 0, pl);
    DCASSERT(pl.isNormal());
    model_var* place = dynamic_cast <model_var*> (pl.other);
    DCASSERT(place);

    SafeCompute(pp[i], 1, tk);
    // error checking of tk here   
    if (0==tk.ivalue) continue;  // print a warning...
    spn->AddInit(place, tk.ivalue, pp[i]->Filename(), pp[i]->Linenumber());
  }
#ifdef DEBUG_PN
  Output << "Exiting init for pn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

void Add_spn_init(PtrTable *fns)
{
  const char* helpdoc = "Assigns the initial marking.  Any places not listed are assumed to be initially empty.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(PN, "net");
  type *tl = new type[2];
  tl[0] = PLACE;
  tl[1] = INT;
  pl[1] = new formal_param(tl, 2, "pt");
  internal_func *p = new internal_func(VOID, "init", 
	compute_spn_init, NULL, pl, 2, 1, helpdoc);  // param 1 repeats
  p->setWithinModel();
  InsertFunction(fns, p);
}


// ********************************************************
// *                         arcs                         *
// ********************************************************

void compute_spn_arcs(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);

#ifdef DEBUG_PN
  Output << "Inside arcs for pn " << spn << "\n";
  Output.flush();
#endif

  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_PN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result first;
    SafeCompute(pp[i], 0, first);
    DCASSERT(first.isNormal());
    model_var* fv = dynamic_cast <model_var*> (first.other);
    DCASSERT(fv);

    result second;
    SafeCompute(pp[i], 1, second); 
    DCASSERT(second.isNormal());
    model_var* sv = dynamic_cast <model_var*> (second.other);
    DCASSERT(sv);
 
    // error checking of first and second here   
 
    expr* card = NULL;
    if (pp[i]->NumComponents()==3) card = pp[i]->GetComponent(2);
    switch (pp[i]->Type(0)) {
      case PLACE:
        spn->AddInput(fv, sv, card, pp[i]->Filename(), pp[i]->Linenumber());
        break;

      case TRANS:
        spn->AddOutput(fv, sv, card, pp[i]->Filename(), pp[i]->Linenumber());
        break;
      
      default:
        Internal.Start(__FILE__, __LINE__, pp[i]->Filename(), pp[i]->Linenumber());
	Internal << "Bad parameter for pn function arcs\n";
	Internal.Stop();
    }
    
    // stuff here
  }

#ifdef DEBUG_PN
  Output << "Exiting arcs for pn " << spn << "\n";
  Output.flush();
#endif

  x.setNull();
}

int typecheck_arcs(List <expr> *params)
{
  // Note: hidden parameter PN has NOT been added yet...
  int np = params->Length();
  int i;
  for (i=0; i<np; i++) {
    expr* p = params->Item(i);
    if (NULL==p) return -1;
    if (ERROR==p) return -1;
    if (p->NumComponents()<2) return -1;
    if (p->NumComponents()>3) return -1;
   
    bool ok1 = (p->Type(0)==PLACE) && (p->Type(1)==TRANS);
    bool ok2 = (p->Type(0)==TRANS) && (p->Type(1)==PLACE);
    if (!(ok1 || ok2)) return -1;

    // check cardinality, if it is there
    if (p->NumComponents()==2) continue;
    if (p->Type(2) == INT) continue;
    if (p->Type(2) == PROC_INT) continue;
    return -1;
  } // for i
  return 0;
}

bool linkparams_arcs(expr **p, int np)
{
  // anything?
  return true;
}

void Add_spn_arcs(PtrTable *fns)
{
  const char* helpdoc = "\b(describe arguments)\nAdds arcs to a Petri net";

  internal_func *p = new internal_func(VOID, "arcs", 
	compute_spn_arcs, NULL, NULL, 0, helpdoc);
  p->setWithinModel();
  p->SetSpecialTypechecking(typecheck_arcs);
  p->SetSpecialParamLinking(linkparams_arcs);
  InsertFunction(fns, p);
}


// ********************************************************
// *                        inhibit                       *
// ********************************************************

void compute_spn_inhibit(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);

#ifdef DEBUG_PN
  Output << "Inside inhibit for pn " << spn << "\n";
  Output.flush();
#endif

  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_PN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result first;
    SafeCompute(pp[i], 0, first);
    DCASSERT(first.isNormal());
    model_var* fv = dynamic_cast <model_var*> (first.other);
    DCASSERT(fv);

    result second;
    SafeCompute(pp[i], 1, second); 
    DCASSERT(second.isNormal());
    model_var* sv = dynamic_cast <model_var*> (second.other);
    DCASSERT(sv);
 
    expr* card = NULL;
    if (pp[i]->NumComponents()==3) card = pp[i]->GetComponent(2);

    spn->AddInhibitor(fv, sv, card, pp[i]->Filename(), pp[i]->Linenumber());
    
  }

#ifdef DEBUG_PN
  Output << "Exiting inhibit for pn " << spn << "\n";
  Output.flush();
#endif

  x.setNull();
}


int typecheck_inhibit(List <expr> *params)
{
  // Note: hidden parameter PN has NOT been added yet...
  int np = params->Length();
  int i;
  for (i=0; i<np; i++) {
    expr* p = params->Item(i);
    if (NULL==p) return -1;
    if (ERROR==p) return -1;
    if (p->NumComponents()<2) return -1;
    if (p->NumComponents()>3) return -1;

    if (p->Type(0)!=PLACE) return -1;
    if (p->Type(1)!=TRANS) return -1;
   
    // check cardinality, if it is there
    if (p->NumComponents()==2) continue;
    if (p->Type(2) == INT) continue;
    if (p->Type(2) == PROC_INT) continue;
    return -1;
  } // for i
  return 0;
}

bool linkparams_inhibit(expr **p, int np)
{
  // anything?
  return true;
}

void Add_spn_inhibit(PtrTable *fns)
{
  const char* helpdoc = "\b(..., place:trans p:t  OR  place:trans:card p:t:c, ...)\nAdds an inhibitor arc from place p to transition t, with cardinality c.  If c is omitted, it is assumed to be 1.  c can have type int or proc int.";

  internal_func *p = new internal_func(VOID, "inhibit", 
	compute_spn_inhibit, NULL, NULL, 0, helpdoc);
  p->setWithinModel();
  p->SetSpecialTypechecking(typecheck_inhibit);
  p->SetSpecialParamLinking(linkparams_inhibit);
  InsertFunction(fns, p);
}


// ********************************************************
// *                         guard                        *
// ********************************************************

void compute_spn_guard(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);
#ifdef DEBUG_PN
  Output << "Inside guard for pn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_PN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result t;
    SafeCompute(pp[i], 0, t);
    DCASSERT(t.isNormal());
    model_var* tv = dynamic_cast <model_var*> (t.other);
    DCASSERT(tv);
    
    spn->AddGuard(tv, pp[i]->GetComponent(1));
  }
#ifdef DEBUG_PN
  Output << "Exiting guard for pn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

void Add_spn_guard(PtrTable *fns)
{
  const char* helpdoc = "Assigns guard expressions to transitions.  If the guard evaluates to false then the transition is disabled.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(PN, "net");
  type *tl = new type[2];
  tl[0] = TRANS;
  tl[1] = PROC_BOOL;
  pl[1] = new formal_param(tl, 2, "tg");
  internal_func *p = new internal_func(VOID, "guard", 
	compute_spn_guard, NULL, pl, 2, 1, helpdoc);  // param 1 repeats
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                        firing                        *
// ********************************************************

void compute_spn_firing(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);
#ifdef DEBUG_PN
  Output << "Inside firing for pn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_PN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result t;
    SafeCompute(pp[i], 0, t);
    DCASSERT(t.isNormal());
    model_var* tv = dynamic_cast <model_var*> (t.other);
    DCASSERT(tv);
    
    spn->AddFiring(tv, pp[i]->GetComponent(1));
  }
#ifdef DEBUG_PN
  Output << "Exiting firing for pn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

int typecheck_firing(List <expr> *params)
{
  // Note: hidden parameter PN has NOT been added yet...
  int np = params->Length();
  int i;
  for (i=0; i<np; i++) {
    expr* p = params->Item(i);
    if (NULL==p) return -1;
    if (ERROR==p) return -1;
    if (p->NumComponents()!=2) return -1;
    if (p->Type(0)!=TRANS) return -1;
    bool ok;
    switch (p->Type(1)) {
      case INT:
      case REAL:
      case EXPO:
      case PH_INT:
      case PH_REAL:
      case RAND_INT:
      case RAND_REAL:
			ok = true;
			break;
      default:
			ok = false;
    }
    if (!ok) return -1;
  } // for i
  return 0;
}

bool linkparams_firing(expr **p, int np)
{
  // anything?
  return true;
}


void Add_spn_firing(PtrTable *fns)
{
  const char* helpdoc = "\b(..., trans:distribution t:d, ...)\nAssigns firing distribution d to transition t.  The distribution d can have types int, real, ph int, ph real, rand int, rand real, and the \"proc\" equivalents.  To set the distribution for immediate transitions, \"weight\" should be used instead.";

  internal_func *p = new internal_func(VOID, "firing", 
	compute_spn_firing, NULL, NULL, 0, helpdoc);  
  p->setWithinModel();
  p->SetSpecialTypechecking(typecheck_firing);
  p->SetSpecialParamLinking(linkparams_firing);
  InsertFunction(fns, p);
}

// ********************************************************
// *                        weight                        *
// ********************************************************

void compute_spn_weight(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);
#ifdef DEBUG_PN
  Output << "Inside weight for pn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_PN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result t;
    SafeCompute(pp[i], 0, t);
    DCASSERT(t.isNormal());
    model_var* tv = dynamic_cast <model_var*> (t.other);
    DCASSERT(tv);
    
    spn->AddWeight(tv, pp[i]->GetComponent(1));
  }
#ifdef DEBUG_PN
  Output << "Exiting weight for pn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

int typecheck_weight(List <expr> *params)
{
  // Note: hidden parameter PN has NOT been added yet...
  int np = params->Length();
  int i;
  for (i=0; i<np; i++) {
    expr* p = params->Item(i);
    if (NULL==p) return -1;
    if (ERROR==p) return -1;
    if (p->NumComponents()!=2) return -1;
    if (p->Type(0)!=TRANS) return -1;
    bool ok;
    switch (p->Type(1)) {
      case REAL:
      case PROC_REAL:
			ok = true;
			break;
      default:
			ok = false;
    }
    if (!ok) return -1;
  } // for i
  return 0;
}

bool linkparams_weight(expr **p, int np)
{
  // anything?
  return true;
}


void Add_spn_weight(PtrTable *fns)
{
  const char* helpdoc = "\b(..., trans:value t:v, ...)\nMake t an immediate transition with weight v (either of type real or proc real).";

  internal_func *p = new internal_func(VOID, "weight", 
	compute_spn_weight, NULL, NULL, 0, helpdoc);  
  p->setWithinModel();
  p->SetSpecialTypechecking(typecheck_weight);
  p->SetSpecialParamLinking(linkparams_weight);
  InsertFunction(fns, p);
}

// ********************************************************
// *                          tk                          *
// ********************************************************

// A Proc function 
void compute_spn_tk(const state &m, expr **pp, int np, result &x)
{
  DCASSERT(np==2);
  DCASSERT(pp);
#ifdef DEBUG
  Output << "Checking tk\n";
  Output.flush();
#endif
  x.Clear();
  SafeCompute(pp[1], 0, x);
#ifdef DEVELOPMENT_CODE
  DCASSERT(x.isNormal());
  model_var* place = dynamic_cast <model_var*> (x.other);
  DCASSERT(place);
#else 
  model_var* place = (model_var*) x.other;
#endif
 
#ifdef DEBUG
  Output << "\tgot param: ";
  PrintResult(Output, INT, x);
  Output << "\n";
  Output.flush();
#endif

  x = m.Read(place->state_index);
}

void Add_spn_tk(PtrTable *fns)
{
  const char* helpdoc = "Returns the number of tokens in place p";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(PN, "net");
  pl[1] = new formal_param(PLACE, "p");
  internal_func *p = new internal_func(PROC_INT, "tk", 
	compute_spn_tk, NULL, pl, 2, helpdoc);  
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                         prio                         *
// ********************************************************

void compute_spn_prio(expr **pp, int np, result &x)
{
  DCASSERT(np>1);
  DCASSERT(pp);
  spn_model *spn = dynamic_cast<spn_model*>(pp[0]);
  DCASSERT(spn);
#ifdef DEBUG_PN
  Output << "Inside prio for pn " << spn << "\n";
#else 
#ifdef DEBUG_PRIO
  Output << "Inside prio for pn " << spn << "\n";
#endif
#endif
  x.Clear();

  for (int i=1; i<np; i++) {
    result s1, s2;
    SafeCompute(pp[i], 0, s1);
    SafeCompute(pp[i], 1, s2);
    // error checking here
    set_result* T1 = dynamic_cast <set_result*> (s1.other);
    DCASSERT(T1);
    set_result* T2 = dynamic_cast <set_result*> (s2.other);
    DCASSERT(T2);
    for (int j1=0; j1<T1->Size(); j1++) {
      result x1;
      x1.Clear();
      T1->GetElement(j1, x1);
      DCASSERT(x1.isNormal());
      transition* t1 = dynamic_cast <transition*> (x1.other);
      DCASSERT(t1);
#ifdef DEBUG_PRIO
      Output << "Setting priority for " << t1 << " higher than:\n\t";
#endif
      for (int j2=0; j2<T2->Size(); j2++) {
        result x2;
        x2.Clear();
        T2->GetElement(j2, x2);
        DCASSERT(x2.isNormal());
        transition* t2 = dynamic_cast <transition*> (x2.other);
        DCASSERT(t2);
	t1->HasPrioOver(t2);
#ifdef DEBUG_PRIO
	Output << t2 << " ";
#endif
      } // for j2
#ifdef DEBUG_PRIO
      Output << "\n";
      Output.flush();
#endif
    } // for j1
  } // for i (parameters)

  x.setNull();
}

void Add_prio(PtrTable *fns)
{
  const char* helpdoc = "Transitions in the set t1 all have priority over transitions in the set t2.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(PN, "net");
  type *tl = new type[2];
  tl[0] = SET_TRANS;
  tl[1] = SET_TRANS;
  pl[1] = new formal_param(tl, 2, "t1:t2");
  internal_func *p = new internal_func(VOID, "prio", 
	compute_spn_prio, NULL, pl, 2, 1, helpdoc);  // param 1 repeats
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                       showlist                       *
// ********************************************************


void compute_showlist(expr **pp, int np, result &x)
{
  DCASSERT(np==2);
  DCASSERT(pp);
  x.Clear();
  SafeCompute(pp[1], 0, x);
  if (x.isError() || x.isNull()) return;

  set_result* T = dynamic_cast <set_result*> (x.other);
  DCASSERT(T); 
  Output << "Transitions in set:\n";
  int i;
  for (i=0; i<T->Size(); i++) {
    x.Clear();
    T->GetElement(i, x);
    if (!x.isNormal()) {
      Output << "\t(bad transition)\n";
    } else {
      symbol* s = dynamic_cast <symbol*> (x.other);
      DCASSERT(s);
      Output << "\t" << s << "\n";
    }
  }
  
  x.setNull();
}

void Add_showlist(PtrTable *fns)
{
  const char* helpdoc = "Test function; displays a set of transitions.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(PN, "net");
  pl[1] = new formal_param(SET_TRANS, "T");
  internal_func *p = new internal_func(VOID, "showlist", 
	compute_showlist, NULL, pl, 2, helpdoc);  
  p->setWithinModel();
  InsertFunction(fns, p);
}


// ******************************************************************
// *                                                                *
// *                        Global front-ends                       *
// *                                                                *
// ******************************************************************

model* MakePetriNet(type t, char* id, formal_param **pl, int np,
			const char* fn, int line)
{
  return new spn_model(fn, line, t, id, pl, np);
}

void InitPNModelFuncs(PtrTable *t)
{
  Add_spn_init(t);
  Add_spn_arcs(t);
  Add_spn_inhibit(t);
  Add_spn_guard(t);
  Add_spn_firing(t);
  Add_spn_weight(t);

  Add_spn_tk(t);

  Add_prio(t);

  // Initialize PN-specific options

  // PN_Marking_Style option
  option_const **mslist = new option_const*[4];
  // alphabetical...
  mslist[0] = &ms_indexed;
  mslist[1] = &ms_safe;
  mslist[2] = &ms_sparse;
  mslist[3] = &ms_vector;
  // ...any others?
  PN_Marking_Style = MakeEnumOption("PnMarkingStyle", "How to display a Petri net marking", mslist, 4, &ms_sparse);
  AddOption(PN_Marking_Style);
}

