
// $Id$

#include "spn.h"

#include "../Base/api.h"
#include "../Language/api.h"
#include "../Main/tables.h"
#include "../Templates/listarray.h"
#include "../States/ops_state.h"

#include "dsm.h"

//#define DEBUG_SPN

// options!

option* PN_Marking_Style;

option_const ms_safe   ("SAFE", "Format is [p1, p3:2, p6]");
option_const ms_sparse ("SPARSE", "Format is [p1:1, p3:2, p6:1]");
option_const ms_indexed("INDEXED", "Format is [p1:1, p2:0, p3:2, p4:0, p5:0, p6:1]");
option_const ms_vector ("VECTOR", "Format is [1, 0, 2, 0, 0, 1]");


// ******************************************************************
// *                                                                *
// *                        Petri net structs                       *
// *                                                                *
// ******************************************************************

class transition : public model_var {
public:
  /// List of inputs
  int inputs;
  /// List of outputs
  int outputs;
  /// List of inhibitors
  int inhibitors;
  /// guard expression
  expr* guard;
  // firing distribution
  expr* firing;
public:
  transition(const char* fn, int line, char* n) : model_var(fn, line, TRANS, n) { inputs = outputs = inhibitors = -1; guard = firing = NULL; }
};


/// Information for each input / output / inhibitor arc
struct spn_arcinfo {
  int place;
  int const_card;  
  expr* proc_card;
  const char* filename;
  int linenumber;
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
  inline expr* InputCompare(model_var* pl) const {
    // we want cardinality of tk(pn) >= arc
    if (NULL==proc_card) 
      return MakeConstCompare(pl, GE, const_card, filename, linenumber);
    // expression for cardinality 
    return MakeExprCompare(pl, GE, MakeCard(), filename, linenumber);
  }
  inline expr* InhibitCompare(model_var* pl) const {
    // we want cardinality of arc > tk(pn)
    if (NULL==proc_card) 
      return MakeConstCompare(pl, LT, const_card, filename, linenumber);
    // expression for cardinality 
    return MakeExprCompare(pl, LT, MakeCard(), filename, linenumber);
  }
  inline expr* AddToPlace(const char* mn, model_var* pn) const {
    if (NULL==proc_card) 
      return ChangeStateVar(mn, pn, const_card, filename, linenumber);
    return ChangeStateVar(mn, pn, PLUS, MakeCard(), filename, linenumber);
  }
  inline expr* SubFromPlace(const char* mn, model_var* pn) const {
    if (NULL==proc_card)
      return ChangeStateVar(mn, pn, -const_card, filename, linenumber);
    return ChangeStateVar(mn, pn, MINUS, MakeCard(), filename, linenumber);
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
// *                                                                *
// *                          spn_dsm class                         *
// *                                                                *
// ******************************************************************

/** A discrete-state model (internel representation) for Stochastic Petri nets.
*/
class spn_dsm : public state_model {
private:
  /// Used to build expressions
  List <expr> *exprlist;
protected:
  model_var** places;
  int num_places;
  transition** transitions;
  sparse_vector <int> *initial;
  listarray <spn_arcinfo> *arcs;
public:
  spn_dsm(const char* name, 
	  model_var** p, int np, 
          transition** t, int nt, 
	  sparse_vector <int> *init,
          listarray<spn_arcinfo> *a,
	  const char* fn, int ln);
  virtual ~spn_dsm();

  virtual bool UsesConstantStateSize() const { return true; }

  // This will need to change once we handle phase firing delays
  virtual int GetConstantStateSize() const { return num_places; }

  virtual void ShowState(OutputStream &s, const state &x) const;
  virtual void ShowEventName(OutputStream &s, int e) const {
    s << transitions[e]->Name();
  }

  virtual int NumInitialStates() const { return 1; }
  virtual void GetInitialState(int n, state &s) const;
  virtual expr* EnabledExpr(int e);
  virtual expr* NextStateExpr(int e);

  virtual void isEnabled(int e, const state &s, result &answer);
  virtual void getNextState(const state &cur, int e, state &nxt, result &err);

  virtual expr* EventDistribution(int e) { 
    CHECK_RANGE(0, e, NumEvents());
    return transitions[e]->firing;
  }
  virtual type EventDistributionType(int e) { 
    CHECK_RANGE(0, e, NumEvents());
    if (transitions[e]->firing) 
      return transitions[e]->firing->Type(0);
    return VOID;
  }
};

// ******************************************************************
// *                         spn_dsm methods                        *
// ******************************************************************

spn_dsm::spn_dsm(const char* name, 
	model_var** p, int np,
	transition** t, int nt,
   	sparse_vector <int> *init,
	listarray<spn_arcinfo> *a,
	const char* fn, int ln) : state_model(name, nt, fn, ln)
{
  places = p;
  num_places = np;
  transitions = t;
  initial = init;
  arcs = a;
  DCASSERT(arcs->IsStatic());
  // temp stuff
  exprlist = new List <expr> (16);
}

spn_dsm::~spn_dsm()
{
  // places and transitions themselves are deleted by the model, right?
  free(places);
  free(transitions);
  delete initial;
  delete arcs;
  // temp stuff
  delete exprlist;
}

void spn_dsm::ShowState(OutputStream &s, const state &x) const
{
/* Idea:
	have different state display "formats" to select from
	using an option; e.g.,
	 #PN_Marking_Style
		SPARSE_SAFE	(don't show #tokens unless > 1)
		SPARSE 		(the one currently implemented)
		FULL_INDEXED	(like sparse, but show also for 0 tokens)
		FULL_VECTOR	(marking is a vector of naturals)
*/
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

expr* spn_dsm::EnabledExpr(int e)
{
  CHECK_RANGE(0, e, NumEvents());
  // Build a huge conjunction, first as a list of expressions
  exprlist->Clear();

  // Go through inputs and inhibitors in order (they're ordered)
  int input_list = transitions[e]->inputs;
  int input_ptr;
  if (input_list>=0) input_ptr = arcs->list_pointer[input_list];
  else input_ptr = -1; // signifies "done"

  int inhib_list = transitions[e]->inhibitors;
  int inhib_ptr;
  if (inhib_list>=0) inhib_ptr = arcs->list_pointer[inhib_list];
  else inhib_ptr = -1; // signifies "done"
  
  while (inhib_ptr>=0 || input_ptr>=0) {
    int i_pl = (input_ptr<0) ? num_places+1 : arcs->value[input_ptr].place;
    int h_pl = (inhib_ptr<0) ? num_places+1 : arcs->value[inhib_ptr].place;
    
    if (i_pl < h_pl) {
      // this place is connected only as input
      expr* cmp = arcs->value[input_ptr].InputCompare(places[i_pl]);
      if (cmp) exprlist->Append(cmp);
      // advance input arc ptr
      input_ptr++;
      if (input_ptr==arcs->list_pointer[input_list+1]) input_ptr = -1;
    } // if input only place
    
    if (h_pl < i_pl) {
      // this place is connected only as an inhibitor
      expr* cmp = arcs->value[inhib_ptr].InhibitCompare(places[h_pl]);
      if (cmp) exprlist->Append(cmp);
      // advance inhibitor arc ptr
      inhib_ptr++;
      if (inhib_ptr==arcs->list_pointer[inhib_list+1]) inhib_ptr = -1;
    } // if inhibitor only place
    
    if (h_pl == i_pl) {
      // this place is connected both as an inhibitor and as input
      // we want inputcard <= tk(inplace) < inhibcard
      expr* cmp = NULL;
      if ((NULL==arcs->value[inhib_ptr].proc_card) &&
          (NULL==arcs->value[input_ptr].proc_card)) {
        // constant cardinality for both arcs, can use a fast operator
        cmp = MakeConstBounds(arcs->value[input_ptr].const_card, 
  	  places[i_pl], arcs->value[inhib_ptr].const_card-1, NULL, -1);
        // advance input arc ptr
        input_ptr++;
        if (input_ptr==arcs->list_pointer[input_list+1]) input_ptr = -1;
        // advance inhibitor arc ptr
        inhib_ptr++;
        if (inhib_ptr==arcs->list_pointer[inhib_list+1]) inhib_ptr = -1;
      } else {
        // no fast operator, so handle them separately
        // input first
        cmp = arcs->value[input_ptr].InputCompare(places[i_pl]);
        // advance input arc ptr
        input_ptr++;
        if (input_ptr==arcs->list_pointer[input_list+1]) input_ptr = -1;
      } // if both constant
      if (cmp) exprlist->Append(cmp);
    } // both input and inhibitor
  } // while

  // add any transition guards to the list here.
  expr* guard = transitions[e]->guard;
  if (guard) {
    // split into products, because we want one huge conjunction
    List <expr> foo(16);
    int np = guard->GetProducts(0, &foo);
    for (int p=0; p<np; p++) {
      exprlist->Append(Copy(foo.Item(p)));
    }
  }

  // have list of conditions, build conjunction
  int numopnds = exprlist->Length();
  if (numopnds==0) return MakeConstExpr(PROC_BOOL, true, NULL, -1);
  expr** opnds = exprlist->Copy();  
  exprlist->Clear();
  return MakeAssocOp(AND, opnds, numopnds, NULL, -1);
}

expr* spn_dsm::NextStateExpr(int e)
{
  CHECK_RANGE(0, e, NumEvents());
  // Build a huge conjunction, first as a list of expressions
  exprlist->Clear();

  // Go through inputs and outputs in order (they're ordered)
  int in_list = transitions[e]->inputs;
  int in_ptr;
  if (in_list>=0) in_ptr = arcs->list_pointer[in_list];
  else in_ptr = -1; // signifies "done"

  int out_list = transitions[e]->outputs;
  int out_ptr;
  if (out_list>=0) out_ptr = arcs->list_pointer[out_list];
  else out_ptr = -1; // signifies "done"

  while (out_ptr>=0 || in_ptr>=0) {
    int i_pl = (in_ptr<0) ? num_places+1 : arcs->value[in_ptr].place;
    int o_pl = (out_ptr<0) ? num_places+1 : arcs->value[out_ptr].place;
    
    if (i_pl < o_pl) {
      // this place is connected only as input, subtract arc card
      expr* sub = arcs->value[in_ptr].SubFromPlace(Name(), places[i_pl]);
      if (sub) exprlist->Append(sub);
      // advance input arc ptr
      in_ptr++;
      if (in_ptr==arcs->list_pointer[in_list+1]) in_ptr = -1;
    } // if input only place
    
    if (o_pl < i_pl) {
      // this place is connected only as output, add arc card
      expr* add = arcs->value[out_ptr].AddToPlace(Name(), places[o_pl]);
      if (add) exprlist->Append(add);
      // advance output arc ptr
      out_ptr++;
      if (out_ptr==arcs->list_pointer[out_list+1]) out_ptr = -1;
    } // if output only place
    
    if (o_pl == i_pl) {
      // this place is connected both as an inhibitor and as input
      // Add (outcard - incard) 
      expr* add = NULL;
      if ((NULL==arcs->value[out_ptr].proc_card) &&
          (NULL==arcs->value[in_ptr].proc_card)) {
        // constant cardinality for both arcs, produce constant delta
        int delta = arcs->value[out_ptr].const_card - arcs->value[in_ptr].const_card;
        add = ChangeStateVar(Name(), places[i_pl], delta, NULL, -1);
      } else {
        // expressions for one or more arcs, produce expression delta
        expr* incard = arcs->value[in_ptr].MakeCard();
        expr* outcard = arcs->value[out_ptr].MakeCard();
        expr* delta = MakeBinaryOp(outcard, MINUS, incard, NULL, -1);
        add = ChangeStateVar(Name(), places[i_pl], PLUS, delta, NULL, -1);
      }
      if (add) exprlist->Append(add);
      // advance input arc ptr
      in_ptr++;
      if (in_ptr==arcs->list_pointer[in_list+1]) in_ptr = -1;
      // advance output arc ptr
      out_ptr++;
      if (out_ptr==arcs->list_pointer[out_list+1]) out_ptr = -1;
    } // both input and output
  } // while

  // have list of state changes, build sequence of them
  int numopnds = exprlist->Length();
  if (numopnds==0) return NULL;
  expr** opnds = exprlist->Copy();  
  exprlist->Clear();
  return MakeAssocOp(SEMI, opnds, numopnds, NULL, -1);
}

void spn_dsm::isEnabled(int e, const state &s, result &answer)
{
  CHECK_RANGE(0, e, NumEvents());
  answer.Clear();
  answer.bvalue = true;
  // go through input arcs
  int ptr;
  int list = transitions[e]->inputs;
  if (list>=0) {
    for (ptr=arcs->list_pointer[list]; ptr<arcs->list_pointer[list+1]; ptr++) {
      int i = arcs->value[ptr].place;
      if (s.Read(i).ivalue < arcs->value[ptr].const_card) {
	answer.bvalue = false;
	return;
      }   
      if (NULL==arcs->value[ptr].proc_card) continue;
      int foo = s.Read(i).ivalue - arcs->value[ptr].const_card;
      result x;
      arcs->value[ptr].proc_card->Compute(s, 0, x);
      if (!x.isNormal()) { answer = x; return; }
      if (foo < x.ivalue) {
	answer.bvalue = false;
	return;
      }
    } // for ptr
  }
  // go through inhibitors
  list = transitions[e]->inhibitors;
  if (list>=0) {
    for (ptr=arcs->list_pointer[list]; ptr<arcs->list_pointer[list+1]; ptr++) {
      int i = arcs->value[ptr].place;
      if (s.Read(i).ivalue >= arcs->value[ptr].const_card) {
	answer.bvalue = false;
	return;
      }   
      if (NULL==arcs->value[ptr].proc_card) continue;
      int foo = s.Read(i).ivalue - arcs->value[ptr].const_card;
      result x;
      arcs->value[ptr].proc_card->Compute(s, 0, x);
      if (!x.isNormal()) { answer = x; return; }
      if (foo >= x.ivalue) {
	answer.bvalue = false;
	return;
      }
    } // for ptr
  }
  // guard
  if (NULL==transitions[e]->guard) return;
  transitions[e]->guard->Compute(s, 0, answer);
}

void spn_dsm::getNextState(const state &cur, int e, state &nxt, result &err)
{
  CHECK_RANGE(0, e, NumEvents());
  err.Clear();
  // go through input arcs, subtract tokens
  int ptr;
  int list = transitions[e]->inputs;
  if (list>=0) {
    for (ptr=arcs->list_pointer[list]; ptr<arcs->list_pointer[list+1]; ptr++) {
      int i = arcs->value[ptr].place;
      nxt[i].ivalue -= arcs->value[ptr].const_card;
      if (NULL==arcs->value[ptr].proc_card) continue;
      arcs->value[ptr].proc_card->Compute(cur, 0, err);
      if (!err.isNormal()) return;
      nxt[i].ivalue -= err.ivalue;
    } // for ptr
  }
  // go through output arcs, add tokens
  list = transitions[e]->outputs;
  if (list>=0) {
    for (ptr=arcs->list_pointer[list]; ptr<arcs->list_pointer[list+1]; ptr++) {
      int i = arcs->value[ptr].place;
      nxt[i].ivalue += arcs->value[ptr].const_card;
      if (NULL==arcs->value[ptr].proc_card) continue;
      arcs->value[ptr].proc_card->Compute(cur, 0, err);
      if (!err.isNormal()) return;
      nxt[i].ivalue += err.ivalue;
    } // for ptr
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
  void AddInput(int place, int trans, expr* card, const char *fn, int ln);
  void AddOutput(int trans, int place, expr* card, const char *fn, int ln);
  void AddInhibitor(int place, int trans, expr* card, const char *fn, int ln);
  void AddGuard(int trans, expr* guard);
  void AddFiring(int trans, expr* firing);
  void AddInit(int place, int tokens, const char *fn, int ln);

  // Required for models:
  virtual model_var* MakeModelVar(const char *fn, int l, type t, char* n);
  virtual void InitModel();
  virtual void FinalizeModel(result &);
  virtual state_model* BuildStateModel(const char *fn, int ln);
protected:
  // true if unique arc
  bool ListAdd(int list,  spn_arcinfo &x, expr* card);
};

// ******************************************************************
// *                       spn_model  methods                       *
// ******************************************************************

spn_model::spn_model(const char *fn, int line, type t, char *n, 
 formal_param **pl, int np) : model(fn, line, t, n, pl, np)
{
  placelist = NULL;
  translist = NULL;
  arcs = NULL;
  initial = NULL;
}

spn_model::~spn_model()
{
  // These *should* be null already
  delete placelist;
  delete translist;
  delete arcs;
  delete initial;
}

bool spn_model::ListAdd(int list,  spn_arcinfo &data, expr* card)
{
  if (NULL==card) {
    data.const_card = 1;
    data.proc_card = NULL;
  } else if (card->Type(0) == INT) {
    // constant cardinality, compute it
    result x;
    SafeCompute(card, 0, x);
    if (x.isNormal()) {
      data.const_card = x.ivalue;
      data.proc_card = NULL;
    } else {
      // error?
      data.const_card = 0;
      data.proc_card = card->Substitute(0);
    }
  } else {
    // proc int
    data.const_card = 0;
    data.proc_card = card->Substitute(0);
  }

  DCASSERT(arcs);
  return (arcs->AddItemInOrder(list, data) == arcs->NumItems()-1);
}

void spn_model::AddInput(int place, int trans, expr* card, const char *fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  CHECK_RANGE(0, trans, translist->Length());

  transition *t = translist->Item(trans);
  if (t->inputs<0) t->inputs = arcs->NewList();

  int list = t->inputs;
  spn_arcinfo data;
  data.place = place;
  data.filename = fn;
  data.linenumber = ln;

  if (ListAdd(list, data, card)) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate arc\n";
  Warning << "\tfrom " << placelist->Item(place);
  Warning << " to " << translist->Item(trans);
  Warning << " in SPN " << Name();
  Warning.Stop();
}

void spn_model::AddOutput(int trans, int place, expr* card, const char *fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  CHECK_RANGE(0, trans, translist->Length());

  transition *t = translist->Item(trans);
  if (t->outputs<0) t->outputs = arcs->NewList();

  int list = t->outputs;
  spn_arcinfo data;
  data.place = place;
  data.filename = fn;
  data.linenumber = ln;

  if (ListAdd(list, data, card)) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate arc\n";
  Warning << "\tfrom " << translist->Item(trans);
  Warning << " to " << placelist->Item(place);
  Warning << " in SPN " << Name();
  Warning.Stop();
}

void spn_model::AddInhibitor(int place, int trans, expr* card, const char *fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  CHECK_RANGE(0, trans, translist->Length());

  transition *t = translist->Item(trans);
  if (t->inhibitors<0)
	t->inhibitors = arcs->NewList();

  int list = t->inhibitors;
  spn_arcinfo data;
  data.place = place;
  data.filename = fn;
  data.linenumber = ln;

  if (ListAdd(list, data, card)) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate inhibitor arc\n";
  Warning << "\tfrom " << placelist->Item(place);
  Warning << " to " << translist->Item(trans);
  Warning << " in SPN " << Name();
  Warning.Stop();
}

void spn_model::AddGuard(int trans, expr* guard)
{
  CHECK_RANGE(0, trans, translist->Length());
  transition *t = translist->Item(trans);
  if (NULL==t->guard) {
    t->guard = guard->Substitute(0);
    return;
  }
  Warning.Start(guard->Filename(), guard->Linenumber());
  Warning << "Merging duplicate guard on transition " << t;
  Warning << " in SPN " << Name();
  Warning.Stop(); 
  expr* merge = MakeBinaryOp(t->guard, AND, guard->Substitute(0), NULL, -1);
  t->guard = merge;
}

void spn_model::AddFiring(int trans, expr* firing)
{
  CHECK_RANGE(0, trans, translist->Length());
  transition *t = translist->Item(trans);
  if (NULL==t->firing) {
    t->firing = firing->Substitute(0);
    return;
  }
  Warning.Start(firing->Filename(), firing->Linenumber());
  Warning << "Ignoring duplicate firing for transition " << t;
  Warning << " in SPN " << Name();
  Warning.Stop();
}

void spn_model::AddInit(int place, int tokens, const char* fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  DCASSERT(tokens);
  int z = initial->BinarySearchIndex(place);
  if (z<0) { // no initialization yet for this place
    initial->SortedAppend(place, tokens);
    return;
  }
  initial->value[z] += tokens;
  Warning.Start(fn, ln);
  Warning << "Summing duplicate initialization for place ";
  Warning << placelist->Item(place) << " in SPN " << Name();
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
	Internal << "Bad type for SPN model variable: " << GetType(t) << "\n";
	Internal.Stop();
	return NULL;  // Never get here
  }
#ifdef DEBUG_SPN
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
    Warning << "Assuming zero initial marking in SPN " << Name();
    Warning.Stop();
  }
#ifdef DEBUG_SPN
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
    if (t->inputs>=0) {
      Output << "\tinputs: ";
      arcs->ShowNodeList(Output, t->inputs);
    }
    if (t->outputs>=0) {
      Output << "\toutputs: ";
      arcs->ShowNodeList(Output, t->outputs);
    }
    if (t->inhibitors>=0) {
      Output << "\tinhibitors: ";
      arcs->ShowNodeList(Output, t->inhibitors);
    }
    Output.flush();
  }
#endif
  // success:
  x.Clear();
  x.notFreeable();
  x.other = this;
}

state_model* spn_model::BuildStateModel(const char* fn, int ln)
{
  int num_places = placelist->Length();
  model_var** plist = placelist->MakeArray();
  delete placelist; 
  placelist = NULL;

  int num_trans = translist->Length();
  transition** tlist = translist->MakeArray();
  delete translist;
  translist = NULL;

  arcs->ConvertToStatic();

  return new spn_dsm(Name(), 
  			plist, num_places, 
			tlist, num_trans, 
			initial, arcs,
			fn, ln);
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
#ifdef DEBUG_SPN
  Output << "Inside init for spn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_SPN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result pl, tk;
    SafeCompute(pp[i], 0, pl);
    SafeCompute(pp[i], 1, tk);
    // error checking of pl and tk here   
    spn->AddInit(pl.ivalue, tk.ivalue, pp[i]->Filename(), pp[i]->Linenumber());
  }
#ifdef DEBUG_SPN
  Output << "Exiting init for spn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

void Add_spn_init(PtrTable *fns)
{
  const char* helpdoc = "Assigns the initial marking.  Any places not listed are assumed to be initially empty.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(SPN, "net");
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

#ifdef DEBUG_SPN
  Output << "Inside arcs for spn " << spn << "\n";
  Output.flush();
#endif

  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_SPN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result first;
    result second;
    SafeCompute(pp[i], 0, first);
    SafeCompute(pp[i], 1, second); 
 
    // error checking of first and second here   
 
    expr* card = NULL;
    if (pp[i]->NumComponents()==3) card = pp[i]->GetComponent(2);
    switch (pp[i]->Type(0)) {
      case PLACE:
        spn->AddInput(first.ivalue, second.ivalue, card, pp[i]->Filename(), pp[i]->Linenumber());
        break;

      case TRANS:
        spn->AddOutput(first.ivalue, second.ivalue, card, pp[i]->Filename(), pp[i]->Linenumber());
        break;
      
      default:
        Internal.Start(__FILE__, __LINE__, pp[i]->Filename(), pp[i]->Linenumber());
	Internal << "Bad parameter for spn function arcs\n";
	Internal.Stop();
    }
    
    // stuff here
  }

#ifdef DEBUG_MC
  Output << "Exiting arcs for spn " << spn << "\n";
  Output.flush();
#endif

