
// $Id$

#include "../defines.h"
#include "options.h"
#include "../Templates/heap.h"

// **************************************************************************
// *                             option methods                             *
// **************************************************************************

OutputStream& operator<< (OutputStream &s, option *o)
{
  if (NULL==o) s << "(null)"; else o->show(s);
  return s;
}

option::option(type t, const char *n, const char* d)
{
  mytype = t;
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

void option::SetValue(const option_const*, const char *f, int l)
{
  Internal.Start(__FILE__, __LINE__, f, l);
  Internal << "Bad call to option::SetValue(option_const*)";
  Internal.Stop();
}

const option_const* option::FindConstant(const char*) const
{
  return NULL;
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

const option_const* option::GetEnum() const
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Bad call to option::GetEnum()";
  Internal.Stop();
  return NULL;
}


// **************************************************************************
// *                            boolean  options                            *
// **************************************************************************

class bool_opt : public option {
  bool value;
public:
  bool_opt(const char* n, const char* d, bool v)
   : option(BOOL, n, d) { value = v; }
  virtual ~bool_opt() { } 
  virtual void SetValue(bool b, const char *, int) { value = b; }
  virtual bool GetBool() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " " << value;
  }
  virtual void ShowRange(OutputStream &s) const {
    s << "[false, true]\n";
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
  int_opt(const char* n, const char* d, int v, int mn, int mx)
   : option(INT, n ,d) { value = v; max = mx; min = mn; }
  virtual ~int_opt() { }
  virtual void SetValue(int b, const char* f, int l) { 
    if (min<max) {
      if ((value<min) || (value>max)) {
	Warning.Start(f, l);
	Warning << "Illegal value for option " << Name() << ", ignoring";
	Warning.Stop();
	return;
      }
    }
    value = b; 
  }
  virtual int GetInt() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " " << value;
  }
  virtual void ShowRange(OutputStream &s) const {
    if (min<max)
      s << "[" << min << ".." << max << "]\n";
    else
      s << "any integer\n";
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
  real_opt(const char* n, const char* d, double v, double mn, double mx)
   : option(REAL, n, d) { value = v; max = mx; min = mn; }
  ~real_opt() { } 
  virtual void SetValue(double b, const char* f, int l) { 
    if (min<max) {
      if ((value<min) || (value>max)) {
	Warning.Start(f, l);
	Warning << "Illegal value for option " << Name() << ", ignoring";
	Warning.Stop();
	return;
      }
    }
    value = b; 
  }
  virtual double GetReal() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    s << " " << value;
  }
  virtual void ShowRange(OutputStream &s) const {
    if (min<max)
      s << "[" << min << ".." << max << "]\n";
    else
      s << "any real value\n";
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
  string_opt(const char* n, const char* d, char *v)
   : option(STRING, n, d) { value = v; }
  virtual ~string_opt() { delete[] value; }
  virtual void SetValue(char *v, const char*, int) { 
    delete[] value; value = v; 
  }
  virtual char* GetString() const { return value; }
  virtual void ShowHeader(OutputStream &s) const {
    show(s);
    if (value) s << '"' << value << '"';
    else s << "null";
  }
  virtual void ShowRange(OutputStream &s) const {
    s << "any string\n";
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
  const char* deflt;
  const char* ranges;
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
  action_opt(type t, const char* n, const char* def, 
  	     const char* d, const char* r, 
             action_set s, action_get g)
   : option(t,n,d) { Set = s; Get = g; ranges = r; deflt = def; }
  virtual ~action_opt() { }
  virtual void SetValue(bool x, const char* f, int l) { SetVoid(&x, f, l); }
  virtual void SetValue(int x, const char* f, int l) { SetVoid(&x, f, l); }
  virtual void SetValue(double x, const char* f, int l) { SetVoid(&x, f, l); }
  virtual void SetValue(char* x, const char* f, int l) { SetVoid(&x, f, l); }

  virtual bool GetBool() { bool b; Get(&b); return b; }
  virtual int GetInt() { int b; Get(&b); return b; }
  virtual double GetReal() { double b; Get(&b); return b; }
  virtual char* GetString() { char* b; Get(&b); return b; }

  virtual void ShowHeader(OutputStream &s) const { 
    show(s); 
    s << " " << deflt;
  }
  virtual void ShowRange(OutputStream &s) const { s << ranges << "\n"; }
};

option* MakeActionOption(type t, const char* name, const char* deflt, 
			 const char* doc, 
                         const char* r, action_set s, action_get g)
{
  return new action_opt(t, name, deflt, doc, r, s, g);
}


// **************************************************************************
// *                              enum options                              *
// **************************************************************************

class enum_opt : public option {
  option_const** possible;
  int numpossible;
  const option_const* value;
public:
  enum_opt(const char* n, const char* d, option_const** p, int np, 
  		const option_const* v)
  : option(VOID, n, d) { possible = p; numpossible = np; value = v; }
  virtual ~enum_opt() { delete[] possible; }
  virtual void SetValue(const option_const* v, const char* f, int l) {
    value = v;
  }
  virtual const option_const* GetEnum() const {
    return value;
  }
  virtual void ShowHeader(OutputStream &s) const {
    DCASSERT(value);
    show(s);
    s << " " << value->name;
  }
  // These are a bit more interesting...
  virtual const option_const* FindConstant(const char* name) const;
  virtual void ShowRange(OutputStream &s) const;
};

/** Find the appropriate value for this enumerated name.
    If the name is bad (i.e., not found), we return NULL.
*/
const option_const* enum_opt::FindConstant(const char* name) const
{
  // binary search
  int low = 0;
  int high = numpossible;
  while (low < high) {
    int mid = (low+high)/2;
    int cmp = strcmp(possible[mid]->name, name);
    if (0==cmp) return possible[mid];
    if (cmp>0) {
      high = mid;
    } else {
      low = mid+1;
    }
  }
  // not found
  return NULL;
}

void enum_opt::ShowRange(OutputStream &s) const
{
  s << "\n";
  int i;
  for (i=0; i<numpossible; i++) {
    s << "\t" << possible[i]->name << "\t" << possible[i]->doc << "\n";
  }
}

option* MakeEnumOption(const char* name, const char* doc, 
                       option_const** values, int numv,
		       const option_const* deflt)
{
  DCASSERT(values);
#ifdef DEVELOPMENT_CODE
  // Check that the values are sorted
  DCASSERT(values[0]);
  int i;
  for (i=1; i<numv; i++) {
    DCASSERT(values[i]);
    int cmp = strcmp(values[i]->name, values[i-1]->name);
    if (cmp<=0) {
      Internal.Start(__FILE__, __LINE__);
      Internal << "Enumerated values for option " << name << " are not sorted!\n";
      Internal.Stop();
    }
  }
#endif
  return new enum_opt(name, doc, values, numv, deflt);
}


// **************************************************************************
// *                                                                        *
// *                         Option list management                         *
// *                                                                        *
// **************************************************************************

HeapOfPointers <option> optionlist(128);
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

