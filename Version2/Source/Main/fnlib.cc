
// $Id$

#include "api.h"
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

  Output.flush();
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
    switch (p->Type(0)) {
      case BOOL:
      case INT:
      case REAL:
      case STRING:
      	break;
      default:
        return -1;
    }
    // width?
    if (p->NumComponents()==1) continue;
    if (p->Type(1)!=INT) return -1;
    if (p->NumComponents()==2) continue;
    // precision, but only for reals
    if (p->Type(0)!=REAL) return -1;
    if (p->Type(2)!=INT) return -1;
    if (p->NumComponents()==3) continue;
    // that should be all
    return -1;
  }
  return 0;
}

bool linkparams_print(expr **p, int np)
{
  // do we need to do anything?
  return true;
}

void do_print(expr **p, int np, result &x, OutputStream &s)
{
  int i;
  result y;
  for (i=0; i<np; i++) {
    x.Clear();
    p[i]->Compute(0, x);
    int width = -1;
    int prec = -1;
    if (p[i]->NumComponents()>1) {
      y.Clear();
      p[i]->Compute(1, y);
      if (!y.infinity && !y.null && !y.error) width = y.ivalue;
      if (p[i]->NumComponents()>2) {
        y.Clear();
	p[i]->Compute(2, y);
        if (!y.infinity && !y.null && !y.error) prec = y.ivalue;
      }
    }
    PrintResult(s, p[i]->Type(0), x, width, prec);
  }
}

void compute_print(expr **p, int np, result &x)
{
  do_print(p, np, x, Output);
  Output.flush();
}

void AddPrint(PtrTable *fns)
{
  const char* helpdoc = "\b(arg1, arg2, ...)\n\tPrint each argument to output\n\t(talk about width specifiers when they are implemented,\n\tand special chars)";

  internal_func *p =
    new internal_func(VOID, "print", compute_print, NULL, NULL, 0, helpdoc);

  p->SetSpecialTypechecking(typecheck_print);
  p->SetSpecialParamLinking(linkparams_print);

  InsertFunction(fns, p);
}

void compute_sprint(expr **p, int np, result &x)
{
  static StringStream strbuffer;
  do_print(p, np, x, strbuffer);
  if (x.error || x.null) return;
  char* bar = strbuffer.GetString();
  strbuffer.flush();
  x.Clear();
  x.other = bar;
}

void AddSprint(PtrTable *fns)
{
  const char* helpdoc = "\b(arg1, arg2, ...)\n\tPrint each argument to a string\n\t(talk about width specifiers when they are implemented,\n\tand special chars)";

  internal_func *p =
    new internal_func(STRING, "sprint", compute_sprint, NULL, NULL, 0, helpdoc);

  p->SetSpecialTypechecking(typecheck_print);
  p->SetSpecialParamLinking(linkparams_print);

  InsertFunction(fns, p);
}


// ********************************************************
// *                  Input  functions                    *
// ********************************************************

void compute_read_int(expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==1);

}

// ********************************************************
// *                   File functions                     *
// ********************************************************

void compute_errorfile(expr **p, int np, result &x)
{
  DCASSERT(np==1);
  x.Clear();
  if (p[0]==NULL) {
    Error.SwitchDisplay(NULL);
    x.bvalue = true;
    return;
  }
  p[0]->Compute(0, x);
  if (x.error) return;
  if (x.null) {
    Error.SwitchDisplay(NULL);
    x.Clear();
    x.bvalue = true;
    return;
  }
  char* filename = (char*) x.other;
  FILE* outfile = fopen(filename, "a");
  if (NULL==outfile) {
    // error, print message?
    x.bvalue = false;
  } else {
    Error.SwitchDisplay(outfile);
    x.bvalue = true;
  }
}

void AddErrorFile(PtrTable *fns)
{
  const char* helpdoc = "Append the error stream to the specified filename.  \n\tIf the file does not exist, it is created.  \n\tIf the filename is null, the error stream is switched to standard error. \n\tReturns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "ErrorFile", compute_errorfile, NULL, pl, 1, helpdoc);

  InsertFunction(fns, p);
}


void compute_outputfile(expr **p, int np, result &x)
{
  DCASSERT(np==1);
  x.Clear();
  if (p[0]==NULL) {
    Output.SwitchDisplay(NULL);
    x.bvalue = true;
    return;
  }
  p[0]->Compute(0, x);
  if (x.error) return;
  if (x.null) {
    Output.SwitchDisplay(NULL);
    x.Clear();
    x.bvalue = true;
    return;
  }
  char* filename = (char*) x.other;
  FILE* outfile = fopen(filename, "a");
  if (NULL==outfile) {
    // error, print message?
    x.bvalue = false;
  } else {
    Output.SwitchDisplay(outfile);
    x.bvalue = true;
  }
}

void AddOutputFile(PtrTable *fns)
{
  const char* helpdoc = "Append the output stream to the specified filename.  \n\tIf the file does not exist, it is created.  \n\tIf the filename is null, the output stream is switched to standard output. \n\tReturns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "OutputFile", compute_outputfile, NULL, pl, 1, helpdoc);

  InsertFunction(fns, p);
}

// ********************************************************
// *               system-like  functions                 *
// ********************************************************

void compute_exit(expr **p, int np, result &x)
{
  int code = 0;
  if (p[0]) {
    p[0]->Compute(0, x);
    if (!x.null && !x.error && !x.infinity) {
      code = x.ivalue;
    }
  }
  smart_exit();
  exit(code);
}

void AddExit(PtrTable *fns)
{
  const char* helpdoc = "Exit smart with specified return code.";
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(INT, "code");

  internal_func *p = 
    new internal_func(VOID, "exit", compute_exit, NULL, pl, 1, helpdoc);

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
  if (b.bvalue) {
    if (pp[1]) pp[1]->Compute(0, x);
    else x.null = true;
  } else {
    if (pp[2]) pp[2]->Compute(0, x); 
    else x.null = true;
  }
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
  // I/O
  AddPrint(t);
  AddSprint(t);
  // Files
  AddErrorFile(t);
  AddOutputFile(t);
  // System stuff
  AddExit(t);
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
