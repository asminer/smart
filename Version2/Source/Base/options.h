
// $Id$

/** @name options.h
    @type File
    @args \

    New option interface.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#include "output.h"

/**  Base class for options.
     Derived classes are "hidden" in options.cc.
*/
class option {
  const char* name;
  const char* documentation;
  bool hidden;
public:
  option(const char* n, const char* d);
  virtual ~option();
  inline void Hide() { hidden = true; }
  inline const char* Name() const { return name; }
  inline bool IsUndocumented() const { return hidden; }
  inline const char* GetDocumentation() const { return documentation; }

  // provided in derived classes

  virtual void SetValue(bool b, const char* file, int line);
  virtual void SetValue(int n, const char* file, int line);
  virtual void SetValue(double r, const char* file, int line);
  virtual void SetValue(char *c, const char* file, int line);

  virtual bool GetBool() const;
  virtual int GetInt() const;
  virtual double GetReal() const;
  virtual char* GetString() const;

  virtual void ShowHeader(OutputStream &s) const = 0;
};

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
			int deflt, int range);

option* MakeBoolOption(const char* name, const char* doc, bool deflt);

option* MakeIntOption(const char* name, const char* doc,
			int deflt, int min, int max);

option* MakeRealOption(const char* name, const char* doc,
			double deflt, double min, double max);

option* MakeStringOption(const char* name, const char* doc, char* deflt);

option* MakeActionOption(const char *name, const char* doc,
			action_set s, action_get g);


void StartOptions();
void AddOption(option *);
void SortOptions();
option* FindOption(char* name);

int NumOptions();
option* GetOptionNumber(int i);



#endif

