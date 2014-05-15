
// $Id$

/**
  \file formalism.h
  The formalism interface is specified here.
  This file should be included only if you are
  building a modeling formalism.
  (Or, if you are the formalism manager.)
*/

#ifndef FORMALISM_H
#define FORMALISM_H

#include "../include/list.h"
#include "../Streams/streams.h"
#include "../SymTabs/symtabs.h"
#include "type.h"

class model_def;
class msr_func;

class formalism : public simple_type {
  symbol_table* funcs;
  symbol_table* idents;
public:
  formalism(const char* n, const char* sd, const char* ld);
  virtual ~formalism();

  virtual bool isAFormalism() const;


  // Formalism-specific functions
  inline void setFunctions(symbol_table* st) {
    DCASSERT(0==funcs);
    funcs = st;
  }
  inline symbol* findFunction(const char* n) const {
    if (funcs)  return funcs->FindSymbol(n);
    else        return 0;
  }
  inline long numFuncNames() const {
    if (funcs)  return funcs->NumNames();
    else        return 0;
  }
  inline void copyFuncsToArray(const symbol** list) const {
    if (funcs)  funcs->CopyToArray(list);
  }
  void addCommonFuncs(List <msr_func> *cfl);


  // Formalism-specific identifiers
  inline void setIdentifiers(symbol_table* st) {
    DCASSERT(0==idents);
    idents = st;
  }
  inline symbol* findIdentifier(const char* n) const {
    if (idents) return idents->FindSymbol(n);
    else        return 0;
  }
  inline long numIdentNames() const {
    if (idents) return idents->NumNames();
    else        return 0;
  }
  inline void copyIdentsToArray(const symbol** list) const {
    if (idents) idents->CopyToArray(list);
  }

  // Required in derived classes:

  virtual model_def* makeNewModel(const char* fn, int ln, char* name,
                                  symbol** formals, int np) const = 0;

  virtual bool canDeclareType(const type* vartype) const = 0;
  virtual bool canAssignType(const type* vartype) const = 0;

  // Default behavior is provided for this one:
  virtual bool isLegalMeasureType(const type* mtype) const;

protected:
  // types of measures that should be included.

  /// Default: false
  virtual bool includeCTL() const;

  /// Default: false
  virtual bool includeStochastic() const;

  /// Default: false
  virtual bool includeDCP() const;
};

#endif
