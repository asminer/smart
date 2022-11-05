
#include "../Streams/streams.h"
#include "../include/defines.h"
#include "lexer.h"
#include "../ExprLib/exprman.h"

#include <string.h>
#include <stdlib.h>

// Takes care of objects that are required by smart.tab.h:
#include "compile.h"
#include "ParseSM/smartyacc.hh"

#include "parse_sm.h"
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
  named_msg lexer_debug;
  /// Stack of input files.
  inputfile** filestack;
  /// Top of file stack.
  int topfile;

public:
  lexer_mod();
  ~lexer_mod();

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
      parent->em->causedBy(Filename(), Linenumber());
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
  inline bool StackFull() const { return (topfile+1 >= max_file_depth); }

  const char* Filename() const;
  int Linenumber() const;

  inline bool scanForTemporal() const {
    DCASSERT(parent);
    return parent->scanForTemporal();
  }
  inline void stopLexingTemporal() {
    DCASSERT(parent);
    parent->stopLexingTemporal();
  }
  inline void startLexingTemporal() {
    DCASSERT(parent);
    parent->startLexingTemporal();
  }

  void Initialize(parse_module* p);

  bool AlreadyOpen(const char* name) const;

  // returns true on success
  bool Open(inputfile* fi);

  /// Returns true if any files are still open.
  bool CloseCurrent();

  bool SetInputs(const char**, int);
};

lexer_mod lexdata;

// ******************************************************************
// *                                                                *
// *                        inputfile  class                        *
// *                                                                *
// ******************************************************************

