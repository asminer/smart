
// $Id$

#include "api.h"
#include "fnlib.h"
#include "compile.h"
#include "../Rng/rng.h"
#include <math.h>

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
  help_search_string = "";
  DCASSERT(np==1);
  DCASSERT(pp); 
  x.Clear();
  SafeCompute(pp[0], 0, x);
  if (x.isNormal()) 
    help_search_string = (char*) x.other;

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

// ********************************************************
// *                   Math functions                     *
// ********************************************************

void compute_div(expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  result a;
  SafeCompute(p[0], 0, a);
  if (a.isNull() || a.isError() || a.isUnknown()) {
    x = a;
    return;
    // null div b = null,
    // ? div b = ?
  }
  SafeCompute(p[1], 0, x);

  if (a.isNormal() && x.isNormal()) {
    if (x.ivalue == 0) {
      // a div 0
      DCASSERT(p[1]);  // otherwise b is null
      Error.Start(p[1]->Filename(), p[1]->Linenumber());
      Error << "Illegal operation: divide by 0";
      Error.Stop();
      x.setError();
      return;
    }
    // ordinary integer division
    x.ivalue = a.ivalue / x.ivalue;
    return;
  }

  if (a.isInfinity() && x.isInfinity()) {
    Error.Start(p[1]->Filename(), p[1]->Linenumber());
    Error << "Illegal operation: infty / infty";
    Error.Stop();
    x.setError();
    return;
  }
  if (x.isInfinity()) {
    // a div +-infty = 0
    x.Clear();
    x.ivalue = 0;
    return;
  }
  if (a.isInfinity()) {
    // infinty div b = +- infinity
    if ((a.ivalue > 0) == (x.ivalue > 0)) {
      // same sign, that's +infinity
      x = a;
      x.ivalue = 1;
    } else {
      // opposite sign, that's -infinity
      x = a;
      x.ivalue = -1;
    }
  }
}

void AddDiv(PtrTable *fns)
{
  const char* helpdoc = "Integer division: computes int(a/b)";
  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(INT, "a");
  pl[1] = new formal_param(INT, "b");

  internal_func *p = 
    new internal_func(INT, "div", compute_div, NULL, pl, 2, helpdoc);

  InsertFunction(fns, p);
}

void compute_mod(expr **p, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(p);
  int a,b;
  SafeCompute(p[0], 0, x);
  if (!x.isNormal()) {
    return;
    // null mod b = null,
    // ? mod b = ?,
    // infty mod b = infty
  }
  a = x.ivalue;
  SafeCompute(p[1], 0, x);
  if (x.isNormal()) {
    b = x.ivalue;
    x.Clear();
    if (b) {
      x.ivalue = a % b;
      return;
    }
    // a mod 0, error
    DCASSERT(p[1]);  // otherwise b is null
    Error.Start(p[1]->Filename(), p[1]->Linenumber());
    Error << "Illegal operation: modulo 0";
    Error.Stop();
    x.setError();
  }
  if (x.isInfinity()) {
    // a mod infty = a
    x.Clear();
    x.ivalue = a;
  }
}

void AddMod(PtrTable *fns)
{
  const char* helpdoc = "Modulo arithmetic: computes a mod b";
  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(INT, "a");
  pl[1] = new formal_param(INT, "b");

  internal_func *p = 
    new internal_func(INT, "mod", compute_mod, NULL, pl, 2, helpdoc);

  InsertFunction(fns, p);
}

void compute_sqrt(expr **p, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(p);
  SafeCompute(p[0], 0, x);

  if (x.isUnknown() || x.isError() || x.isNull()) return;

  if (x.isInfinity()) {
    if (x.ivalue>0) return;  // sqrt(infty) = infty
  } else {
    if (x.rvalue>0) {
      x.rvalue = sqrt(x.rvalue);
      return;
    }
  }
  
  // negative square root, error (we don't have complex)
  Error.Start(p[0]->Filename(), p[0]->Linenumber());
  Error << "Square root with negative argument: ";
  PrintResult(Error, REAL, x);
  Error.Stop();
  x.setError();
}

void sample_sqrt(Rng &seed, expr **p, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(p);
  SafeSample(p[0], seed, 0, x);

  if (x.isUnknown() || x.isError() || x.isNull()) return;

  if (x.isInfinity()) {
    if (x.ivalue>0) return;  // sqrt(infty) = infty
  } else {
    if (x.rvalue>0) {
      x.rvalue = sqrt(x.rvalue);
      return;
    }
  }
  
  // negative square root, error (we don't have complex)
  Error.Start(p[0]->Filename(), p[0]->Linenumber());
  Error << "Square root with negative argument: ";
  PrintResult(Error, REAL, x);
  Error.Stop();
  x.setError();
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

void sample_bernoulli(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);

  SafeCompute(pp[0], 0, x);

  if (x.isNormal()) {
    if ((x.rvalue>=0.0) && (x.rvalue<=1.0)) {
      x.ivalue = (strm.uniform() < x.rvalue) ? 1 : 0;
      return;
    }
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Bernoulli probability " << x.rvalue << " out of range";
    Error.Stop();
    x.setError();
    return;
  }
  if (x.isInfinity()) {
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Bernoulli probability is infinite";
    Error.Stop();
    x.setError();
    return;
  }

  // other strange values (error, null, unknown) may propogate
}

