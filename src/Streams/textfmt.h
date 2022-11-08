
#ifndef TEXTFMT_H
#define TEXTFMT_H

#include "streams.h"

/** Centralized documentation formatting class.

    Derive a class from this one, to have documentation
    formatted in different, fancy ways (e.g., html, LaTeX).

    The main idea is to dump strings into the formatter,
    using the comfortable OutputStream interface,
    and it will automatically wrap lines around, and
    produce generally pleasing results.

    Future additions, if necessary:
      A tabular environment?
      Font changes?
*/
class doc_formatter {
public:
  doc_formatter();
  virtual ~doc_formatter();

  /// For writing text.
  virtual OutputStream& Out() = 0;

  /// Start a new section, with given name.
  virtual void section(const char* name) = 0;

  /** Signifies the start of a heading.
      This should be called when starting to display
      an "entry", e.g., a function header.
  */
  virtual void begin_heading() = 0;

  /// Signifies the end of a heading.
  virtual void end_heading() = 0;

  /// Increase level of indentation.
  virtual void begin_indent() = 0;

  /// Decrease level of indentation.
  virtual void end_indent() = 0;

  /** Start a "description" environment.
      This is a list of the form:
        item_1  description of item 1
        item_2  description of item 2, possibly so long that
                it is forced to wrap around.

        @param  width  Width (in chars) of the widest "item".
  */
  virtual void begin_description(int width) = 0;

  /** Set the next item in the list.
      Does nothing if we are not within a description environment.
        @param  str  The next item.
  */
  virtual void item(const char* str) = 0;

  /// end a "description" environment.
  virtual void end_description() = 0;

  /// eject the current "page" of text.
  virtual void eject_page() = 0;

  // Not strictly documentation formatting, but it needs to go somewhere...

  /** Determine if a string matches a keyword (for searches).
      Default behavior: check if the string contains the keyword
      (ignoring case).
      Derive a class from this and override if you want other behavior.
  */
  virtual bool Matches(const char* str, const char* keyword) const;
};


doc_formatter* MakeTextFormatter(int width, OutputStream &out);

#endif