  x.setNull();
}

int typecheck_arcs(List <expr> *params)
{
  // Note: hidden parameter SPN has NOT been added yet...
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

#ifdef DEBUG_SPN
  Output << "Inside inhibit for spn " << spn << "\n";
  Output.flush();
#endif

  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_SPN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result first;
    result second;
    SafeCompute(pp[i], 0, first);
    SafeCompute(pp[i], 1, second); 
 
    // error checking of first and second here   
 
    expr* card = NULL;
    if (pp[i]->NumComponents()==3) card = pp[i]->GetComponent(2);

    spn->AddInhibitor(first.ivalue, second.ivalue, card, pp[i]->Filename(), pp[i]->Linenumber());
    
  }

#ifdef DEBUG_MC
  Output << "Exiting inhibit for spn " << spn << "\n";
  Output.flush();
#endif

  x.setNull();
}


int typecheck_inhibit(List <expr> *params)
{
  // Note: hidden parameter SPN has NOT been added yet...
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
  const char* helpdoc = "\b(describe arguments)\nAdds inhibitor arcs to a Petri net";

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
#ifdef DEBUG_SPN
  Output << "Inside guard for spn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_SPN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result t;
    SafeCompute(pp[i], 0, t);
    // error checking of t here   
    spn->AddGuard(t.ivalue, pp[i]->GetComponent(1));
  }
