
/*
	Test of the splay tree class.
*/

#include "../Streams/streams.h" 
#include <stdlib.h>
#include <string.h>
// #define DEBUG
#include "../include/splay.h"

io_environ myio;

class myitem {
  const char* word;
public:
  myitem(const char* w) { word = w; }
  const char* Word() const { return word; }
  inline int Compare(const myitem* foo) const {
    return strcmp(word, foo->word);
  }
};

inline int Compare(char* a, char* b)
{
  if ((0==a) && (0==b)) return 0;
  if (0==a) return -1;
  if (0==b) return 1;
  return strcmp(a, b);
}

int main()
{
  DisplayStream& cout = myio.Output;

  cout << "Creating splay tree of strings\n";
  cout.flush();

  SplayOfPointers <myitem> dict(0, 0);

  cout << "Adding words\n";
  cout.flush();

  myitem word01("The");
  myitem word02("quick");
  myitem word03("brown");
  myitem word04("fox");
  myitem word05("jumped");
  myitem word06("over");
  myitem word07("the");
  myitem word08("lazy");
  myitem word09("dogs");
  myitem word10("A");
  myitem word11("man");
  myitem word12("said");

  dict.Insert(&word01);
  dict.Insert(&word02);
  dict.Insert(&word03);
  dict.Insert(&word04);
  dict.Insert(&word05);
  dict.Insert(&word06);
  dict.Insert(&word07);
  dict.Insert(&word08);
  dict.Insert(&word09);
  dict.Insert(&word10);
  dict.Insert(&word11);
  dict.Insert(&word12);

#ifdef DEBUG
  cout << "Current representation:\n";
  cout.flush();

  dict.Show(cout);
#endif

  cout << "Copying to array of strings\n";
  cout.flush();

  long length = dict.NumElements();
  myitem** sorted = new myitem* [length];

  dict.CopyToArray(sorted);

  cout << "Sorted list of words:\n";
  for (long i=0; i<length; i++) {
    cout << "\t" << sorted[i]->Word() << "\n";
  }

  cout << "Done\n";
  cout.flush();
  return 0;
}
