
// $Id$

/** \file options.h

    New option interface.
*/

#ifndef OPTIONS_H
#define OPTIONS_H

class OutputStream;  // defined in streams.h
class doc_formatter;   // defined in streams.h
class option_manager;

// **************************************************************************
// *                         option_const interface                         *
// **************************************************************************

/** Abstract base class for option constants.
    Used by Radio button and checkbox type options.
    Neat trick!  We can have suboptions for these now!
*/
class option_const {
private:
  /// Name of the constant.
  const char* name;
  /// Documentation.
  const char* doc;
protected:
  /// Settings for this button
  const option_manager* settings;
public:
  option_const(const char* n, const char* d);
  virtual ~option_const();

  inline const char* Name() const { return name; }
  inline const char* Documentation() const { return doc; }

  void show(OutputStream &s) const;

  int Compare(const option_const* b) const;
  int Compare(const char* name) const;

  inline const option_manager* readSettings() const { return settings; }
  inline void makeSettings(const option_manager* s) { settings = s; }

  bool isApropos(const doc_formatter* df, const char* keyword) const;
};

// **************************************************************************
// *                           radio_button class                           *
// **************************************************************************

/** Radio button.
    If a "customized" radio button option is needed, derive the radio
    button items from this class and provide an AssignToMe()
    method.
*/
class radio_button : public option_const {
  int index;
public:
  radio_button(const char* n, const char* d, int i);
  inline int GetIndex() const { return index; }
  /** Called when this radio button is selected.
        @return  true on success, false otherwise.
  */
  virtual bool AssignToMe();
};

// **************************************************************************
// *                         checklist_const  class                         *
// **************************************************************************

/** Constant in a checklist.
    Can be an actual item, or a virtual item (e.g., a group of items).
*/
class checklist_const : public option_const {
public:
  checklist_const(const char* n, const char* d);

  /** Called when this item is checked in a checklist.
        @return  true on success, false otherwise.
  */
  virtual bool CheckMe() = 0;

  /** Called when this item is unchecked in a checklist.
        @return  true on success, false otherwise.
  */
  virtual bool UncheckMe() = 0;

  /// Returns true iff this is a checked item in a checklist.
  virtual bool IsChecked() const = 0;
};

// **************************************************************************
// *                            option interface                            *
// **************************************************************************

/**  Base class for options.
     Derived classes are "hidden" in options.cc.
*/
class option {
public:
  /// Errors for options.
  enum error {
    /// The operation was successful.
    Success = 0,
    /// Type mismatch error.
    WrongType,
    /// Tried to set a value out of range.
    RangeError,
    /// Null action set/get function or pointer.
    NullFunction,
    /// Option is already finalized.
    Finalized,
    /// Duplicate checklist item.
    Duplicate
  };
  /// Types for options.
  enum type {
    /// Dummy type
    NoType = 0,
    /// Boolean options.
    Boolean,
    /// Integer options.
    Integer,
    /// Real options.
    Real,
    /// String options.
    String,
    /// Radio button options.
    RadioButton,
    /// Checklist options.
    Checklist
  };
private:
  type mytype;
  const char* name;
  const char* documentation;
  bool hidden;
public:
  /** Constructor.
        @param  t  The type of option.
        @param  n  The option name.
        @param  d  Documentation for the option.
  */
  option(type t, const char* n, const char* d);
  virtual ~option();
  inline type Type() const { return mytype; }
  inline const char* Name() const { return name; }
  inline const char* GetDocumentation() const { return documentation; }
  inline bool IsUndocumented() const { return hidden; }

  inline void Hide() { hidden = true; }

  void show(OutputStream &s) const;

  /** Set the value for a boolean option.
        @param  b  Value to set.
        @return Appropriate error code.
  */
  virtual error SetValue(bool b);

  /** Set the value for an integer option.
        @param  n  Value to set.
        @return Appropriate error code.
  */
  virtual error SetValue(long n);

  /** Set the value for a real option.
        @param  r  Value to set.
        @return Appropriate error code.
  */
  virtual error SetValue(double r);

  /** Set the value for a string option.
        @param  c  Value to set.
        @return Appropriate error code.
  */
  virtual error SetValue(char *c);

  /** Set the value for a radio button option.
        @param  c  Value to set.
        @return Appropriate error code.
  */
  virtual error SetValue(radio_button* c); 

  /** Find the option constant for this option,
      with the specified name.
      If this is not a RadioButton or CheckList option,
      we always return 0.
        @param  name  Name of option constant to find.
        @return An option constant "owned" by this option
                with the given name, if it exists; 0 otherwise.
  */
  virtual option_const* FindConstant(const char* name) const;

  /** Get the value for a boolean option.
        @param  v  Value stored here.
        @return Appropriate error code.
  */
  virtual error GetValue(bool &v) const;

  /** Get the value for an integer option.
        @param  v  Value stored here.
        @return Appropriate error code.
  */
  virtual error GetValue(long &v) const;

  /** Get the value for a real option.
        @param  v  Value stored here.
        @return Appropriate error code.
  */
  virtual error GetValue(double &v) const;

  /** Get the value for a string option.
        @param  v  Value stored here.
        @return Appropriate error code.
  */
  virtual error GetValue(const char* &v) const;

  /** Get the value for a radio button option.
        @param  v  Value stored here.
        @return Appropriate error code.
  */
  virtual error GetValue(const radio_button* &v) const;

  virtual int NumConstants() const;
  virtual option_const* GetConstant(long i) const;

  virtual void ShowHeader(OutputStream &s) const = 0;
  virtual void ShowCurrent(OutputStream &s) const;
  virtual void ShowRange(doc_formatter* df) const = 0;

  /** Add a new item to a checklist option.
        @param  v  The new checklist item.
        @return Appropriate error code.
  */
  virtual error AddCheckItem(checklist_const* v);

