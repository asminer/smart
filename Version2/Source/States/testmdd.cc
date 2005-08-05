
// $Id$

#include "mdds.h"

void smart_exit()
{
}

int main()
{
  node_manager bar;
  Output << "Enter command sequence:\n\ta k sz\tto add a node\n\td #\t to delete a node\n";
  Output.flush();
  char cmd;
  int k, p;
  while (Input.Get(cmd)) {
    switch (cmd) {
      case 'a':
   	Input.Get(k);
	Input.Get(p);
	bar.TempNode(k, p);
	bar.Dump(Output);
	break;

      case 'd':
        Input.Get(p);
        bar.Unlink(p);
	bar.Dump(Output);
	break;
        
      default:
	continue;
    }
  }  
  Output << "Later!\n";
  return 0;
}

