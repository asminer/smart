
// $Id$

#include "mdds.h"
#include "mdd_ops.h"

void ReadMDD(node_manager &bar, InputStream &in)
{
  // File format is:
  // index
  // level
  // truncated--full size
  // entry1 ... entryn

  int mapsize = 1024;
  int* indexmap = (int*) malloc(mapsize*sizeof(int));
  indexmap[0] = 0;
  indexmap[1] = 1;
  
  int index;
  while (in.Get(index)) {
    int k;
    int sz;
    in.Get(k);
    in.Get(sz);
    int n = bar.TempNode(k, sz);
    if (index>=mapsize) {
      mapsize = (1+index/1024)*1024;
      indexmap = (int *) realloc(indexmap, mapsize*sizeof(int));
      DCASSERT(indexmap);
    }
    // read, translate, and store the pointers
    for (int i=0; i<sz; i++) {
      int ptr;
      in.Get(ptr);
      bar.Link(indexmap[ptr]);
      bar.SetArc(n, i, indexmap[ptr]);
    }
    indexmap[index] = bar.Reduce(n);
  }
  free(indexmap);
}


void smart_exit()
{
}

void AddNode(node_manager &bar)
{
  int k;
  int p;
  Input.Get(k);
  Input.Get(p);
  int a = bar.TempNode(k, p);

  Output << "Enter " << p << " pointers\n";
  Output.flush();
  for (int i=0; i<p; i++) {
    int d;
    Input.Get(d);
    bar.SetArc(a, i, d);
  }
  a = bar.Reduce(a);
  Output << "That is node " << a << "\n";
}

void DeleteNode(node_manager &bar)
{
  int p;
  Input.Get(p);
  bar.Unlink(p);
}

void ReadMDD(node_manager &bar)
{
  char filename[50];
  // skip whitespace
  while (1) {
    if (!Input.Get(filename[0])) return;
    if (' ' == filename[0]) continue;
    if ('\t' == filename[0]) continue;
    if ('\n' == filename[0]) continue;
    break;
  }
  int p = 1;
  while (1) {
    if (!Input.Get(filename[p])) break;
    if (' ' == filename[p]) break;
    if ('\t' == filename[p]) break;
    if ('\n' == filename[p]) break;
    p++;
  }
  filename[p] = 0; 

  FILE* foo = fopen(filename, "r"); 
  if (NULL==foo) {
    Output << "Couldn't open file " << filename << "\n"; 
    Output.flush();
    return;
  }
  InputStream reader(foo);
  ReadMDD(bar, reader);
}

void Union(operations &bar)
{
  int p, q, r;
  Input.Get(p);
  Input.Get(q);
  r = bar.Union(p, q); 
  Output << "Union of " << p << " and " << q << " is " << r << "\n";
  Output << "Union cache: " << bar.Uhits() << " hits / " << bar.Upings() << " pings\n";
}

int main()
{
  node_manager bar;
  operations cruft(&bar);
  Output << "Enter command sequence:\n\ta k sz\tto add a node\n\td #\t to delete a node\n\tr file\tto read mdd from file\n";
  Output <<"\n\tu p1 p2\tCompute set union\n";
  Output.flush();
  char cmd;
  while (Input.Get(cmd)) {
    switch (cmd) {
      case 'a':	AddNode(bar);	break;

      case 'd': DeleteNode(bar);	break;
        
      case 'r': ReadMDD(bar);	break;

      case 'u': Union(cruft);	break;

      default:
	continue;
    }
    bar.Dump(Output);
  }  
  Output << "Later!\n";
  return 0;
}

