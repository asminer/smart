
// $Id$

/** @name converge.h
    @type File
    @args \ 

  Everything to do with converge statements.
 
*/


#ifndef CONVERGE_H
#define CONVERGE_H

#include "variables.h"
#include "stmts.h"
#include "arrays.h"


/** Real variables within a converge.
    Members are public because they're used by converge statements.
 */
class cvgfunc : public variable {
public:
  result current;
  result update;
  bool was_updated;
  bool hasconverged;
public:
  cvgfunc(const char *fn, int line, type t, char *n);
  virtual void ClearCache() { } // no cache
  virtual void Compute(Rng *r, const state *st, int i, result &x);
  virtual void ShowHeader(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *mlist);
  void UpdateAndCheck();
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

cvgfunc* MakeConvergeVar(type t, char* id, const char* file, int line);

statement* MakeGuessStmt(cvgfunc* v, expr* guess, const char* file, int line);
statement* MakeAssignStmt(cvgfunc* v, expr* rhs, const char* file, int line);
statement* MakeConverge(statement** block, int bsize, const char* fn, int ln);
statement* MakeArrayCvgGuess(array* f, expr* guess, const char* file, int ln);
statement* MakeArrayCvgAssign(array* f, expr* rhs, const char* file, int ln);

void InitConvergeOptions();

#endif
