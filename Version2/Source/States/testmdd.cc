
// $Id$

#include "mdds.h"

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
  bar.Dump(Output);
}

void DeleteNode(node_manager &bar)
{
  int p;
  Input.Get(p);
  bar.Unlink(p);
  bar.Dump(Output);
}

void CacheNode(node_manager &bar, int delta)
{
  int p;
  Input.Get(p);
  if (delta>0) bar.CacheAdd(p);
  if (delta<0) bar.CacheRemove(p);
  bar.Dump(Output); 
}

int main()
{
  node_manager bar(GC_Pessimistic);
  Output << "Enter command sequence:\n\ta k sz\tto add a node\n\tu #\tto unlink a node\n\t+ #\tto cache add\n\t- #\tto cache subtract\n\n";
  Output.flush();
  char cmd;
  while (Input.Get(cmd)) {
    switch (cmd) {
      case 'a':	AddNode(bar);	break;

      case 'u': DeleteNode(bar);	break;

      case '+': CacheNode(bar, 1);	break;

      case '-': CacheNode(bar, -1);	break;
        
      default:
	continue;
    }
  }  
  Output << "Later!\n";
  return 0;
}

