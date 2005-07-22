
// $Id$

#include "api.h"
#include "fnlib.h"
#include "compile.h"
#include "../Base/docs.h"
#include "../Rng/rng.h"
#include "../Formalisms/api.h"
#include "../Language/strings.h"
#include <math.h>

char** environment; // used by system function

shared_string empty_string(strdup(""));

extern PtrTable Builtins;

//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//--//

// Used by computehelp
char* help_search_string;


void ShowDocs(void *x)
{
  PtrTable::splayitem *foo = (PtrTable::splayitem *) x;
  if (!DocMatches(foo->name, help_search_string)) return;  // not interested...
  bool unshown = true;
  List <function> *bar = (List <function> *)foo->ptr;
  int i;
  for (i=0; i<bar->Length(); i++) {
    function *hit = bar->Item(i);
#ifdef RELEASE_CODE
    if (hit->IsUndocumented()) continue;  // don't show this to the public
#endif
    // show to the public
    if (unshown) {
      Output << foo->name << "\n";
      unshown = false;
    }
#ifdef DEVELOPMENT_CODE
    if (hit->IsUndocumented()) Output.Put("Undocumented");
#endif
    Output.Pad(' ', 5);
    hit->ShowHeader(Output);
    // special type checking: documentation must display parameters, too
    const char* d = hit->GetDocumentation();
    if (d) {
      // For template functions, show only the last one
      bool next_is_different = true;
      if (i+1<bar->Length()) {
        function *next = bar->Item(i+1);
        if (next->GetDocumentation() == d) 
	  next_is_different = false;
      }
      if (next_is_different) 
	DisplayDocs(Output, d, 5, 75, true);
    } else { // user defined... 
	Output << "\tdefined in "; 
        const char* fn = hit->Filename();
	if (fn[0]=='-' && fn[1]==0) 
	  Output << "standard input";
	else
	  Output << fn;
	Output << " near line " << hit->Linenumber() << "\n";
    }
    Output << "\n";
    Output.Check();
  }
}

void compute_help(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  help_search_string = "";
  DCASSERT(np==1);
  DCASSERT(pp); 
  SafeCompute(pp[0], x);
  if (x.answer->isNormal()) 
    help_search_string = x.answer->svalue->string;

  // Help topics
  MatchTopics(help_search_string, Output, 5, 75);
  Output.Check();

  // Builtin functions
  Builtins.Traverse(ShowDocs);

  // Model functions
  HelpModelFuncs();

  // Now go through options
  int i;
  for (i=0; i<NumOptions(); i++) {
    option* o = GetOptionNumber(i);
    if (o->IsUndocumented()) continue;
    if (!o->isApropos(help_search_string)) continue;
    o->ShowHeader(Output);
    DisplayDocs(Output, o->GetDocumentation(), 5, 75, false);
    o->ShowRange(Output, 5, 75);
    Output << "\n";
  }

  Output.flush();
  // return something...
  x.answer->setNull();
}

void AddHelp(PtrTable *fns)
{
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "search");

  const char* helpdoc = "An on-line help mechanism.  Searches for help topics, functions, options, and option constants containing the substring <search>.  Documentation is displayed for all matches.  Use search string \"topics\" to view the available help topics.  An empty string will cause *all* documentation to be displayed.";

  internal_func *hlp =
    new internal_func(VOID, "help", compute_help, pl, 1, helpdoc);

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

void do_print(expr **p, int np, compute_data &x, OutputStream &s)
{
  int i;
  result y;
  for (i=0; i<np; i++) {
    SafeCompute(p[i], x);
    int width = -1;
    int prec = -1;
    if (NumComponents(p[i])>1) {
      result* answer = x.answer;
      x.answer = &y;
      x.aggregate = 1;
      SafeCompute(p[i], x);
      if (y.isNormal()) width = y.ivalue;
      if (NumComponents(p[i])>2) {
        y.Clear();
        x.aggregate = 2;
	SafeCompute(p[i], x);
        if (y.isNormal()) prec = y.ivalue;
      }
      x.answer = answer;
    }
    PrintResult(s, Type(p[i], 0), *x.answer, width, prec);
  }
}

void compute_print(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  do_print(p, np, x, Output);
  if (IsInteractive()) Output.flush();
  else Output.Check();
}

void AddPrint(PtrTable *fns)
{
  const char* helpdoc = "\b(arg1, arg2, ...)\nPrint each argument to output stream.  Arguments can be any printable type, and may include an optional width specifier as \"arg:width\".  Real values may also specify the number of digits of precision, as \"arg:width:prec\" (the format of reals is specified with the option RealFormat).  Strings may include the following special characters:\n\t\\a\taudible bell\n\t\\b\tbackspace\n\t\\f\tflush output\n\t\\n\tnewline\n\t\\q\tdouble quote \"\n\t\\t\ttab character";

  internal_func *p =
    new internal_func(VOID, "print", compute_print, NULL, 0, helpdoc);

  p->SetSpecialTypechecking(typecheck_print);
  p->SetSpecialParamLinking(linkparams_print);

  InsertFunction(fns, p);
}

