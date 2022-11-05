
#ifndef OPTMAN_H
#define OPTMAN_H

class doc_formatter;   // defined in streams.h
class option;

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

/** Make a new option manager.
      @return A new manager, or NULL on error.
*/
option_manager* MakeOptionManager();



#endif

