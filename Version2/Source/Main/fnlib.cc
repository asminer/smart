
// $Id$

#include "api.h"
#include "fnlib.h"
#include "compile.h"
#include "../Rng/rng.h"
#include "../Formalisms/api.h"
#include <math.h>

extern PtrTable Builtins;

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
  help_search_string = "";
  DCASSERT(np==1);
  DCASSERT(pp); 
  x.Clear();
  SafeCompute(pp[0], 0, x);
  if (x.isNormal()) 
    help_search_string = (char*) x.other;

  // Look through functions
  Builtins.Traverse(ShowDocs);

  // Look through model functions
  HelpModelFuncs();

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
  x.setNull();
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
    if (NULL==p) continue;
    if (ERROR==p) return -1;
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
    SafeCompute(p[i], 0, x);
    int width = -1;
    int prec = -1;
    if (NumComponents(p[i])>1) {
      y.Clear();
      SafeCompute(p[i], 1, y);
      if (y.isNormal()) width = y.ivalue;
      if (NumComponents(p[i])>2) {
        y.Clear();
	SafeCompute(p[i], 2, y);
        if (y.isNormal()) prec = y.ivalue;
      }
    }
    PrintResult(s, Type(p[i], 0), x, width, prec);
  }
}

void compute_print(expr **p, int np, result &x)
{
  do_print(p, np, x, Output);
  if (IsInteractive()) Output.flush();
  else Output.Check();
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
  if (!x.isNormal()) return;
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
  DCASSERT(p);
  x.Clear();
  SafeCompute(p[0], 0, x);
  if (x.isError()) return;
  if (x.isNull()) {
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


void compute_warningfile(expr **p, int np, result &x)
{
  DCASSERT(np==1);
  DCASSERT(p);
  x.Clear();
  SafeCompute(p[0], 0, x);
  if (x.isError()) return;
  if (x.isNull()) {
    Warning.SwitchDisplay(NULL);
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
    Warning.SwitchDisplay(outfile);
    x.bvalue = true;
  }
}

void AddWarningFile(PtrTable *fns)
{
  const char* helpdoc = "Append the warning stream to the specified filename.  \n\tIf the file does not exist, it is created.  \n\tIf the filename is null, the warning stream is switched to standard error. \n\tReturns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "WarningFile", compute_warningfile, NULL, pl, 1, helpdoc);

  InsertFunction(fns, p);
}


void compute_outputfile(expr **p, int np, result &x)
{
  DCASSERT(np==1);
  DCASSERT(p);
  x.Clear();
  SafeCompute(p[0], 0, x);
  if (x.isError()) return;
  if (x.isNull()) {
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

// ******************************************************************
// *                                                                *
// *                        Math functions                          *
// *                                                                *
// ******************************************************************

// ********************************************************
// *                        div                           *
// ********************************************************

// x = a div b; checks errors
// expression err is used only to obtain filename and linenumber
// in case of an error.
inline void div(const result &a, const result &b, result &x, expr *err)
{
  x.Clear();
  if (a.isNull() || a.isError() || a.isUnknown()) {
    x = a; // propogate the "error"
    return;
  }
  if (b.isNull() || b.isError() || b.isUnknown()) {
    x = b; // propogate the "error"
    return;
  }
  if (a.isNormal() && b.isNormal()) {
    if (b.ivalue == 0) {
      // a div 0
      DCASSERT(err);
      Error.Start(err->Filename(), err->Linenumber());
      Error << "Illegal operation: divide by 0";
      Error.Stop();
      x.setError();
      return;
    }
    // ordinary integer division
    x.ivalue = a.ivalue / b.ivalue;
    return;
  }

  if (a.isInfinity() && b.isInfinity()) {
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Illegal operation: infty / infty";
    Error.Stop();
    x.setError();
    return;
  }
  if (b.isInfinity()) {
    // a div +-infty = 0
    x.ivalue = 0;
    return;
  }
  if (a.isInfinity()) {
    x.setInfinity();
    // infinty div b = +- infinity
    if ((a.ivalue > 0) == (b.ivalue > 0)) {
      // same sign, that's +infinity
      x.ivalue = 1;
    } else {
      // opposite sign, that's -infinity
      x.ivalue = -1;
    }
  }
  // still here? some type of error
  x.setError();
}

void compute_div(expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeCompute(p[0], 0, a);
  SafeCompute(p[1], 0, b);
  div(a, b, x, p[0]);
}

void sample_div(Rng &s, expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeSample(p[0], s, 0, a);
  SafeSample(p[1], s, 0, b);
  div(a, b, x, p[0]);
}

void proc_compute_div(const state &m, expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeCompute(p[0], m, 0, a);
  SafeCompute(p[1], m, 0, b);
  div(a, b, x, p[0]);
}

void proc_sample_div(Rng &s, const state &m, expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeSample(p[0], s, m, 0, a);
  SafeSample(p[1], s, m, 0, b);
  div(a, b, x, p[0]);
}


void AddDiv(PtrTable *fns)
{
  const char* helpdoc = "Integer division: computes int(a/b)";

  // Deterministic version
  formal_param **pl_1 = new formal_param*[2];
  pl_1[0] = new formal_param(INT, "a");
  pl_1[1] = new formal_param(INT, "b");
  internal_func *p1 = 
    new internal_func(INT, "div", compute_div, NULL, pl_1, 2, helpdoc);

  // Random version
  formal_param **pl_2 = new formal_param*[2];
  pl_2[0] = new formal_param(RAND_INT, "a");
  pl_2[1] = new formal_param(RAND_INT, "b");
  internal_func *p2 = 
    new internal_func(RAND_INT, "div", NULL, sample_div, pl_2, 2, helpdoc);

  // Proc version
  formal_param **pl_3 = new formal_param*[2];
  pl_3[0] = new formal_param(PROC_INT, "a");
  pl_3[1] = new formal_param(PROC_INT, "b");
  internal_func *p3 = 
    new internal_func(PROC_INT, "div", compute_div, NULL, pl_3, 2, helpdoc);

  // Proc rand version
  formal_param **pl_4 = new formal_param*[2];
  pl_4[0] = new formal_param(PROC_RAND_INT, "a");
  pl_4[1] = new formal_param(PROC_RAND_INT, "b");
  internal_func *p4 = 
    new internal_func(PROC_RAND_INT, "div", compute_div, NULL, pl_4, 2, helpdoc);

  InsertFunction(fns, p1);
  InsertFunction(fns, p2);
  InsertFunction(fns, p3);
  InsertFunction(fns, p4);
}

// ********************************************************
// *                        mod                           *
// ********************************************************

inline void mod(const result &a, const result &b, result &x, expr *err)
{
  x.Clear();
  if (a.isNull() || a.isError() || a.isUnknown()) {
    x = a; // propogate the "error"
    return;
  }
  if (b.isNull() || b.isError() || b.isUnknown()) {
    x = b; // propogate the "error"
    return;
  }
  if (a.isNormal() && b.isNormal()) {
    if (b.ivalue == 0) {
      // a mod 0
      DCASSERT(err);
      Error.Start(err->Filename(), err->Linenumber());
      Error << "Illegal operation: modulo 0";
      Error.Stop();
      x.setError();
      return;
    }
    // ordinary mod 
    x.ivalue = a.ivalue % b.ivalue;
    return;
  }
  if (a.isInfinity() && b.isInfinity()) {
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Illegal operation: infty mod infty";
    Error.Stop();
    x.setError();
    return;
  }
  if (b.isInfinity()) {
    // a mod +-infty = a
    x.ivalue = a.ivalue;  // should we check signs?
    return;
  }
  if (a.isInfinity()) {
    // +- infty mod b is undefined
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Illegal operation: infty mod " << b.ivalue;
    Error.Stop();
    x.setError();
    return;
  }
  // still here? some type of error
  x.setError();
}

void compute_mod(expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeCompute(p[0], 0, a);
  SafeCompute(p[1], 0, b);
  mod(a, b, x, p[0]);
}

void sample_mod(Rng &s, expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeSample(p[0], s, 0, a);
  SafeSample(p[1], s, 0, b);
  mod(a, b, x, p[0]);
}

void proc_compute_mod(const state &m, expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeCompute(p[0], m, 0, a);
  SafeCompute(p[1], m, 0, b);
  mod(a, b, x, p[0]);
}

void proc_sample_mod(Rng &s, const state &m, expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  SafeSample(p[0], s, m, 0, a);
  SafeSample(p[1], s, m, 0, b);
  mod(a, b, x, p[0]);
}

void AddMod(PtrTable *fns)
{
  const char* helpdoc = "Modulo arithmetic: computes a mod b";
  formal_param **pl;
  internal_func *p;

  // Deterministic version
  pl = new formal_param*[2];
  pl[0] = new formal_param(INT, "a");
  pl[1] = new formal_param(INT, "b");
  p = new internal_func(INT, "mod", compute_mod, NULL, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Random version
  pl = new formal_param*[2];
  pl[0] = new formal_param(RAND_INT, "a");
  pl[1] = new formal_param(RAND_INT, "b");
  p = new internal_func(RAND_INT, "mod", NULL, sample_mod, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_INT, "a");
  pl[1] = new formal_param(PROC_INT, "b");
  p = new internal_func(PROC_INT, "mod", proc_compute_mod, NULL, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc_Rand version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_RAND_INT, "a");
  pl[1] = new formal_param(PROC_RAND_INT, "b");
  p = new internal_func(PROC_RAND_INT, "mod", NULL, proc_sample_mod, pl, 2, helpdoc);
  InsertFunction(fns, p);

}

// ********************************************************
// *                        sqrt                          *
// ********************************************************

// In-place sqrt of x, with error checking
inline void my_sqrt(result &x, expr *err)
{
  if (x.isUnknown() || x.isError() || x.isNull()) return;

  if (x.isInfinity()) {
    if (x.ivalue>0) return;  // sqrt(infty) = infty
  } else {
    if (x.rvalue>=0) {
      x.rvalue = sqrt(x.rvalue);
      return;
    }
  }
  
  // negative square root, error (we don't have complex)
  DCASSERT(err);
  Error.Start(err->Filename(), err->Linenumber());
  Error << "Square root with negative argument: ";
  PrintResult(Error, REAL, x);
  Error.Stop();
  x.setError();
}

void compute_sqrt(expr **p, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(p);
  SafeCompute(p[0], 0, x);
  my_sqrt(x, p[0]);
}

void sample_sqrt(Rng &seed, expr **p, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(p);
  SafeSample(p[0], seed, 0, x);
  my_sqrt(x, p[0]);
}

void AddSqrt(PtrTable *fns)
{
  const char* helpdoc = "The positive square root of x";

  // Deterministic real version
  
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "x");
  internal_func *p =
    new internal_func(REAL, "sqrt", compute_sqrt, NULL, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Random real version

  formal_param **pl2 = new formal_param*[1];
  pl2[0] = new formal_param(RAND_REAL, "x");
  internal_func *p2 =
    new internal_func(RAND_REAL, "sqrt", NULL, sample_sqrt, pl2, 1, helpdoc);
  InsertFunction(fns, p2);
}

// ********************************************************
// *                                                      *
// *                                                      *
// *               Discrete Distributions                 *
// *                                                      *
// *                                                      *
// ********************************************************


// ********************************************************
// *                      Bernoulli                       *
// ********************************************************

inline void bernoulli_sample(Rng &strm, result &x, expr *err)
{
  if (x.isNormal()) {
    if ((x.rvalue>=0.0) && (x.rvalue<=1.0)) {
      x.ivalue = (strm.uniform() < x.rvalue) ? 1 : 0;
      return;
    }
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Bernoulli probability " << x.rvalue << " out of range";
    Error.Stop();
    x.setError();
    return;
  }
  if (x.isInfinity()) {
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Bernoulli probability is infinite";
    Error.Stop();
    x.setError();
    return;
  }
  // other strange values (error, null, unknown) may propogate
}

void sample_bernoulli(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], 0, x);
  bernoulli_sample(strm, x, pp[0]);
}

void rand_sample_bernoulli(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeSample(pp[0], strm, 0, x);
  bernoulli_sample(strm, x, pp[0]);
}

void proc_sample_bernoulli(Rng &strm, const state &m, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], m, 0, x);
  bernoulli_sample(strm, x, pp[0]);
}

void proc_rand_sample_bernoulli(Rng &strm, const state &m, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeSample(pp[0], strm, m, 0, x);
  bernoulli_sample(strm, x, pp[0]);
}


void AddBernoulli(PtrTable *fns)
{
  const char* helpdoc = "Bernoulli distribution: one with probability p, zero otherwise";

  formal_param **pl;
  internal_func *p;

  // PH_INT version
  pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "p");
  p = new internal_func(PH_INT, "bernoulli", 
  	NULL, // Add a function here to return a phase int
	sample_bernoulli, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // RAND_INT version, note parameter 
  pl = new formal_param*[1];
  pl[0] = new formal_param(RAND_REAL, "p");
  p = new internal_func(RAND_INT, "bernoulli", NULL,
	rand_sample_bernoulli, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // PROC_PH_INT version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_REAL, "p");
  p = new internal_func(PROC_PH_INT, "bernoulli", 
  	NULL, // Add a function here to return a phase int
	proc_sample_bernoulli, 
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // PROC_RAND_INT version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_RAND_REAL, "p");
  p = new internal_func(PROC_RAND_INT, "bernoulli", NULL, 
	proc_rand_sample_bernoulli, 
	pl, 1, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                     Equilikely                       *
// ********************************************************

inline void equilikely_sample(Rng &strm, const result &a, const result &b, result &x, expr* err)
{
  x.Clear();
  
  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    x.ivalue = int(a.ivalue + (b.ivalue-a.ivalue+1)*strm.uniform());
    return;
  }
  if (a.isInfinity() || b.isInfinity()) {
    x.setError();
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Equilikely with infinite argument";
    Error.Stop();
    return;
  }
  if (a.isUnknown() || b.isUnknown()) {
    x.setUnknown();
    return;
  }
  if (a.isNull() || b.isNull()) {
    x.setNull();
    return;
  }
  // any other errors here
  x.setError();
}

void sample_equilikely(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeCompute(pp[0], 0, a);
  SafeCompute(pp[1], 0, b);
  equilikely_sample(strm, a, b, x, pp[0]);
}

void rand_sample_equilikely(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeSample(pp[0], strm, 0, a);
  SafeSample(pp[1], strm, 0, b);
  equilikely_sample(strm, a, b, x, pp[0]);
}

void proc_sample_equilikely(Rng &strm, const state &m, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeCompute(pp[0], m, 0, a);
  SafeCompute(pp[1], m, 0, b);
  equilikely_sample(strm, a, b, x, pp[0]);
}

void proc_rand_sample_equilikely(Rng &strm, const state &m, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeSample(pp[0], strm, m, 0, a);
  SafeSample(pp[1], strm, m, 0, b);
  equilikely_sample(strm, a, b, x, pp[0]);
}


void AddEquilikely(PtrTable *fns)
{
  const char* helpdoc = "Distribution: integers [a..b] with equal probability";
  formal_param **pl;
  internal_func *p;

  // Deterministic parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(INT, "a");
  pl[1] = new formal_param(INT, "b");
  p = new internal_func(PH_INT, "equilikely", 
  	NULL, // Add a function here to return a phase int
	sample_equilikely, // function for sampling
	pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Random parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(RAND_INT, "a");
  pl[1] = new formal_param(RAND_INT, "b");
  p = new internal_func(RAND_INT, "equilikely", 
  	NULL, 
	rand_sample_equilikely, 
	pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_INT, "a");
  pl[1] = new formal_param(PROC_INT, "b");
  p = new internal_func(PROC_PH_INT, "equilikely", 
  	NULL,  // add ph-int version
	proc_sample_equilikely, 
	pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc rand parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_RAND_INT, "a");
  pl[1] = new formal_param(PROC_RAND_INT, "b");
  p = new internal_func(PROC_RAND_INT, "equilikely", 
  	NULL,  
	proc_rand_sample_equilikely, 
	pl, 2, helpdoc);
  InsertFunction(fns, p);

}

// ********************************************************
// *                      Geometric                       *
// ********************************************************

inline void geometric_sample(Rng &strm, result &x, expr *err)
{
  if (x.isNormal()) {
    if ((x.rvalue>0.0) && (x.rvalue<1.0)) {
      x.ivalue = int(log(strm.uniform()) / log(x.rvalue));
      return;
    }
    if (0.0 == x.rvalue) return; 
    if (1.0 == x.rvalue) {
      x.setInfinity();
      return;
    }
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Geometric probability " << x.rvalue << " out of range";
    Error.Stop();
    x.setError();
    return;
  }
  if (x.isInfinity()) {
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Geometric probability is infinite";
    Error.Stop();
    x.setError();
    return;
  }
  // other strange values (error, null, unknown) may propogate
}

void sample_geometric(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], 0, x);
  geometric_sample(strm, x, pp[0]);
}

void rand_sample_geometric(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeSample(pp[0], strm, 0, x);
  geometric_sample(strm, x, pp[0]);
}

void proc_sample_geometric(Rng &strm, const state& m, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], m, 0, x);
  geometric_sample(strm, x, pp[0]);
}

void proc_rand_sample_geometric(Rng &strm, const state& m, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeSample(pp[0], strm, m, 0, x);
  geometric_sample(strm, x, pp[0]);
}

void AddGeometric(PtrTable *fns)
{
  const char* helpdoc = "Geometric distribution: Pr(X=x) = (1-p)*p^x";
  formal_param **pl;
  internal_func *p;

  // Deterministic version
  pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "p");
  p = new internal_func(PH_INT, "geometric", 
  	NULL, // Add a function here to return a phase int
	sample_geometric, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Random version
  pl = new formal_param*[1];
  pl[0] = new formal_param(RAND_REAL, "p");
  p = new internal_func(RAND_INT, "geometric", 
  	NULL, 
	rand_sample_geometric, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_REAL, "p");
  p = new internal_func(PROC_PH_INT, "geometric", 
  	NULL,  // add phase here
	proc_sample_geometric, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc-rand version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_RAND_REAL, "p");
  p = new internal_func(PROC_RAND_INT, "geometric", 
  	NULL, 
	proc_rand_sample_geometric, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);
}


// ********************************************************
// *                                                      *
// *                                                      *
// *              Continuous Distributions                *
// *                                                      *
// *                                                      *
// ********************************************************

// ********************************************************
// *                       Uniform                        *
// ********************************************************

inline void uniform(Rng &strm, result &a, result &b, result &x, expr *err)
{
  x.Clear();
  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    x.rvalue = a.rvalue + (b.rvalue-a.rvalue)*strm.uniform();
    return;
  }
  if (a.isInfinity() || b.isInfinity()) {
    x.setError();
    DCASSERT(err);
    Error.Start(err->Filename(), err->Linenumber());
    Error << "Uniform with infinite argument";
    Error.Stop();
    return;
  }
  if (a.isUnknown() || b.isUnknown()) {
    x.setUnknown();
    return;
  }
  if (a.isNull() || b.isNull()) {
    x.setNull();
    return;
  }
  // any other errors here
  x.setError();
}

