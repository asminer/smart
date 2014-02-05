
// $Id$

#include "lexer.h"
#include "compile.h"
#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "parse_icp.h"


parse_module::parse_module(exprman* the_em)
{
  em = the_em;
  
  num_measures = 0;
  measure_names = 0;
  measure_calls = 0;

  compiler_ready = false;
}

void parse_module::Initialize()
{
  if (compiler_ready)  return;
  InitLexer(this);
  InitCompiler(this);
  compiler_ready = true;
  StartModel();
}

int parse_module::ParseICPFiles(const char** files, int count)
{
  if (SetInputs(this, files, count))    return Compile(this);

  return -1;
}

int parse_module::ParseICPFile(FILE* file, const char* name)
{
  if (SetInput(this, file, name))     return Compile(this);

  return -1;
}

void parse_module::Finish()
{
  FinishModel();
}

const char* parse_module::filename() const 
{
  return Filename();
}

int parse_module::linenumber() const
{
  return Linenumber();
}
