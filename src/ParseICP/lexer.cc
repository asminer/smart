
#include "../Streams/streams.h"
#include "../include/defines.h"
#include "lexer.h"
#include "../ExprLib/exprman.h"

#include <string.h>
#include <stdlib.h>

// Takes care of objects that are required by smart.tab.h:
#include "compile.h"
#include "ParseICP/icpyacc.hh"

#include "parse_icp.h"
#include "../Options/options.h"

// #define LEXER_DEBUG

const int my_buffer_size = 16384;

const int max_file_depth = 1024;

// These will be defined in lex.yy.c (assuming flex does its job):
extern int yylineno;
extern char* yytext;
extern int yyleng;
struct yy_buffer_state;
yy_buffer_state* yy_create_buffer(FILE*, int);
void yy_switch_to_buffer(yy_buffer_state*);
void yy_delete_buffer(yy_buffer_state*);

// ******************************************************************
// *                                                                *
// *                        lexer_mod  class                        *
// *                                                                *
// ******************************************************************

class inputfile;

struct lexer_mod {
  parse_module* parent;
  debugging_msg lexer_debug;
  /// Stack of input files.
  inputfile** filestack;
  /// Top of file stack.
  int topfile;

  io_environ* ioenv;  // cached..

public:
  lexer_mod();
  ~lexer_mod();

  inline bool StackFull() const { return (topfile+1 >= max_file_depth); }

  const location& Where() const;

  void Initialize(parse_module* p);

  bool AlreadyOpen(const char* name) const;

  // returns true on success
  bool Open(inputfile* fi);

  /// Returns true if any files are still open.
  bool CloseCurrent();

  bool SetInputs(const char**, int);


  inline const type* FindOWDType(const char* s) const {
    DCASSERT(parent);
    return parent->FindOWDType(s);
  }
  inline modifier FindModif(const char* s) const {
    DCASSERT(parent);
    return parent->FindModif(s);
  }
  inline bool startInternal(const char* file, int line) {
    DCASSERT(parent);
    return parent->startInternal(file, line);
  }
  inline OutputStream& internal() {
    DCASSERT(parent);
    return parent->internal();
  }
  inline bool startError() {
    DCASSERT(parent);
    return parent->startError();
  }
  inline OutputStream& cerr() {
    DCASSERT(parent);
    return parent->cerr();
  }
  inline void stopError() {
    DCASSERT(parent);
    parent->stopError();
  }
  inline bool startWarning() {
    DCASSERT(parent);
    DCASSERT(parent->em);
    if (parent->em->hasIO()) {
      parent->em->startWarning();
      parent->em->causedBy(Where());
      return true;
    }
    return false;
  }
  inline OutputStream& warn() {
    DCASSERT(parent);
    DCASSERT(parent->em);
    DCASSERT(parent->em->hasIO());
    return parent->em->warn();
  }
  inline bool startDebug() {
    return lexer_debug.startReport();
  }
  inline OutputStream& debug() {
    return lexer_debug.report();
  }
  inline void stopDebug() {
    lexer_debug.stopIO();
  }
};

lexer_mod lexdata;

// ******************************************************************
// *                                                                *
// *                        inputfile  class                        *
// *                                                                *
// ******************************************************************

class inputfile {
  location where;
  FILE* input;
  yy_buffer_state* buffer;
  int consumed_lines;
  int counter_start;
public:
  /// Create lex buffer for this file.
  inputfile(const char* n);

  /// Create a lex buffer for an already opened file.
  inputfile(FILE* file, const char* n);

  /// Close file and such.
  ~inputfile();

  /** Start consuming tokens from this file.
      @return   true on success,
    false on error (i.e., couldn't open file)
  */
  bool StartTokenizing();

  /// Stop (perhaps temporarily) consuming tokens from this file.
  void StopTokenizing();

  inline const location& Where() {
      where.setline( consumed_lines + (yylineno - counter_start) );
      return where;
  }

  inline const char* Name() const {
      return where.getFile();
  }

  /// Are we reading from standard input?
  inline bool is_stdin() const {
    return where.is_stdin();
  }

