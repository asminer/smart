
#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

/** \file lexer.h
   Utilities for tokenization of SMART input files.
   Also, minimalist front-end functions to obtain
   the current file being processed, and linenumber.
*/
 
// Handy for lexer and parser.

const char* TokenName(int tk);

/* =====================================================================

  Functions used by the lexer.
  These are intended to be called from within smartlex.l
  and are probably of no use otherwise.

   ===================================================================== */

/// End of line when an option is processed.
int ProcessEndpnd();

/// "converge" keyword.
int ProcessConverge();

/// "default" keyword.
int ProcessDefault();

/// "for" keyword.
int ProcessFor();

/// "guess" keyword.
int ProcessGuess();

/// "in" keyword.
int ProcessIn();

/// "null" keyword.
int ProcessNull();

/// "true" or "false" token.
int ProcessBool();

/// integer token.
int ProcessInt();

/// real token.
int ProcessReal();

/// quoted string token.
int ProcessString();

/// identifier.  Could be type, or modifier, or identifier.
int ProcessID();

/// left parenthesis.
int ProcessLpar();

/// right parenthesis.
int ProcessRpar();

/// left bracket.
int ProcessLbrak();

/// right bracket.
int ProcessRbrak();

/// left brace.
int ProcessLbrace();

/// right brace.
int ProcessRbrace();

/// comma.
int ProcessComma();

/// semicolon.
int ProcessSemi();

/// colon.
int ProcessColon();

/// single dot.
int ProcessDot();

/// double dots.
int ProcessDotdot();

/// assignment operator.
int ProcessGets();

/// plus operator.
int ProcessPlus();

/// minus operator.
int ProcessMinus();

/// multiplication operator.
int ProcessTimes();

/// division operator.
int ProcessDivide();

/// modulo operator.
int ProcessMod();

/// logical or.
int ProcessOr();

/// logical and.
int ProcessAnd();

/// set difference.
int ProcessSetDiff();

/// implication.
int ProcessImplies();

/// logical negation.
int ProcessNot();

/// Equality.
int ProcessEquals();

/// Inequality.
int ProcessNequal();

/// Greater than.
int ProcessGt();

/// Greater or equal.
int ProcessGe();

/// Less than.
int ProcessLt();

/// Less or equal.
int ProcessLe();

/// Pound.
int ProcessPound();

/// Process an unclosed comment
void UnclosedComment();

/// We hit an illegal character
void IllegalToken();

/// Process the include tokens
void Include();

/// Handle end of file
int yywrap();

#define YY_SKIP_YYWRAP

/* =====================================================================

  Front-end functions, used by the parser.

   ===================================================================== */

class parse_module;

/** Perform any initialization required.
      @param  pm  Environment.
*/
void InitLexer(parse_module* pm);

/** Set the input files for tokens.
      @param pm         Environment (for error messages and such)
      @param files      List of files to tokenize.
      @param filecount  Number of files to tokenize.
      @return true      iff at least one file could be opened.
              false     otherwise.
*/
bool SetInputs(parse_module* pm, const char** files, int filecount);

/** Set the input file for tokens.
      @param pm     Environment (for error messages and such)
      @param file   File to use, already opened.
      @param name   Name to display for errors.
      @return true  iff the file is ready for input
              false otherwise.
*/
bool SetInput(parse_module* pm, FILE* file, const char* name);

/// The current file being read.
const char* Filename();

/// The linenumber of the current file.
int Linenumber();

#endif
