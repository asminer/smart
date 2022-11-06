
#include "lexer.h"
#include "compile.h"
#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "../Options/optman.h"
#include "parse_sm.h"


parse_module::parse_module(exprman* the_em)
{
  em = the_em;
  builtins = 0;

  compiler_ready = false;

  lex_temporal_operators = true;
  temporal_operator_option = false;
  minimum_trace_option = false;
}

void parse_module::Initialize()
{
  if (compiler_ready)  return;
  InitLexer(this);
  InitCompiler(this);

  // Build option for CTL/LTL parsing
  if (em->OptMan()) {
    em->OptMan()->addBoolOption(
      "ParseTemporalOperators",
      "Should the parser treat letters A, E, F, G, U, and X as temporal operators for CTL and LTL formulas?  If true, then these become reserved letters and may not appear in any identifier.",
      temporal_operator_option
    );
    em->OptMan()->addBoolOption(
      "MinimumTrace",
      "Whether to generate the minimum trace.",
      minimum_trace_option
    );
  }

  compiler_ready = true;
}

int parse_module::ParseSmartFiles(const char** files, int count)
{
  if (SetInputs(this, files, count)) return Compile(this);

  return -1;
}

int parse_module::ParseSmartFile(FILE* file, const char* name)
{
  if (SetInput(this, file, name)) return Compile(this);

  return -1;
}

const char* parse_module::filename() const
{
  return Filename();
}

int parse_module::linenumber() const
{
  return Linenumber();
}