  inline bool matches(const char* fname) const {
    return 0 == strcmp(where.getFile(), fname);
  }
};

// ******************************************************************
// *                       inputfile  methods                       *
// ******************************************************************

inputfile::inputfile(const char* n)
{
  DCASSERT(n);
  if (n[0] == '-' && n[1] == 0) {
    where.start(0);
  } else {
    where.start(n);
  }
  input = 0;
  buffer = 0;
  consumed_lines = 1;
  counter_start = 0;
}

inputfile::inputfile(FILE* f, const char* n)
{
  DCASSERT(n);
  if (n[0] == '-' && n[1] == 0) {
    where.start(0);
  } else {
    where.start(n);
  }
  input = f;
  buffer = 0;
  consumed_lines = 1;
  counter_start = 0;
}

inputfile::~inputfile()
{
  if (buffer)  yy_delete_buffer(buffer);
  if (input)   if (stdin != input) fclose(input);
}

bool inputfile::StartTokenizing()
{
  // open file if necessary
  if (0==input) {

    if (is_stdin())     input = stdin;
    else                input = fopen(where.getFile(), "r");

    if (0==input) {
      if (lexdata.startError()) {
        lexdata.cerr() << "couldn't open file " << where.getFile() << ", ignoring";
        lexdata.stopError();
      }
      return false;
    }
  }

  // create buffer if necessary
  if (0==buffer) buffer = yy_create_buffer(input, my_buffer_size);

  yy_switch_to_buffer(buffer);
  counter_start = yylineno;

  return true;
}

void inputfile::StopTokenizing()
{
  consumed_lines += (yylineno - counter_start);
}

/* =====================================================================

  Functions used by the lexer.

   ===================================================================== */

const char* TokenName(int tk)
{
  switch (tk) {
    case 0:           return "";
    case POUND:       return "POUND";
    case ENDPND:      return "ENDPND";
    case IN:          return "IN";
    case LPAR:        return "LPAR";
    case RPAR:        return "RPAR";
    case LBRACE:      return "LBRACE";
    case RBRACE:      return "RBRACE";
    case COMMA:       return "COMMA";
    case SEMI:        return "SEMI";
    case DOTDOT:      return "DOTDOT";
    case GETS:        return "GETS";
    case PLUS:        return "PLUS";
    case MINUS:       return "MINUS";
    case TIMES:       return "TIMES";
    case DIVIDE:      return "DIVIDE";
    case OR:          return "OR";
    case AND:         return "AND";
    case IMPLIES:     return "IMPLIES";
    case NOT:         return "NOT";
    case EQUALS:      return "EQUALS";
    case NEQUAL:      return "NEQUAL";
    case GT:          return "GT";
    case GE:          return "GE";
    case LT:          return "LT";
    case LE:          return "LE";
    case BOOLCONST:   return "BOOLCONST";
    case INTCONST:    return "INTCONST";
    case TYPE:        return "TYPE";
    case IDENT:       return "IDENT";
    case MAXIMIZE:    return "MAXIMIZE";
    case MINIMIZE:    return "MINIMIZE";
    case SATISFIABLE: return "SATISFIABLE";
  }
  return "no such token";
}

inline int ProcessToken(int tk)
{
  if (lexdata.startDebug()) {
    lexdata.debug() << "generated token:";
    lexdata.debug().Put(TokenName(tk), 12);
    lexdata.debug() << "  from text: ";
    lexdata.debug().Put(yytext);
    lexdata.debug().Put('\n');
    lexdata.stopDebug();
  }
  return tk;
}

int ProcessMaximize()
{
  return ProcessToken(MAXIMIZE);
}

int ProcessMinimize()
{
  return ProcessToken(MINIMIZE);
}

int ProcessSatisfiable()
{
  return ProcessToken(SATISFIABLE);
}

int ProcessIn()
{
  return ProcessToken(IN);
}

int ProcessLpar()
{
  return ProcessToken(LPAR);
}

int ProcessRpar()
{
  return ProcessToken(RPAR);
}

int ProcessLbrace()
{
  return ProcessToken(LBRACE);
}

int ProcessRbrace()
{
  return ProcessToken(RBRACE);
}