  /// Will be called when the option list is finalized.
  virtual void Finish();

  int Compare(const option* b) const;
  int Compare(const char* name) const;

  /// Determine if this option matches the given keyword.
  virtual bool isApropos(const doc_formatter* df, const char* keyword) const;

  /** Write documentation header and body for this option.
        @param  df  Document formatter; output is sent here.
  */
  void PrintDocs(doc_formatter* df, const char* keyword) const;

  /// Recursively document children as appropriate.
  virtual void RecurseDocs(doc_formatter* df, const char* keyword) const;
};

// **************************************************************************
// *                        custom_option  interface                        *
// **************************************************************************

/** If you want setting / getting option values to trigger an action,
    derive a class from this one and use it for your option.
    You must provide the appropriate SetValue() and GetValue() methods.
*/
class custom_option : public option {
  const char* range;
public:
  custom_option(type t, const char* name, const char* doc, const char* range);

  virtual ~custom_option();

  virtual void ShowHeader(OutputStream &s) const;
  virtual void ShowRange(doc_formatter* df) const;
  virtual bool isApropos(const doc_formatter* df, const char* keyword) const;
};

// **************************************************************************
// *                        option_manager interface                        *
// **************************************************************************

/** Option manager (abstract) class.
    A centralized collection of options, with operations for
    addition of options (during initialization) and
    retrieval of optiions (during computation).
*/
class option_manager {
public:
  option_manager();
  virtual ~option_manager();
  /// Add a new option to this collection.
  virtual void AddOption(option *) = 0;
  /** Called when initialization is complete.
      After this is called, no new options may be added.
  */
  virtual void DoneAddingOptions() = 0;
  /** Find an option with matching name.
        @param  name  Name of the desired option.
        @return The matching option, or NULL if none present with \a name.
  */
  virtual option* FindOption(const char* name) const = 0;
  /// Total number of options.
  virtual long NumOptions() const = 0;
  /** Retrieve an option by index.
      This is useful for enumerating all options.
        @param  i  Index of option to retrieve.
        @return The ith option.
  */
  virtual option* GetOptionNumber(long i) const = 0;

  /** For online help and documentation.
      Show documentation of all options matching the given keyword.
  */
  virtual void DocumentOptions(doc_formatter* df, const char* keyword) const = 0;

  /** For online help and documentation.
      List all options, with their current settings.
  */
  virtual void ListOptions(doc_formatter* df) const = 0;
};

// **************************************************************************
// *                            Global interface                            *
// **************************************************************************


/** Make a new option of type "radio button".
      @param  name      The option name
      @param  doc       Documentation for the option.
      @param  values    Possible values the option can assume
      @param  numvalues Number of values.
      @param  link      Link to current selection (the value index).
      @return A new option, or NULL on error.
              An error will occur if the values are not sorted!
*/
option* MakeRadioOption(const char* name, const char* doc, 
           radio_button** values, int numvalues,
           int& link);


/** Make a new option constant for a checklist.
      @param  name  The constant name
      @param  doc   Documentation for the option.
      @param  link  Link to "are we checked or not".
      @return A new constant, or NULL on error.
*/
checklist_const* MakeChecklistConstant(const char* name, const char* doc, bool &link);

/** Make a new option constant for a group of checklist items.
      @param  name  The constant name
      @param  doc   Documentation for the option.
      @param  items Items that this group contains.
      @param  ni    Dimension of items array.
      @return A new constant, or NULL on error.
*/
checklist_const* MakeChecklistGroup(const char* name, const char* doc, checklist_const** items, int ni);

/** Make a new option of type "checklist".
    Possible items on the list are added dynamically.
      @param  name  The option name
      @param  doc   Documentation for the option.
      @return A new option, or NULL on error.
*/
option* MakeChecklistOption(const char* name, const char* doc);

/** Make a new option of type boolean.
      @param  name  The option name
      @param  doc   Documentation for the option.
      @param  link  Link to the boolean value; value can be
                    changed by the option or otherwise.
      @return A new option, or NULL on error.
*/
option* MakeBoolOption(const char* name, const char* doc, bool &link);

/** Make a new option of type integer.
      @param  name  The option name
      @param  doc   Documentation for the option.
      @param  link  Link to the value; value can be changed by
                    the option or otherwise.
      @param  min   The minimum allowed value.
      @param  max   The maximum allowed value.
                    Use min > max for all possible integers.
      @return A new option, or NULL on error.
*/
option* MakeIntOption(const char* name, const char* doc,
      long &link, long min, long max);


/** Make a new option of type real.
      @param  name            The option name
      @param  doc             Documentation for the option.
      @param  link            Link to the value; value can be changed by
                              the option or otherwise.
      @param  has_lower       Does the option have a lower bound?
                              If not, ignore the lower bound parameters.
      @param  includes_lower  Is the lower bound included in the
                              set of possible values?
      @param  lower           Lower bound.
      @param  has_upper       Does the option have an upper bound?
                              If not, ignore the upper bound parameters.
      @param  includes_upper  Is the upper bound included in the
                              set of possible values?
      @param  upper           Upper bound.
      @return A new option, or NULL on error.
*/
option* MakeRealOption(const char* name, const char* doc, double &link, 
      bool has_lower, bool includes_lower, double lower,
      bool has_upper, bool includes_upper, double upper);


/** Make a new option of type string.
      @param  name  The option name.
      @param  doc   Documentation for the option.
      @param  link  Link to the value.
      @return  A new option, or NULL on error.
*/
option* MakeStringOption(const char* name, const char* doc, char* &link);



/** Make a new option manager.
      @return A new manager, or NULL on error.
*/
option_manager* MakeOptionManager();



#endif

