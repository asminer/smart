
// $Id$

#include "fnlib.h"
#include "compile.h"

extern PtrTable* Builtins;

//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//

// Used by computecond
char* help_search_string;

void DumpDocs(const char* doc)
{
  // Note: this is in case we want to do fancy stuff, in the future ;^)

  Output << "\t" << doc << "\n";
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
    DumpHeader(Output, hit);
    Output << "\n";
    const char* d = hit->GetDocumentation();
    if (d) DumpDocs(d);
    else { // user defined... 
	Output << "\tdefined in " << hit->Filename() << " near line ";
	Output << hit->Linenumber() << "\n";
    }
    Output << "\n";
  }
}

void computehelp(expr **pp, int np, result &x)
{

Output << "Hello!\n";

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


  Builtins->Traverse(ShowDocs);

  // return something...
  x.null = true;
}

void AddHelp(PtrTable *fns)
{
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "search");

  const char* helpdoc = "Searches on-line help for function and option names containing <search>";

  internal_func *hlp =
    new internal_func(BOOL, "help", computehelp, NULL, pl, 1, helpdoc);

  InsertFunction(fns, hlp);
}


void computecond(expr **pp, int np, result &x)
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

void samplecond(long &seed, expr **pp, int np, result &x)
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
    new internal_func(t, "cond", computecond, samplecond, pl, 3, conddoc);

  InsertFunction(fns, cnd);
}



void InitBuiltinFunctions(PtrTable *t)
{
  AddHelp(t);
  // Conditionals
  type i;
  for (i=FIRST_SIMPLE; i<=LAST_SIMPLE; i++)	AddCond(i, t);
  for (i=FIRST_PROC; i<=LAST_PROC; i++)		AddCond(i, t);
  for (i=FIRST_VOID; i<=LAST_VOID; i++)		AddCond(i, t);
}

