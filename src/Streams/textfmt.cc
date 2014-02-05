
// $Id$

#include "textfmt.h"
#include "streams.h"

const int item_sep=4;  // separation distance in descriptions

// ******************************************************************
// *                                                                *
// *                      text_formatter class                      *
// *                                                                *
// ******************************************************************

class text_formatter : public doc_formatter {
  int pagewidth;
  int left;
  int right;
  OutputStream& out;
  StringStream buffer;

  bool in_heading;
  int indent_depth;
  int desc_width;
public:
  text_formatter(int width, OutputStream &out);

  virtual OutputStream& Out() { return buffer; }

  virtual void section(const char* name);
  virtual void begin_heading();
  virtual void end_heading();
  virtual void begin_indent();
  virtual void end_indent();
  virtual void begin_description(int w);
  virtual void item(const char* s);
  virtual void end_description();
  virtual void eject_page();

  void FlushText();
protected:
  inline bool EndOfWord(char c) {
    return (c==0) || (c==' ') || (c=='\n');
  }
};

// ******************************************************************
// *                     text_formatter methods                     *
// ******************************************************************

text_formatter::text_formatter(int width, OutputStream &o)
 : out(o)
{
  pagewidth = width;
  left = 0;
  right = width-1;
  in_heading = 0;
  indent_depth = 0;
  desc_width = 0;
}

void text_formatter::section(const char* name)
{
  FlushText();
  buffer << name;
  FlushText();
}

void text_formatter::begin_heading()
{
  FlushText();
  in_heading = true;
}

void text_formatter::end_heading()
{
  FlushText();
  in_heading = false;
}

void text_formatter::begin_indent()
{
  FlushText();
  indent_depth++;
  left += 4;
  right -= 4; 
}

void text_formatter::end_indent()
{
  FlushText();
  if (0==indent_depth)  return;
  indent_depth--;
  left -= 4;
  right += 4;
}

void text_formatter::begin_description(int w)
{
  FlushText();
  desc_width = w; 
}

void text_formatter::item(const char* s)
{
  int old_left = left;
  left += desc_width+item_sep;
  FlushText();
  left = old_left;
  out.Pad(' ', left);
  out.Put(s, -desc_width);
  out.Pad(' ', item_sep);
}

void text_formatter::end_description()
{
  int old_left = left;
  left += desc_width+item_sep;
  FlushText();
  left = old_left;
  desc_width = 0;
}

void text_formatter::eject_page()
{
  FlushText();
}

void text_formatter::FlushText()
{
  const char* doc = buffer.ReadString();
  int ptr = 0;
  int linewidth = right - left;
  bool ignore_marg = desc_width;
  while (doc[ptr]) {
    // We are at the start of a line, skip whitespace (but not newlines)
    if (' ' == doc[ptr]) {
      ptr++;
      continue;
    }
    // margin, except the first line of a description
    if (ignore_marg) {
      ignore_marg = false;
    } else {
      out.Pad(' ', left);
    }
    //
    // write first word, regardless of length
    int written = 0;
    while (1) {
      if (EndOfWord(doc[ptr]))  break;
      if ('~'==doc[ptr])  out.Put(' ');
      else                out.Put(doc[ptr]);
      written++;
      ptr++;
    }
    //
    // continue writing, one word at a time, until either:
    // () we exceed the line width, or
    // () we hit a newline, or
    // () we reach the end of input.
    while (written < linewidth) {
      if (0==doc[ptr])    break;
      if ('\n'==doc[ptr]) {
        ptr++;
        break;
      }
      // print inter-word space
      out.Put(' ');
      written++;
      // advance to start of next word
      while (doc[ptr]==' ') ptr++;
      // count spaces of next word
      int ns = ptr;
      while (1) {
        if (EndOfWord(doc[ns])) break;
        ns++;
        written++;
      }
      if (written < linewidth) {
        // write the next word
        while (1) {
          if (EndOfWord(doc[ptr])) break;
          if ('~'==doc[ptr]) out.Put(' ');
          else out.Put(doc[ptr]);
          ptr++;
        }
      }
    } // while written
    // That's all we can fit on this line, or it is the last line.
    out.Put('\n');
  } // while doc[ptr]
  out.flush();
  buffer.flush(); 
}


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

doc_formatter* MakeTextFormatter(int width, OutputStream &out)
{
  return new text_formatter(width, out);
}