#ifdef DEBUG_SPN
  Output << "Exiting guard for spn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

void Add_spn_guard(PtrTable *fns)
{
  const char* helpdoc = "Assigns guard expressions to transitions.  If the guard evaluates to false then the transition is disabled.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(SPN, "net");
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
#ifdef DEBUG_SPN
  Output << "Inside firing for spn " << spn << "\n";
#endif
  x.Clear();
  int i;
  for (i=1; i<np; i++) {
    DCASSERT(pp[i]);
    DCASSERT(pp[i]!=ERROR);
#ifdef DEBUG_SPN
    Output << "\tparameter " << i << " is " << pp[i] << "\n";
#endif
    result t;
    SafeCompute(pp[i], 0, t);
    // error checking of t here   
    spn->AddFiring(t.ivalue, pp[i]->GetComponent(1));
  }
#ifdef DEBUG_SPN
  Output << "Exiting firing for spn " << spn << "\n";
  Output.flush();
#endif
  x.setNull();
}

int typecheck_firing(List <expr> *params)
{
  // Note: hidden parameter SPN has NOT been added yet...
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
  const char* helpdoc = "\b(..., trans : distribution, ...)\nAssigns firing distributions to transitions.";

  internal_func *p = new internal_func(VOID, "firing", 
	compute_spn_firing, NULL, NULL, 0, helpdoc);  
  p->setWithinModel();
  p->SetSpecialTypechecking(typecheck_firing);
  p->SetSpecialParamLinking(linkparams_firing);
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
  // error checking here...
#ifdef DEBUG
  Output << "\tgot param: ";
  PrintResult(Output, INT, x);
  Output << "\n";
  Output.flush();
#endif
  int place = x.ivalue;

  // error checking here for m, place out of bounds, etc.

  x = m.Read(place);
}

void Add_spn_tk(PtrTable *fns)
{
  const char* helpdoc = "Returns the number of tokens in place p";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(SPN, "net");
  pl[1] = new formal_param(PLACE, "p");
  internal_func *p = new internal_func(PROC_INT, "tk", 
	compute_spn_tk, NULL, pl, 2, helpdoc);  
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

  Add_spn_tk(t);

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

