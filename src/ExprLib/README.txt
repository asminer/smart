
README file for ExprLib module.

Overall:

  If you just want to "use" expressions, probably you can
  get away with

  #include "expr.h"

  But if you want to construct expressions, you will need (at least)

  #include "exprman.h"

Void expression (statements) computation:

  These can modify the ivalue on the traverse_data.answer:

  set to -1:  there was an error
  set to 1:  no error, but we'd like to be computed again
      (for converge, i.e., not converged yet)

GetSymbols, GetProducts

  These return a list of expressions that should not be Deleted.
  In the future this may change.
