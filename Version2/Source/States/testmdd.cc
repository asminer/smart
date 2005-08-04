
// $Id$

#include "mdds.h"

void smart_exit()
{
}

int main()
{
  node_manager bar;
  Output << "Enter command sequence:\n\ta\tto add a node\n\td #\t to delete a node\n";
  Output.flush();
  char cmd;
  int p;
  while (Input.Get(cmd)) {
    switch (cmd) {
      case 'a':
	bar.TempNode(0, 0);
	bar.Dump(Output);
	break;

      case 'd':
        Input.Get(p);
        bar.FreeNode(p);
	bar.Dump(Output);
	break;
        
      default:
	continue;
    }
  }  
  Output << "Later!\n";
  return 0;
}