void compute_sprint(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  static StringStream strbuffer;
  do_print(p, np, x, strbuffer);
  if (!x.answer->isNormal()) return;
  char* bar = strbuffer.GetString();
  strbuffer.flush();
  x.answer->Clear();
  x.answer->svalue = new shared_string(bar);
}

void AddSprint(PtrTable *fns)
{
  const char* helpdoc = "\b(arg1, arg2, ...)\nJust like \"print\", except the result is written into a string, which is returned.";

  internal_func *p =
    new internal_func(STRING, "sprint", compute_sprint, NULL, 0, helpdoc);

  p->SetSpecialTypechecking(typecheck_print);
  p->SetSpecialParamLinking(linkparams_print);

  InsertFunction(fns, p);
}


// ********************************************************
// *                  Input  functions                    *
// ********************************************************

void compute_read_bool(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np==1);

  SafeCompute(pp[0], x);
  if (x.answer->isError()) return;

  char c=' ';
  while (1) {
    if (Input.IsDefault()) 
      if (!x.answer->isNull()) {
        Output << "Enter the [y/n] value for ";
        x.answer->svalue->show(Output);
	Output << " : ";
        Output.flush();
      }     
    Input.Get(c);
    if (c=='y' || c=='Y' || c=='n' || c=='N') break;
  }
  DeleteResult(STRING, *x.answer);

  x.answer->Clear();
  x.answer->bvalue = (c=='y' || c=='Y');
}

void AddReadBool(PtrTable *fns)
{
  const char* helpdoc = "Read a boolean value from the input stream.  If the current input stream is standard input, then the string given by \"prompt\" is displayed first.";

  formal_param **fp = new formal_param*[1];
  fp[0] = new formal_param(STRING, "prompt");
  internal_func *p =
    new internal_func(BOOL, "read_bool", compute_read_bool, fp, 1, helpdoc);

  InsertFunction(fns, p);
}


void compute_read_int(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np==1);

  SafeCompute(pp[0], x);
  if (x.answer->isError()) return;

  if (Input.IsDefault()) 
    if (!x.answer->isNull()) {
      Output << "Enter the (integer) value for ";
      x.answer->svalue->show(Output);
      Output << " : ";
      Output.flush();
    }   
  DeleteResult(STRING, *x.answer);

  x.answer->Clear();
  if (!Input.Get(x.answer->ivalue)) {
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Expecting integer from input stream\n";
    Error.Stop();
    x.answer->setError();
  }
}

void AddReadInt(PtrTable *fns)
{
  const char* helpdoc = "Prompt for and read an integer from the input stream";

  formal_param **fp = new formal_param*[1];
  fp[0] = new formal_param(STRING, "prompt");
  internal_func *p =
    new internal_func(INT, "read_int", compute_read_int, fp, 1, helpdoc);

  InsertFunction(fns, p);
}


void compute_read_real(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np==1);

  SafeCompute(pp[0], x);
  if (x.answer->isError()) return;

  if (Input.IsDefault()) 
    if (!x.answer->isNull()) {
      Output << "Enter the (real) value for ";
      x.answer->svalue->show(Output);
      Output << " : ";
      Output.flush();
    }   
  DeleteResult(STRING, *x.answer);

  x.answer->Clear();
  if (!Input.Get(x.answer->rvalue)) {
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Expecting real value from input stream\n";
    Error.Stop();
    x.answer->setError();
  }
}

void AddReadReal(PtrTable *fns)
{
  const char* helpdoc = "Prompt for and read a real value from the input stream";

  formal_param **fp = new formal_param*[1];
  fp[0] = new formal_param(STRING, "prompt");
  internal_func *p =
    new internal_func(REAL, "read_real", compute_read_real, fp, 1, helpdoc);

  InsertFunction(fns, p);
}



void compute_read_string(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np==2);

  SafeCompute(pp[1], x);
  if (x.answer->isError()) return;
  int length = x.answer->ivalue;

  SafeCompute(pp[0], x);
  if (x.answer->isError()) return;

  if (Input.IsDefault()) 
    if (!x.answer->isNull()) {
      Output << "Enter the (string) value for ";
      x.answer->svalue->show(Output);
      Output << " : ";
      Output.flush();
    }   
  DeleteResult(STRING, *x.answer);

  x.answer->Clear();
  char* buffer = new char[length+2];
  char c;
  do {
    if (!Input.Get(c)) {
      Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
      Error << "End of input stream before expected string\n";
      Error.Stop();
      x.answer->setError();
      delete[] buffer;
      return;
    }
  } while ((c==' ') || (c=='\t') || (c=='\n'));
  int i;
  for (i=0; i<length; i++) {
    buffer[i] = c;
    if (0==c) break;
    if (!Input.Get(c)) c=0;
    if ((c==' ') || (c=='\t') || (c=='\n')) c=0;
  }
  buffer[i] = 0;  // in case we go past the end
  x.answer->svalue = new shared_string(buffer);
}

void AddReadString(PtrTable *fns)
{
  const char* helpdoc = "Prompt for and read a string of at most <length> characters. The string is read from the input stream.";

  formal_param **fp = new formal_param*[2];
  fp[0] = new formal_param(STRING, "prompt");
  fp[1] = new formal_param(INT, "length");
  internal_func *p =
    new internal_func(STRING, "read_string", compute_read_string, fp, 2, helpdoc);

  InsertFunction(fns, p);
}