void sample_uniform(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeCompute(pp[0], 0, a);
  SafeCompute(pp[1], 0, b);
  uniform(strm, a, b, x, pp[0]);
}

void rand_sample_uniform(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeSample(pp[0], strm, 0, a);
  SafeSample(pp[0], strm, 0, b);
  uniform(strm, a, b, x, pp[0]);
}

void proc_sample_uniform(Rng &strm, const state& m, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeCompute(pp[0], m, 0, a);
  SafeCompute(pp[0], m, 0, b);
  uniform(strm, a, b, x, pp[0]);
}

void proc_rand_sample_uniform(Rng &strm, const state& m, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result a,b;
  SafeSample(pp[0], strm, m, 0, a);
  SafeSample(pp[0], strm, m, 0, b);
  uniform(strm, a, b, x, pp[0]);
}

void AddUniform(PtrTable *fns)
{
  const char* helpdoc = "Uniform distribution: reals between (a,b) with equal probability";
  formal_param **pl;
  internal_func *p;

  // Deterministic params
  pl = new formal_param*[2];
  pl[0] = new formal_param(REAL, "a");
  pl[1] = new formal_param(REAL, "b");
  p = new internal_func(RAND_REAL, "uniform", 
	NULL, sample_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Random params
  pl = new formal_param*[2];
  pl[0] = new formal_param(RAND_REAL, "a");
  pl[1] = new formal_param(RAND_REAL, "b");
  p = new internal_func(RAND_REAL, "uniform", 
	NULL, rand_sample_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc params
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_REAL, "a");
  pl[1] = new formal_param(PROC_REAL, "b");
  p = new internal_func(PROC_RAND_REAL, "uniform", 
	NULL, proc_sample_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc-rand params
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_RAND_REAL, "a");
  pl[1] = new formal_param(PROC_RAND_REAL, "b");
  p = new internal_func(PROC_RAND_REAL, "uniform", 
	NULL, proc_rand_sample_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                         Expo                         *
// ********************************************************

void compute_expo(expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], 0, x);
}

void proc_compute_expo(const state& m, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], m, 0, x);
}

