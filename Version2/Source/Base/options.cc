
// $Id$

#include "options.h"
#include "errors.h"
#include "../defines.h"
#include "../heap.h"

// **************************************************************************
// *                             option methods                             *
// **************************************************************************

option::option(const char *n, const char* d)
{
  name = n;
  documentation = d;
  hidden = false;
}

option::~option()
{
}

void option::SetValue(bool, const char *f, int l)
{
  Internal.Start(__FILE__, __LINE__, f, l);
  Internal << "Bad call to option::SetValue(bool)";
  Internal.Stop();
}

void option::SetValue(int, const char *f, int l)
{
  Internal.Start(__FILE__, __LINE__, f, l);
  Internal << "Bad call to option::SetValue(int)";
  Internal.Stop();
}

void option::SetValue(double, const char *f, int l)
{
  Internal.Start(__FILE__, __LINE__, f, l);
  Internal << "Bad call to option::SetValue(double)";
  Internal.Stop();
}

void option::SetValue(char*, const char *f, int l)
{
  Internal.Start(__FILE__, __LINE__, f, l);
  Internal << "Bad call to option::SetValue(char*)";
  Internal.Stop();
}

bool option::GetBool() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Bad call to option::GetBool()";
  Internal.Stop();
  return 0;
}

int option::GetInt() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Bad call to option::GetInt()";
  Internal.Stop();
  return 0;
}

double option::GetReal() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Bad call to option::GetReal()";
  Internal.Stop();
  return 0;
}

char* option::GetString() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Bad call to option::GetString()";
  Internal.Stop();
  return NULL;
}


// **************************************************************************
// *                            boolean  options                            *
// **************************************************************************

class bool_opt : public option {
  bool value;
public:
  bool_opt(const char* n, const char* d, bool v) : option(n,d) { value = v; }
  virtual ~bool_opt() { } 
  virtual void SetValue(bool b, const char *, int) { value = b; }
  virtual bool GetBool() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    s << "#" << Name() << " " << value;
  }
};

option* MakeBoolOption(const char* name, const char* doc, bool deflt)
{
  return new bool_opt(name, doc, deflt);
}

// **************************************************************************
// *                            integer  options                            *
// **************************************************************************

class int_opt : public option {
  int max;
  int min;
  int value;
public:
  int_opt(const char* n, const char* d, int v, int mx, int mn) : option(n,d) 
  { value = v; max = mx; min = mn; }
  virtual ~int_opt() { }
  virtual void SetValue(int b, const char* f, int l) { 
    if (min<max) {
      if ((value<min) || (value>max)) {
	Warn.Start(f, l);
	Warn << "Illegal value for option " << Name() << ", ignoring";
	Warn.Stop();
	return;
      }
    }
    value = b; 
  }
  virtual int GetInt() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    s << "#" << Name() << " ";
    if (min<max) s << "[" << min << ".." << max << "] ";
    s << value;
  }
};

option* MakeIntOption(const char* name, const char* doc, 
			int v, int min, int max)
{
  return new int_opt(name, doc, v, min, max);
}

// **************************************************************************
// *                              real options                              *
// **************************************************************************

class real_opt : public option {
  double max;
  double min;
  double value;
public:
  real_opt(const char* n, const char* d, double v, double mx, double mn)
   : option(n,d) { value = v; max = mx; min = mn; }
  ~real_opt() { } 
  virtual void SetValue(double b, const char* f, int l) { 
    if (min<max) {
      if ((value<min) || (value>max)) {
	Warn.Start(f, l);
	Warn << "Illegal value for option " << Name() << ", ignoring";
	Warn.Stop();
	return;
      }
    }
    value = b; 
  }
  virtual double GetReal() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    s << "#" << Name() << " ";
    if (min<max) s << "[" << min << ".." << max << "] ";
    s << value;
  }
};

option* MakeRealOption(const char* name, const char* doc, 
			double v, double min, double max)
{
  return new real_opt(name, doc, v, min, max);
}