// ********************************************************
// *                   File functions                     *
// ********************************************************

void compute_inputfile(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(np==1);
  DCASSERT(p);
  SafeCompute(p[0], x);
  if (x.answer->isError()) return;
  if (x.answer->isNull()) {
    Input.SwitchInput(NULL);
    x.answer->Clear();
    x.answer->bvalue = true;
    return;
  }
  FILE* infile = fopen(x.answer->svalue->string, "r");
  DeleteResult(STRING, *x.answer); 
  x.answer->Clear();
  if (NULL==infile) {
    x.answer->bvalue = false;
  } else {
    Input.SwitchInput(infile);
    x.answer->bvalue = true;
  }
}

void AddInputFile(PtrTable *fns)
{
  const char* helpdoc = "Switch the input stream from the specified filename.  If the filename is null, the input stream is switched to standard input. If the filename does not exist or cannot be opened, return false. Returns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "InputFile", compute_inputfile, pl, 1, helpdoc);

  InsertFunction(fns, p);
}

void compute_file(DisplayStream &s, expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(np==1);
  DCASSERT(p);
  SafeCompute(p[0], x);
  if (x.answer->isError()) return;
  if (x.answer->isNull()) {
    s.SwitchDisplay(NULL);
    x.answer->Clear();
    x.answer->bvalue = true;
    return;
  }
  FILE* outfile = fopen(x.answer->svalue->string, "a");
  DeleteResult(STRING, *x.answer);
  if (NULL==outfile) {
    x.answer->bvalue = false;
  } else {
    s.SwitchDisplay(outfile);
    x.answer->bvalue = true;
  }
}

void compute_errorfile(expr **p, int np, compute_data &x)
{
  compute_file(Error, p, np, x);
}

void AddErrorFile(PtrTable *fns)
{
  const char* helpdoc = "Append the error stream to the specified filename. If the file does not exist, it is created. If the filename is null, the error stream is switched to standard error. Returns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "ErrorFile", compute_errorfile, pl, 1, helpdoc);

  InsertFunction(fns, p);
}


void compute_warningfile(expr **p, int np, compute_data &x)
{
  compute_file(Warning, p, np, x);
}

void AddWarningFile(PtrTable *fns)
{
  const char* helpdoc = "Append the warning stream to the specified filename. If the file does not exist, it is created. If the filename is null, the warning stream is switched to standard error. Returns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "WarningFile", compute_warningfile, pl, 1, helpdoc);

  InsertFunction(fns, p);
}


void compute_outputfile(expr **p, int np, compute_data &x)
{
  compute_file(Output, p, np, x);
}

void AddOutputFile(PtrTable *fns)
{
  const char* helpdoc = "Append the output stream to the specified filename. If the file does not exist, it is created. If the filename is null, the output stream is switched to standard output. Returns true on success.";

  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "filename");

  internal_func *p =
    new internal_func(BOOL, "OutputFile", compute_outputfile, pl, 1, helpdoc);

  InsertFunction(fns, p);
}

// ******************************************************************
// *                                                                *
// *                       string functions                         *
// *                                                                *
// ******************************************************************

void compute_substr(expr** p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(p);
  DCASSERT(np==3);
  result start;
  result stop;
  result* answer = x.answer;

  SafeCompute(p[0], x);
  if (!answer->isNormal()) return;
  char* src = answer->svalue->string;
  int srclen = strlen(src);

  x.answer = &start;
  SafeCompute(p[1], x);
  x.answer = answer;
  if (start.isInfinity()) {
    if (start.ivalue>0) {
      // +infinity: result is empty string
      DeleteResult(STRING, *answer);
      answer->svalue = &empty_string;
      Share(answer->svalue);
      return;
    } else {
      // -infinity is the same as 0 here
      start.Clear();
      start.ivalue = 0; 
    }
  }
  if (!start.isNormal()) {
    DeleteResult(STRING, *answer);
    *answer = start;
    return;
  }

  x.answer = &stop;
  SafeCompute(p[2], x);
  x.answer = answer;
  if (stop.isInfinity()) {
    if (stop.ivalue>0) {
      // +infinity is the same as the string length
      stop.Clear();
      stop.ivalue = srclen;
    } else {
      // -infinity: stop is less than start, this gives empty string
      DeleteResult(STRING, *answer);
      answer->svalue = &empty_string;
      Share(answer->svalue);
      return;
    }
  }
  if (!stop.isNormal()) {
    DeleteResult(STRING, *answer);
    *answer = stop;
    return;
  }

  // still here? stop and start are both finite, ordinary integers

  if (stop.ivalue<0 || 
      start.ivalue > srclen || 
      start.ivalue>stop.ivalue) {  // definitely empty string
    DeleteResult(STRING, *answer);
    answer->svalue = &empty_string;
    Share(answer->svalue);
    return;
  }

  start.ivalue = MAX(start.ivalue, 0);
  stop.ivalue =  MIN(stop.ivalue, srclen);
  // is it the full string?
  if ((0==start.ivalue) && (srclen==stop.ivalue)) {
    // x is the string parameter, keep it that way
    Share(answer->svalue);
    return;
  }
  // we are a proper substring, fill it
  char* sub = new char[stop.ivalue - start.ivalue+2]; 
  strncpy(sub, src + start.ivalue, 1+(stop.ivalue-start.ivalue));
  sub[stop.ivalue - start.ivalue + 1] = 0;
  DeleteResult(STRING, *answer);
  answer->Clear();
  answer->svalue = new shared_string(sub);
}