class inputfile {
  const char* name;
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
        @return   true  on success,
                  false on error (i.e., couldn't open file)
  */
  bool StartTokenizing();

  /// Stop (perhaps temporarily) consuming tokens from this file.
  void StopTokenizing();

  /// The input file name.
  inline const char* Name() const { return name; }

  /// Current linenumber of input file.
  inline int Line() const {
    return consumed_lines + (yylineno - counter_start);
  }

  /// Are we reading from standard input?
  inline bool is_stdin() const {
    if (name[0] != '-')  return false;
    return 0==name[1];
  }
};

// ******************************************************************
// *                       inputfile  methods                       *
// ******************************************************************

inputfile::inputfile(const char* n)
{
  name = n;
  input = 0;
  buffer = 0;
  consumed_lines = 1;
  counter_start = 0;
}

inputfile::inputfile(FILE* f, const char* n)
{
  name = n;
  input = f;
  buffer = 0;
  consumed_lines = 1;
  counter_start = 0;
}

inputfile::~inputfile()
{
  if (buffer)  yy_delete_buffer(buffer);
  if (input)   if (stdin != input) fclose(input);
  // don't delete name,
  // lots of expressions could be pointing to it!
}

bool inputfile::StartTokenizing()
{
  // open file if necessary
  if (0==input) {
    if (('-'==name[0]) && (0==name[1]))   input = stdin;
    else                                  input = fopen(name, "r");

    if (0==input) {
      if (lexdata.startError()) {
        lexdata.cerr() << "couldn't open file " << name << ", ignoring";
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
    case ENDPND:      return "ENDPND";
    case CONVERGE:    return "CONVERGE";
    case DEFAULT:     return "DEFAULT";
    case FOR:         return "FOR";
    case GUESS:       return "GUESS";
    case IN:          return "IN";
    case NUL:         return "NUL";
    case LPAR:        return "LPAR";
    case RPAR:        return "RPAR";
    case LBRAK:       return "LBRAK";
    case RBRAK:       return "RBRAK";
    case LBRACE:      return "LBRACE";
    case RBRACE:      return "RBRACE";
    case COMMA:       return "COMMA";
    case SEMI:        return "SEMI";
    case COLON:       return "COLON";
    case DOT:         return "DOT";
    case DOTDOT:      return "DOTDOT";
    case GETS:        return "GETS";
    case PLUS:        return "PLUS";
    case MINUS:       return "MINUS";
    case TIMES:       return "TIMES";
    case DIVIDE:      return "DIVIDE";
    case OR:          return "OR";
    case AND:         return "AND";
    case SET_DIFF:    return "SET_DIFF";
    case IMPLIES:     return "IMPLIES";
    case NOT:         return "NOT";
    case EQUALS:      return "EQUALS";
    case NEQUAL:      return "NEQUAL";
    case GT:          return "GT";
    case GE:          return "GE";
    case LT:          return "LT";
    case LE:          return "LE";
    case POUND:       return "POUND";
    case BOOLCONST:   return "BOOLCONST";
    case INTCONST:    return "INTCONST";
    case REALCONST:   return "REALCONST";
    case STRCONST:    return "STRCONST";
    case FORMALISM:   return "FORMALISM";
    case TYPE:        return "TYPE";
    case MODIF:       return "MODIF";
    case PROC:        return "PROC";
    case IDENT:       return "IDENT";
    case FORALL:      return "FORALL";
    case EXISTS:      return "EXISTS";
    case FUTURE:      return "FUTURE";
    case GLOBALLY:    return "GLOBALLY";
    case UNTIL:       return "UNTIL";
    case NEXT:        return "NEXT";
    case TEMPORALAND: return "TEMPORALAND";
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

int ProcessEndpnd()
{
  lexdata.startLexingTemporal();
  return ProcessToken(ENDPND);
}

int ProcessConverge()
{
  return ProcessToken(CONVERGE);
}

int ProcessDefault()
{
  return ProcessToken(DEFAULT);
}

int ProcessFor()
{
  return ProcessToken(FOR);
}

int ProcessGuess()
{
  return ProcessToken(GUESS);
}

int ProcessIn()
{
  return ProcessToken(IN);
}

int ProcessNull()
{
  return ProcessToken(NUL);
}

int ProcessLpar()
{
  return ProcessToken(LPAR);
}

int ProcessRpar()
{
  return ProcessToken(RPAR);
}

int ProcessLbrak()
{
  return ProcessToken(LBRAK);
}

int ProcessRbrak()
{
  return ProcessToken(RBRAK);
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

int ProcessColon()
{
  return ProcessToken(COLON);
}

int ProcessDot()
{
  return ProcessToken(DOT);
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

int ProcessSetDiff()
{
  return ProcessToken(SET_DIFF);
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

int ProcessPound()
{
  lexdata.stopLexingTemporal();
  return ProcessToken(POUND);
}

int ProcessTemporalAnd()
{
  return ProcessToken(TEMPORALAND);
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

int ProcessReal()
{
  yylval.name = strdup(yytext);
  return ProcessToken(REALCONST);
}

int ProcessString()
{
  // This is adapted from page 84 of the "Lex & Yacc" book.

  yylval.name = strdup(yytext+1);  // skip open quote

  if (yylval.name[yyleng-2] != '"') {
    if (lexdata.startError()) {
      lexdata.cerr() << "Unclosed quote";
      lexdata.stopError();
    }
  } else
    yylval.name[yyleng-2] = '\0'; // erase close quote

  return ProcessToken(STRCONST);
}

char isTemporalActive()
{
  return lexdata.scanForTemporal();
}

char isTemporalOperator(char x)
{
  return
    ('A' == x) ||
    ('E' == x) ||
    ('F' == x) ||
    ('G' == x) ||
    ('U' == x) ||
    ('X' == x)
  ;
}

int ProcessTemporalOperator()
{
  switch (yytext[0]) {
    case 'A':   return ProcessToken(FORALL);
    case 'E':   return ProcessToken(EXISTS);
    case 'F':   return ProcessToken(FUTURE);
    case 'G':   return ProcessToken(GLOBALLY);
    case 'U':   return ProcessToken(UNTIL);
    // weak until?
    case 'X':   return ProcessToken(NEXT);
    default:
      if (lexdata.startInternal(__FILE__, __LINE__)) {
        lexdata.internal() << "Temporal operator error, unknown operator '";
        lexdata.internal() << yytext[0] << "'.";
        lexdata.stopError();
      }
      return 0;
  }
}

int ProcessID()
{
  yylval.name = 0;
  const type* t = lexdata.FindOWDType(yytext);
  if (t) {
    yylval.Type_ID = t;
    if (t->isAFormalism())  return ProcessToken(FORMALISM);
    return ProcessToken(TYPE);
  }
  if (strcmp(yytext, "proc")==0) return ProcessToken(PROC);
  yylval.name = strdup(yytext);
  if (lexdata.FindModif(yytext)!=NO_SUCH_MODIFIER) return ProcessToken(MODIF);
  return ProcessToken(IDENT);
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
  for(start=0; start<tokenlen; start++) if (token[start]=='"') break;
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
}

lexer_mod::~lexer_mod()
{
  delete[] filestack;
}

const char* lexer_mod::Filename() const
{
  return (topfile<0) ? "<" : filestack[topfile]->Name();
}

int lexer_mod::Linenumber() const
{
  return (topfile<0) ? -1 : filestack[topfile]->Line();
}

void lexer_mod::Initialize(parse_module* p)
{
  if (p == parent)  return;
  parent = p;
  option* debug = parent ? parent->findOption("Debug") : 0;
  lexer_debug.Initialize(debug,
    "lexer",
    "When set, very low-level lexer messages are displayed.",
    false
  );
#ifdef LEXER_DEBUG
  lexer_debug.active = true;
#endif
}

bool lexer_mod::AlreadyOpen(const char* name) const
{
  for (int i=0; i<=topfile; i++) {
    if (0==strcmp(name, filestack[i]->Name()))  return true;
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
    debug() << "no more input files\n";
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

const char* Filename()
{
  return lexdata.Filename();
}

int Linenumber()
{
  return lexdata.Linenumber();
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

