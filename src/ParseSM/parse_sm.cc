
#include "lexer.h"
#include "compile.h"
#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "parse_sm.h"


parse_module::parse_module(exprman* the_em)
{
  em = the_em;
  builtins = 0;

  compiler_ready = false;
}

void parse_module::Initialize()
{
  if (compiler_ready)  return;
  InitLexer(this);
  InitCompiler(this);
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
