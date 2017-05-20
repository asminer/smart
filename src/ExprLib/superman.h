
#ifndef SUPERMAN_H
#define SUPERMAN_H

#include "exprman.h"

#include "../include/list.h"
#include "../include/splay.h"

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         superman class                         *
// *                                                                *
// *                                                                *
// ******************************************************************

/** Implementation of the exprman interface.
    Ok, so the class name is a bit funny, but, whatever.
*/
class superman : public exprman {

  // special error and default expressions
  expr* error_expr;
  expr* default_expr;

  // Type stuff.
  type** reg_type;
  int alloc_types;
  int last_type;

  // Type promotion stuff
  List <general_conv> general_rules;
  List <specific_conv> promotion_rules;
  List <specific_conv> casting_rules;

  // Registered operations, by opcode.
  const unary_op** reg_unary;
  const binary_op** reg_binary;
  const trinary_op** reg_trinary;
  const assoc_op** reg_assoc;

  // List of registered libraries.
  const library** extlibs;
  int num_libs;
  int max_libs;

  // Engine types, pre-Finalize():
  SplayOfPointers <engtype>* ETTree;

  // Engine types, post-Finalize():
  engtype** ETList;
  int num_ets;

 
public:
  superman(io_environ* io, option_manager* om);

  virtual ~superman();

  virtual void finalize();

  // Special expressions

  virtual expr* makeError() const;
  virtual bool isError(const expr* e) const;
  virtual expr* makeDefault() const;
  virtual bool isDefault(const expr* e) const;
  virtual bool isOrdinary(const expr* e) const;

  // Types

  virtual bool registerType(type* t);
  virtual bool setFundamentalTypes();
  virtual const type* findOWDType(const char* name) const;
  virtual simple_type* findSimple(const char* name);
  virtual const type* findType(const char* name) const;
  virtual modifier findModifier(const char* name) const;
  virtual int getNumTypes() const;
  virtual const type* getTypeNumber(int i) const;

  // Promotions and casting

  virtual void registerConversion(general_conv *);
  virtual void registerConversion(specific_conv *);
  virtual int getPromoteDistance(const type* t1, const type* t2) const;
  virtual bool isCastable(const type* t1, const type* t2) const;
  virtual expr* makeTypecast(const char* file, int line, 
      const type* newtype, expr* e) const;
  // TBD: others here?

  // Registering operations

  virtual bool registerOperation(unary_op* op);
  virtual bool registerOperation(binary_op* op);
  virtual bool registerOperation(trinary_op* op);
  virtual bool registerOperation(assoc_op* op);

  // Building expressions with operators

  const type* getTypeOf(unary_opcode op, const type* x) const;
  const type* getTypeOf(const type* l, binary_opcode op, const type* r) const;
  const type* getTypeOf(trinary_opcode op, const type* left, 
      const type* middle, const type* right) const;
  virtual const type* getTypeOf(const type* left, bool flip, assoc_opcode op, 
      const type* right) const;

  virtual expr* makeUnaryOp(const char* file, int line, 
      unary_opcode op, expr* opnd) const;
  virtual expr* makeBinaryOp(const char* fn, int ln, 
      expr* left, binary_opcode op, expr* rt) const;
  virtual expr* makeTrinaryOp(const char* fn, int ln, trinary_opcode op, 
      expr* l, expr* m, expr* r) const;
  virtual expr* makeAssocOp(const char* fn, int ln, assoc_opcode op, 
      expr** opnds, bool* f, int nops) const;

  // Solution engine types

  virtual bool registerEngineType(engtype* et);
  virtual engtype* findEngineType(const char* n) const;
  virtual int getNumEngineTypes() const;
  virtual const engtype* getEngineTypeNumber(int i) const;

  // Supporting  libraries

  virtual char registerLibrary(const library* lib);
  virtual void printLibraryVersions(OutputStream &s) const;
  virtual void printLibraryCopyrights(doc_formatter* df) const;
};


#endif
