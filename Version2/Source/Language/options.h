
// $Id$

/** @name options.h
    @type File
    @args \

    New option interface.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

#include "types.h"

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
  virtual void ShowHeader(OutputStream &s) const = 0;
  virtual void GetValue(result &x) = 0;
  virtual void SetValue(const result &x) = 0;
};

/** For action options, use the following declaration:

    void MyGetAction(result &x);
*/
typedef void (*action_get) (result &x);


/** For action options, use the following declaration:

    void MySetAction(const result &x);
*/
typedef void (*action_set) (const result &x);


// **************************************************************************
// *                            Global interface                            *
// **************************************************************************

/**
*/
option* MakeEnumOption(const char *name, const char* doc, 
			int deflt, int range);

/**
*/
option* MakeActionOption(const char *name, const char* doc,
			action_get g, action_set s);

void AddOption(option *);
void SortOptions();
option* FindOption(char* name);

bool GetBoolOption(option *);
int GetIntOption(option *);
double GetRealOption(option *);
char* GetStringOption(option *);

#endif

