
// $Id$

#include "../defines.h"
#include "docs.h"

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