void AddExpo(PtrTable *fns)
{
  const char* helpdoc = "Exponential distribution with rate lambda";
  formal_param **pl;
  internal_func *p;

  // Deterministic param
  pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "lambda");
  p = new internal_func(EXPO, "expo", 
	compute_expo, 
	NULL, // sampling is handled in casting.cc
	pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc. param
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_REAL, "lambda");
  p = new internal_func(PROC_EXPO, "expo", 
	proc_compute_expo, 
	NULL, // sampling is handled in casting.cc
	pl, 1, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                probability and such                  *
// ********************************************************

// #define DEBUG_AVG

void compute_avg(expr **p, int np, result &x)
{
  const int N = 20;
  // For testing right now.  Write a better version eventually.
  Rng foo(123456789);
  
  // A bad way to compute the average...
  x.Clear();
  x.rvalue = 0.0;
  int i;
  for (i=0; i<N; i++) {
    result sample;
    p[0]->ClearCache(); // reset samples
    SafeSample(p[0], foo, 0, sample);
    
#ifdef DEBUG_AVG
    Output << "  sampled ";
    PrintResult(Output, REAL, sample);
    Output << "\n";
#endif
   
    if (sample.isNormal()) {
      x.rvalue += sample.rvalue;
      continue;
    }
    if (sample.isInfinity()) {
      if (x.isInfinity()) {
	// if signs don't match, error
	if (SIGN(x.ivalue)!=SIGN(sample.ivalue)) {
	  x.setError();
	  Error.Start(p[0]->Filename(), p[0]->Linenumber());
	  Error << "Undefined value (infinity - infinity) in Avg";
	  Error.Stop();
	  return;
	}
      }
      x = sample;
      continue;
    }
    // if we get here we've got null or an error, bail out
    x = sample;
    return;
  } // for i

  // Divide by N 
  if (x.isInfinity()) return;  // unless we're infinity

  x.rvalue /= N;
}

void AddAvg(PtrTable *fns)
{
  const char* helpdoc = "Computes the expected value of x";

  formal_param **pl2 = new formal_param*[1];
  pl2[0] = new formal_param(RAND_REAL, "x");
  internal_func *p2 =
    new internal_func(REAL, "avg", compute_avg, NULL, pl2, 1, helpdoc);
  InsertFunction(fns, p2);
}


// ********************************************************
// *               system-like  functions                 *
// ********************************************************

void compute_exit(expr **p, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(p);
  int code = 0;
  SafeCompute(p[0], 0, x);
  if (x.isNormal()) {
    code = x.ivalue;
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
// *                        cond                          *
// ********************************************************

void compute_cond(expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  SafeCompute(pp[0], 0, b);
  if (b.isNull() || b.isError()) {
    // error stuff?
    x = b;
    return;
  }
  if (b.bvalue) SafeCompute(pp[1], 0, x);
  else SafeCompute(pp[2], 0, x);
}

void sample_cond(Rng &seed, expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  SafeSample(pp[0], seed, 0, b);
  if (b.isNull() || b.isError()) {
    x = b;
    return;
  }
  if (b.bvalue) SafeSample(pp[1], seed, 0, x);
  else SafeSample(pp[2], seed, 0, x); 
}

void proc_compute_cond(const state &m, expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  SafeCompute(pp[0], m, 0, b);
  if (b.isNull() || b.isError()) {
    // error stuff?
    x = b;
    return;
  }
  if (b.bvalue) SafeCompute(pp[1], m, 0, x);
  else SafeCompute(pp[2], m, 0, x);
}

void proc_sample_cond(Rng &s, const state &m, expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  SafeSample(pp[0], s, m, 0, b);
  if (b.isNull() || b.isError()) {
    // error stuff?
    x = b;
    return;
  }
  if (b.bvalue) SafeSample(pp[1], s, m, 0, x);
  else SafeSample(pp[2], s, m, 0, x);
}

const char* conddoc = "If <b> is true, returns <t>; else, returns <f>.";

void AddCond(type bt, type t, PtrTable *fns)
{
  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(bt, "b");
  pl[1] = new formal_param(t, "t");
  pl[2] = new formal_param(t, "f");

  internal_func *cnd = NULL;
  switch (bt) {
    case BOOL:
      	cnd = new internal_func(t, "cond", compute_cond, NULL, pl, 3, conddoc);
	break;
    
    case RAND_BOOL:
      	cnd = new internal_func(t, "cond", NULL, sample_cond, pl, 3, conddoc);
	break;

    case PROC_BOOL:
      	cnd = new internal_func(t, "cond", proc_compute_cond, NULL, pl, 3, conddoc);
	break;

    case PROC_RAND_BOOL:
      	cnd = new internal_func(t, "cond", NULL, proc_sample_cond, pl, 3, conddoc);
	break;
 
    default:
      	DCASSERT(0);
	return;
  }
  
  InsertFunction(fns, cnd);
}

// ********************************************************
// *                        case                          *
// ********************************************************

void compute_case(expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np>1);
  result c;
  SafeCompute(pp[0], 0, c);
  if (c.isNull() || c.isError()) {
    // error stuff?
    x = c;
    return;
  }
  int i;
  for (i=2; i<np; i++) {
    result m;
    SafeCompute(pp[i], 0, m);
    if (m.isNormal()) if (m.ivalue == c.ivalue) {
      // this is it!
      SafeCompute(pp[i], 1, x);
      return;
    }
  }
  // still here?  use the default value
  SafeCompute(pp[1], 0, x);
}

void sample_case(Rng &s, expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np>1);
  result c;
  SafeSample(pp[0], s, 0, c);
  if (!c.isNormal()) {
    x.setError();
    return;
  }
  int i;
  for (i=2; i<np; i++) {
    result m;
    SafeSample(pp[i], s, 0, m);
    if (m.isNormal()) if (m.ivalue == c.ivalue) {
      // this is it!
      SafeCompute(pp[i], 1, x);
      return;
    }
  }
  // still here?  use the default value
  SafeCompute(pp[1], 0, x);
}

void proc_compute_case(const state &s, expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np>1);
  result c;
  SafeCompute(pp[0], s, 0, c);
  if (c.isNull() || c.isError()) {
    // error stuff?
    x = c;
    return;
  }
  int i;
  for (i=2; i<np; i++) {
    result m;
    SafeCompute(pp[i], s, 0, m);
    if (m.isNormal()) if (m.ivalue == c.ivalue) {
      // this is it!
      SafeCompute(pp[i], s, 1, x);
      return;
    }
  }
  // still here?  use the default value
  SafeCompute(pp[1], s, 0, x);
}

