
// $Id$

#include "../defines.h"
#include "compile.h"
#include "../Base/api.h"

#include <stdio.h>
#include <FlexLexer.h>

extern char* filename;
extern yyFlexLexer lexer;

expr* MakeBoolConst(char* s)
{
  if (strcmp(s, "true") == 0) {
    return MakeConstExpr(true, filename, lexer.lineno());
  }
  if (strcmp(s, "false") == 0) {
    return MakeConstExpr(false, filename, lexer.lineno());
  }
  // error
  Internal.Start(filename, lexer.lineno());
  Internal << "bad boolean constant";
  Internal.Stop();
  return NULL;
}

expr* MakeIntConst(char* s)
{
  int value;
  sscanf(s, "%ld", &value);
  return MakeConstExpr(value, filename, lexer.lineno());
}

expr* MakeRealConst(char* s)
{
  double value;
  sscanf(s, "%lf", &value);
  return MakeConstExpr(value, filename, lexer.lineno());
}

expr* BuildBinary(expr* left, int op, expr* right)
{
  if (NULL==left || NULL==right) {
    Delete(left);
    Delete(right);
    return NULL;
  }

  // type checking goes here

  return MakeBinaryOp(left, op, right, filename, lexer.lineno());
}

void* BuildExprStatement(expr *x)
{
  if (NULL==x) return NULL;

  Output << "Rough expression: " << x << "\n";
  Optimize(0, x);
  Output << "Optimized expression: " << x << "\n";
  result r;
  x->Compute(0, r);
  Output << "Evaluation: " << r.ivalue << "\n";
/*
  statement* s = MakeExprStatement(x, filename, lexer.lineno());
  if (NULL==s) return NULL;

  // remove this eventually...
  s->Execute();
*/

  /* If we're within a loop or something... */
  /*
  List <statement>* a = new List<statement> (256);   
  a->Append(s);
  return a;
  */
  /* If we're not within a loop or anything... */
  return NULL;
}
