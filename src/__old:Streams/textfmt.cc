
#include "textfmt.h"
#include "streams.h"

#include <cstring>

const int item_sep=4;  // separation distance in descriptions

// ==================================================================
// |                                                                |
// |                     doc_formatter  methods                     |
// |                                                                |
// ==================================================================

doc_formatter::doc_formatter()
{
}

doc_formatter::~doc_formatter()
{
}

bool doc_formatter::Matches(const char* item, const char* keyword) const
{
  if (NULL==keyword) return true;
  int slen = strlen(keyword);
  int last = strlen(item) - slen;
  for (int i=0; i<=last; i++) {
    if (0==strncasecmp(item+i, keyword, slen)) return true;
  }
  return false;
}

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
#ifdef OLD_STREAMS
  StringStream buffer;
#else
  std::stringstream buffer;
#endif

  bool in_heading;
  int indent_depth;
  int desc_width;
public:
  text_formatter(int width, OutputStream &out);
#ifdef OLD_STREAMS
  virtual OutputStream& Out() { return buffer; }
#else
  virtual std::ostream& Out() { return buffer; }
#endif

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
#ifdef OLD_STREAMS
  out.Pad(' ', left);
  out.Put(s, -desc_width);
  out.Pad(' ', item_sep);
#else
  Pad(out, ' ', left);
  out << std::setw(-desc_width) << s;
  Pad(out, ' ', item_sep);
#endif
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
#ifdef OLD_STREAMS
  const char* doc = buffer.ReadString();
#else
  std::string doc = buffer.str();
#endif
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
#ifdef OLD_STREAMS
      out.Pad(' ', left);
#else
      Pad(out, ' ', left);
#endif
    }
    //
    // write first word, regardless of length
    int written = 0;
    while (1) {
      if (EndOfWord(doc[ptr]))  break;
      if ('~'==doc[ptr])  out << ' ';
      else                out << doc[ptr];
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
      out << ' ';
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
          if ('~'==doc[ptr]) out << ' ';
          else out << doc[ptr];
          ptr++;
        }
      }
    } // while written
    // That's all we can fit on this line, or it is the last line.
    out << '\n';
  } // while doc[ptr]
  out.flush();
#ifdef OLD_STREAMS
  buffer.flush();
#else
  buffer.str(std::string());    // clear the buffer
#endif
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

