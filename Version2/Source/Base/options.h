
// $Id$

/** @name options.h
    @type File
    @args \

    New option interface.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#include "output.h"
#include "types.h"

/** Structure for option constants.
    Used by enumeration options;
    e.g., #Solver JACOBI.
    (JACOBI is an option_const).
*/
struct option_const {
  /// Name of the constant.
  const char* name;
  /// Documentation.
  const char* doc;
  // handy...
  option_const(const char* n, const char* d) { name = n; doc = d; }
};

/**  Base class for options.
     Derived classes are "hidden" in options.cc.
*/
class option {
  type mytype;
  const char* name;
  const char* documentation;
  bool hidden;
public:
  option(type t, const char* n, const char* d);
  virtual ~option();
  inline const char* Name() const { return name; }
  inline type Type() const { return mytype; }
  inline const char* GetDocumentation() const { return documentation; }

  inline bool IsUndocumented() const { return hidden; }
  inline void Hide() { hidden = true; }

  inline void show(OutputStream &s) const { s << "#" << name; }

  // provided in derived classes
  virtual void SetValue(bool b, const char* file, int line);
  virtual void SetValue(int n, const char* file, int line);
  virtual void SetValue(double r, const char* file, int line);
  virtual void SetValue(char *c, const char* file, int line);
  virtual void SetValue(const option_const*, const char* file, int line);

  virtual const option_const* FindConstant(const char* name) const;

  virtual bool GetBool() const;
  virtual int GetInt() const;
  virtual double GetReal() const;
  virtual char* GetString() const;
  virtual const option_const* GetEnum() const;

  virtual void ShowHeader(OutputStream &s) const = 0;
  virtual void ShowRange(OutputStream &s) const = 0;
};

OutputStream& operator<< (OutputStream &s, option* o);

/** For action options, use the following declaration:
    void MyActionGet(void *x);
*/
typedef void (*action_get) (void *x);


/** For action options, use the following declaration:
    void MyActionSet(void *x, const char* filename, int lineno);
*/
typedef void (*action_set) (void *x, const char* f, int l);


// **************************************************************************
// *                            Global interface                            *
// **************************************************************************

option* MakeEnumOption(const char* name, const char* doc, 
		       option_const** values, int numvalues,
		       const option_const* deflt);

option* MakeBoolOption(const char* name, const char* doc, bool deflt);

option* MakeIntOption(const char* name, const char* doc,
			int deflt, int min, int max);

option* MakeRealOption(const char* name, const char* doc,
			double deflt, double min, double max);

option* MakeStringOption(const char* name, const char* doc, char* deflt);

option* MakeActionOption(type t, const char *name, const char* doc, 
			const char* range, action_set s, action_get g);


void StartOptions();
void AddOption(option *);
void SortOptions();
option* FindOption(char* name);

int NumOptions();
option* GetOptionNumber(int i);



#endif