int ProcessComma()
{
  return ProcessToken(COMMA);
}

int ProcessSemi()
{
  return ProcessToken(SEMI);
}

int ProcessDotdot()
{
  return ProcessToken(DOTDOT);
}

int ProcessGets()
{
  return ProcessToken(GETS);
}

int ProcessPlus()
{
  return ProcessToken(PLUS);
}

int ProcessMinus()
{
  return ProcessToken(MINUS);
}

int ProcessTimes()
{
  return ProcessToken(TIMES);
}

int ProcessDivide()
{
  return ProcessToken(DIVIDE);
}

int ProcessMod()
{
  return ProcessToken(MOD);
}

int ProcessOr()
{
  return ProcessToken(OR);
}

int ProcessAnd()
{
  return ProcessToken(AND);
}

int ProcessImplies()
{
  return ProcessToken(IMPLIES);
}

int ProcessNot()
{
  return ProcessToken(NOT);
}

int ProcessEquals()
{
  return ProcessToken(EQUALS);
}

int ProcessNequal()
{
  return ProcessToken(NEQUAL);
}

int ProcessGt()
{
  return ProcessToken(GT);
}

int ProcessGe()
{
  return ProcessToken(GE);
}

int ProcessLt()
{
  return ProcessToken(LT);
}

int ProcessLe()
{
  return ProcessToken(LE);
}

int ProcessBool()
{
  yylval.name = strdup(yytext);
  return ProcessToken(BOOLCONST);
}

int ProcessInt()
{
  yylval.name = strdup(yytext);
  return ProcessToken(INTCONST);
}

int ProcessID()
{
  yylval.name = strdup(yytext);
  const type* t = lexdata.FindOWDType(yytext);
  if (0==t)  return ProcessToken(IDENT);
  return ProcessToken(TYPE);
}

int ProcessEndpnd()
{
  return ProcessToken(ENDPND);
}

int ProcessPound()
{
  return ProcessToken(POUND);
}

void UnclosedComment()
{
  if (lexdata.startError()) {
    lexdata.cerr() << "Unclosed comment";
    lexdata.stopError();
  }
}

void IllegalToken()
{
  if (lexdata.startError()) {
    lexdata.cerr() << "Illegal syntactical element: '" << yytext << "'";
    lexdata.stopError();
  }
}

/**
  Process the include tokens.
  In other words, handle nested includes and such.
*/
void Include()
{
  // Check for stack overflow.
  if (lexdata.StackFull()) {
    if (lexdata.startError()) {
      lexdata.cerr() << "Too many #includes, maximum depth exceeded!";
      lexdata.stopError();
    }
    return;
  }

  // Get the filename.  Ignore everything until first quote
  const char *token = yytext;
  int tokenlen = strlen(token);
  int start;
  int stop;
  for (start=0; start<tokenlen; start++) if (token[start]=='"') break;
  if (start>=tokenlen) {
    if (lexdata.startInternal(__FILE__, __LINE__)) {
      lexdata.internal() << "Include() error, missing quote?";
      lexdata.stopError();
    }
    return;
  }
  // stop should be the last char, but who knows?
  for (stop=tokenlen-1; stop>start; stop--) if (token[stop]=='"') break;
  if (stop<=start) {
    if (lexdata.startInternal(__FILE__, __LINE__)) {
      lexdata.internal() << "Include() error, missing final quote?";
      lexdata.stopError();
    }
    return;
  }

  // Bail out for empty strings
  if (stop-1==start) {
    if (lexdata.startWarning()) {
      lexdata.warn() << "Empty filename for include, ignoring";
      lexdata.stopError();
    }
    return;
  }

  // Build the filename
  char* fn = (char*) malloc(stop-start);
  fn[stop-start-1] = 0;
  int p;
  for (p=start+1; p<stop; p++) fn[p-start-1] = token[p];

  // Check for circular dependency
  if (lexdata.AlreadyOpen(fn)) {
    if (lexdata.startWarning()) {
      lexdata.warn() << "circular file dependency caused by #include ";
      lexdata.warn() << fn << ", ignoring";
      lexdata.stopError();
    }
    free(fn);
    return;
  }

  // Ok, try to open the file
  lexdata.Open(new inputfile(fn));
}