// **************************************************************************
// *                             string options                             *
// **************************************************************************

class string_opt : public option {
  char* value;
public:
  string_opt(const char* n, const char* d, char *v) : option(n,d) { value = v; }
  virtual ~string_opt() { delete[] value; }
  virtual void SetValue(char *v, const char*, int) { 
    delete[] value; value = v; 
  }
  virtual char* GetString() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    s << "#" << Name() << " ";
    if (value) s << '"' << value << '"';
    else s << "null";
  }
};

option* MakeStringOption(const char* name, const char* doc, char* v)
{
  return new string_opt(name, doc, v);
}


// **************************************************************************
// *                             action options                             *
// **************************************************************************

class action_opt : public option {
  action_set Set; 
  action_get Get;
protected:
  inline void SetVoid(void* x, const char *f, int l) {
    if (NULL==Set) {
      Internal.Start(__FILE__, __LINE__, f, l);
      Internal << "Bad Set for action option";
      Internal.Stop();
      return;
    }
    Set(x, f, l);
  }
  inline void GetVoid(void* x) {
    if (NULL==Get) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Bad Get for action option";
      Internal.Stop();
      return;
    }
    Get(x);
  }
public:
  action_opt(const char* n, const char* d, action_set s, action_get g)
   : option(n,d) { Set = s; Get = g; }
  virtual ~action_opt() { }
  virtual void SetValue(bool x, const char* f, int l) { SetVoid(&x, f, l); }
  virtual void SetValue(int x, const char* f, int l) { SetVoid(&x, f, l); }
  virtual void SetValue(double x, const char* f, int l) { SetVoid(&x, f, l); }
  virtual void SetValue(char* x, const char* f, int l) { SetVoid(&x, f, l); }

  virtual bool GetBool() { bool b; Get(&b); return b; }
  virtual int GetInt() { int b; Get(&b); return b; }
  virtual double GetReal() { double b; Get(&b); return b; }
  virtual char* GetString() { char* b; Get(&b); return b; }

  virtual void ShowHeader(OutputStream &s) const { s << "#" << Name(); }
};

option* MakeActionOption(const char* name, const char* doc, 
			action_set s, action_get g)
{
  return new action_opt(name, doc, s, g);
}


// **************************************************************************
// *                              enum options                              *
// **************************************************************************

option* MakeEnumOption(const char*, const char*, int d, int r)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Enumerated options not done yet\n";
  Internal.Stop();
  return NULL;
}


// **************************************************************************
// *                                                                        *
// *                         Option list management                         *
// *                                                                        *
// **************************************************************************

Heap <option> optionlist(128);
option** SortedOptions;
int NumSortedOptions;

inline int Compare(option *a, option *b) 
{ 
  return strcmp(a->Name(), b->Name()); 
}

void StartOptions()
{
  SortedOptions = NULL;
  NumSortedOptions = 0;
}

void AddOption(option *o)
{
  DCASSERT(NULL==SortedOptions);
  optionlist.Insert(o);
}

void SortOptions()
{
  DCASSERT(NULL==SortedOptions);
  optionlist.Sort();
  NumSortedOptions = optionlist.Length();
  SortedOptions = optionlist.MakeArray();
}

option* FindOption(char* name)
{
  DCASSERT(SortedOptions);
  // binary search
  int low = 0;
  int high = NumSortedOptions;
  while (low < high) {
    int mid = (low+high)/2;
    int cmp = strcmp(SortedOptions[mid]->Name(), name);
    if (0==cmp) return SortedOptions[mid];
    if (cmp>0) {
      high = mid;
    } else {
      low = mid+1;
    }
  }
  // not found
  return NULL;
}

int NumOptions() 
{
  return NumSortedOptions;
}

option* GetOptionNumber(int i)
{
  DCASSERT(SortedOptions);
  return SortedOptions[i];
}