void proc_sample_case(Rng &r, const state &s, expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np>1);
  result c;
  SafeSample(pp[0], r, s, 0, c);
  if (c.isNull() || c.isError()) {
    // error stuff?
    x = c;
    return;
  }
  int i;
  for (i=2; i<np; i++) {
    result m;
    SafeSample(pp[i], r, s, 0, m);
    if (m.isNormal()) if (m.ivalue == c.ivalue) {
      // this is it!
      SafeSample(pp[i], r, s, 1, x);
      return;
    }
  }
  // still here?  use the default value
  SafeSample(pp[1], r, s, 0, x);
}


const char* casedoc = "Match the value <c> with one of the <c:v> pairs. If matched, return the matching v; Otherwise return the default value <dv>.";

void AddCase(type it, type t, PtrTable *fns)
{
  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(it, "c");
  pl[1] = new formal_param(t, "dv");
  type *tl = new type[2];
  tl[0] = it;
  tl[1] = t;
  pl[2] = new formal_param(tl, 2, "cv");

  internal_func *ca = NULL;
  switch (it) {
    case INT:
      	ca = new internal_func(t, "case", compute_case, NULL, pl, 3, 2, casedoc);
	break;
    
    case RAND_INT:
      	ca = new internal_func(t, "case", NULL, sample_case, pl, 3, 2, casedoc);
	break;

    case PROC_INT:
      	ca = new internal_func(t, "case", proc_compute_case, NULL, pl, 3, 2, casedoc);
	break;
    
    case PROC_RAND_INT:
      	ca = new internal_func(t, "case", NULL, proc_sample_case, pl, 3, 2, casedoc);
	break;

    default:
      	DCASSERT(0);
	return;
  }
  
  InsertFunction(fns, ca);
}



