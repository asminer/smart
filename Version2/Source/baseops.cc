
// $Id$

#include "baseops.h"
//@Include: baseops.h

/** @name baseops.cc
    @type File
    @args \ 

   Implementation of base expression classes for things like addition.

 */

//@{

// ******************************************************************
// *                                                                *
// *                          negop  class                          *
// *                                                                *
// ******************************************************************

negop::negop(const char* fn, int line, expr* x) : expr(fn, line) 
{ 
  opnd = x;
}

negop::~negop() 
{
  delete opnd;
}

void negop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  char op = '-';
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    op = '!';
  s << op << opnd;
}

// ******************************************************************
// *                                                                *
// *                          addop  class                          *
// *                                                                *
// ******************************************************************

addop::addop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

addop::~addop() {
  delete left;
  delete right;
}

void addop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  char opnd = '+';
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    opnd = '|';
  s << "(" << left << opnd << right << ")";
}

int addop::GetSums(expr **sums=NULL, int N=0, int offset=0)
{
  int answer = 0;
  if (left) {
    answer = left->GetSums(sums, N, offset);
  } else {
    answer = 1;
    if (offset<N) sums[offset] = NULL;
  }
  if (right) {
    answer += right->GetSums(sums, N, offset+answer);
  } else {
    if (offset+answer<N) sums[offset+answer] = NULL;
    answer++;
  }
  return answer;
}

// ******************************************************************
// *                                                                *
// *                          subop  class                          *
// *                                                                *
// ******************************************************************

subop::subop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

subop::~subop() {
  delete left;
  delete right;
}

void subop::show(ostream &s) const 
{
  s << "(" << left << "-" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                          multop class                          *
// *                                                                *
// ******************************************************************

multop::multop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

multop::~multop() {
  delete left;
  delete right;
}

void multop::show(ostream &s) const 
{
  type t = Type(0); // sneaky...  this is slow but so's I/O so who cares
  char opnd = '*';
  if ((t==BOOL) || (t==RAND_BOOL) || (t==PROC_BOOL) || (t==PROC_RAND_BOOL))
    opnd = '&';
  s << "(" << left << opnd << right << ")";
}

int multop::GetProducts(expr **prods=NULL, int N=0, int offset=0)
{
  int answer = 0;
  if (left) {
    answer = left->GetProducts(prods, N, offset);
  } else {
    answer = 1;
    if (offset<N) prods[offset] = NULL;
  }
  if (right) {
    answer += right->GetProducts(prods, N, offset+answer);
  } else {
    if (offset+answer<N) prods[offset+answer] = NULL;
    answer++;
  }
  return answer;
}

// ******************************************************************
// *                                                                *
// *                          divop  class                          *
// *                                                                *
// ******************************************************************

divop::divop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

divop::~divop() {
  delete left;
  delete right;
}

void divop::show(ostream &s) const 
{
  s << "(" << left << "/" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        consteqop  class                        *
// *                                                                *
// ******************************************************************

consteqop::consteqop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

consteqop::~consteqop() 
{
  delete left;
  delete right;
}

void consteqop::show(ostream &s) const 
{
  s << "(" << left << "==" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constneqop class                        *
// *                                                                *
// ******************************************************************

constneqop::constneqop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

constneqop::~constneqop() 
{
  delete left;
  delete right;
}

void constneqop::show(ostream &s) const 
{
  s << "(" << left << "!=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constgtop  class                        *
// *                                                                *
// ******************************************************************

constgtop::constgtop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

constgtop::~constgtop() 
{
  delete left;
  delete right;
}

void constgtop::show(ostream &s) const 
{
  s << "(" << left << ">" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constgeop  class                        *
// *                                                                *
// ******************************************************************

constgeop::constgeop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

constgeop::~constgeop() 
{
  delete left;
  delete right;
}

void constgeop::show(ostream &s) const 
{
  s << "(" << left << ">=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constltop  class                        *
// *                                                                *
// ******************************************************************

constltop::constltop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

constltop::~constltop() 
{
  delete left;
  delete right;
}

void constltop::show(ostream &s) const 
{
  s << "(" << left << "<" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                        constleop  class                        *
// *                                                                *
// ******************************************************************

constleop::constleop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

constleop::~constleop() 
{
  delete left;
  delete right;
}

void constleop::show(ostream &s) const 
{
  s << "(" << left << "<=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

eqop::eqop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

eqop::~eqop() 
{
  delete left;
  delete right;
}

void eqop::show(ostream &s) const 
{
  s << "(" << left << "==" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

neqop::neqop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

neqop::~neqop() 
{
  delete left;
  delete right;
}

void neqop::show(ostream &s) const 
{
  s << "(" << left << "!=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

gtop::gtop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

gtop::~gtop() 
{
  delete left;
  delete right;
}

void gtop::show(ostream &s) const 
{
  s << "(" << left << ">" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

geop::geop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

geop::~geop() 
{
  delete left;
  delete right;
}

void geop::show(ostream &s) const 
{
  s << "(" << left << ">=" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

ltop::ltop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

ltop::~ltop() 
{
  delete left;
  delete right;
}

void ltop::show(ostream &s) const 
{
  s << "(" << left << "<" << right << ")";
}

// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

leop::leop(const char* fn, int line, expr* l, expr* r) : expr(fn, line) 
{ 
  left = l;
  right = r;
}

leop::~leop() 
{
  delete left;
  delete right;
}

void leop::show(ostream &s) const 
{
  s << "(" << left << "<=" << right << ")";
}

//@}

