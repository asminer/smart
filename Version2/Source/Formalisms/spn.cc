
// $Id$

#include "spn.h"

#include "../Base/api.h"
#include "../Language/api.h"
#include "../Main/tables.h"
#include "../Templates/listarray.h"

#include "dsm.h"

#define DEBUG_SPN

// ******************************************************************
// *                                                                *
// *                          spn_dsm class                         *
// *                                                                *
// ******************************************************************

/** A discrete-state model (internel representation) for Stochastic Petri nets.
*/

// ******************************************************************
// *                                                                *
// *                        spn_model  class                        *
// *                                                                *
// ******************************************************************

/** Smart support for the Petri net "formalism".
*/
class spn_model : public model {
public:
  struct tinfo {
    int inputs;
    int outputs;
    int inhibitors;
  };
  struct arcinfo {
    int place;
    int const_card;  
    expr* proc_card;
    inline bool operator>(const arcinfo &a) const { return place > a.place; }
    inline bool operator==(const arcinfo &a) const { return place == a.place; }
    void operator+=(const arcinfo &a) {
	DCASSERT(0);
    }
  };
protected:
  List <model_var> *placelist;
  List <model_var> *translist;
  DataList <tinfo> *transinfo;
  listarray <arcinfo> *arcs;
public:
  spn_model(const char* fn, int line, type t, char *n, 
		formal_param **pl, int np);

  virtual ~spn_model();

  // For construction
  void AddInput(int place, int trans, expr* card, const char *fn, int ln);
  void AddOutput(int trans, int place, expr* card, const char *fn, int ln);
  void AddInhibitor(int place, int trans, expr* card, const char *fn, int ln);

  // Required for models:
  virtual model_var* MakeModelVar(const char *fn, int l, type t, char* n);
  virtual void InitModel();
  virtual void FinalizeModel(result &);
  virtual state_model* BuildStateModel();
protected:
  // true if unique arc
  bool ListAdd(int list,  arcinfo &x, expr* card);
};

OutputStream& operator<< (OutputStream& s, const spn_model::arcinfo &a)
{
  s << "Place: " << a.place << "\t const_card: " << a.const_card;
  s << "\t proc_card: " << a.proc_card;
  return s;
}

// ******************************************************************
// *                       spn_model  methods                       *
// ******************************************************************

spn_model::spn_model(const char *fn, int line, type t, char *n, 
 formal_param **pl, int np) : model(fn, line, t, n, pl, np)
{
  placelist = NULL;
  translist = NULL;
  transinfo = NULL;
  arcs = NULL;
}

spn_model::~spn_model()
{
  // These *should* be null already
  delete placelist;
  delete translist;
  delete transinfo;
  delete arcs;
}

bool spn_model::ListAdd(int list,  arcinfo &data, expr* card)
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
      data.proc_card = Copy(card);
    }
  } else {
    // proc int
    data.const_card = 0;
    data.proc_card = Copy(card);
  }

  DCASSERT(arcs);
  return (arcs->AddItemInOrder(list, data) == arcs->NumItems()-1);
}

void spn_model::AddInput(int place, int trans, expr* card, const char *fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  CHECK_RANGE(0, trans, translist->Length());

  if (transinfo->data[trans].inputs<0)
	transinfo->data[trans].inputs = arcs->NewList();

  int list = transinfo->data[trans].inputs;
  arcinfo data;
  data.place = place;

  if (ListAdd(list, data, card)) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate arc\n";
  Warning << "\tfrom " << placelist->Item(place);
  Warning << " to " << translist->Item(trans);
  Warning << "in SPN " << Name();
  Warning.Stop();
}

void spn_model::AddOutput(int trans, int place, expr* card, const char *fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  CHECK_RANGE(0, trans, translist->Length());

  if (transinfo->data[trans].outputs<0)
	transinfo->data[trans].outputs = arcs->NewList();

  int list = transinfo->data[trans].outputs;
  arcinfo data;
  data.place = place;

  if (ListAdd(list, data, card)) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate arc\n";
  Warning << "\tfrom " << translist->Item(trans);
  Warning << " to " << placelist->Item(place);
  Warning << "in SPN " << Name();
  Warning.Stop();
}

void spn_model::AddInhibitor(int place, int trans, expr* card, const char *fn, int ln)
{
  CHECK_RANGE(0, place, placelist->Length());
  CHECK_RANGE(0, trans, translist->Length());

  if (transinfo->data[trans].inhibitors<0)
	transinfo->data[trans].inhibitors = arcs->NewList();

  int list = transinfo->data[trans].inhibitors;
  arcinfo data;
  data.place = place;

  if (ListAdd(list, data, card)) return;

  // Duplicate entry, give warning
  Warning.Start(fn, ln);
  Warning << "Summing cardinalities on duplicate inhibitor arc\n";
  Warning << "\tfrom " << placelist->Item(place);
  Warning << " to " << translist->Item(trans);
  Warning << "in SPN " << Name();
  Warning.Stop();
}


model_var* spn_model::MakeModelVar(const char *fn, int l, type t, char* n)
{
  model_var* s = new model_var(fn, l, t, n);
  int index;
  switch (t) {
    case PLACE:
        index = placelist->Length();
	s->SetIndex(index);
	placelist->Append(s);
	break;

    case TRANS:
	index = translist->Length();
	s->SetIndex(index);
	translist->Append(s);
        transinfo->AppendBlank(); 
        transinfo->data[index].inputs = -1;
        transinfo->data[index].outputs = -1;
        transinfo->data[index].inhibitors = -1;
	break;

    default:
	Internal.Start(__FILE__, __LINE__, fn, l);
	Internal << "Bad type for SPN model variable: " << GetType(t) << "\n";
	Internal.Stop();
	return NULL;  // Never get here
  }
#ifdef DEBUG_SPN
  Output << "\tModel " << Name() << " created " << GetType(t);
  Output << " " << s << " index " << index << "\n"; 
  Output.flush();
#endif
  return s;
}

void spn_model::InitModel()
{
  DCASSERT(NULL==placelist);
  DCASSERT(NULL==translist); 
  placelist = new List <model_var> (16);
  translist = new List <model_var> (16);
  transinfo = new DataList <spn_model::tinfo> (16);
  arcs = new listarray <arcinfo>;
}

void spn_model::FinalizeModel(result &x)
{
  int i;
  for (i=0; i<transinfo->Length(); i++) {
    Output << "Transition " << translist->Item(i) << "\n";
    if (transinfo->data[i].inputs>=0) {
      Output << "\tinputs: ";
      arcs->ShowNodeList(Output, transinfo->data[i].inputs);
    }
    if (transinfo->data[i].outputs>=0) {
      Output << "\toutputs: ";
      arcs->ShowNodeList(Output, transinfo->data[i].outputs);
    }
    if (transinfo->data[i].inhibitors>=0) {
      Output << "\tinhibitors: ";
      arcs->ShowNodeList(Output, transinfo->data[i].inhibitors);
    }
  }
  // success:
  x.Clear();
  x.notFreeable();
  x.other = this;
}

state_model* spn_model::BuildStateModel()
{
  return NULL;
}

// ******************************************************************
// *                                                                *
// *                      PN-specific functions                     *
// *                                                                *
// ******************************************************************

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
  Add_spn_arcs(t);
  Add_spn_inhibit(t);

  Add_spn_tk(t);
}