// ********************************************************
// *                      is_null                         *
// ********************************************************

void compute_is_null(expr **pp, int np, result &x)
{
  DCASSERT(pp);
  DCASSERT(np==1);
  x.Clear();
  result y;
  SafeCompute(pp[0], 0, y);
  x.bvalue = y.isNull();
}

void AddIsNull(PtrTable *t, type paramtype)
{
  const char* doc = "Returns true if the argument is null.";
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(paramtype, "x");
  internal_func *foo = new internal_func(BOOL, "is_null", compute_is_null, NULL, pl, 1, doc);
  InsertFunction(t, foo);
}

// ********************************************************
// *                      dontknow                        *
// ********************************************************


void compute_dontknow(expr **pp, int np, result &x)
{
  x.Clear();
  x.setUnknown();
}

void AddDontKnow(PtrTable *t)
{
  const char* dkdoc = "Returns a finite, unknown value.";
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(INT, "dummy");
  internal_func *foo = new internal_func(INT, "DontKnow", compute_dontknow, NULL, pl, 1, dkdoc);
  foo->HideDocs();  // this is not for public consumption
  InsertFunction(t, foo);
}

void InitBuiltinFunctions(PtrTable *t)
{
  AddHelp(t);
  // I/O
  AddPrint(t);
  AddSprint(t);
  // Files
  AddErrorFile(t);
  AddWarningFile(t);
  AddOutputFile(t);
  // Arithmetic
  AddDiv(t);
  AddMod(t);
  AddSqrt(t);
  // Discrete distributions
  AddBernoulli(t);
  AddEquilikely(t);
  AddGeometric(t);
  // Continuous distributions
  AddUniform(t);
  AddExpo(t);
  // Probability
  AddAvg(t);
  // System stuff
  AddExit(t);
  // Conditionals
  type i;
  for (i=FIRST_SIMPLE; i<=PH_REAL; i++)		AddCond(BOOL, i, t);
  for (i=RAND_BOOL; i<=RAND_REAL; i++)		AddCond(RAND_BOOL, i, t);
  for (i=PROC_BOOL; i<=PROC_PH_REAL; i++)	AddCond(PROC_BOOL, i, t);
  for (i=PROC_RAND_BOOL; i<=PROC_RAND_REAL; i++)AddCond(PROC_RAND_BOOL, i, t);
  for (i=FIRST_VOID; i<=LAST_VOID; i++)		AddCond(BOOL, i, t);
  // Case
  for (i=FIRST_SIMPLE; i<=PH_REAL; i++)		AddCase(INT, i, t);
  for (i=RAND_BOOL; i<=RAND_REAL; i++)		AddCase(RAND_INT, i, t);
  for (i=PROC_BOOL; i<=PROC_PH_REAL; i++)	AddCase(PROC_INT, i, t);
  for (i=PROC_RAND_BOOL; i<=PROC_RAND_REAL; i++)AddCase(PROC_RAND_INT, i, t);
  for (i=FIRST_VOID; i<=LAST_VOID; i++)		AddCase(INT, i, t);
  // is_null
  for (i=VOID; i<=STATESET; i++) 	AddIsNull(t, i);

  // Misc
  AddDontKnow(t);
}


void InitBuiltinConstants(PtrTable *t)
{
  // Infinity
  constfunc *infty = MakeConstant(INT, "infinity", NULL, -1);
  infty->SetReturn(MakeInfinityExpr(1, NULL, -1));
  t->AddNamePtr("infinity", infty);

  // Other constants? 
}