void AddSubstr(PtrTable *fns)
{
  const char* helpdoc = "Get the substring between (and including) elements left and right of string x.  If left>right, returns the empty string.";

  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(STRING, "x");
  pl[1] = new formal_param(INT, "left");
  pl[2] = new formal_param(INT, "right");

  internal_func *p =
    new internal_func(STRING, "substr", compute_substr, pl, 3, helpdoc);

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
void compute_div(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(p);
  result a,b;
  result* answer = x.answer;
  answer->Clear();
  x.answer = &a;
  SafeCompute(p[0], x);
  if (a.isNull() || a.isError() || a.isUnknown()) {
    x.answer = answer;
    *answer = a; // propogate the "error"
    return;
  }
  x.answer = &b;
  SafeCompute(p[1], x);
  x.answer = answer;
  if (b.isNull() || b.isError() || b.isUnknown()) {
    *answer = b; // propogate the "error"
    return;
  }
  if (a.isNormal() && b.isNormal()) {
    if (b.ivalue == 0) {
      // a div 0
      DCASSERT(p[1]);
      Error.Start(p[1]->Filename(), p[1]->Linenumber());
      Error << "Illegal operation: divide by 0";
      Error.Stop();
      answer->setError();
      return;
    }
    // ordinary integer division
    answer->ivalue = a.ivalue / b.ivalue;
    return;
  }

  if (a.isInfinity() && b.isInfinity()) {
    DCASSERT(p[1]);
    Error.Start(p[1]->Filename(), p[1]->Linenumber());
    Error << "Illegal operation: infty / infty";
    Error.Stop();
    answer->setError();
    return;
  }
  if (b.isInfinity()) {
    // a div +-infty = 0
    answer->ivalue = 0;
    return;
  }
  if (a.isInfinity()) {
    answer->setInfinity();
    // infinty div b = +- infinity
    if ((a.ivalue > 0) == (b.ivalue > 0)) {
      // same sign, that's +infinity
      answer->ivalue = 1;
    } else {
      // opposite sign, that's -infinity
      answer->ivalue = -1;
    }
  }
  // still here? some type of error
  answer->setError();
}


void AddDiv(PtrTable *fns)
{
  const char* helpdoc = "Integer division: computes int(a/b)";

  // Deterministic version
  formal_param **pl_1 = new formal_param*[2];
  pl_1[0] = new formal_param(INT, "a");
  pl_1[1] = new formal_param(INT, "b");
  internal_func *p1 = 
    new internal_func(INT, "div", compute_div, pl_1, 2, helpdoc);

  // Random version
  formal_param **pl_2 = new formal_param*[2];
  pl_2[0] = new formal_param(RAND_INT, "a");
  pl_2[1] = new formal_param(RAND_INT, "b");
  internal_func *p2 = 
    new internal_func(RAND_INT, "div", compute_div, pl_2, 2, helpdoc);

  // Proc version
  formal_param **pl_3 = new formal_param*[2];
  pl_3[0] = new formal_param(PROC_INT, "a");
  pl_3[1] = new formal_param(PROC_INT, "b");
  internal_func *p3 = 
    new internal_func(PROC_INT, "div", compute_div, pl_3, 2, helpdoc);

  // Proc rand version
  formal_param **pl_4 = new formal_param*[2];
  pl_4[0] = new formal_param(PROC_RAND_INT, "a");
  pl_4[1] = new formal_param(PROC_RAND_INT, "b");
  internal_func *p4 = 
    new internal_func(PROC_RAND_INT, "div", compute_div, pl_4, 2, helpdoc);

  InsertFunction(fns, p1);
  InsertFunction(fns, p2);
  InsertFunction(fns, p3);
  InsertFunction(fns, p4);
}

// ********************************************************
// *                        mod                           *
// ********************************************************

void compute_mod(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(p);
  result* answer = x.answer;
  answer->Clear();
  result a,b;
  x.answer = &a;
  SafeCompute(p[0], x);
  if (a.isNull() || a.isError() || a.isUnknown()) {
    *answer = a; // propogate the "error"
    x.answer = answer;
    return;
  }
  x.answer = &b;
  SafeCompute(p[1], x);
  x.answer = answer;
  if (b.isNull() || b.isError() || b.isUnknown()) {
    *answer = b; // propogate the "error"
    return;
  }
  if (a.isNormal() && b.isNormal()) {
    if (b.ivalue == 0) {
      // a mod 0
      DCASSERT(p[1]);
      Error.Start(p[1]->Filename(), p[1]->Linenumber());
      Error << "Illegal operation: modulo 0";
      Error.Stop();
      answer->setError();
      return;
    }
    // ordinary mod 
    answer->ivalue = a.ivalue % b.ivalue;
    return;
  }
  if (a.isInfinity() && b.isInfinity()) {
    DCASSERT(p[1]);
    Error.Start(p[1]->Filename(), p[1]->Linenumber());
    Error << "Illegal operation: infty mod infty";
    Error.Stop();
    answer->setError();
    return;
  }
  if (b.isInfinity()) {
    // a mod +-infty = a
    answer->ivalue = a.ivalue;  // should we check signs?
    return;
  }
  if (a.isInfinity()) {
    // +- infty mod b is undefined
    DCASSERT(p[1]);
    Error.Start(p[1]->Filename(), p[1]->Linenumber());
    Error << "Illegal operation: infty mod " << b.ivalue;
    Error.Stop();
    answer->setError();
    return;
  }
  // still here? some type of error
  answer->setError();
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
  p = new internal_func(INT, "mod", compute_mod, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Random version
  pl = new formal_param*[2];
  pl[0] = new formal_param(RAND_INT, "a");
  pl[1] = new formal_param(RAND_INT, "b");
  p = new internal_func(RAND_INT, "mod", compute_mod, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_INT, "a");
  pl[1] = new formal_param(PROC_INT, "b");
  p = new internal_func(PROC_INT, "mod", compute_mod, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc_Rand version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_RAND_INT, "a");
  pl[1] = new formal_param(PROC_RAND_INT, "b");
  p = new internal_func(PROC_RAND_INT, "mod", compute_mod, pl, 2, helpdoc);
  InsertFunction(fns, p);

}

// ********************************************************
// *                        sqrt                          *
// ********************************************************

void compute_sqrt(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  DCASSERT(p);
  SafeCompute(p[0], x);
  if (x.answer->isUnknown() || x.answer->isError() || x.answer->isNull()) return;

  if (x.answer->isInfinity()) {
    if (x.answer->ivalue>0) return;  // sqrt(infty) = infty
  } else {
    if (x.answer->rvalue>=0) {
      x.answer->rvalue = sqrt(x.answer->rvalue);
      return;
    }
  }
  
  // negative square root, error (we don't have complex)
  DCASSERT(p[0]);
  Error.Start(p[0]->Filename(), p[0]->Linenumber());
  Error << "Square root with negative argument: ";
  PrintResult(Error, REAL, *x.answer);
  Error.Stop();
  x.answer->setError();
}

void AddSqrt(PtrTable *fns)
{
  const char* helpdoc = "The positive square root of x";

  // Deterministic real version
  
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(REAL, "x");
  internal_func *p =
    new internal_func(REAL, "sqrt", compute_sqrt, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Random real version

  formal_param **pl2 = new formal_param*[1];
  pl2[0] = new formal_param(RAND_REAL, "x");
  internal_func *p2 =
    new internal_func(RAND_REAL, "sqrt", compute_sqrt, pl2, 1, helpdoc);
  InsertFunction(fns, p2);

  // Proc real version

  formal_param **pl3 = new formal_param*[1];
  pl3[0] = new formal_param(PROC_REAL, "x");
  internal_func *p3 =
    new internal_func(PROC_REAL, "sqrt", compute_sqrt, pl3, 1, helpdoc);
  InsertFunction(fns, p3);

  // Proc rand real version

  formal_param **pl4 = new formal_param*[1];
  pl4[0] = new formal_param(PROC_RAND_REAL, "x");
  internal_func *p4 =
    new internal_func(PROC_RAND_REAL, "sqrt", compute_sqrt, pl4, 1, helpdoc);
  InsertFunction(fns, p4);
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

void compute_bernoulli(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], x);

  if (x.answer->isNormal()) {
    if ((x.answer->rvalue>=0.0) && (x.answer->rvalue<=1.0)) {
      // p value is in legal range; do the computation.
      if (x.stream) {
	// We have a Rng stream, sample the value
        x.answer->ivalue = (x.stream->uniform() < x.answer->rvalue) ? 1 : 0;
      } else {
	// No Rng stream, build ph int
        Internal.Start(__FILE__, __LINE__, pp[0]->Filename(), pp[0]->Linenumber());
        Internal << "Construction of bernoulli ph int not done\n";
        Internal.Stop();
        x.answer->setNull();
      }
      return;
    }
    // illegal p value
    DCASSERT(pp[0]);
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Bernoulli probability " << x.answer->rvalue << " out of range";
    Error.Stop();
    x.answer->setError();
    return;
  }
  // still here, x is abnormal

  if (x.answer->isInfinity()) {
    DCASSERT(pp[0]);
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Bernoulli probability is infinite";
    Error.Stop();
    x.answer->setError();
    return;
  }
  // propogate any other errors (silently).
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
  	compute_bernoulli, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // RAND_INT version, note parameter 
  pl = new formal_param*[1];
  pl[0] = new formal_param(RAND_REAL, "p");
  p = new internal_func(RAND_INT, "bernoulli", 
	compute_bernoulli, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // PROC_PH_INT version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_REAL, "p");
  p = new internal_func(PROC_PH_INT, "bernoulli", 
  	compute_bernoulli, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // PROC_RAND_INT version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_RAND_REAL, "p");
  p = new internal_func(PROC_RAND_INT, "bernoulli", 
	compute_bernoulli, pl, 1, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                     Equilikely                       *
// ********************************************************

void compute_equilikely(expr **pp, int np, compute_data &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  result* answer = x.answer;
  answer->Clear();
  result a,b;
  x.answer = &a;
  SafeCompute(pp[0], x);
  x.answer = &b;
  SafeCompute(pp[1], x);
  x.answer = answer;
  
  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    if (x.stream) {
      // we have a Rng stream, sample it
      answer->ivalue = int(a.ivalue + (b.ivalue-a.ivalue+1)*x.stream->uniform());
    } else {
      // No Rng stream, build ph int
      Internal.Start(__FILE__, __LINE__, pp[0]->Filename(), pp[0]->Linenumber());
      Internal << "Construction of equilikely ph int not done\n";
      Internal.Stop();
      answer->setNull();
    }
    return;
  }

  // Deal with abnormal cases

  if (a.isInfinity() || b.isInfinity()) {
    answer->setError();
    DCASSERT(pp[0]);
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Equilikely with infinite argument";
    Error.Stop();
    return;
  }
  if (a.isUnknown() || b.isUnknown()) {
    answer->setUnknown();
    return;
  }
  if (a.isNull() || b.isNull()) {
    answer->setNull();
    return;
  }
  // any other errors here
  answer->setError();
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
	compute_equilikely, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Random parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(RAND_INT, "a");
  pl[1] = new formal_param(RAND_INT, "b");
  p = new internal_func(RAND_INT, "equilikely", 
	compute_equilikely, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_INT, "a");
  pl[1] = new formal_param(PROC_INT, "b");
  p = new internal_func(PROC_PH_INT, "equilikely", 
  	compute_equilikely, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc rand parameter version
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_RAND_INT, "a");
  pl[1] = new formal_param(PROC_RAND_INT, "b");
  p = new internal_func(PROC_RAND_INT, "equilikely", 
	compute_equilikely, pl, 2, helpdoc);
  InsertFunction(fns, p);

}

// ********************************************************
// *                      Geometric                       *
// ********************************************************

void compute_geometric(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], x);

  if (x.answer->isNormal()) {
    if ((x.answer->rvalue>=0.0) && (x.answer->rvalue<=1.0)) {
      // Legal p value
      if (x.stream) {
	// We have a Rng, sample
	if (0.0 == x.answer->rvalue) return;  	// geom(0) = const(0)
	if (1.0 == x.answer->rvalue) {
	  x.answer->setInfinity();		// geom(1) = const(infinity)
	  return;
	}
        x.answer->ivalue = int(log(x.stream->uniform()) / log(x.answer->rvalue));
      } else {
	// No Rng, build ph int
        Internal.Start(__FILE__, __LINE__, pp[0]->Filename(), pp[0]->Linenumber());
        Internal << "Construction of geometric ph int not done\n";
        Internal.Stop();
        x.answer->setNull();
      }
      return;
    }
    // Illegal p value
    DCASSERT(pp[0]);
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Geometric probability " << x.answer->rvalue << " out of range";
    Error.Stop();
    x.answer->setError();
    return;
  }
  if (x.answer->isInfinity()) {
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Geometric probability is infinite";
    Error.Stop();
    x.answer->setError();
    return;
  }
  // other strange values (error, null, unknown) may propogate
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
	compute_geometric, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Random version
  pl = new formal_param*[1];
  pl[0] = new formal_param(RAND_REAL, "p");
  p = new internal_func(RAND_INT, "geometric", 
	compute_geometric, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_REAL, "p");
  p = new internal_func(PROC_PH_INT, "geometric", 
	compute_geometric, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc-rand version
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_RAND_REAL, "p");
  p = new internal_func(PROC_RAND_INT, "geometric", 
	compute_geometric, pl, 1, helpdoc);
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

void compute_uniform(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(pp);
  DCASSERT(x.stream);  // we must sample this one
  result* answer = x.answer;
  answer->Clear();
  result a,b;
  x.answer = &a;
  SafeCompute(pp[0], x);
  x.answer = &b;
  SafeCompute(pp[1], x);
  x.answer = answer;
  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    x.answer->rvalue = a.rvalue + (b.rvalue-a.rvalue)*x.stream->uniform();
    return;
  }
  if (a.isInfinity() || b.isInfinity()) {
    x.answer->setError();
    DCASSERT(pp[0]);
    Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "Uniform with infinite argument";
    Error.Stop();
    return;
  }
  if (a.isUnknown() || b.isUnknown()) {
    x.answer->setUnknown();
    return;
  }
  if (a.isNull() || b.isNull()) {
    x.answer->setNull();
    return;
  }
  // any other errors here
  x.answer->setError();
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
	compute_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Random params
  pl = new formal_param*[2];
  pl[0] = new formal_param(RAND_REAL, "a");
  pl[1] = new formal_param(RAND_REAL, "b");
  p = new internal_func(RAND_REAL, "uniform", 
	compute_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc params
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_REAL, "a");
  pl[1] = new formal_param(PROC_REAL, "b");
  p = new internal_func(PROC_RAND_REAL, "uniform", 
	compute_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);

  // Proc-rand params
  pl = new formal_param*[2];
  pl[0] = new formal_param(PROC_RAND_REAL, "a");
  pl[1] = new formal_param(PROC_RAND_REAL, "b");
  p = new internal_func(PROC_RAND_REAL, "uniform", 
	compute_uniform, pl, 2, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                         Expo                         *
// ********************************************************

void compute_expo(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  DCASSERT(pp);
  SafeCompute(pp[0], x);
  if (NULL==x.stream) return;

  // If Rng stream is provided, sample
  if (x.answer->isNormal()) {
    if (x.answer->rvalue>0) {
      x.answer->rvalue = - log(x.stream->uniform()) / x.answer->rvalue;
      return;
    } 
    if (x.answer->rvalue<0) {
      DCASSERT(pp[0]);
      Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
      Error << "expo with parameter " << x.answer->rvalue << ", must be non-negative";
      Error.Stop();
      x.answer->setError();
      return;
    } 
    // still here?  Must be expo(0) = const(infinity)
    x.answer->setInfinity();  
    x.answer->ivalue = 1;
    return;
  }
  if (x.answer->isInfinity()) {  // expo(infintity) has mean 0
    if (x.answer->ivalue < 0) {
      DCASSERT(pp[0]);
      Error.Start(pp[0]->Filename(), pp[0]->Linenumber());
      Error << "expo with parameter ";
      PrintResult(Error, REAL, *x.answer);
      Error.Stop();
      x.answer->setError();
      return; 
    }
    x.answer->Clear();
    x.answer->rvalue = 0.0;
    return;
  }
  // some other error, just propogate it
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
	compute_expo, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Rand param
  pl = new formal_param*[1];
  pl[0] = new formal_param(RAND_REAL, "lambda");
  p = new internal_func(RAND_REAL, "expo", 
	compute_expo, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc param
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_REAL, "lambda");
  p = new internal_func(PROC_EXPO, "expo", 
	compute_expo, pl, 1, helpdoc);
  InsertFunction(fns, p);

  // Proc Rand param
  pl = new formal_param*[1];
  pl[0] = new formal_param(PROC_RAND_REAL, "lambda");
  p = new internal_func(PROC_RAND_REAL, "expo", 
	compute_expo, pl, 1, helpdoc);
  InsertFunction(fns, p);
}

// ********************************************************
// *                probability and such                  *
// ********************************************************

//#define DEBUG_AVG

void compute_avg(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  const int N = 100000;
  // For testing right now.  Write a better version eventually.
  Rng* oldstream = x.stream;
  Rng foo(123456789);
  x.stream = &foo;
  
  // A bad way to compute the average...
  result* answer = x.answer;
  answer->Clear();
  answer->rvalue = 0.0;
  result sample;
  x.answer = &sample;
  int i;
  for (i=0; i<N; i++) {
    p[0]->ClearCache(); // reset samples
    SafeCompute(p[0], x);
    
#ifdef DEBUG_AVG
    Output << "  sampled ";
    PrintResult(Output, REAL, sample);
    Output << "\n";
#endif
   
    if (sample.isNormal()) {
      answer->rvalue += sample.rvalue;
      continue;
    }
    if (sample.isInfinity()) {
      if (answer->isInfinity()) {
	// if signs don't match, error
	if (SIGN(answer->ivalue)!=SIGN(sample.ivalue)) {
	  answer->setError();
	  Error.Start(p[0]->Filename(), p[0]->Linenumber());
	  Error << "Undefined value (infinity - infinity) in Avg";
	  Error.Stop();
	  x.stream = oldstream;
	  return;
	}
      }
      *answer = sample;
      continue;
    }
    // if we get here we've got null or an error, bail out
    *answer = sample;
    x.answer = answer;
    x.stream = oldstream;
    return;
  } // for i

  // Divide by N 
  if (answer->isInfinity()) return;  // unless we're infinity

  answer->rvalue /= N;
  x.answer = answer;
  x.stream = oldstream;
}

void AddAvg(PtrTable *fns)
{
  const char* helpdoc = "Computes the expected value of x";

  formal_param **pl2 = new formal_param*[1];
  pl2[0] = new formal_param(RAND_REAL, "x");
  internal_func *p2 =
    new internal_func(REAL, "avg", compute_avg, pl2, 1, helpdoc);
  InsertFunction(fns, p2);
}


// ********************************************************
// *               system-like  functions                 *
// ********************************************************

void compute_filename(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(0==np);
  x.answer->Clear();
  x.answer->svalue = new shared_string(strdup(Filename()));
}

void AddFilename(PtrTable *fns)
{
  const char* helpdoc = "Return the name of the current source file being read by the Smart interpreter, as invoked from the command line or an #include.  The name \"-\" is used when reading from standard input.";

  internal_func *p = 
    new internal_func(STRING, "Filename", compute_filename, NULL, 0, helpdoc);

  InsertFunction(fns, p);
}


void compute_env(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  DCASSERT(p);
  SafeCompute(p[0], x);
  if (!x.answer->isNormal()) return;
  if (NULL==environment) return;
  char* key = x.answer->svalue->string;
  int xlen = strlen(key);
  for (int i=0; environment[i]; i++) {
    char* equals = strstr(environment[i], "=");
    if (NULL==equals) continue;  // shouldn't happen, right? 
    int length = (equals - environment[i]);
    if (length!=xlen) continue;
    if (strncmp(environment[i], key, length)!=0) continue;  
    // match, clear out old x
    DeleteResult(STRING, *x.answer); 
    x.answer->Clear();
    x.answer->svalue = new shared_string(strdup(equals+1));
    return;
  }
  // not found
  DeleteResult(STRING, *x.answer); 
}

void AddEnv(PtrTable *fns)
{
  const char* helpdoc = "Return the first environment string that matches parameter <find>.";
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(STRING, "find");

  internal_func *p = 
    new internal_func(STRING, "env", compute_env, pl, 1, helpdoc);

  InsertFunction(fns, p);
}

void compute_exit(expr **p, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  DCASSERT(p);
  int code = 0;
  SafeCompute(p[0], x);
  if (x.answer->isNormal()) {
    code = x.answer->ivalue;
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
    new internal_func(VOID, "exit", compute_exit, pl, 1, helpdoc);

  InsertFunction(fns, p);
}

// ********************************************************
// *                        cond                          *
// ********************************************************

void compute_cond(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np==3);
  result b;
  result *answer = x.answer;
  x.answer = &b;
  SafeCompute(pp[0], x);
  x.answer = answer;
  if (b.isNull() || b.isError()) {
    // error stuff?
    *answer = b;
    return;
  }
  if (b.bvalue) SafeCompute(pp[1], x);
  else SafeCompute(pp[2], x);
}

void AddCond(type bt, type t, PtrTable *fns)
{
  const char* conddoc = "If <b> is true, returns <t>; else, returns <f>.";

  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(bt, "b");
  pl[1] = new formal_param(t, "t");
  pl[2] = new formal_param(t, "f");

  internal_func *cnd = cnd = new internal_func(t, "cond", 
  	compute_cond, pl, 3, conddoc);

  InsertFunction(fns, cnd);
}

// ********************************************************
// *                        case                          *
// ********************************************************

void compute_case(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np>1);
  result c;
  result* answer = x.answer;
  x.answer = &c;
  SafeCompute(pp[0], x);
  if (c.isNull() || c.isError()) {
    // error stuff?
    *answer = c;
    x.answer = answer;
    return;
  }
  int i;
  result m;
  x.answer = &m;
  for (i=2; i<np; i++) {
    SafeCompute(pp[i], x);
    if (m.isNormal()) if (m.ivalue == c.ivalue) {
      // this is it!
      x.answer = answer;
      x.aggregate = 1;
      SafeCompute(pp[i], x);
      return;
    }
  }
  // still here?  use the default value
  x.answer = answer;
  SafeCompute(pp[1], x);
}

void AddCase(type it, type t, PtrTable *fns)
{
  const char* casedoc = "Match the value <c> with one of the <c:v> pairs. If matched, return the matching v; Otherwise return the default value <dv>.";
  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(it, "c");
  pl[1] = new formal_param(t, "dv");
  type *tl = new type[2];
  tl[0] = it;
  tl[1] = t;
  pl[2] = new formal_param(tl, 2, "cv");

  internal_func *ca = new internal_func(t, "case", 
  	compute_case, pl, 3, 2, casedoc);

  InsertFunction(fns, ca);
}


// ********************************************************
// *                      is_null                         *
// ********************************************************

void compute_is_null(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pp);
  DCASSERT(np==1);
  SafeCompute(pp[0], x);
  if (x.answer->isNull()) {
    x.answer->Clear();
    x.answer->bvalue = true;
  } else {
    x.answer->Clear();
    x.answer->bvalue = false;
  }
}

void AddIsNull(PtrTable *t, type paramtype)
{
  const char* doc = "Returns true if the argument is null.";
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(paramtype, "x");
  internal_func *foo = new internal_func(BOOL, "is_null", 
  	compute_is_null, pl, 1, doc);
  InsertFunction(t, foo);
}

// ********************************************************
// *                      dontknow                        *
// ********************************************************


void compute_dontknow(expr **pp, int np, compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  x.answer->Clear();
  x.answer->setUnknown();
}

void AddDontKnow(PtrTable *t)
{
  const char* dkdoc = "Returns a finite, unknown value.";
  formal_param **pl = new formal_param*[1];
  pl[0] = new formal_param(INT, "dummy");
  internal_func *foo = new internal_func(INT, "DontKnow", 
  	compute_dontknow, pl, 1, dkdoc);
  foo->HideDocs();  // this is not for public consumption
  InsertFunction(t, foo);
}

void InitBuiltinFunctions(PtrTable *t)
{
  AddHelp(t);
  // Input
  AddReadBool(t);
  AddReadInt(t);
  AddReadReal(t);
  AddReadString(t);
  // Output
  AddPrint(t);
  AddSprint(t);
  // Files
  AddInputFile(t);
  AddErrorFile(t);
  AddWarningFile(t);
  AddOutputFile(t);
  // Handy string functions
  AddSubstr(t);
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
  AddFilename(t);
  AddEnv(t);
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
