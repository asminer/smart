
// $Id$

#ifndef SYMBOLS_H
#define SYMBOLS_H

/** @name symbols.h
    @type File
    @args \ 

  Base class for symbols.
  That's all.

 */

#include "types.h"

//@{
  



// ******************************************************************
// *                                                                *
// *                          symbol class                          *
// *                                                                *
// ******************************************************************

/**   The base class of all symbols.
      That includes formal parameters, for loop iterators,
      functions, and model objects like states, places, and transitions.

*/  

class symbol {
  private:
    /// The name of the file we were declared in.
    const char* filename;  
    /// The line number of the file we were declared on.
    int linenumber;  
    /// The symbol name.
    char* name;
    /// The symbol type.
    type mytype;

  public:

  symbol(const char* fn, int line, type t, char* n) {
    filename = fn;
    linenumber = line;
    mytype = t;
    name = n;
  }

  virtual ~symbol() { 
    delete[] name;
  }

  inline const char* Filename() const { return filename; } 
  inline int Linenumber() const { return linenumber; }
  inline type Type() const { return mytype; }
  inline const char* Name() const { return name; }

  // Do we need a way to distinguish between functions, parameters, etc?

  /// Display the symbol to the given output stream.
  virtual void show(ostream &s) const = 0;
};

inline ostream& operator<< (ostream &s, symbol *x)
{
  if (x) x->show(s);
  else s << "null";
  return s;
}


//@}

#endif