/// Handle end of file
int yywrap()
{
  if (lexdata.CloseCurrent())  return 0;
  return 1;
}


// ******************************************************************
// *                       lexer_mod  methods                       *
// ******************************************************************

lexer_mod::lexer_mod()
{
  parent = 0;
  filestack = new inputfile*[max_file_depth];
  topfile = -1;
  ioenv = 0;
}

lexer_mod::~lexer_mod()
{
  delete[] filestack;
}

const location& lexer_mod::Where() const
{
    if (topfile < 0) {
        return location::EOINPUT();
    } else {
        return filestack[topfile]->Where();
    }
}

void lexer_mod::Initialize(parse_module* p)
{
  if (p == parent)  return;
  parent = p;
  lexer_debug.initialize(parent ? parent->OptMan() : 0, "lexer",
    "When set, very low-level lexer messages are displayed."
  );
#ifdef LEXER_DEBUG
  lexer_debug.Activate();
#endif
}

bool lexer_mod::AlreadyOpen(const char* name) const
{
  for (int i=0; i<=topfile; i++) {
    if (filestack[i]->matches(name)) return true;
  }
  return false;
}

bool lexer_mod::Open(inputfile* fi)
{
  // Try to start tokenizing this file
  filestack[topfile+1] = fi;
  if (! filestack[topfile+1]->StartTokenizing() ) {
    // couldn't open the file!
    delete filestack[topfile+1];
    filestack[topfile+1] = 0;
    return false;
  }
  // Stop tokenizing the old file, if any
  filestack[topfile]->StopTokenizing();
  // Push the stack
  topfile++;

  if (startDebug()) {
    debug() << "switching to input file: ";
    debug().Put(fi->Name());
    debug().Put('\n');
    stopDebug();
  }

  return true;
}

bool lexer_mod::CloseCurrent()
{
  if (topfile<0)  return false;

  if (startDebug()) {
    debug() << "finished with input file: ";
    debug().Put(filestack[topfile]->Name());
    debug().Put('\n');
    stopDebug();
  }

  // end of current file
  delete filestack[topfile];
  filestack[topfile] = 0;
  topfile--;

  // If the stack is non-empty, pop a file and start tokenizing it.
  while (topfile >= 0) {
    if (filestack[topfile]->StartTokenizing()) {

      if (startDebug()) {
        debug() << "switching to input file: ";
        debug().Put(filestack[topfile]->Name());
        debug().Put('\n');
        stopDebug();
      }

      return true;
    }
    // still here?  We couldn't open the file
    delete filestack[topfile];
    filestack[topfile] = 0;
    topfile--;
  } // while

  // stack is empty, must be end of input
  if (startDebug()) {
    debug() << "Lexer: no more input files\n";
    stopDebug();
  }
  return false;
}

bool lexer_mod::SetInputs(const char** files, int filecount)
{
  if (filecount >= max_file_depth) {
//  error message here!
    return false;
  }
  for (topfile=0; topfile<filecount; topfile++) {
    const char* fname = files[filecount-topfile-1];
    filestack[topfile] = new inputfile(fname);
  };

  // switch to a valid file
  for (topfile--; topfile>=0; topfile--) {
    if (filestack[topfile]->StartTokenizing()) {

      if (startDebug()) {
        debug() << "switching to input file: ";
        debug().Put(filestack[topfile]->Name());
        debug().Put('\n');
        stopDebug();
      }

      return true;
    }

    // couldn't open file
    delete filestack[topfile];
    filestack[topfile] = 0;
  }
  return false;
}


// ==================================================================
//
//                            Global front end
//
// ==================================================================

const location& Where()
{
    return lexdata.Where();
}

void InitLexer(parse_module* pm)
{
  lexdata.Initialize(pm);
}

bool SetInputs(parse_module* pm, const char** files, int filecount)
{
  return lexdata.SetInputs(files, filecount);
}

bool SetInput(parse_module* pm, FILE* file, const char* name)
{
  inputfile* fi = new inputfile(file, name);
  return lexdata.Open(fi);
}

