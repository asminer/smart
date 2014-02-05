
// $Id$

/** \file symtabs.h

  New symbol table module.

*/

#ifndef SYMTABS_H
#define SYMTABS_H

/// Defined in symbols.h
class symbol;

/// Defined in streams.h
class doc_formatter;

/** Symbol table interface.

*/
class symbol_table {
protected:
  long num_syms;
  long num_names;
public:
  symbol_table();
  virtual ~symbol_table();

  /// The number of symbols stored in the table.
  inline long NumSymbols() const { return num_syms; }

  /// The number of unique names stored in the table.
  inline long NumNames() const { return num_names; }

  /// Add the symbol s to the table.
  virtual void AddSymbol(symbol* s) = 0;

  /// Find a (list of) symbol matching the given name, otherwise null.
  virtual symbol* FindSymbol(const char* name) = 0;
  
  /// Remove the given symbol.  Return true if the item was in the table.
  virtual bool RemoveSymbol(symbol* s) = 0;

  /** Pop and return the last added symbol.
      This only works if there is no chaining, i.e., 
      symbol names are unique; if this might not be the case, we return 0.
  */
  virtual symbol* Pop() = 0;

  /** Return the ith symbol added.
      Like Pop(), this works only if there is no chaining.
        @param  i  The symbol number to return.
        @return 0, if i is out of range; a valid symbol, otherwise.
  */
  virtual symbol* GetItem(int n) const = 0;

  /** Grab a copy of all symbols, in order.
      Used primarily for documentation.
  */
  virtual void CopyToArray(const symbol** list) const = 0;

  void DocumentSymbols(doc_formatter* df, const char* keyword) const;
};


symbol_table* MakeSymbolTable();  // any params necessary?

#endif
