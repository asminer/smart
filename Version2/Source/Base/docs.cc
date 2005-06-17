
// $Id$

#include "../defines.h"
#include "docs.h"

#include "../Templates/list.h"

// ******************************************************************
// *                                                                *
// *                       Help topic classes                       *
// *                                                                *
// ******************************************************************

class help_topic {
protected:
  const char* topic;
  // overridden in derived classes
  virtual void Display(OutputStream &disp, int LM, int RM) const = 0;
public:
  help_topic(const char* t) { topic = t; }
  virtual ~help_topic() { }; // don't delete

  void Match(const char* key, OutputStream &d, int LM, int RM) const {
    if (DocMatches(topic, key)) {
      d << "Help topic: " << topic << "\n";
      Display(d, LM, RM);
    }
  }
};

class static_help : public help_topic {
  const char* doc;
public:
  static_help(const char* t, const char* d) : help_topic(t) {
    doc = d;
  }
  virtual void Display(OutputStream &disp, int LM, int RM) const {
    DisplayDocs(disp, doc, LM, RM, false);
  }
};

class dynamic_help : public help_topic {
  Topic_func show;
public:
  dynamic_help(const char* t, Topic_func s) : help_topic(t) {
    show = s;
    DCASSERT(show);
  }
  virtual void Display(OutputStream &disp, int LM, int RM) const {
    DCASSERT(show);
    show(disp, LM, RM);
  }
};

List <help_topic> topics(4);

// ******************************************************************
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// ******************************************************************

void AddTopic(const char* topic, const char* doc)
{
  topics.Append(new static_help(topic, doc));
}

void AddTopic(const char* topic, Topic_func f)
{
  topics.Append(new dynamic_help(topic, f));
}

void MatchTopics(const char* key, OutputStream &display, int LM, int RM)
{
  for (int i=0; i<topics.Length(); i++) {
    topics.Item(i)->Match(key, display, LM, RM);
  }
}


// ******************************************************************
// *                                                                *
// *                       Display  functions                       *
// *                                                                *
// ******************************************************************

void DisplayDocs(OutputStream &disp, const char* doc, int LM, int RM, bool rule)
{
  int docwidth = RM - LM;
  int ptr = 0;
  bool left_pad = true;
  if ('\b' == doc[0]) {
    // "backspace": no initial newline, don't break the first line
    for (ptr=1; doc[ptr]!='\n'; ptr++) {
      DCASSERT(doc[ptr]);
      disp.Put(doc[ptr]);
    }
    ptr++;
    disp.Put('\n');
  } else if ('\a' == doc[0]) {
    // no initial newline 
    ptr++;
    left_pad = false; 
  } else {
    // default: newline before documentation
    disp.Put('\n');
  }
  if (rule) {
    disp.Pad(' ', LM);
    disp.Pad('-', docwidth);
    disp.Put('\n');
  }
  while (doc[ptr]) {
    // start of a line, skip spaces (but not tabs or newlines)
    while (doc[ptr]==' ') ptr++;
    if (0==doc[ptr]) break;

    if (left_pad) disp.Pad(' ', LM);
    left_pad = true;

    // write first word
    int written = 0;
    while (1) {
      if (doc[ptr]==' ' || doc[ptr]=='\n' || doc[ptr]==0)
        break;
      disp.Put(doc[ptr]);
      written++;
      ptr++;
    }

    // continue writing words until we exceed the line width or hit newline
    while (written < docwidth) {
      if (doc[ptr]=='\n' || doc[ptr]==0) break;

      // print inter-word space
      disp.Put(' ');
      written++;

      // get to start of next word
      while (doc[ptr]==' ') ptr++;

      // count spaces of next word
      int ns = ptr;
      while (1) {
        if (doc[ns]==' ' || doc[ns]=='\n' || doc[ns]==0) break;
        ns++;
        written++;
      }
      if (written < docwidth) {
        // write the next word
        while (1) {
          if (doc[ptr]==' ' || doc[ptr]=='\n' || doc[ptr]==0)
            break;
          disp.Put(doc[ptr]);
          ptr++;
        }
      }
    } // while written

    // That's all we can fit on this line, or it is the last line.
    disp.Put('\n');
    if (doc[ptr]=='\n') ptr++;
  } // while doc[ptr]
}

// ******************************************************************
// *                                                                *
// *                       Matching functions                       *
// *                                                                *
// ******************************************************************

bool DocMatches(const char* doc, const char* srch)
{
  if (NULL==srch) return true; 
  int slen = strlen(srch);
  int last = strlen(doc) - slen;
  for (int i=0; i<=last; i++) {
    if (0==strncasecmp(doc+i, srch, slen)) return true;
  }
  return false;
}


