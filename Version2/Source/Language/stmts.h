
// $Id$

#ifndef STMTS_H
#define STMTS_H

/** @name stmts.h
    @type File
    @args \ 

  Statements.

  Not sure how much needs to be re-designed here.

 */

#include "exprs.h"

//@{

// ******************************************************************
// *                                                                *
// *                        statement  class                        *
// *                                                                *
// ******************************************************************

/** Base (abstract) class for statements.
   
    For now this is very simple.
    When we add converges, that might change.
 */

class statement {
private:
  /// The statement that contains us (e.g., a for loop), or NULL.
  // statement *parent;
  /// The name of the file we were declared in.
  const char* filename;
  /// The line number of the file we were declared on.
  int linenumber;
public:
  statement(const char* fn, int line) {
    // parent = NULL;
    filename = fn;
    linenumber = line;
  }

  virtual ~statement() { }

  inline const char* Filename() const { return filename; } 
  inline int Linenumber() const { return linenumber; }
  // inline statement* Parent() { return parent; }
  // inline void SetParent(statement *p) { parent = p; }

  /** Execute the statement.
      Must be provided in derived classes.
   */
  virtual void Execute() = 0;

  /** Reset.  Used for statements within models.
  */
  virtual void Clear() { }

  /** Execute guesses.
      Default behavior is to do nothing.
  */
  virtual void InitialGuess() { }

  /** Has the statement converged?
      Default is always return true.
  */
  virtual bool HasConverged() { return true; }

  /** Permanently affix statements after convergence.
      Default behavior is to do nothing.
  */
  virtual void Affix() { }

  /** Make variables have state "CS_Defined".
      Used by converges: once we have seen the close of a converge,
      any variable that is only "guessed" within that converge
      is essentially defined to be itself.
  */
  virtual void GuessesToDefs() { }

  /// Display the statement to the given output stream.
  virtual void show(OutputStream &s) const = 0;

  virtual void showfancy(int dpth, OutputStream &s) const = 0;
};

inline OutputStream& operator<< (OutputStream &s, statement *x)
{
  if (x) x->show(s);
  return s;
}



// ******************************************************************
// *                                                                *
// *              Global functions to build statements              *
// *                                                                *
// ******************************************************************

/// Build a "function call".
statement* MakeExprStatement(expr* e, const char* file, int line);

/// Build a "value" option statement
statement* MakeOptionStatement(option *o, expr *e, const char* file, int line);

/// Build an "enumerated" option statement
statement* MakeOptionStatement(option *o, const option_const *v, 
				const char* file, int line);
  
//@}

#endif