void AddBernoulli(PtrTable *fns)
{
  const char* helpdoc = "Bernoulli distribution: one with probability p, zero otherwise";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "p");
  internal_func *p = new internal_func(PH_INT, "bernoulli", 
  	NULL, // Add a function here to return a phase int
	sample_bernoulli, // function for sampling
	pl, 1, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                     Equilikely                       *
// ********************************************************

void sample_equilikely(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  
  result a,b;

  SafeCompute(pp[0], 0, a);
  SafeCompute(pp[1], 0, b);

  x.Clear();
  
  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    x.ivalue = int(a.ivalue + (b.ivalue-a.ivalue+1)*strm.uniform());
    return;
  }

  if (a.isInfinity() || b.isInfinity()) {
    x.setError();
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
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

void AddEquilikely(PtrTable *fns)
{
  const char* helpdoc = "Distribution: integers [a..b] with equal probability";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(INT, "a");
  pl[1] = new formal_param(INT, "b");
  internal_func *p = new internal_func(PH_INT, "equilikely", 
  	NULL, // Add a function here to return a phase int
	sample_equilikely, // function for sampling
	pl, 2, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                      Geometric                       *
// ********************************************************

void sample_geometric(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(1==np);
  DCASSERT(pp);

  SafeCompute(pp[0], 0, x);

  if (x.isNormal()) {
    if ((x.rvalue>0.0) && (x.rvalue<1.0)) {
      x.ivalue = int(log(strm.uniform()) / log(x.rvalue)) ? 1 : 0;
      return;
    }
    if (0.0 == x.rvalue) return; 
    if (1.0 == x.rvalue) {
      x.setInfinity();
      return;
    }
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Geometric probability " << x.rvalue << " out of range";
    Error.Stop();
    x.setError();
    return;
  }
  if (x.isInfinity()) {
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Geometric probability is infinite";
    Error.Stop();
    x.setError();
    return;
  }

  // other strange values (error, null, unknown) may propogate
}

void AddGeometric(PtrTable *fns)
{
  const char* helpdoc = "Geometric distribution: Pr(X=x) = (1-p)*p^x";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "p");
  internal_func *p = new internal_func(PH_INT, "geometric", 
  	NULL, // Add a function here to return a phase int
	sample_geometric, // function for sampling
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

void sample_uniform(Rng &strm, expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  
  result a,b;

  SafeCompute(pp[0], 0, a);
  SafeCompute(pp[1], 0, b);

  x.Clear();
  
  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    x.rvalue = a.rvalue + (b.rvalue-a.rvalue)*strm.uniform();
    return;
  }

  if (a.isInfinity() || b.isInfinity()) {
    x.setError();
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
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

void AddUniform(PtrTable *fns)
{
  const char* helpdoc = "Uniform distribution: reals between (a,b) with equal probability";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(INT, "a");
  pl[1] = new formal_param(INT, "b");
  internal_func *p = new internal_func(RAND_REAL, "uniform", 
	NULL, sample_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                probability and such                  *
// ********************************************************

void compute_avg(expr **p, int np, result &x)
{
  const int N = 100000;
  // For testing right now.  Write a better version eventually.
  Rng foo(123456789);
  
  // A bad way to compute the average...
  x.Clear();
  x.rvalue = 0.0;
  int i;
  for (i=0; i<N; i++) {
    result sample;
    SafeSample(p[0], foo, 0, sample);
    /*
    Output << "  sampled ";
    PrintResult(Output, REAL, sample);
    Output << "\n";
    */
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
// *                  switch functions                    *
// ********************************************************

void compute_cond(expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  b.Clear();
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
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  b.Clear();
  SafeSample(pp[0], seed, 0, b);
  if (b.isNull() || b.isError()) {
    x = b;
    return;
  }
  if (b.bvalue) SafeSample(pp[1], seed, 0, x);
  else SafeSample(pp[2], seed, 0, x); 
}

const char* conddoc = "If <b> is true, returns <t>; else, returns <f>.";

void AddCond(type bt, type t, PtrTable *fns)
{
  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(bt, "b");
  pl[1] = new formal_param(t, "t");
  pl[2] = new formal_param(t, "f");

  internal_func *cnd = 
    new internal_func(t, "cond", compute_cond, sample_cond, pl, 3, conddoc);

  InsertFunction(fns, cnd);
}


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
  // Misc
  AddDontKnow(t);
}


void InitBuiltinConstants(PtrTable *t)
{
  // Infinity
  constfunc *infty = MakeConstant(INT, "infty", NULL, -1);
  infty->SetReturn(MakeInfinityExpr(1, NULL, -1));
  t->AddNamePtr("infty", infty);

  // Other constants? 
}
