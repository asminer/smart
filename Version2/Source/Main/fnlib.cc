
// $Id$

#include "fnlib.h"
#include "compile.h"

extern PtrTable* Builtins;

//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//

// Used by computehelp
char* help_search_string;

void DumpDocs(const char* doc)
{
  // Note: this is in case we want to do fancy stuff, in the future ;^)
  if (doc[0] != '\b') 
    Output << "\t" << doc << "\n";
  else {
    const char* doc1 = doc+1;
    Output << doc1 << "\n";
  }
}

void ShowDocs(void *x)
{
  PtrTable::splayitem *foo = (PtrTable::splayitem *) x;
  if (NULL==strstr(foo->name, help_search_string)) return;  // not interested...
  bool unshown = true;
  List <function> *bar = (List <function> *)foo->ptr;
  int i;
  for (i=0; i<bar->Length(); i++) {
    function *hit = bar->Item(i);
    if (hit->IsUndocumented()) continue;  // don't show this to the public
    // show to the public
    if (unshown) {
      Output << foo->name << "\n";
      unshown = false;
    }
    Output << "\t";
    hit->ShowHeader(Output);
    if (!hit->HasSpecialTypechecking()) Output << "\n";
    // special type checking: documentation must display parameters, too
    const char* d = hit->GetDocumentation();
    if (d) DumpDocs(d);
    else { // user defined... 
	Output << "\tdefined in "; 
        const char* fn = hit->Filename();
	if (fn[0]=='-' && fn[1]==0) 
	  Output << "standard input";
	else
	  Output << fn;
	Output << " near line " << hit->Linenumber() << "\n";
    }
    Output << "\n";
  }
}

void compute_help(expr **pp, int np, result &x)
{
  DCASSERT(np==1);
  if (NULL==pp) 
    help_search_string = "";
  else if (NULL==pp[0])
    help_search_string = "";
  else {
    x.Clear();
    pp[0]->Compute(0, x);
    help_search_string = (char*) x.other;
  }

  // Look through functions
  Builtins->Traverse(ShowDocs);

  // Now go through options
  int i;
  for (i=0; i<NumOptions(); i++) {
    option* o = GetOptionNumber(i);
    if (NULL==strstr(o->Name(), help_search_string)) continue;
    if (o->IsUndocumented()) continue;
    o->ShowHeader(Output);
    Output << "\n";
    Output << "\t" << o->GetDocumentation() << "\n";
    Output << "\tLegal values: ";
    o->ShowRange(Output);
    Output << "\n";
  }

  // return something...
  x.null = true;
}

void AddHelp(PtrTable *fns)
{
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "search");

  const char* helpdoc = "Searches on-line help for function and option names containing <search>";

  internal_func *hlp =
    new internal_func(VOID, "help", compute_help, NULL, pl, 1, helpdoc);

  InsertFunction(fns, hlp);
}



// ********************************************************
// *                  Output functions                    *
// ********************************************************

int typecheck_print(List <expr> *params)
{
  int np = params->Length();
  int i;
  for (i=0; i<np; i++) {
    expr* p = params->Item(i);
    if (p->NumComponents()>1) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Sorry, print does not handle width specifiers yet\n";
      Internal.Stop();
    }
    switch (p->Type(0)) {
      case BOOL:
      case INT:
      case REAL:
      case STRING:
      	continue;
      default:
        return -1;
    }
  }
  return 0;
}

bool linkparams_print(expr **p, int np)
{
  // do we need to do anything?
  return true;
}

void compute_print(expr **p, int np, result &x)
{
  int i;
  for (i=0; i<np; i++) {
    x.Clear();
    if (x.error) return;
    p[i]->Compute(0, x);
    PrintResult(p[i]->Type(0), x, Output);
  }
  Output.flush();
}

void AddPrint(PtrTable *fns)
{
  const char* helpdoc = "\b(arg1, arg2, ...)\n\tPrint each argument\n\t(talk about width specifiers when they are implemented,\n\tand special chars)";

  internal_func *p =
    new internal_func(VOID, "print", compute_print, NULL, NULL, 0, helpdoc);

  p->SetSpecialTypechecking(typecheck_print);
  p->SetSpecialParamLinking(linkparams_print);

  InsertFunction(fns, p);
}



// ********************************************************
// *                  switch functions                    *
// ********************************************************

void compute_cond(expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==3);
  if (NULL==pp[0]) {
    // some type of error stuff?
    x.null = true;
    return;
  }
  result b;
  b.Clear();
  pp[0]->Compute(0, b);
  if (b.null || b.error) {
    // error stuff?
    x = b;
    return;
  }
  if (b.bvalue) pp[1]->Compute(0, x);
  else pp[2]->Compute(0, x); 
}

void sample_cond(long &seed, expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==3);
  if (NULL==pp[0]) {
    x.null = true;
    return;
  }
  result b;
  b.Clear();
  pp[0]->Sample(seed, 0, b);
  if (b.null || b.error) {
    x = b;
    return;
  }
  if (b.bvalue) pp[1]->Sample(seed, 0, x);
  else pp[2]->Sample(seed, 0, x); 
}

const char* conddoc = "If <b> is true, returns <t>; else, returns <f>.";

void AddCond(type t, PtrTable *fns)
{
  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(BOOL, "b");
  pl[1] = new formal_param(t, "t");
  pl[2] = new formal_param(t, "f");

  internal_func *cnd = 
    new internal_func(t, "cond", compute_cond, sample_cond, pl, 3, conddoc);

  InsertFunction(fns, cnd);
}



void InitBuiltinFunctions(PtrTable *t)
{
  AddHelp(t);
  AddPrint(t);
  // Conditionals
  type i;
  for (i=FIRST_SIMPLE; i<=LAST_SIMPLE; i++)	AddCond(i, t);
  for (i=FIRST_PROC; i<=LAST_PROC; i++)		AddCond(i, t);
  for (i=FIRST_VOID; i<=LAST_VOID; i++)		AddCond(i, t);
}


void InitBuiltinConstants(PtrTable *t)
{
  // Infinity
  constfunc *infty = new determfunc(NULL, -1, INT, "infty");
  infty->SetReturn(MakeInfinityExpr(1, NULL, -1));
  t->AddNamePtr("infty", infty);

  // Other constants? 
}
