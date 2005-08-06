
// $Id$

#include "mdds.h"
#include "mdds_malloc.h"

void smart_exit()
{
}

void AddNode(mdd_node_manager &bar)
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

void DeleteNode(mdd_node_manager &bar)
{
  int p;
  Input.Get(p);
  bar.Unlink(p);
  bar.Dump(Output);
}

int main()
{
  mdd_node_manager bar;
  Output << "Enter command sequence:\n\ta k sz\tto add a node\n\td #\t to delete a node\n";
  Output.flush();
  char cmd;
  while (Input.Get(cmd)) {
    switch (cmd) {
      case 'a':	AddNode(bar);	break;

      case 'd': DeleteNode(bar);	break;
        
      default:
	continue;
    }
  }  
  Output << "Later!\n";
  return 0;
}

